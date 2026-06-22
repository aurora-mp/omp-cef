#include "app.hpp"

#include <format>
#include <fstream>
#include <chrono>

#include <nlohmann/json.hpp>
#include <game_sa/CSprite.h>
#include <game_sa/RenderWare.h>
#include <windows.h>
#include <algorithm>

#include "shared/events.hpp"
#include "shared/packet.hpp"
#include "shared/version.hpp"
#include "browser/manager.hpp"
#include "browser/audio.hpp"
#include "browser/focus.hpp"
#include "system/resource_manager.hpp"
#include "network/network_manager.hpp"
#include "ui/hud_manager.hpp"
#include "system/logger.hpp"
#include "samp/components/chat.hpp"
#include "samp/components/netgame.hpp"
#include "samp/common.hpp"
#include "utf8.hpp"

static bool SendEmitToBrowser(BrowserManager& browserManager, int browserId, const std::string& name, const std::vector<Argument>& args)
{
    auto* instance = browserManager.GetBrowserInstance(browserId);
    if (!instance)
        return false;

    CefRefPtr<CefBrowser> browser = instance->browser;
    if (!browser)
        return false;

    if (!browser->IsValid())
        return false;

    CefRefPtr<CefFrame> frame = browser->GetMainFrame();
    if (!frame)
        return false;

    if (!frame->IsValid())
        return false;

    CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("emit_event");
    CefRefPtr<CefListValue> list = msg->GetArgumentList();

    list->SetString(0, EnsureUtf8ForCef(name));

    for (size_t i = 0; i < args.size(); ++i)
    {
        const auto& arg = args[i];
        const size_t idx = i + 1;

        switch (arg.type)
        {
            case ArgumentType::Integer:
                list->SetInt(idx, arg.intValue);
                break;

            case ArgumentType::Float:
                list->SetDouble(idx, arg.floatValue);
                break;

            case ArgumentType::Bool:
                list->SetBool(idx, arg.boolValue);
                break;

            case ArgumentType::String:
                list->SetString(idx, EnsureUtf8ForCef(arg.stringValue));
                break;

            default:
                list->SetNull(idx);
                break;
        }
    }

    frame->SendProcessMessage(PID_RENDERER, msg);
    return true;
}

void App::Initialize()
{
    LOG_INFO("Init omp-cef client app ... (v{}.{}.{})", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

    network_.SetPacketHandler([this](const NetworkPacket& p) {
        OnPacketReceived(p);
    });

    resources_.Initialize();
}

void App::ResetSession()
{
    // Clear pending actions
    pending_creates_.clear();
    pending_emits_.clear();
    flushed_once_ = false;
    pending_clear_chat_ = false;

    // Destroy all browsers
    browser_.DestroyAllBrowsers();
}

bool App::ResourcesReady() const
{
    return resources_.GetState() == DownloadState::COMPLETED;
}

void App::FlushPendingIfReady()
{
    if (!ResourcesReady())
        return;

    if (!flushed_once_)
    {
        flushed_once_ = true;
        network_.SendPacket(PacketType::DownloadComplete, {});
        LOG_DEBUG("[CEF] Resources completed -> flushing pending creates={}, emits={}.", (int)pending_creates_.size(), (int)pending_emits_.size());
    }

    if (!pending_creates_.empty())
    {
        std::vector<PendingCreate> creates;
        creates.swap(pending_creates_);

        for (const auto& crate : creates)
        {
            if (crate.kind == PendingCreate::Kind::Overlay)
            {
                browser_.CreateBrowser(crate.id, crate.url, crate.focused, crate.controls_chat, crate.width, crate.height);
            }
            else if (crate.kind == PendingCreate::Kind::World)
            {
                browser_.CreateWorldBrowser(crate.id, crate.url, crate.textureName, crate.width, crate.height);
            }
            else // World2D
            {
                browser_.CreateWorld2DBrowser(crate.id, crate.url, crate.worldX, crate.worldY, crate.worldZ, crate.width, crate.height, crate.offsetZ, crate.pivotX, crate.pivotY);
            }
        }
    }

    if (!pending_emits_.empty())
    {
        std::vector<PendingEmit> remaining;
        remaining.reserve(pending_emits_.size());

        for (const auto& emit : pending_emits_)
        {
            auto* browser = browser_.GetBrowserInstance(emit.browserId);
            if (browser)
            {
				SendEmitToBrowser(browser_, emit.browserId, emit.eventName, emit.args);  
            }
            else
            {
                remaining.push_back(emit);
            }
        }

        pending_emits_ = std::move(remaining);
    }
}

void App::Tick()
{
    const auto now = ::GetTickCount64();
    auto* netGame = GetComponent<NetGameComponent>();

    if (network_.GetState() != ConnectionState::DISCONNECTED)
    {
        bool should_reset_network_session = false;
        std::string reset_reason;

        if (!netGame || !netGame->IsConnected())
        {
            should_reset_network_session = true;
            reset_reason = "SA-MP is no longer connected";
        }
        else
        {
            const int localPlayerId = netGame->GetLocalPlayerId();
            const std::string host = netGame->GetIp();
            const int gamePort = netGame->GetPort();

            if (localPlayerId < 0 || host.empty() || gamePort <= 0)
            {
                should_reset_network_session = true;
                reset_reason = "invalid SA-MP connection metadata";
            }
            else if (connected_player_id_ >= 0 && localPlayerId != connected_player_id_)
            {
                should_reset_network_session = true;
                reset_reason = "SA-MP local player id changed";
            }
            else if (!connected_game_host_.empty() && (host != connected_game_host_ || gamePort != connected_game_port_))
            {
                should_reset_network_session = true;
                reset_reason = "SA-MP server endpoint changed";
            }
        }

        if (should_reset_network_session)
        {
            LOG_INFO("[CEF] Resetting network session: {}", reset_reason.c_str());

            network_.Disconnect();
            resources_.OnDisconnect();
            ResetSession();

            net_endpoint_ready_ = false;
            net_host_.clear();
            net_port_ = 0;
            connected_player_id_ = -1;
            connected_game_host_.clear();
            connected_game_port_ = 0;
            next_connect_attempt_ms_ = now + 500ULL;
        }
    }

    if (network_.GetState() == ConnectionState::DISCONNECTED && !network_.IsNonCefServer())
    {
        const bool can_attempt_connect = (now >= next_connect_attempt_ms_);

        if (can_attempt_connect && netGame && netGame->IsConnected())
        {
            const int localPlayerId = netGame->GetLocalPlayerId();
            if (localPlayerId >= 0)
            {
                const std::string host = netGame->GetIp();
                const int gamePort = netGame->GetPort();

                if (!host.empty() && gamePort > 0)
                {
                    const int cefPortInt = gamePort + cef_port_offset_;

                    if (cefPortInt <= 0 || cefPortInt > 65535)
                    {
                        LOG_ERROR("[CEF] Invalid computed CEF port: gamePort={} offset={} => {}", gamePort, cef_port_offset_, cefPortInt);
                        next_connect_attempt_ms_ = now + 2000ULL;
                    }
                    else
                    {
                        const unsigned short cefPort = static_cast<unsigned short>(cefPortInt);

                        const bool endpoint_changed =
                            (!net_endpoint_ready_ || host != net_host_ || cefPort != net_port_);

                        if (endpoint_changed)
                        {
                            net_endpoint_ready_ = false;

                            if (!network_.Initialize(host, cefPort))
                            {
                                LOG_ERROR("[CEF] Failed to init endpoint {}:{} (game {} + {}) - will retry", host.c_str(), (int)cefPort, gamePort, cef_port_offset_);
                                next_connect_attempt_ms_ = now + 2000ULL;
                            }
                            else
                            {
                                net_endpoint_ready_ = true;
                                net_host_ = host;
                                net_port_ = cefPort;

                                LOG_INFO("[CEF] Endpoint {}:{} (derived from {}:{})", net_host_.c_str(), (int)net_port_, host.c_str(), gamePort);
                            }

                            LOG_DEBUG("[CEF] Server endpoint changed -> resetting session state.");
                            resources_.OnDisconnect();
                            ResetSession();
                        }

                        if (net_endpoint_ready_)
                        {
                            connected_player_id_ = localPlayerId;
                            connected_game_host_ = host;
                            connected_game_port_ = gamePort;
                            network_.Connect(localPlayerId);
                        }
                    }
                }
            }
        }
    }

    FlushPendingIfReady();

    focus_.Update();
    browser_.TickGameData();
    browser_.RenderAll();
    
    if (pending_clear_chat_)
    {
        pending_clear_chat_ = false;

        if (auto* chat = GetComponent<ChatComponent>())
            chat->Clear();
    }
    nlohmann::json jsonArray = nlohmann::json::array();

    // Process player 3D labels
    if (!player_labels_.empty())
    {
        
        if (netGame)
        {
            auto* playerPool = netGame->GetPlayerPool();
            if (playerPool)
            {
                for (auto it = player_labels_.begin(); it != player_labels_.end(); ++it)
                {
                    int targetId = it->first;
                    
                    if (!playerPool->IsPlayerStreamedIn(targetId))
                        continue;

                    float x, y, z;
                    if (playerPool->GetPlayerPos(targetId, x, y, z))
                    {
                        float lx, ly, lz;
                        if (playerPool->GetPlayerPos(netGame->GetLocalPlayerId(), lx, ly, lz))
                        {
                            float dist = std::sqrt((lx - x) * (lx - x) + (ly - y) * (ly - y) + (lz - z) * (lz - z));
                            
                            if (dist > it->second.drawDistance) // Draw distance limit from JSON
                                continue;
                        }

                        z += 1.15f; // Overhead offset
                        
                        RwV3d worldPos = { x, y, z };
                        RwV3d screenPos;
                        float w, h;
                        
                        bool visible = CSprite::CalcScreenCoors(worldPos, &screenPos, &w, &h, true, true);
                        
                        if (visible && screenPos.z > 0.0f)
                        {
                            nlohmann::json labelData;
                            labelData["targetId"] = targetId;
                            labelData["x"] = screenPos.x;
                            labelData["y"] = screenPos.y;
                            labelData["distance"] = screenPos.z;
                            labelData["gameWidth"] = RsGlobal.maximumWidth;
                            labelData["gameHeight"] = RsGlobal.maximumHeight;
                            
                            try {
                                labelData["data"] = nlohmann::json::parse(it->second.dataJSON);
                            } catch (...) {
                                labelData["data"] = it->second.dataJSON;
                            }

                            jsonArray.push_back(labelData);
                        }
                    }
                }
            }
        }
    }

    bool has_labels_to_draw = !jsonArray.empty();
    if (has_labels_to_draw || sent_labels_last_tick_)
    {
        std::string payload = jsonArray.dump();
        std::vector<Argument> args;
        args.push_back(payload);

        for (const auto &kv : browser_.GetAllBrowsers())
        {
            if (kv.second && kv.second->visible)
            {
                SendEmitToBrowser(browser_, kv.first, "PlayerLabelsUpdate", args);
            }
        }
        sent_labels_last_tick_ = has_labels_to_draw;
    }
}

void App::RemovePendingCreate(int id)
{
    pending_creates_.erase(
        std::remove_if(pending_creates_.begin(), pending_creates_.end(),
            [id](const PendingCreate& pending_create) { 
                return pending_create.id == id; 
            }), pending_creates_.end());
}

void App::QueueOrCreateOverlay(int id, const std::string& url, bool focused, bool controls_chat, float width, float height)
{
    if (!ResourcesReady())
    {
        PendingCreate pending_create;
        pending_create.kind = PendingCreate::Kind::Overlay;
        pending_create.id = id;
        pending_create.url = url;
        pending_create.focused = focused;
        pending_create.controls_chat = controls_chat;
        pending_create.width = width;
        pending_create.height = height;

        pending_creates_.push_back(std::move(pending_create));
        LOG_INFO("[CEF] CreateBrowser queued (id={}, url={}) - waiting resources...", id, url.c_str());
        return;
    }

    browser_.CreateBrowser(id, url, focused, controls_chat, width, height);
}

void App::QueueOrCreateWorld(int id, const std::string& url, const std::string& textureName, float width, float height)
{
    if (!ResourcesReady())
    {
        PendingCreate pending_create;
        pending_create.kind = PendingCreate::Kind::World;
        pending_create.id = id;
        pending_create.url = url;
        pending_create.textureName = textureName;
        pending_create.width = width;
        pending_create.height = height;

        pending_creates_.push_back(std::move(pending_create));
        LOG_INFO("[CEF] CreateWorldBrowser queued (id={}, url={}) - waiting resources...", id, url.c_str());
        return;
    }

    browser_.CreateWorldBrowser(id, url, textureName, width, height);
}

void App::QueueOrCreateWorld2D(int id, const std::string& url, float worldX, float worldY, float worldZ, float width, float height, float offsetZ, float pivotX, float pivotY)
{
    if (!ResourcesReady())
    {
        PendingCreate pending_create;
        pending_create.kind = PendingCreate::Kind::World2D;
        pending_create.id = id;
        pending_create.url = url;
        pending_create.width = width;
        pending_create.height = height;

        pending_create.worldX = worldX;
        pending_create.worldY = worldY;
        pending_create.worldZ = worldZ;
        pending_create.offsetZ = offsetZ;
        pending_create.pivotX = pivotX;
        pending_create.pivotY = pivotY;

        pending_creates_.push_back(std::move(pending_create));
        LOG_INFO("[CEF] CreateWorld2DBrowser queued (id={}, url={}) - waiting resources...", id, url.c_str());
        return;
    }

    browser_.CreateWorld2DBrowser(id, url, worldX, worldY, worldZ, width, height, offsetZ, pivotX, pivotY);
}

void App::OnPacketReceived(const NetworkPacket& packet)
{
    switch (packet.type)
    {
        case PacketType::ServerConfig:
        {
            const auto& cfg = std::get<ServerConfigPacket>(packet.payload);
            resources_.SetMasterKey(cfg.master_resource_key);
            resources_.SetResourcesLoaderUiEnabled(cfg.resources_loader_ui);
            resources_.MarkAsReadyToDownload();
            break;
        }
        case PacketType::FileData:
        {
            resources_.OnFileData(std::get<FileDataPacket>(packet.payload));
            break;
        }
        case PacketType::EmitEvent:
        {
            const auto& event = std::get<EmitEventPacket>(packet.payload);

            if (event.name == CefEvent::Server::CreateBrowser && event.args.size() >= 4) {
                int id = event.args[0].intValue;
                const std::string& url = event.args[1].stringValue;
                bool focused = event.args[2].boolValue;
                bool controls_chat = event.args[3].boolValue;

                QueueOrCreateOverlay(id, url, focused, controls_chat, -1.f, -1.f);
            }
            else if (event.name == CefEvent::Server::CreateWorldBrowser && event.args.size() >= 5) {
                int id = event.args[0].intValue;
                const std::string& url = event.args[1].stringValue;
                const std::string& textureName = event.args[2].stringValue;
                float width = event.args[3].floatValue;
                float height = event.args[4].floatValue;

                QueueOrCreateWorld(id, url, textureName, width, height);
            }
            else if (event.name == CefEvent::Server::CreateWorld2DBrowser && event.args.size() >= 10) {
                int id = event.args[0].intValue;
                const std::string& url = event.args[1].stringValue;
                float worldX = event.args[2].floatValue;
                float worldY = event.args[3].floatValue;
                float worldZ = event.args[4].floatValue;
                float width = event.args[5].floatValue;
                float height = event.args[6].floatValue;
                float offsetZ = event.args[7].floatValue;
                float pivotX = event.args[8].floatValue;
                float pivotY = event.args[9].floatValue;

                QueueOrCreateWorld2D(id, url, worldX, worldY, worldZ, width, height, offsetZ, pivotX, pivotY);
            } 
            else if (event.name == CefEvent::Server::SetWorld2DBrowserPos && event.args.size() >= 4) {
                int id = event.args[0].intValue;
                float worldX = event.args[1].floatValue;
                float worldY = event.args[2].floatValue;
                float worldZ = event.args[3].floatValue;

                browser_.SetWorld2DBrowserPos(id, worldX, worldY, worldZ);
            }
            else if (event.name == CefEvent::Server::SetBrowserVisible && event.args.size() >= 2) {
                int id = event.args[0].intValue;
                bool visible = event.args[1].boolValue;

                browser_.SetBrowserVisible(id, visible);
            }
            else if (event.name == CefEvent::Server::DestroyBrowser && event.args.size() >= 1) {
                const int id = event.args[0].intValue;

                RemovePendingCreate(id);

                browser_.DestroyBrowser(id);
            }
            else if (event.name == CefEvent::Server::ReloadBrowser && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                bool ignoreCache = event.args[1].boolValue;

                browser_.ReloadBrowser(browserId, ignoreCache);
            }
            else if (event.name == CefEvent::Server::FocusBrowser && event.args.size() >= 2) {
                int browserId = event.args[0].intValue;
                bool toggle = event.args[1].boolValue;

                browser_.FocusBrowser(browserId, toggle);
            }
            else if (event.name == CefEvent::Server::LoadUrl && event.args.size() >= 2)
            {
                const int browserId = event.args[0].intValue;
                const std::string& url = event.args[1].stringValue;

                browser_.LoadUrl(browserId, url);
            }
            else if (event.name == CefEvent::Server::AttachBrowserToObject && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                int objectId = event.args[1].intValue;

                browser_.AttachBrowserToObject(browserId, objectId);
            }
            else if (event.name == CefEvent::Server::DetachBrowserFromObject && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                int objectId = event.args[1].intValue;

                browser_.DetachBrowserFromObject(browserId, objectId);
            }
            else if (event.name == CefEvent::Server::MuteBrowser && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                bool muted = event.args[1].boolValue;

                audio_.SetStreamMuted(browserId, muted);
            }
            else if (event.name == CefEvent::Server::EnableDevTools && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                bool enabled  = event.args[1].boolValue;

                browser_.SetDevToolsEnabled(browserId, enabled);
            }
            else if (event.name == CefEvent::Server::SetAudioMode && event.args.size() >= 2)
            {
                int browserId = event.args[0].intValue;
                int modeValue = event.args[1].intValue;

                AudioMode mode = static_cast<AudioMode>(modeValue);
                audio_.SetStreamAudioMode(browserId, mode);
            }
            else if (event.name == CefEvent::Server::SetAudioSettings && event.args.size() >= 3)
            {
                int browserId = event.args[0].intValue;
                float maxDistance = event.args[1].floatValue;
                float refDistance = event.args[2].floatValue;

                audio_.SetStreamAudioSettings(browserId, maxDistance, refDistance);
            }
            else if (event.name == CefEvent::Server::ToggleHudComponent && event.args.size() >= 2)
            {
                int componentId = event.args[0].intValue;
                bool toggle = event.args[1].boolValue;

                hud_.ToggleComponent(static_cast<EHudComponent>(componentId), toggle);
            }
            else if (event.name == CefEvent::Server::ToggleSpawnScreen && event.args.size() >= 1)
            {
                bool toggle = event.args[0].boolValue;

                hud_.SetClassSelectionVisible(toggle);
            }
            else if (event.name == CefEvent::Server::ClearChat)
            {
                pending_clear_chat_ = true;
            }
            else if (event.name == CefEvent::Server::SetPlayerLabelData && event.args.size() >= 2)
            {
                int targetId = event.args[0].intValue;
                const std::string& dataJSON = event.args[1].stringValue;

                if (dataJSON.empty()) {
                    player_labels_.erase(targetId);
                } else {
                    player_labels_[targetId].dataJSON = dataJSON;
                    player_labels_[targetId].drawDistance = 30.0f; // Default

                    try {
                        auto j = nlohmann::json::parse(dataJSON);
                        if (j.contains("distance") && j["distance"].is_number()) {
                            player_labels_[targetId].drawDistance = j["distance"].get<float>();
                        }
                    } catch (...) {}
                }
            }
            else if (event.name == CefEvent::Server::ToggleChatInput && event.args.size() >= 1)
            {
                bool toggle = event.args[0].boolValue;

                focus_.SetChatInputEnabled(toggle);
            }
            else if (event.name == CefEvent::Server::SetKeyCapture && event.args.size() >= 1)
            {
                bool enabled = event.args[0].boolValue;

                browser_.SetKeyCaptureEnabled(enabled);
            }
            else if (event.name == CefEvent::Server::EnableKey && event.args.size() >= 2)
            {
                int key = event.args[0].intValue;
                bool enabled = event.args[1].boolValue;

                browser_.EnableKey(key, enabled);
            }
            else if (event.name == CefEvent::Server::ExitGame)
            {
                browser_.ExitGame();
            }
            else if (event.name == CefEvent::Server::SetEscapeMenuMode && event.args.size() >= 1)
            {
                int mode = event.args[0].intValue;

                browser_.SetEscapeMenuMode(static_cast<EscapeMenuMode>(mode));
            }
            else if (event.name == CefEvent::Server::SetPlayerListMode && event.args.size() >= 1)
            {
                int mode = event.args[0].intValue;

                browser_.SetPlayerListMode(static_cast<PlayerListMode>(mode));
            }
            break;
        }
        case PacketType::EmitBrowserEvent:
        {
            const auto& event = std::get<EmitEventPacket>(packet.payload);

            const int browserId = event.browserId;
            const std::string& eventName = event.name;

            auto* browser = browser_.GetBrowserInstance(browserId);
            if (browser)
            {
				SendEmitToBrowser(browser_, browserId, eventName, event.args);
                LOG_DEBUG("[CEF] EmitEvent {} sent to browser {} with {} args", eventName.c_str(), browserId, event.args.size());
            }
            else
            {
                PendingEmit pending_emit;
                pending_emit.browserId = browserId;
                pending_emit.eventName = eventName;
                pending_emit.args = event.args;
                pending_emits_.push_back(std::move(pending_emit));

                LOG_INFO("[CEF] EmitEvent '{}' queued for browser {} (not created yet).", eventName.c_str(), browserId);
            }

            break;
        }
        default:
            break;
    }
}

void App::Shutdown()
{
    LOG_INFO("Shutdown omp-cef client app ...");

    network_.Shutdown();
    resources_.OnDisconnect();
    ResetSession();

    browser_.Shutdown();
    audio_.Shutdown();

    LOG_INFO("Good bye! See you soon.");
}