#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <vector>
#include <cstdint>
#include <memory>
#include <atomic>
#include <functional>
#include <asio.hpp>
#include <kcp/ikcp.h>

constexpr size_t FILE_CHUNK_SIZE = 1200;

int kcp_output_callback(const char* buf, int len, ikcpcb* kcp, void* user);

enum class HandshakeStatus : uint8_t
{
	NONE,
	CHALLENGED,
	CONNECTED
};

struct FileTransfer
{
	std::string resourceName;
	std::string relativePath;
	std::string fileHash;
	std::vector<uint8_t> content;
	uint32_t totalChunks = 0;
	uint32_t currentChunkIndex = 0;
};

struct NetworkSession
{
	int playerid = -1;
	asio::ip::udp::endpoint address;
	ikcpcb* kcp_instance = nullptr;

	HandshakeStatus handshake_status{HandshakeStatus::NONE};
	std::atomic_bool handshake_complete{false};
	std::atomic_bool cef_init_timer_started{false};
	bool cef_init_notified{false};
	bool cef_success{false};

	std::vector<uint8_t> rx_key;
	std::vector<uint8_t> tx_key;

	std::function<void(const asio::ip::udp::endpoint&, const char*, int)> send_fn;

	std::queue<std::shared_ptr<FileTransfer>> download_queue;
	std::shared_ptr<FileTransfer> current_transfer = nullptr;
	std::atomic<bool> is_download_paused{false};

	bool chat_input_open{false};

	std::mutex kcp_mutex;
};

class NetworkSessionManager
{
public:
	NetworkSessionManager() = default;
	~NetworkSessionManager() = default;
	NetworkSessionManager(const NetworkSessionManager&) = delete;
	NetworkSessionManager& operator=(const NetworkSessionManager&) = delete;

	void SetSender(std::function<void(const asio::ip::udp::endpoint&, const char*, int)> fn);
	void RegisterPlayer(int playerid);
	void RemovePlayer(int playerid);

	void UpdateAllKcpInstances(uint32_t now_ms);
	std::shared_ptr<NetworkSession> GetOrCreateSession(int playerid);
	std::shared_ptr<NetworkSession> GetSessionFromAddress(const asio::ip::udp::endpoint& addr);
	std::shared_ptr<NetworkSession> GetSession(int playerid);
	std::vector<std::shared_ptr<NetworkSession>> GetAllSessions();
	bool HasPlayerPlugin(int playerid) const;
	void MapAddressToPlayer(int playerid, const asio::ip::udp::endpoint& addr);
	void SetDownloadPaused(int playerid, bool paused);

private:
	void UnmapAddress(const asio::ip::udp::endpoint& addr);
	std::string EndpointToStr(const asio::ip::udp::endpoint& addr) const;

	mutable std::mutex mutex_;

	std::function<void(const asio::ip::udp::endpoint&, const char*, int)> send_fn_;
	std::unordered_map<int, std::shared_ptr<NetworkSession>> player_sessions_;
	std::unordered_map<std::string, int> addr_str_to_playerid_;
};