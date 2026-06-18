#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <aes.hpp>
#include <sodium.h>

constexpr size_t AES_IV_BYTES = 16;

constexpr size_t COOKIE_KEY_BYTES = crypto_aead_xchacha20poly1305_ietf_KEYBYTES;
constexpr size_t COOKIE_NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
constexpr size_t COOKIE_MAC_BYTES = crypto_aead_xchacha20poly1305_ietf_ABYTES;

constexpr size_t PACKET_KEY_BYTES = crypto_secretbox_KEYBYTES;
constexpr size_t PACKET_NONCE_BYTES = crypto_secretbox_NONCEBYTES;
constexpr size_t PACKET_MAC_BYTES = crypto_secretbox_MACBYTES;

constexpr std::array<uint8_t, 5> RESOURCE_ENTRY_MAGIC = { 'O', 'C', 'E', 'F', '2' };
constexpr uint8_t RESOURCE_ENTRY_FLAG_ENCRYPTED = 1 << 0;
constexpr size_t RESOURCE_ENTRY_FLAGS_BYTES = 1;
constexpr size_t RESOURCE_ENTRY_HEADER_BYTES = RESOURCE_ENTRY_MAGIC.size() + RESOURCE_ENTRY_FLAGS_BYTES;

inline void PadDataTo16(std::vector<uint8_t>& data)
{
    size_t remainder = data.size() % AES_BLOCKLEN;
    size_t padding_needed = (remainder == 0) ? AES_BLOCKLEN : (AES_BLOCKLEN - remainder);
    for (size_t i = 0; i < padding_needed; ++i) {
        data.push_back(static_cast<uint8_t>(padding_needed));
    }
}

inline bool UnpadData(std::vector<uint8_t>& data)
{
    if (data.empty()) return false;
    uint8_t padding_value = data.back();
    if (padding_value == 0 || padding_value > AES_BLOCKLEN || padding_value > data.size()) return false;

    for (size_t i = 0; i < padding_value; ++i) {
        if (data[data.size() - 1 - i] != padding_value) return false;
    }
    data.resize(data.size() - padding_value);
    return true;
}

inline std::vector<uint8_t> EncryptFile(const std::vector<uint8_t>& data, const std::vector<uint8_t>& key, const std::array<uint8_t, AES_IV_BYTES>& iv)
{
    if (key.empty()) return {};

    AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key.data(), iv.data());
    std::vector<uint8_t> padded_data = data;
    PadDataTo16(padded_data);
    AES_CBC_encrypt_buffer(&ctx, padded_data.data(), padded_data.size());
    return padded_data;
}

inline std::vector<uint8_t> DecryptFile(const std::vector<uint8_t>& encrypted_data, const std::vector<uint8_t>& key, const std::array<uint8_t, AES_IV_BYTES>& iv)
{
    if (key.empty() || encrypted_data.size() % AES_BLOCKLEN != 0) return {};

    AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key.data(), iv.data());
    std::vector<uint8_t> data = encrypted_data;
    AES_CBC_decrypt_buffer(&ctx, data.data(), data.size());
    if (!UnpadData(data)) return {};
    return data;
}

inline bool HasResourceEntryMagic(const std::vector<uint8_t>& data)
{
    return data.size() >= RESOURCE_ENTRY_HEADER_BYTES &&
        std::equal(RESOURCE_ENTRY_MAGIC.begin(), RESOURCE_ENTRY_MAGIC.end(), data.begin());
}

inline void AppendResourceEntryHeader(std::vector<uint8_t>& output, uint8_t flags)
{
    output.insert(output.end(), RESOURCE_ENTRY_MAGIC.begin(), RESOURCE_ENTRY_MAGIC.end());
    output.push_back(flags);
}

inline std::vector<uint8_t> EncodeResourceEntry(
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& master_key,
    bool encrypt)
{
    std::vector<uint8_t> output;

    if (!encrypt)
    {
        output.reserve(RESOURCE_ENTRY_HEADER_BYTES + plaintext.size());
        AppendResourceEntryHeader(output, 0);
        output.insert(output.end(), plaintext.begin(), plaintext.end());
        return output;
    }

    if (master_key.empty())
    {
        return {};
    }

    std::array<uint8_t, AES_IV_BYTES> iv{};
    randombytes_buf(iv.data(), iv.size());

    std::vector<uint8_t> encrypted_payload = EncryptFile(plaintext, master_key, iv);
    if (encrypted_payload.empty() && !plaintext.empty())
    {
        return {};
    }

    output.reserve(RESOURCE_ENTRY_HEADER_BYTES + iv.size() + encrypted_payload.size());
    AppendResourceEntryHeader(output, RESOURCE_ENTRY_FLAG_ENCRYPTED);
    output.insert(output.end(), iv.begin(), iv.end());
    output.insert(output.end(), encrypted_payload.begin(), encrypted_payload.end());
    return output;
}

inline bool DecodeLegacyEncryptedResourceEntry(
    const std::vector<uint8_t>& input,
    const std::vector<uint8_t>& master_key,
    std::vector<uint8_t>& output)
{
    output.clear();

    if (master_key.empty() || input.size() < AES_IV_BYTES)
    {
        return false;
    }

    std::array<uint8_t, AES_IV_BYTES> iv{};
    std::copy_n(input.begin(), AES_IV_BYTES, iv.begin());

    std::vector<uint8_t> ciphertext(input.begin() + static_cast<std::ptrdiff_t>(AES_IV_BYTES), input.end());
    output = DecryptFile(ciphertext, master_key, iv);
    return !output.empty() || ciphertext.size() == AES_BLOCKLEN;
}

inline bool DecodeResourceEntry(
    const std::vector<uint8_t>& input,
    const std::vector<uint8_t>& master_key,
    std::vector<uint8_t>& output)
{
    output.clear();

    if (!HasResourceEntryMagic(input))
    {
        return DecodeLegacyEncryptedResourceEntry(input, master_key, output);
    }

    const uint8_t flags = input[RESOURCE_ENTRY_MAGIC.size()];
    const size_t payload_offset = RESOURCE_ENTRY_HEADER_BYTES;

    if ((flags & RESOURCE_ENTRY_FLAG_ENCRYPTED) == 0)
    {
        output.assign(input.begin() + static_cast<std::ptrdiff_t>(payload_offset), input.end());
        return true;
    }

    if (master_key.empty() || input.size() < payload_offset + AES_IV_BYTES)
    {
        return false;
    }

    std::array<uint8_t, AES_IV_BYTES> iv{};
    std::copy_n(input.begin() + static_cast<std::ptrdiff_t>(payload_offset), AES_IV_BYTES, iv.begin());

    const size_t ciphertext_offset = payload_offset + AES_IV_BYTES;
    std::vector<uint8_t> ciphertext(input.begin() + static_cast<std::ptrdiff_t>(ciphertext_offset), input.end());

    output = DecryptFile(ciphertext, master_key, iv);
    return !output.empty() || ciphertext.size() == AES_BLOCKLEN;
}

inline std::vector<uint8_t> EncryptCookie(const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& key)
{
    if (key.size() != COOKIE_KEY_BYTES) return {};

    std::vector<uint8_t> nonce(COOKIE_NONCE_BYTES);
    randombytes_buf(nonce.data(), nonce.size());

    std::vector<uint8_t> ciphertext(plaintext.size() + COOKIE_MAC_BYTES);
    unsigned long long ciphertext_len;

    crypto_aead_xchacha20poly1305_ietf_encrypt(
        ciphertext.data(), &ciphertext_len,
        plaintext.data(), plaintext.size(),
        nullptr, 0,
        nullptr, nonce.data(), key.data());

    ciphertext.resize(ciphertext_len);

    std::vector<uint8_t> result;
    result.reserve(nonce.size() + ciphertext.size());
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

inline std::vector<uint8_t> DecryptCookie(const std::vector<uint8_t>& ciphertext_with_nonce, const std::vector<uint8_t>& key)
{
    if (key.size() != COOKIE_KEY_BYTES || ciphertext_with_nonce.size() < COOKIE_NONCE_BYTES + COOKIE_MAC_BYTES) {
        return {};
    }

    const uint8_t* nonce = ciphertext_with_nonce.data();
    const uint8_t* ciphertext = ciphertext_with_nonce.data() + COOKIE_NONCE_BYTES;
    size_t ciphertext_len = ciphertext_with_nonce.size() - COOKIE_NONCE_BYTES;

    std::vector<uint8_t> decrypted(ciphertext_len);
    unsigned long long decrypted_len;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
        decrypted.data(), &decrypted_len,
        nullptr,
        ciphertext, ciphertext_len,
        nullptr, 0,
        nonce, key.data()) != 0) {
        return {};
    }

    decrypted.resize(decrypted_len);
    return decrypted;
}

inline std::vector<uint8_t> EncryptPacket(const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& key)
{
    if (key.size() != PACKET_KEY_BYTES) return {};

    std::vector<uint8_t> nonce(PACKET_NONCE_BYTES);
    randombytes_buf(nonce.data(), nonce.size());

    std::vector<uint8_t> ciphertext(plaintext.size() + PACKET_MAC_BYTES);

    crypto_secretbox_easy(ciphertext.data(), plaintext.data(), plaintext.size(), nonce.data(), key.data());

    std::vector<uint8_t> result;
    result.reserve(nonce.size() + ciphertext.size());
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

inline std::vector<uint8_t> DecryptPacket(const std::vector<uint8_t>& ciphertext_with_nonce, const std::vector<uint8_t>& key)
{
    if (key.size() != PACKET_KEY_BYTES || ciphertext_with_nonce.size() < PACKET_NONCE_BYTES + PACKET_MAC_BYTES) {
        return {};
    }

    const uint8_t* nonce = ciphertext_with_nonce.data();
    const uint8_t* ciphertext = ciphertext_with_nonce.data() + PACKET_NONCE_BYTES;
    size_t ciphertext_len = ciphertext_with_nonce.size() - PACKET_NONCE_BYTES;

    std::vector<uint8_t> decrypted(ciphertext_len - PACKET_MAC_BYTES);

    if (crypto_secretbox_open_easy(decrypted.data(), ciphertext, ciphertext_len, nonce, key.data()) != 0) {
        return {};
    }

    return decrypted;
}
