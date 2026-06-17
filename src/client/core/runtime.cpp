#include "runtime.hpp"

#include <windows.h>

#include "app.hpp"
#include "browser/audio.hpp"
#include "browser/focus.hpp"
#include "browser/manager.hpp"
#include "hooks/cursor_hook.hpp"
#include "hooks/hook_manager.hpp"
#include "hooks/render_hook.hpp"
#include "hooks/wndproc.hpp"
#include "network/network_manager.hpp"
#include "rendering/render_manager.hpp"
#include "samp/addresses.hpp"
#include "samp/samp.hpp"
#include "samp/samp_version_manager.hpp"
#include "samp/hooks/netgame.hpp"
#include "samp/hooks/chat.hpp"
#include "samp/hooks/scoreboard.hpp"
#include "system/config_manager.hpp"
#include "system/gta.hpp"
#include "system/logger.hpp"
#include "system/resource_manager.hpp"
#include "ui/hud_manager.hpp"
#include "ui/download_dialog.hpp"
#include "game_sa/CMenuManager.h"

std::unique_ptr<Runtime> Runtime::CreateDefault()
{
	auto runtime = std::unique_ptr<Runtime>(new Runtime());

	runtime->logger_ = std::make_unique<Logger>();
	logging::SetLogger(runtime->logger_.get());

	runtime->config_ = std::make_unique<ConfigManager>();
	runtime->gta_ = std::make_unique<Gta>();
	runtime->hud_ = std::make_unique<HudManager>();
	runtime->audio_ = std::make_unique<AudioManager>();

	return runtime;
}

void Runtime::UpdateBrowserDrawState()
{
    if (!browser_)
        return;

    const HWND hwnd = gta_ ? gta_->GetHwnd() : nullptr;
    const bool window_active = hwnd && ::IsWindow(hwnd) && (::GetForegroundWindow() == hwnd);
    const bool pause_menu_open = FrontEndMenuManager.m_bMenuActive && !FrontEndMenuManager.m_bDontDrawFrontEnd;

    browser_->SetDrawEnabled(window_active && !pause_menu_open);
}

bool Runtime::Start()
{
	const auto user_files = std::filesystem::path(gta_->GetUserFilesPath());
	const auto user_cef_dir = user_files / "nsy";

	std::error_code error_code;
	std::filesystem::create_directories(user_cef_dir, error_code);

	const auto config_path = (user_cef_dir / "config.json").string();
	(void)config_->Load(config_path);

	const auto client_log = (user_cef_dir / "client.log").string();

	logger_->SetLogFile(client_log);
	logger_->SetDebugMode(config_->Get<bool>("debug", false));

	gta_->Initialize();
    hud_->Initialize();
    
    if (!audio_->Initialize()) {
        LOG_ERROR("AudioManager init failed. Audio disabled.");
    }
    
    resources_ = std::make_unique<ResourceManager>(*gta_);
    network_ = std::make_unique<NetworkManager>(*resources_);
    resources_->SetNetworkManager(*network_);
    
    browser_ = std::make_unique<BrowserManager>(*audio_, *gta_, *resources_, *network_);
    focus_ = std::make_unique<FocusManager>(*browser_);
    browser_->SetFocusManager(focus_.get());
    
    if (!browser_->Initialize()) {
        LOG_FATAL("CEF init failed. Disabling CEF features.");
        return true;
    }
    
    hooks_ = std::make_unique<HookManager>();
    if (!hooks_->Initialize()) {
        LOG_FATAL("HookManager init failed.");
        return true;
    }
    
    CursorHook::Instance().Initialize(*hooks_);

    gta_->SetOnHwndFound([this](HWND hwnd) {
        LOG_INFO("[Runtime] HWND callback triggered, finalizing initialization...");
        FinalizeInitialization(hwnd);
    });
    
    RenderManager::Instance().SetHookManager(hooks_.get());
    //RenderManager::Instance().SetGameWindow(gta_->GetHwnd());
    
    if (!RenderManager::Instance().Initialize()) {
        LOG_FATAL("RenderManager init failed.");
        return true;
    }

    RenderManager::Instance().OnPresent = [this]()
    {
        UpdateBrowserDrawState();

        if (gta_)
            gta_->PumpMainThreadCallbacks();

        int frames = cursor_recenter_frames_.load(std::memory_order_acquire);
        if (frames > 0)
        {
            cursor_recenter_frames_.store(frames - 1, std::memory_order_release);

            if (frames == 1)
            {
                HWND hwnd = gta_ ? gta_->GetHwnd() : nullptr;
                if (hwnd && ::IsWindow(hwnd) && ::GetForegroundWindow() == hwnd)
                {
                    RECT rect{};
                    if (::GetClientRect(hwnd, &rect))
                    {
                        POINT center{
                            rect.left + (rect.right  - rect.left) / 2,
                            rect.top + (rect.bottom - rect.top) / 2
                        };

                        ::ClientToScreen(hwnd, &center);
                        ::SetCursorPos(center.x, center.y);

                        RECT clip { 
                            center.x - 1, 
                            center.y - 1, 
                            center.x + 1, 
                            center.y + 1 
                        };

                        ::ClipCursor(&clip);
                        ::ClipCursor(nullptr);
                    }
                }
            }
        }

        if (!RenderManager::Instance().GetDevice())
            RenderManager::Instance().PollD3D();

        if (app_)
            app_->Tick();
    };

	// SA:MP version
	samp_version_ = std::make_unique<SampVersionManager>();
	samp_version_->Initialize();

	SampAddresses::Instance().Initialize(*samp_version_);

	// SA:MP hooks
	samp_ = std::make_unique<Samp>(*hooks_);
	samp_->OnLoaded = [this]()
	{
		if (!RenderManager::Instance().GetDevice())
			RenderManager::Instance().PollD3D();

		renderhook_ = std::make_unique<RenderHook>(*hooks_, *browser_);
		if (!renderhook_->Initialize())
		{
			LOG_FATAL("Failed to install RenderHook after SA-MP loaded.");
		}
	};

	samp_->OnExit = [this]()
	{
		if (renderhook_)
		{
			renderhook_->Shutdown();
			renderhook_.reset();
		}
	};

	if (!samp_->Initialize())
	{
		LOG_FATAL("Failed to initialize SA-MP hooks (game unaffected).");
		return true;
	}

	netgame_hook_ = std::make_unique<NetGameHook>(*hooks_, *resources_);
	if (!netgame_hook_->Initialize())
	{
		LOG_FATAL("Failed to initialize NetGame hooks (game unaffected).");
		return true;
	}

	network_->SetSessionActiveHandler([this](bool active) {
		if (netgame_hook_)
			netgame_hook_->SetSessionActive(active);
	});

	chat_hook_ = std::make_unique<ChatHook>(*hooks_, *focus_, *browser_, *network_);
	chat_hook_->Initialize();

	scoreboard_hook_ = std::make_unique<ScoreboardHook>(*hooks_, *browser_);
	scoreboard_hook_->Initialize();

	download_dialog_ = std::make_unique<DownloadDialog>(network_.get(), hud_.get(), samp_.get(), browser_.get());
	resources_->SetDownloadDialog(*download_dialog_);

	app_ = std::make_unique<App>(*network_, *browser_, *audio_, *focus_, *resources_, *hud_);
	app_->Initialize();

	return true;
}

void Runtime::FinalizeInitialization(HWND hwnd)
{
    if (init_finalized_.exchange(true, std::memory_order_acq_rel))
    {
        LOG_DEBUG("[Runtime] FinalizeInitialization already done, skipping.");
        return;
    }

    LOG_INFO("[Runtime] Finalizing initialization with HWND={}...", static_cast<void*>(hwnd));

    if (!hwnd || !::IsWindow(hwnd))
    {
        LOG_ERROR("[Runtime] FinalizeInitialization called with invalid HWND!");
        init_finalized_.store(false, std::memory_order_release);
        return;
    }

    // WndProc
    if (!wndproc_)
    {
        wndproc_ = std::make_unique<WndProcHook>(hwnd);
        if (!wndproc_->Initialize())
        {
            LOG_ERROR("[Runtime] WndProc hook failed with HWND={}", static_cast<void*>(hwnd));
            init_finalized_.store(false, std::memory_order_release); 
            wndproc_.reset();
            return;
        }

        wndproc_->OnMessage = [this](HWND h, UINT msg, WPARAM wParam, LPARAM lParam) -> std::optional<LRESULT>
        {
            if (browser_ && browser_->OnWndProcMessage(h, msg, wParam, lParam))
                return { TRUE };

            if (msg == WM_ACTIVATE)
            {
                const bool active = (LOWORD(wParam) != WA_INACTIVE);

                if (browser_)
                    browser_->SetDrawEnabled(active);

                if (active)
                {
                    CursorHook::Instance().OnGameActivated();

                    if (focus_)
                        focus_->RequestResync();

                    cursor_recenter_frames_.store(5, std::memory_order_release);
                }
                else
                {
                    cursor_recenter_frames_.store(0, std::memory_order_release);
                    ::ClipCursor(nullptr);

                    if (browser_)
                        browser_->OnGameFocusLost();
                }

                return std::nullopt;
            }

            if (msg == WM_ACTIVATEAPP)
            {
                if (!wParam)
                    ::ClipCursor(nullptr);

                return std::nullopt;
            }

            if (msg == WM_SETCURSOR)
            {
                if (browser_ && browser_->IsAnyBrowserFocused() && ::GetForegroundWindow() == h)
                {
                    ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
                    return { TRUE };
                }

                return std::nullopt;
            }

            return std::nullopt;
        };

        LOG_INFO("[Runtime] WndProc hook installed successfully.");
    }

    RenderManager::Instance().SetGameWindow(hwnd);
    LOG_INFO("[Runtime] Initialization finalized.");
}

void Runtime::Stop()
{
	if (app_)
		app_->Shutdown();

	if (download_dialog_)
		download_dialog_.reset();

	if (scoreboard_hook_)
	{
		scoreboard_hook_->Shutdown();
		scoreboard_hook_.reset();
	}

	if (chat_hook_)
	{
		chat_hook_->Shutdown();
		chat_hook_.reset();
	}

	if (netgame_hook_)
	{
		netgame_hook_->Shutdown();
		netgame_hook_.reset();
	}

	if (renderhook_)
	{
		renderhook_->Shutdown();
		renderhook_.reset();
	}

	if (wndproc_)
	{
		wndproc_->Shutdown();
		wndproc_.reset();
	}

	CursorHook::Instance().Shutdown(*hooks_);
	RenderManager::Instance().Shutdown();

	if (hooks_)
	{
		hooks_->Shutdown();
		hooks_.reset();
	}
}

Runtime::~Runtime()
{
	Stop();
}