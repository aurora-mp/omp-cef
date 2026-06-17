#include "scoreboard.hpp"

#include <cstdint>

#include "browser/manager.hpp"
#include "hooks/hook_manager.hpp"
#include "samp/addresses.hpp"
#include "system/logger.hpp"

namespace
{
    struct ScoreboardAddresses
    {
        std::uintptr_t enable = 0;
        std::uintptr_t draw = 0;
        std::uintptr_t close = 0;
    };

    ScoreboardAddresses GetScoreboardAddresses(SampVersion version)
    {
        switch (version)
        {
            case SampVersion::V037:
                return { 0x6AD30, 0x6AA10, 0x6A320 };
            case SampVersion::V037R3:
                return { 0x6EC80, 0x6E960, 0x6E270 };
            case SampVersion::V037R5:
                return { 0x6F3D0, 0x6F0B0, 0x6E9E0 };
            case SampVersion::V03DLR1:
                return { 0x6EE10, 0x6EAF0, 0x6E410 };
            default:
                return {};
        }
    }
}

bool ScoreboardHook::Initialize()
{
    s_self_ = this;

    auto& addrs = SampAddresses::Instance();
    auto* base = addrs.Base();
    if (!base)
    {
        LOG_ERROR("[ScoreboardHook] SA:MP base address is null.");
        s_self_ = nullptr;
        return false;
    }

    const auto offsets = GetScoreboardAddresses(addrs.Version());
    if (!offsets.enable || !offsets.draw || !offsets.close)
    {
        LOG_ERROR("[ScoreboardHook] Unsupported SA:MP version.");
        s_self_ = nullptr;
        return false;
    }

    auto* enable_addr = reinterpret_cast<void*>(base + offsets.enable);
    auto* draw_addr = reinterpret_cast<void*>(base + offsets.draw);
    s_close_ = reinterpret_cast<FnClose>(base + offsets.close);

    if (!hooks_.Install("ScoreboardHook::Enable", enable_addr, reinterpret_cast<void*>(&Hook_Enable)))
    {
        LOG_ERROR("[ScoreboardHook] Failed to install Enable hook.");
        s_self_ = nullptr;
        s_close_ = nullptr;
        return false;
    }

    if (!hooks_.Install("ScoreboardHook::Draw", draw_addr, reinterpret_cast<void*>(&Hook_Draw)))
    {
        LOG_ERROR("[ScoreboardHook] Failed to install Draw hook.");
        hooks_.Uninstall("ScoreboardHook::Enable");
        s_self_ = nullptr;
        s_close_ = nullptr;
        return false;
    }

    s_orig_enable_ = reinterpret_cast<FnEnable>(hooks_.GetOriginal("ScoreboardHook::Enable"));
    s_orig_draw_ = reinterpret_cast<FnDraw>(hooks_.GetOriginal("ScoreboardHook::Draw"));

    if (!s_orig_enable_ || !s_orig_draw_)
    {
        LOG_ERROR("[ScoreboardHook] Failed to retrieve original function pointers.");
        Shutdown();
        return false;
    }

    LOG_INFO("[ScoreboardHook] Native SA:MP scoreboard hooks installed.");
    return true;
}

void ScoreboardHook::Shutdown()
{
    hooks_.Uninstall("ScoreboardHook::Draw");
    hooks_.Uninstall("ScoreboardHook::Enable");

    s_orig_enable_ = nullptr;
    s_orig_draw_ = nullptr;
    s_close_ = nullptr;
    s_self_ = nullptr;
}

bool ScoreboardHook::IsNativeScoreboardEnabled(void* pThis)
{
    if (!pThis)
        return false;

    return *reinterpret_cast<BOOL*>(pThis) != FALSE;
}

void ScoreboardHook::DisableNativeScoreboard(void* pThis, bool hideCursor)
{
    if (!pThis)
        return;

    if (IsNativeScoreboardEnabled(pThis) && s_close_)
    {
        s_close_(pThis, hideCursor);
        return;
    }

    *reinterpret_cast<BOOL*>(pThis) = FALSE;
}

void __fastcall ScoreboardHook::Hook_Enable(void* pThis, void* edx)
{
    auto* self = s_self_;
    if (!self)
        return;

    if (self->browser_.HandleNativePlayerListOpenRequest())
    {
        DisableNativeScoreboard(pThis, true);
        return;
    }

    if (s_orig_enable_)
        s_orig_enable_(pThis, edx);
}

void __fastcall ScoreboardHook::Hook_Draw(void* pThis, void* edx)
{
    auto* self = s_self_;
    if (!self)
        return;

    if (self->browser_.ShouldSuppressNativePlayerList())
    {
        DisableNativeScoreboard(pThis, true);
        return;
    }

    if (s_orig_draw_)
        s_orig_draw_(pThis, edx);
}
