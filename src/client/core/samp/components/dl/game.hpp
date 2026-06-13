#pragma once

#include "samp/components/game_view.hpp"

class GameView_DL : public IGameView 
{
public:
    GameView_DL();
    ~GameView_DL() override = default;

    void SetCursorMode(int mode, bool hideCursorImmediately) override;
    void ProcessInputEnabling() override;

private:
    void* c_game_ptr_ = nullptr;
    void* set_cursor_fn_ = nullptr;
    void* process_input_enable_fn_ = nullptr;
};