#include "game.hpp"
#include <samp/addresses.hpp>

GameView_DL::GameView_DL()
{
    auto* base = SampAddresses::Instance().Base();
    if (!base) {
        LOG_ERROR("[GameView_DL] Failed to get samp.dll base address, initialization aborted.");
        return;
    }

    c_game_ptr_ = *reinterpret_cast<void**>(base + 0x2ACA3C);
    set_cursor_fn_ = base + 0xA0530;
    process_input_enable_fn_ = base + 0xA0410;
}

void GameView_DL::SetCursorMode(int mode, bool hideCursorImmediately)
{
    if (!set_cursor_fn_ || !c_game_ptr_)
        return;

    using Fn = void(__thiscall*)(void* thisptr, int mode, int hide);
    auto func = reinterpret_cast<Fn>(set_cursor_fn_);
    func(c_game_ptr_, mode, hideCursorImmediately);
}

void GameView_DL::ProcessInputEnabling()
{
    if (!process_input_enable_fn_ || !c_game_ptr_)
        return;

    using Fn = void(__thiscall*)(void* thisptr);
    auto func = reinterpret_cast<Fn>(process_input_enable_fn_);
    func(c_game_ptr_);
}