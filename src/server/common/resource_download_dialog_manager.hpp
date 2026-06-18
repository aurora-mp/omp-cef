#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class IPlatformBridge;

enum class ResourceLoaderUiMode
{
    Cef,
    SampDialog,
    None,
};

class ResourceDownloadDialogManager
{
public:
    static constexpr int DefaultDialogId = 32700;

    void SetMode(ResourceLoaderUiMode mode);
    ResourceLoaderUiMode GetMode() const;

    void SetDialogId(int dialogId);
    int GetDialogId() const;

    bool UsesClientLoader() const;
    bool UsesServerDialog() const;

    void StartDownload(IPlatformBridge& bridge, int playerid);
    void SetPlan(IPlatformBridge& bridge, int playerid, const std::vector<std::pair<std::string, size_t>>& files);
    void UpdateFileProgress(IPlatformBridge& bridge, int playerid, const std::string& fileName, size_t bytesSent, size_t totalBytes);
    void CompleteCurrentFile(IPlatformBridge& bridge, int playerid, const std::string& fileName);
    void FinishDownload(IPlatformBridge& bridge, int playerid);
    void AbortDownload(IPlatformBridge& bridge, int playerid);

    bool HandleDialogResponse(IPlatformBridge& bridge, int playerid, int dialogId);

private:
    struct FileState
    {
        std::string fileName;
        size_t bytesSent = 0;
        size_t totalBytes = 0;
        bool completed = false;
    };

    struct ProgressSnapshot
    {
        std::string currentFileName;
        size_t fileBytesSent = 0;
        size_t fileTotalBytes = 0;
        size_t totalBytesSent = 0;
        size_t totalBytes = 0;
        int filePercent = 0;
        int totalPercent = 0;
        int fileDownloadedKb = 0;
        int fileTotalKb = 0;
        int totalDownloadedKb = 0;
        int totalKb = 0;
        int completedFiles = 0;
        int totalFiles = 0;
    };

    struct ProgressThrottle
    {
        std::string currentFileName;
        size_t fileBytesSent = 0;
        size_t totalBytesSent = 0;
        int filePercent = -1;
        int totalPercent = -1;
        std::chrono::steady_clock::time_point lastUpdate{};
    };

    struct PlayerState
    {
        bool active = false;
        bool dialogVisible = false;
        bool filePlanAvailable = false;
        std::vector<FileState> files;
        std::string currentFileName;
        ProgressThrottle dialogThrottle;
        ProgressThrottle callbackThrottle;
    };

    static bool IsValidDialogId(int dialogId);
    static int CalculatePercent(size_t value, size_t total);
    static int ToKilobytes(size_t bytes);
    static std::string FormatByteSize(size_t bytes);

    PlayerState& GetOrCreateState(int playerid);
    ProgressSnapshot BuildSnapshot(const PlayerState& state) const;

    bool ShouldRefreshDialog(PlayerState& state, const ProgressSnapshot& snapshot);
    bool ShouldEmitCallback(PlayerState& state, const ProgressSnapshot& snapshot);

    void ShowDialog(IPlatformBridge& bridge, int playerid, PlayerState& state, bool force);
    void HideDialog(IPlatformBridge& bridge, int playerid, PlayerState& state);

    void EmitProgressCallbackIfNeeded(IPlatformBridge& bridge, int playerid, PlayerState& state, const ProgressSnapshot& snapshot);

    std::string BuildTitle(const ProgressSnapshot& snapshot) const;
    std::string BuildBody(const PlayerState& state, const ProgressSnapshot& snapshot) const;
    std::string BuildButton(const ProgressSnapshot& snapshot) const;

    static constexpr std::chrono::milliseconds DialogUpdateInterval{100};
    static constexpr std::chrono::milliseconds CallbackUpdateInterval{250};

    ResourceLoaderUiMode mode_ = ResourceLoaderUiMode::Cef;
    int dialogId_ = DefaultDialogId;
    std::unordered_map<int, PlayerState> players_;
};
