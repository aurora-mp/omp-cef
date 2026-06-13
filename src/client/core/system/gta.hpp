#pragma once

#include <windows.h>
#include <string>
#include <atomic>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>

class Gta
{
public:
    Gta() = default;
    ~Gta();

    Gta(const Gta&) = delete;
    Gta& operator=(const Gta&) = delete;

    void Initialize();
    void Shutdown();
    void PumpMainThreadCallbacks();

    std::string GetUserFilesPath();

    HWND GetHwnd() const { return hwnd_.load(std::memory_order_acquire); }
    bool IsReady() const { return hwnd_.load(std::memory_order_acquire) != nullptr; }

    void SetOnHwndFound(std::function<void(HWND)> callback);
    void PostToMainThread(std::function<void()> fn);

private:
    HWND FindHwndFromMemory();
    HWND FindHwndByTitle();
    HWND FindHwndByClass();
    HWND FindHwndByProcess();

    void SearchThreadLoop();
    void OnHwndDiscovered(HWND hwnd);


    struct SearchData { DWORD pid; HWND result; };

    static bool BelongsToCurrentProcess(HWND hwnd, DWORD pid) noexcept
    {
        DWORD wpid = 0;
        ::GetWindowThreadProcessId(hwnd, &wpid);
        return wpid == pid;
    }
private:
    static constexpr uintptr_t HwndAddress = 0xC97C1C;

    std::atomic<HWND> hwnd_{ nullptr };
    std::atomic<bool> shutdown_{ false };
    std::atomic<bool> found_{ false };

    std::thread search_thread_;

    std::mutex callback_mutex_;
    std::function<void(HWND)> on_hwnd_found_;

    std::mutex main_thread_queue_mutex_;
    std::queue<std::function<void()>> main_thread_queue_;
};