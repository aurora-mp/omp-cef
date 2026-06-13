#pragma once

class HookManager;
class ResourceManager;

class NetGameHook
{
public:
    NetGameHook(HookManager& hooks, ResourceManager& resources) : hooks_(hooks), resources_(resources) {}

    bool Initialize();
    void Shutdown();

    void SetSessionActive(bool active);
    bool IsSessionActive() const { return session_active_.load(); }

private:
    using FnProcessGameStuff = void(__thiscall*)(void*);
    static void __fastcall Hook_ProcessGameStuff(void* pThis, void* _edx);

    HookManager& hooks_;
    ResourceManager& resources_;

    std::atomic<bool> session_active_{ false };
    bool cef_download_triggered_ = false;

    static inline NetGameHook* s_self_ = nullptr;
    static inline FnProcessGameStuff s_orig_ = nullptr;
};