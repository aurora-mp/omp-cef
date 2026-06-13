#pragma once

#include <sodium.h>
#include <vector>
#include <stdexcept>

struct ClientKeys
{
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> private_key;
};

struct SessionKeys
{
    std::vector<uint8_t> rx; 
    std::vector<uint8_t> tx;
};

class SecurityManager
{
public:
    static void Initialize()
    {
        if (sodium_init() < 0) {
            throw std::runtime_error("Failed to initialize libsodium on client");
        }
    }

    static ClientKeys GenerateKeys()
    {
        ClientKeys keys;
        keys.public_key.resize(crypto_kx_PUBLICKEYBYTES);
        keys.private_key.resize(crypto_kx_SECRETKEYBYTES);
        crypto_kx_keypair(keys.public_key.data(), keys.private_key.data());
        return keys;
    }

    static SessionKeys GenerateClientSessionKeys(
        const std::vector<uint8_t>& client_pk,
        const std::vector<uint8_t>& client_sk,
        const std::vector<uint8_t>& server_pk)
    {
        SessionKeys session_keys;
        session_keys.rx.resize(crypto_kx_SESSIONKEYBYTES);
        session_keys.tx.resize(crypto_kx_SESSIONKEYBYTES);

        if (crypto_kx_client_session_keys(
            session_keys.rx.data(),
            session_keys.tx.data(),
            client_pk.data(),
            client_sk.data(),
            server_pk.data()) != 0)
        {
            return {};
        }

        return session_keys;
    }
};