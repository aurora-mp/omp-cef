#include "resource_download_dialog_manager.hpp"

#include "bridge.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

namespace
{
    constexpr const char* ColorDefault = "{FFFFFF}";
    constexpr const char* ColorMuted = "{808080}";
    constexpr const char* ColorSuccess = "{88AA62}";

    bool HasUpdateTimestamp(const std::chrono::steady_clock::time_point& timePoint)
    {
        return timePoint.time_since_epoch().count() != 0;
    }
}

void ResourceDownloadDialogManager::SetMode(ResourceLoaderUiMode mode)
{
    mode_ = mode;
}

ResourceLoaderUiMode ResourceDownloadDialogManager::GetMode() const
{
    return mode_;
}

void ResourceDownloadDialogManager::SetDialogId(int dialogId)
{
    dialogId_ = IsValidDialogId(dialogId) ? dialogId : DefaultDialogId;
}

int ResourceDownloadDialogManager::GetDialogId() const
{
    return dialogId_;
}

bool ResourceDownloadDialogManager::UsesClientLoader() const
{
    return mode_ == ResourceLoaderUiMode::Cef;
}

bool ResourceDownloadDialogManager::UsesServerDialog() const
{
    return mode_ == ResourceLoaderUiMode::SampDialog;
}

void ResourceDownloadDialogManager::StartDownload(IPlatformBridge& bridge, int playerid)
{
    PlayerState& state = GetOrCreateState(playerid);
    state = PlayerState{};
    state.active = true;

    ShowDialog(bridge, playerid, state, true);
}

void ResourceDownloadDialogManager::SetPlan(IPlatformBridge& bridge, int playerid, const std::vector<std::pair<std::string, size_t>>& files)
{
    PlayerState& state = GetOrCreateState(playerid);
    state.active = true;
    state.filePlanAvailable = true;
    state.files.clear();
    state.files.reserve(files.size());

    for (const auto& file : files)
    {
        FileState fileState;
        fileState.fileName = file.first;
        fileState.totalBytes = file.second;
        state.files.push_back(std::move(fileState));
    }

    state.dialogThrottle = {};
    state.callbackThrottle = {};
    ShowDialog(bridge, playerid, state, true);
}

void ResourceDownloadDialogManager::UpdateFileProgress(IPlatformBridge& bridge, int playerid, const std::string& fileName, size_t bytesSent, size_t totalBytes)
{
    auto it = players_.find(playerid);
    if (it == players_.end() || !it->second.active)
        return;

    PlayerState& state = it->second;
    state.currentFileName = fileName;

    auto fileIt = std::find_if(state.files.begin(), state.files.end(), [&fileName](const FileState& file)
    {
        return file.fileName == fileName;
    });

    if (fileIt == state.files.end())
    {
        FileState fileState;
        fileState.fileName = fileName;
        fileState.totalBytes = totalBytes;
        state.files.push_back(std::move(fileState));
        fileIt = std::prev(state.files.end());
        state.filePlanAvailable = true;
    }

    fileIt->totalBytes = totalBytes;
    fileIt->bytesSent = std::min(bytesSent, totalBytes);
    fileIt->completed = totalBytes > 0 && fileIt->bytesSent >= totalBytes;

    const ProgressSnapshot snapshot = BuildSnapshot(state);
    EmitProgressCallbackIfNeeded(bridge, playerid, state, snapshot);
    ShowDialog(bridge, playerid, state, false);
}

void ResourceDownloadDialogManager::CompleteCurrentFile(IPlatformBridge& bridge, int playerid, const std::string& fileName)
{
    auto it = players_.find(playerid);
    if (it == players_.end() || !it->second.active)
        return;

    PlayerState& state = it->second;

    auto fileIt = std::find_if(state.files.begin(), state.files.end(), [&fileName](const FileState& file)
    {
        return file.fileName == fileName;
    });

    if (fileIt != state.files.end())
    {
        fileIt->bytesSent = fileIt->totalBytes;
        fileIt->completed = true;
    }

    state.currentFileName.clear();

    const ProgressSnapshot snapshot = BuildSnapshot(state);
    EmitProgressCallbackIfNeeded(bridge, playerid, state, snapshot);
    ShowDialog(bridge, playerid, state, true);
}

void ResourceDownloadDialogManager::FinishDownload(IPlatformBridge& bridge, int playerid)
{
    auto it = players_.find(playerid);
    if (it == players_.end())
        return;

    HideDialog(bridge, playerid, it->second);
    players_.erase(it);
}

void ResourceDownloadDialogManager::AbortDownload(IPlatformBridge& bridge, int playerid)
{
    auto it = players_.find(playerid);
    if (it == players_.end())
        return;

    HideDialog(bridge, playerid, it->second);
    players_.erase(it);
}

bool ResourceDownloadDialogManager::HandleDialogResponse(IPlatformBridge& bridge, int playerid, int dialogId)
{
    if (dialogId != dialogId_)
        return false;

    auto it = players_.find(playerid);
    if (it == players_.end() || !it->second.active)
        return false;

    ShowDialog(bridge, playerid, it->second, true);
    return true;
}

bool ResourceDownloadDialogManager::IsValidDialogId(int dialogId)
{
    return dialogId >= 0 && dialogId <= 32767;
}

int ResourceDownloadDialogManager::CalculatePercent(size_t value, size_t total)
{
    if (total == 0)
        return 0;

    if (value >= total)
        return 100;

    const double percent = (static_cast<double>(value) * 100.0) / static_cast<double>(total);
    return std::clamp(static_cast<int>(percent), 0, 100);
}

int ResourceDownloadDialogManager::ToKilobytes(size_t bytes)
{
    static constexpr size_t BytesPerKilobyte = 1024;
    const size_t kilobytes = bytes / BytesPerKilobyte;

    if (kilobytes > static_cast<size_t>(std::numeric_limits<int>::max()))
        return std::numeric_limits<int>::max();

    return static_cast<int>(kilobytes);
}

std::string ResourceDownloadDialogManager::FormatByteSize(size_t bytes)
{
    static constexpr double KiB = 1024.0;
    static constexpr double MiB = 1024.0 * 1024.0;
    static constexpr double GiB = 1024.0 * 1024.0 * 1024.0;

    double value = static_cast<double>(bytes);
    const char* unit = "B";

    if (bytes >= static_cast<size_t>(GiB))
    {
        value /= GiB;
        unit = "GB";
    }
    else if (bytes >= static_cast<size_t>(MiB))
    {
        value /= MiB;
        unit = "MB";
    }
    else if (bytes >= static_cast<size_t>(KiB))
    {
        value /= KiB;
        unit = "KB";
    }

    std::ostringstream stream;
    if (unit[0] == 'B')
        stream << bytes << ' ' << unit;
    else
        stream << std::fixed << std::setprecision(2) << value << ' ' << unit;

    return stream.str();
}

ResourceDownloadDialogManager::PlayerState& ResourceDownloadDialogManager::GetOrCreateState(int playerid)
{
    return players_[playerid];
}

ResourceDownloadDialogManager::ProgressSnapshot ResourceDownloadDialogManager::BuildSnapshot(const PlayerState& state) const
{
    ProgressSnapshot snapshot;
    snapshot.currentFileName = state.currentFileName;
    snapshot.totalFiles = static_cast<int>(state.files.size());

    for (const FileState& file : state.files)
    {
        snapshot.totalBytes += file.totalBytes;
        snapshot.totalBytesSent += std::min(file.bytesSent, file.totalBytes);

        if (file.completed)
            ++snapshot.completedFiles;

        if (!snapshot.currentFileName.empty() && file.fileName == snapshot.currentFileName)
        {
            snapshot.fileBytesSent = std::min(file.bytesSent, file.totalBytes);
            snapshot.fileTotalBytes = file.totalBytes;
        }
    }

    if (snapshot.currentFileName.empty())
    {
        for (const FileState& file : state.files)
        {
            if (!file.completed)
            {
                snapshot.currentFileName = file.fileName;
                snapshot.fileBytesSent = std::min(file.bytesSent, file.totalBytes);
                snapshot.fileTotalBytes = file.totalBytes;
                break;
            }
        }
    }

    snapshot.filePercent = CalculatePercent(snapshot.fileBytesSent, snapshot.fileTotalBytes);
    snapshot.totalPercent = CalculatePercent(snapshot.totalBytesSent, snapshot.totalBytes);
    snapshot.fileDownloadedKb = ToKilobytes(snapshot.fileBytesSent);
    snapshot.fileTotalKb = ToKilobytes(snapshot.fileTotalBytes);
    snapshot.totalDownloadedKb = ToKilobytes(snapshot.totalBytesSent);
    snapshot.totalKb = ToKilobytes(snapshot.totalBytes);

    if (snapshot.totalFiles == 0 && !state.filePlanAvailable)
        snapshot.totalPercent = 0;

    return snapshot;
}

bool ResourceDownloadDialogManager::ShouldRefreshDialog(PlayerState& state, const ProgressSnapshot& snapshot)
{
    const auto now = std::chrono::steady_clock::now();
    ProgressThrottle& throttle = state.dialogThrottle;

    const bool changed =
        throttle.currentFileName != snapshot.currentFileName ||
        throttle.fileBytesSent != snapshot.fileBytesSent ||
        throttle.totalBytesSent != snapshot.totalBytesSent ||
        throttle.filePercent != snapshot.filePercent ||
        throttle.totalPercent != snapshot.totalPercent;

    if (!changed)
        return false;

    const bool elapsed = !HasUpdateTimestamp(throttle.lastUpdate) || now - throttle.lastUpdate >= DialogUpdateInterval;
    const bool important =
        throttle.currentFileName != snapshot.currentFileName ||
        snapshot.filePercent >= 100 ||
        snapshot.totalPercent >= 100;

    if (!elapsed && !important)
        return false;

    throttle.currentFileName = snapshot.currentFileName;
    throttle.fileBytesSent = snapshot.fileBytesSent;
    throttle.totalBytesSent = snapshot.totalBytesSent;
    throttle.filePercent = snapshot.filePercent;
    throttle.totalPercent = snapshot.totalPercent;
    throttle.lastUpdate = now;
    return true;
}

bool ResourceDownloadDialogManager::ShouldEmitCallback(PlayerState& state, const ProgressSnapshot& snapshot)
{
    const auto now = std::chrono::steady_clock::now();
    ProgressThrottle& throttle = state.callbackThrottle;

    const bool changed =
        throttle.currentFileName != snapshot.currentFileName ||
        throttle.fileBytesSent != snapshot.fileBytesSent ||
        throttle.totalBytesSent != snapshot.totalBytesSent ||
        throttle.filePercent != snapshot.filePercent ||
        throttle.totalPercent != snapshot.totalPercent;

    if (!changed)
        return false;

    const bool elapsed = !HasUpdateTimestamp(throttle.lastUpdate) || now - throttle.lastUpdate >= CallbackUpdateInterval;
    const bool important =
        throttle.currentFileName != snapshot.currentFileName ||
        snapshot.filePercent >= 100 ||
        snapshot.totalPercent >= 100;

    if (!elapsed && !important)
        return false;

    throttle.currentFileName = snapshot.currentFileName;
    throttle.fileBytesSent = snapshot.fileBytesSent;
    throttle.totalBytesSent = snapshot.totalBytesSent;
    throttle.filePercent = snapshot.filePercent;
    throttle.totalPercent = snapshot.totalPercent;
    throttle.lastUpdate = now;
    return true;
}

void ResourceDownloadDialogManager::ShowDialog(IPlatformBridge& bridge, int playerid, PlayerState& state, bool force)
{
    if (!UsesServerDialog() || !state.active)
        return;

    const ProgressSnapshot snapshot = BuildSnapshot(state);
    if (!force && !ShouldRefreshDialog(state, snapshot))
        return;

    bridge.ShowResourceDownloadDialog(
        playerid,
        dialogId_,
        BuildTitle(snapshot),
        BuildBody(state, snapshot),
        BuildButton(snapshot),
        "");

    state.dialogVisible = true;
}

void ResourceDownloadDialogManager::HideDialog(IPlatformBridge& bridge, int playerid, PlayerState& state)
{
    if (!state.dialogVisible)
        return;

    bridge.HideResourceDownloadDialog(playerid);
    state.dialogVisible = false;
}

void ResourceDownloadDialogManager::EmitProgressCallbackIfNeeded(IPlatformBridge& bridge, int playerid, PlayerState& state, const ProgressSnapshot& snapshot)
{
    if (!ShouldEmitCallback(state, snapshot))
        return;

    bridge.CallOnDownloadProgress(
        playerid,
        snapshot.currentFileName,
        snapshot.filePercent,
        snapshot.totalPercent,
        snapshot.fileDownloadedKb,
        snapshot.fileTotalKb,
        snapshot.totalDownloadedKb,
        snapshot.totalKb);
}

std::string ResourceDownloadDialogManager::BuildTitle(const ProgressSnapshot& snapshot) const
{
    if (snapshot.totalFiles <= 0)
        return "Downloading resources ...";

    return "Downloading resources ... (" +
        std::to_string(snapshot.completedFiles) + "/" +
        std::to_string(snapshot.totalFiles) + ")";
}

std::string ResourceDownloadDialogManager::BuildBody(const PlayerState& state, const ProgressSnapshot& snapshot) const
{
    std::ostringstream body;
    body << "File\tStatus";

    if (!state.filePlanAvailable)
    {
        body << "\nPreparing ...\tWaiting ...";
        return body.str();
    }

    if (state.files.empty())
    {
        body << "\nNo resource file queued.\tWaiting ...";
        return body.str();
    }

    for (const FileState& file : state.files)
    {
        body << '\n' << file.fileName << '\t';

        if (file.completed)
        {
            body << ColorSuccess << "Done ... (" << FormatByteSize(file.totalBytes) << ")";
            continue;
        }

        if (file.fileName == snapshot.currentFileName)
        {
            body << ColorDefault << "Downloading ... ("
                 << FormatByteSize(std::min(file.bytesSent, file.totalBytes))
                 << " / "
                 << FormatByteSize(file.totalBytes)
                 << ")";
            continue;
        }

        body << ColorMuted << "Pending ... (" << FormatByteSize(file.totalBytes) << ")";
    }

    return body.str();
}

std::string ResourceDownloadDialogManager::BuildButton(const ProgressSnapshot& snapshot) const
{
    return std::to_string(snapshot.totalPercent) + "%";
}
