#pragma once

#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include "include/cef_audio_handler.h"

class BrowserManager;
class AudioManager;
class FocusManager;
class NetworkManager;

class BrowserClient : public CefClient,
    public CefLifeSpanHandler,
    public CefLoadHandler,
    public CefRenderHandler,
    public CefDisplayHandler,
    public CefResourceRequestHandler,
    public CefRequestHandler,
    public CefAudioHandler
 {

public:
    static CefRefPtr<BrowserClient> Create(int id, BrowserManager& mgr, AudioManager& audio, FocusManager* focus, NetworkManager& network);
    BrowserClient(int id, BrowserManager& mgr, AudioManager& audio, FocusManager* focus, NetworkManager& network);

    // CefClient overrides
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; };
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefAudioHandler>  GetAudioHandler()  override { return this; }
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override;

    // CefLifeSpanHandler overrides
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;

    // CefRenderHandler overrides
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser,
        PaintElementType type,
        const RectList& dirtyRects,
        const void* buffer,
        int width, int height) override;

    // CefDisplayHandler overrides
    bool OnCursorChange(CefRefPtr<CefBrowser> browser,
        CefCursorHandle cursor,
        cef_cursor_type_t type,
        const CefCursorInfo& custom_cursor_info) override;

    CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request,
    bool is_navigation,
    bool is_download,
    const CefString& request_initiator,
    bool& disable_default_handling) override { return this; }

    bool OnBeforeBrowse(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        bool user_gesture,
        bool is_redirect) override;

    CefResourceRequestHandler::ReturnValue OnBeforeResourceLoad(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        CefRefPtr<CefCallback> callback) override;

    // CefAudioHandler overrides
    void OnAudioStreamStarted(CefRefPtr<CefBrowser> browser, const CefAudioParameters& params, int channels) override;
    void OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float** data, int frames, int64_t pts) override;
    void OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) override;
    void OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString& message) override;

    bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                     cef_log_severity_t level,
                     const CefString& message,
                     const CefString& source,
                     int line) override;

private:
    int browserId_;
    BrowserManager& manager_;
    AudioManager& audio_;
    FocusManager* focus_{};
    NetworkManager& network_;

    // Audio parameters cached from stream start
    int sample_rate_ = 44100;
    int channels_ = 2;

    IMPLEMENT_REFCOUNTING(BrowserClient);
};