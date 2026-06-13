#pragma once

#include "component.hpp"
#include "chat_view.hpp"

#include <memory>

class ChatComponent : public ISampComponent
{
public:
    void Initialize() override;

    void Clear();

private:
    std::unique_ptr<IChatView> view_;
};
