#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <sodium.h>

inline bool EnsureSodiumInitialized()
{
    static const bool initialized = (sodium_init() >= 0);
    return initialized;
}

inline std::string BytesToHex(const uint8_t* data, size_t size)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');

    for (size_t i = 0; i < size; ++i)
    {
        stream << std::setw(2) << static_cast<int>(data[i]);
    }

    return stream.str();
}

inline std::string CalculateSHA256FromData(const std::vector<uint8_t>& data)
{
    if (!EnsureSodiumInitialized())
    {
        return "";
    }

    std::vector<uint8_t> hash(crypto_hash_sha256_BYTES);
    if (crypto_hash_sha256(hash.data(), data.data(), static_cast<unsigned long long>(data.size())) != 0)
    {
        return "";
    }

    return BytesToHex(hash.data(), hash.size());
}

inline std::string CalculateSHA256(const std::string& filePath)
{
    if (!EnsureSodiumInitialized())
    {
        return "";
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        return "";
    }

    crypto_hash_sha256_state state;
    if (crypto_hash_sha256_init(&state) != 0)
    {
        return "";
    }

    std::vector<uint8_t> buffer(64 * 1024);

    while (file.good())
    {
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

        const std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0)
        {
            crypto_hash_sha256_update(&state, buffer.data(), static_cast<unsigned long long>(bytes_read));
        }
    }

    if (file.bad())
    {
        return "";
    }

    std::vector<uint8_t> hash(crypto_hash_sha256_BYTES);
    if (crypto_hash_sha256_final(&state, hash.data()) != 0)
    {
        return "";
    }

    return BytesToHex(hash.data(), hash.size());
}

inline std::string FormatBytes(uint64_t bytes)
{
    if (bytes == 0) {
        return "0 bytes";
    }

    const double kb = 1024.0;
    const double mb = 1024.0 * 1024.0;
    const double gb = 1024.0 * 1024.0 * 1024.0;

    double size = static_cast<double>(bytes);

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2);

    if (size >= gb) {
        stream << (size / gb) << " GB";
    }
    else if (size >= mb) {
        stream << (size / mb) << " MB";
    }
    else if (size >= kb) {
        stream << (size / kb) << " KB";
    }
    else {
        stream << std::setprecision(0) << size << " bytes";
    }

    return stream.str();
}

inline uint32_t iclock()
{
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}
