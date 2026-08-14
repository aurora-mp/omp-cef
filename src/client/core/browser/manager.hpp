#pragma once

#include <atomic>
#include <bitset>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "client.hpp"
#include "escape_menu.hpp"
#include "player_list.hpp"
#include "player_stats.hpp"
#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "rendering/screen_capture.hpp"
#include "rendering/view.hpp"
#include "rendering/world_renderer.hpp"
#include "shared/packet.hpp"

class AudioManager;
class FocusManager;
class ResourceManager;
class NetworkManager;
class Gta;
class CEntity;

// Status of a browser creation
enum class BrowserCreateStatus : int
{
    Success = 0,
    Error_Generic = 1,
    Error_IdAlreadyInUse = 2
};

// Defines how a browser is rendered in the game
enum class RenderMode
{
    Overlay2D,
    WorldObject3D,
    World2D
};

//
struct World2DBrowserData
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    // Additional vertical offset applied to z (useful for tooltips above actors).
    float offsetZ = 1.f;

    // Anchor in [0..1] applied to the view size.
    float pivotX = 0.5f;
    float pivotY = 1.0f;
};

// Holds all data and state related to a single browser instance
struct BrowserInstance
{
    int id;
    std::string url;
    CefRefPtr<BrowserClient> client;
    CefRefPtr<CefBrowser> browser;
    View view;

    RenderMode mode = RenderMode::Overlay2D;
    std::string textureName; // Used only for WorldObject3D mode

    World2DBrowserData world2d;

    bool visible = true;
    bool controls_chat_input = true;
    bool closing = false;

    std::atomic<bool> clear_texture{ false };

    bool devtools_requested = false;
    bool devtools_open = false;
    CefRefPtr<CefClient> devtools_client;
    CefRefPtr<CefBrowser> devtools_browser;

    explicit BrowserInstance(int id) : id(id), view(id) {}
};

struct PendingPaint
{
    std::mutex mutex;
    std::vector<uint8_t> pixels;
    std::vector<cef_rect_t> dirty_rects;
    int width = 0;
    int height = 0;
    bool ready = false;
    uint64_t tick = 0;
};

class BrowserManager
{
public:
    using PlayerStatsSnapshot = PlayerStats::Snapshot;
    using PlayerStatsPollState = PlayerStats::PollState;

    BrowserManager(AudioManager& audio, Gta& gta, ResourceManager& resource_manager, NetworkManager& network) : audio_(audio), gta_(gta), resource_manager_(resource_manager), network_(network) {}
    ~BrowserManager()
    {
        Shutdown();
    }

    BrowserManager(const BrowserManager&) = delete;
    BrowserManager& operator=(const BrowserManager&) = delete;
    BrowserManager(BrowserManager&&) = delete;
    BrowserManager& operator=(BrowserManager&&) = delete;

    void SetFocusManager(FocusManager* focus)
    {
        focus_ = focus;
    }

    void SetEntityResolver(std::function<CEntity*(int)> resolver)
    {
        entity_resolver_ = std::move(resolver);
    }

    bool Initialize();
    void Shutdown();

    // Browser management
    void CreateBrowser(int id, const std::string& url, bool focused, bool controls_chat, float width, float height);
    void CreateWorldBrowser(int id, const std::string& url, const std::string& textureName, float width, float height);
    void CreateWorld2DBrowser(int id, const std::string& url, float worldX, float worldY, float worldZ, float width, float height, float offsetZ, float pivotX, float pivotY);
    void SetWorld2DBrowserPos(int id, float worldX, float worldY, float worldZ);
    void SetBrowserVisible(int id, bool visible);
    void DestroyBrowser(int id);
    void DestroyAllBrowsers();
    void ReloadBrowser(int id, bool ignoreCache);
    void LoadUrl(int id, const std::string& url);
    void SetDevToolsEnabled(int browserId, bool enabled);

    // 3D World interaction
    void AttachBrowserToObject(int browserId, int objectId);
    void DetachBrowserFromObject(int browserId, int objectId);
    void OnBeforeEntityRender(CEntity* entity);
    void OnAfterEntityRender(CEntity* entity);
    void UpdateAudioSpatialization();

    // Keyboard capture / filtering (client -> server)
    void SetKeyCaptureEnabled(bool enabled);
    void EnableKey(int key, bool enabled);

    void SetPlayerStatsPolling(int browserId, bool enabled, int intervalMs);
    void TickGameData();
    void OnGameFocusGained();
    void OnGameFocusLost();

    void StartScreenCapture(int browserId, int width, int height, int fps);
    void StopScreenCapture(int browserId);
    void CaptureScreen();

    void ExitGame();

    // Native GTA SA ESC/pause menu handling
    void SetEscapeMenuMode(EscapeMenuMode mode);

    // Native SA:MP/open.mp TAB player list / scoreboard handling
    void SetPlayerListMode(PlayerListMode mode);
    bool ShouldSuppressNativePlayerList() const;
    bool HandleNativePlayerListOpenRequest();

    // Callbacks from BrowserClient
    void OnBrowserCreated(int id, CefRefPtr<CefBrowser> browser);
    void OnBrowserClosed(int id);
    void OnPaint(int id, const void* buffer, int w, int h, const cef_rect_t* dirtyRects, size_t dirtyRectCount);

    void RequestTextureClear(int id);

    bool RenderAll();
    LRESULT OnWndProcMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    BrowserInstance* GetBrowserInstance(int id);
    BrowserInstance* GetFocusedBrowser();
    const std::unordered_map<int, std::unique_ptr<BrowserInstance>>& GetAllBrowsers() const
    {
        return browsers_;
    }
    bool IsAnyBrowserVisible() const;
    bool IsAnyBrowserFocused() const;
    cef_cursor_type_t GetCursorType() const
    {
        return cursor_type_;
    }

    void FocusBrowser(int id, bool focus);
    void SetDrawEnabled(bool enabled)
    {
        draw_enabled_ = enabled;
    }
    void SetCursorType(cef_cursor_type_t type)
    {
        cursor_type_ = type;
    }

    void OnDeviceLost();
    void OnDeviceReset(IDirect3DDevice9* device);

private:
    void CreateBrowserInternal(int id, const std::string& url, bool focused, bool controls_chat, float width, float height);
    void CreateWorldBrowserInternal(int id, const std::string& url, std::string textureName, float width, float height);
    void CreateWorld2DBrowserInternal(int id, const std::string& url, float worldX, float worldY, float worldZ, float width, float height, float offsetZ, float pivotX, float pivotY);

    bool ShouldSkipBrowserRendering() const;
    bool IsFocusedTextInputActive() const;
    void UpdateNativeUiInput();
    void DispatchNativeUiEvents();
    void EmitCustomEscapeMenuVisibility();
    void EmitCustomPlayerListVisibility();

    CEntity* GetEntityFromObjectId(int objectId);
    void ClearPendingPaint(int id);
    void RestoreBrowserTextures();
    void RequestVisibleBrowsersRepaint();
    void SendExternalBeginFrames();
    void DispatchExternalBeginFramesOnUi();
    void DispatchScreenFrameOnUi(std::shared_ptr<CapturedScreenFrame> frame);

private:
    bool initialized_ = false;
    DWORD uiThreadId_ = 0;
    std::atomic<bool> is_shutting_down_{false};
    std::atomic<bool> isCefUpdatesPaused_{ false };

    // The single source for which browser has focus. -1 means none.
    int focusedBrowserId_ = -1;

    std::unordered_map<int, std::unique_ptr<BrowserInstance>> browsers_;
    std::unordered_map<int, std::unique_ptr<WorldRenderer>> worldRenderers_;
    std::unordered_map<CEntity*, int> entityToBrowserId_;

    bool draw_enabled_ = true;
    cef_cursor_type_t cursor_type_ = CT_POINTER;

    AudioManager& audio_;
    Gta& gta_;
    ResourceManager& resource_manager_;
    NetworkManager& network_;
    FocusManager* focus_ = nullptr;

    std::function<CEntity*(int)> entity_resolver_{};

    std::unordered_map<int, PendingPaint> pending_;
    std::atomic<bool> begin_frame_task_pending_{false};

    // Keyboard capture / filtering (client -> server)
    bool key_capture_enabled_ = false;
    std::bitset<256> key_allowed_{};

    std::unordered_map<int, PlayerStatsPollState> player_stats_poll_;
    ScreenCaptureManager screen_capture_;

    // Native GTA SA ESC/pause menu handling
    EscapeMenuController escape_menu_;

    // Native SA-MP/open.mp TAB player list / scoreboard handling
    PlayerListController player_list_;
};
