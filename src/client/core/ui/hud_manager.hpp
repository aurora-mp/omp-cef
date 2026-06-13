#pragma once

enum class EHudComponent 
{
    ALL = 0,
    AMMO,
    ARMOUR,
    BREATH,
    CROSSHAIR,
    HEALTH,
    MONEY,
    RADAR,
    WANTED_STARS,
    WEAPON
};

struct HudPatchInfo 
{
    uintptr_t address;
    std::vector<unsigned char> original_bytes;
    std::vector<unsigned char> disabled_bytes;
    bool original_saved = false;
};

class HudManager
{
public:
    HudManager() = default;
    ~HudManager() = default;

    HudManager(const HudManager&) = delete;
    HudManager& operator=(const HudManager&) = delete;

    void Initialize();
    void ToggleComponent(EHudComponent component, bool toggle);
    void SetClassSelectionVisible(bool visible);

private:
    void Patch(uintptr_t address, const std::vector<unsigned char>& data);

    std::unordered_map<EHudComponent, HudPatchInfo> patches_;
};