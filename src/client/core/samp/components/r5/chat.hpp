#pragma once

#include "samp/components/chat_view.hpp"

#include <sampapi/0.3.7-R5-1/CChat.h>

class ChatView_R5 : public IChatView
{
public:
    ChatView_R5() = default;
    ~ChatView_R5() override = default;

    void Clear() override;

private:
    sampapi::v037r5::CChat* GetChat() const;
};
