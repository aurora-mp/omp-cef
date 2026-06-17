#include "escape_menu.hpp"

#include <game_sa/CMenuManager.h>
#include <game_sa/CPad.h>
#include <game_sa/CTimer.h>

namespace
{
    bool IsPadStartPressed(CPad* pad)
    {
        if (!pad)
            return false;

        return pad->NewState.Start != 0
            || pad->OldState.Start != 0
            || pad->PCTempKeyState.Start != 0
            || pad->PCTempJoyState.Start != 0
            || pad->PCTempMouseState.Start != 0;
    }

    void ClearPadStartState(CPad* pad)
    {
        if (!pad)
            return;

        pad->NewState.Start = 0;
        pad->OldState.Start = 0;
        pad->PCTempKeyState.Start = 0;
        pad->PCTempJoyState.Start = 0;
        pad->PCTempMouseState.Start = 0;
    }
}

const char* EscapeMenuController::GetModeName(EscapeMenuMode mode)
{
    switch (mode)
    {
        case EscapeMenuMode::Gta:
            return "gta";
        case EscapeMenuMode::Disabled:
            return "disabled";
        case EscapeMenuMode::Custom:
            return "custom";
    }

    return "unknown";
}

EscapeMenuMode EscapeMenuController::NormalizeMode(EscapeMenuMode mode)
{
    switch (mode)
    {
        case EscapeMenuMode::Gta:
        case EscapeMenuMode::Disabled:
        case EscapeMenuMode::Custom:
            return mode;
    }

    return EscapeMenuMode::Gta;
}

bool EscapeMenuController::ShouldSuppressNativeMenu(EscapeMenuMode mode)
{
    return mode == EscapeMenuMode::Disabled || mode == EscapeMenuMode::Custom;
}

EscapeMenuMode EscapeMenuController::SetMode(EscapeMenuMode mode)
{
    mode = NormalizeMode(mode);

    const EscapeMenuMode previous = mode_.exchange(mode, std::memory_order_acq_rel);

    if (mode != EscapeMenuMode::Custom)
        SetCustomMenuOpen(false);

    if (mode == EscapeMenuMode::Gta)
    {
        input_down_.store(false, std::memory_order_release);
    }
    else
    {
        SuppressNativeMenu();
    }

    return previous;
}

EscapeMenuMode EscapeMenuController::GetMode() const
{
    return mode_.load(std::memory_order_acquire);
}

bool EscapeMenuController::IsNativePauseMenuVisible() const
{
    return FrontEndMenuManager.m_bMenuActive && !FrontEndMenuManager.m_bDontDrawFrontEnd;
}

bool EscapeMenuController::UpdateInput()
{
    const EscapeMenuMode mode = GetMode();
    if (!ShouldSuppressNativeMenu(mode))
    {
        input_down_.store(false, std::memory_order_release);
        return false;
    }

    const bool escape_down = (::GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    const bool start_down = IsPadStartPressed(CPad::GetPad(0)) || IsPadStartPressed(CPad::GetPad(1));

    return ProcessInputState(escape_down || start_down);
}

bool EscapeMenuController::ConsumeWndProcMessage(UINT message, WPARAM w_param, LPARAM l_param)
{
    if (!ShouldSuppressNativeMenu(GetMode()))
        return false;

    if ((message == WM_KEYDOWN || message == WM_KEYUP || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)
        && ((static_cast<int>(w_param) & 0xFF) == VK_ESCAPE))
    {
        const bool down = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
        const bool repeat = down ? (((static_cast<std::uint32_t>(l_param) >> 30) & 1u) != 0u) : false;

        if (!(down && repeat))
        {
            ProcessInputState(down);
        }
        else
        {
            SuppressNativeMenu();
        }

        return true;
    }

    if (message == WM_CHAR && w_param == VK_ESCAPE)
    {
        SuppressNativeMenu();
        return true;
    }

    return false;
}

void EscapeMenuController::SuppressNativeMenu()
{
    if (!ShouldSuppressNativeMenu(GetMode()))
        return;

    ClearPadStartState(CPad::GetPad(0));
    ClearPadStartState(CPad::GetPad(1));

    FrontEndMenuManager.m_bActivateMenuNextFrame = false;

    if (FrontEndMenuManager.m_bMenuActive)
    {
        FrontEndMenuManager.m_bMenuActive = false;
        FrontEndMenuManager.m_bActivateMenuNextFrame = false;
        CTimer::EndUserPause();

        if (auto* pad = CPad::GetPad(0))
        {
            pad->JustOutOfFrontEnd = 1;
        }
    }
}

bool EscapeMenuController::IsCustomMenuOpen() const
{
    return custom_menu_open_.load(std::memory_order_acquire);
}

bool EscapeMenuController::HasPendingCustomMenuVisibilityChange()
{
    return custom_menu_visibility_changed_.exchange(false, std::memory_order_acq_rel);
}

bool EscapeMenuController::ProcessInputState(bool is_down)
{
    const EscapeMenuMode mode = GetMode();

    if (mode == EscapeMenuMode::Gta)
    {
        input_down_.store(is_down, std::memory_order_release);
        return false;
    }

    const bool was_down = input_down_.exchange(is_down, std::memory_order_acq_rel);
    const bool just_pressed = is_down && !was_down;

    SuppressNativeMenu();

    if (mode == EscapeMenuMode::Custom && just_pressed)
    {
        ToggleCustomMenu();
    }

    return true;
}

void EscapeMenuController::SetCustomMenuOpen(bool open)
{
    const bool previous = custom_menu_open_.exchange(open, std::memory_order_acq_rel);
    if (previous != open)
    {
        custom_menu_visibility_changed_.store(true, std::memory_order_release);
    }
}

void EscapeMenuController::ToggleCustomMenu()
{
    SetCustomMenuOpen(!IsCustomMenuOpen());
}
