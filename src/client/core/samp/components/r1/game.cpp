#include "game.hpp"

#include <samp/addresses.hpp>

GameView_R1::GameView_R1()
{
    auto* base = SampAddresses::Instance().Base();

    c_game_ptr_ = *reinterpret_cast<void**>(base + 0x21A10C);
    set_cursor_fn_ = base + 0x9BD30;
    process_input_enable_fn_ = base + 0x9BC10;
}

void GameView_R1::SetCursorMode(int mode, bool hideCursorImmediately)
{
    if (!set_cursor_fn_ || !c_game_ptr_)
        return;

    using Fn = void(__thiscall*)(void*, int, int);
    auto func = reinterpret_cast<Fn>(set_cursor_fn_);
    func(c_game_ptr_, mode, hideCursorImmediately);
}

void GameView_R1::ProcessInputEnabling()
{
    if (!process_input_enable_fn_ || !c_game_ptr_)
        return;

    using Fn = void(__thiscall*)(void*);
    auto func = reinterpret_cast<Fn>(process_input_enable_fn_);
    func(c_game_ptr_);
}