#include "chat.hpp"

#include <cstdint>

#include "browser/focus.hpp"
#include "browser/manager.hpp"
#include "hooks/hook_manager.hpp"
#include "network/network_manager.hpp"
#include "samp/addresses.hpp"
#include "system/logger.hpp"
#include "shared/events.hpp"
#include "utf8.hpp"

bool ChatHook::Initialize()
{
    s_self_ = this;

    auto& addrs = SampAddresses::Instance();
    auto* base = addrs.Base();

    void* addrOpenChat = nullptr;
    void* addrCloseChat = nullptr;

    switch (addrs.Version())
    {
        case SampVersion::V037:
            addrOpenChat = reinterpret_cast<void*>(base + 0x657E0);
            addrCloseChat = reinterpret_cast<void*>(base + 0x658E0);
            break;
        case SampVersion::V037R3:
            addrOpenChat = reinterpret_cast<void*>(base + 0x68D10);
            addrCloseChat = reinterpret_cast<void*>(base + 0x68E10);
            break;
        case SampVersion::V037R5:
            addrOpenChat = reinterpret_cast<void*>(base + 0x69480);
            addrCloseChat = reinterpret_cast<void*>(base + 0x69580);
            break;
        case SampVersion::V03DLR1:
            addrOpenChat = reinterpret_cast<void*>(base + 0x68EC0);
            addrCloseChat = reinterpret_cast<void*>(base + 0x68FC0);
            break;
        default:
            break;
    }

    if (!addrOpenChat)
    {
        LOG_FATAL("[ChatHook] Unsupported SA:MP version for OpenChatInput.");
        return false;
    }

    if (!hooks_.Install("ChatHook::OpenChatInput", addrOpenChat, reinterpret_cast<void*>(&Hook_OpenChatInput)))
    {
        LOG_ERROR("[ChatHook] Failed to install OpenChatInput hook.");
        return false;
    }

    if (!hooks_.Install("ChatHook::CloseChatInput", addrCloseChat, reinterpret_cast<void*>(&Hook_CloseChatInput)))
    {
        LOG_ERROR("[ChatHook] Failed to install CloseChatInput hook.");
        return false;
    }

    s_orig_open_ = reinterpret_cast<FnOpenChatInput>(hooks_.GetOriginal("ChatHook::OpenChatInput"));
    if (!s_orig_open_)
    {
        LOG_FATAL("[ChatHook] Failed to get original function pointer.");
        hooks_.Uninstall("ChatHook::OpenChatInput");
        return false;
    }

    s_orig_close_ = (FnCloseChatInput)hooks_.GetOriginal("ChatHook::CloseChatInput");
    if (!s_orig_close_)
    {
        LOG_FATAL("[ChatHook] Failed to get original function pointer.");
        hooks_.Uninstall("ChatHook::CloseChatInput");
        return false;
    }

    LOG_DEBUG("[ChatHook] OpenChatInput hook installed.");
    LOG_DEBUG("[ChatHook] CloseChatInput hook installed.");
    return true;
}

void ChatHook::Shutdown()
{
    hooks_.Uninstall("ChatHook::OpenChatInput");
    hooks_.Uninstall("ChatHook::CloseChatInput");
    s_orig_open_ = nullptr;
    s_orig_close_ = nullptr;
    s_self_ = nullptr;
}

void ChatHook::SetChatInputState(bool open)
{
    // Avoid redundant notifications
    if (focus_.IsChatInputOpen() == open)
        return;

    focus_.SetChatInputOpen(open);

    // Local JS event to all browsers
    for (const auto& kv : browser_.GetAllBrowsers())
    {
        const int id = kv.first;
        auto* inst = browser_.GetBrowserInstance(id);
        if (!inst || !inst->browser)
            continue;

        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("emit_event");
        CefRefPtr<CefListValue> list = msg->GetArgumentList();
        list->SetString(0, EnsureUtf8ForCef("omp:cef:internal:chatInputState"));
        list->SetBool(1, open);
        inst->browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, msg);
    }

    // Server callback
    ClientEmitEventPacket ev;
    ev.browserId = -1;
    ev.name = CefEvent::Client::ChatInputState;
    ev.args.emplace_back(open);
    network_.SendPacket(PacketType::ClientEmitEvent, ev);
}

void __fastcall ChatHook::Hook_OpenChatInput(void* pThis, void* _edx)
{
    auto* self = s_self_;
    if (!self)
        return;

    if (self->focus_.ShouldBlockChat())
        return;

    if (s_orig_open_)
        s_orig_open_(pThis, _edx);

    self->SetChatInputState(true);
}

void __fastcall ChatHook::Hook_CloseChatInput(void* pThis, void* _edx)
{
    auto* self = s_self_;
    if (!self)
        return;

    if (s_orig_close_)
        s_orig_close_(pThis, _edx);

    self->SetChatInputState(false);
}