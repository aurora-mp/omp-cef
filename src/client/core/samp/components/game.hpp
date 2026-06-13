#pragma once

#include "component.hpp"
#include "game_view.hpp"

class GameComponent : public ISampComponent 
{
public:
    void Initialize() override;

    void SetCursorMode(int mode, bool hideCursorImmediately);
    void ProcessInputEnabling();

private:
    std::unique_ptr<IGameView> view_;
};
