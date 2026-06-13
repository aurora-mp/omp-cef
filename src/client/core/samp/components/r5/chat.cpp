#include "chat.hpp"

#include <cstring>

sampapi::v037r5::CChat* ChatView_R5::GetChat() const
{
    return sampapi::v037r5::RefChat();
}

void ChatView_R5::Clear()
{
    auto* chat = GetChat();
    if (!chat)
        return;

    for (int i = 0; i < sampapi::v037r5::CChat::MAX_MESSAGES; ++i)
    {
        std::memset(&chat->m_entry[i], 0, sizeof(chat->m_entry[i]));
    }

    if (chat->m_szLastMessage)
        chat->m_szLastMessage[0] = '\0';

    chat->m_nScrollbarPos = 0;
    chat->m_bRedraw = TRUE;

    chat->UpdateScrollbar();
    chat->ScrollToBottom();
}
