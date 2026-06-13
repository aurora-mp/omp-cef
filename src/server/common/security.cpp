#include "security.hpp"

#include <asio.hpp>

#include <sodium.h>

#include "shared/crypto.hpp"

void SecurityManager::Initialize(asio::io_context& io)
{
    if (sodium_init() < 0)
    {
        throw std::runtime_error("Failed to initialize libsodium");
    }

    secret_rotation_timer_ = std::make_unique<asio::steady_timer>(io);
    RotateCookieSecret();
}

void SecurityManager::Shutdown()
{
    if (secret_rotation_timer_)
    {
        secret_rotation_timer_->cancel();
    }
}

void SecurityManager::RotateCookieSecret()
{
    std::lock_guard<std::mutex> lock(cookie_mutex_);
    cookie_secret_.resize(32);
    randombytes_buf(cookie_secret_.data(), cookie_secret_.size());

    secret_rotation_timer_->expires_after(std::chrono::hours(1));
    secret_rotation_timer_->async_wait(
        [this](const std::error_code& ec)
        {
            if (!ec)
            {
                RotateCookieSecret();
            }
        });
}

std::vector<uint8_t> SecurityManager::GenerateCookie(const asio::ip::udp::endpoint& client_endpoint)
{
    std::vector<uint8_t> plaintext;
    auto address_bytes = client_endpoint.address().to_v4().to_bytes();
    plaintext.insert(plaintext.end(), address_bytes.begin(), address_bytes.end());

    uint16_t port = client_endpoint.port();
    plaintext.push_back(static_cast<uint8_t>(port >> 8));
    plaintext.push_back(static_cast<uint8_t>(port & 0xFF));

    std::lock_guard<std::mutex> lock(cookie_mutex_);
    return EncryptCookie(plaintext, cookie_secret_);
}

bool SecurityManager::ValidateCookie(const asio::ip::udp::endpoint& client_endpoint, const std::vector<uint8_t>& cookie)
{
    std::vector<uint8_t> decrypted_cookie;
    {
        std::lock_guard<std::mutex> lock(cookie_mutex_);
        decrypted_cookie = DecryptCookie(cookie, cookie_secret_);
    }

    if (decrypted_cookie.empty())
        return false;

    try
    {
        asio::ip::address_v4::bytes_type address_bytes{};
        std::copy_n(decrypted_cookie.begin(), 4, address_bytes.begin());
        asio::ip::address_v4 address(address_bytes);

        uint16_t port = (static_cast<uint16_t>(decrypted_cookie[4]) << 8) | decrypted_cookie[5];

        asio::ip::udp::endpoint cookie_endpoint(address, port);
        return cookie_endpoint == client_endpoint;
    }
    catch (...)
    {
        return false;
    }
}

std::vector<uint8_t> SecurityManager::InitiateKeyExchange(int playerid)
{
    KeyExchangeState state;
    state.server_public_key.resize(crypto_kx_PUBLICKEYBYTES);
    state.server_private_key.resize(crypto_kx_SECRETKEYBYTES);
    crypto_kx_keypair(state.server_public_key.data(), state.server_private_key.data());

    std::lock_guard<std::mutex> lock(kx_mutex_);
    pending_key_exchanges_[playerid] = std::move(state);
    return pending_key_exchanges_[playerid].server_public_key;
}

std::unique_ptr<SessionKeys> SecurityManager::FinalizeKeyExchange(int playerid,
                                                                  const std::vector<uint8_t>& client_public_key)
{
    std::lock_guard<std::mutex> lock(kx_mutex_);
    auto it = pending_key_exchanges_.find(playerid);
    if (it == pending_key_exchanges_.end())
        return nullptr;

    KeyExchangeState state = std::move(it->second);
    pending_key_exchanges_.erase(it);

    auto keys = std::make_unique<SessionKeys>();
    keys->rx.resize(crypto_kx_SESSIONKEYBYTES);
    keys->tx.resize(crypto_kx_SESSIONKEYBYTES);

    if (crypto_kx_server_session_keys(keys->rx.data(),
                                      keys->tx.data(),
                                      state.server_public_key.data(),
                                      state.server_private_key.data(),
                                      client_public_key.data()) != 0)
    {
        return nullptr;
    }
    return keys;
}