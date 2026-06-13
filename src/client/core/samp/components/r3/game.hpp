#pragma once

#include "samp/components/game_view.hpp"

class GameView_R3 : public IGameView
{
public:
    GameView_R3();

    void SetCursorMode(int mode, bool hideCursorImmediately) override;
    void ProcessInputEnabling() override;

private:
    void* c_game_ptr_ = nullptr;
    void* set_cursor_fn_ = nullptr;
    void* process_input_enable_fn_ = nullptr;
};