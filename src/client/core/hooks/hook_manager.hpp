#pragma once


#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>


class HookManager
{
public:
HookManager() = default;
~HookManager() = default;


HookManager(const HookManager&) = delete;
HookManager& operator=(const HookManager&) = delete;


bool Initialize();
void Shutdown();


bool Install(const std::string& name, void* target, void* detour);
bool Uninstall(const std::string& name);


bool Enable(const std::string& name);
bool Disable(const std::string& name);


void* GetOriginal(const std::string& name) const;


bool IsInstalled(std::string_view name) const;
bool IsEnabled(std::string_view name) const;


void DisableAll();
void EnableAll();


size_t GetHookCount() const;


private:
struct HookInfo
{
void* target = nullptr;
void* original = nullptr;
bool enabled = false;
};


mutable std::mutex lock_;
std::unordered_map<std::string, HookInfo> hooks_;
bool initialized_ = false;
};