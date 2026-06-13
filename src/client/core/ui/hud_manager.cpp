#include "hud_manager.hpp"
#include <samp/addresses.hpp>

void HudManager::Initialize()
{
    patches_[EHudComponent::AMMO] = { 0x5893B0, {}, {0xC3}, false };
    patches_[EHudComponent::ARMOUR] = { 0x5890A0, {}, {0xC3}, false };
    patches_[EHudComponent::BREATH] = { 0x589190, {}, {0xC3}, false };
    patches_[EHudComponent::CROSSHAIR] = { 0x58E020, {}, {0xC3}, false };
    patches_[EHudComponent::HEALTH] = { 0x589270, {}, {0xC3}, false };
    patches_[EHudComponent::MONEY] = { 0x58F47D, {}, {0x90, 0xE9}, false };
    patches_[EHudComponent::RADAR] = { 0x58A330, {}, {0xC3}, false };
    patches_[EHudComponent::WANTED_STARS] = { 0x58D9A0, {}, {0xC3}, false };
    patches_[EHudComponent::WEAPON] = { 0x58D7D0, {}, {0xC3}, false };

    LOG_INFO("[HudManager] Initialized.");
}

void HudManager::ToggleComponent(EHudComponent component, bool toggle)
{
    if (component == EHudComponent::ALL) {
        for (const auto& kv : patches_) {
            ToggleComponent(kv.first, toggle);
        }

        return;
    }

    auto it = patches_.find(component);
    if (it == patches_.end())
        return;

    auto& patch = it->second;

    // Capture original bytes once so we can restore later
    if (!patch.original_saved) {
        patch.original_bytes.resize(patch.disabled_bytes.size());
        std::memcpy(patch.original_bytes.data(), reinterpret_cast<void*>(patch.address), patch.original_bytes.size());
        patch.original_saved = true;
    }

    if (toggle) {
        Patch(patch.address, patch.original_bytes);
    }
    else {
        Patch(patch.address, patch.disabled_bytes);
    }
}

void HudManager::SetClassSelectionVisible(bool visible)
{
    const auto version = SampAddresses::Instance().Version();
    const auto base = SampAddresses::Instance().Base();

    if (base == nullptr) {
        LOG_WARN("[HudManager] Cannot set class selection visibility: SA-MP base address is null.");
        return;
    }

    uintptr_t ptr_offset = 0;
    constexpr uintptr_t flag_offset = 0x13;

    switch (version)
    {
        case SampVersion::V037:    
            ptr_offset = 0x21A18C; 
            break;
        case SampVersion::V037R3:  
            ptr_offset = 0x26E974; 
            break;
        case SampVersion::V037R5:  
            ptr_offset = 0x26EC2C; 
            break;
        case SampVersion::V03DLR1: 
            ptr_offset = 0x2ACABC; 
            break;
        default:
            LOG_WARN("[HudManager] Unsupported SA-MP version for class selection visibility toggle.");
            return;
    }

    if (ptr_offset == 0) {
        LOG_WARN("[HudManager] Offset for this SA-MP version is not configured.");
        return;
    }

    try
    {
        auto pClassSelection = *reinterpret_cast<uintptr_t*>(base + ptr_offset);
        if (pClassSelection == 0)
            return;

        auto* pVisibilityFlag = reinterpret_cast<uint8_t*>(pClassSelection + flag_offset);
        *pVisibilityFlag = visible ? 1 : 0;

        LOG_DEBUG("[HudManager] Class selection visibility set to {}.", visible ? 1 : 0);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("[HudManager] Exception while toggling class selection visibility: {}", e.what());
    }
}

void HudManager::Patch(uintptr_t address, const std::vector<unsigned char>& data)
{
    if (!address || data.empty())
        return;

    DWORD oldProtect{};
    if (VirtualProtect(reinterpret_cast<void*>(address), data.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        std::memcpy(reinterpret_cast<void*>(address), data.data(), data.size());
        DWORD tmp{};
        VirtualProtect(reinterpret_cast<void*>(address), data.size(), oldProtect, &tmp);
    }
}
