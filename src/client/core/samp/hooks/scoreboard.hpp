#pragma once

class BrowserManager;
class HookManager;

class ScoreboardHook
{
public:
    ScoreboardHook(HookManager& hooks, BrowserManager& browser)
        : hooks_(hooks)
        , browser_(browser)
    {
    }

    bool Initialize();
    void Shutdown();

private:
    using FnEnable = void(__fastcall*)(void* pThis, void* edx);
    using FnDraw = void(__fastcall*)(void* pThis, void* edx);
    using FnClose = void(__thiscall*)(void* pThis, bool hideCursor);

    static void __fastcall Hook_Enable(void* pThis, void* edx);
    static void __fastcall Hook_Draw(void* pThis, void* edx);

    static bool IsNativeScoreboardEnabled(void* pThis);
    static void DisableNativeScoreboard(void* pThis, bool hideCursor);

private:
    HookManager& hooks_;
    BrowserManager& browser_;

    static inline ScoreboardHook* s_self_ = nullptr;
    static inline FnEnable s_orig_enable_ = nullptr;
    static inline FnDraw s_orig_draw_ = nullptr;
    static inline FnClose s_close_ = nullptr;
};
