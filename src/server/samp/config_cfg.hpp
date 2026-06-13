#pragma once

#include <string>
#include <unordered_map>

class ConfigCfg
{
public:
    ConfigCfg() = default;

    bool Load(const std::string& path = "server.cfg");
    bool Has(const std::string& key) const;

    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
    int GetInt(const std::string& key, int defaultValue = 0) const;

    int GetPort(int defaultPort = 7777) const;
    int GetCefUdpPort(int offset = 2, int defaultCefPort = 7779) const;

private:
    static std::string Trim(std::string s);
    static std::string ToLower(std::string s);

    static bool ParseLine(const std::string& line, std::string& outKey, std::string& outValue);

private:
    std::unordered_map<std::string, std::string> values_;
};
