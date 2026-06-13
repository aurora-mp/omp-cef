#include "config_cfg.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

static bool StartsWith(const std::string& s, const std::string& prefix)
{
    return s.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), s.begin());
}

std::string ConfigCfg::Trim(std::string s)
{
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };

    s.erase(s.begin(),
        std::find_if(s.begin(), s.end(), [&](unsigned char c) { return !is_space(c); }));

    s.erase(
        std::find_if(s.rbegin(), s.rend(), [&](unsigned char c) { return !is_space(c); }).base(),
        s.end());

    return s;
}

std::string ConfigCfg::ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool ConfigCfg::ParseLine(const std::string& raw, std::string& outKey, std::string& outValue)
{
    std::string line = Trim(raw);
    if (line.empty())
        return false;

    if (StartsWith(line, "#") || StartsWith(line, ";") || StartsWith(line, "//"))
        return false;

    auto cutPos = line.find("//");
    if (cutPos != std::string::npos) line = line.substr(0, cutPos);
    cutPos = line.find('#');
    if (cutPos != std::string::npos) line = line.substr(0, cutPos);
    cutPos = line.find(';');
    if (cutPos != std::string::npos) line = line.substr(0, cutPos);

    line = Trim(line);
    if (line.empty())
        return false;

    std::istringstream iss(line);
    std::string key;
    if (!(iss >> key))
        return false;

    std::string value;
    std::getline(iss, value);
    value = Trim(value);

    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        value = value.substr(1, value.size() - 2);

    outKey = ToLower(Trim(key));
    outValue = Trim(value);
    return !outKey.empty();
}

bool ConfigCfg::Load(const std::string& path)
{
    values_.clear();

    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::string line;
    while (std::getline(file, line))
    {
        std::string key, value;
        if (ParseLine(line, key, value))
            values_[key] = value;
    }

    return true;
}

bool ConfigCfg::Has(const std::string& key) const
{
    const auto k = ToLower(key);
    return values_.find(k) != values_.end();
}

std::string ConfigCfg::GetString(const std::string& key, const std::string& defaultValue) const
{
    const auto k = ToLower(key);
    const auto it = values_.find(k);
    if (it == values_.end())
        return defaultValue;

    return it->second;
}

int ConfigCfg::GetInt(const std::string& key, int defaultValue) const
{
    const std::string s = GetString(key, "");
    if (s.empty())
        return defaultValue;

    try
    {
        const int v = std::stoi(s);
        return v;
    }
    catch (...)
    {
        return defaultValue;
    }
}

int ConfigCfg::GetPort(int defaultPort) const
{
    int p = GetInt("port", defaultPort);
    if (p < 1 || p > 65535)
        p = defaultPort;

    return p;
}

int ConfigCfg::GetCefUdpPort(int offset, int defaultCefPort) const
{
    const int serverPort = GetPort(7777);
    const int cefPort = serverPort + offset;

    if (cefPort < 1 || cefPort > 65535)
        return defaultCefPort;

    return cefPort;
}
