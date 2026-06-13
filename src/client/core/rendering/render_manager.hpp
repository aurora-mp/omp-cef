#pragma once

#include <d3d9.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>

class HookManager;

/*
* Goals:
*  - Work even when the D3D device is created BEFORE our plugin starts (ASI/ENB/wrappers).
*  - Do NOT rely on CreateDevice arguments (&gGameDevice), do NOT proxy COM interfaces.
*  - Prefer "bootstrap" vtable hooks from a dummy device; fallback to GTA globals polling.
*
* Strategy:
*  1) Bootstrap: create a tiny dummy D3D9 device -> read vtable -> install hooks on methods.
*  2) First Present() that matches the game window captures the real game device.
*  3) Fallback: PollD3D reads GTA global gGameDevice and installs hooks on that device if needed.
*/

class RenderManager
{
public:
    static RenderManager& Instance() noexcept
    {
        static RenderManager instance;
        return instance;
    }

    RenderManager(const RenderManager&) = delete;
    RenderManager& operator=(const RenderManager&) = delete;
    RenderManager(RenderManager&&) = delete;
    RenderManager& operator=(RenderManager&&) = delete;

    void SetHookManager(HookManager* hooks) noexcept { hooks_ = hooks; }
    void SetGameWindow(HWND hwnd) noexcept { game_hwnd_ = hwnd; }
    HWND GetGameWindow() const noexcept { return game_hwnd_; }

    bool Initialize();
    void Shutdown();

    // Safe to call from WndProc (throttled).
    void PollD3D() noexcept;

    // Called when we decide a device is "the game device"
    void AttachDevice(IDirect3D9* direct, IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS& params) noexcept;
    void DetachDevice() noexcept;

    // Accessors
    IDirect3DDevice9* GetDevice() const noexcept;
    bool GetScreenSize(float& width, float& height) const noexcept;

    // Base 640x480 conversion helpers
    static constexpr float kBaseWidth = 640.0f;
    static constexpr float kBaseHeight = 480.0f;

    bool ConvertBaseXValueToScreenXValue(float base, float& screen) const noexcept;
    bool ConvertBaseYValueToScreenYValue(float base, float& screen) const noexcept;
    bool ConvertScreenXValueToBaseXValue(float screen, float& base) const noexcept;
    bool ConvertScreenYValueToBaseYValue(float screen, float& base) const noexcept;

    // Device lifecycle callbacks
    std::function<void(IDirect3D9*, IDirect3DDevice9*, const D3DPRESENT_PARAMETERS&)> OnDeviceInitialize;
    std::function<void()> OnDeviceDestroy;

    // Render events triggered by our device method hooks
    std::function<void()> OnBeforeBeginScene;
    std::function<void()> OnAfterBeginScene;

    std::function<void()> OnBeforeEndScene;
    std::function<void()> OnAfterEndScene;

    std::function<void()> OnPresent;

    std::function<void()> OnBeforeReset;
    std::function<void(IDirect3DDevice9*, const D3DPRESENT_PARAMETERS&)> OnAfterReset;

    // Runtime state used by rendering pipeline
    bool reset_status_ = false;
    bool scene_status_ = false;

private:
    RenderManager() = default;
    ~RenderManager() = default;

    // Device method detours
    static HRESULT __stdcall hkReset(IDirect3DDevice9* self, D3DPRESENT_PARAMETERS* pp);
    static HRESULT __stdcall hkBeginScene(IDirect3DDevice9* self);
    static HRESULT __stdcall hkEndScene(IDirect3DDevice9* self);
    static HRESULT __stdcall hkPresent(IDirect3DDevice9* self, const RECT* src, const RECT* dst, HWND wnd, const RGNDATA* dirty);

private:
    bool TryInstallBootstrapHooks() noexcept;
    bool TryCaptureDeviceFromPresent(IDirect3DDevice9* device, HWND presentHwnd) noexcept;
    bool TryCaptureDeviceFromPointer(IDirect3DDevice9* device) noexcept;

    void EnsureDeviceHooksInstalled(IDirect3DDevice9* device) noexcept;

    bool IsGameDeviceCandidate(const D3DPRESENT_PARAMETERS* pp, HWND presentHwnd) const noexcept;

    static bool IsReadablePointer(const void* ptr, size_t bytes) noexcept;

private:
    static inline RenderManager* s_self_ = nullptr;

    static constexpr const char* hookNameReset = "RenderManager::Device::Reset";
    static constexpr const char* hookNameBeginScene = "RenderManager::Device::BeginScene";
    static constexpr const char* hookNameEndScene = "RenderManager::Device::EndScene";
    static constexpr const char* hookNamePresent = "RenderManager::Device::Present";

    // IDirect3DDevice9 vtable indices
    static constexpr size_t idxReset = 16;
    static constexpr size_t idxPresent = 17;
    static constexpr size_t idxBeginScene = 41;
    static constexpr size_t idxEndScene = 42;

    using Reset_t = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
    using BeginScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
    using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
    using Present_t = HRESULT(__stdcall*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);

    HookManager* hooks_ = nullptr;
    bool initialized_ = false;

    HWND game_hwnd_ = nullptr;

    mutable std::mutex device_lock_;
    std::mutex hook_lock_;

    IDirect3D9* direct_ = nullptr;
    IDirect3DDevice9* device_ = nullptr;
    D3DPRESENT_PARAMETERS device_parameters_{};

    std::atomic<IDirect3DDevice9*> fast_device_{ nullptr };
    std::atomic<int> capture_rank_{ 0 };
    std::atomic<IDirect3DDevice9*> initialized_device_{ nullptr };

    // Hook state
    bool device_hooks_installed_ = false;
    void* hooked_reset_addr_ = nullptr;
    void* hooked_begin_addr_ = nullptr;
    void* hooked_end_addr_ = nullptr;
    void* hooked_present_addr_ = nullptr;

    Reset_t orig_reset_ = nullptr;
    BeginScene_t orig_begin_scene_ = nullptr;
    EndScene_t orig_end_scene_ = nullptr;
    Present_t orig_present_ = nullptr;

    // Poll throttling
    uint64_t last_poll_ms_ = 0;
};