#include "session.hpp"

int kcp_output_callback(const char* buf, int len, ikcpcb* /*kcp*/, void* user)
{
	auto session = static_cast<NetworkSession*>(user);
	if (!session || !session->send_fn)
		return -1;

	session->send_fn(session->address, buf, len);
	return 0;
}

void NetworkSessionManager::SetSender(std::function<void(const asio::ip::udp::endpoint&, const char*, int)> fn)
{
	std::lock_guard<std::mutex> lock(mutex_);
	send_fn_ = std::move(fn);
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
    }

    session->playerid = playerid;
    session->send_fn = send_fn_;
    session->handshake_status = HandshakeStatus::NONE;
    session->handshake_complete = false;
    session->cef_init_notified = false;
    session->cef_success = false;
    session->cef_init_timer_started = false;
    session->is_download_paused = false;
}

void NetworkSessionManager::RemovePlayer(int playerid)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = player_sessions_.find(playerid);
    if (it != player_sessions_.end()) {
        if (it->second->handshake_status != HandshakeStatus::NONE) {
            UnmapAddress(it->second->address);
        }

        if (it->second->kcp_instance) {
            ikcp_release(it->second->kcp_instance);
            it->second->kcp_instance = nullptr;
        }

        player_sessions_.erase(it);
    }
}

void NetworkSessionManager::UpdateAllKcpInstances(uint32_t now_ms)
{
    std::vector<std::shared_ptr<NetworkSession>> sessions;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions.reserve(player_sessions_.size());

        for (auto& [playerid, session] : player_sessions_) {
            if (session && session->kcp_instance &&
                session->handshake_status == HandshakeStatus::CONNECTED) {
                sessions.push_back(session);
            }
        }
    }

    for (auto& session : sessions) {
        std::lock_guard<std::mutex> lock(session->kcp_mutex);
        if (session->kcp_instance &&
            session->handshake_status == HandshakeStatus::CONNECTED) {
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
        session->handshake_status = HandshakeStatus::NONE;
        session->handshake_complete = false;
        session->cef_init_notified = false;
        session->cef_success = false;
        session->cef_init_timer_started = false;
        session->is_download_paused = false;

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
