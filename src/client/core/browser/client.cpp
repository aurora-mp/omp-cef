#include "client.hpp"

#include <vector>

#include <include/wrapper/cef_helpers.h>

#include "browser/audio.hpp"
#include "browser/focus.hpp"
#include "browser/manager.hpp"
#include "network/network_manager.hpp"

static inline std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { 
            return static_cast<char>(std::tolower(c)); 
        });

    return s;
}

static inline bool StartsWith(const std::string& s, const std::string& prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

static inline bool Contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

static inline std::string StripFragment(const std::string& url)
{
    const auto pos = url.find('#');
    return pos == std::string::npos ? url : url.substr(0, pos);
}

// Detect if URL points to a site that needs Referer/Origin help (YouTube/Twitch).
static inline bool IsVideoSiteUrl(const std::string& url)
{
    const std::string u = ToLower(url);

    // YouTube
    if (Contains(u, "youtube.com") || Contains(u, "youtu.be"))
        return true;

    // Google video domains used by YouTube playback
    if (Contains(u, "googlevideo.com") || Contains(u, "ytimg.com") || Contains(u, "googleusercontent.com"))
        return true;

    // Twitch
    if (Contains(u, "twitch.tv") || Contains(u, "ttvnw.net"))
        return true;

    return false;
}

// Detect if the URL is specifically an embed/player
static inline bool IsVideoEmbedUrl(const std::string& url)
{
    const std::string u = ToLower(url);

    // https://www.youtube.com/embed/XXXX
    if (Contains(u, "youtube.com/embed/"))
        return true;

    // https://player.twitch.tv/?channel=xxx&parent=twitch.tv
    if (Contains(u, "player.twitch.tv/"))
        return true;

    return false;
}

// Choose a stable referrer for these sites
static inline std::string ChooseReferrerForUrl(const std::string& url)
{
    const std::string u = ToLower(url);

    // Twitch is VERY picky about Origin/Referer
    if (Contains(u, "twitch.tv") || Contains(u, "ttvnw.net"))
        return "https://www.twitch.tv/";

    // YouTube
    return "https://www.youtube.com/";
}

CefRefPtr<BrowserClient> BrowserClient::Create(int id, BrowserManager& mgr, AudioManager& audio, FocusManager* focus, NetworkManager& network)
{
    return new BrowserClient(id, mgr, audio, focus, network);
}

BrowserClient::BrowserClient(int id, BrowserManager& mgr, AudioManager& audio, FocusManager* focus, NetworkManager& network)
	: browserId_(id), manager_(mgr), audio_(audio), focus_(focus), network_(network)
{
}

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();
    manager_.OnBrowserCreated(browserId_, browser);

    // Inject focus tracking script
    const char* control_chat_js_script = R"JS(
        (function() {
            let isFocused = false;
            const checkFocus = (target) => {
                const isInput = target.tagName === 'INPUT' || 
                                target.tagName === 'TEXTAREA' || 
                                target.isContentEditable;
                if (isInput !== isFocused) {
                    isFocused = isInput;
                    if (window.cef && window.cef.emit) {
                        window.cef.emit('omp:cef:internal:inputFocusChanged', isFocused);
                    }
                }
            };
            document.addEventListener('focusin', (e) => checkFocus(e.target), true);
            document.addEventListener('focusout', (e) => {
                if (isFocused) {
                    isFocused = false;
                    if (window.cef && window.cef.emit) {
                        window.cef.emit('omp:cef:internal:inputFocusChanged', false);
                    }
                }
            }, true);
            document.addEventListener('mousedown', (e) => checkFocus(e.target), true);
        })();
    )JS";

    browser->GetMainFrame()->ExecuteJavaScript(control_chat_js_script, browser->GetMainFrame()->GetURL(), 0);
}

void BrowserClient::GetViewRect(CefRefPtr<CefBrowser> /*browser*/, CefRect& rect)
{
    rect = manager_.GetBrowserInstance(browserId_)->view.rect();
}

void BrowserClient::OnPaint(CefRefPtr<CefBrowser> /*browser*/,
                            PaintElementType type,
                            const RectList& dirtyRects,
                            const void* buffer,
                            int width,
                            int height)
{
    if (!buffer || type != PET_VIEW)
        return;

    std::vector<cef_rect_t> dirty_rects;
    dirty_rects.reserve(dirtyRects.size());

    for (const auto& rect : dirtyRects)
        dirty_rects.push_back(cef_rect_t{ rect.x, rect.y, rect.width, rect.height });

    manager_.OnPaint(browserId_, buffer, width, height, dirty_rects.data(), dirty_rects.size());
}

bool BrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> /*browser*/,
                                             CefRefPtr<CefFrame> /*frame*/,
                                             CefProcessId /*source_process*/,
                                             CefRefPtr<CefProcessMessage> message)
{
    CEF_REQUIRE_UI_THREAD();

    const std::string msg_name = message->GetName();

    if (msg_name == "cef_screen_capture_start")
    {
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        const int width = args->GetSize() > 0 && args->GetType(0) == VTYPE_INT
            ? args->GetInt(0) : 640;
        const int height = args->GetSize() > 1 && args->GetType(1) == VTYPE_INT
            ? args->GetInt(1) : 360;
        const int fps = args->GetSize() > 2 && args->GetType(2) == VTYPE_INT
            ? args->GetInt(2) : 15;

        manager_.StartScreenCapture(browserId_, width, height, fps);
        return true;
    }

    if (msg_name == "cef_screen_capture_stop")
    {
        manager_.StopScreenCapture(browserId_);
        return true;
    }

    if (msg_name == "emit_event")
    {
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        if (args->GetSize() < 1 || args->GetType(0) != VTYPE_STRING)
            return true;
        const std::string event_name = args->GetString(0);

        if (event_name == "omp:cef:internal:inputFocusChanged")
        {
            if (focus_ && args->GetSize() > 1 && args->GetType(1) == VTYPE_BOOL)
            {
                bool has_focus = args->GetBool(1);
                focus_->SetInputFocus(browserId_, has_focus);
            }
            return true;
        }

        // Client-only game data polling (avoid spamming the server for HUD-like stats)
        // JS: cef.emit('game:data:pollPlayerStats', enabled, intervalMs)
        if (event_name == "game:data:pollPlayerStats")
        {
            bool enabled = true;
            int intervalMs = 50;

            if (args->GetSize() > 1 && args->GetType(1) == VTYPE_BOOL)
                enabled = args->GetBool(1);

            if (args->GetSize() > 2 && args->GetType(2) == VTYPE_INT)
                intervalMs = args->GetInt(2);

            manager_.SetPlayerStatsPolling(browserId_, enabled, intervalMs);
            return true;
        }

        ClientEmitEventPacket event_packet;
        event_packet.browserId = browserId_;
        event_packet.name = event_name;

        for (size_t i = 1; i < args->GetSize(); ++i) {
            CefValueType type = args->GetType(i);
            if (type == VTYPE_BOOL)
                event_packet.args.emplace_back(args->GetBool(i));
            else if (type == VTYPE_INT)   
                event_packet.args.emplace_back(args->GetInt(i));
            else if (type == VTYPE_DOUBLE)
                event_packet.args.emplace_back(static_cast<float>(args->GetDouble(i)));
            else if (type == VTYPE_STRING)
                event_packet.args.emplace_back(args->GetString(i).ToString());
        }

		network_.SendPacket(PacketType::ClientEmitEvent, event_packet);
        return true;
    }

    if (message->GetName() == "cef_set_focus")
    {
        const bool hasFocus = message->GetArgumentList()->GetBool(0);
        manager_.FocusBrowser(browserId_, hasFocus);

        return true;
    }

    if (message->GetName() == "cef_exit_game")
    {
        manager_.ExitGame();

        return true;
    }

    return false;
}

bool BrowserClient::OnCursorChange(CefRefPtr<CefBrowser> /*browser*/,
                                   CefCursorHandle /*cursor*/,
                                   cef_cursor_type_t type,
                                   const CefCursorInfo& /*custom_cursor_info*/)
{
    manager_.SetCursorType(type);
    return true;
}

bool BrowserClient::OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        bool user_gesture,
        bool is_redirect){
    const std::string url = request->GetURL().ToString();

    if (frame && frame->IsMain() &&
        StripFragment(frame->GetURL().ToString()) != StripFragment(url))
    {
        manager_.RequestTextureClear(browserId_);
    }

    if (!IsVideoSiteUrl(url)) 
        return false;

    const std::string ref = ChooseReferrerForUrl(url);
    if (!ref.empty())
        request->SetReferrer(ref, REFERRER_POLICY_ORIGIN);

    return false;
}

CefResourceRequestHandler::ReturnValue BrowserClient::OnBeforeResourceLoad(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefCallback> callback)
{
    const std::string url = request->GetURL().ToString();
    if (!IsVideoSiteUrl(url))
        return RV_CONTINUE;

    const std::string ref = ChooseReferrerForUrl(url);
    if (ref.empty())
        return RV_CONTINUE;

    CefRequest::HeaderMap headers;
    request->GetHeaderMap(headers);

    auto referer_range = headers.equal_range("Referer");
    headers.erase(referer_range.first, referer_range.second);
    
    headers.insert(std::make_pair("Referer", ref));

    // Twitch
    if (url.find("twitch.tv") != std::string::npos || url.find("ttvnw.net") != std::string::npos)
    {
        auto origin_range = headers.equal_range("Origin");
        headers.erase(origin_range.first, origin_range.second);
        
        headers.insert(std::make_pair("Origin", "https://www.twitch.tv"));
        headers.insert(std::make_pair("Sec-Fetch-Dest", "iframe"));
        headers.insert(std::make_pair("Sec-Fetch-Mode", "navigate"));
        headers.insert(std::make_pair("Sec-Fetch-Site", "cross-site"));
    }
    
    // Youtube
    if (url.find("youtube.com") != std::string::npos || 
        url.find("googlevideo.com") != std::string::npos ||
        url.find("ytimg.com") != std::string::npos)
    {
        auto origin_range = headers.equal_range("Origin");
        headers.erase(origin_range.first, origin_range.second);
        
        headers.insert(std::make_pair("Origin", "https://www.youtube.com"));
        headers.insert(std::make_pair("Sec-Fetch-Dest", "iframe"));
        headers.insert(std::make_pair("Sec-Fetch-Mode", "navigate"));
        headers.insert(std::make_pair("Sec-Fetch-Site", "cross-site"));
    }

    // User-Agent
    if (headers.find("User-Agent") == headers.end()) {
        headers.insert(std::make_pair("User-Agent", 
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"));
    }

    request->SetHeaderMap(headers);
    return RV_CONTINUE;
}

void BrowserClient::OnAudioStreamStarted(CefRefPtr<CefBrowser> /*browser*/,
                                         const CefAudioParameters& params,
                                         int channels)
{
    LOG_DEBUG("[CEF] AudioStreamStarted | browserId={}, sample_rate={}, channels={}",
              browserId_,
              params.sample_rate,
              channels);
    channels_ = channels;
    sample_rate_ = params.sample_rate;
    audio_.EnsureStream(browserId_);
}

void BrowserClient::OnAudioStreamPacket(CefRefPtr<CefBrowser> /*browser*/,
                                        const float** data,
                                        int frames,
                                        int64_t /*pts*/)
{
    audio_.OnPcmPacket(browserId_, data, frames, channels_, sample_rate_);
}

void BrowserClient::OnAudioStreamStopped(CefRefPtr<CefBrowser> /*browser*/)
{
    LOG_DEBUG("[CEF] AudioStreamStopped | browserId={}", browserId_);
    audio_.RemoveStream(browserId_);
}

void BrowserClient::OnAudioStreamError(CefRefPtr<CefBrowser> /*browser*/, const CefString& message)
{
    LOG_ERROR("[CEF] AudioStreamError | browserId={} | message={}", browserId_, message.ToString().c_str());
    audio_.RemoveStream(browserId_);
}

bool BrowserClient::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                     cef_log_severity_t level,
                                     const CefString& message,
                                     const CefString& source,
                                     int line)
{
    std::string level_str;
    switch(level) {
        case LOGSEVERITY_ERROR: level_str = "ERROR"; break;
        case LOGSEVERITY_WARNING: level_str = "WARN"; break;
        default: level_str = "INFO"; break;
    }
    
    if (message.ToString().find("153") != std::string::npos ||
        message.ToString().find("playback") != std::string::npos ||
        message.ToString().find("ErrorNotSupported") != std::string::npos) {
        LOG_ERROR("[VIDEO ERROR] [JS Console {}] {} ({}:{})", 
            level_str, message.ToString(), source.ToString(), line);
    } else {
        LOG_INFO("[JS Console {}] {} ({}:{})", 
            level_str, message.ToString(), source.ToString(), line);
    }
    return false;
}
