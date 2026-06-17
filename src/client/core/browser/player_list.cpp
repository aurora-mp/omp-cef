#include "player_list.hpp"

namespace
{
    bool IsAltDown()
    {
        return (::GetKeyState(VK_MENU) & 0x8000) != 0;
    }

    bool IsTabMessage(UINT message, WPARAM w_param)
    {
        return (message == WM_KEYDOWN || message == WM_KEYUP || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)
            && ((static_cast<int>(w_param) & 0xFF) == VK_TAB);
    }

    bool IsKeyDownMessage(UINT message)
    {
        return message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    }

    bool IsAutoRepeat(LPARAM l_param)
    {
        return (((static_cast<std::uint32_t>(l_param) >> 30) & 1u) != 0u);
    }
}

const char* PlayerListController::GetModeName(PlayerListMode mode)
{
    switch (mode)
    {
        case PlayerListMode::Default:
            return "default";
        case PlayerListMode::Disabled:
            return "disabled";
        case PlayerListMode::Custom:
            return "custom";
    }

    return "unknown";
}

PlayerListMode PlayerListController::NormalizeMode(PlayerListMode mode)
{
    switch (mode)
    {
        case PlayerListMode::Default:
        case PlayerListMode::Disabled:
        case PlayerListMode::Custom:
            return mode;
    }

    return PlayerListMode::Default;
}

bool PlayerListController::ShouldSuppressNativePlayerList(PlayerListMode mode)
{
    return mode == PlayerListMode::Disabled || mode == PlayerListMode::Custom;
}

PlayerListMode PlayerListController::SetMode(PlayerListMode mode)
{
    mode = NormalizeMode(mode);

    const PlayerListMode previous = mode_.exchange(mode, std::memory_order_acq_rel);

    input_down_.store(false, std::memory_order_release);

    if (mode != PlayerListMode::Custom)
        SetCustomPlayerListOpen(false);

    return previous;
}

PlayerListMode PlayerListController::GetMode() const
{
    return mode_.load(std::memory_order_acquire);
}

bool PlayerListController::HandleNativePlayerListOpenRequest(bool allow_custom_player_list)
{
    const PlayerListMode mode = GetMode();
    if (!ShouldSuppressNativePlayerList(mode))
        return false;

    if (mode == PlayerListMode::Custom && allow_custom_player_list)
    {
        ProcessInputState(true);
    }
    else
    {
        input_down_.store(true, std::memory_order_release);
    }

    return true;
}

bool PlayerListController::ConsumeWndProcMessage(UINT message, WPARAM w_param, LPARAM l_param)
{
    if (!ShouldSuppressNativePlayerList(GetMode()))
        return false;

    // Never consume Alt+Tab. This must remain an OS/window-manager shortcut.
    if (IsAltDown())
        return false;

    if (IsTabMessage(message, w_param))
    {
        const bool down = IsKeyDownMessage(message);
        const bool repeat = down ? IsAutoRepeat(l_param) : false;

        if (!(down && repeat))
            ProcessInputState(down);

        return true;
    }

    if (message == WM_CHAR && w_param == '\t')
        return true;

    return false;
}

bool PlayerListController::IsCustomPlayerListOpen() const
{
    return custom_player_list_open_.load(std::memory_order_acquire);
}

bool PlayerListController::HasPendingCustomPlayerListVisibilityChange()
{
    return custom_player_list_visibility_changed_.exchange(false, std::memory_order_acq_rel);
}

bool PlayerListController::ProcessInputState(bool is_down)
{
    const PlayerListMode mode = GetMode();

    if (mode == PlayerListMode::Default)
    {
        input_down_.store(is_down, std::memory_order_release);
        return false;
    }

    const bool was_down = input_down_.exchange(is_down, std::memory_order_acq_rel);

    if (mode == PlayerListMode::Custom && was_down != is_down)
        SetCustomPlayerListOpen(is_down);

    if (mode == PlayerListMode::Disabled && was_down && !is_down)
        SetCustomPlayerListOpen(false);

    return true;
}

void PlayerListController::SetCustomPlayerListOpen(bool open)
{
    const bool previous = custom_player_list_open_.exchange(open, std::memory_order_acq_rel);
    if (previous != open)
        custom_player_list_visibility_changed_.store(true, std::memory_order_release);
}
