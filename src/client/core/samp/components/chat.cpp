#include "chat.hpp"
#include "component_registry.hpp"

#include "samp/addresses.hpp"

#include "r1/chat.hpp"
#include "r3/chat.hpp"
#include "r5/chat.hpp"
#include "dl/chat.hpp"

REGISTER_SAMP_COMPONENT(ChatComponent);

void ChatComponent::Initialize()
{
    auto version = SampAddresses::Instance().Version();
    switch (version)
    {
        case SampVersion::V037:
            view_ = std::make_unique<ChatView_R1>();
            break;
        case SampVersion::V037R3:
            view_ = std::make_unique<ChatView_R3>();
            break;
        case SampVersion::V037R5:
            view_ = std::make_unique<ChatView_R5>();
            break;
        case SampVersion::V03DLR1:
            view_ = std::make_unique<ChatView_DL>();
            break;
        default:
            view_ = nullptr;
            break;
    }
}

void ChatComponent::Clear()
{
    if (!view_)
        return;

    view_->Clear();
}