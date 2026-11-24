#include "samp_bridge.hpp"
#include <common/encoding.hpp>

#define SAMPGDK_STATIC
#include <sampgdk.h>

extern std::vector<AMX*> g_AmxList;

std::unique_ptr<IPlatformBridge> CreateSampPlatformBridge()
{
    return std::make_unique<SampPlatformBridge>();
}

void SampPlatformBridge::LogInfo(const std::string& message)
{
    sampgdk::logprintf("[CEF] [INFO] %s", message.c_str());
}

void SampPlatformBridge::LogWarn(const std::string& message)
{
    sampgdk::logprintf("[CEF] [WARN] %s", message.c_str());
}

void SampPlatformBridge::LogError(const std::string& message)
{
    sampgdk::logprintf("[CEF] [ERROR] %s", message.c_str());
}

void SampPlatformBridge::LogDebug(const std::string& message)
{
    sampgdk::logprintf("[CEF] [DEBUG] %s", message.c_str());
}

void SampPlatformBridge::CallPawnPublic(const std::string& name, const std::vector<Argument>& args)
{
    for (AMX* amx : g_AmxList)
    {
        int idx = 0;
        if (amx_FindPublic(amx, name.c_str(), &idx) != AMX_ERR_NONE)
            continue;

        const cell heap_before = amx->hea;
        int error = AMX_ERR_NONE;

        for (auto it = args.rbegin(); it != args.rend() && error == AMX_ERR_NONE; ++it)
        {
            const auto& arg = *it;
            switch (arg.type)
            {
                case ArgumentType::String:
                {
                    cell amx_addr = 0;
                    std::string ansi_string = Utf8ToAnsi(arg.stringValue);
                    error = amx_PushString(amx, &amx_addr, NULL, ansi_string.c_str(), 0, 0);
                    break;
                }
                case ArgumentType::Integer:
                    error = amx_Push(amx, arg.intValue);
                    break;
                case ArgumentType::Float:
                    error = amx_Push(amx, amx_ftoc(arg.floatValue));
                    break;
                case ArgumentType::Bool:
                    error = amx_Push(amx, arg.boolValue);
                    break;
            }
        }

        if (error == AMX_ERR_NONE)
        {
            cell ret;
            error = amx_Exec(amx, &ret, idx);
        }

        amx_Release(amx, heap_before);

        if (error != AMX_ERR_NONE)
            LogError("Failed to call " + name + " (AMX error " + std::to_string(error) + ").");
    }
}

void SampPlatformBridge::CallOnBrowserCreated(int playerid, int browserId, bool success, int code, const std::string& reason)
{
    for (AMX* amx : g_AmxList)
    {
        int idx;
        if (amx_FindPublic(amx, "OnCefBrowserCreated", &idx) != AMX_ERR_NONE) 
            continue;

        const cell heap_before = amx->hea;
        cell reason_addr = 0;

        if (amx_PushString(amx, &reason_addr, NULL, reason.c_str(), 0, 0) == AMX_ERR_NONE)
        {
            amx_Push(amx, code);
            amx_Push(amx, success);
            amx_Push(amx, browserId);
            amx_Push(amx, playerid);

            cell retval;
            amx_Exec(amx, &retval, idx);
        }

        amx_Release(amx, heap_before);
    }
}

void SampPlatformBridge::CallOnDownloadStart(int playerid)
{
    for (AMX* amx : g_AmxList)
    {
        int idx;
        if (amx_FindPublic(amx, "OnCefDownloadStart", &idx) != AMX_ERR_NONE) 
            continue;

        amx_Push(amx, playerid);

        cell retval;
        amx_Exec(amx, &retval, idx);
    }
}

void SampPlatformBridge::CallOnDownloadProgress(
    int playerid,
    const std::string& fileName,
    int filePercent,
    int totalPercent,
    int fileDownloadedKb,
    int fileTotalKb,
    int totalDownloadedKb,
    int totalKb)
{
    for (AMX* amx : g_AmxList)
    {
        int idx;
        if (amx_FindPublic(amx, "OnCefDownloadProgress", &idx) != AMX_ERR_NONE)
            continue;

        cell file_name_addr = 0;
        std::string ansi_file_name = Utf8ToAnsi(fileName);

        amx_Push(amx, totalKb);
        amx_Push(amx, totalDownloadedKb);
        amx_Push(amx, fileTotalKb);
        amx_Push(amx, fileDownloadedKb);
        amx_Push(amx, totalPercent);
        amx_Push(amx, filePercent);
        const cell heap_before = amx->hea;

        if (amx_PushString(amx, &file_name_addr, NULL, ansi_file_name.c_str(), 0, 0) == AMX_ERR_NONE)
        {
            amx_Push(amx, playerid);

            cell retval;
            amx_Exec(amx, &retval, idx);
        }

        amx_Release(amx, heap_before);
    }
}

void SampPlatformBridge::CallOnDownloadFinish(int playerid)
{
    for (AMX* amx : g_AmxList)
    {
        int idx;
        if (amx_FindPublic(amx, "OnCefDownloadFinish", &idx) != AMX_ERR_NONE) 
            continue;

        amx_Push(amx, playerid);

        cell retval;
        amx_Exec(amx, &retval, idx);
    }
}

void SampPlatformBridge::CallOnPressKey(int playerid, int key, int scancode, int modifiers, bool down, bool repeat)
{
    for (AMX* amx : g_AmxList)
    {
        int idx;
        if (amx_FindPublic(amx, "OnCefPressKey", &idx) != AMX_ERR_NONE) 
            continue;

        amx_Push(amx, repeat);
        amx_Push(amx, down);
        amx_Push(amx, modifiers);
        amx_Push(amx, scancode);
        amx_Push(amx, key);
        amx_Push(amx, playerid);

        cell retval;
        amx_Exec(amx, &retval, idx);
    }
}

void SampPlatformBridge::ShowResourceDownloadDialog(
    int playerid,
    int dialogid,
    const std::string& title,
    const std::string& body,
    const std::string& button1,
    const std::string& button2)
{
    static constexpr int DialogStyleTablistHeaders = 5;

    const std::string ansi_title = Utf8ToAnsi(title);
    const std::string ansi_body = Utf8ToAnsi(body);
    const std::string ansi_button1 = Utf8ToAnsi(button1);
    const std::string ansi_button2 = Utf8ToAnsi(button2);

    sampgdk_ShowPlayerDialog(
        playerid,
        dialogid,
        DialogStyleTablistHeaders,
        ansi_title.c_str(),
        ansi_body.c_str(),
        ansi_button1.c_str(),
        ansi_button2.c_str());
}

void SampPlatformBridge::HideResourceDownloadDialog(int playerid)
{
    static constexpr int DialogStyleMsgBox = 0;

    sampgdk_ShowPlayerDialog(playerid, -1, DialogStyleMsgBox, " ", " ", " ", " ");
}

std::string SampPlatformBridge::GetPlayerAddressIp(int playerid)
{
    char ip[64] = {};

    // GetPlayerIp returns true on success (sampgdk declares it as bool).
    if (sampgdk_GetPlayerIp(playerid, ip, sizeof(ip)) && ip[0] != '\0')
        return std::string(ip);

    return {};
}

void SampPlatformBridge::KickPlayer(int playerid)
{
    sampgdk_Kick(playerid);
}

bool SampPlatformBridge::IsPlayerNpcBot(int playerid)
{
    return sampgdk_IsPlayerNPC(playerid);
}
