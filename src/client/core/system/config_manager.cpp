#include "config_manager.hpp"
#include "system/logger.hpp"
#include <filesystem>
#include <fstream>
#include <type_traits>

bool ConfigManager::Load(const std::string& path)
{
    namespace fs = std::filesystem;

    if (!fs::exists(path)) {
        std::ofstream out(path);
        if (!out.is_open()) {
            LOG_ERROR("[ConfigManager] Could not create default config file at: {}", path);
            return false;
        }
        nlohmann::json def = { {"debug", false} };
        out << def.dump(4);
        out.close();
        LOG_INFO("[ConfigManager] Created default config file at: {}", path);
        data_ = std::move(def);
        return true;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("[ConfigManager] Could not open config file at: {}", path);
        return false;
    }

    try { file >> data_; }
    catch (const nlohmann::json::parse_error& e) { LOG_ERROR("[ConfigManager] Failed to parse config file: {}", e.what()); return false; }
    catch (...) { LOG_ERROR("[ConfigManager] Unknown error while parsing config file."); return false; }
    return true;
}

template <typename T>
T ConfigManager::Get(const std::string& key, T fallback) const
{
    if (!data_.contains(key)) return fallback;
    const auto& v = data_[key];

    if constexpr (std::is_same_v<T, bool>) {
        if (v.is_boolean()) return v.get<T>();
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (v.is_string()) return v.get<T>();
    } else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
        if (v.is_number()) return v.get<T>();
    }

    LOG_WARN("[ConfigManager] Key '{}' exists but has wrong type. Returning fallback.", key.c_str());
    return fallback;
}

// Explicit instantiations
template bool        ConfigManager::Get<bool>(const std::string&, bool) const;
template std::string ConfigManager::Get<std::string>(const std::string&, std::string) const;
template int         ConfigManager::Get<int>(const std::string&, int) const;
template float       ConfigManager::Get<float>(const std::string&, float) const;