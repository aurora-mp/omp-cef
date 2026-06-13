#pragma once

#include <cstdint>
#include <string>
#include <string_view>

struct BrowserInstance;

class PlayerStats
{
public:
    struct Snapshot
    {
        int hp = 0;
        int max_hp = 0;
        int arm = 0;
        int breath = 0;

        int wanted = 0;
        int weapon = 0;
        int ammo = 0;
        int max_ammo = 0;
        int money = 0;

        float speed = 0.f;

        // World position
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;

        // Player heading
        float heading = 0.f;

        // Camera heading
        float camera_heading = 0.f;

        // Flags
        bool in_vehicle = false;
        bool aiming = false;

        // Vehicle data (valid when in_vehicle is true)
        int vehicle_model = 0;
        float vehicle_health = 0.f;
    };

    struct PollState
    {
        bool enabled = false;
        uint32_t intervalMs = 100;
        uint64_t nextTickMs = 0;
        uint64_t lastEmitMs = 0;

        bool hasLast = false;
        Snapshot last{};
    };

    static bool Equal(const Snapshot& a, const Snapshot& b) noexcept;
    static std::string ToJson(const Snapshot& s);
    static void EmitJson(BrowserInstance* instance, std::string_view eventName, std::string_view jsonPayload);
};