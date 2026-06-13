#include "addresses.hpp"
#include "system/logger.hpp"
#include <cassert>
#include <windows.h>

void SampAddresses::Initialize(const SampVersionManager& svm)
{
    HMODULE mod = svm.GetModule();
    version_ = svm.GetVersion();

    if (!mod || version_ == SampVersion::Unknown) {
        base_ = nullptr;
        LOG_ERROR("SampAddresses: invalid module or unknown version.");
        return;
    }

    base_ = reinterpret_cast<uint8_t*>(mod);
    LOG_DEBUG("SampAddresses initialized (base={}, version={})", static_cast<void*>(base_), svm.GetVersionString());
}

void* SampAddresses::SampInitialization() const noexcept
{
    assert(base_ != nullptr);
    switch (version_) {
        case SampVersion::V037:    
            return base_ + 0x2565E2;
        case SampVersion::V037R3:  
            return base_ + 0x0C57E2;
        case SampVersion::V037R5:  
            return base_ + 0x0C57E2;
        case SampVersion::V03DLR1: 
            return base_ + 0x0C6614;
        default: return nullptr;
    }
}

void* SampAddresses::SampDeinitialization() const noexcept
{
    assert(base_ != nullptr);
    switch (version_) {
        case SampVersion::V037:    
            return base_ + 0x009380;
        case SampVersion::V037R3:  
            return base_ + 0x009510;
        case SampVersion::V037R5:  
            return base_ + 0x009880;
        case SampVersion::V03DLR1: 
            return base_ + 0x009570;
        default: return nullptr;
    }
}