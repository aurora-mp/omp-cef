#include "component.hpp"
#include "omp_bridge.hpp"

#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>

#include <shared/version.hpp>

#include <string>

namespace
{
    ResourceLoaderUiMode ReadResourceLoaderMode(IEarlyConfig& config, ILogger& logger)
    {
        if (config.getType("cef.resources_loader_mode") == ConfigOptionType_String)
        {
            const StringView mode = config.getString("cef.resources_loader_mode");
            const std::string value(mode.data(), mode.length());

            if (value == "cef")
                return ResourceLoaderUiMode::Cef;

            if (value == "samp_dialog")
                return ResourceLoaderUiMode::SampDialog;

            if (value == "none")
                return ResourceLoaderUiMode::None;

            logger.printLn(
                "[CEF] Invalid cef.resources_loader_mode '%.*s'. Expected 'cef', 'samp_dialog' or 'none'. Defaulting to 'cef'.",
                static_cast<int>(mode.length()),
                mode.data());

            return ResourceLoaderUiMode::Cef;
        }

        const bool legacy_ui_enabled = config.getBool("cef.resources_loader_ui")
            ? *config.getBool("cef.resources_loader_ui")
            : true;

        return legacy_ui_enabled ? ResourceLoaderUiMode::Cef : ResourceLoaderUiMode::None;
    }
}

StringView CefOmpComponent::componentName() const
{
    return "Cef Component";
}

SemanticVersion CefOmpComponent::componentVersion() const
{
	return { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_BUILD };
}

void CefOmpComponent::onLoad(ICore* core)
{
    core_ = core;
    core_->getPlayers().getPlayerConnectDispatcher().addEventHandler(this);
    core_->getEventDispatcher().addEventHandler(this);
    setAmxLookups(core_);
}

void CefOmpComponent::onInit(IComponentList* components)
{
    plugin_ = std::make_unique<CefPlugin>();
    pawn_ = components->queryComponent<IPawnComponent>();

    if (pawn_)
    {
        setAmxFunctions(pawn_->getAmxFunctions());
		setAmxLookups(components);
        pawn_->getEventDispatcher().addEventHandler(this);
    }

    CefPluginOptions options;
    options.log_level = debug_enabled_ ? CefLogLevel::Debug : CefLogLevel::Info;
    options.master_resource_key = master_resource_key_;
    options.resources_loader_mode = resource_loader_ui_mode_;
    options.resources_loader_dialog_id = resource_download_dialog_id_;

    auto bridge = CreateOmpPlatformBridge(core_, pawn_);
    plugin_->Initialize(std::move(bridge), cef_network_port_, options);

    LOG_INFO("Component initialized (v%d.%d.%d).", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

void CefOmpComponent::onReady() {}

void CefOmpComponent::onFree(IComponent* component)
{
    if (component == pawn_)
    {
        pawn_ = nullptr;
    }
}

void CefOmpComponent::provideConfiguration(ILogger& logger, IEarlyConfig& config, bool defaults) 
{
    int* port_ptr = config.getInt("network.port");
    const int port_value = (port_ptr && *port_ptr > 0 && *port_ptr <= 65535) ? *port_ptr : 7777;
    
    server_port_ = static_cast<uint16_t>(port_value);
    cef_network_port_ = static_cast<uint16_t>(port_value + 2);

	if (defaults) {
		config.setBool("cef.debug", false);
		config.setString("cef.master_resource_key", "ThisIsA16ByteKey");
        config.setString("cef.resources_loader_mode", "cef");
        config.setInt("cef.resources_loader_dialog_id", ResourceDownloadDialogManager::DefaultDialogId);
	}
	else {
		if (config.getType("cef.debug") == ConfigOptionType_None) {
			config.setBool("cef.debug", false);
		}

		if (config.getType("cef.master_resource_key") == ConfigOptionType_None) {
			config.setString("cef.master_resource_key", "ThisIsA16ByteKey");
		}

        if (config.getType("cef.resources_loader_mode") == ConfigOptionType_None &&
            config.getType("cef.resources_loader_ui") == ConfigOptionType_None) {
            config.setString("cef.resources_loader_mode", "cef");
        }

        if (config.getType("cef.resources_loader_dialog_id") == ConfigOptionType_None) {
            config.setInt("cef.resources_loader_dialog_id", ResourceDownloadDialogManager::DefaultDialogId);
        }
	}

	debug_enabled_ = config.getBool("cef.debug") ? *config.getBool("cef.debug") : false;

	StringView key_sv = config.getString("cef.master_resource_key");
	size_t key_len = key_sv.length();

	if (key_len == 16 || key_len == 24 || key_len == 32) {
		master_resource_key_.assign(key_sv.data(), key_sv.data() + key_len);
		logger.printLn("[CEF Security] Master resource key loaded successfully (%zu bytes, for AES-%zu).", key_len, key_len * 8);
	}
	else {
		logger.printLn("===================================================================");
		logger.printLn("[CEF SECURITY ERROR] The 'cef.master_resource_key' is INVALID!");
		logger.printLn("[CEF SECURITY ERROR] Its length is %zu bytes, but it MUST be 16, 24, or 32 bytes.", key_len);
		logger.printLn("[CEF SECURITY ERROR] Resource encryption will fail. Please fix your configuration.");
		logger.printLn("===================================================================");
	}

    resource_loader_ui_mode_ = ReadResourceLoaderMode(config, logger);

    int* dialog_id = config.getInt("cef.resources_loader_dialog_id");
    resource_download_dialog_id_ = dialog_id ? *dialog_id : ResourceDownloadDialogManager::DefaultDialogId;
}

void CefOmpComponent::free()
{
    delete this;
}

void CefOmpComponent::reset() {}

void CefOmpComponent::onAmxLoad(IPawnScript& script) 
{
    pawn_natives::AmxLoad(script.GetAMX());
}

void CefOmpComponent::onAmxUnload(IPawnScript& script) 
{

}

void CefOmpComponent::onPlayerConnect(IPlayer& player)
{
    plugin_->OnPlayerConnect(player.getID());
}

void CefOmpComponent::onPlayerClientInit(IPlayer& player)
{
    plugin_->OnPlayerClientInit(player.getID());
}

void CefOmpComponent::onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason) 
{
	plugin_->OnPlayerDisconnect(player.getID());
}

void CefOmpComponent::onTick(Microseconds elapsed, TimePoint now)
{
    if (plugin_)
    {
        plugin_->ProcessPendingCallbacks();
    }
}

CefOmpComponent::~CefOmpComponent()
{
    if (pawn_)
    {
        pawn_->getEventDispatcher().removeEventHandler(this);
    }

    if (core_)
    {
        core_->getPlayers().getPlayerConnectDispatcher().removeEventHandler(this);
        core_->getEventDispatcher().removeEventHandler(this);
    }
}