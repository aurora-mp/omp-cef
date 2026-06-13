#include "samp_version_manager.hpp"
#include "system/logger.hpp"

#include <vector>

bool SampVersionManager::Initialize()
{
    while ((_sampModule = ::GetModuleHandleA("samp.dll")) == nullptr)
        ::Sleep(100);

    if (!_sampModule)
    {
        LOG_ERROR("samp.dll module handle is null.");
        _version = SampVersion::Unknown;
        return false;
    }

    wchar_t path[MAX_PATH]{};
    if (GetModuleFileNameW(_sampModule, path, MAX_PATH) == 0)
    {
        LOG_ERROR("Failed to retrieve samp.dll path.");
        _version = SampVersion::Unknown;
        return false;
    }

    DWORD dummy = 0;
    DWORD size = GetFileVersionInfoSizeW(path, &dummy);
    if (size == 0)
    {
        LOG_ERROR("Failed to retrieve file version info size.");
        _version = SampVersion::Unknown;
        return false;
    }

    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoW(path, 0, size, buffer.data()))
    {
        LOG_ERROR("Failed to read file version info.");
        _version = SampVersion::Unknown;
        return false;
    }

    VS_FIXEDFILEINFO* fileInfo = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<LPVOID*>(&fileInfo), &len))
    {
        LOG_ERROR("Failed to query version info.");
        _version = SampVersion::Unknown;
        return false;
    }

    const int major    = (fileInfo->dwFileVersionMS >> 16) & 0xFFFF;
    const int minor    = (fileInfo->dwFileVersionMS >>  0) & 0xFFFF;
    const int build    = (fileInfo->dwFileVersionLS >> 16) & 0xFFFF;
    const int revision = (fileInfo->dwFileVersionLS >>  0) & 0xFFFF;

    LOG_DEBUG("SA:MP version detected : {}.{}.{}.{}", major, minor, build, revision);

    if      (major == 0 && minor == 3 && build == 7 && revision == 0) 
        _version = SampVersion::V037;
    else if (major == 0 && minor == 3 && build == 7 && revision == 2) 
        _version = SampVersion::V037R3;
    else if (major == 0 && minor == 3 && build == 7 && revision == 5) 
        _version = SampVersion::V037R5;
    else if (major == 0 && minor == 3 && build == 8 && revision == 0) 
        _version = SampVersion::V03DLR1;
    else                                                              
        _version = SampVersion::Unknown;

    LOG_INFO("Detected SA-MP version : {}", GetVersionString());
    return true;
}

const char* SampVersionManager::GetVersionString() const
{
    switch (_version)
    {
        case SampVersion::V037:    
            return "0.3.7-R1";
        case SampVersion::V037R3:  
            return "0.3.7-R3-1";
        case SampVersion::V037R5:  
            return "0.3.7-R5-1";
        case SampVersion::V03DLR1: 
            return "0.3.DL-R1";
        default:                   
            return "Unknown";
    }
}