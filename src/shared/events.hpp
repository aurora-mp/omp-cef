#pragma once

namespace CefEvent
{
    namespace Server
    {
        inline constexpr const char* CreateBrowser = "CreateBrowser";
        inline constexpr const char* CreateWorldBrowser = "CreateWorldBrowser";
        inline constexpr const char* CreateWorld2DBrowser = "CreateWorld2DBrowser";

        inline constexpr const char* SetWorld2DBrowserPos = "SetWorld2DBrowserPos";

        inline constexpr const char* SetBrowserVisible = "SetBrowserVisible";
        inline constexpr const char* DestroyBrowser = "DestroyBrowser";
        inline constexpr const char* ReloadBrowser = "ReloadBrowser";
        inline constexpr const char* FocusBrowser = "FocusBrowser";

        inline constexpr const char* EnableDevTools = "EnableDevTools";

        inline constexpr const char* AttachBrowserToObject = "AttachBrowserToObject";
        inline constexpr const char* DetachBrowserFromObject = "DetachBrowserFromObject";

        inline constexpr const char* MuteBrowser = "MuteBrowser";
        inline constexpr const char* SetAudioMode = "SetAudioMode";
        inline constexpr const char* SetAudioSettings = "SetAudioSettings";

        inline constexpr const char* ToggleHudComponent = "ToggleHudComponent";
        inline constexpr const char* ToggleSpawnScreen = "ToggleSpawnScreen";

        inline constexpr const char* ClearChat = "ClearChat";
        inline constexpr const char* ToggleChatInput = "ToggleChatInput";

        inline constexpr const char* SetKeyCapture = "SetKeyCapture";
        inline constexpr const char* EnableKey = "EnableKey";

        inline constexpr const char* ExitGame = "ExitGame";
    }

    namespace Client 
    {
        inline constexpr const char* BrowserCreateResult = "BrowserCreateResult";
        inline constexpr const char* DownloadStart = "DownloadStart";
        inline constexpr const char* DownloadFinish = "DownloadFinish";
        inline constexpr const char* PressKey = "PressKey";
        inline constexpr const char* ChatInputState = "ChatInputState";
    }
}