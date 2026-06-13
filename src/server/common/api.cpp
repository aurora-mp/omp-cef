#include "api.hpp"
#include "plugin.hpp"
#include "shared/events.hpp"
#include "natives.hpp"

CefApi* CefApi::instance_ = nullptr;

CefApi::CefApi(CefPlugin& plugin) : plugin_(plugin)
{
    instance_ = this;
}

CefApi::~CefApi()
{
    instance_ = nullptr;
}

bool CefApi::PlayerHasPlugin(int playerid)
{
	return plugin_.GetNetworkSessionManager().HasPlayerPlugin(playerid);
}

void CefApi::AddResource(const std::string& resourceName)
{
    plugin_.GetResourceManager().AddResource(
        resourceName,
        plugin_.GetMasterKey()
    );
}

void CefApi::CreateBrowser(int playerid, int browserid, const std::string& url, bool focused, bool controls_chat)
{
    EmitEventPacket event;

    event.name = CefEvent::Server::CreateBrowser;
    event.args.emplace_back(browserid);
    event.args.emplace_back(url);
    event.args.emplace_back(focused);
    event.args.emplace_back(controls_chat);

    plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::CreateWorldBrowser(int playerid, int browserid, const std::string& url, const std::string& textureName, float width, float height)
{
	EmitEventPacket event;

	event.name = CefEvent::Server::CreateWorldBrowser;
	event.args.emplace_back(browserid);
	event.args.emplace_back(std::string(url));
	event.args.emplace_back(std::string(textureName));
	event.args.emplace_back(width);
	event.args.emplace_back(height);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::CreateWorld2DBrowser(int playerid, int browserid, const std::string& url, float worldX, float worldY, float worldZ, float width, float height, float offsetZ, float pivotX, float pivotY)
{
	EmitEventPacket event;

	event.name = CefEvent::Server::CreateWorld2DBrowser;
	event.args.emplace_back(browserid);
	event.args.emplace_back(std::string(url));
	event.args.emplace_back(worldX);
	event.args.emplace_back(worldY);
	event.args.emplace_back(worldZ);
	event.args.emplace_back(width);
	event.args.emplace_back(height);
	event.args.emplace_back(offsetZ);
	event.args.emplace_back(pivotX);
	event.args.emplace_back(pivotY);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
} 

void CefApi::SetWorld2DBrowserPos(int playerid, int browserid, float worldX, float worldY, float worldZ) 
{
	EmitEventPacket event;

	event.name = CefEvent::Server::SetWorld2DBrowserPos;
	event.args.emplace_back(browserid);
	event.args.emplace_back(worldX);
	event.args.emplace_back(worldY);
	event.args.emplace_back(worldZ);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::SetBrowserVisible(int playerid, int browserid, bool visible)
{
	EmitEventPacket event;

	event.name = CefEvent::Server::SetBrowserVisible;
	event.args.emplace_back(browserid);
	event.args.emplace_back(visible);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::DestroyBrowser(int playerid, int browserid)
{
	EmitEventPacket event;

	event.name = CefEvent::Server::DestroyBrowser;
	event.args.emplace_back(browserid);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::RegisterEvent(const std::string& name, const std::string& callback, const std::vector<ArgumentType>& signature) 
{
	plugin_.RegisterEvent(name, callback, signature);
}

void CefApi::EmitEvent(int playerid, int browserid, const std::string& name, const std::vector<Argument>& args)
{
	LOG_DEBUG("[CEF] EmitEvent: playerid=%d, browserid=%d, name=%.*s, args=%zu", playerid, browserid, static_cast<int>(name.size()), name.data(), args.size());

	EmitEventPacket event;

	event.browserId = browserid;
	event.name = std::string(name);
	event.args = args;

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitBrowserEvent, event);
}

void CefApi::ReloadBrowser(int playerid, int browserid, bool ignoreCache)
{
	LOG_DEBUG("ReloadBrowser: playerid=%d, browserid=%d", playerid, browserid);

	EmitEventPacket event;

	event.name = CefEvent::Server::ReloadBrowser;
	event.args.emplace_back(browserid);
	event.args.emplace_back(ignoreCache);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::FocusBrowser(int playerid, int browserid, bool focused)
{
	LOG_DEBUG("FocusBrowser: playerid=%d, browserid=%d, focused=%d", playerid, browserid, focused);

	EmitEventPacket event;

	event.name = CefEvent::Server::FocusBrowser;
	event.args.emplace_back(browserid);
	event.args.emplace_back(focused);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::EnableDevTools(int playerid, int browserid, bool enabled)
{
	LOG_DEBUG("EnableDevTools: playerid=%d, browserid=%d, enabled=%d", playerid, browserid, enabled);

	EmitEventPacket event;

	event.name = CefEvent::Server::EnableDevTools;
	event.args.emplace_back(browserid);
	event.args.emplace_back(enabled);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::AttachBrowserToObject(int playerid, int browserid, int objectId)
{
	EmitEventPacket event;

	event.name = CefEvent::Server::AttachBrowserToObject;
	event.args.emplace_back(browserid);
	event.args.emplace_back(objectId);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::DetachBrowserFromObject(int playerid, int browserid, int objectid)
{
	EmitEventPacket event;

	event.name = CefEvent::Server::DetachBrowserFromObject;
	event.args.emplace_back(browserid);
	event.args.emplace_back(objectid);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::SetBrowserMuted(int playerid, int browserid, bool muted)
{
	LOG_DEBUG("SetBrowserMuted: playerid=%d, browserid=%d, muted=%d", playerid, browserid, muted);

	EmitEventPacket event;

	event.name = CefEvent::Server::MuteBrowser;
	event.args.emplace_back(browserid);
	event.args.emplace_back(muted);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::SetBrowserAudioMode(int playerid, int browserid, int mode)
{
	if (mode < AUDIO_MODE_WORLD || mode > AUDIO_MODE_UI) {
		LOG_ERROR("SetAudioMode: Invalid audio mode (%d).", mode);
		return;
	}

	EmitEventPacket event;

	event.name = CefEvent::Server::SetAudioMode;
	event.args.emplace_back(browserid);
	event.args.emplace_back(mode);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::SetBrowserAudioSettings(int playerid, int browserid, float maxDistance, float referenceDistance)
{
	EmitEventPacket event;

	event.name = CefEvent::Server::SetAudioSettings;
	event.args.emplace_back(browserid);
	event.args.emplace_back(maxDistance);
	event.args.emplace_back(referenceDistance);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::ToggleHudComponent(int playerid, int componentid, bool toggle)
{
	LOG_DEBUG("ToggleHudComponent: playerid=%d, componentid=%d, toggle=%d", playerid, componentid, toggle);

	EmitEventPacket event;
	event.name = CefEvent::Server::ToggleHudComponent;

	event.args.emplace_back(componentid);
	event.args.emplace_back(toggle);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::ToggleSpawnScreen(int playerid, bool toggle)
{
	LOG_DEBUG("ToggleSpawnScreen: playerid=%d, toggle=%d", playerid, toggle);

	EmitEventPacket event;
	event.name = CefEvent::Server::ToggleSpawnScreen;
	event.args.emplace_back(toggle);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::ClearChat(int playerid)
{
	LOG_DEBUG("ClearChat: playerid=%d", playerid);

    EmitEventPacket event;
    event.name = CefEvent::Server::ClearChat;

    plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::ToggleChatInput(int playerid, bool toggle)
{
	LOG_DEBUG("ToggleChatInput: playerid=%d, toggle=%d", playerid, toggle);

	EmitEventPacket event;
	event.name = CefEvent::Server::ToggleChatInput;
	event.args.emplace_back(toggle);

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

bool CefApi::IsChatInputOpen(int playerid)
{
    auto session = plugin_.GetNetworkSessionManager().GetSession(playerid);
    if (!session)
        return false;

    return session->chat_input_open;
}

void CefApi::SetKeyCapture(int playerid, bool enabled)
{
	LOG_DEBUG("SetKeyCapture: playerid=%d, enabled=%d", playerid, enabled);

    EmitEventPacket event;
    event.name = CefEvent::Server::SetKeyCapture;
    event.args.emplace_back(enabled);

    plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::EnableKey(int playerid, int key, bool enabled)
{
    EmitEventPacket event;
    event.name = CefEvent::Server::EnableKey;
    event.args.emplace_back(key);
    event.args.emplace_back(enabled);

    plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}

void CefApi::ExitGame(int playerid)
{
	EmitEventPacket event;
	event.name = CefEvent::Server::ExitGame;

	plugin_.SendPacketToPlayer(playerid, PacketType::EmitEvent, event);
}