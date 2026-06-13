#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct FileInfo
{
    std::string relativePath;
    uint64_t fileSize{};
    std::string fileHash;
};

struct Resource
{
    std::string name;
    std::vector<FileInfo> files;
    uint64_t totalSize = 0;
};

class ResourceManager
{
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    void AddResource(const std::string& resourceName, const std::vector<uint8_t>& master_key);

    bool GetPakContent(const std::string& resourceName, std::vector<uint8_t>& outContent) const;
    nlohmann::json GetManifestAsJson();
    bool IsFileValid(const std::string& resourceName, const std::string& relativePath) const;

private:
    nlohmann::json ReadManifest(const std::string& manifestPath);
    void WriteManifest(const std::string& manifestPath, const nlohmann::json& data);

    bool ProcessResourceDirectory(const std::string& resourceName, const std::vector<uint8_t>& encryption_key);

private:
    std::map<std::string, Resource> registered_resources_;
    mutable std::mutex resource_mutex_;
};