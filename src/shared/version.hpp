#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

#define VERSION_MAJOR 1
#define VERSION_MINOR 2
#define VERSION_PATCH 0
#define VERSION_BUILD 0

static constexpr uint32_t MakeVersionU32(uint8_t maj, uint8_t min, uint8_t patch, uint8_t build)
{
    return (uint32_t(maj) << 24) | (uint32_t(min) << 16) | (uint32_t(patch) << 8) | uint32_t(build);
}

static constexpr uint32_t PLUGIN_VERSION_U32 =
    MakeVersionU32(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_BUILD);

static inline std::string VersionToString(uint32_t v)
{
    const uint8_t maj   = uint8_t((v >> 24) & 0xFF);
    const uint8_t min   = uint8_t((v >> 16) & 0xFF);
    const uint8_t patch = uint8_t((v >> 8) & 0xFF);
    const uint8_t build = uint8_t((v >> 0) & 0xFF);

    char buf[32]{};
    std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u", maj, min, patch, build);
    return std::string(buf);
}