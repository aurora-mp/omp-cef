#include "render_manager.hpp"

#include <windows.h>
#include <new>

#include "system/logger.hpp"
#include "hooks/hook_manager.hpp"
#include "hooks/d3d9.hpp"

namespace
{
    void LogLoadedD3D9Module()
    {
        HMODULE mod = ::GetModuleHandleA("d3d9.dll");
        if (!mod)
            mod = ::LoadLibraryA("d3d9.dll");

        if (!mod)
            return;

        char path[MAX_PATH] = {};
        ::GetModuleFileNameA(mod, path, MAX_PATH);
        LOG_INFO("[D3D9] d3d9.dll loaded from: {}", path);
    }

    void* ResolveD3D9Export(const char* name)
    {
        HMODULE mod = ::GetModuleHandleA("d3d9.dll");
        if (!mod)
            mod = ::LoadLibraryA("d3d9.dll");

        if (!mod)
            return nullptr;

        return reinterpret_cast<void*>(::GetProcAddress(mod, name));
    }

    bool TryGetPresentParameters(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS& out) noexcept
    {
        if (!device)
            return false;

        IDirect3DSwapChain9* swapChain = nullptr;
        HRESULT hr = device->GetSwapChain(0, &swapChain);
        if (FAILED(hr) || !swapChain)
            return false;

        D3DPRESENT_PARAMETERS pp{};
        hr = swapChain->GetPresentParameters(&pp);
        swapChain->Release();

        if (FAILED(hr))
            return false;

        out = pp;
        return true;
    }

    static HWND CreateDummyWindow()
    {
        constexpr const char* className = "omp_cef_d3d9_dummy";

        WNDCLASSEXA wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = ::DefWindowProcA;
        wc.hInstance = GetModuleHandleA(nullptr);
        wc.lpszClassName = className;
        ::RegisterClassExA(&wc);

        HWND hwnd = ::CreateWindowExA(
            WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT,
            className,
            "",
            WS_POPUP,
            -32000, -32000, 1, 1,
            nullptr, nullptr, wc.hInstance, nullptr);

        if (hwnd)
        {
            ::SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
            ::ShowWindow(hwnd, SW_HIDE);
        }

        return hwnd;
    }

    void DestroyDummyWindow(HWND hwnd) noexcept
    {
        if (hwnd)
            DestroyWindow(hwnd);
    }

    bool CreateDummyDevice(IDirect3DDevice9** outDevice) noexcept
    {
        if (!outDevice)
            return false;

        *outDevice = nullptr;

        using Create9Fn = IDirect3D9* (WINAPI*)(UINT);

        auto* create9 = reinterpret_cast<Create9Fn>(ResolveD3D9Export("Direct3DCreate9"));
        if (!create9)
            return false;

        IDirect3D9* d3d = create9(D3D_SDK_VERSION);
        if (!d3d)
            return false;

        HWND hwnd = CreateDummyWindow();
        if (!hwnd)
        {
            d3d->Release();
            return false;
        }

        D3DPRESENT_PARAMETERS pp{};
        pp.Windowed = TRUE;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = hwnd;
        pp.BackBufferFormat = D3DFMT_UNKNOWN;
        pp.BackBufferWidth = 16;
        pp.BackBufferHeight = 16;
        pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

        IDirect3DDevice9* dev = nullptr;

        DWORD flags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

        HRESULT hr = d3d->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            hwnd,
            flags,
            &pp,
            &dev);

        // Fallback: REF if HAL fails (rare, but better than failing bootstrap entirely)
        if (FAILED(hr) || !dev)
        {
            hr = d3d->CreateDevice(
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_REF,
                hwnd,
                flags,
                &pp,
                &dev);
        }

        DestroyDummyWindow(hwnd);
        d3d->Release();

        if (FAILED(hr) || !dev)
            return false;

        *outDevice = dev;
        return true;
    }
}

bool RenderManager::IsReadablePointer(const void* pointer, size_t size) noexcept
{
    if (!pointer || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION mbi{};
    if (::VirtualQuery(pointer, &mbi, sizeof(mbi)) == 0)
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    const DWORD protect = mbi.Protect & 0xFF;
    if (protect == PAGE_NOACCESS || protect == PAGE_GUARD)
        return false;

    const uintptr_t start = reinterpret_cast<uintptr_t>(pointer);
    const uintptr_t end = start + static_cast<uintptr_t>(size);
    const uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
    const uintptr_t regionEnd = regionStart + static_cast<uintptr_t>(mbi.RegionSize);

    return end <= regionEnd;
}

bool RenderManager::Initialize()
{
    if (initialized_)
        return true;

    if (!hooks_)
    {
        LOG_FATAL("RenderManager::Initialize called without HookManager set. Call SetHookManager() first.");
        return false;
    }

    s_self_ = this;

    LogLoadedD3D9Module();

    // Bootstrap: hook methods from a dummy device (covers most wrappers/mods)
    if (!TryInstallBootstrapHooks())
    {
        // Not fatal. PollD3D fallback can still attach if GTA globals are readable.
        LOG_WARN("[D3D9] Bootstrap hooks failed (dummy device). Will rely on PollD3D fallback.");
    }

    initialized_ = true;
    return true;
}

void RenderManager::Shutdown()
{
    if (!hooks_)
        return;

    hooks_->Uninstall(hookNameReset);
    hooks_->Uninstall(hookNameBeginScene);
    hooks_->Uninstall(hookNameEndScene);
    hooks_->Uninstall(hookNamePresent);

    device_hooks_installed_ = false;
    hooked_reset_addr_ = nullptr;
    hooked_begin_addr_ = nullptr;
    hooked_end_addr_ = nullptr;
    hooked_present_addr_ = nullptr;

    orig_reset_ = nullptr;
    orig_begin_scene_ = nullptr;
    orig_end_scene_ = nullptr;
    orig_present_ = nullptr;

    DetachDevice();

    initialized_ = false;
}

bool RenderManager::TryInstallBootstrapHooks() noexcept
{
    IDirect3DDevice9* dummy = nullptr;

    if (!CreateDummyDevice(&dummy) || !dummy)
        return false;

    // Install hooks based on dummy vtable function pointers.
    EnsureDeviceHooksInstalled(dummy);

    dummy->Release();

    // If we got at least Present hooked, we're good.
    return (orig_present_ != nullptr);
}

void RenderManager::PollD3D() noexcept
{
    if (!initialized_)
        return;

    const uint64_t now = ::GetTickCount64();
    if (now - last_poll_ms_ < 250)
        return;

    last_poll_ms_ = now;

    // GTA global fallback (works in many setups)
    IDirect3DDevice9* game_device = const_cast<IDirect3DDevice9*>(gGameDevice);
    if (!game_device || !IsReadablePointer(game_device, sizeof(void*)))
        return;

    TryCaptureDeviceFromPointer(game_device);

    // Ensure hooks are installed for THIS device class (some wrappers use unique thunks)
    EnsureDeviceHooksInstalled(game_device);
}

void RenderManager::AttachDevice(IDirect3D9* direct, IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS& params) noexcept
{
    std::scoped_lock lock(device_lock_);

    direct_ = direct;
    device_ = device;
    device_parameters_ = params;

    fast_device_.store(device, std::memory_order_release);
}

void RenderManager::DetachDevice() noexcept
{
    std::scoped_lock lock(device_lock_);

    direct_ = nullptr;
    device_ = nullptr;
    device_parameters_ = {};

    fast_device_.store(nullptr, std::memory_order_release);
    capture_rank_.store(0, std::memory_order_release);
    initialized_device_.store(nullptr, std::memory_order_release);

    reset_status_ = false;
    scene_status_ = false;
}

IDirect3DDevice9* RenderManager::GetDevice() const noexcept
{
    return fast_device_.load(std::memory_order_acquire);
}

bool RenderManager::GetScreenSize(float& width, float& height) const noexcept
{
    std::scoped_lock lock(device_lock_);
    if (!device_)
        return false;

    if (device_parameters_.BackBufferWidth == 0 || device_parameters_.BackBufferHeight == 0)
        return false;

    width = static_cast<float>(device_parameters_.BackBufferWidth);
    height = static_cast<float>(device_parameters_.BackBufferHeight);
    return true;
}

bool RenderManager::ConvertBaseXValueToScreenXValue(float base, float& screen) const noexcept
{
    std::scoped_lock lock(device_lock_);
    if (!device_ || device_parameters_.BackBufferWidth == 0)
        return false;

    screen = (static_cast<float>(device_parameters_.BackBufferWidth) / kBaseWidth) * base;
    return true;
}

bool RenderManager::ConvertBaseYValueToScreenYValue(float base, float& screen) const noexcept
{
    std::scoped_lock lock(device_lock_);
    if (!device_ || device_parameters_.BackBufferHeight == 0)
        return false;

    screen = (static_cast<float>(device_parameters_.BackBufferHeight) / kBaseHeight) * base;
    return true;
}

bool RenderManager::ConvertScreenXValueToBaseXValue(float screen, float& base) const noexcept
{
    std::scoped_lock lock(device_lock_);
    if (!device_ || device_parameters_.BackBufferWidth == 0)
        return false;

    base = (kBaseWidth / static_cast<float>(device_parameters_.BackBufferWidth)) * screen;
    return true;
}

bool RenderManager::ConvertScreenYValueToBaseYValue(float screen, float& base) const noexcept
{
    std::scoped_lock lock(device_lock_);
    if (!device_ || device_parameters_.BackBufferHeight == 0)
        return false;

    base = (kBaseHeight / static_cast<float>(device_parameters_.BackBufferHeight)) * screen;
    return true;
}

bool RenderManager::IsGameDeviceCandidate(const D3DPRESENT_PARAMETERS* pp, HWND presentHwnd) const noexcept
{
    // If we know the game hwnd, we filter strictly to avoid capturing overlay/helper devices.
    if (!game_hwnd_)
        return true;

    DWORD wpid = 0;
    ::GetWindowThreadProcessId(game_hwnd_, &wpid);
    if (wpid != ::GetCurrentProcessId())
        return true;

    if (presentHwnd && presentHwnd == game_hwnd_)
        return true;

    if (pp && pp->hDeviceWindow && pp->hDeviceWindow == game_hwnd_)
        return true;

    return false;
}

bool RenderManager::TryCaptureDeviceFromPointer(IDirect3DDevice9* device) noexcept
{
    if (!device)
        return false;

    IDirect3DDevice9* current = fast_device_.load(std::memory_order_acquire);
    const int current_rank = capture_rank_.load(std::memory_order_acquire);

    // Same device already captured
    if (current == device && current_rank >= 1)
        return false;

    D3DPRESENT_PARAMETERS pp{};
    (void)TryGetPresentParameters(device, pp);

    if (!IsGameDeviceCandidate(&pp, nullptr))
        return false;

    // Notify destroy when switching devices
    if (current && current != device)
    {
        if (OnDeviceDestroy)
            OnDeviceDestroy();
    }

    AttachDevice(nullptr, device, pp);
    capture_rank_.store(1, std::memory_order_release);

    // Fire OnDeviceInitialize once
    IDirect3DDevice9* prevInit = initialized_device_.exchange(device, std::memory_order_acq_rel);
    if (prevInit != device)
    {
        if (OnDeviceInitialize)
            OnDeviceInitialize(nullptr, device, pp);
    }

    if (!current)
        LOG_INFO("[D3D9] Captured device via PollD3D fallback: device={}", (void*)device);
    else if (current != device)
        LOG_WARN("[D3D9] Game device changed (PollD3D): old={}, new={}", (void*)current, (void*)device);

    return true;
}

bool RenderManager::TryCaptureDeviceFromPresent(IDirect3DDevice9* device, HWND presentHwnd) noexcept
{
    if (!device)
        return false;

    IDirect3DDevice9* current = fast_device_.load(std::memory_order_acquire);
    const int current_rank = capture_rank_.load(std::memory_order_acquire);

    // Already the confirmed game device
    if (current == device && current_rank >= 2)
        return false;

    D3DPRESENT_PARAMETERS pp{};
    (void)TryGetPresentParameters(device, pp);

    if (!IsGameDeviceCandidate(&pp, presentHwnd))
        return false;

    // Upgrade / switch to this device
    if (current && current != device)
    {
        if (OnDeviceDestroy)
            OnDeviceDestroy();
    }

    AttachDevice(nullptr, device, pp);
    capture_rank_.store(2, std::memory_order_release);

    IDirect3DDevice9* prevInit = initialized_device_.exchange(device, std::memory_order_acq_rel);
    if (prevInit != device)
    {
        if (OnDeviceInitialize)
            OnDeviceInitialize(nullptr, device, pp);
    }

    if (current && current != device)
        LOG_WARN("[D3D9] Game device upgraded via Present: old={}, new={}, hwnd={}", (void*)current, (void*)device, (void*)presentHwnd);

    LOG_INFO("[D3D9] Captured device via Present: device={}, hwnd={}", (void*)device, (void*)presentHwnd);
    return true;
}

void RenderManager::EnsureDeviceHooksInstalled(IDirect3DDevice9* device) noexcept
{
    if (!device || !hooks_)
        return;

    void** vtable = *reinterpret_cast<void***>(device);
    if (!vtable || !IsReadablePointer(vtable, sizeof(void*) * 64))
        return;

    void* addrReset = vtable[idxReset];
    void* addrPresent = vtable[idxPresent];
    void* addrBegin = vtable[idxBeginScene];
    void* addrEnd = vtable[idxEndScene];

    if (!addrReset || !addrPresent || !addrBegin || !addrEnd)
        return;

    std::scoped_lock lock(hook_lock_);

    // Same targets already hooked
    if (device_hooks_installed_ &&
        addrReset == hooked_reset_addr_ &&
        addrBegin == hooked_begin_addr_ &&
        addrEnd == hooked_end_addr_ &&
        addrPresent == hooked_present_addr_)
    {
        return;
    }

    device_hooks_installed_ = false;

    if (!hooks_->Install(hookNameReset, addrReset, reinterpret_cast<void*>(&RenderManager::hkReset)))
        return;

    if (!hooks_->Install(hookNameBeginScene, addrBegin, reinterpret_cast<void*>(&RenderManager::hkBeginScene)))
    {
        hooks_->Uninstall(hookNameReset);
        return;
    }

    if (!hooks_->Install(hookNameEndScene, addrEnd, reinterpret_cast<void*>(&RenderManager::hkEndScene)))
    {
        hooks_->Uninstall(hookNameBeginScene);
        hooks_->Uninstall(hookNameReset);
        return;
    }

    if (!hooks_->Install(hookNamePresent, addrPresent, reinterpret_cast<void*>(&RenderManager::hkPresent)))
    {
        hooks_->Uninstall(hookNameEndScene);
        hooks_->Uninstall(hookNameBeginScene);
        hooks_->Uninstall(hookNameReset);
        return;
    }

    hooked_reset_addr_ = addrReset;
    hooked_begin_addr_ = addrBegin;
    hooked_end_addr_ = addrEnd;
    hooked_present_addr_ = addrPresent;

    orig_reset_ = reinterpret_cast<Reset_t>(hooks_->GetOriginal(hookNameReset));
    orig_begin_scene_ = reinterpret_cast<BeginScene_t>(hooks_->GetOriginal(hookNameBeginScene));
    orig_end_scene_ = reinterpret_cast<EndScene_t>(hooks_->GetOriginal(hookNameEndScene));
    orig_present_ = reinterpret_cast<Present_t>(hooks_->GetOriginal(hookNamePresent));

    device_hooks_installed_ = true;

    LOG_INFO("[D3D9] Hooks installed: Reset={}, BeginScene={}, EndScene={}, Present={}",
        addrReset, addrBegin, addrEnd, addrPresent);
}

HRESULT __stdcall RenderManager::hkReset(IDirect3DDevice9* self, D3DPRESENT_PARAMETERS* pp)
{
    auto* rm = s_self_;
    if (!rm || !rm->orig_reset_)
        return D3DERR_INVALIDCALL;

    const bool isGameDevice = (self == rm->fast_device_.load(std::memory_order_acquire));

    if (isGameDevice)
    {
        rm->reset_status_ = true;
        if (rm->OnBeforeReset)
            rm->OnBeforeReset();
    }

    HRESULT hr = rm->orig_reset_(self, pp);

    if (isGameDevice)
    {
        rm->reset_status_ = false;

        if (SUCCEEDED(hr) && pp)
        {
            std::scoped_lock lock(rm->device_lock_);
            rm->device_parameters_ = *pp;
        }

        if (SUCCEEDED(hr) && rm->OnAfterReset)
        {
            D3DPRESENT_PARAMETERS copy{};
            {
                std::scoped_lock lock(rm->device_lock_);
                copy = rm->device_parameters_;
            }
            rm->OnAfterReset(self, copy);
        }
    }

    return hr;
}

HRESULT __stdcall RenderManager::hkBeginScene(IDirect3DDevice9* self)
{
    auto* rm = s_self_;
    if (!rm || !rm->orig_begin_scene_)
        return D3DERR_INVALIDCALL;

    const bool isGameDevice = (self == rm->fast_device_.load(std::memory_order_acquire));

    if (isGameDevice)
    {
        rm->scene_status_ = true;
        if (rm->OnBeforeBeginScene)
            rm->OnBeforeBeginScene();
    }

    HRESULT hr = rm->orig_begin_scene_(self);

    if (isGameDevice)
    {
        if (rm->OnAfterBeginScene)
            rm->OnAfterBeginScene();
    }

    return hr;
}

HRESULT __stdcall RenderManager::hkEndScene(IDirect3DDevice9* self)
{
    auto* rm = s_self_;
    if (!rm || !rm->orig_end_scene_)
        return D3DERR_INVALIDCALL;

    const bool isGameDevice = (self == rm->fast_device_.load(std::memory_order_acquire));

    if (isGameDevice)
    {
        if (rm->OnBeforeEndScene)
            rm->OnBeforeEndScene();
    }

    HRESULT hr = rm->orig_end_scene_(self);

    if (isGameDevice)
    {
        rm->scene_status_ = false;
        if (rm->OnAfterEndScene)
            rm->OnAfterEndScene();
    }

    return hr;
}

HRESULT __stdcall RenderManager::hkPresent(IDirect3DDevice9* self, const RECT* src, const RECT* dst, HWND wnd, const RGNDATA* dirty)
{
    auto* rm = s_self_;
    if (!rm || !rm->orig_present_)
        return D3DERR_INVALIDCALL;

    // Capture game device on first valid Present()
    rm->TryCaptureDeviceFromPresent(self, wnd);

    const bool isGameDevice = (self == rm->fast_device_.load(std::memory_order_acquire));

    if (isGameDevice)
    {
        if (rm->OnPresent)
            rm->OnPresent();
    }

    return rm->orig_present_(self, src, dst, wnd, dirty);
}