#pragma once

#include <functional>

class HookManager;
class SampAddresses;

class Samp
{
public:
    Samp(HookManager& hooks) : hooks_(hooks) {}
    ~Samp() = default;

    Samp(const Samp&) = delete;
    Samp& operator=(const Samp&) = delete;

    bool Initialize();
    // void SetControlsLocked(bool locked);

    void* GetOriginal(const char* name) const;
    void DisableHook(const char* name);

    std::function<void()> OnLoaded;
    std::function<void()> OnExit;

private:
    HookManager& hooks_;

    static void SampInitializationHook() noexcept;
    static void SampDeinitializationHook() noexcept;
};