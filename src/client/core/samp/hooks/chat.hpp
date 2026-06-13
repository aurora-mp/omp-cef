#pragma once

#include <Windows.h>

class HookManager;
class FocusManager;
class BrowserManager;
class NetworkManager;

class ChatHook
{
public:
    ChatHook(HookManager& hooks, FocusManager& focus, BrowserManager& browser, NetworkManager& network)
        : hooks_(hooks)
        , focus_(focus)
        , browser_(browser)
        , network_(network)
    {
    }

    bool Initialize();
    void Shutdown();

private:
    using FnOpenChatInput = void(__fastcall*)(void* pThis, void* _edx);
    static void __fastcall Hook_OpenChatInput(void* pThis, void* _edx);

    using FnCloseChatInput = void(__fastcall*)(void* pThis, void* _edx);
    static void __fastcall Hook_CloseChatInput(void* pThis, void* _edx);

    void SetChatInputState(bool open);

private:
    HookManager& hooks_;
    FocusManager& focus_;
    BrowserManager& browser_;
    NetworkManager& network_;

    static inline ChatHook* s_self_ = nullptr;
    static inline FnOpenChatInput s_orig_open_ = nullptr;
    static inline FnCloseChatInput s_orig_close_ = nullptr;
};