#pragma once

#include <vector>
#include <string>
#include <array>
#include <sodium.h>
#include <stdexcept>
#include <aes.hpp>

constexpr size_t AES_IV_BYTES = 16;

constexpr size_t COOKIE_KEY_BYTES = crypto_aead_xchacha20poly1305_ietf_KEYBYTES;
constexpr size_t COOKIE_NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
constexpr size_t COOKIE_MAC_BYTES = crypto_aead_xchacha20poly1305_ietf_ABYTES;

constexpr size_t PACKET_KEY_BYTES = crypto_secretbox_KEYBYTES;
constexpr size_t PACKET_NONCE_BYTES = crypto_secretbox_NONCEBYTES;
constexpr size_t PACKET_MAC_BYTES = crypto_secretbox_MACBYTES;

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
	AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key.data(), iv.data());
	std::vector<uint8_t> padded_data = data;
	PadDataTo16(padded_data);
	AES_CBC_encrypt_buffer(&ctx, padded_data.data(), padded_data.size());
	return padded_data;
}

inline std::vector<uint8_t> DecryptFile(const std::vector<uint8_t>& encrypted_data, const std::vector<uint8_t>& key, const std::array<uint8_t, AES_IV_BYTES>& iv)
{
	if (encrypted_data.size() % AES_BLOCKLEN != 0) return {};
	AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key.data(), iv.data());
	std::vector<uint8_t> data = encrypted_data;
	AES_CBC_decrypt_buffer(&ctx, data.data(), data.size());
	if (!UnpadData(data)) return {};
	return data;
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