#include "resource_manager.hpp"
#include "logger.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include <miniz.h>

#include <shared/crypto.hpp>
#include <shared/utils.hpp>

namespace
{
    constexpr int RESOURCE_MANIFEST_VERSION = 2;
    constexpr uint64_t MAX_FILE_SIZE = 20 * 1024 * 1024;

    struct ResourceFileToPack
    {
        std::filesystem::path diskPath;
        std::string internalPath;
        uint64_t size{};
        int64_t modified{};
        bool encrypted{};
    };

    int64_t GetLastWriteTimeSeconds(const std::filesystem::directory_entry& entry)
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            entry.last_write_time().time_since_epoch()
        ).count();
    }

    std::string ToLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    bool IsAllowedExtension(const std::string& extension)
    {
        static const std::set<std::string> allowed_extensions =
        {
            ".html",
            ".css",
            ".js",
            ".json",
            ".xml",
            ".png",
            ".jpg",
            ".jpeg",
            ".gif",
            ".svg",
            ".ico",
            ".mp3",
            ".ogg",
            ".wav",
            ".woff",
            ".woff2",
            ".ttf",
            ".otf",
            ".eot"
        };

        return allowed_extensions.find(extension) != allowed_extensions.end();
    }

    bool ShouldEncryptExtension(const std::string& extension)
    {
        static const std::set<std::string> encrypted_extensions =
        {
            ".html",
            ".css",
            ".js",
            ".json",
            ".xml",
            ".svg"
        };

        return encrypted_extensions.find(extension) != encrypted_extensions.end();
    }

    bool ManifestMetadataMatches(const nlohmann::json& manifestFiles, const ResourceFileToPack& file)
    {
        if (!manifestFiles.is_object() || !manifestFiles.contains(file.internalPath))
            return false;

        const auto& previous = manifestFiles[file.internalPath];
        if (!previous.is_object())
            return false;

        return previous.value("size", uint64_t{}) == file.size &&
            previous.value("modified", int64_t{}) == file.modified &&
            previous.value("encrypted", false) == file.encrypted;
    }

    std::vector<uint8_t> ReadFileToMemory(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return {};
        }

        const std::streamsize size = file.tellg();
        if (size < 0)
        {
            return {};
        }

        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(static_cast<size_t>(size));
        if (!data.empty() && !file.read(reinterpret_cast<char*>(data.data()), size))
        {
            return {};
        }

        return data;
    }

    long long ElapsedMs(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
}

void ResourceManager::AddResource(const std::string& resourceName, const std::vector<uint8_t>& master_key)
{
    if (master_key.empty())
    {
        LOG_ERROR("[ResourceManager] Cannot add resource '%s' because Master Resource Key is not set.", resourceName.c_str());
        return;
    }

    this->ProcessResourceDirectory(resourceName, master_key);
}

bool ResourceManager::GetPakContent(const std::string& resourceName, std::vector<uint8_t>& outContent) const
{
    std::string pakPath = "scriptfiles/cef/" + resourceName + ".pak";
    if (!std::filesystem::exists(pakPath))
    {
        LOG_ERROR("[ResourceManager] GetPakContent failed: File not found at %s.", pakPath.c_str());
        return false;
    }

    try
    {
        std::ifstream file(pakPath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            LOG_ERROR("[ResourceManager] GetPakContent failed: Could not open file at %s.", pakPath.c_str());
            return false;
        }

        std::streamsize size = file.tellg();
        if (size < 0)
            return false;

        file.seekg(0, std::ios::beg);

        outContent.resize(static_cast<size_t>(size));
        if (outContent.empty() || file.read(reinterpret_cast<char*>(outContent.data()), size))
        {
            LOG_DEBUG("[ResourceManager] Successfully read %lld bytes from %s.", static_cast<long long>(size), pakPath.c_str());
            return true;
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("[ResourceManager] GetPakContent exception while reading %s: %s", pakPath.c_str(), e.what());
    }

    return false;
}

bool ResourceManager::GetPakInfo(const std::string& resourceName, FileInfo& outPakInfo) const
{
    std::lock_guard<std::mutex> lock(resource_mutex_);

    auto it = registered_resources_.find(resourceName);
    if (it == registered_resources_.end() || it->second.files.empty())
        return false;

    outPakInfo = it->second.files.front();
    return true;
}

nlohmann::json ResourceManager::ReadManifest(const std::string& manifestPath)
{
    if (!std::filesystem::exists(manifestPath))
    {
        return nullptr;
    }

    std::ifstream f(manifestPath);
    try
    {
        return nlohmann::json::parse(f);
    }
    catch (...)
    {
        return nullptr;
    }
}

void ResourceManager::WriteManifest(const std::string& manifestPath, const nlohmann::json& data)
{
    try
    {
        std::string content = data.dump(4);
        std::ofstream o(manifestPath, std::ios::binary | std::ios::trunc);
        o.write(content.c_str(), static_cast<std::streamsize>(content.size()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("[ResourceManager] Failed to write manifest file '%s': %s", manifestPath.c_str(), e.what());
    }
}

nlohmann::json ResourceManager::GetManifestAsJson()
{
    nlohmann::json rootJson = nlohmann::json::object();
    std::lock_guard<std::mutex> lock(resource_mutex_);

    for (const auto& resource_pair : registered_resources_)
    {
        const std::string& resourceName = resource_pair.first;
        const Resource& resourceData = resource_pair.second;
        nlohmann::json filesArray = nlohmann::json::array();

        for (const auto& fileInfo : resourceData.files)
        {
            nlohmann::json fileJson =
            {
                {"path", fileInfo.relativePath},
                {"size", fileInfo.fileSize},
                {"hash", fileInfo.fileHash}
            };

            filesArray.push_back(fileJson);
        }

        rootJson[resourceName] = filesArray;
    }

    return rootJson;
}

bool ResourceManager::IsFileValid(const std::string& resourceName, const std::string& relativePath) const
{
    std::lock_guard<std::mutex> lock(resource_mutex_);

    auto it = registered_resources_.find(resourceName);
    if (it == registered_resources_.end())
        return false;

    const auto& files = it->second.files;
    if (files.size() == 1 && files[0].relativePath == relativePath)
        return true;

    return false;
}

bool ResourceManager::ProcessResourceDirectory(const std::string& resourceName, const std::vector<uint8_t>& encryption_key)
{
    try
    {
        if (resourceName.find("..") != std::string::npos)
        {
            LOG_WARN("[ResourceManager] Resource loading denied, potential path traversal in resource name: %s", resourceName.c_str());
            return false;
        }

        std::string basePath = "scriptfiles/cef/" + resourceName;
        if (!std::filesystem::is_directory(basePath))
        {
            LOG_ERROR("[ResourceManager] Resource directory not found: %s", basePath.c_str());
            return false;
        }

        std::string basePath_u8 = std::filesystem::path(basePath).generic_u8string();

        std::string pakPath = "scriptfiles/cef/" + resourceName + ".pak";
        std::string manifestPath = pakPath + ".manifest";
        nlohmann::json manifest_data = ReadManifest(manifestPath);
        nlohmann::json files_manifest = nlohmann::json::object();

        std::vector<ResourceFileToPack> files_to_pack;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath))
        {
            if (!entry.is_regular_file())
                continue;

            std::string extension = ToLowerCopy(entry.path().extension().string());

            if (entry.file_size() > MAX_FILE_SIZE || !IsAllowedExtension(extension))
            {
                continue;
            }

            std::string full_path_str = entry.path().generic_u8string();
            std::string internalPath = full_path_str.substr(basePath_u8.length() + 1);

            ResourceFileToPack file;
            file.diskPath = entry.path();
            file.internalPath = internalPath;
            file.size = entry.file_size();
            file.modified = GetLastWriteTimeSeconds(entry);
            file.encrypted = ShouldEncryptExtension(extension);

            files_manifest[internalPath] =
            {
                {"size", file.size},
                {"modified", file.modified},
                {"encrypted", file.encrypted}
            };

            files_to_pack.push_back(std::move(file));
        }

        std::sort(files_to_pack.begin(), files_to_pack.end(),
            [](const ResourceFileToPack& left, const ResourceFileToPack& right)
            {
                return left.internalPath < right.internalPath;
            });

        if (files_to_pack.empty())
        {
            LOG_WARN("[ResourceManager] No valid files found for resource '%s'.", resourceName.c_str());

            if (std::filesystem::exists(pakPath))
                std::filesystem::remove(pakPath);

            if (std::filesystem::exists(manifestPath))
                std::filesystem::remove(manifestPath);

            return false;
        }

        bool needs_recompilation = true;

        const bool has_valid_manifest =
            !manifest_data.is_null() &&
            manifest_data.is_object() &&
            manifest_data.value("version", 0) == RESOURCE_MANIFEST_VERSION &&
            manifest_data.contains("pak") &&
            manifest_data["pak"].is_object() &&
            manifest_data.contains("files") &&
            manifest_data["files"].is_object() &&
            std::filesystem::exists(pakPath);

        if (!has_valid_manifest)
        {
            LOG_DEBUG("[ResourceManager] No valid .pak or v%d manifest found for '%s', forcing recompilation.", RESOURCE_MANIFEST_VERSION, resourceName.c_str());
        }
        else
        {
            needs_recompilation = false;
            LOG_DEBUG("[ResourceManager] Validating cache for resource '%s'...", resourceName.c_str());

            const auto& manifestFiles = manifest_data["files"];
            if (manifestFiles.size() != files_to_pack.size())
            {
                needs_recompilation = true;
                LOG_DEBUG(
                    "[ResourceManager] File count mismatch for '%s' (old: %zu, new: %zu), recompilation needed.",
                    resourceName.c_str(),
                    manifestFiles.size(),
                    files_to_pack.size()
                );
            }
            else
            {
                for (const auto& file : files_to_pack)
                {
                    if (!ManifestMetadataMatches(manifestFiles, file))
                    {
                        needs_recompilation = true;
                        LOG_DEBUG("[ResourceManager] Change detected in '%s', recompilation needed.", file.internalPath.c_str());
                        break;
                    }
                }
            }
        }

        if (!needs_recompilation)
        {
            FileInfo pakInfo;
            pakInfo.relativePath = resourceName + ".pak";
            pakInfo.fileSize = manifest_data["pak"].value("size", std::filesystem::file_size(pakPath));
            pakInfo.fileHash = manifest_data["pak"].value("hash", std::string{});

            if (pakInfo.fileHash.empty())
            {
                needs_recompilation = true;
                LOG_DEBUG("[ResourceManager] Pak hash missing for '%s', recompilation needed.", resourceName.c_str());
            }
            else
            {
                Resource pakResource;
                pakResource.name = resourceName;
                pakResource.files.push_back(pakInfo);
                pakResource.totalSize = pakInfo.fileSize;

                {
                    std::lock_guard<std::mutex> lock(resource_mutex_);
                    registered_resources_[resourceName] = pakResource;
                }

                LOG_INFO("[ResourceManager] Resource '%s' is up-to-date. Loaded from cache.", resourceName.c_str());
                return true;
            }
        }

        const auto total_start = std::chrono::steady_clock::now();
        LOG_INFO("[ResourceManager] Packing and encrypting resource '%s' ...", resourceName.c_str());

        mz_zip_archive zip_archive = {};
        if (mz_zip_writer_init_file(&zip_archive, pakPath.c_str(), 0) == MZ_FALSE)
        {
            LOG_ERROR("[ResourceManager] Could not create pak file at %s.", pakPath.c_str());
            return false;
        }

        size_t encryptedCount = 0;
        size_t rawCount = 0;
        uint64_t totalSourceSize = 0;

        const auto entries_start = std::chrono::steady_clock::now();

        for (const auto& file : files_to_pack)
        {
            std::vector<uint8_t> content = ReadFileToMemory(file.diskPath);

            if (content.empty() && file.size > 0)
            {
                LOG_WARN("[ResourceManager] Failed to read '%s'.", file.diskPath.generic_u8string().c_str());
                continue;
            }

            std::vector<uint8_t> final_data = EncodeResourceEntry(content, encryption_key, file.encrypted);
            if (final_data.empty() && !content.empty())
            {
                LOG_WARN("[ResourceManager] Failed to encode '%s'.", file.internalPath.c_str());
                continue;
            }

            if (mz_zip_writer_add_mem(
                &zip_archive,
                file.internalPath.c_str(),
                reinterpret_cast<const char*>(final_data.data()),
                final_data.size(),
                MZ_NO_COMPRESSION) == MZ_FALSE)
            {
                LOG_WARN("[ResourceManager] Failed to add '%s' to pak.", file.internalPath.c_str());
                continue;
            }

            totalSourceSize += file.size;
            if (file.encrypted)
                ++encryptedCount;
            else
                ++rawCount;
        }

        const auto entries_end = std::chrono::steady_clock::now();

        const auto finalize_start = std::chrono::steady_clock::now();
        if (mz_zip_writer_finalize_archive(&zip_archive) == MZ_FALSE)
        {
            mz_zip_writer_end(&zip_archive);
            LOG_ERROR("[ResourceManager] Failed to finalize pak file at %s.", pakPath.c_str());
            return false;
        }
        mz_zip_writer_end(&zip_archive);
        const auto finalize_end = std::chrono::steady_clock::now();

        const auto pak_hash_start = std::chrono::steady_clock::now();
        FileInfo pakInfo;
        pakInfo.relativePath = resourceName + ".pak";
        pakInfo.fileSize = std::filesystem::file_size(pakPath);
        pakInfo.fileHash = CalculateSHA256(pakPath);
        const auto pak_hash_end = std::chrono::steady_clock::now();

        if (pakInfo.fileHash.empty())
        {
            LOG_ERROR("[ResourceManager] Failed to calculate pak hash for '%s'.", resourceName.c_str());
            return false;
        }

        nlohmann::json new_manifest_data =
        {
            {"version", RESOURCE_MANIFEST_VERSION},
            {"pak", {
                {"path", pakInfo.relativePath},
                {"size", pakInfo.fileSize},
                {"hash", pakInfo.fileHash}
            }},
            {"files", files_manifest}
        };

        WriteManifest(manifestPath, new_manifest_data);

        Resource pakResource;
        pakResource.name = resourceName;
        pakResource.files.push_back(pakInfo);
        pakResource.totalSize = pakInfo.fileSize;

        {
            std::lock_guard<std::mutex> lock(resource_mutex_);
            registered_resources_[resourceName] = pakResource;
        }

        const auto total_end = std::chrono::steady_clock::now();

        std::string formattedSourceSize = FormatBytes(totalSourceSize);
        std::string formattedPakSize = FormatBytes(pakResource.totalSize);
        LOG_INFO(
            "[ResourceManager] Resource '%s' successfully packed to '%s' (%zu files, %s source, %s pak, encrypted=%zu, raw=%zu, entries=%lld ms, finalize=%lld ms, pak_hash=%lld ms, total=%lld ms).",
            resourceName.c_str(),
            pakPath.c_str(),
            files_to_pack.size(),
            formattedSourceSize.c_str(),
            formattedPakSize.c_str(),
            encryptedCount,
            rawCount,
            ElapsedMs(entries_start, entries_end),
            ElapsedMs(finalize_start, finalize_end),
            ElapsedMs(pak_hash_start, pak_hash_end),
            ElapsedMs(total_start, total_end)
        );

        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("[ResourceManager] A critical error occurred while processing resource '%s': %s", resourceName.c_str(), e.what());
        return false;
    }
}
