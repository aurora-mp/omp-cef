#pragma once

class HookManager;
class BrowserManager;

struct CEntity;

class RenderHook
{
public:
    RenderHook(HookManager& hooks, BrowserManager& browser) : hooks_(hooks), browser_(browser) {}
    ~RenderHook() = default;

    bool Initialize();
    void Shutdown();

private:
    using CEntity_Render_t = void(__thiscall*)(CEntity*);
    static void __fastcall hkCEntity_Render(CEntity* pThis, void* _edx);

    static inline CEntity_Render_t s_original_ = nullptr;
    static inline RenderHook* s_self_ = nullptr;

    HookManager& hooks_;
    BrowserManager& browser_;
};