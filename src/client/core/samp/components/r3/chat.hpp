#pragma once

#include "samp/components/chat_view.hpp"

#include <sampapi/0.3.7-R3-1/CChat.h>

class ChatView_R3 : public IChatView
{
public:
    ChatView_R3() = default;
    ~ChatView_R3() override = default;

    void Clear() override;

private:
    sampapi::v037r3::CChat* GetChat() const;
};
