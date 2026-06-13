#pragma once

#include "shared/packet.hpp"
#include "network/network_manager.hpp"

#include <string>
#include <vector>

class BrowserManager;
class AudioManager;
class FocusManager;
class ResourceManager;
class HudManager;

class App {
public:
    struct PendingCreate
    {
        enum class Kind { Overlay, World, World2D };
        Kind kind = Kind::Overlay;
        int id = -1;
        std::string url;

        bool focused = false;
        bool controls_chat = false;
        float width = -1.f;
        float height = -1.f;

        std::string textureName;

        float worldX = 0.f;
        float worldY = 0.f;
        float worldZ = 0.f;
        float offsetZ = 1.f;
        float pivotX = 0.5f;
        float pivotY = 1.0f;
    };

    struct PendingEmit
    {
        int browserId = -1;
        std::string eventName;
        std::vector<Argument> args;
    };

public:
    App(NetworkManager& network,
        BrowserManager& browser,
        AudioManager& audio,
        FocusManager& focus,
        ResourceManager& resources,
        HudManager& hud) noexcept
        : network_(network), browser_(browser), audio_(audio), focus_(focus), resources_(resources), hud_(hud) {}

    ~App() = default;

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void Initialize();
    void Shutdown();
    void Tick();
    void OnPacketReceived(const NetworkPacket& packet);

private:
    void ResetSession();

    bool ResourcesReady() const;
    void FlushPendingIfReady();
    
    void RemovePendingCreate(int id);
    void QueueOrCreateOverlay(int id, const std::string& url, bool focused, bool controls_chat, float width, float height);
    void QueueOrCreateWorld(int id, const std::string& url, const std::string& textureName, float width, float height);
    void QueueOrCreateWorld2D(int id, const std::string& url, float worldX, float worldY, float worldZ, float width, float height, float offsetZ, float pivotX, float pivotY);

private:
    static constexpr int cef_port_offset_ = 2;

private:
    NetworkManager& network_;
    BrowserManager& browser_;
    AudioManager& audio_;
    FocusManager& focus_;
    ResourceManager& resources_;
    HudManager& hud_;

    bool net_endpoint_ready_ = false;
    std::string net_host_;
    unsigned short net_port_ = 0;
    unsigned long long next_connect_attempt_ms_ = 0;

    bool flushed_once_ = false;
    std::vector<PendingCreate> pending_creates_;
    std::vector<PendingEmit> pending_emits_;

    bool pending_clear_chat_ = false;
};