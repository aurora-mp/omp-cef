#pragma once

#include "samp/components/chat_view.hpp"

#include <sampapi/0.3.DL-1/CChat.h>

class ChatView_DL : public IChatView
{
public:
    ChatView_DL() = default;
    ~ChatView_DL() override = default;

    void Clear() override;

private:
    sampapi::v03dl::CChat* GetChat() const;
};
