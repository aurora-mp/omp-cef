#pragma once

#include <string>
#include <vector>
#include "shared/packet.hpp"

class CefPlugin;

class CefApi
{
public:
    explicit CefApi(CefPlugin& plugin);
    ~CefApi();

    static CefApi* Instance()
    {
        return instance_;
    }

    bool PlayerHasPlugin(int playerid);
    void AddResource(const std::string& resourceName);
    void CreateBrowser(int playerid, int browserid, const std::string& url, bool focused, bool controls_chat);
    void CreateWorldBrowser(int playerid, int browserid, const std::string& url, const std::string& textureName, float width, float height);
    void CreateWorld2DBrowser(int playerid, int browserid, const std::string& url, float worldX, float worldY, float worldZ, float width, float height, float offsetZ, float pivotX, float pivotY);
    void SetWorld2DBrowserPos(int playerid, int browserid, float worldX, float worldY, float worldZ);
    void SetBrowserVisible(int playerid, int browserid, bool visible);
    void DestroyBrowser(int playerid, int browserid);
    void RegisterEvent(const std::string& name, const std::string& callback, const std::vector<ArgumentType>& signature);
    void EmitEvent(int playerid, int browserid, const std::string& name, const std::vector<Argument>& args);
    void ReloadBrowser(int playerid, int browserid, bool ignoreCache);
    void FocusBrowser(int playerid, int id, bool focused);
    void LoadUrl(int playerid, int browserid, const std::string& url);

    void EnableDevTools(int playerid, int browserid, bool enabled);

    void AttachBrowserToObject(int playerid, int browserid, int objectid);
    void DetachBrowserFromObject(int playerid, int browserid, int objectid);

    void SetBrowserMuted(int playerid, int browserid, bool muted);
    void SetBrowserAudioMode(int playerid, int browserid, int mode);
    void SetBrowserAudioSettings(int playerid, int browserid, float maxDistance, float referenceDistance);

    void ToggleHudComponent(int playerid, int componentid, bool toggle);
    void ToggleSpawnScreen(int playerid, bool toggle, bool trackState = true);

    void ClearChat(int playerid);
    void ToggleChatInput(int playerid, bool toggle);
    bool IsChatInputOpen(int playerid);

    void SetKeyCapture(int playerid, bool enabled);
    void EnableKey(int playerid, int key, bool enabled);

    void ExitGame(int playerid);

    void SetEscapeMenuMode(int playerid, int mode);
    void SetPlayerListMode(int playerid, int mode);

    void SetPlayerLabelData(int playerid, int targetid, const std::string& dataJSON);
private:
    CefPlugin& plugin_;

    static CefApi* instance_;
};