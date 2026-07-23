#pragma once

#include <asio.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <shared/packet.hpp>
#include <unordered_map>
#include <vector>

#include "api.hpp"
#include "bridge.hpp"
#include "logger.hpp"
#include "network.hpp"
#include "resource_manager.hpp"
#include "resource_download_dialog_manager.hpp"
#include "security.hpp"
#include "session.hpp"

struct CefPluginOptions
{
    CefLogLevel log_level = CefLogLevel::Info;
	std::vector<uint8_t> master_resource_key = {};
	ResourceLoaderUiMode resources_loader_mode = ResourceLoaderUiMode::Cef;
	int resources_loader_dialog_id = ResourceDownloadDialogManager::DefaultDialogId;
};

struct RegisteredEvent
{
    std::string callback;
    std::vector<ArgumentType> signature;
};

enum CefInitReason : int
{
    CEF_INIT_OK = 0,
    CEF_INIT_TIMEOUT = 1,
    CEF_INIT_VERSION_MISMATCH = 2,
    CEF_INIT_IP_MISMATCH = 3,
    CEF_INIT_HANDSHAKE_FAILED = 4,
    CEF_INIT_UNKNOWN = 5
};

class CefPlugin
{
public:
	CefPlugin();
	CefPlugin(const CefPlugin&) = delete;
	CefPlugin& operator=(const CefPlugin&) = delete;
	~CefPlugin();

	void Initialize(std::unique_ptr<IPlatformBridge> bridge, uint16_t listen_port, const CefPluginOptions& options);
	void Shutdown();

	void OnPlayerConnect(int playerid);
	void OnPlayerClientInit(int playerid);
	void OnPlayerDisconnect(int playerid);
	bool OnDialogResponse(int playerid, int dialogid);
	void SetSpawnScreenState(int playerid, bool visible);
	void InvalidatePawnBridge();

	void OnPacketReceived(const asio::ip::udp::endpoint& from, const char* data, int len);

	void HandleFileRequest(const std::shared_ptr<NetworkSession>& session, uint64_t epoch, const RequestFilesPacket& request);
	void ProcessFileTransfers();
	void ProcessMainThreadTasks();

	void SendRawPacketToEndpoint(const asio::ip::udp::endpoint& endpoint, PacketType type, const PacketPayload& payload);
	void SendPacketToPlayer(int playerid, PacketType type, const PacketPayload& payload);

	void NotifyCefInitialize(std::shared_ptr<NetworkSession> session, bool success, int reason = CEF_INIT_OK, std::string message = {});
	void NotifyCefReady(std::shared_ptr<NetworkSession> session);
	void HandleClientEvent(int playerid, const ClientEmitEventPacket& payload);
	void RegisterEvent(const std::string& name, const std::string& callback, const std::vector<ArgumentType>& signature);

	ResourceManager& GetResourceManager()
	{
		return *resource_;
	}

	NetworkSessionManager& GetNetworkSessionManager()
	{
		return *sessions_;
	}

	const std::vector<uint8_t>& GetMasterKey() const { return master_resource_key_; }

private:
	void HandleRequestJoin(const asio::ip::udp::endpoint& from, const RequestJoinPacket& packet);
	void HandleHandshakeFinalize(const asio::ip::udp::endpoint& from, const HandshakeFinalizePacket& finalize_packet, std::shared_ptr<NetworkSession> session);
	void HandleKcpInput(std::shared_ptr<NetworkSession> session);
	void SendPacketToSession(const std::shared_ptr<NetworkSession>& session, PacketType type, const PacketPayload& payload);
	void EnqueueMainThreadTask(std::function<void()> task);
	bool IsSessionCurrent(const std::shared_ptr<NetworkSession>& session, uint64_t epoch) const;
	void DispatchCefReadyMainThread(const std::shared_ptr<NetworkSession>& session, uint64_t epoch);

    void BeginDownloadUi(int playerid);
    void EndDownloadUi(int playerid);

private:
	std::unique_ptr<IPlatformBridge> bridge_;
	std::unique_ptr<SecurityManager> security_;
	std::unique_ptr<NetworkSessionManager> sessions_;
	std::unique_ptr<CefApi> api_;
	std::unique_ptr<ResourceManager> resource_;

	Logger logger_;

	std::vector<uint8_t> master_resource_key_;
	struct PlayerUiState
	{
		bool spawnScreenVisible = true;
	};

	ResourceDownloadDialogManager resource_download_dialogs_;
	std::unordered_map<int, PlayerUiState> player_ui_states_;

	asio::io_context io_context_;
	asio::steady_timer transfer_timer_{ io_context_ };
	std::unique_ptr<NetworkServer> network_server_;
	std::thread network_thread_;
	std::atomic<bool> running_{ false };
	std::mutex main_thread_tasks_mutex_;
	std::deque<std::function<void()>> main_thread_tasks_;

	std::unordered_map<std::string, RegisteredEvent> registered_events_;
};
