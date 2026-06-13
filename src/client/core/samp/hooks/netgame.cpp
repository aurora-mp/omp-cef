#include "netgame.hpp"

#include "hooks/hook_manager.hpp"
#include "samp/addresses.hpp"
#include "system/logger.hpp"
#include "system/resource_manager.hpp"

bool NetGameHook::Initialize()
{
    s_self_ = this;

    auto& addrs = SampAddresses::Instance();
    auto* base = addrs.Base();
    void* addrProcessGameStuff = nullptr;

    switch (addrs.Version())
    {
        case SampVersion::V037:
            addrProcessGameStuff = reinterpret_cast<void*>(base + 0x8680);
            break;
        case SampVersion::V037R3:
            addrProcessGameStuff = reinterpret_cast<void*>(base + 0x87B0);
            break;
        case SampVersion::V037R5:
            addrProcessGameStuff = reinterpret_cast<void*>(base + 0x8B20);
            break;
        case SampVersion::V03DLR1:
            addrProcessGameStuff = reinterpret_cast<void*>(base + 0x8810);
            break;
        default:
            break;
    }

    if (!addrProcessGameStuff)
    {
        LOG_FATAL("[NetGameHook] Unsupported SA:MP version for ProcessGameStuff.");
        return false;
    }

    if (!hooks_.Install(
            "NetGameHook::ProcessGameStuff", addrProcessGameStuff, reinterpret_cast<void*>(&Hook_ProcessGameStuff)))
    {
        LOG_ERROR("[NetGameHook] Failed to install ProcessGameStuff hook.");
        return false;
    }

    s_orig_ = reinterpret_cast<FnProcessGameStuff>(hooks_.GetOriginal("NetGameHook::ProcessGameStuff"));
    if (!s_orig_)
    {
        LOG_FATAL("[NetGameHook] Failed to get original ProcessGameStuff pointer.");
        hooks_.Uninstall("NetGameHook::ProcessGameStuff");
        return false;
    }

    return true;
}

void NetGameHook::Shutdown()
{
    hooks_.Uninstall("NetGameHook::ProcessGameStuff");
    s_orig_ = nullptr;
    s_self_ = nullptr;
}

void NetGameHook::SetSessionActive(bool active)
{
    session_active_.store(active);

    if (!active)
        cef_download_triggered_ = false;
}

void __fastcall NetGameHook::Hook_ProcessGameStuff(void* pThis, void* /*_edx*/)
{
    if (!s_self_ || !s_orig_)
        return;

    const auto state = s_self_->resources_.GetState();

    if (!s_self_->session_active_.load() || state == DownloadState::IDLE)
    {
        s_orig_(pThis);
        return;
    }

    if (!s_self_->cef_download_triggered_ && state == DownloadState::AWAITING_TRIGGER)
    {
        s_self_->cef_download_triggered_ = true;
        s_self_->resources_.TriggerDownload();
    }

    if (state != DownloadState::COMPLETED)
        return;

    s_orig_(pThis);
}