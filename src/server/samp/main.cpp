#include <malloc.h>

#include <algorithm>
#include <cctype>
#include <string>

#include "amx/amx.h"
#include "plugin.h"
#include <pawn-natives/NativeFunc.hpp>
#include <pawn-natives/NativesMain.hpp>
#include <common/plugin.hpp>
#include <common/logger.hpp>
#include <shared/version.hpp>
#include "samp_bridge.hpp"
#include "config_cfg.hpp"

#define SAMPGDK_STATIC
#include <sampgdk.h>

extern void* pAMXFunctions;
std::vector<AMX*> g_AmxList;
std::unique_ptr<CefPlugin> plugin_;

namespace
{
    std::string NormalizeConfigValue(std::string value)
    {
        const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c)
        {
            return std::isspace(c) != 0;
        });

        const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c)
        {
            return std::isspace(c) != 0;
        }).base();

        if (begin >= end)
            return {};

        value = std::string(begin, end);
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });

        return value;
    }

    ResourceLoaderUiMode ReadResourceLoaderMode(const ConfigCfg& config)
    {
        const bool hasMode = config.Has("cef_resources_loader_mode");
        const std::string rawValue = hasMode
            ? config.GetString("cef_resources_loader_mode", "cef")
            : config.GetString("cef_resources_loader_ui", "1");

        const std::string value = NormalizeConfigValue(rawValue);

        if (value == "cef" || value == "1")
            return ResourceLoaderUiMode::Cef;

        if (value == "samp_dialog")
            return ResourceLoaderUiMode::SampDialog;

        if (value == "none" || value == "0")
            return ResourceLoaderUiMode::None;

        sampgdk::logprintf(
            "[CEF] [WARN] Invalid resource loader mode '%s'. Expected 'cef', 'samp_dialog' or 'none'. Defaulting to 'cef'.",
            rawValue.c_str());

        return ResourceLoaderUiMode::Cef;
    }
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
    return sampgdk::Supports() | SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void** ppData)
{
    pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
    
    bool load = sampgdk::Load(ppData);

    plugin_ = std::make_unique<CefPlugin>();

    ConfigCfg config;
    config.Load("server.cfg");

    const int server_port = config.GetPort(7777);
    const int cef_network_port = config.GetCefUdpPort(2);

    std::string debug_str = config.GetString("cef_debug", "0");
    bool debug_enabled_ = (std::stoi(debug_str) != 0);

    std::string master_key_str = config.GetString("cef_master_resource_key", "ThisIsA16ByteKey");

    std::vector<uint8_t> master_key(master_key_str.begin(), master_key_str.end());
    if (master_key.size() < 16) 
        master_key.resize(16, 0);

    if (master_key.size() > 16) 
        master_key.resize(16);

    const ResourceLoaderUiMode resource_loader_mode = ReadResourceLoaderMode(config);
    const int resource_download_dialog_id = config.GetInt("cef_resources_loader_dialog_id", ResourceDownloadDialogManager::DefaultDialogId);

    CefPluginOptions options;
    options.log_level = debug_enabled_ ? CefLogLevel::Debug : CefLogLevel::Info;
    options.master_resource_key = master_key;
    options.resources_loader_mode = resource_loader_mode;
    options.resources_loader_dialog_id = resource_download_dialog_id;

    auto bridge = CreateSampPlatformBridge();
    plugin_->Initialize(std::move(bridge), cef_network_port, options);

    LOG_INFO("Plugin loaded (v%d.%d.%d)", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

    return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	plugin_->Shutdown();
    sampgdk::Unload();
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx)
{
    g_AmxList.push_back(amx);
    return pawn_natives::AmxLoad(amx);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX* amx)
{
    g_AmxList.erase(std::remove(g_AmxList.begin(), g_AmxList.end(), amx), g_AmxList.end());
    return AMX_ERR_NONE;
}


PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerConnect(int playerid)
{
	plugin_->OnPlayerConnect(playerid);
    return true;
}

PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerDisconnect(int playerid, int reason)
{
    (void)reason;
    if (plugin_)
        plugin_->OnPlayerDisconnect(playerid);
    return true;
}

// SA-MP doesn't have an "OnPlayerClientInit" callback like open.mp.
// We use early client lifecycle callbacks (class selection) to start
// the handshake timeout logic and detect players without the CEF client.
static inline void MaybeNotifyClientInit(int playerid)
{
    if (!plugin_)
        return;

    plugin_->OnPlayerClientInit(playerid);
}

PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerRequestClass(int playerid, int classid)
{
    (void)classid;
    MaybeNotifyClientInit(playerid);
    return true;
}