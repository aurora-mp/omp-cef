#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "picosha2.h"

inline std::string CalculateSHA256(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    picosha2::hash256_one_by_one hasher;
    std::vector<char> buffer(8192);

    while (file.good()) {
        file.read(buffer.data(), buffer.size());

        std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0) {
            hasher.process(buffer.begin(), buffer.begin() + bytes_read);
        }
    }

    if (file.bad()) {
        return "";
    }

    hasher.finish();
    return picosha2::get_hash_hex_string(hasher);
}

inline std::string CalculateSHA256FromData(const std::vector<uint8_t>& data)
{
    return picosha2::hash256_hex_string(data);
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