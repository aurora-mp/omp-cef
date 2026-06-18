#include "plugin.hpp"

#include <shared/crypto.hpp>

#include "resource_manager.hpp"
#include "session.hpp"
#include "shared/packet-serializer.hpp"
#include "shared/packet.hpp"
#include "shared/utils.hpp"
#include "shared/events.hpp"
#include "shared/version.hpp"

CefPlugin::CefPlugin()
{
	security_ = std::make_unique<SecurityManager>();
	sessions_ = std::make_unique<NetworkSessionManager>();
	api_ = std::make_unique<CefApi>(*this);
	resource_ = std::make_unique<ResourceManager>();
}

void CefPlugin::Initialize(std::unique_ptr<IPlatformBridge> bridge, uint16_t listen_port, const CefPluginOptions& options)
{
	if (running_)
		return;

	bridge_ = std::move(bridge);
	master_resource_key_ = options.master_resource_key;
    resources_loader_ui_ = options.resources_loader_ui;

	logger_.SetBridge(bridge_.get());
	logger_.SetLevel(options.log_level);
	logging::SetLogger(&logger_);

	security_->Initialize(io_context_);

    const uint16_t port = (listen_port != 0 ? listen_port : static_cast<uint16_t>(7779));

	try
	{
		network_server_ = std::make_unique<NetworkServer>(
			port,
			io_context_,
			[this](const asio::ip::udp::endpoint& from, const char* data, int len)
			{ 
				this->OnPacketReceived(from, data, len); 
			},
			[this](uint32_t now_ms)
			{
				sessions_->UpdateAllKcpInstances(now_ms);
				this->ProcessFileTransfers();
			});

		sessions_->SetSender(
			[this](const asio::ip::udp::endpoint& addr, const char* data, int len)
			{
				if (!network_server_)
					return;

				network_server_->SendTo(addr, data, len);
			});

		network_server_->Start();

		io_context_.restart();

		network_thread_ = std::thread([this]() {
			io_context_.run();
		});

		running_ = true;

		LOG_INFO("Network Server started successfully on port %d.", (int)port);
	}
	catch (const std::exception& e)
	{
		network_server_.reset();
		logging::SetLogger(nullptr);
		bridge_.reset();

		LOG_ERROR("Failed to start Network Server: %s", e.what());
	}
}

void CefPlugin::Shutdown()
{
	if (!running_)
		return;

	running_ = false;

    if (network_server_) {
        network_server_->Stop();
    }

    if (security_) {
        security_->Shutdown();
        security_.reset();
    }

    io_context_.stop();

    if (network_thread_.joinable()) {
        network_thread_.join();
    }

    network_server_.reset();
    sessions_.reset();
    api_.reset();
    resource_.reset();

    logging::SetLogger(nullptr);
    bridge_.reset();
}

void CefPlugin::OnPlayerConnect(int playerid)
{
    if (!bridge_)
        return;

    if (bridge_->IsPlayerNpcBot(playerid))
    {
        LOG_DEBUG("OnPlayerConnect: player %d is NPC -> ignored", playerid);
        return;
    }

    sessions_->RegisterPlayer(playerid);
    LOG_DEBUG("OnPlayerConnect: player %d registered", playerid);
}

void CefPlugin::OnPlayerClientInit(int playerid)
{
    if (!bridge_ || !running_)
        return;

    if (bridge_->IsPlayerNpcBot(playerid))
    {
        LOG_DEBUG("OnPlayerClientInit: player %d is NPC -> ignored", playerid);
        return;
    }

    auto session = sessions_->GetSession(playerid);
    if (!session)
    {
        LOG_WARN("OnPlayerClientInit: no session for player %d (not registered?)", playerid);
        return;
    }

    if (session->handshake_complete && !session->cef_init_notified)
    {
        LOG_DEBUG("Player %d client init: handshake already complete -> OnCefInitialize(true)", playerid);
        NotifyCefInitialize(session, true, CEF_INIT_OK, "");
        return;
    }

    // Avoid starting multiple timers
	bool expected = false;
	if (!session->cef_init_timer_started.compare_exchange_strong(expected, true))
	{
	    return;
	}

    auto timer = std::make_shared<asio::steady_timer>(io_context_);
    timer->expires_after(std::chrono::seconds(10));
    timer->async_wait([this, playerid, timer](const std::error_code& error_code)
    {
        if (error_code || !running_ || !bridge_)
            return;

        auto session = sessions_->GetSession(playerid);
        if (!session)
            return;

        if (!session->handshake_complete && !session->cef_init_notified)
        {
            LOG_DEBUG("Player %d: handshake timeout -> OnCefInitialize(false)", playerid);
            NotifyCefInitialize(session, false, CEF_INIT_TIMEOUT, "CEF handshake timeout");
        }
    });
}

void CefPlugin::OnPlayerDisconnect(int playerid)
{
    if (!bridge_)
        return;

    if (bridge_->IsPlayerNpcBot(playerid))
        return;

    sessions_->RemovePlayer(playerid);
    LOG_DEBUG("OnPlayerDisconnect: player %d removed", playerid);
}

void CefPlugin::OnPacketReceived(const asio::ip::udp::endpoint& from, const char* data, int len)
{
    if (!data || len <= 0)
        return;

    auto network_session = sessions_->GetSessionFromAddress(from);
    
    // Fallback: Roaming support. 
    // If we don't recognize the endpoint but the packet looks like KCP, try to find session by conv ID (playerid).
    if (!network_session && len >= 24)
    {
        uint32_t conv = 0;
        std::memcpy(&conv, data, 4);

        auto session = sessions_->GetSession(static_cast<int>(conv));
        // Security check: only allow roaming if the IP address matches.
        if (session && session->handshake_status == HandshakeStatus::CONNECTED && session->address.address() == from.address())
        {
            LOG_DEBUG("[CEF] Player %d roaming detected: %s:%d -> %s:%d", 
                session->playerid, 
                session->address.address().to_string().c_str(), (int)session->address.port(),
                from.address().to_string().c_str(), (int)from.port());

            sessions_->MapAddressToPlayer(session->playerid, from);
            {
                std::lock_guard<std::mutex> lock(session->kcp_mutex);
                session->address = from;
            }
            network_session = session;
        }
    }

    if (network_session && network_session->handshake_status == HandshakeStatus::CONNECTED && network_session->kcp_instance) {
		{
			std::lock_guard<std::mutex> lock(network_session->kcp_mutex);
			ikcp_input(network_session->kcp_instance, data, len);
		}

		HandleKcpInput(network_session);
		return;
	}

    const auto packet_type = static_cast<PacketType>(static_cast<uint8_t>(data[0]));

    // Before a transport is attached to an endpoint, the server must only accept
    // the raw packets that are correct for the current handshake state.
    if (!network_session)
    {
        const auto legacy_request_join_size = static_cast<int>(1 + sizeof(int));
        const auto request_join_size = static_cast<int>(legacy_request_join_size + sizeof(uint32_t));

        if (packet_type != PacketType::RequestJoin ||
            (len != legacy_request_join_size && len != request_join_size))
        {
            return;
        }
    }
    else if (network_session->handshake_status == HandshakeStatus::CHALLENGED)
    {
        if (packet_type != PacketType::HandshakeFinalize)
            return;
    }
    else
    {
        return;
    }

	NetworkPacket packet;
    if (!DeserializePacket(data, len, packet))
        return;

    if (!network_session && sessions_->IsEndpointRecentlyClosed(from))
    {
        bool allow_fresh_join_from_reused_endpoint = false;

        if (packet.type == PacketType::RequestJoin)
        {
            const auto& join_packet = std::get<RequestJoinPacket>(packet.payload);
            allow_fresh_join_from_reused_endpoint =
                join_packet.playerid >= 0 &&
                join_packet.client_version == PLUGIN_VERSION_U32 &&
                sessions_->GetSession(join_packet.playerid) != nullptr;
        }

        if (!allow_fresh_join_from_reused_endpoint)
        {
            return;
        }

        sessions_->ClearClosedEndpoint(from);
    }

	switch (packet.type)
	{
		case PacketType::RequestJoin: {
			HandleRequestJoin(from, std::get<RequestJoinPacket>(packet.payload));
			break;
		}
		case PacketType::HandshakeFinalize:
			if (network_session && network_session->handshake_status == HandshakeStatus::CHALLENGED) {
				HandleHandshakeFinalize(from, std::get<HandshakeFinalizePacket>(packet.payload), network_session);
			}

            break;
		default:
			break;
	}
}

void CefPlugin::HandleRequestJoin(const asio::ip::udp::endpoint& from, const RequestJoinPacket& join_packet)
{
    if (!bridge_ || !running_)
        return;

    const int playerid = join_packet.playerid;
    const std::string from_ip = from.address().to_string();
    const int from_port = static_cast<int>(from.port());

    if (playerid < 0)
    {
        LOG_DEBUG("RequestJoin refused: invalid pid=%d (from %s:%d)", playerid, from_ip.c_str(), from_port);
        return;
    }

    auto session = sessions_->GetSession(playerid);
    if (!session)
    {
        LOG_DEBUG("RequestJoin refused: no session for pid=%d (from %s:%d)", playerid, from_ip.c_str(), from_port);
        return;
    }

    if (bridge_->IsPlayerNpcBot(playerid))
    {
        LOG_DEBUG("RequestJoin refused: pid=%d is NPC (from %s:%d)", playerid, from_ip.c_str(), from_port);
        return;
    }

    const std::string official_ip = bridge_->GetPlayerAddressIp(playerid);

    LOG_INFO("RequestJoin pid=%d from=%s:%d official=%s", playerid, from_ip.c_str(), from_port, official_ip.c_str());

    if (!official_ip.empty() && official_ip != from_ip)
    {
        LOG_WARN("RequestJoin dropped: IP mismatch (pid=%d, official=%s, from=%s)", playerid, official_ip.c_str(), from_ip.c_str());
        return;
    }

    const uint32_t clientVersion = join_packet.client_version;
    const uint32_t serverVersion = PLUGIN_VERSION_U32;
    if (clientVersion != serverVersion)
    {
        JoinResponsePacket reject;
        reject.accepted = false;
        reject.kcp_conv_id = 0;
        reject.manifest_json.clear();
        reject.reject_reason = static_cast<uint8_t>(JoinRejectReason::VersionMismatch);
        reject.server_version = serverVersion;
        reject.client_version = clientVersion;
        reject.message = "CEF version mismatch";

        LOG_WARN("RequestJoin refused: version mismatch (pid=%d client=%s server=%s)",
            playerid,
            VersionToString(clientVersion).c_str(),
            VersionToString(serverVersion).c_str());

        SendRawPacketToEndpoint(from, PacketType::JoinResponse, reject);

        if (session->handshake_status != HandshakeStatus::CONNECTED)
        {
            session->handshake_status = HandshakeStatus::NONE;
            session->handshake_complete = false;
            NotifyCefInitialize(session, false, CEF_INIT_VERSION_MISMATCH, reject.message);
        }

        return;
    }

    if (session->handshake_status == HandshakeStatus::CONNECTED)
    {
        if (session->address == from)
        {
            LOG_DEBUG("RequestJoin ignored: pid=%d already CONNECTED on the same endpoint", playerid);
            return;
        }

        LOG_INFO("RequestJoin rejoin: pid=%d endpoint changed from %s:%d to %s:%d -> resetting CEF transport",
            playerid,
            session->address.address().to_string().c_str(),
            static_cast<int>(session->address.port()),
            from_ip.c_str(),
            from_port);

        sessions_->ResetPlayerTransport(playerid);
        session = sessions_->GetSession(playerid);
        if (!session)
            return;
    }
    else if (session->handshake_status == HandshakeStatus::CHALLENGED && session->address != from)
    {
        LOG_DEBUG("RequestJoin reattempt: pid=%d endpoint changed before finalize from %s:%d to %s:%d",
            playerid,
            session->address.address().to_string().c_str(),
            static_cast<int>(session->address.port()),
            from_ip.c_str(),
            from_port);

        sessions_->ResetPlayerTransport(playerid);
        session = sessions_->GetSession(playerid);
        if (!session)
            return;
    }

    session->address = from;
    session->handshake_status = HandshakeStatus::CHALLENGED;
    session->handshake_complete = false;
    session->cef_init_notified = false;
    session->cef_success = false;
    session->cef_init_timer_started = false;
    sessions_->MapAddressToPlayer(playerid, from);

    HandshakeChallengePacket response;
    response.cookie = security_->GenerateCookie(from);
    response.server_public_key = security_->InitiateKeyExchange(playerid);

    SendRawPacketToEndpoint(from, PacketType::HandshakeChallenge, response);
}

void CefPlugin::HandleHandshakeFinalize(const asio::ip::udp::endpoint& from, 
	const HandshakeFinalizePacket& finalize_packet, 
	std::shared_ptr<NetworkSession> session)
{
    if (!session || !bridge_ || !running_)
        return;

    if (session->address.address() != from.address())
    {
        std::string from_ip = from.address().to_string();
        LOG_WARN("HandshakeFinalize dropped: IP mismatch pid=%d (official=%s, from=%s)", 
            session->playerid, session->address.address().to_string().c_str(), from_ip.c_str());
        return;
    }

    if (session->address.port() != from.port())
    {
        LOG_DEBUG("[CEF] Player %d port changed during handshake: %d -> %d", 
            session->playerid, (int)session->address.port(), (int)from.port());
        session->address = from;
        sessions_->MapAddressToPlayer(session->playerid, from);
    }

    if (!security_->ValidateCookie(from, finalize_packet.cookie))
    {
        LOG_WARN("HandshakeFinalize failed: invalid cookie pid=%d", session->playerid);
        session->handshake_status = HandshakeStatus::NONE;
        session->handshake_complete = false;

        NotifyCefInitialize(session, false, CEF_INIT_HANDSHAKE_FAILED, "Handshake failed: invalid cookie");
        return;
    }

    auto session_keys = security_->FinalizeKeyExchange(session->playerid, finalize_packet.client_public_key);
    if (!session_keys)
    {
        LOG_WARN("HandshakeFinalize failed: key exchange pid=%d", session->playerid);
        session->handshake_status = HandshakeStatus::NONE;
        session->handshake_complete = false;

        NotifyCefInitialize(session, false, CEF_INIT_HANDSHAKE_FAILED, "Handshake failed: key exchange");
        return;
    }

	session->rx_key = std::move(session_keys->rx);
	session->tx_key = std::move(session_keys->tx);

	JoinResponsePacket join_response;
	join_response.accepted = true;
	join_response.kcp_conv_id = session->playerid;
    join_response.reject_reason = static_cast<uint8_t>(JoinRejectReason::None);
    join_response.server_version = PLUGIN_VERSION_U32;
    join_response.client_version = PLUGIN_VERSION_U32;
    join_response.message.clear();

	nlohmann::json manifest = resource_->GetManifestAsJson();
	if (!manifest.is_null()) {
		join_response.manifest_json = manifest.dump();
	}

	SendRawPacketToEndpoint(from, PacketType::JoinResponse, join_response);

	session->kcp_instance = ikcp_create(session->playerid, session.get());
	session->kcp_instance->output = kcp_output_callback;

	ikcp_nodelay(session->kcp_instance, 1, 10, 2, 1);
	ikcp_wndsize(session->kcp_instance, 256, 256);

	session->handshake_status = HandshakeStatus::CONNECTED;

	ServerConfigPacket config_packet;
	config_packet.master_resource_key = master_resource_key_;
    config_packet.resources_loader_ui = resources_loader_ui_;
	SendPacketToPlayer(session->playerid, PacketType::ServerConfig, config_packet);

	session->handshake_complete = true;

    NotifyCefInitialize(session, true, CEF_INIT_OK, "");

    LOG_DEBUG("Network handshake for player %d is complete.", session->playerid);
}

void CefPlugin::HandleKcpInput(std::shared_ptr<NetworkSession> session)
{
    if (!session)
        return;

    std::vector<NetworkPacket> pendingPackets;

    {
        std::lock_guard<std::mutex> lock(session->kcp_mutex);

        if (!session->kcp_instance)
            return;

        std::vector<char> kcp_buffer(65535);
        int msg_size;

        while ((msg_size = ikcp_recv(session->kcp_instance, kcp_buffer.data(),
            static_cast<int>(kcp_buffer.size()))) > 0)
        {
            std::vector<uint8_t> decrypted =
                DecryptPacket({ kcp_buffer.begin(), kcp_buffer.begin() + msg_size }, session->rx_key);

            if (decrypted.empty())
                continue;

            NetworkPacket packet;
            if (!DeserializePacket(reinterpret_cast<const char*>(decrypted.data()), decrypted.size(), packet))
                continue;

            pendingPackets.emplace_back(std::move(packet));
        }
    }

    for (auto& packet : pendingPackets)
    {
        switch (packet.type)
        {
            case PacketType::RequestFiles:
            {
                HandleFileRequest(session->playerid, std::get<RequestFilesPacket>(packet.payload));
                break;
            }
            case PacketType::DownloadComplete:
            {
				NotifyCefReady(session);
                break;
            }
            case PacketType::ClientEmitEvent:
            {
                if (auto* event = std::get_if<ClientEmitEventPacket>(&packet.payload)) {
                    HandleClientEvent(session->playerid, *event);
                }
                else {
                    LOG_ERROR("ClientEmitEvent payload mismatch (variant index=%zu)", packet.payload.index());
                }
                break;
            }
            default:
                break;
        }
    }
}

void CefPlugin::HandleFileRequest(int playerid, const RequestFilesPacket& request)
{
	auto session = sessions_->GetSession(playerid);
	if (!session)
		return;

	for (const auto& [resourceName, relativePath] : request.files) {
		if (resource_->IsFileValid(resourceName, relativePath)) {

			std::vector<uint8_t> content;
			if (!resource_->GetPakContent(resourceName, content)) {
				continue;
			}

			FileInfo pakInfo;
			if (!resource_->GetPakInfo(resourceName, pakInfo)) {
				LOG_WARN("[CEF] Could not resolve pak metadata for resource '%s'.", resourceName.c_str());
				continue;
			}

			auto transfer = std::make_shared<FileTransfer>();
			transfer->resourceName = resourceName;
			transfer->relativePath = relativePath;
			transfer->fileHash = pakInfo.fileHash;
			transfer->content = std::move(content);
			transfer->totalChunks = (transfer->content.size() + FILE_CHUNK_SIZE - 1) / FILE_CHUNK_SIZE;
			transfer->currentChunkIndex = 0;

			session->download_queue.push(transfer);
		}
	}
}

void CefPlugin::ProcessFileTransfers()
{
    static constexpr int MAX_CHUNKS_PER_TICK = 64;
    static constexpr int MAX_IN_FLIGHT_SEGMENTS = 220; 

    auto all_sessions = sessions_->GetAllSessions();
    for (auto& session : all_sessions)
    {
        if (!session || !session->kcp_instance || !session->handshake_complete)
            continue;

		if (session->is_download_paused.load(std::memory_order_relaxed)) {
            continue;
        }

        if (!session->current_transfer && !session->download_queue.empty())
        {
            session->current_transfer = session->download_queue.front();
            session->download_queue.pop();

            LOG_DEBUG("Starting transfer for player %d - file '%s'", session->playerid, session->current_transfer->relativePath.c_str());
        }

        auto& transfer = session->current_transfer;
        if (!transfer) 
            continue;

        int sent_this_tick = 0;

        while (transfer && sent_this_tick < MAX_CHUNKS_PER_TICK)
        {
			if (session->is_download_paused.load(std::memory_order_relaxed)) {
                break;
            }

            if (transfer->currentChunkIndex >= transfer->totalChunks)
            {
                LOG_DEBUG("Completed transfer for player %d - file '%s'", session->playerid, transfer->relativePath.c_str());

                session->current_transfer = nullptr;
                break;
            }

            int in_flight = 0;
            {
                std::lock_guard<std::mutex> guard(session->kcp_mutex);
                if (!session->kcp_instance) break;
                in_flight = ikcp_waitsnd(session->kcp_instance);
            }

            if (in_flight >= MAX_IN_FLIGHT_SEGMENTS)
            {
                break;
            }

            size_t chunkOffset = transfer->currentChunkIndex * FILE_CHUNK_SIZE;
            size_t remaining = transfer->content.size() - chunkOffset;
            size_t chunkSize = std::min(static_cast<size_t>(FILE_CHUNK_SIZE), remaining);

            FileDataPacket packet;
            packet.resourceName = transfer->resourceName;
            packet.relativePath = transfer->relativePath;
            packet.fileHash = transfer->fileHash;
            packet.chunkIndex = transfer->currentChunkIndex;
            packet.totalChunks = transfer->totalChunks;
            packet.data.assign(
                transfer->content.begin() + chunkOffset,
                transfer->content.begin() + chunkOffset + chunkSize
            );

            SendPacketToPlayer(session->playerid, PacketType::FileData, packet);

            ++transfer->currentChunkIndex;
            ++sent_this_tick;
        }
    }
}

void CefPlugin::SendRawPacketToEndpoint(const asio::ip::udp::endpoint& endpoint, PacketType type, const PacketPayload& payload)
{
	NetworkPacket packet{ type, payload };

	std::string raw_data;
	if (!SerializePacket(packet, raw_data))
		return;

	if (network_server_)
		network_server_->SendTo(endpoint, raw_data.data(), static_cast<int>(raw_data.size()));
}

void CefPlugin::SendPacketToPlayer(int playerid, PacketType type, const PacketPayload& payload)
{
    auto session = sessions_->GetSession(playerid);
    if (!session)
        return;

    NetworkPacket packet{ type, payload };

    std::string raw_data;
    if (!SerializePacket(packet, raw_data)) {
        LOG_ERROR("Failed to serialize packet (type %d) for player %d", (int)type, playerid);
        return;
    }

    std::vector<uint8_t> encrypted = EncryptPacket({ raw_data.begin(), raw_data.end() }, session->tx_key);
    if (encrypted.empty())
        return;

    std::lock_guard<std::mutex> lock(session->kcp_mutex);
    if (!session->kcp_instance)
        return;

    ikcp_send(session->kcp_instance, (const char*)encrypted.data(), (int)encrypted.size());

	// TODO: Not always flush immediately (for file transfer?)
    ikcp_flush(session->kcp_instance);
}

void CefPlugin::NotifyCefInitialize(std::shared_ptr<NetworkSession> session, bool success, int reason, std::string message)
{
	if (!session || !bridge_)
		return;
    
	if (session->cef_init_notified)
		return;

    session->cef_init_notified = true;
    session->cef_success = success;

    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callback_queue_.push([this, playerid = session->playerid, success, reason, message]() {
            if (bridge_) {
                std::vector<Argument> args;
                args.emplace_back(playerid);
                args.emplace_back(success);
                args.emplace_back(reason);
                args.emplace_back(message);
                bridge_->CallPawnPublic("OnCefInitialize", args);
            }
        });
    }
}

void CefPlugin::NotifyCefReady(std::shared_ptr<NetworkSession> session)
{
	if (!session || !bridge_)
		return;

	if (!session->cef_init_notified)
		return;

    if (!session->cef_success)
		return;

    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callback_queue_.push([this, playerid = session->playerid]() {
            if (bridge_) {
                std::vector<Argument> args;
                args.emplace_back(playerid);
                bridge_->CallPawnPublic("OnCefReady", args);
            }
        });
    }
}

void CefPlugin::HandleClientEvent(int playerid, const ClientEmitEventPacket& payload)
{
	if (payload.name == CefEvent::Client::BrowserCreateResult)
	{
		if (payload.args.size() >= 3)
		{
			int browserId = payload.browserId;
			bool success = payload.args[0].boolValue;
			int code = payload.args[1].intValue;
			const std::string& reason = payload.args[2].stringValue;

            std::lock_guard<std::mutex> lock(callback_mutex_);
            callback_queue_.push([this, playerid, browserId, success, code, reason]() {
                if (bridge_) {
                    bridge_->CallOnBrowserCreated(playerid, browserId, success, code, reason);
                }
            });
		}

		return;
	}

    if (payload.name == CefEvent::Client::DownloadStart)
	{
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callback_queue_.push([this, playerid]() {
            if (bridge_) {
                bridge_->CallOnDownloadStart(playerid);
            }
        });
		return;
	}

    if (payload.name == CefEvent::Client::DownloadFinish)
	{
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callback_queue_.push([this, playerid]() {
            if (bridge_) {
                bridge_->CallOnDownloadFinish(playerid);
            }
        });
		return;
	}

    if (payload.name == CefEvent::Client::PressKey)
	{
		if (payload.args.size() >= 5)
		{
			int key = payload.args[0].intValue;
			int scancode = payload.args[1].intValue;
			int modifiers = payload.args[2].intValue;
			bool down = payload.args[3].boolValue;
			bool repeat = payload.args[4].boolValue;

            std::lock_guard<std::mutex> lock(callback_mutex_);
            callback_queue_.push([this, playerid, key, scancode, modifiers, down, repeat]() {
                if (bridge_) {
                    bridge_->CallOnPressKey(playerid, key, scancode, modifiers, down, repeat);
                }
            });
		}

		return;
	}

    if (payload.name == CefEvent::Client::ChatInputState)
	{
		if (payload.args.size() >= 1)
		{
			bool open = payload.args[0].boolValue;

			auto session = sessions_->GetSession(playerid);
			if (session)
				session->chat_input_open = open;

            std::lock_guard<std::mutex> lock(callback_mutex_);
            callback_queue_.push([this, playerid, open]() {
                if (bridge_) {
                    std::vector<Argument> args;
                    args.emplace_back(playerid);
                    args.emplace_back(open);
                    bridge_->CallPawnPublic("OnCefChatInputState", args);
                }
            });
		}

		return;
	}

    auto it = registered_events_.find(payload.name);
    if (it == registered_events_.end())
        return;

    const auto& reg = it->second;
    const auto& signature = reg.signature;

    if (signature.size() != payload.args.size())
    {
        LOG_WARN("Argument count mismatch for event '%s' (callback '%s'). Expected %zu, got %zu.",
            payload.name.c_str(), reg.callback.c_str(), signature.size(), payload.args.size());
        return;
    }

    for (size_t i = 0; i < payload.args.size(); ++i)
    {
        const auto& arg = payload.args[i];
        const auto expected = signature[i];

        if (arg.type != expected)
        {
            LOG_WARN("Type mismatch for event '%s' (callback '%s') at arg %zu. Expected %d, got %d.",
                payload.name.c_str(), reg.callback.c_str(), i,
                static_cast<int>(expected), static_cast<int>(arg.type));
            return;
        }
    }

    std::vector<Argument> final_args;
    final_args.reserve(2 + payload.args.size());
    final_args.emplace_back(playerid);
    final_args.emplace_back(payload.browserId);
    final_args.insert(final_args.end(), payload.args.begin(), payload.args.end());

    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callback_queue_.push([this, callback = reg.callback, args = std::move(final_args)]() {
            if (bridge_) {
                bridge_->CallPawnPublic(callback, args);
            }
        });
    }
}

void CefPlugin::ProcessPendingCallbacks()
{
    std::queue<std::function<void()>> callbacks;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        std::swap(callbacks, callback_queue_);
    }

    while (!callbacks.empty())
    {
        callbacks.front()();
        callbacks.pop();
    }
}

void CefPlugin::RegisterEvent(const std::string& name, const std::string& callback, const std::vector<ArgumentType>& signature)
{
	for (size_t i = 0; i < signature.size(); ++i)
	{
		const auto& type = signature[i];
		const char* typeName = "Unknown";

		switch (type)
		{
			case ArgumentType::String:
				typeName = "String";
				break;
			case ArgumentType::Integer:
				typeName = "Integer";
				break;
			case ArgumentType::Float:
				typeName = "Float";
				break;
			case ArgumentType::Bool:
				typeName = "Bool";
				break;
		}
	}

	RegisteredEvent event;
    event.callback = callback.empty() ? name : callback;
    event.signature = signature;

    registered_events_[name] = std::move(event);
}

CefPlugin::~CefPlugin()
{
	Shutdown();
}