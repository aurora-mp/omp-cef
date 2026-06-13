#include "gta.hpp"
#include "system/logger.hpp"

#include <shlobj.h>
#include <filesystem>
#include <chrono>

Gta::~Gta()
{
    Shutdown();
}

void Gta::Initialize()
{
    LOG_INFO("[GTA] Starting HWND search ...");

    shutdown_.store(false, std::memory_order_release);
    found_.store(false, std::memory_order_release);
    search_thread_ = std::thread(&Gta::SearchThreadLoop, this);
}

void Gta::Shutdown()
{
    if (shutdown_.exchange(true, std::memory_order_acq_rel))
        return;

    if (search_thread_.joinable())
        search_thread_.join();

    LOG_INFO("[GTA] HWND search stopped.");
}

void Gta::PumpMainThreadCallbacks()
{
    std::queue<std::function<void()>> local;

    {
        std::lock_guard<std::mutex> lock(main_thread_queue_mutex_);
        std::swap(local, main_thread_queue_);
    }

    while (!local.empty())
    {
        auto& fn = local.front();
        if (fn)
        {
            try { fn(); }
            catch (const std::exception& e)
            {
                LOG_ERROR("[GTA] Exception in main-thread callback: {}", e.what());
            }
        }
        local.pop();
    }
}

void Gta::PostToMainThread(std::function<void()> fn)
{
    std::lock_guard<std::mutex> lock(main_thread_queue_mutex_);
    main_thread_queue_.push(std::move(fn));
}

void Gta::SetOnHwndFound(std::function<void(HWND)> callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_hwnd_found_ = std::move(callback);

    if (found_.load(std::memory_order_acquire))
    {
        HWND hwnd = hwnd_.load(std::memory_order_acquire);
        if (hwnd)
        {
            LOG_DEBUG("[GTA] HWND already found, posting deferred callback...");
            PostToMainThread([this, hwnd]() {
                LOG_DEBUG("[GTA] Firing OnHwndFound callback on main thread (deferred).");
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (on_hwnd_found_)
                    on_hwnd_found_(hwnd);
            });
        }
    }
}

void Gta::SearchThreadLoop()
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    int iteration = 0;
    auto start = steady_clock::now();
    auto last_log = start;

    while (!shutdown_.load(std::memory_order_acquire) && !found_.load(std::memory_order_acquire))
    {
        iteration++;

        HWND hwnd = FindHwndFromMemory();

        if (!hwnd) 
            hwnd = FindHwndByTitle();

        if (!hwnd) 
            hwnd = FindHwndByClass();

        if (!hwnd && iteration > 50) 
            hwnd = FindHwndByProcess();

        if (hwnd && ::IsWindow(hwnd))
        {
            OnHwndDiscovered(hwnd);
            break;
        }

        auto now = steady_clock::now();
        if (duration_cast<seconds>(now - last_log).count() >= 3)
        {
            int elapsed = (int)duration_cast<seconds>(now - start).count();
            LOG_DEBUG("[GTA] Searching for HWND... (attempt={}, elapsed={}s)", iteration, elapsed);
            last_log = now;
        }

        auto delay = std::min(10ms + milliseconds(iteration * 2), 100ms);
        std::this_thread::sleep_for(delay);
    }
}

void Gta::OnHwndDiscovered(HWND hwnd)
{
    if (found_.exchange(true, std::memory_order_acq_rel))
        return;

    hwnd_.store(hwnd, std::memory_order_release);

    wchar_t title[256] = {};
    ::GetWindowTextW(hwnd, title, 256);

    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, title, -1, nullptr, 0, nullptr, nullptr);
    std::string titleStr;
    if (utf8Len > 1)
    {
        titleStr.resize(utf8Len - 1);
        WideCharToMultiByte(CP_UTF8, 0, title, -1, &titleStr[0], utf8Len, nullptr, nullptr);
    }

    LOG_DEBUG("[GTA] HWND found! handle={} title='{}'",
        static_cast<void*>(hwnd), titleStr);

    PostToMainThread([this, hwnd]() {
        LOG_DEBUG("[GTA] Firing OnHwndFound callback on main thread.");

        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_hwnd_found_)
            on_hwnd_found_(hwnd);
    });
}


HWND Gta::FindHwndFromMemory()
{
    __try {
        HWND* pHwnd = reinterpret_cast<HWND*>(HwndAddress);
        HWND  hwnd  = *pHwnd;
        if (hwnd && ::IsWindow(hwnd))
            return hwnd;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return nullptr;
}

HWND Gta::FindHwndByTitle()
{
    const DWORD pid = ::GetCurrentProcessId();

    HWND hwnd = ::FindWindowW(nullptr, L"GTA: San Andreas");
    if (hwnd && BelongsToCurrentProcess(hwnd, pid))
        return hwnd;

    hwnd = ::FindWindowW(nullptr, L"GTA:SA:MP");
    if (hwnd && BelongsToCurrentProcess(hwnd, pid))
        return hwnd;

    SearchData data{ pid, nullptr };

    ::EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* d = reinterpret_cast<SearchData*>(lp);

        DWORD wpid = 0;
        ::GetWindowThreadProcessId(h, &wpid);
        if (wpid != d->pid)
            return TRUE;

        wchar_t title[256] = {};
        if (::GetWindowTextW(h, title, 256) > 0)
        {
            std::wstring t(title);
            if (t.find(L"GTA") != std::wstring::npos &&
                t.find(L"San Andreas") != std::wstring::npos)
            {
                d->result = h;
                return FALSE;
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data));

    return data.result;
}

HWND Gta::FindHwndByClass()
{
    const DWORD pid = ::GetCurrentProcessId();

    HWND hwnd = ::FindWindowW(L"Grand theft auto San Andreas", nullptr);
    if (hwnd && BelongsToCurrentProcess(hwnd, pid))
        return hwnd;

    SearchData data{ pid, nullptr };

    ::EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* d = reinterpret_cast<SearchData*>(lp);

        DWORD wpid = 0;
        ::GetWindowThreadProcessId(h, &wpid);
        if (wpid != d->pid)
            return TRUE;

        wchar_t cls[256] = {};
        if (::GetClassNameW(h, cls, 256) > 0)
        {
            std::wstring c(cls);
            std::transform(c.begin(), c.end(), c.begin(), ::towlower);
            if (c.find(L"grand") != std::wstring::npos &&
                c.find(L"theft") != std::wstring::npos)
            {
                d->result = h;
                return FALSE;
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data));

    return data.result;
}

HWND Gta::FindHwndByProcess()
{
    const DWORD pid = ::GetCurrentProcessId();

    SearchData data{ pid, nullptr };

    ::EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* d = reinterpret_cast<SearchData*>(lp);
        DWORD wpid = 0;
        ::GetWindowThreadProcessId(h, &wpid);

        if (wpid == d->pid &&
            ::IsWindowVisible(h) &&
            ::GetParent(h) == nullptr)
        {
            d->result = h;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data));

    return data.result;
}

std::string Gta::GetUserFilesPath()
{
    PWSTR pwszPath = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &pwszPath)))
    {
        LOG_ERROR("[GTA] Failed to get Documents folder path.");
        return "";
    }

    int len = WideCharToMultiByte(CP_UTF8, 0, pwszPath, -1, nullptr, 0, nullptr, nullptr);
    std::string path(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, pwszPath, -1, &path[0], len, nullptr, nullptr);
    CoTaskMemFree(pwszPath);

    if (!path.empty() && path.back() == '\0')
        path.pop_back();

    try {
        std::filesystem::path result = path;
        result /= "GTA San Andreas User Files";
        return result.string();
    }
    catch (const std::exception& e) {
        LOG_ERROR("[GTA] Path error: {}", e.what());
        return "";
    }
}