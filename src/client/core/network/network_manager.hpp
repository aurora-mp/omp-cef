#pragma once

#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <string>
#include <array>
#include <mutex>

#include <asio.hpp>
#include <asio/steady_timer.hpp>
#include <ikcp.h>
#include "shared/packet.hpp"

class ResourceManager;

enum class ConnectionState
{
	DISCONNECTED,
	SENDING_JOIN,
	AWAITING_CHALLENGE,
	AWAITING_ACCEPTANCE,
	CONNECTED
};

class NetworkManager
{
public:
	using PacketHandler = std::function<void(const NetworkPacket&)>;

	NetworkManager(ResourceManager& resource);
	~NetworkManager();

	NetworkManager(const NetworkManager&) = delete;
	NetworkManager& operator=(const NetworkManager&) = delete;

	bool Initialize(const std::string& ip, unsigned short port);
	void Shutdown();

	void Connect(int playerid);
	void Disconnect();
	void SendPacket(PacketType type, const PacketPayload& payload);
	void SetPacketHandler(PacketHandler handler);
	void SendRaw(const char* data, int size);
	void SendBrowserCreateResult(int browserId, bool success, int code, const std::string& reason);

	bool IsNonCefServer() const { return non_cef_server_.load(); }

	using SessionActiveHandler = std::function<void(bool)>;
	void SetSessionActiveHandler(SessionActiveHandler handler);

	ConnectionState GetState() const { return state_; }

private:
	void DoReceive();
	void DoSendRequestJoin();
	void DoKcpUpdate();

	void HandleRawMessage(const char* data, size_t len);
	void HandleKcpInput();

	void FireSessionActive(bool active);

private:
	ResourceManager& resource_;

	std::atomic<ConnectionState> state_{ ConnectionState::DISCONNECTED };
	std::atomic<bool> running_{ false };
	int playerid_ = -1;

	asio::io_context io_context_;
	asio::ip::udp::socket socket_{ io_context_ };
	asio::ip::udp::endpoint server_endpoint_;
	asio::ip::udp::endpoint remote_endpoint_;
	std::thread network_thread_;
	std::array<char, 65535> recv_buffer_{};

	asio::steady_timer connect_timer_{ io_context_ };
	asio::steady_timer kcp_update_timer_{ io_context_ };

	std::mutex kcp_mutex_;
	ikcpcb* kcp_instance_ = nullptr;

	std::vector<uint8_t> rx_key_;
	std::vector<uint8_t> tx_key_;
	std::vector<uint8_t> client_public_key_;
	std::vector<uint8_t> client_private_key_;

	PacketHandler packet_handler_;
	std::mutex handler_mutex_;

	int join_attempts_ = 0;
	static constexpr int MAX_JOIN_ATTEMPTS = 5;

	std::atomic<bool> non_cef_server_{ false };

	SessionActiveHandler session_handler_;
	std::mutex session_mutex_;
};
