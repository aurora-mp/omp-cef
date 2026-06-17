#pragma once

#include <atomic>
#include <cstdint>

#include <windows.h>

enum class PlayerListMode : std::uint8_t
{
    Default = 0,
    Disabled = 1,
    Custom = 2
};

class PlayerListController
{
public:
    static const char* GetModeName(PlayerListMode mode);
    static PlayerListMode NormalizeMode(PlayerListMode mode);
    static bool ShouldSuppressNativePlayerList(PlayerListMode mode);

    PlayerListMode SetMode(PlayerListMode mode);
    PlayerListMode GetMode() const;

    bool ConsumeWndProcMessage(UINT message, WPARAM w_param, LPARAM l_param);
    bool HandleNativePlayerListOpenRequest(bool allow_custom_player_list);

    bool IsCustomPlayerListOpen() const;
    bool HasPendingCustomPlayerListVisibilityChange();

private:
    bool ProcessInputState(bool is_down);
    void SetCustomPlayerListOpen(bool open);

    std::atomic<PlayerListMode> mode_{ PlayerListMode::Default };
    std::atomic<bool> input_down_{ false };
    std::atomic<bool> custom_player_list_open_{ false };
    std::atomic<bool> custom_player_list_visibility_changed_{ false };
};
