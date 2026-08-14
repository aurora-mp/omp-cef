#include "manager.hpp"

#include <algorithm>
#include <filesystem>

#include <game_sa/CPlayerPed.h>
#include <game_sa/common.h>
#include <include/base/cef_bind.h>
#include <include/wrapper/cef_closure_task.h>
#include <include/wrapper/cef_helpers.h>
#include <windowsx.h>

#include "app.hpp"
#include "audio.hpp"
#include "client.hpp"
#include "focus.hpp"
#include "rendering/render_manager.hpp"
#include "network/network_manager.hpp"
#include "scheme_handler.hpp"
#include "system/gta.hpp"
#include "samp/components/netgame.hpp"
#include "game_sa/CSprite.h"
#include "game_sa/CCamera.h"
#include "shared/events.hpp"
#include "utf8.hpp"

static void ConfigureBrowserSettings(CefBrowserSettings& settings)
{
    settings.javascript = STATE_ENABLED;
    settings.javascript_access_clipboard = STATE_ENABLED;
    settings.javascript_dom_paste = STATE_ENABLED;
    settings.remote_fonts = STATE_ENABLED;
    settings.webgl = STATE_ENABLED;
    settings.tab_to_links = STATE_DISABLED;
}

static uint32_t GetCefEventFlags()
{
    uint32_t flags = 0;
    if (GetKeyState(VK_SHIFT) & 0x8000)
        flags |= EVENTFLAG_SHIFT_DOWN;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        flags |= EVENTFLAG_CONTROL_DOWN;
    if (GetKeyState(VK_MENU) & 0x8000)
        flags |= EVENTFLAG_ALT_DOWN;
    if (GetKeyState(VK_LBUTTON) & 0x8000)
        flags |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    if (GetKeyState(VK_RBUTTON) & 0x8000)
        flags |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
    if (GetKeyState(VK_MBUTTON) & 0x8000)
        flags |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    if (GetKeyState(VK_CAPITAL) & 1)
        flags |= EVENTFLAG_CAPS_LOCK_ON;
    if (GetKeyState(VK_NUMLOCK) & 1)
        flags |= EVENTFLAG_NUM_LOCK_ON;
    return flags;
}

namespace
{
    static bool EnsureDirectory(const std::filesystem::path& path, const char* label)
    {
        std::error_code error_code;
        std::filesystem::create_directories(path, error_code);

        if (error_code)
        {
            LOG_FATAL("[CEF] Cannot create {} directory '{}': {}", label, path.string().c_str(), error_code.message().c_str());
            return false;
        }

        if (!std::filesystem::is_directory(path))
        {
            LOG_FATAL("[CEF] {} directory does not exist: {}", label, path.string().c_str());
            return false;
        }

        return true;
    }

    static bool RequireFile(const std::filesystem::path& path, const char* label)
    {
        std::error_code error_code;
        const bool exists = std::filesystem::is_regular_file(path, error_code);

        if (error_code || !exists)
        {
            if (error_code)
                LOG_FATAL("[CEF] Cannot access required {} '{}': {}", label, path.string().c_str(), error_code.message().c_str());
            else
                LOG_FATAL("[CEF] Missing required {}: {}", label, path.string().c_str());

            return false;
        }

        LOG_DEBUG("[CEF] {}: {}", label, path.string().c_str());
        return true;
    }

    static bool RequireDirectory(const std::filesystem::path& path, const char* label)
    {
        std::error_code error_code;
        const bool exists = std::filesystem::is_directory(path, error_code);

        if (error_code || !exists)
        {
            if (error_code)
                LOG_FATAL("[CEF] Cannot access required {} directory '{}': {}", label, path.string().c_str(), error_code.message().c_str());
            else
                LOG_FATAL("[CEF] Missing required {} directory: {}", label, path.string().c_str());

            return false;
        }

        LOG_DEBUG("[CEF] {} directory: {}", label, path.string().c_str());
        return true;
    }

    static void LogOptionalFile(const std::filesystem::path& path, const char* label)
    {
        std::error_code error_code;
        const bool exists = std::filesystem::is_regular_file(path, error_code);

        if (exists)
            LOG_DEBUG("[CEF] {}: {}", label, path.string().c_str());
        else if (error_code)
            LOG_WARN("[CEF] Cannot access optional {} '{}': {}", label, path.string().c_str(), error_code.message().c_str());
        else
            LOG_WARN("[CEF] Optional {} not found: {}", label, path.string().c_str());
    }

    static bool ValidateCefRuntime(const std::filesystem::path& cef_dir)
    {
        bool valid = true;

        valid &= RequireDirectory(cef_dir, "runtime");
        valid &= RequireFile(cef_dir / "renderer.exe", "renderer subprocess");
        valid &= RequireFile(cef_dir / "libcef.dll", "libcef.dll");
        valid &= RequireFile(cef_dir / "icudtl.dat", "ICU data file");
        valid &= RequireFile(cef_dir / "resources.pak", "resources.pak");
        valid &= RequireDirectory(cef_dir / "locales", "locales");

        LogOptionalFile(cef_dir / "chrome_100_percent.pak", "chrome_100_percent.pak");
        LogOptionalFile(cef_dir / "chrome_200_percent.pak", "chrome_200_percent.pak");
        LogOptionalFile(cef_dir / "v8_context_snapshot.bin", "v8_context_snapshot.bin");

        return valid;
    }

    class DevToolsClient final : public CefClient, public CefLifeSpanHandler
    {
    public:
        DevToolsClient(int ownerId, BrowserManager* mgr)
            : ownerId_(ownerId), mgr_(mgr) {}

        CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }

        void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
        {
            CEF_REQUIRE_UI_THREAD();

            if (!mgr_)
            {
                if (auto h = browser->GetHost()) h->CloseBrowser(true);
                return;
            }

            auto* inst = mgr_->GetBrowserInstance(ownerId_);
            if (!inst)
            {
                if (auto h = browser->GetHost()) h->CloseBrowser(true);
                return;
            }

            if (!inst->devtools_requested)
            {
                if (auto h = browser->GetHost()) h->CloseBrowser(true);
                return;
            }

            inst->devtools_browser = browser;
            inst->devtools_open = true;
        }

        void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
        {
            CEF_REQUIRE_UI_THREAD();

            if (!mgr_) return;

            auto* inst = mgr_->GetBrowserInstance(ownerId_);
            if (!inst) return;

            if (inst->devtools_browser && inst->devtools_browser->IsSame(browser))
            {
                inst->devtools_browser = nullptr;
                inst->devtools_client = nullptr;
                inst->devtools_open = false;
            }
        }

    private:
        const int ownerId_;
        BrowserManager* mgr_;

        IMPLEMENT_REFCOUNTING(DevToolsClient);
    };
}

bool BrowserManager::Initialize()
{
    if (initialized_)
        return true;

    LOG_INFO("Initialize CEF Browser manager ...");

    CefMainArgs main_args(GetModuleHandle(nullptr));
    CefRefPtr<DefaultCefApp> app = new DefaultCefApp();
    CefSettings settings;

    wchar_t exe_path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

    const auto base = std::filesystem::path(exe_path).parent_path();
    const auto cef_dir = base / "cef";
    const auto cef_subprocess = cef_dir / "renderer.exe";
    const auto cef_resources_dir = cef_dir;
    const auto cef_locales_dir = cef_dir / "locales";

    // Use GTA user files (writable) for ALL CEF internal data.
    // Keep game folder 'cef/' for binaries/resources, not for cache/profile.
    const auto user_files = std::filesystem::path(gta_.GetUserFilesPath());
    const auto user_cef_root = user_files / "cef";
    const auto user_cef_ui_cache = user_cef_root / "cache";

    // CEF internal cache/profile
    const auto user_cache_dir = user_cef_root / "cef_cache";

    // CEF debug log
    const auto cef_log_file = user_cef_root / "debug.log";

    LOG_DEBUG("[CEF] Game executable: {}", std::filesystem::path(exe_path).string().c_str());
    LOG_DEBUG("[CEF] Game directory: {}", base.string().c_str());
    LOG_DEBUG("[CEF] Runtime directory: {}", cef_dir.string().c_str());
    LOG_DEBUG("[CEF] Resources directory: {}", cef_resources_dir.string().c_str());
    LOG_DEBUG("[CEF] Locales directory: {}", cef_locales_dir.string().c_str());
    LOG_DEBUG("[CEF] Renderer subprocess: {}", cef_subprocess.string().c_str());
    LOG_DEBUG("[CEF] UserFiles: {}", user_files.string().c_str());
    LOG_DEBUG("[CEF] UI cache: {}", user_cef_ui_cache.string().c_str());
    LOG_DEBUG("[CEF] CEF cache: {}", user_cache_dir.string().c_str());
    LOG_DEBUG("[CEF] CEF log: {}", cef_log_file.string().c_str());

    if (!ValidateCefRuntime(cef_dir))
    {
        LOG_FATAL("[CEF] Runtime validation failed. Reinstall the full omp-cef client package. Do not mix CEF files from different versions.");
        return false;
    }

    if (!EnsureDirectory(user_cef_ui_cache, "UI cache"))
        return false;

    if (!EnsureDirectory(user_cache_dir, "CEF cache"))
        return false;

    CefString(&settings.log_file) = cef_log_file.wstring();
    CefString(&settings.root_cache_path) = user_cache_dir.wstring();
    CefString(&settings.cache_path) = user_cache_dir.wstring();
    CefString(&settings.browser_subprocess_path) = cef_subprocess.wstring();
    CefString(&settings.resources_dir_path) = cef_resources_dir.wstring();
    CefString(&settings.locales_dir_path) = cef_locales_dir.wstring();

    settings.no_sandbox = true;
    settings.log_severity = LOGSEVERITY_INFO;
    settings.multi_threaded_message_loop = true;
    settings.windowless_rendering_enabled = true;
    settings.persist_session_cookies = true;

    int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);
    if (exit_code >= 0)
    {
        LOG_FATAL("[CEF] CefExecuteProcess exited with code: {}.", exit_code);
        return false;
    }

    if (!CefInitialize(main_args, settings, app.get(), nullptr))
    {
        LOG_FATAL("[CEF] CefInitialize returned false.");
        LOG_FATAL("[CEF] Runtime directory: {}", cef_dir.string().c_str());
        LOG_FATAL("[CEF] Resources directory: {}", cef_resources_dir.string().c_str());
        LOG_FATAL("[CEF] Locales directory: {}", cef_locales_dir.string().c_str());
        LOG_FATAL("[CEF] Renderer subprocess: {}", cef_subprocess.string().c_str());
        LOG_FATAL("[CEF] Cache directory: {}", user_cache_dir.string().c_str());
        LOG_FATAL("[CEF] Delete the CEF cache and reinstall the full matching client package if this persists.");
        return false;
    }

    // Register a custom scheme handler for "cef://"
    CefRegisterSchemeHandlerFactory("http", "cef", new LocalSchemeHandlerFactory(resource_manager_));

    initialized_ = true;
    uiThreadId_ = GetCurrentThreadId();

    LOG_INFO("[CEF] Browser manager initialized on UI thread id: {}.", uiThreadId_);

    // Hook the device lifecycle callbacks
    auto& render_manager = RenderManager::Instance();

    render_manager.OnBeforeReset = [this]() {
        this->OnDeviceLost();
    };
    
    render_manager.OnAfterReset = [this](IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS& pp) {
        this->OnDeviceReset(device);
    };
    
    LOG_INFO("[CEF] Browser manager device lifecycle callbacks registered.");
    return true;
}

static bool StartsWithI(const std::string& s, const std::string& prefix)
{
    if (s.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i)
    {
        if (std::tolower((unsigned char)s[i]) != std::tolower((unsigned char)prefix[i]))
            return false;
    }
    return true;
}

static std::string UrlDecode(const std::string& in)
{
    std::string out;
    out.reserve(in.size());

    for (size_t i = 0; i < in.size(); ++i)
    {
        const char c = in[i];
        if (c == '%' && i + 2 < in.size())
        {
            auto hex = [](char h) -> int {
                if (h >= '0' && h <= '9') 
                    return h - '0';
                if (h >= 'a' && h <= 'f') 
                    return 10 + (h - 'a');
                if (h >= 'A' && h <= 'F') 
                    return 10 + (h - 'A');

                return -1;
            };

            const int hi = hex(in[i+1]);
            const int lo = hex(in[i+2]);

            if (hi >= 0 && lo >= 0)
            {
                out.push_back((char)((hi << 4) | lo));
                i += 2;
                continue;
            }
        }

        if (c == '+') { 
            out.push_back(' '); 
            continue; 
        }

        out.push_back(c);
    }

    return out;
}

static std::string GetQueryParam(const std::string& url, const std::string& key)
{
    const auto qpos = url.find('?');
    if (qpos == std::string::npos) 
        return {};

    const auto frag = url.find('#', qpos + 1);
    const std::string qs = url.substr(qpos + 1, (frag == std::string::npos ? url.size() : frag) - (qpos + 1));

    size_t start = 0;
    while (start < qs.size())
    {
        const size_t amp = qs.find('&', start);
        const size_t end = (amp == std::string::npos) ? qs.size() : amp;
        const size_t eq = qs.find('=', start);

        std::string k, v;
        if (eq != std::string::npos && eq < end)
        {
            k = qs.substr(start, eq - start);
            v = qs.substr(eq + 1, end - (eq + 1));
        }
        else
        {
            k = qs.substr(start, end - start);
        }

        if (UrlDecode(k) == key)
            return UrlDecode(v);

        start = end + 1;
    }

    return {};
}

static std::string ExtractYouTubeId(const std::string& url)
{
    // Accept:
    // - https://www.youtube.com/embed/VIDEO_ID
    // - https://youtu.be/VIDEO_ID
    // - https://www.youtube.com/watch?v=VIDEO_ID
    // - https://www.youtube.com/shorts/VIDEO_ID
    const std::string u = url;

    auto extractAfter = [&](const std::string& needle) -> std::string {
        auto p = u.find(needle);
        if (p == std::string::npos) 
            return {};

        p += needle.size();
        if (p >= u.size()) 
            return {};

        size_t e = p;
        while (e < u.size())
        {
            const char c = u[e];
            if (c == '?' || c == '&' || c == '#' || c == '/') 
                break;

            ++e;
        }

        return u.substr(p, e - p);
    };

    if (auto id = extractAfter("youtube.com/embed/"); !id.empty()) return id;
    if (auto id = extractAfter("youtube-nocookie.com/embed/"); !id.empty()) return id;
    if (auto id = extractAfter("youtu.be/"); !id.empty()) return id;
    if (auto id = extractAfter("youtube.com/shorts/"); !id.empty()) return id;

    if (StartsWithI(u, "http") && u.find("youtube.com/") != std::string::npos)
    {
        auto id = GetQueryParam(u, "v");
        if (!id.empty()) return id;
    }

    return {};
}

static std::string NormalizeUrlForEmbeds(const std::string& url)
{
    // YouTube -> internal wrapper
    if (url.find("youtube.com") != std::string::npos || url.find("youtu.be") != std::string::npos || url.find("youtube-nocookie.com") != std::string::npos)
    {
        const std::string id = ExtractYouTubeId(url);
        if (!id.empty())
        {
            const std::string autoplay = GetQueryParam(url, "autoplay").empty() ? "1" : GetQueryParam(url, "autoplay");
            const std::string mute = GetQueryParam(url, "mute").empty() ? "1" : GetQueryParam(url, "mute");
            const std::string controls = GetQueryParam(url, "controls").empty() ? "0" : GetQueryParam(url, "controls");
            const std::string rel = GetQueryParam(url, "rel").empty() ? "0" : GetQueryParam(url, "rel");
            const std::string start = GetQueryParam(url, "start");
            const std::string end = GetQueryParam(url, "end");

            std::string internal = "http://cef/__internal/youtube.html?v=" + id +
                "&autoplay=" + autoplay +
                "&mute=" + mute +
                "&controls=" + controls +
                "&rel=" + rel;

            if (!start.empty()) internal += "&start=" + start;
            if (!end.empty()) internal += "&end=" + end;

            return internal;
        }
    }

    // Twitch -> internal wrapper
    if (url.find("twitch.tv") != std::string::npos)
    {
        // If already a player.twitch.tv URL, we keep query but strip "parent".
        const auto qpos = url.find('?');
        if (qpos != std::string::npos)
        {
            std::string qs = url.substr(qpos + 1);

            std::string out;
            size_t start = 0;
            while (start < qs.size())
            {
                size_t amp = qs.find('&', start);
                size_t end = (amp == std::string::npos) ? qs.size() : amp;
                std::string kv = qs.substr(start, end - start);

                std::string k = kv;
                auto eq = kv.find('=');
                if (eq != std::string::npos) k = kv.substr(0, eq);
                k = UrlDecode(k);

                if (k != "parent")
                {
                    if (!out.empty()) out.push_back('&');
                    out += kv;
                }

                start = end + 1;
            }

            if (out.find("channel=") != std::string::npos || out.find("video=") != std::string::npos || out.find("clip=") != std::string::npos)
            {
                return std::string("http://cef/__internal/twitch.html?") + out;
            }
        }

        // https://www.twitch.tv/<channel> -> internal wrapper
        const auto slash = url.rfind('/');
        if (slash != std::string::npos && slash + 1 < url.size())
        {
            std::string tail = url.substr(slash + 1);
            const auto q = tail.find_first_of("?#");
            if (q != std::string::npos) tail = tail.substr(0, q);

            if (!tail.empty() && tail != "videos" && tail != "clips")
            {
                return std::string("http://cef/__internal/twitch.html?channel=") + tail;
            }
        }
    }

    return url;
}

void BrowserManager::Shutdown()
{
    if (!initialized_ || is_shutting_down_.exchange(true))
        return;

    LOG_DEBUG("Shutting down CEF Browser manager...");

    screen_capture_.StopAll();
    screen_capture_.OnDeviceLost();
    browsers_.clear();
    worldRenderers_.clear();
    entityToBrowserId_.clear();

    CefShutdown();
    initialized_ = false;

    LOG_INFO("CEF Browser manager shut down successfully.");
}

void BrowserManager::CreateBrowser(
    int id, const std::string& url, bool focused, bool controls_chat, float width, float height)
{
    LOG_DEBUG("[CEF] CreateBrowser called with ID={}, url={}", id, url);
    
    if (browsers_.count(id))
    {
        LOG_ERROR("[CEF] CreateBrowser failed: Browser with ID {} already exists.", id);
        LOG_ERROR("[CEF] Existing browser mode: {}", (int)browsers_[id]->mode);
        return;
    }
    CreateBrowserInternal(id, url, focused, controls_chat, width, height);
}

void BrowserManager::CreateWorldBrowser(
    int id, const std::string& url, const std::string& textureName, float width, float height)
{
    LOG_DEBUG("[CEF] CreateWorldBrowser called with ID={}, url={}", id, url);
    
    const std::string normalized_url = NormalizeUrlForEmbeds(url);
    if (normalized_url != url)
        LOG_DEBUG("[CEF] Normalized URL -> {}", normalized_url);

    if (browsers_.count(id))
    {
        LOG_ERROR("[CEF] CreateWorldBrowser failed: Browser with ID {} already exists.", id);
        LOG_ERROR("[CEF] Existing browser mode: {}", (int)browsers_[id]->mode);
        return;
    }

    CreateWorldBrowserInternal(id, normalized_url, textureName, width, height);
}

void BrowserManager::CreateWorld2DBrowser(
    int id, const std::string& url, float worldX, float worldY, float worldZ, float width, float height, float offsetZ, float pivotX, float pivotY)
{
    LOG_DEBUG("[CEF] CreateWorld2DBrowser called with ID={}, url={}, worldX={}, worldY={}, worldZ={}, offsetZ={}, pivotX={}, pivotY={}", 
        id, url, worldX, worldY, worldZ, offsetZ, pivotX, pivotY);
    
    const std::string normalized_url = NormalizeUrlForEmbeds(url);
    if (normalized_url != url)
        LOG_DEBUG("[CEF] Normalized URL -> {}", normalized_url);

    if (browsers_.count(id))
    {
        LOG_ERROR("[CEF] CreateWorld2DBrowser failed: Browser with ID {} already exists.", id);
        LOG_ERROR("[CEF] Existing browser mode: {}", (int)browsers_[id]->mode);
        return;
    }

    CreateWorld2DBrowserInternal(id, normalized_url, worldX, worldY, worldZ, width, height, offsetZ, pivotX, pivotY);
}

void BrowserManager::CreateBrowserInternal(
    int id, const std::string& url, bool focused, bool controls_chat, float width, float height)
{
    if (CefCurrentlyOn(TID_UI) == false)
    {
        CefPostTask(TID_UI,
                    base::BindOnce(&BrowserManager::CreateBrowserInternal,
                                   base::Unretained(this),
                                   id,
                                   url,
                                   focused,
                                   controls_chat,
                                   width,
                                   height));
        return;
    }

    if (browsers_.count(id))
    {
        LOG_ERROR("[CEF] CreateBrowserInternal: Browser with ID {} already exists (race condition?).", id);
        return;
    }

    auto instance = std::make_unique<BrowserInstance>(id);
    instance->mode = RenderMode::Overlay2D;
    instance->url = url;
    instance->client = BrowserClient::Create(id, *this, audio_, focus_, network_);
    instance->controls_chat_input = controls_chat;

    browsers_[id] = std::move(instance);

    gta_.PostToMainThread([this, id, width, height]() {
        auto* inst = GetBrowserInstance(id);
        if (!inst) return;

        auto* device = RenderManager::Instance().GetDevice();
        if (!device) return;

        inst->view.Initialize(device);

        int browser_width  = (int)width;
        int browser_height = (int)height;

        if (browser_width  <= 0 || browser_height <= 0)
        {
            float screen_width, screen_height;
            if (RenderManager::Instance().GetScreenSize(screen_width, screen_height))
            {
                browser_width  = (int)screen_width;
                browser_height = (int)screen_height;
            }
            else
            {
                browser_width  = 1280;
                browser_height = 720;
            }
        }

        D3DCAPS9 caps{};
        int maxTextureWidth = 7680; // 8k
        int maxTextureHeight = 4320;

        if (SUCCEEDED(device->GetDeviceCaps(&caps)))
        {
            maxTextureWidth = std::clamp(static_cast<int>(caps.MaxTextureWidth), 1, maxTextureWidth);
            maxTextureHeight = std::clamp(static_cast<int>(caps.MaxTextureHeight), 1, maxTextureHeight);
        }

        browser_width = std::clamp(browser_width, 1, maxTextureWidth);
        browser_height = std::clamp(browser_height, 1, maxTextureHeight);
        inst->view.Create(browser_width, browser_height);
    });

    if (focused)
        FocusBrowser(id, true);

    CefWindowInfo windowInfo;
    windowInfo.SetAsWindowless(gta_.GetHwnd());
    windowInfo.external_begin_frame_enabled = true;
    CefBrowserSettings bs;
    ConfigureBrowserSettings(bs);
    CefBrowserHost::CreateBrowser(
        windowInfo, browsers_[id]->client, url, bs, nullptr, CefRequestContext::GetGlobalContext());
}

void BrowserManager::CreateWorldBrowserInternal(
    int id, const std::string& url, std::string textureName, float width, float height)
{
    if (CefCurrentlyOn(TID_UI) == false)
    {
        CefPostTask(TID_UI,
                    base::BindOnce(&BrowserManager::CreateWorldBrowserInternal,
                                   base::Unretained(this),
                                   id,
                                   url,
                                   textureName,
                                   width,
                                   height));
        return;
    }

    if (browsers_.count(id))
    {
        LOG_ERROR("[CEF] CreateWorldBrowserInternal: Browser with ID {} already exists (race condition?).", id);
        return;
    }

    auto instance = std::make_unique<BrowserInstance>(id);
    instance->mode = RenderMode::WorldObject3D;
    instance->url = url;
    instance->textureName = textureName;
    instance->client = BrowserClient::Create(id, *this, audio_, focus_, network_);
    browsers_[id] = std::move(instance);

    const int browser_width = std::clamp((int)width, 1, 1024);
    const int browser_height = std::clamp((int)height, 1, 1024);

    gta_.PostToMainThread([this, id, textureName, browser_width, browser_height]() {
        auto* inst = GetBrowserInstance(id);
        if (!inst) return;

        auto* device = RenderManager::Instance().GetDevice();
        if (!device) return;

        worldRenderers_[id] = std::make_unique<WorldRenderer>(textureName, (float)browser_width, (float)browser_height);
        inst->view.Initialize(device);
        inst->view.Create(browser_width, browser_height);
    });

    // Prepare audio stream but keep it muted until attached to a world entity
    audio_.EnsureStream(id);
    audio_.SetStreamMuted(id, true);

    CefWindowInfo windowInfo;
    windowInfo.SetAsWindowless(gta_.GetHwnd());
    windowInfo.external_begin_frame_enabled = true;
    CefBrowserSettings bs;
    ConfigureBrowserSettings(bs);
    CefBrowserHost::CreateBrowser(windowInfo, browsers_[id]->client, url, bs, nullptr, CefRequestContext::GetGlobalContext());
}

void BrowserManager::CreateWorld2DBrowserInternal(
    int id, const std::string& url, float worldX, float worldY, float worldZ, float width, float height, float offsetZ, float pivotX, float pivotY)
{
    if (CefCurrentlyOn(TID_UI) == false)
    {
        CefPostTask(TID_UI,
            base::BindOnce(&BrowserManager::CreateWorld2DBrowserInternal,
                base::Unretained(this),
                id,
                url,
                worldX,
                worldY,
                worldZ,
                width,
                height,
                offsetZ,
                pivotX,
                pivotY));
        return;
    }

    if (browsers_.count(id))
    {
        LOG_ERROR("[CEF] CreateWorld2DBrowserInternal: Browser with ID {} already exists (race condition?).", id);
        return;
    }

    auto instance = std::make_unique<BrowserInstance>(id);
    instance->mode = RenderMode::World2D;
    instance->url = url;
    instance->client = BrowserClient::Create(id, *this, audio_, focus_, network_);
    instance->controls_chat_input = false;

    instance->world2d.x = worldX;
    instance->world2d.y = worldY;
    instance->world2d.z = worldZ;
    instance->world2d.offsetZ = offsetZ;
    instance->world2d.pivotX = pivotX;
    instance->world2d.pivotY = pivotY;
    browsers_[id] = std::move(instance);

    gta_.PostToMainThread([this, id, width, height]() {
        auto* inst = GetBrowserInstance(id);
        if (!inst) return;

        auto* device = RenderManager::Instance().GetDevice();
        if (!device) return;

        inst->view.Initialize(device);

        int bw = std::clamp((int)width, 1, 1024);
        int bh = std::clamp((int)height, 1, 1024);
        inst->view.Create(bw, bh);
    });

    CefWindowInfo windowInfo;
    windowInfo.SetAsWindowless(gta_.GetHwnd());
    windowInfo.external_begin_frame_enabled = true;
    CefBrowserSettings bs;
    ConfigureBrowserSettings(bs);
    CefBrowserHost::CreateBrowser(windowInfo, browsers_[id]->client, url, bs, nullptr, CefRequestContext::GetGlobalContext());
}

void BrowserManager::SetWorld2DBrowserPos(int id, float worldX, float worldY, float worldZ)
{
    if (CefCurrentlyOn(TID_UI) == false)
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::SetWorld2DBrowserPos, base::Unretained(this), id, worldX, worldY, worldZ));
        return;
    }

    auto* instance = GetBrowserInstance(id);
    if (!instance)
    {
        LOG_WARN("[CEF] SetWorld2DBrowserPos: Could not find browser with ID {}.", id);
        return;
    }

    if (instance->mode != RenderMode::World2D)
    {
        LOG_WARN("[CEF] SetWorld2DBrowserPos: Browser {} is not a World2D browser (mode={}).", id, (int)instance->mode);
        return;
    }

    instance->world2d.x = worldX;
    instance->world2d.y = worldY;
    instance->world2d.z = worldZ;
}

void BrowserManager::SetBrowserVisible(int id, bool visible)
{
    if (CefCurrentlyOn(TID_UI) == false)
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::SetBrowserVisible, base::Unretained(this), id, visible));
        return;
    }

    auto* instance = GetBrowserInstance(id);
    if (!instance)
    {
        LOG_WARN("[CEF] SetBrowserVisible: Could not find browser with ID {}.", id);
        return;
    }

    instance->visible = visible;

    if (instance->browser)
    {
        if (auto host = instance->browser->GetHost())
        {
            host->WasHidden(!visible);

            if (visible)
            {
                host->WasResized();
                host->Invalidate(PET_VIEW);
            }
        }
    }

    if (!visible)
    {
        ClearPendingPaint(id);

        if (focusedBrowserId_ == id)
            FocusBrowser(id, false);
    }
}

void BrowserManager::DestroyBrowser(int id)
{
    if (!CefCurrentlyOn(TID_UI))
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::DestroyBrowser, base::Unretained(this), id));
        return;
    }

    player_stats_poll_.erase(id);
    pending_.erase(id);

    auto it = browsers_.find(id);
    if (it == browsers_.end())
        return;

    auto& instance = it->second;

    if (focusedBrowserId_ == id)
        FocusBrowser(id, false);

    if (instance->browser && instance->browser->GetHost())
        instance->browser->GetHost()->CloseDevTools();

    instance->devtools_requested = false;

    if (instance->devtools_browser && instance->devtools_browser->GetHost())
        instance->devtools_browser->GetHost()->CloseBrowser(true);

    instance->devtools_open = false;
    instance->devtools_browser = nullptr;
    instance->devtools_client = nullptr;

    for (auto eit = entityToBrowserId_.begin(); eit != entityToBrowserId_.end();)
    {
        if (eit->second == id)
        {
            OnAfterEntityRender(eit->first);
            eit = entityToBrowserId_.erase(eit);
        }
        else
        {
            ++eit;
        }
    }

    worldRenderers_.erase(id);

    if (instance->browser && instance->browser->GetHost())
    {
        instance->view.SetFocused(false);
        instance->browser->GetHost()->CloseBrowser(true);
        instance->browser = nullptr;
    }

    browsers_.erase(it);
    LOG_DEBUG("[CEF] Browser ID {} destroyed and removed from map.", id);
}

void BrowserManager::DestroyAllBrowsers()
{
    if (!CefCurrentlyOn(TID_UI))
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::DestroyAllBrowsers, base::Unretained(this)));
        return;
    }

    std::vector<int> ids;
    ids.reserve(browsers_.size());
    for (auto& kv : browsers_)
        ids.push_back(kv.first);

    for (int id : ids)
        DestroyBrowser(id);
}

void BrowserManager::ReloadBrowser(int id, bool ignoreCache)
{
    if (CefCurrentlyOn(TID_UI) == false)
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::ReloadBrowser, base::Unretained(this), id, ignoreCache));
        return;
    }

    if (auto* inst = GetBrowserInstance(id))
    {
        if (inst->browser)
        {
            ignoreCache ? inst->browser->ReloadIgnoreCache() : inst->browser->Reload();
            LOG_DEBUG("[CEF] Reloading browser with ID {}.", id);
        }
    }
}

void BrowserManager::LoadUrl(int id, const std::string& url)
{
    if (!CefCurrentlyOn(TID_UI))
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::LoadUrl, base::Unretained(this), id, url));
        return;
    }

    auto* inst = GetBrowserInstance(id);
    if (!inst || !inst->browser)
    {
        LOG_WARN("[CEF] LoadUrl: Could not find browser with ID {}.", id);
        return;
    }

    auto frame = inst->browser->GetMainFrame();
    if (!frame)
    {
        LOG_WARN("[CEF] LoadUrl: Browser ID {} has no main frame.", id);
        return;
    }

    frame->LoadURL(url);

    LOG_DEBUG("[CEF] Loading URL in browser ID {}: {}", id, url);
}

void BrowserManager::SetDevToolsEnabled(int browserId, bool enabled)
{
    if (!CefCurrentlyOn(TID_UI))
    {
        CefPostTask(TID_UI,
            base::BindOnce(&BrowserManager::SetDevToolsEnabled,
                base::Unretained(this), browserId, enabled));
        return;
    }

    auto* inst = GetBrowserInstance(browserId);
    if (!inst || !inst->browser)
        return;

    auto host = inst->browser->GetHost();
    if (!host)
        return;

    inst->devtools_requested = enabled;

    if (enabled)
    {
        if (inst->devtools_open)
            return;

        if (!inst->devtools_client)
            inst->devtools_client = new DevToolsClient(browserId, this);

        CefWindowInfo windowInfo;
        windowInfo.SetAsPopup(nullptr, "DevTools");

        CefBrowserSettings settings;
        host->ShowDevTools(windowInfo, inst->devtools_client, settings, CefPoint());

        LOG_INFO("[CEF] DevTools enabled for browser {}", browserId);
    }
    else
    {
        host->CloseDevTools();

        if (inst->devtools_browser && inst->devtools_browser->GetHost())
            inst->devtools_browser->GetHost()->CloseBrowser(true);

        inst->devtools_open = false;
        inst->devtools_browser = nullptr;
        inst->devtools_client = nullptr;

        LOG_INFO("[CEF] DevTools disabled for browser {}", browserId);
    }
}

void BrowserManager::AttachBrowserToObject(int browserId, int objectId)
{
    if (!browsers_.count(browserId))
    {
        LOG_WARN("[CEF] AttachBrowserToObject: Browser ID {} does not exist.", browserId);
        return;
    }
    CEntity* nativeEntity = GetEntityFromObjectId(objectId);
    if (nativeEntity)
    {
        entityToBrowserId_[nativeEntity] = browserId;
        audio_.SetStreamMuted(browserId, false); // unmute when attached

        LOG_DEBUG("[CEF] Browser {} attached to object {} (Entity: {})", browserId, objectId, (const void*)nativeEntity);
    }
    else
    {
        LOG_WARN("[CEF] AttachBrowserToObject: Could not find entity for object ID {}.", objectId);
    }
}

void BrowserManager::DetachBrowserFromObject(int browserId, int objectId)
{
    CEntity* nativeEntity = GetEntityFromObjectId(objectId);
    if (nativeEntity)
    {
        auto it = entityToBrowserId_.find(nativeEntity);
        if (it != entityToBrowserId_.end() && it->second == browserId)
        {
            OnAfterEntityRender(nativeEntity); // ensure texture restored
            entityToBrowserId_.erase(it);
            audio_.SetStreamMuted(browserId, true); // mute when detached
            LOG_DEBUG("[CEF] Browser {} detached from object {} (Entity: {})",
                      browserId,
                      objectId,
                      (const void*)nativeEntity);
        }
    }
    else
    {
        LOG_WARN("[CEF] DetachBrowserFromObject: Could not find entity for object ID {}.", objectId);
    }
}

CEntity* BrowserManager::GetEntityFromObjectId(int objectId)
{
    auto* netGame = GetComponent<NetGameComponent>();
    if (!netGame)
        return nullptr;

    return netGame->GetEntityFromObjectId(objectId);
}

void BrowserManager::OnBrowserCreated(int id, CefRefPtr<CefBrowser> browser)
{
    auto it = browsers_.find(id);
    if (it != browsers_.end())
    {
        it->second->browser = browser;
        if (auto host = browser->GetHost())
        {
            host->WasResized();
            host->Invalidate(PET_VIEW);
            if (focusedBrowserId_ == id)
            {
                it->second->view.SetFocused(true);
                browser->GetHost()->SetFocus(true);
            }
        }

        network_.SendBrowserCreateResult(id, true, static_cast<int>(BrowserCreateStatus::Success), "Successfully created");
    }
}

void BrowserManager::OnBrowserClosed(int id)
{
    player_stats_poll_.erase(id);
    screen_capture_.Stop(id);
    pending_.erase(id);
    browsers_.erase(id);
}

void BrowserManager::StartScreenCapture(int browserId, int width, int height, int fps)
{
    if (!GetBrowserInstance(browserId))
        return;

    const auto config = screen_capture_.Start(browserId, width, height, fps);
    LOG_INFO("[ScreenCapture] Started for browser {} ({}x{} @ {} FPS)",
        browserId, config.width, config.height, config.fps);
}

void BrowserManager::StopScreenCapture(int browserId)
{
    screen_capture_.Stop(browserId);
    LOG_INFO("[ScreenCapture] Stopped for browser {}", browserId);
}

void BrowserManager::CaptureScreen()
{
    if (isCefUpdatesPaused_ || is_shutting_down_ || !screen_capture_.HasSubscriptions())
        return;

    auto* device = RenderManager::Instance().GetDevice();
    if (!device)
        return;

    auto frames = screen_capture_.CaptureDueFrames(device, ::GetTickCount64());
    for (auto& frame : frames)
    {
        const int browser_id = frame->browser_id;
        const uint64_t generation = frame->generation;
        if (!CefPostTask(TID_UI, base::BindOnce(
                &BrowserManager::DispatchScreenFrameOnUi,
                base::Unretained(this),
                std::move(frame))))
        {
            screen_capture_.CompleteFrame(browser_id, generation);
        }
    }
}

void BrowserManager::DispatchScreenFrameOnUi(std::shared_ptr<CapturedScreenFrame> frame)
{
    CEF_REQUIRE_UI_THREAD();

    if (!frame)
        return;

    const auto complete = [this, &frame]() {
        screen_capture_.CompleteFrame(frame->browser_id, frame->generation);
    };

    if (is_shutting_down_ ||
        !screen_capture_.IsCurrent(frame->browser_id, frame->generation) ||
        !frame->rgba || frame->rgba->empty())
    {
        complete();
        return;
    }

    auto* instance = GetBrowserInstance(frame->browser_id);
    if (!instance || !instance->browser || !instance->browser->IsValid())
    {
        complete();
        return;
    }

    CefRefPtr<CefFrame> main_frame = instance->browser->GetMainFrame();
    if (!main_frame || !main_frame->IsValid())
    {
        complete();
        return;
    }

    CefRefPtr<CefBinaryValue> pixels = CefBinaryValue::Create(
        frame->rgba->data(), frame->rgba->size());
    if (!pixels)
    {
        complete();
        return;
    }

    CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("cef_screen_capture_frame");
    CefRefPtr<CefListValue> args = message->GetArgumentList();
    args->SetBinary(0, pixels);
    args->SetInt(1, frame->width);
    args->SetInt(2, frame->height);
    args->SetInt(3, static_cast<int>(frame->sequence));
    args->SetDouble(4, frame->timestamp_ms);
    main_frame->SendProcessMessage(PID_RENDERER, message);

    complete();
}

void BrowserManager::ClearPendingPaint(int id)
{
    auto it = pending_.find(id);
    if (it == pending_.end())
        return;

    auto& pending_paint = it->second;
    std::lock_guard<std::mutex> lock(pending_paint.mutex);
    pending_paint.pixels.clear();
    pending_paint.dirty_rects.clear();
    pending_paint.width = 0;
    pending_paint.height = 0;
    pending_paint.ready = false;
    pending_paint.tick = 0;
}

void BrowserManager::RestoreBrowserTextures()
{
    for (auto& [id, instance] : browsers_)
    {
        if (!instance || !instance->visible)
            continue;

        auto it = pending_.find(id);
        if (it == pending_.end())
            continue;

        auto& pending_paint = it->second;
        std::lock_guard<std::mutex> lock(pending_paint.mutex);

        if (pending_paint.pixels.empty() || pending_paint.width <= 0 || pending_paint.height <= 0)
            continue;

        if (instance->mode == RenderMode::WorldObject3D)
        {
            auto world_renderer = worldRenderers_.find(id);
            if (world_renderer != worldRenderers_.end() && world_renderer->second)
            {
                world_renderer->second->OnPaint(
                    pending_paint.pixels.data(),
                    pending_paint.width,
                    pending_paint.height
                );
            }
        }
        else
        {
            instance->view.OnPaint(pending_paint.pixels.data(), pending_paint.width, pending_paint.height);
        }

        pending_paint.ready = false;
        pending_paint.dirty_rects.clear();
    }
}

void BrowserManager::RequestVisibleBrowsersRepaint()
{
    if (!CefCurrentlyOn(TID_UI))
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::RequestVisibleBrowsersRepaint, base::Unretained(this)));
        return;
    }

    for (auto& [id, instance] : browsers_)
    {
        if (!instance || !instance->visible || !instance->browser || !instance->browser->IsValid())
            continue;

        auto host = instance->browser->GetHost();
        if (!host)
            continue;

        host->WasHidden(false);
        host->WasResized();
        host->Invalidate(PET_VIEW);
        host->SendExternalBeginFrame();
    }
}

void BrowserManager::RequestTextureClear(int id)
{
    if (auto* instance = GetBrowserInstance(id))
        instance->clear_texture.store(true, std::memory_order_release);
}

void BrowserManager::OnPaint(int id, const void* buffer, int width, int height, const cef_rect_t* dirtyRects, size_t dirtyRectCount)
{
    if (isCefUpdatesPaused_ || !buffer || width <= 0 || height <= 0)
        return;

    auto* instance = GetBrowserInstance(id);
    if (!instance)
        return;

    if (!instance->visible)
    {
        ClearPendingPaint(id);
        return;
    }

    auto& pending_paint = pending_[id];
    {
        std::lock_guard<std::mutex> lock(pending_paint.mutex);
        const size_t buffer_size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
        const bool use_dirty_rects = instance->mode == RenderMode::Overlay2D && dirtyRects && dirtyRectCount > 0;
        const bool size_changed = pending_paint.width != width || pending_paint.height != height || pending_paint.pixels.size() != buffer_size;

        pending_paint.width = width;
        pending_paint.height = height;

        if (size_changed || !use_dirty_rects)
        {
            pending_paint.pixels.resize(buffer_size);
            std::memcpy(pending_paint.pixels.data(), buffer, buffer_size);
            pending_paint.dirty_rects.clear();
        }
        else
        {
            const auto* source = static_cast<const uint8_t*>(buffer);
            auto* target = pending_paint.pixels.data();
            const bool full_update_pending = pending_paint.ready && pending_paint.dirty_rects.empty();

            pending_paint.dirty_rects.reserve(pending_paint.dirty_rects.size() + dirtyRectCount);

            for (size_t i = 0; i < dirtyRectCount; ++i)
            {
                const auto& rect = dirtyRects[i];
                const int left = std::max(0, rect.x);
                const int top = std::max(0, rect.y);
                const int right = std::min(width, rect.x + rect.width);
                const int bottom = std::min(height, rect.y + rect.height);

                if (left >= right || top >= bottom)
                    continue;

                const int rect_width = right - left;
                const int rect_height = bottom - top;

                for (int row = 0; row < rect_height; ++row)
                {
                    const size_t offset = (static_cast<size_t>(top + row) * static_cast<size_t>(width) + static_cast<size_t>(left)) * 4;
                    std::memcpy(target + offset, source + offset, static_cast<size_t>(rect_width) * 4);
                }

                if (!full_update_pending)
                    pending_paint.dirty_rects.push_back(cef_rect_t{ left, top, rect_width, rect_height });
            }

            if (!full_update_pending && pending_paint.dirty_rects.empty())
                return;
        }

        pending_paint.ready = true;
        pending_paint.tick = ::GetTickCount64();
    }
}

void BrowserManager::SendExternalBeginFrames()
{
    if (!CefCurrentlyOn(TID_UI))
    {
        bool expected = false;
        if (!begin_frame_task_pending_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
            return;

        if (!CefPostTask(TID_UI, base::BindOnce(&BrowserManager::DispatchExternalBeginFramesOnUi, base::Unretained(this))))
            begin_frame_task_pending_.store(false, std::memory_order_release);
        return;
    }

    DispatchExternalBeginFramesOnUi();
}

void BrowserManager::DispatchExternalBeginFramesOnUi()
{
    CEF_REQUIRE_UI_THREAD();

    for (auto& [id, inst] : browsers_)
    {
        if (!inst || !inst->visible || !inst->browser || !inst->browser->IsValid())
            continue;

        if (auto host = inst->browser->GetHost())
            host->SendExternalBeginFrame();
    }

    begin_frame_task_pending_.store(false, std::memory_order_release);
}

bool BrowserManager::RenderAll()
{
    UpdateNativeUiInput();

    if (ShouldSkipBrowserRendering())
        return false;

    UpdateAudioSpatialization();
    SendExternalBeginFrames();

    for (auto& [id, inst] : browsers_)
    {
        if (!inst)
            continue;

        if (inst->clear_texture.exchange(false, std::memory_order_acq_rel))
        {
            ClearPendingPaint(id);

            if (inst->mode == RenderMode::WorldObject3D)
            {
                auto world_renderer = worldRenderers_.find(id);
                if (world_renderer != worldRenderers_.end() && world_renderer->second)
                    world_renderer->second->Clear();
            }
            else
            {
                inst->view.Clear();
            }
        }

        if (!inst->visible)
        {
            ClearPendingPaint(id);
            continue;
        }

        auto it = pending_.find(id);
        if (it == pending_.end())
            continue;

        auto& pending_paint = it->second;
        std::lock_guard<std::mutex> lock(pending_paint.mutex);

        if (!pending_paint.ready || pending_paint.pixels.empty())
            continue;

        if (pending_paint.width <= 0 || pending_paint.height <= 0)
        {
            pending_paint.ready = false;
            continue;
        }

        cef_rect_t rect{ 0, 0, pending_paint.width, pending_paint.height };
        const cef_rect_t* dirty_rects = pending_paint.dirty_rects.empty() ? &rect : pending_paint.dirty_rects.data();
        const size_t dirty_rect_count = pending_paint.dirty_rects.empty() ? 1 : pending_paint.dirty_rects.size();

        if (inst->mode == RenderMode::WorldObject3D)
        {
            auto world_renderer = worldRenderers_.find(id);
            if (world_renderer != worldRenderers_.end() && world_renderer->second)
            {
                world_renderer->second->OnPaint(
                    pending_paint.pixels.data(),
                    pending_paint.width,
                    pending_paint.height
                );
            }
        }
        else
        {
            inst->view.UpdateTexture(pending_paint.pixels.data(), dirty_rects, dirty_rect_count);
        }

        pending_paint.ready = false;
        pending_paint.dirty_rects.clear();
    }

    bool any_visible = false;

    for (auto& [id, browser] : browsers_)
    {
        if (!browser)
            continue;

        if (!browser->visible)
            continue;

        // Overlay2D
        if (browser->mode == RenderMode::Overlay2D)
        {
            browser->view.Draw();
            any_visible = true;
            continue;
        }

        // World2D
        if (browser->mode == RenderMode::World2D)
        {
            // Project world position to screen position (GTA native projection)
            RwV3d worldPos{
                browser->world2d.x,
                browser->world2d.y,
                browser->world2d.z + browser->world2d.offsetZ
            };

            RwV3d out{};
            float w = 0.f, h = 0.f;

            const bool ok = CSprite::CalcScreenCoors(worldPos, &out, &w, &h, true, true);
            if (!ok || out.z <= 0.0f)
                continue;

            const auto r = browser->view.rect();
            const int viewW = r.width;
            const int viewH = r.height;

            const int x = static_cast<int>(out.x - static_cast<float>(viewW) * browser->world2d.pivotX);
            const int y = static_cast<int>(out.y - static_cast<float>(viewH) * browser->world2d.pivotY);

            browser->view.SetPosition(x, y);
            browser->view.Draw();
            any_visible = true;
            continue;
        }
    }

    return any_visible;
}

void BrowserManager::OnBeforeEntityRender(CEntity* entity)
{
    if (ShouldSkipBrowserRendering())
        return;

    auto it = entityToBrowserId_.find(entity);
    if (it == entityToBrowserId_.end())
        return;

    const int browserId = it->second;

    auto* browser = GetBrowserInstance(browserId);
    if (!browser || !browser->visible)
        return;

    auto wrIt = worldRenderers_.find(browserId);
    if (wrIt == worldRenderers_.end() || !wrIt->second)
        return;

    wrIt->second->SwapTexture(entity);
}

void BrowserManager::OnAfterEntityRender(CEntity* entity)
{
    auto it = entityToBrowserId_.find(entity);
    if (it == entityToBrowserId_.end())
        return;
    int browserId = it->second;
    auto wrIt = worldRenderers_.find(browserId);
    if (wrIt != worldRenderers_.end())
    {
        wrIt->second->RestoreTexture();
    }
}

BrowserInstance* BrowserManager::GetBrowserInstance(int id)
{
    auto it = browsers_.find(id);
    return it != browsers_.end() ? it->second.get() : nullptr;
}

bool BrowserManager::IsAnyBrowserVisible() const
{
    if (ShouldSkipBrowserRendering())
        return false;

    for (const auto& [id, browser] : browsers_)
    {
        if (browser && browser->visible)
            return true;
    }
    return false;
}

bool BrowserManager::IsAnyBrowserFocused() const
{
    for (const auto& [id, instance] : browsers_)
    {
        if (instance && instance->view.IsFocused())
            return true;
    }
    return false;
}

void BrowserManager::FocusBrowser(int browserId, bool focus)
{
    if (CefCurrentlyOn(TID_UI) == false)
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::FocusBrowser, base::Unretained(this), browserId, focus));
        return;
    }

    auto* instance_to_change = GetBrowserInstance(browserId);
    if (!instance_to_change)
    {
        LOG_WARN("[CEF] FocusBrowser: Could not find browser with ID {}.", browserId);
        return;
    }

    if (focus)
    {
        if (focusedBrowserId_ == browserId)
            return;
        if (focusedBrowserId_ != -1)
        {
            if (auto* old = GetBrowserInstance(focusedBrowserId_))
            {
                old->view.SetFocused(false);
                if (old->browser && old->browser->GetHost())
                    old->browser->GetHost()->SetFocus(false);
            }
        }
        focusedBrowserId_ = browserId;
        instance_to_change->view.SetFocused(true);

        if (instance_to_change->browser && instance_to_change->browser->GetHost()) {
            instance_to_change->browser->GetHost()->SetFocus(true);
        }
            
        LOG_DEBUG("[CEF] Browser {} gained focus.", browserId);
    }
    else
    {
        if (focusedBrowserId_ != browserId)
            return;

        focusedBrowserId_ = -1;
        instance_to_change->view.SetFocused(false);

        if (instance_to_change->browser)
        {
            if (auto host = instance_to_change->browser->GetHost())
            {
                host->SetFocus(false);
                host->SendCaptureLostEvent();
            }
        }

        LOG_DEBUG("[CEF] Browser {} lost focus.", browserId);
    }
}

BrowserInstance* BrowserManager::GetFocusedBrowser()
{
    return focusedBrowserId_ == -1 ? nullptr : GetBrowserInstance(focusedBrowserId_);
}

void BrowserManager::UpdateAudioSpatialization()
{
    // Update listener from player camera
    if (CPlayerPed* player = FindPlayerPed(-1))
    {
        if (player->m_matrix)
        {
            const auto& pos = player->m_matrix->pos;
            const auto& at = player->m_matrix->at;
            const auto& up = player->m_matrix->up;
            audio_.UpdateListenerPosition(pos.x, pos.y, pos.z, at.x, at.y, at.z, up.x, up.y, up.z);
        }
    }

    // Update 3D positions for world browsers bound to entities
    for (const auto& [entity, browserId] : entityToBrowserId_)
    {
        if (entity && entity->m_matrix)
        {
            const auto& pos = entity->m_matrix->pos;
            audio_.UpdateSourcePosition(browserId, pos.x, pos.y, pos.z);
        }
    }
}

void BrowserManager::SetKeyCaptureEnabled(bool enabled)
{
    key_capture_enabled_ = enabled;

    LOG_INFO("[CEF] KeyCapture {}", enabled ? "enabled" : "disabled");
}

void BrowserManager::EnableKey(int key, bool enabled)
{
    if (key < 0 || key > 255)
        return;

    key_allowed_.set(static_cast<size_t>(key), enabled);

    LOG_INFO("[CEF] Key {} {}", key, enabled ? "enabled" : "disabled");
}

void BrowserManager::OnGameFocusGained()
{
    RestoreBrowserTextures();
    RequestVisibleBrowsersRepaint();
}

void BrowserManager::OnGameFocusLost()
{
    if (!CefCurrentlyOn(TID_UI))
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::OnGameFocusLost, base::Unretained(this)));
        return;
    }

    auto* focused = GetFocusedBrowser();
    if (!focused || !focused->browser)
        return;

    if (auto host = focused->browser->GetHost())
    {
        host->SetFocus(false);
        host->SendCaptureLostEvent();
    }
}

void BrowserManager::ExitGame()
{
    HWND hwnd = gta_.GetHwnd();
    if (hwnd)
    {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;
    }

    // Fallback
    ExitProcess(0);
}

void BrowserManager::SetEscapeMenuMode(EscapeMenuMode mode)
{
    const EscapeMenuMode previous = escape_menu_.SetMode(mode);
    const EscapeMenuMode current = escape_menu_.GetMode();

    DispatchNativeUiEvents();

    if (previous != current)
    {
        LOG_INFO("[CEF] Escape menu mode changed: {} -> {}",
            EscapeMenuController::GetModeName(previous),
            EscapeMenuController::GetModeName(current));
    }
}

bool BrowserManager::ShouldSkipBrowserRendering() const
{
    return !draw_enabled_ || escape_menu_.IsNativePauseMenuVisible();
}

bool BrowserManager::IsFocusedTextInputActive() const
{
    if (!focus_ || focusedBrowserId_ < 0)
        return false;

    const auto browser = browsers_.find(focusedBrowserId_);
    return browser != browsers_.end()
        && browser->second
        && focus_->IsTextInputFocused(browser->second->id);
}

void BrowserManager::UpdateNativeUiInput()
{
    bool changed = false;

    changed = escape_menu_.UpdateInput() || changed;

    if (changed)
        DispatchNativeUiEvents();
}

void BrowserManager::DispatchNativeUiEvents()
{
    if (escape_menu_.HasPendingCustomMenuVisibilityChange())
        EmitCustomEscapeMenuVisibility();

    if (player_list_.HasPendingCustomPlayerListVisibilityChange())
        EmitCustomPlayerListVisibility();
}

void BrowserManager::EmitCustomEscapeMenuVisibility()
{
    if (!CefCurrentlyOn(TID_UI))
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::EmitCustomEscapeMenuVisibility, base::Unretained(this)));
        return;
    }

    const bool visible = escape_menu_.IsCustomMenuOpen();

    for (const auto& [id, instance] : browsers_)
    {
        if (!instance || !instance->browser || !instance->browser->IsValid())
            continue;

        CefRefPtr<CefFrame> frame = instance->browser->GetMainFrame();
        if (!frame || !frame->IsValid())
            continue;

        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("emit_event");
        CefRefPtr<CefListValue> list = msg->GetArgumentList();
        list->SetString(0, "cef:escape_menu");
        list->SetBool(1, visible);

        frame->SendProcessMessage(PID_RENDERER, msg);
    }
}

void BrowserManager::EmitCustomPlayerListVisibility()
{
    if (!CefCurrentlyOn(TID_UI))
    {
        CefPostTask(TID_UI, base::BindOnce(&BrowserManager::EmitCustomPlayerListVisibility, base::Unretained(this)));
        return;
    }

    const bool visible = player_list_.IsCustomPlayerListOpen();

    for (const auto& [id, instance] : browsers_)
    {
        if (!instance || !instance->browser || !instance->browser->IsValid())
            continue;

        CefRefPtr<CefFrame> frame = instance->browser->GetMainFrame();
        if (!frame || !frame->IsValid())
            continue;

        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("emit_event");
        CefRefPtr<CefListValue> list = msg->GetArgumentList();
        list->SetString(0, "cef:player_list");
        list->SetBool(1, visible);

        frame->SendProcessMessage(PID_RENDERER, msg);
    }
}

void BrowserManager::SetPlayerListMode(PlayerListMode mode)
{
    const PlayerListMode previous = player_list_.SetMode(mode);
    const PlayerListMode current = player_list_.GetMode();

    DispatchNativeUiEvents();

    if (previous != current)
    {
        LOG_INFO("[CEF] Player list mode changed: {} -> {}",
            PlayerListController::GetModeName(previous),
            PlayerListController::GetModeName(current));
    }
}


bool BrowserManager::ShouldSuppressNativePlayerList() const
{
    return PlayerListController::ShouldSuppressNativePlayerList(player_list_.GetMode());
}

bool BrowserManager::HandleNativePlayerListOpenRequest()
{
    const bool should_suppress = ShouldSuppressNativePlayerList();
    if (!should_suppress)
        return false;

    const bool changed = player_list_.HandleNativePlayerListOpenRequest(!IsFocusedTextInputActive());
    if (changed)
        DispatchNativeUiEvents();

    return true;
}

void BrowserManager::OnDeviceLost()
{
    // Stop CEF rendering during device reset
    // This prevents CEF from trying to update textures while they're invalid
    isCefUpdatesPaused_ = true;
    screen_capture_.OnDeviceLost();
    
    // Release all browser View resources (2D overlays)
    for (auto& [id, instance] : browsers_) 
    {
        if (instance) 
        {
            LOG_DEBUG("[BrowserManager] Releasing browser {} View resources", id);
            instance->view.OnDeviceLost();
        }
    }
    
    // Release all WorldRenderer resources (3D world browsers)
    for (auto& [browserId, renderer] : worldRenderers_) 
    {
        if (renderer)
        {
            LOG_DEBUG("[BrowserManager] Releasing WorldRenderer for browser {} resources", browserId);
            renderer->OnDeviceLost();
        }
    }
}

void BrowserManager::OnDeviceReset(IDirect3DDevice9* device)
{
    // Recreate all browser View resources (2D overlays)
    for (auto& [id, instance] : browsers_) 
    {
        if (instance) 
        {
            LOG_DEBUG("[BrowserManager] Recreating browser {} View resources", id);
            instance->view.OnDeviceReset(device);
        }
    }
    
    // Recreate all WorldRenderer resources (3D world browsers)
    for (auto& [browserId, renderer] : worldRenderers_) 
    {
        if (renderer) 
        {
            LOG_DEBUG("[BrowserManager] Recreating WorldRenderer for browser {} resources", browserId);
            renderer->OnDeviceReset(device);
        }
    }
    
    // Resume CEF updates and restore the last known browser pixels immediately.
    // Static pages may not produce another OnPaint after Alt+Tab/device reset,
    // especially when another client plugin changes the D3D reset timing (like Samp Addons).
    isCefUpdatesPaused_ = false;
    RestoreBrowserTextures();
    RequestVisibleBrowsersRepaint();
}

LRESULT BrowserManager::OnWndProcMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    bool native_ui_message_consumed = false;

    native_ui_message_consumed = escape_menu_.ConsumeWndProcMessage(msg, wParam, lParam) || native_ui_message_consumed;

    if (!IsFocusedTextInputActive())
        native_ui_message_consumed = player_list_.ConsumeWndProcMessage(msg, wParam, lParam) || native_ui_message_consumed;

    if (native_ui_message_consumed)
    {
        DispatchNativeUiEvents();
        return true;
    }

    if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP)
    {
        if (key_capture_enabled_ && network_.GetState() == ConnectionState::CONNECTED && !network_.IsNonCefServer())
        {
            const int vk = static_cast<int>(wParam) & 0xFF;
            const bool down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            const bool repeat = down ? (((static_cast<uint32_t>(lParam) >> 30) & 1u) != 0u) : false;

            if (vk >= 0 && vk <= 255 && key_allowed_.test(static_cast<size_t>(vk)))
            {
                // Ignore auto-repeat keydown to avoid spamming packets (send first down + up only).
                if (!(down && repeat))
                {
                    ClientEmitEventPacket event;
                    event.name = CefEvent::Client::PressKey;
                    event.args.emplace_back(vk);
                    event.args.emplace_back((int)((static_cast<uint32_t>(lParam) >> 16) & 0xFFu));
                    event.args.emplace_back((int)GetCefEventFlags());
                    event.args.emplace_back(down);
                    event.args.emplace_back(repeat);

                    network_.SendPacket(PacketType::ClientEmitEvent, event);
                }
            }
        }
    }

    auto* focused_inst = GetFocusedBrowser();
    if (!focused_inst || !focused_inst->browser)
        return false;

    auto host = focused_inst->browser->GetHost();
    if (!host)
        return false;

    switch (msg)
    {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEWHEEL:
        {
            CefMouseEvent evt;
            evt.x = GET_X_LPARAM(lParam);
            evt.y = GET_Y_LPARAM(lParam);
            evt.modifiers = GetCefEventFlags();

            if (msg == WM_MOUSEMOVE)
                host->SendMouseMoveEvent(evt, false);
            else if (msg == WM_LBUTTONDOWN)
                host->SendMouseClickEvent(evt, MBT_LEFT, false, 1);
            else if (msg == WM_LBUTTONUP)
                host->SendMouseClickEvent(evt, MBT_LEFT, true, 1);
            else if (msg == WM_RBUTTONDOWN)
                host->SendMouseClickEvent(evt, MBT_RIGHT, false, 1);
            else if (msg == WM_RBUTTONUP)
                host->SendMouseClickEvent(evt, MBT_RIGHT, true, 1);
            else if (msg == WM_MOUSEWHEEL)
                host->SendMouseWheelEvent(evt, 0, GET_WHEEL_DELTA_WPARAM(wParam));

            return true;
        }
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        {
            const bool textInputFocused = (focus_ != nullptr) && focus_->IsTextInputFocused(focused_inst->id);
            if (!textInputFocused)
            {
                // T chat, TAB scoreboard, etc..
                return false;
            }

            CefKeyEvent evt;
            evt.windows_key_code = static_cast<int>(wParam);
            evt.native_key_code = static_cast<int>(lParam);
            evt.modifiers = GetCefEventFlags();

            if (msg == WM_CHAR && (GetKeyState(VK_RMENU) & 0x8000))
                evt.modifiers &= ~(EVENTFLAG_CONTROL_DOWN | EVENTFLAG_ALT_DOWN);

            if (msg == WM_KEYDOWN)
                evt.type = KEYEVENT_RAWKEYDOWN;
            else if (msg == WM_KEYUP)
                evt.type = KEYEVENT_KEYUP;
            else
                evt.type = KEYEVENT_CHAR;

            host->SendKeyEvent(evt);

            if (msg != WM_KEYUP)
            {
                if (wParam == 'T' || wParam == 'F' || wParam == VK_RETURN || wParam == VK_ESCAPE ||
                    (wParam >= 'A' && wParam <= 'Z') || (wParam >= VK_LEFT && wParam <= VK_DOWN))
                {
                    return true;
                }
            }

            return true;
        }
    }

    return false;
}

void BrowserManager::SetPlayerStatsPolling(int browserId, bool enabled, int intervalMs)
{
    if (intervalMs <= 0)
        intervalMs = 50;

    auto& state = player_stats_poll_[browserId];
    state.enabled = enabled;
    state.intervalMs = static_cast<uint32_t>(intervalMs);
    state.nextTickMs = 0;
    state.lastEmitMs = 0;
    state.hasLast = false;

    LOG_DEBUG("[CEF] Browser {} playerStats polling {} ({}ms).", browserId, enabled ? "enabled" : "disabled", intervalMs);
}

void BrowserManager::TickGameData()
{
    if (!initialized_ || player_stats_poll_.empty())
        return;

    const uint64_t now = ::GetTickCount64();

    PlayerStatsSnapshot current{};
    bool havePlayer = false;

    CPlayerPed* ped = FindPlayerPed(-1);
    if (!ped)
        return;

    // Only proceed when ped is fully constructed (RwObject present).
    if (!ped->m_pRwObject)
        return;

    havePlayer = true;

    current.hp = static_cast<int>(ped->m_fHealth);
    current.max_hp = static_cast<int>(ped->m_fMaxHealth);
    current.arm = static_cast<int>(ped->m_fArmour);

    if (ped->m_pPlayerData)
        current.breath = static_cast<int>(ped->m_pPlayerData->m_fBreath);

    current.wanted = ped->GetWantedLevel();

    // Weapon + ammo
    const int slot = static_cast<int>(ped->m_nActiveWeaponSlot);
    if (slot >= 0 && slot < 13)
    {
        const CWeapon& w = ped->m_aWeapons[slot];
        current.weapon = static_cast<int>(w.m_nType);
        current.ammo = static_cast<int>(w.m_nAmmoInClip);
        current.max_ammo = static_cast<int>(w.m_nTotalAmmo);
    }

    // Money
    if (CPlayerInfo* pi = ped->GetPlayerInfoForThisPlayerPed())
        current.money = pi->m_nMoney;

    // Position
    {
        const CVector& p = ped->GetPosition();
        current.x = p.x;
        current.y = p.y;
        current.z = p.z;
    }

    // In vehicle + vehicle info
    CVehicle* veh = ped->m_pVehicle;
    current.in_vehicle = (veh != nullptr);
    if (veh)
    {
        current.vehicle_health = veh->m_fHealth;
        current.vehicle_model = static_cast<int>(veh->m_nModelIndex);
    }
    else
    {
        current.vehicle_health = 0.f;
        current.vehicle_model = 0;
    }

    // Speed
    {
        const CVector& v = (veh ? veh->m_vecMoveSpeed : ped->m_vecMoveSpeed);
        current.speed = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z) * 100.0f;
    }

    auto NormalizeDeg = [](float deg) -> float {
        while (deg < 0.f) deg += 360.f;
        while (deg >= 360.f) deg -= 360.f;
        return deg;
    };

    // Player heading
    {
        float rad = ped->m_fCurrentRotation;
        
        // current.heading = rad * (180.0f / 3.14159265f);
        float deg = rad * (180.0f / 3.14159265f);
        current.heading = NormalizeDeg(-deg); 

        if (current.heading < 0.f) current.heading += 360.f;
        if (current.heading >= 360.f) current.heading = std::fmod(current.heading, 360.f);
    }

    // Camera heading (deg)
    {
        const CVector& fwd = TheCamera.m_mCameraMatrix.at;

        float camRad = std::atan2(fwd.x, fwd.y);
        float camDeg = camRad * (180.0f / 3.14159265f);

        current.camera_heading = NormalizeDeg(camDeg);
    }

    // Aiming
    {
        bool aiming = false;

        if (CPad* pad = ped->GetPadFromPlayer())
            aiming = pad->GetTarget();
        else if (CPad* pad = CPad::GetPad(0))
            aiming = pad->GetTarget();

        current.aiming = aiming;
    }

    // Emit
    for (auto it = player_stats_poll_.begin(); it != player_stats_poll_.end(); )
    {
        const int browserId = it->first;
        auto& st = it->second;

        auto* inst = GetBrowserInstance(browserId);
        if (!inst)
        {
            it = player_stats_poll_.erase(it);
            continue;
        }

        if (!st.enabled || !havePlayer)
        {
            ++it;
            continue;
        }

        if (st.nextTickMs == 0 || now >= st.nextTickMs)
        {
            st.nextTickMs = now + st.intervalMs;

            const bool changed = !st.hasLast || !PlayerStats::Equal(st.last, current);
            const bool force = (st.lastEmitMs == 0) || (now - st.lastEmitMs) >= 1000ULL;

            if (changed || force)
            {
                const std::string json = PlayerStats::ToJson(current);
                PlayerStats::EmitJson(inst, "game:data:playerStats", json);

                st.lastEmitMs = now;
                st.last = current;
                st.hasLast = true;
            }
        }

        ++it;
    }
}
