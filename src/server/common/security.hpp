#pragma once

#include <vector>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <asio.hpp>

struct KeyExchangeState 
{
    std::vector<uint8_t> server_private_key;
    std::vector<uint8_t> server_public_key;
};

struct SessionKeys 
{
    std::vector<uint8_t> rx;
    std::vector<uint8_t> tx;
};

class SecurityManager {
public:
    SecurityManager() = default;
    ~SecurityManager() = default;

    void Initialize(asio::io_context& io);
    void Shutdown();

    std::vector<uint8_t> GenerateCookie(const asio::ip::udp::endpoint& client_endpoint);
    bool ValidateCookie(const asio::ip::udp::endpoint& client_endpoint, const std::vector<uint8_t>& cookie);

    std::vector<uint8_t> InitiateKeyExchange(int playerid);
    std::unique_ptr<SessionKeys> FinalizeKeyExchange(int playerid, const std::vector<uint8_t>& client_public_key);

private:
    void RotateCookieSecret();

    std::unique_ptr<asio::steady_timer> secret_rotation_timer_;
    std::vector<uint8_t> cookie_secret_;
    std::mutex cookie_mutex_;

    std::unordered_map<int, KeyExchangeState> pending_key_exchanges_;
    std::mutex kx_mutex_;
};