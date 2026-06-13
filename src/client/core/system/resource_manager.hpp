#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "shared/packet.hpp"

class Gta;
class NetworkManager;
class DownloadDialog;

// In-memory Virtual File System: maps an internal file path to its decrypted content
using VirtualFileSystem = std::map<std::string, std::vector<uint8_t>>;

enum class DownloadState
{
	IDLE,
	AWAITING_TRIGGER,
	VERIFYING_CACHE,
	DOWNLOADING,
	COMPLETED
};

class ResourceManager
{
public:
	ResourceManager(Gta& gta);
	~ResourceManager() = default;

	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;

	void SetNetworkManager(NetworkManager& net);
	void SetDownloadDialog(DownloadDialog& dialog);

	void Initialize();

	// Controls the internal CEF loader UI shown during resource downloads.
	// When false, downloads occur silently without opening the internal loader browser.
	void SetResourcesLoaderUiEnabled(bool enabled);
	bool IsResourcesLoaderUiEnabled() const { return resources_loader_ui_enabled_; }

	void OnConnect(const std::string& ip, uint16_t port);
	void OnDisconnect();

	void SetMasterKey(const std::vector<uint8_t>& key);

	void OnManifestReceived(const std::string& manifestJson);
	void MarkAsReadyToDownload();
	void TriggerDownload();

	void OnFileData(const FileDataPacket& packet);

	bool GetFileContent(const std::string& resourceName,
		const std::string& internalPath,
		std::vector<uint8_t>& outContent);

	DownloadState GetState() const { return state_; }
private:
	bool LoadPakIntoVFS(const std::string& resourceName, const std::string& pakPath);

	struct FileProgressData
	{
		std::string fileName;
		std::string fileHash;
		size_t totalSize = 0;
		size_t bytesReceived = 0;
		uint32_t totalChunks = 0;
		uint32_t receivedChunks = 0;
		bool isComplete = false;
	};

	struct FileAssemblyData
	{
		uint32_t totalChunks = 0;
		uint32_t receivedChunks = 0;
		std::vector<std::vector<uint8_t>> chunks;
	};

private:
	Gta& gta_;
	NetworkManager* net_ = nullptr;
	DownloadDialog* download_dialog_ = nullptr;
	bool resources_loader_ui_enabled_ = true;

	std::string base_cache_path_;
	std::string server_cache_path_;

	std::string server_ip_;
	std::vector<uint8_t> master_key_;

	std::atomic<DownloadState> state_{ DownloadState::IDLE };
	nlohmann::json server_manifest_;

	std::map<std::string, VirtualFileSystem> loaded_resources_vfs_;
	std::mutex vfs_mutex_;

	std::mutex download_mutex_;
	std::vector<FileProgressData> download_progress_;
	std::map<std::string, FileAssemblyData> assembling_files_;

	std::chrono::steady_clock::time_point last_packet_time_;
};
