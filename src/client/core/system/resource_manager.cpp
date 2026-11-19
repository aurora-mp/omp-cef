#include "resource_manager.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string_view>

#include <miniz.h>

#include "gta.hpp"
#include "network/network_manager.hpp"
#include "system/logger.hpp"
#include "shared/crypto.hpp"
#include "shared/utils.hpp"
#include "ui/download_dialog.hpp"

namespace
{
	// Upper bound for a single resource pak (the game is a 32-bit process).
	constexpr size_t kMaxResourceFileSize = 512ull * 1024 * 1024;

	// The server currently sends 1200-byte chunks. This value only bounds the
	// chunk table so a hostile totalChunks cannot force a huge allocation.
	constexpr size_t kMinChunkBytes = 64;

	constexpr size_t kMaxResourcePathLength = 200;

	bool IsReservedDosName(const std::string& component)
	{
		std::string stem = component.substr(0, component.find('.'));
		for (auto& c : stem)
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

		if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL")
			return true;

		if (stem.size() == 4 && (stem.rfind("COM", 0) == 0 || stem.rfind("LPT", 0) == 0) &&
			stem[3] >= '1' && stem[3] <= '9')
			return true;

		return false;
	}

	bool IsSafePathComponent(const std::string& component)
	{
		if (component.empty() || component == "." || component == "..")
			return false;

		// Windows silently strips trailing dots and spaces.
		if (component.back() == '.' || component.back() == ' ')
			return false;

		for (const char ch : component)
		{
			const auto c = static_cast<unsigned char>(ch);
			if (c < 0x20 || c == 0x7F)
				return false;

			switch (ch)
			{
				case '<': case '>': case ':': case '"': case '|': case '?': case '*':
					return false;
				default:
					break;
			}
		}

		return !IsReservedDosName(component);
	}

	// Paths in the manifest and in FileData packets come from the server, which the
	// client does not trust. Only accept plain relative paths to a ".pak" file.
	bool IsSafeResourcePath(const std::string& path)
	{
		if (path.empty() || path.size() > kMaxResourcePathLength)
			return false;

		if (path.front() == '/' || path.front() == '\\')
			return false;

		size_t start = 0;
		while (true)
		{
			const size_t end = path.find_first_of("/\\", start);
			if (!IsSafePathComponent(path.substr(start, end == std::string::npos ? std::string::npos : end - start)))
				return false;

			if (end == std::string::npos)
				break;

			start = end + 1;
		}

		constexpr std::string_view extension = ".pak";
		if (path.size() <= extension.size())
			return false;

		for (size_t i = 0; i < extension.size(); ++i)
		{
			const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(path[path.size() - extension.size() + i])));
			if (c != extension[i])
				return false;
		}

		return true;
	}

	// Resolves relativePath under baseDir and checks that the result stays inside it.
	bool ResolveCachePath(const std::string& baseDir, const std::string& relativePath, std::filesystem::path& out)
	{
		if (baseDir.empty() || !IsSafeResourcePath(relativePath))
			return false;

		std::filesystem::path base = std::filesystem::path(baseDir).lexically_normal();
		if (!base.has_filename())
			base = base.parent_path();

		const std::filesystem::path candidate = (base / std::filesystem::path(relativePath)).lexically_normal();
		const std::filesystem::path relative = candidate.lexically_relative(base);

		if (relative.empty() || relative.is_absolute() || *relative.begin() == "..")
			return false;

		out = candidate;
		return true;
	}
}

ResourceManager::ResourceManager(Gta& gta) : gta_(gta) {}

std::string ResourceManager::MakeFileKey(const std::string& resourceName, const std::string& relativePath)
{
	return resourceName + "/" + relativePath;
}


void ResourceManager::SetNetworkManager(NetworkManager& net)
{
	net_ = &net;
}

void ResourceManager::SetDownloadDialog(DownloadDialog& dialog)
{
    download_dialog_ = &dialog;
}

void ResourceManager::Initialize()
{
	base_cache_path_ = gta_.GetUserFilesPath() + "/cef/cache/";
	std::filesystem::create_directories(base_cache_path_);
}

void ResourceManager::SetResourcesLoaderUiEnabled(bool enabled)
{
	resources_loader_ui_enabled_ = enabled;

	if (download_dialog_)
		download_dialog_->SetEnabled(enabled);
}

void ResourceManager::OnConnect(const std::string& ip, uint16_t port)
{
	server_ip_ = ip + "_" + std::to_string(port);
	server_cache_path_ = base_cache_path_ + "/" + server_ip_ + "/";

	std::filesystem::create_directories(server_cache_path_);

	LOG_INFO("[ResourceManager] Connected to server, cache path: {}", server_cache_path_);
}

void ResourceManager::OnDisconnect()
{
	std::lock_guard<std::mutex> lock(download_mutex_);

	state_ = DownloadState::IDLE;
	server_manifest_ = nlohmann::json{};
	download_progress_.clear();
	assembling_files_.clear();
	expected_files_.clear();
}

void ResourceManager::SetMasterKey(const std::vector<uint8_t>& key)
{
	master_key_ = key;
}

void ResourceManager::OnManifestReceived(const std::string& manifestJson)
{
	try {
		server_manifest_ = nlohmann::json::parse(manifestJson);
		LOG_DEBUG("[ResourceManager] Manifest received with {} resources", server_manifest_.size());
	}
	catch (const std::exception& e) {
		LOG_ERROR("[ResourceManager] Failed to parse manifest: {}", e.what());
	}
}

void ResourceManager::MarkAsReadyToDownload()
{
	DownloadState expected = DownloadState::IDLE;
	if (state_.compare_exchange_strong(expected, DownloadState::AWAITING_TRIGGER)) {
		LOG_INFO("[ResourceManager] Ready to download.");
	}
}

void ResourceManager::TriggerDownload()
{
	DownloadState expected = DownloadState::AWAITING_TRIGGER;
	if (!state_.compare_exchange_strong(expected, DownloadState::VERIFYING_CACHE)) {
		return;
	}

	if (!net_) {
		state_ = DownloadState::AWAITING_TRIGGER;
		return;
	}

	if (net_->GetState() != ConnectionState::CONNECTED) {
		state_ = DownloadState::AWAITING_TRIGGER;
		return;
	}

	if (master_key_.empty()) {
		LOG_WARN("[ResourceManager] Master key not yet received, waiting...");
		state_ = DownloadState::AWAITING_TRIGGER;
		return;
	}

	state_ = DownloadState::VERIFYING_CACHE;
	LOG_INFO("[ResourceManager] Verifying cache...");

	std::vector<std::pair<std::string, std::string>> files_to_request;
	std::vector<FileProgressData> progress_list;
	std::map<std::string, ExpectedFile> expected_files;

	const nlohmann::json manifest = server_manifest_.is_object() ? server_manifest_ : nlohmann::json::object();

	for (const auto& [resourceName, files] : manifest.items()) {
		if (!files.is_array()) {
			LOG_ERROR("[ResourceManager] Ignoring resource '{}': invalid manifest entry", resourceName);
			continue;
		}

		for (const auto& file_entry : files) {
			if (!file_entry.is_object() ||
				!file_entry.contains("path") || !file_entry["path"].is_string() ||
				!file_entry.contains("hash") || !file_entry["hash"].is_string() ||
				!file_entry.contains("size") || !file_entry["size"].is_number_unsigned())
			{
				LOG_ERROR("[ResourceManager] Ignoring malformed file entry in resource '{}'", resourceName);
				continue;
			}

			const std::string path = file_entry["path"].get<std::string>();
			const std::string server_hash = file_entry["hash"].get<std::string>();
			const uint64_t server_size = file_entry["size"].get<uint64_t>();

			std::filesystem::path local_path;
			if (!ResolveCachePath(server_cache_path_, path, local_path)) {
				LOG_ERROR("[ResourceManager] Refusing unsafe resource path '{}' in resource '{}'", path, resourceName);
				continue;
			}

			if (server_size == 0 || server_size > kMaxResourceFileSize) {
				LOG_ERROR("[ResourceManager] Refusing resource file '{}' with invalid size {}", path, server_size);
				continue;
			}

			std::error_code ec;
			if (std::filesystem::is_regular_file(local_path, ec)) {
				const std::string local_hash = CalculateSHA256(local_path.string());

				if (local_hash == server_hash && LoadPakIntoVFS(resourceName, local_path.string())) {
					continue;
				}
			}

			ExpectedFile expected;
			expected.resourceName = resourceName;
			expected.relativePath = path;
			expected.hash = server_hash;
			expected.size = static_cast<size_t>(server_size);
			expected.savePath = local_path;
			expected_files[MakeFileKey(resourceName, path)] = std::move(expected);

			files_to_request.push_back({ resourceName, path });
			FileProgressData progress;
			progress.fileName = path;
			progress.fileHash = server_hash;
			progress.totalSize = static_cast<size_t>(server_size);
			progress.isComplete = false;
			progress_list.push_back(std::move(progress));
		}
	}

	{
		std::lock_guard<std::mutex> lock(download_mutex_);
		download_progress_ = std::move(progress_list);
		expected_files_ = std::move(expected_files);
		assembling_files_.clear();
	}

	if (!download_progress_.empty()) {
		state_ = DownloadState::DOWNLOADING;
		last_packet_time_ = std::chrono::steady_clock::now();

		std::vector<std::pair<std::string, size_t>> dialog_files;
		for (const auto& p : download_progress_) {
			dialog_files.emplace_back(p.fileName, p.totalSize);
		}

		LOG_DEBUG("[ResourceManager] Starting download for {} file(s):", download_progress_.size());
		download_dialog_->Start(dialog_files);

		RequestFilesPacket request_packet;
		request_packet.files = std::move(files_to_request);
		net_->SendPacket(PacketType::RequestFiles, request_packet);
	}
	else {
		LOG_INFO("[ResourceManager] All local resources are up-to-date.");
		state_ = DownloadState::COMPLETED;
	}
}

void ResourceManager::OnFileData(const FileDataPacket& packet)
{
	if (state_ != DownloadState::DOWNLOADING) {
		LOG_WARN("[ResourceManager] Received FileData in wrong state: {}", static_cast<int>(state_.load()));
		return;
	}

	last_packet_time_ = std::chrono::steady_clock::now();

	std::lock_guard<std::mutex> lock(download_mutex_);

	const std::string fileKey = MakeFileKey(packet.resourceName, packet.relativePath);

	// Only accept data for files this client requested from the manifest.
	// Everything else (unknown resource, other path, traversal attempts) is dropped.
	const auto expected_it = expected_files_.find(fileKey);
	if (expected_it == expected_files_.end())
	{
		LOG_ERROR("[ResourceManager] Dropping FileData for unrequested file '{}'", fileKey);
		return;
	}

	const ExpectedFile& expected = expected_it->second;

	const size_t max_chunks = expected.size / kMinChunkBytes + 1;
	if (packet.totalChunks == 0 || packet.totalChunks > max_chunks || packet.data.empty())
	{
		LOG_ERROR("[ResourceManager] Invalid chunk layout for '{}' (totalChunks={}, size={})", fileKey, packet.totalChunks, packet.data.size());
		return;
	}

	auto& assembly = assembling_files_[fileKey];

	if (assembly.totalChunks == 0)
	{
		assembly.totalChunks = packet.totalChunks;
		assembly.chunks.resize(packet.totalChunks);
	}

	if (packet.totalChunks != assembly.totalChunks)
	{
		LOG_ERROR("[ResourceManager] Chunk count mismatch for '{}': expected {}, got {}", packet.relativePath, assembly.totalChunks, packet.totalChunks);
		return;
	}

	if (packet.chunkIndex >= assembly.totalChunks)
	{
		LOG_ERROR("[ResourceManager] Invalid chunk index {} for '{}'", packet.chunkIndex, packet.relativePath);
		return;
	}

	if (assembly.chunks[packet.chunkIndex].empty())
	{
		if (packet.data.size() > expected.size - assembly.receivedBytes)
		{
			LOG_ERROR("[ResourceManager] '{}' exceeds its announced size ({} bytes), aborting this file", fileKey, expected.size);
			assembling_files_.erase(fileKey);
			return;
		}

		assembly.chunks[packet.chunkIndex] = packet.data;
		assembly.receivedChunks++;
		assembly.receivedBytes += packet.data.size();

		for (size_t i = 0; i < download_progress_.size(); ++i) {
			if (download_progress_[i].fileHash == expected.hash) {
				auto& progress = download_progress_[i];
				download_progress_[i].bytesReceived += packet.data.size();

				int percentage = (progress.totalSize > 0)
					? static_cast<int>((static_cast<double>(progress.bytesReceived) / progress.totalSize) * 100.0)
					: 100;
				
				download_dialog_->Update(static_cast<uint32_t>(i), download_progress_[i].bytesReceived);
			}
		}
	}

	if (assembly.receivedChunks == assembly.totalChunks)
	{
		LOG_INFO("[ResourceManager] All chunks received for '{}', assembling...", packet.relativePath);

		std::vector<uint8_t> completeFile;
		size_t totalSize = 0;
		for (const auto& chunk : assembly.chunks) {
			totalSize += chunk.size();
		}

		completeFile.reserve(totalSize);

		for (const auto& chunk : assembly.chunks) {
			completeFile.insert(completeFile.end(), chunk.begin(), chunk.end());
		}

		// Verify against the manifest, not against the hash carried by the packet itself.
		std::string receivedHash = CalculateSHA256FromData(completeFile);
		if (completeFile.size() != expected.size || receivedHash != expected.hash)
		{
			LOG_ERROR("[ResourceManager] Integrity check failed for '{}': expected {} ({} bytes), got {} ({} bytes)",
				packet.relativePath, expected.hash, expected.size, receivedHash, completeFile.size());

			assembling_files_.erase(fileKey);

			for (auto& progress : download_progress_)
			{
				if (progress.fileName == packet.relativePath) {
					progress.isComplete = false;
					break;
				}
			}
			return;
		}

		// savePath was resolved and checked to stay inside the server cache when the file was requested.
		const std::string savePath = expected.savePath.string();

		try {
			std::filesystem::create_directories(expected.savePath.parent_path());

			std::ofstream outFile(expected.savePath, std::ios::binary | std::ios::trunc);
			if (!outFile)
			{
				LOG_ERROR("[ResourceManager] Cannot open '{}' for writing", savePath);
				assembling_files_.erase(fileKey);
				return;
			}
			outFile.write(reinterpret_cast<const char*>(completeFile.data()), completeFile.size());
			outFile.close();

			LOG_INFO("[ResourceManager] File '{}' saved successfully ({} bytes)", packet.relativePath, completeFile.size());

			if (LoadPakIntoVFS(packet.resourceName, savePath))
			{
				LOG_INFO("[ResourceManager] Loaded '{}' into VFS", packet.resourceName);
			}

			for (auto& progress : download_progress_)
			{
				if (progress.fileName == packet.relativePath) {
					progress.isComplete = true;
					progress.fileHash = receivedHash;
					break;
				}
			}

			assembling_files_.erase(fileKey);

			bool allComplete = true;

			for (const auto& progress : download_progress_)
			{
				if (!progress.isComplete) {
					allComplete = false;
					break;
				}
			}

			if (allComplete)
			{
				LOG_INFO("[ResourceManager] All downloads complete!");

				state_ = DownloadState::COMPLETED;
				download_dialog_->Finish();
			}
		}
		catch (const std::exception& e) {
			LOG_ERROR("[ResourceManager] Failed to save file '{}': {}", packet.relativePath, e.what());
		}
	}
}

bool ResourceManager::LoadPakIntoVFS(const std::string& resourceName, const std::string& pakPath)
{
	if (master_key_.empty())
	{
		LOG_ERROR("[ResourceManager] Cannot load PAK: master key not set");
		return false;
	}

	mz_zip_archive zip{};
	if (!mz_zip_reader_init_file(&zip, pakPath.c_str(), 0))
	{
		LOG_ERROR("[ResourceManager] Failed to open PAK file: {}", pakPath);
		return false;
	}

	const mz_uint num_files = mz_zip_reader_get_num_files(&zip);
	LOG_DEBUG("[ResourceManager] Loading PAK with {} files", num_files);

	VirtualFileSystem vfs;

	for (mz_uint i = 0; i < num_files; ++i)
	{
		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&zip, i, &file_stat))
			continue;

		size_t uncomp_size = 0;
		void* p = mz_zip_reader_extract_to_heap(&zip, i, &uncomp_size, 0);
		if (!p)
		{
			LOG_WARN("[ResourceManager] Failed to extract file '{}' from PAK", file_stat.m_filename);
			continue;
		}

		std::vector<uint8_t> entry_data(static_cast<uint8_t*>(p), static_cast<uint8_t*>(p) + uncomp_size);
		mz_free(p);

		std::vector<uint8_t> decoded;
		if (!DecodeResourceEntry(entry_data, master_key_, decoded))
		{
			LOG_WARN("[ResourceManager] Failed to decode file '{}'", file_stat.m_filename);
			continue;
		}

		const auto decoded_size = decoded.size();
		vfs[file_stat.m_filename] = std::move(decoded);
		LOG_DEBUG("[ResourceManager] Loaded file: {} ({} bytes)", file_stat.m_filename, decoded_size);
	}

	mz_zip_reader_end(&zip);

	{
		std::lock_guard<std::mutex> lock(vfs_mutex_);
		loaded_resources_vfs_[resourceName] = std::move(vfs);
	}

	LOG_INFO("[ResourceManager] Loaded resource '{}' into VFS ({} files)", resourceName, num_files);
	return true;
}

bool ResourceManager::GetFileContent(const std::string& resourceName,
	const std::string& internalPath,
	std::vector<uint8_t>& outContent)
{
	std::lock_guard<std::mutex> lock(vfs_mutex_);

	auto it = loaded_resources_vfs_.find(resourceName);
	if (it == loaded_resources_vfs_.end())
	{
		LOG_WARN("[ResourceManager] Resource '{}' not found in VFS", resourceName);
		return false;
	}

	auto& vfs = it->second;
	auto file_it = vfs.find(internalPath);
	if (file_it == vfs.end())
	{
		LOG_WARN("[ResourceManager] File '{}' not found in resource '{}'", internalPath, resourceName);
		return false;
	}

	outContent = file_it->second;
	return true;
}
