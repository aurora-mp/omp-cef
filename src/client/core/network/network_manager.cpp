#include "network_manager.hpp"
#include "shared/packet-serializer.hpp"
#include "shared/crypto.hpp"
#include "shared/utils.hpp"
#include "system/security_manager.hpp"
#include "system/resource_manager.hpp"
#include "system/logger.hpp"
#include "samp/samp.hpp"
#include "samp/components/netgame.hpp"
#include "samp/hooks/netgame.hpp"
#include "shared/events.hpp"
#include "shared/version.hpp"

constexpr int CONNECT_RETRY_INTERVAL_MS = 2000;
constexpr int KCP_UPDATE_INTERVAL_MS = 10;

static int kcp_client_output_callback(const char* buf, int len, ikcpcb* /*kcp*/, void* user)
{
	auto* self = static_cast<NetworkManager*>(user);
	self->SendRaw(buf, len);
	return 0;
}

NetworkManager::NetworkManager(ResourceManager& resource) : resource_(resource), socket_(io_context_), connect_timer_(io_context_), kcp_update_timer_(io_context_)
{

}

NetworkManager::~NetworkManager()
{
	Shutdown();
}

void NetworkManager::SetSessionActiveHandler(SessionActiveHandler handler)
{
    std::lock_guard<std::mutex> lock(session_mutex_);
    session_handler_ = std::move(handler);
}

bool NetworkManager::Initialize(const std::string& ip, unsigned short port)
{
	try {
		SecurityManager::Initialize();
		asio::ip::udp::resolver resolver(io_context_);
		server_endpoint_ = *resolver.resolve(asio::ip::udp::v4(), ip, std::to_string(port)).begin();
	}
	catch (const std::exception& e) {
		LOG_ERROR("[NetworkManager] Initialization failed: {}", e.what());
		return false;
	}

	non_cef_server_.store(false);
	FireSessionActive(false);

	LOG_INFO("[NetworkManager] Initialized for server at {}:{}", server_endpoint_.address().to_string().c_str(), server_endpoint_.port());
	return true;
}

void NetworkManager::Shutdown()
{
	Disconnect();

	io_context_.stop();

	if (network_thread_.joinable()) {
		network_thread_.join();
	}
}

void NetworkManager::Connect(int playerid)
{
    LOG_INFO("[CLIENT] Connect() called with playerid={}", playerid);
    ConnectionState expected = ConnectionState::DISCONNECTED;
    if (!state_.compare_exchange_strong(expected, ConnectionState::SENDING_JOIN, std::memory_order_acq_rel, std::memory_order_acquire))
    {
        LOG_WARN("[CLIENT] Connect() rejected: already in state {}", (int)state_.load());
        return;
    }

    LOG_DEBUG("[CLIENT] Current non_cef_server={}", non_cef_server_.load());

	if (non_cef_server_.load())
    {
        state_.store(ConnectionState::DISCONNECTED, std::memory_order_release);
		return;
    }

	playerid_ = playerid;
	join_attempts_ = 0;

	LOG_INFO("[CLIENT] Connecting to server for playerid {}...", playerid_);

	try {
		if (io_context_.stopped()) io_context_.restart();

		if (!socket_.is_open()) {
			asio::error_code ec;
			socket_.open(asio::ip::udp::v4(), ec);
			if (ec) throw std::runtime_error("Socket open failed: " + ec.message());
			socket_.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0), ec);
			LOG_INFO("[CLIENT] Socket opened and bound to local port {}.", socket_.local_endpoint().port());

			DoReceive();

			if (!network_thread_.joinable()) {
				network_thread_ = std::thread([this]() {
					LOG_INFO("[CLIENT] Network thread started.");
					
					// Use a work guard so the thread never exits until Shutdown()
					auto work_guard = asio::make_work_guard(io_context_);
					io_context_.run();
					
					LOG_INFO("[CLIENT] Network thread finished.");
				});
			}
		}

		auto keys = SecurityManager::GenerateKeys();
		client_public_key_ = std::move(keys.public_key);
		client_private_key_ = std::move(keys.private_key);

		DoSendRequestJoin();
	}
	catch (const std::exception& e) {
		LOG_ERROR("[CLIENT] Connection process failed to start: {}", e.what());
		Disconnect();
	}
}

void NetworkManager::Disconnect()
{
    ConnectionState prev = state_.exchange(ConnectionState::DISCONNECTED, std::memory_order_acq_rel);
	if (prev == ConnectionState::DISCONNECTED)
		return;

	LOG_INFO("[CLIENT] Disconnecting...");
	FireSessionActive(false);

	asio::post(io_context_, [this]() {
		connect_timer_.cancel();
		kcp_update_timer_.cancel();

        {
            std::lock_guard<std::mutex> lock(kcp_mutex_);
            if (kcp_instance_) {
                ikcp_release(kcp_instance_);
                kcp_instance_ = nullptr;
            }
        }
		// Do NOT close the socket here so the thread stays alive for reconnections.
	});
}

void NetworkManager::DoSendRequestJoin()
{
	if (state_ != ConnectionState::SENDING_JOIN) 
		return;

	join_attempts_++;
	if (join_attempts_ > MAX_JOIN_ATTEMPTS)
	{
		LOG_WARN("[CLIENT] No handshake after {} attempts -> assuming non-CEF server. Disconnecting.", MAX_JOIN_ATTEMPTS);
		non_cef_server_.store(true);
		Disconnect();
		return;
	}

	LOG_INFO("[CLIENT] Sending RequestJoin packet (attempt {}/{})...", join_attempts_, MAX_JOIN_ATTEMPTS);

	RequestJoinPacket pkt{ playerid_, PLUGIN_VERSION_U32 };
	SendPacket(PacketType::RequestJoin, pkt);

	connect_timer_.expires_after(std::chrono::milliseconds(CONNECT_RETRY_INTERVAL_MS));
	connect_timer_.async_wait([this](const std::error_code& ec) {
		if (!ec) {
			LOG_WARN("[CLIENT] Connection timer expired, retrying join...");
			DoSendRequestJoin();
		}
	});
}

void NetworkManager::DoReceive()
{
	socket_.async_receive_from(
		asio::buffer(recv_buffer_), remote_endpoint_,
		[this](std::error_code ec, std::size_t bytes_recvd) {
			if (!ec && bytes_recvd > 0) {
				if (remote_endpoint_ == server_endpoint_ && state_ != ConnectionState::DISCONNECTED) {
					HandleRawMessage(recv_buffer_.data(), bytes_recvd);
				}
			}
			
			// Always continue receiving unless the socket was closed by Shutdown()
			if (socket_.is_open()) {
				DoReceive();
			}
		}
	);
}

void NetworkManager::HandleRawMessage(const char* data, size_t len)
{
	if (kcp_instance_) {
		int input_res = ikcp_input(kcp_instance_, data, static_cast<long>(len));
		if (input_res < 0) {
			LOG_WARN("[KCP] ikcp_input error: {}.", input_res);
		}

		HandleKcpInput();
		return;
	}

	NetworkPacket packet;
	if (!DeserializePacket(data, len, packet)) {
		LOG_WARN("[CLIENT] Failed to deserialize raw packet of size {}.", len);
		return;
	}

	LOG_INFO("[CLIENT] Raw packet deserialized, type: {}", static_cast<int>(packet.type));

	switch (packet.type)
	{
		case PacketType::HandshakeChallenge:
		{
			if (state_ != ConnectionState::SENDING_JOIN) 
				return;

			connect_timer_.cancel();
			state_ = ConnectionState::AWAITING_ACCEPTANCE;

			const auto& challenge = std::get<HandshakeChallengePacket>(packet.payload);
			HandshakeFinalizePacket finalize_pkt;
			finalize_pkt.cookie = challenge.cookie;
			finalize_pkt.client_public_key = client_public_key_;

			LOG_INFO("[CLIENT] Sending HandshakeFinalize...");
			SendPacket(PacketType::HandshakeFinalize, finalize_pkt);

			auto session_keys = SecurityManager::GenerateClientSessionKeys(
				client_public_key_, client_private_key_, challenge.server_public_key);
			if (session_keys.rx.empty()) { Disconnect(); return; }
			rx_key_ = std::move(session_keys.rx);
			tx_key_ = std::move(session_keys.tx);
			sodium_memzero(client_private_key_.data(), client_private_key_.size());
			break;
		}
		case PacketType::JoinResponse:
		{
			if (state_ != ConnectionState::AWAITING_ACCEPTANCE && state_ != ConnectionState::SENDING_JOIN)
				return;

			const auto& response = std::get<JoinResponsePacket>(packet.payload);
			if (!response.accepted) {
				const uint32_t sv = response.server_version;
				const uint32_t cv = response.client_version;
				std::string msg = response.message;
				
				if (msg.empty()) 
					msg = "Join rejected";

				LOG_ERROR("[CLIENT] Join rejected by server: {} (reason={}, client={}, server={})",
					msg,
					(int)response.reject_reason,
					VersionToString(cv),
					VersionToString(sv));

				non_cef_server_.store(true);
				Disconnect();
				return;
			}

			LOG_INFO("[CLIENT] Join accepted! Initializing KCP with conv id {}.", response.kcp_conv_id);

			non_cef_server_.store(false);
			FireSessionActive(true);

			kcp_instance_ = ikcp_create(response.kcp_conv_id, this);
			kcp_instance_->output = kcp_client_output_callback;
			ikcp_nodelay(kcp_instance_, 1, 10, 2, 1);
			ikcp_wndsize(kcp_instance_, 128, 128);

			if (rx_key_.empty() || tx_key_.empty()) {
				LOG_ERROR("[CLIENT] Session keys not initialized!");
				Disconnect();
				return;
			}

			auto* netGame = GetComponent<NetGameComponent>();
			if (netGame) {
				resource_.OnConnect(netGame->GetIp(), netGame->GetPort());
			}

			resource_.OnManifestReceived(response.manifest_json);

			state_ = ConnectionState::CONNECTED;

			DoKcpUpdate();

			PacketHandler handler;
			{
				std::lock_guard lock(handler_mutex_);
				handler = packet_handler_;
			}

			if (handler)
				handler(packet);

			break;
		}
		default:
			LOG_WARN("[CLIENT] Received unexpected raw packet of type {} during handshake.", static_cast<int>(packet.type));
			break;
	}
}

void NetworkManager::HandleKcpInput()
{
	std::vector<char> kcp_buffer(65535);
	int msg_size;

	while ((msg_size = ikcp_recv(kcp_instance_, kcp_buffer.data(), static_cast<int>(kcp_buffer.size()))) > 0)
	{
		std::vector<uint8_t> decrypted = DecryptPacket({ kcp_buffer.begin(), kcp_buffer.begin() + msg_size }, rx_key_);
		if (decrypted.empty()) {
			LOG_WARN("[CLIENT] Failed to decrypt KCP packet.");
			continue;
		}

		NetworkPacket packet;
		if (!DeserializePacket(reinterpret_cast<const char*>(decrypted.data()), decrypted.size(), packet)) {
			LOG_WARN("[CLIENT] Failed to deserialize decrypted KCP packet.");
			continue;
		}

		PacketHandler handler;
		{ std::lock_guard lock(handler_mutex_); handler = packet_handler_; }
		if (handler) handler(packet);
	}
}

void NetworkManager::FireSessionActive(bool active)
{
    SessionActiveHandler handler;
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        handler = session_handler_;
    }

    if (handler)
        handler(active);
}

void NetworkManager::SendPacket(PacketType type, const PacketPayload& payload)
{
	LOG_DEBUG("[CLIENT] SendPacket called, type={}", static_cast<int>(type));

	NetworkPacket packet{ type, payload };
	std::string raw;

	if (!SerializePacket(packet, raw)) {
		LOG_ERROR("[CLIENT] Failed to serialize packet type {}", static_cast<int>(type));
		return;
	}

	std::lock_guard<std::mutex> lock(kcp_mutex_);


	if (kcp_instance_ && state_ == ConnectionState::CONNECTED) {
		if (tx_key_.empty()) {
			LOG_ERROR("[CLIENT] tx_key_ is empty!");
			return;
		}

		if (tx_key_.size() != 32) {
			LOG_ERROR("[CLIENT] tx_key_ has invalid size: {}", tx_key_.size());
			return;
		}

		std::vector<uint8_t> encrypted = EncryptPacket({ raw.begin(), raw.end() }, tx_key_);

		if (encrypted.empty()) {
			LOG_ERROR("[CLIENT] Failed to encrypt packet");
			return;
		}

		int sent = ikcp_send(kcp_instance_,
			reinterpret_cast<const char*>(encrypted.data()),
			static_cast<int>(encrypted.size()));

		if (sent < 0) {
			LOG_ERROR("[CLIENT] ikcp_send failed with error: {}", sent);
			return;
		}

		ikcp_flush(kcp_instance_);
	}
	else {
		SendRaw(raw.data(), static_cast<int>(raw.size()));
	}
}

void NetworkManager::DoKcpUpdate()
{
	std::lock_guard<std::mutex> lock(kcp_mutex_);

	if (state_ != ConnectionState::CONNECTED || !kcp_instance_)
		return;

	ikcp_update(kcp_instance_, iclock());

	kcp_update_timer_.expires_after(std::chrono::milliseconds(KCP_UPDATE_INTERVAL_MS));
	kcp_update_timer_.async_wait([this](const std::error_code& ec) { if (!ec) DoKcpUpdate(); });
}

void NetworkManager::SendRaw(const char* data, int size) {
	auto buffer = std::make_shared<std::vector<char>>(data, data + size);
	socket_.async_send_to(
		asio::buffer(*buffer), server_endpoint_,
		[this, buffer, size](std::error_code ec, std::size_t) {
			if (ec) {
				LOG_ERROR("[CLIENT] UDP send error: {}", ec.message());
			}
		}
	);
}

void NetworkManager::SetPacketHandler(PacketHandler handler) 
{
	std::lock_guard lock(handler_mutex_);
	packet_handler_ = std::move(handler);
}

void NetworkManager::SendBrowserCreateResult(int browserId, bool success, int code, const std::string& reason)
{
	ClientEmitEventPacket event;
    event.name = CefEvent::Client::BrowserCreateResult;
    event.browserId = browserId;
    event.args.emplace_back(success);
    event.args.emplace_back(code);
    event.args.emplace_back(reason);

	SendPacket(PacketType::ClientEmitEvent, event);
}