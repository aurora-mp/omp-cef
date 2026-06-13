#pragma once

#include <windows.h>
#include <atomic>

class HookManager;

class CursorHook
{
public:
    static CursorHook& Instance() noexcept
    {
        static CursorHook instance;
        return instance;
    }

    CursorHook(const CursorHook&) = delete;
    CursorHook& operator=(const CursorHook&) = delete;

    void Initialize(HookManager& hooks);
    void Shutdown(HookManager& hooks);

    void SetForcedCursor(HCURSOR cursor)
    {
        forced_cursor_.store(cursor, std::memory_order_release);
        forced_.store(cursor != nullptr, std::memory_order_release);
    }

    void SetForced(bool forced)
    {
        forced_.store(forced, std::memory_order_release);
        if (!forced)
            forced_cursor_.store(nullptr, std::memory_order_release);
    }

    void ClearForcedCursor() { SetForced(false); }

    void OnGameActivated() noexcept
    {
        last_activate_tick_.store(::GetTickCount(), std::memory_order_release);
    }

private:
    CursorHook() = default;
    ~CursorHook() = default;

    static HCURSOR WINAPI hkSetCursor(HCURSOR hCursor);

    using SetCursor_t = HCURSOR(WINAPI*)(HCURSOR);

    SetCursor_t          orig_set_cursor_    = nullptr;
    std::atomic<bool>    forced_             { false };
    std::atomic<HCURSOR> forced_cursor_      { nullptr };
    std::atomic<DWORD>   last_activate_tick_ { 0 };

    static constexpr DWORD kActivateGraceMs = 50;
};