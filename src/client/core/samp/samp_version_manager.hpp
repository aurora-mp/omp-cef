#pragma once

#include <Windows.h>

enum class SampVersion
{
    V037,     // SA-MP 0.3.7-R1
    V037R3,   // SA-MP 0.3.7-R3-1
    V037R5,   // SA-MP 0.3.7-R5-1
    V03DLR1,  // SA-MP 0.3.DL-R1
    Unknown
};

class SampVersionManager
{
public:
    SampVersionManager() = default;
    ~SampVersionManager() = default;

    SampVersionManager(const SampVersionManager&) = delete;
    SampVersionManager& operator=(const SampVersionManager&) = delete;

    bool Initialize();

    SampVersion GetVersion() const { return _version; }
    HMODULE     GetModule()  const { return _sampModule; }
    const char* GetVersionString() const;
    bool        IsValid() const { return _version != SampVersion::Unknown; }

private:
    SampVersion _version    = SampVersion::Unknown;
    HMODULE     _sampModule = nullptr;
};