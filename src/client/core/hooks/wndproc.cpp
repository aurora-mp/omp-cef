#include "wndproc.hpp"
#include "system/logger.hpp"

bool WndProcHook::Initialize()
{
    if (!hwnd_ || !::IsWindow(hwnd_)) {
        LOG_ERROR("[WndProcHook] Invalid or destroyed HWND: {}", static_cast<void*>(hwnd_));
        return false;
    }
    
    s_self_ = this;

    ::SetLastError(0);
    auto prev = reinterpret_cast<WNDPROC>(
        ::SetWindowLongPtrW(hwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&StaticWndProc))
    );

    DWORD err = ::GetLastError();
    if (!prev && err != 0) {
        LOG_ERROR("[WndProcHook] SetWindowLongPtr failed (error={})", err);
        s_self_ = nullptr;
        return false;
    }

    baseProc_ = prev;
    nextProc_ = prev;

    LOG_INFO("[WndProcHook] Hook installed (hwnd={}, baseProc={})", static_cast<void*>(hwnd_), static_cast<void*>(baseProc_));
    return true;
}

void WndProcHook::EnsureInstalled()
{
    if (!hwnd_ || !::IsWindow(hwnd_))
        return;

    const DWORD now = ::GetTickCount();
    if (now - lastEnsureTick_ < 250)
        return;

    lastEnsureTick_ = now;

    const auto current = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(hwnd_, GWLP_WNDPROC));
    if (!current)
        return;

    if (current == reinterpret_cast<WNDPROC>(&StaticWndProc))
        return;

    nextProc_ = current;

    ::SetLastError(0);
    ::SetWindowLongPtrW(hwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&StaticWndProc));

    if (::GetLastError() == 0) {
        LOG_DEBUG("[WndProcHook] Hook reinstalled (nextProc={})", static_cast<void*>(nextProc_));
    }
}

void WndProcHook::Shutdown()
{
    if (!hwnd_)
        return;

    const auto current = reinterpret_cast<WNDPROC>(::GetWindowLongPtrW(hwnd_, GWLP_WNDPROC));

    if (current == reinterpret_cast<WNDPROC>(&StaticWndProc))
    {
        auto restore = nextProc_ ? nextProc_ : baseProc_;
        if (restore && ::IsWindow(hwnd_))
            ::SetWindowLongPtrW(hwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(restore));
    }

    hwnd_ = nullptr;
    baseProc_ = nullptr;
    nextProc_ = nullptr;
    s_self_ = nullptr;

    LOG_INFO("[WndProcHook] Hook uninstalled.");
}

LRESULT CALLBACK WndProcHook::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* self = s_self_;

    static thread_local int depth = 0;
    const bool reentered = (depth++ > 0);

    if (!reentered && self && self->OnMessage)
    {
        auto result = self->OnMessage(hwnd, msg, wParam, lParam);
        if (result.has_value())
        {
            depth--;
            return result.value();
        }
    }

    WNDPROC target = nullptr;
    if (self)
        target = reentered ? self->baseProc_ : self->nextProc_;

    const LRESULT out = ::CallWindowProcW(
        target ? target : ::DefWindowProcW, hwnd, msg, wParam, lParam);

    depth--;
    return out;
}