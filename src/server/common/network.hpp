#pragma once

#include <asio.hpp>
#include <asio/steady_timer.hpp>
#include <functional>
#include <atomic>
#include <array>

using PacketHandler = std::function<void(const asio::ip::udp::endpoint&, const char*, int)>;

class NetworkServer
{
public:
    using KcpTickHandler = std::function<void(uint32_t)>;

    NetworkServer(unsigned short port,
                  asio::io_context& context,
                  PacketHandler handler,
                  KcpTickHandler kcp_tick_handler);

    ~NetworkServer();

    void Start();
    void Stop();
    void SendTo(const asio::ip::udp::endpoint& addr, const char* data, int length);

private:
    void DoReceive();
    void DoKcpUpdate();

    std::atomic<bool> running_ = false;

    PacketHandler handler_;
    KcpTickHandler kcp_tick_handler_;

    asio::io_context& io_context_;
    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint remote_endpoint_;
    std::array<char, 65535> recv_buffer_;
    asio::steady_timer kcp_update_timer_;
};