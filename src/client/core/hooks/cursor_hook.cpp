#include "cursor_hook.hpp"

#include "hooks/hook_manager.hpp"
#include "system/logger.hpp"

static constexpr const char* kHookNameSetCursor = "CursorHook::SetCursor";

void CursorHook::Initialize(HookManager& hooks)
{
    HMODULE user32 = ::GetModuleHandleA("user32.dll");
    if (!user32)
        user32 = ::LoadLibraryA("user32.dll");

    if (!user32)
    {
        LOG_WARN("[CursorHook] user32.dll not found.");
        return;
    }

    void* addr = reinterpret_cast<void*>(::GetProcAddress(user32, "SetCursor"));
    if (!addr)
    {
        LOG_WARN("[CursorHook] SetCursor export not found.");
        return;
    }

    if (!hooks.Install(kHookNameSetCursor, addr,
            reinterpret_cast<void*>(&CursorHook::hkSetCursor)))
    {
        LOG_WARN("[CursorHook] Failed to hook SetCursor.");
        return;
    }

    orig_set_cursor_ = reinterpret_cast<SetCursor_t>(
        hooks.GetOriginal(kHookNameSetCursor));

    LOG_INFO("[CursorHook] SetCursor hook installed.");
}

void CursorHook::Shutdown(HookManager& hooks)
{
    hooks.Uninstall(kHookNameSetCursor);
    orig_set_cursor_ = nullptr;
}

HCURSOR WINAPI CursorHook::hkSetCursor(HCURSOR hCursor)
{
    auto& self = CursorHook::Instance();

    if (!self.orig_set_cursor_)
        return nullptr;

    const DWORD now = ::GetTickCount();
    const DWORD lastActivate = self.last_activate_tick_.load(std::memory_order_acquire);

    if ((now - lastActivate) < kActivateGraceMs)
        return self.orig_set_cursor_(hCursor);

    if (self.forced_.load(std::memory_order_acquire))
    {
        HCURSOR forced = self.forced_cursor_.load(std::memory_order_acquire);
        if (forced)
            return self.orig_set_cursor_(forced);
    }

    return self.orig_set_cursor_(hCursor);
}