#pragma once

#include <string>
#include <shared/packet.hpp>

class IPlatformBridge
{
public:
    virtual ~IPlatformBridge() = default;

    virtual void LogInfo(const std::string& message) = 0;
    virtual void LogWarn(const std::string& message) = 0;
    virtual void LogError(const std::string& message) = 0;
    virtual void LogDebug(const std::string& message) = 0;

    virtual void CallPawnPublic(const std::string& name, const std::vector<Argument>& args) = 0;
    virtual void CallOnBrowserCreated(int playerid, int browserId, bool success, int code, const std::string& reason) = 0;
    virtual void CallOnDownloadStart(int playerid) = 0;
    virtual void CallOnDownloadProgress(
        int playerid,
        const std::string& fileName,
        int filePercent,
        int totalPercent,
        int fileDownloadedKb,
        int fileTotalKb,
        int totalDownloadedKb,
        int totalKb) = 0;
    virtual void CallOnDownloadFinish(int playerid) = 0;
    virtual void CallOnPressKey(int playerid, int key, int scancode, int modifiers, bool down, bool repeat) = 0;

    virtual void ShowResourceDownloadDialog(
        int playerid,
        int dialogId,
        const std::string& title,
        const std::string& body,
        const std::string& button1,
        const std::string& button2) = 0;
    virtual void HideResourceDownloadDialog(int playerid) = 0;

    virtual std::string GetPlayerAddressIp(int playerid) = 0;
    virtual void KickPlayer(int playerid) = 0;
    virtual bool IsPlayerNpcBot(int playerid) = 0;
};