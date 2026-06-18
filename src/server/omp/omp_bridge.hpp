#pragma once

#include <Server/Components/Pawn/pawn.hpp>
#include "common/bridge.hpp"

std::unique_ptr<IPlatformBridge> CreateOmpPlatformBridge(ICore* core, IPawnComponent* pawn);

class OmpPlatformBridge final : public IPlatformBridge
{
public:
    OmpPlatformBridge(ICore* core, IPawnComponent* pawn);

    void LogInfo(const std::string& message) override;
    void LogWarn(const std::string& message) override;
    void LogError(const std::string& message) override;
    void LogDebug(const std::string& message) override;

    void CallPawnPublic(const std::string& name, const std::vector<Argument>& args) override;
    void CallOnBrowserCreated(int playerid, int browserid, bool success, int code, const std::string& reason) override;
    void CallOnDownloadStart(int playerid) override;
    void CallOnDownloadProgress(
        int playerid,
        const std::string& fileName,
        int filePercent,
        int totalPercent,
        int fileDownloadedKb,
        int fileTotalKb,
        int totalDownloadedKb,
        int totalKb) override;
    void CallOnDownloadFinish(int playerid) override;
    void CallOnPressKey(int playerid, int key, int scancode, int modifiers, bool down, bool repeat) override;

    void ShowResourceDownloadDialog(
        int playerid,
        int dialogid,
        const std::string& title,
        const std::string& body,
        const std::string& button1,
        const std::string& button2) override;
    void HideResourceDownloadDialog(int playerid) override;

    std::string GetPlayerAddressIp(int playerid) override;
    void KickPlayer(int playerid) override;
    bool IsPlayerNpcBot(int playerid) override;

private:
    ICore* core_;
    IPawnComponent* pawn_;
};
