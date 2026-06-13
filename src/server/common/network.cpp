#include "network.hpp"
#include "logger.hpp"

#include <shared/utils.hpp>

NetworkServer::NetworkServer(unsigned short port,
                             asio::io_context& context,
                             PacketHandler handler,
                             KcpTickHandler kcp_tick_handler)
    : handler_(std::move(handler)),
      kcp_tick_handler_(std::move(kcp_tick_handler)),
      io_context_(context),
      socket_(io_context_, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
      kcp_update_timer_(context)
{
}

NetworkServer::~NetworkServer()
{
    Stop();
}

void NetworkServer::Start()
{
    if (running_)
        return;

    running_ = true;

    DoReceive();
    DoKcpUpdate();
}

void NetworkServer::Stop()
{

    if (!running_.exchange(false))
        return;

    std::error_code error_code;

    kcp_update_timer_.cancel();

    error_code.clear();
    socket_.cancel(error_code);
    socket_.close(error_code);
}

void NetworkServer::SendTo(const asio::ip::udp::endpoint& addr, const char* data, int length)
{
    if (!running_)
        return;

    auto send_buffer = std::make_shared<std::vector<char>>(data, data + length);
    socket_.async_send_to(asio::buffer(*send_buffer),
        addr,
        [send_buffer](std::error_code ec, std::size_t)
        {
            if (ec && ec != asio::error::operation_aborted)
            {
                LOG_ERROR("[Network] Async send error: %s", ec.message().c_str());
            }
        });
}

void NetworkServer::DoReceive()
{
    socket_.async_receive_from(asio::buffer(recv_buffer_),
        remote_endpoint_,
        [this](std::error_code ec, std::size_t bytes_recvd)
        {
            if (running_ && !ec && bytes_recvd > 0)
            {
                try { 
                    handler_(remote_endpoint_, recv_buffer_.data(), (int)bytes_recvd); 
                }
                catch (const std::exception& e) { 
                    LOG_ERROR("[Network] handler exception: %s", e.what()); 
                }
                catch (...) { 
                    LOG_ERROR("[Network] handler unknown exception"); 
                }
            }

            if (running_ && ec != asio::error::operation_aborted)
            {
                DoReceive();
            }
        });
}

void NetworkServer::DoKcpUpdate()
{
    if (!running_)
        return;

    if (kcp_tick_handler_)
    {
        kcp_tick_handler_(iclock());
    }

    kcp_update_timer_.expires_after(std::chrono::milliseconds(10));
    kcp_update_timer_.async_wait(
        [this](const std::error_code& ec)
        {
            if (ec == asio::error::operation_aborted) 
                return;

            if (!ec && running_)
            {
                DoKcpUpdate();
            }
        });
}
