#include "session.hpp"

int kcp_output_callback(const char* buf, int len, ikcpcb* /*kcp*/, void* user)
{
	auto session = static_cast<NetworkSession*>(user);
	if (!session || !session->send_fn)
		return -1;

	session->send_fn(session->address, buf, len);
	return 0;
}

void NetworkSession::ClearDownloadState()
{
	std::queue<std::shared_ptr<FileTransfer>> empty_queue;
	download_queue.swap(empty_queue);
	current_transfer = nullptr;
	is_download_paused.store(false);
}

void NetworkSession::ReleaseKcp()
{
	std::lock_guard<std::mutex> kcp_lock(kcp_mutex);
	if (kcp_instance)
	{
		ikcp_release(kcp_instance);
		kcp_instance = nullptr;
	}
}

void NetworkSession::Reset()
{
	ReleaseKcp();

	address = asio::ip::udp::endpoint{};
	handshake_status = HandshakeStatus::NONE;
	handshake_complete.store(false);
	cef_init_timer_started.store(false);
	cef_init_notified = false;
	cef_success = false;
	rx_key.clear();
	tx_key.clear();
	chat_input_open = false;

	ClearDownloadState();
}

void NetworkSessionManager::SetSender(std::function<void(const asio::ip::udp::endpoint&, const char*, int)> fn)
{
	std::lock_guard<std::mutex> lock(mutex_);
	send_fn_ = std::move(fn);
}

void NetworkSessionManager::TrackClosedEndpoint(const asio::ip::udp::endpoint& addr)
{
	if (addr.address().is_unspecified() || addr.port() == 0)
		return;

	closed_endpoint_expirations_[EndpointToStr(addr)] = std::chrono::steady_clock::now() + CLOSED_ENDPOINT_RETENTION;
}

void NetworkSessionManager::RemoveExpiredClosedEndpoints()
{
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

	for (auto it = closed_endpoint_expirations_.begin(); it != closed_endpoint_expirations_.end();)
	{
		if (it->second <= now)
			it = closed_endpoint_expirations_.erase(it);
		else
			++it;
	}
}

void NetworkSessionManager::RegisterPlayer(int playerid)
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::shared_ptr<NetworkSession> session;

	auto it = player_sessions_.find(playerid);
	if (it == player_sessions_.end())
	{
		session = std::make_shared<NetworkSession>();
		player_sessions_[playerid] = session;
	}
	else
	{
		session = it->second;
		if (session->handshake_status != HandshakeStatus::NONE)
			UnmapAddress(session->address);
	}

	session->playerid = playerid;
	session->send_fn = send_fn_;
	session->Reset();
}

void NetworkSessionManager::RemovePlayer(int playerid)
{
	std::lock_guard<std::mutex> lock(mutex_);

	RemoveExpiredClosedEndpoints();

	auto it = player_sessions_.find(playerid);
	if (it != player_sessions_.end())
	{
		TrackClosedEndpoint(it->second->address);

		if (it->second->handshake_status != HandshakeStatus::NONE)
			UnmapAddress(it->second->address);

		it->second->Reset();
		player_sessions_.erase(it);
	}
}

void NetworkSessionManager::ResetPlayerTransport(int playerid)
{
	std::lock_guard<std::mutex> lock(mutex_);

	RemoveExpiredClosedEndpoints();

	auto it = player_sessions_.find(playerid);
	if (it == player_sessions_.end() || !it->second)
		return;

	auto& session = *it->second;

	TrackClosedEndpoint(session.address);

	if (session.handshake_status != HandshakeStatus::NONE)
		UnmapAddress(session.address);

	session.Reset();
	session.playerid = playerid;
	session.send_fn = send_fn_;
}

bool NetworkSessionManager::IsEndpointRecentlyClosed(const asio::ip::udp::endpoint& addr)
{
	std::lock_guard<std::mutex> lock(mutex_);

	RemoveExpiredClosedEndpoints();
	return closed_endpoint_expirations_.find(EndpointToStr(addr)) != closed_endpoint_expirations_.end();
}

void NetworkSessionManager::ClearClosedEndpoint(const asio::ip::udp::endpoint& addr)
{
	std::lock_guard<std::mutex> lock(mutex_);
	closed_endpoint_expirations_.erase(EndpointToStr(addr));
}

void NetworkSessionManager::UpdateAllKcpInstances(uint32_t now_ms)
{
	std::vector<std::shared_ptr<NetworkSession>> sessions;

	{
		std::lock_guard<std::mutex> lock(mutex_);
		sessions.reserve(player_sessions_.size());

		for (auto& [playerid, session] : player_sessions_)
		{
			if (session && session->kcp_instance && session->handshake_status == HandshakeStatus::CONNECTED)
				sessions.push_back(session);
		}
	}

	for (auto& session : sessions)
	{
		std::lock_guard<std::mutex> lock(session->kcp_mutex);
		if (session->kcp_instance && session->handshake_status == HandshakeStatus::CONNECTED) {
			ikcp_update(session->kcp_instance, now_ms);
		}
	}
}

std::shared_ptr<NetworkSession> NetworkSessionManager::GetOrCreateSession(int playerid)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto it = player_sessions_.find(playerid);
	if (it == player_sessions_.end())
	{
		auto session = std::make_shared<NetworkSession>();

		session->playerid = playerid;
		session->send_fn = send_fn_;
		session->Reset();

		player_sessions_[playerid] = session;

		return session;
	}

	return it->second;
}

std::shared_ptr<NetworkSession> NetworkSessionManager::GetSessionFromAddress(const asio::ip::udp::endpoint& addr)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto it_pid = addr_str_to_playerid_.find(EndpointToStr(addr));
	if (it_pid != addr_str_to_playerid_.end())
	{
		int playerid = it_pid->second;
		auto it_session = player_sessions_.find(playerid);
		if (it_session != player_sessions_.end()) 
		{
			return it_session->second;
		}
	}

	return nullptr;
}

std::shared_ptr<NetworkSession> NetworkSessionManager::GetSession(int playerid)
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto it = player_sessions_.find(playerid);
	if (it != player_sessions_.end()) {
		return it->second;
	}

	return nullptr;
}

std::vector<std::shared_ptr<NetworkSession>> NetworkSessionManager::GetAllSessions()
{
	std::lock_guard<std::mutex> lock(mutex_);

	std::vector<std::shared_ptr<NetworkSession>> result;
	result.reserve(player_sessions_.size());

	for (const auto& [playerid, session] : player_sessions_)
	{
		if (session && session->handshake_complete) 
		{
			result.push_back(session);
		}
	}

	return result;
}

bool NetworkSessionManager::HasPlayerPlugin(int playerid) const
{
    std::lock_guard<std::mutex> lock(mutex_);

	auto it = player_sessions_.find(playerid);
    if (it != player_sessions_.end()) {
		return it->second->handshake_complete;
    }

    return false;
}

void NetworkSessionManager::MapAddressToPlayer(int playerid, const asio::ip::udp::endpoint& addr)
{
	std::lock_guard<std::mutex> lock(mutex_);

    RemoveExpiredClosedEndpoints();
    closed_endpoint_expirations_.erase(EndpointToStr(addr));

    auto it = player_sessions_.find(playerid);
    if (it != player_sessions_.end())
    {
        auto& session = it->second;
        if (session->handshake_status != HandshakeStatus::NONE && session->address != addr)
        {
            // Remove old mapping if it exists
            addr_str_to_playerid_.erase(EndpointToStr(session->address));
        }
    }

	addr_str_to_playerid_[EndpointToStr(addr)] = playerid;
}

void NetworkSessionManager::SetDownloadPaused(int playerid, bool paused) 
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = player_sessions_.find(playerid);
    if (it != player_sessions_.end()) {
        it->second->is_download_paused = paused;
    }
}

void NetworkSessionManager::UnmapAddress(const asio::ip::udp::endpoint& addr)
{
    addr_str_to_playerid_.erase(EndpointToStr(addr));
}

std::string NetworkSessionManager::EndpointToStr(const asio::ip::udp::endpoint& addr) const
{
	return addr.address().to_string() + ":" + std::to_string(addr.port());
}
