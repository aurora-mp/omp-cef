#include "resource_manager.hpp"
#include "logger.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <shared/crypto.hpp>
#include <shared/utils.hpp>
#include <thread>
#include <miniz.h>

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
        file.seekg(0, std::ios::beg);

        outContent.resize(static_cast<size_t>(size));
        if (file.read(reinterpret_cast<char*>(outContent.data()), size))
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
        nlohmann::json new_manifest_data = nlohmann::json::object();

        bool needs_recompilation = false;

        if (manifest_data.is_null() || !std::filesystem::exists(pakPath))
        {
            needs_recompilation = true;
            LOG_DEBUG("[ResourceManager] No valid .pak or manifest found for '%s', forcing recompilation.", resourceName.c_str());
        }
        else
        {
            LOG_DEBUG("[ResourceManager] Validating cache for resource '%s'...", resourceName.c_str());
        }

        const uint64_t MAX_FILE_SIZE = 20 * 1024 * 1024;
        const std::set<std::string> ALLOWED_EXTENSIONS = 
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

        std::vector<std::pair<std::filesystem::path, std::string>> files_to_pack;
        size_t current_file_count = 0;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath))
        {
            if (!entry.is_regular_file())
                continue;

            std::string extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (entry.file_size() > MAX_FILE_SIZE || ALLOWED_EXTENSIONS.find(extension) == ALLOWED_EXTENSIONS.end())
            {
                continue;
            }

            current_file_count++;

            std::string full_path_str = entry.path().generic_u8string();
            std::string internalPath = full_path_str.substr(basePath_u8.length() + 1);

            files_to_pack.emplace_back(entry.path(), internalPath);

            auto last_write_time =
                std::chrono::duration_cast<std::chrono::seconds>(entry.last_write_time().time_since_epoch()).count();
            std::string file_hash = CalculateSHA256(entry.path().string());
            new_manifest_data[internalPath] = {{"hash", file_hash}, {"modified", last_write_time}};

            if (!needs_recompilation)
            {
                if (!manifest_data.contains(internalPath) || manifest_data[internalPath]["hash"] != file_hash ||
                    manifest_data[internalPath]["modified"] != last_write_time)
                {
                    needs_recompilation = true;
                    LOG_DEBUG("[ResourceManager] Change detected in '%s', recompilation needed.", internalPath.c_str());
                }
            }
        }

        if (!needs_recompilation && manifest_data.size() != current_file_count)
        {
            needs_recompilation = true;
            LOG_DEBUG("[ResourceManager] File count mismatch (old: %zu, new: %zu), recompilation needed.", manifest_data.size(), current_file_count);
        }

        if (!needs_recompilation)
        {
            FileInfo pakInfo;
            pakInfo.relativePath = resourceName + ".pak";
            pakInfo.fileSize = std::filesystem::file_size(pakPath);
            pakInfo.fileHash = CalculateSHA256(pakPath);

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

        LOG_INFO("[ResourceManager] Packing and encrypting resource '%s' with master key...", resourceName.c_str());

        mz_zip_archive zip_archive = {};
        if (mz_zip_writer_init_file(&zip_archive, pakPath.c_str(), 0) == MZ_FALSE)
        {
            LOG_ERROR("[ResourceManager] Could not create pak file at %s.", pakPath.c_str());
            return false;
        }

        for (const auto& file_pair : files_to_pack)
        {
            const auto& path = file_pair.first;
            const auto& internalPath = file_pair.second;

            std::ifstream file(path, std::ios::binary);
            if (!file.is_open())
                continue;

            std::vector<uint8_t> content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
            file.close();

            std::array<uint8_t, 16> iv;
            randombytes_buf(iv.data(), iv.size());

            std::vector<uint8_t> encrypted_content = EncryptFile(content, encryption_key, iv);

            std::vector<uint8_t> final_data;
            final_data.reserve(iv.size() + encrypted_content.size());
            final_data.insert(final_data.end(), iv.begin(), iv.end());
            final_data.insert(final_data.end(), encrypted_content.begin(), encrypted_content.end());

            if (mz_zip_writer_add_mem(&zip_archive, internalPath.c_str(), reinterpret_cast<const char*>(final_data.data()), final_data.size(), MZ_DEFAULT_COMPRESSION) == MZ_FALSE)
            {
                LOG_WARN("[ResourceManager] Failed to add '%s' to pak.", internalPath.c_str());
            }
        }

        mz_zip_writer_finalize_archive(&zip_archive);
        mz_zip_writer_end(&zip_archive);

        if (files_to_pack.empty())
        {
            LOG_WARN("[ResourceManager] No valid files found for resource '%s'. Pak file will be empty.", resourceName.c_str());

            std::filesystem::remove(pakPath);
            if (std::filesystem::exists(manifestPath))
                std::filesystem::remove(manifestPath);

            return false;
        }

        WriteManifest(manifestPath, new_manifest_data);

        FileInfo pakInfo;
        pakInfo.relativePath = resourceName + ".pak";
        pakInfo.fileSize = std::filesystem::file_size(pakPath);
        pakInfo.fileHash = CalculateSHA256(pakPath);

        Resource pakResource;
        pakResource.name = resourceName;
        pakResource.files.push_back(pakInfo);
        pakResource.totalSize = pakInfo.fileSize;

        {
            std::lock_guard<std::mutex> lock(resource_mutex_);
            registered_resources_[resourceName] = pakResource;
        }

        std::string formattedSize = FormatBytes(pakResource.totalSize);
        LOG_INFO("[ResourceManager] Resource '%s' successfully packed to '%s' (%zu files, %s).", resourceName.c_str(), pakPath.c_str(), files_to_pack.size(), formattedSize.c_str());

        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("[ResourceManager] A critical error occurred while processing resource '%s': %s", resourceName.c_str(), e.what());
        return false;
    }
}
