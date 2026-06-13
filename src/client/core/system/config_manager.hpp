#pragma once

#include <nlohmann/json.hpp>
#include <string>

class ConfigManager
{
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool Load(const std::string& path);

    template <typename T>
    T Get(const std::string& key, T fallback = T()) const;

private:
    nlohmann::json data_;
};
