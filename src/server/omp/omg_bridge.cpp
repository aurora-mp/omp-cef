#include "omp_bridge.hpp"
#include <sdk.hpp>
#include <common/encoding.hpp>

OmpPlatformBridge::OmpPlatformBridge(ICore* core, IPawnComponent* pawn) : core_(core), pawn_(pawn) {}

std::unique_ptr<IPlatformBridge> CreateOmpPlatformBridge(ICore* core, IPawnComponent* pawn)
{
    return std::make_unique<OmpPlatformBridge>(core, pawn);
}

void OmpPlatformBridge::LogInfo(const std::string& message)
{
    if (core_)
        core_->printLn("[CEF] %s", message.c_str());
}

void OmpPlatformBridge::LogWarn(const std::string& message)
{
    if (core_)
        core_->printLn("[CEF] [WARN] %s", message.c_str());
}

void OmpPlatformBridge::LogError(const std::string& message)
{
    if (core_)
        core_->printLn("[CEF] [ERROR] %s", message.c_str());
}

void OmpPlatformBridge::LogDebug(const std::string& message)
{
    if (core_)
        core_->printLn("[CEF] [DEBUG] %s", message.c_str());
}

void OmpPlatformBridge::CallPawnPublic(const std::string& name, const std::vector<Argument>& args)
{
    if (!pawn_)
        return;

    auto call_on_script = [&](IPawnScript* script)
    {
        if (!script)
            return;

        int idx = 0;
        if (script->FindPublic(name.c_str(), &idx) != AMX_ERR_NONE)
            return;

        const cell heap_before = script->GetHEA();
        int error = AMX_ERR_NONE;

        for (auto it = args.rbegin(); it != args.rend() && error == AMX_ERR_NONE; ++it)
        {
            const auto& arg = *it;
            switch (arg.type)
            {
                case ArgumentType::String:
                {
                    std::string ansi_string = Utf8ToAnsi(arg.stringValue);
                    cell amx_addr = 0;
                    error = script->PushString(&amx_addr, nullptr, ansi_string, false, false);
                    break;
                }
                case ArgumentType::Integer:
                    error = script->Push(arg.intValue);
                    break;
                case ArgumentType::Float:
                    error = script->Push(amx_ftoc(arg.floatValue));
                    break;
                case ArgumentType::Bool:
                    error = script->Push(arg.boolValue);
                    break;
            }
        }

        if (error == AMX_ERR_NONE)
        {
            cell retval;
            error = script->Exec(&retval, idx);
        }

        script->Release(heap_before);

        if (error != AMX_ERR_NONE)
            LogError("Failed to call " + name + " (AMX error " + std::to_string(error) + ").");
    };

    call_on_script(pawn_->mainScript());

    for (IPawnScript* script : pawn_->sideScripts())
    {
        call_on_script(script);
    }
}

void OmpPlatformBridge::CallOnBrowserCreated(int playerid, int browserId, bool success, int code, const std::string& reason)
{
    if (!pawn_) 
        return;

    auto call = [&](IPawnScript* script) {
        if (!script) 
            return;

        script->Call("OnCefBrowserCreated", DefaultReturnValue_False, playerid, browserId, success, code, StringView(reason));
    };

    call(pawn_->mainScript());
    for (IPawnScript* script : pawn_->sideScripts()) {
        call(script);
    }
}

void OmpPlatformBridge::CallOnDownloadStart(int playerid)
{
    if (!pawn_) 
        return;

    auto call = [&](IPawnScript* script) {
        if (!script) 
            return;

        script->Call("OnCefDownloadStart", DefaultReturnValue_True, playerid);
    };

    call(pawn_->mainScript());
    for (IPawnScript* script : pawn_->sideScripts()) {
        call(script);
    }
}

void OmpPlatformBridge::CallOnDownloadProgress(
    int playerid,
    const std::string& fileName,
    int filePercent,
    int totalPercent,
    int fileDownloadedKb,
    int fileTotalKb,
    int totalDownloadedKb,
    int totalKb)
{
    if (!pawn_)
        return;

    auto call = [&](IPawnScript* script) {
        if (!script)
            return;

        script->Call(
            "OnCefDownloadProgress",
            DefaultReturnValue_True,
            playerid,
            StringView(fileName),
            filePercent,
            totalPercent,
            fileDownloadedKb,
            fileTotalKb,
            totalDownloadedKb,
            totalKb);
    };

    call(pawn_->mainScript());
    for (IPawnScript* script : pawn_->sideScripts()) {
        call(script);
    }
}

void OmpPlatformBridge::CallOnDownloadFinish(int playerid)
{
    if (!pawn_) 
        return;

    auto call = [&](IPawnScript* script) {
        if (!script) 
            return;

        script->Call("OnCefDownloadFinish", DefaultReturnValue_True, playerid);
    };

    call(pawn_->mainScript());
    for (IPawnScript* script : pawn_->sideScripts()) {
        call(script);
    }
}

void OmpPlatformBridge::CallOnPressKey(int playerid, int key, int scancode, int modifiers, bool down, bool repeat)
{
    if (!pawn_) 
        return;

    auto call = [&](IPawnScript* script) {
        if (!script) 
            return;

        script->Call("OnCefPressKey", DefaultReturnValue_True, playerid, key, scancode, modifiers, down, repeat);
    };

    call(pawn_->mainScript());
    for (IPawnScript* script : pawn_->sideScripts()) {
        call(script);
    }
}

void OmpPlatformBridge::ShowResourceDownloadDialog(
    int playerid,
    int dialogid,
    const std::string& title,
    const std::string& body,
    const std::string& button1,
    const std::string& button2)
{
    if (!core_)
        return;

    IPlayer* player = core_->getPlayers().get(playerid);
    if (!player)
        return;

    auto* dialog = queryExtension<IPlayerDialogData>(*player);
    if (!dialog)
        return;

    dialog->show(
        *player,
        dialogid,
        DialogStyle_TABLIST_HEADERS,
        StringView(title),
        StringView(body),
        StringView(button1),
        StringView(button2));
}

void OmpPlatformBridge::HideResourceDownloadDialog(int playerid)
{
    if (!core_)
        return;

    IPlayer* player = core_->getPlayers().get(playerid);
    if (!player)
        return;

    auto* dialog = queryExtension<IPlayerDialogData>(*player);
    if (!dialog)
        return;

    dialog->hide(*player);
}

std::string OmpPlatformBridge::GetPlayerAddressIp(int playerid)
{
    if (core_)
    {
        IPlayer* player = core_->getPlayers().get(playerid);
        if (player)
        {
            const PeerAddress& addr = player->getNetworkData().networkID.address;

            PeerAddress::AddressString address_str;
            if (PeerAddress::ToString(addr, address_str))
            {
                return std::string(address_str.data());
            }
        }
    }

    return "";
}

void OmpPlatformBridge::KickPlayer(int playerid)
{
    if (core_)
    {
        IPlayer* player = core_->getPlayers().get(playerid);
        if (!player)
            return;

        player->kick();
    }
}

bool OmpPlatformBridge::IsPlayerNpcBot(int playerid)
{
    if (!core_)
        return false;

    IPlayer* player = core_->getPlayers().get(playerid);
    if (!player)
        return false;

	return player->isBot();
}

void OmpPlatformBridge::InvalidatePawn()
{
	pawn_ = nullptr;
}
