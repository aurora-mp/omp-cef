#include "download_dialog.hpp"

#include <chrono>
#include <string>

#include "network/network_manager.hpp"
#include "ui/hud_manager.hpp"
#include "samp/samp.hpp"
#include "browser/manager.hpp"

#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

#include "system/logger.hpp"
#include "shared/events.hpp"

// Internal loader browser (reserved id)
static constexpr int kLoaderBrowserId = 999999;
static constexpr const char* kLoaderUrl = "http://cef/__internal/loading.html";

static uint64_t NowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

DownloadDialog::DownloadDialog(NetworkManager* net, HudManager* hud, Samp* samp, BrowserManager* browser)
    : network_(net), hud_(hud), samp_(samp), browser_(browser)
{
}


void DownloadDialog::SetEnabled(bool enabled)
{
    enabled_ = enabled;

    // If UI is disabled while a loader is visible, close it immediately.
    if (!enabled_ && loader_visible_)
    {
        HideLoader();
    }
}

void DownloadDialog::EnsureLoaderVisible()
{
    if (!enabled_)
        return;

    if (!browser_)
        return;

    if (browser_->GetBrowserInstance(kLoaderBrowserId))
    {
        loader_visible_ = true;
        return;
    }

    if (hud_) {
        hud_->ToggleComponent(EHudComponent::ALL, false);
        hud_->SetClassSelectionVisible(false);
    }

    // Create internal loader page (no focus)
    browser_->CreateBrowser(kLoaderBrowserId, kLoaderUrl, false, false, -1.f, -1.f);
    loader_visible_ = true;

    // Push manifest info right away
    JsCall("window.__ompcef && window.__ompcef.manifest(" +
        std::to_string(files_.size()) + "," + std::to_string(total_bytes_) + ");");
}

void DownloadDialog::HideLoader()
{
    if (!browser_)
        return;

    const bool has_loader = (browser_->GetBrowserInstance(kLoaderBrowserId) != nullptr);
    if (has_loader)
        browser_->DestroyBrowser(kLoaderBrowserId);

    if (hud_ && (loader_visible_ || has_loader)) {
        hud_->ToggleComponent(EHudComponent::ALL, true);
        // hud_->SetClassSelectionVisible(true);
    }

    loader_visible_ = false;
}

void DownloadDialog::JsCall(const std::string& js)
{
    if (!enabled_)
        return;

    if (!browser_)
        return;

    // Execute on CEF UI thread
    CefPostTask(TID_UI, base::BindOnce([](DownloadDialog* self, std::string code)
    {
        auto* inst = self->browser_->GetBrowserInstance(kLoaderBrowserId);
        if (!inst || !inst->browser)
            return;

        auto frame = inst->browser->GetMainFrame();
        frame->ExecuteJavaScript(code, frame->GetURL(), 0);
    }, base::Unretained(this), js));
}

void DownloadDialog::Start(std::vector<std::pair<std::string, size_t>> files)
{
    active_ = true;
    files_ = std::move(files);
    received_by_file_.assign(files_.size(), 0);

    total_bytes_ = 0;
    for (const auto& f : files_)
        total_bytes_ += (uint64_t)f.second;

    ClientEmitEventPacket event;
    event.name = CefEvent::Client::DownloadStart;
    network_->SendPacket(PacketType::ClientEmitEvent, event);

    last_ui_sent_ms_ = 0;

    EnsureLoaderVisible();

    LOG_INFO("[DownloadDialog] Started ({} file(s), {} bytes).", (int)files_.size(), (uint64_t)total_bytes_);
}

void DownloadDialog::Update(uint32_t index, uint64_t received)
{
    if (!enabled_)
        return;

    if (!active_)
        return;

    if (index >= received_by_file_.size())
        return;

    received_by_file_[index] = received;

    // throttle JS updates (smoothness handled by JS interpolation anyway)
    const uint64_t now = NowMs();
    if (now - last_ui_sent_ms_ < 50)
        return;

    last_ui_sent_ms_ = now;

    uint64_t overall_received = 0;
    for (auto v : received_by_file_)
        overall_received += v;

    const std::string& fileName = files_[index].first;
    const uint64_t fileTotal = (uint64_t)files_[index].second;

    const std::string js =
        "window.__ompcef && window.__ompcef.progress(" +
        std::string("'") + fileName + "'," +
        std::to_string(received) + "," +
        std::to_string(fileTotal) + "," +
        std::to_string(overall_received) + "," +
        std::to_string(total_bytes_) + ");";

    JsCall(js);
}

void DownloadDialog::ShowError(const char* filename, unsigned long status_code, int attempt, int max_retry)
{
    LOG_WARN("[DownloadDialog] Error file={} status={} attempt={}/{}",
        filename ? filename : "<null>", status_code, attempt, max_retry);
}

void DownloadDialog::Finish()
{
    if (!active_)
        return;

    active_ = false;

    
    ClientEmitEventPacket event;
    event.name = CefEvent::Client::DownloadFinish;
    network_->SendPacket(PacketType::ClientEmitEvent, event);

    if (enabled_)
    {
        JsCall("window.__ompcef && window.__ompcef.done();");

        // Let user see 100% shortly, then hide
        CefPostDelayedTask(TID_UI,
            base::BindOnce([](DownloadDialog* self) { 
                self->HideLoader(); 
            }, base::Unretained(this)),
            700);
    }

    LOG_INFO("[DownloadDialog] Finished.");
}