#pragma once

#include <vector>
#include <utility>
#include <cstdint>
#include <string>

class NetworkManager;
class HudManager;
class Samp;
class BrowserManager;

class DownloadDialog
{
public:
    DownloadDialog(NetworkManager* net, HudManager* hud, Samp* samp, BrowserManager* browser);
    ~DownloadDialog() = default;

    void Start(std::vector<std::pair<std::string, size_t>> files);
    void Update(uint32_t index, uint64_t received);
    void ShowError(const char* filename, unsigned long status_code, int attempt, int max_retry);
    void Finish();

    void SetEnabled(bool enabled);
    bool IsEnabled() const { return enabled_; }

private:
    void EnsureLoaderVisible();
    void HideLoader();
    void JsCall(const std::string& js);

private:
    NetworkManager* network_;
    HudManager* hud_;
    Samp* samp_;
    BrowserManager* browser_;

    bool active_ = false;
    bool enabled_ = true;
    bool loader_visible_ = false;

    std::vector<std::pair<std::string, size_t>> files_;
    std::vector<uint64_t> received_by_file_;
    uint64_t total_bytes_ = 0;

    uint64_t last_ui_sent_ms_ = 0;
};