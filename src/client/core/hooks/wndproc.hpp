#pragma once

#include <windows.h>
#include <functional>

class WndProcHook
{
public:
    explicit WndProcHook(HWND hwnd) : hwnd_(hwnd) {}
    ~WndProcHook() = default;

    WndProcHook(const WndProcHook&) = delete;
    WndProcHook& operator=(const WndProcHook&) = delete;

    bool Initialize();
    void Shutdown();
    void EnsureInstalled();
    
    bool IsValid() const { 
        return hwnd_ && ::IsWindow(hwnd_); 
    }

    std::function<std::optional<LRESULT>(HWND, UINT, WPARAM, LPARAM)> OnMessage;
private:
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND hwnd_ = nullptr;
    WNDPROC baseProc_ = nullptr;
    WNDPROC nextProc_ = nullptr;
    DWORD lastEnsureTick_ = 0;

    static inline WndProcHook* s_self_ = nullptr;
};