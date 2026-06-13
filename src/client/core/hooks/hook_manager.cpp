#include "hook_manager.hpp"
#include "MinHook.h"
#include "system/logger.hpp"
#include <windows.h>

bool HookManager::Initialize()
{
    if (initialized_) return true;

    if (MH_Initialize() != MH_OK)
    {
        LOG_FATAL("Failed to initialize MinHook.");
        MessageBoxA(nullptr, "Failed to initialize hook manager.", "Fatal Error", MB_ICONERROR);
        ExitProcess(1);
    }

    initialized_ = true;
    LOG_DEBUG("HookManager (MinHook) initialized.");
    return true;
}

void HookManager::Shutdown()
{
    std::scoped_lock lock(lock_);
    if (!initialized_) return;

    for (auto& [name, hook] : hooks_) {
        if (hook.enabled) {
            const auto code = MH_DisableHook(hook.target);
            if (code != MH_OK) LOG_ERROR("Failed to disable hook '{}' (code={}).", name.c_str(), (int)code);
        }
        const auto rm = MH_RemoveHook(hook.target);
        if (rm != MH_OK) LOG_ERROR("Failed to remove hook '{}' (code={}).", name.c_str(), (int)rm);
    }

    hooks_.clear();

    const auto un = MH_Uninitialize();
    if (un != MH_OK) LOG_ERROR("Failed to uninitialize MinHook (code={}).", (int)un);

    initialized_ = false;
    LOG_DEBUG("HookManager shutdown completed.");
}

bool HookManager::Install(const std::string& name, void* target, void* detour)
{
    std::scoped_lock lock(lock_);

    if (hooks_.count(name)) {
        LOG_WARN("Hook '{}' is already installed.", name.c_str());
        return false;
    }

    void* original = nullptr;
    const auto create_status = MH_CreateHook(target, detour, &original);
    if (create_status != MH_OK) {
        LOG_ERROR("Failed to create hook '{}' (code={}).", name.c_str(), (int)create_status);
        return false;
    }
    const auto enable_status = MH_EnableHook(target);
    if (enable_status != MH_OK) {
        LOG_ERROR("Failed to enable hook '{}' (code={}).", name.c_str(), (int)enable_status);
        MH_RemoveHook(target);
        return false;
    }

    hooks_.emplace(name, HookInfo{ target, original, true });
    LOG_DEBUG("Hook '{}' installed successfully.", name.c_str());
    return true;
}

bool HookManager::Uninstall(const std::string& name)
{
    std::scoped_lock lock(lock_);
    auto it = hooks_.find(name);
    if (it == hooks_.end()) { LOG_WARN("Hook '{}' not found for uninstallation.", name.c_str()); return false; }

    if (it->second.enabled) {
        const auto dis = MH_DisableHook(it->second.target);
        if (dis != MH_OK) LOG_ERROR("Failed to disable hook '{}' (code={}).", name.c_str(), (int)dis);
    }

    const auto rm = MH_RemoveHook(it->second.target);
    if (rm != MH_OK) {
        LOG_ERROR("Failed to remove hook '{}' (code={}).", name.c_str(), (int)rm);
        return false;
    }

    hooks_.erase(it);
    LOG_DEBUG("Hook '{}' uninstalled.", name.c_str());
    return true;
}

bool HookManager::Enable(const std::string& name)
{
    std::scoped_lock lock(lock_);
    auto it = hooks_.find(name);
    if (it == hooks_.end()) { LOG_WARN("Hook '{}' not found.", name.c_str()); return false; }
    if (it->second.enabled) return true;
    const auto en = MH_EnableHook(it->second.target);
    if (en != MH_OK) { LOG_ERROR("Failed to enable hook '{}' (code={}).", name.c_str(), (int)en); return false; }
    it->second.enabled = true; LOG_DEBUG("Hook '{}' enabled.", name.c_str()); return true;
}

bool HookManager::Disable(const std::string& name)
{
    std::scoped_lock lock(lock_);
    auto it = hooks_.find(name);
    if (it == hooks_.end()) { LOG_WARN("Hook '{}' not found.", name.c_str()); return false; }
    if (!it->second.enabled) return true;
    const auto dis = MH_DisableHook(it->second.target);
    if (dis != MH_OK) { LOG_ERROR("Failed to disable hook '{}' (code={}).", name.c_str(), (int)dis); return false; }
    it->second.enabled = false; LOG_DEBUG("Hook '{}' disabled.", name.c_str()); return true;
}

void* HookManager::GetOriginal(const std::string& name) const
{
    std::scoped_lock lock(lock_);
    auto it = hooks_.find(name);
    return (it != hooks_.end()) ? it->second.original : nullptr;
}

bool HookManager::IsInstalled(std::string_view name) const
{
    std::scoped_lock lock(lock_);
    return hooks_.find(std::string(name)) != hooks_.end();
}

bool HookManager::IsEnabled(std::string_view name) const
{
    std::scoped_lock lock(lock_);
    auto it = hooks_.find(std::string(name));
    return (it != hooks_.end()) && it->second.enabled;
}

void HookManager::DisableAll()
{
    std::scoped_lock lock(lock_);
    for (auto& [name, hook] : hooks_) {
        if (hook.enabled) {
            const auto dis = MH_DisableHook(hook.target);
            if (dis != MH_OK) LOG_ERROR("Failed to disable hook '{}' (code={}).", name.c_str(), (int)dis);
            else hook.enabled = false;
        }
    }
    LOG_DEBUG("All hooks disabled.");
}

void HookManager::EnableAll()
{
    std::scoped_lock lock(lock_);
    for (auto& [name, hook] : hooks_) {
        if (!hook.enabled) {
            const auto en = MH_EnableHook(hook.target);
            if (en != MH_OK) LOG_ERROR("Failed to enable hook '{}' (code={}).", name.c_str(), (int)en);
            else hook.enabled = true;
        }
    }
    LOG_DEBUG("All hooks enabled.");
}

size_t HookManager::GetHookCount() const
{
    std::scoped_lock lock(lock_);
    return hooks_.size();
}
