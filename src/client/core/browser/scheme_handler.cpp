#include "scheme_handler.hpp"
#include "system/resource_manager.hpp"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_parser.h"
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <cstring>

static std::string GetInternalLoadingHtml()
{
    // Smooth loader page served at: http://cef/__internal/loading.html
    // Native can call JS functions:
    //   window.__ompcef.manifest(filesCount, totalBytes)
    //   window.__ompcef.progress(fileName, fileReceived, fileTotal, overallReceived, overallTotal)
    //   window.__ompcef.done()

    return R"HTML(<!doctype html>
        <html>
            <head>
                <meta charset="utf-8" />
                <meta name="viewport" content="width=device-width, initial-scale=1" />
                <title>Downloading resources UI</title>
                <style>
                    html,body{margin:0;height:100%;font-family:system-ui,Segoe UI,Roboto,Arial;background:transparent;color:#e7eefc}
                    .wrap{height:100%;display:flex;align-items:center;justify-content:center}
                    .card{width:min(560px,92vw);padding:18px 18px 14px;border-radius:16px;background:#1a1a1a;box-shadow:0 10px 40px rgba(0,0,0,.35);backdrop-filter:blur(10px)}
                    .title{font-size:18px;font-weight:700;margin:0 0 10px}
                    .sub{opacity:.75;font-size:13px;margin:0 0 14px}
                    .bar{height:12px;border-radius:999px;background:rgba(255,255,255,.08);overflow:hidden}
                    .fill{height:100%;width:0%;/*background:linear-gradient(90deg,#63e,#3cf)*/background:#ff9933;}
                    .row{display:flex;justify-content:space-between;gap:12px;margin-top:12px;font-size:12px;opacity:.85}
                    .file{white-space:nowrap;overflow:hidden;text-overflow:ellipsis;max-width:65%}
                </style>
            </head>

            <body>
                <div class="wrap">
                    <div class="card">
                        <p class="title">Downloading ressources ...</p>
                        <p class="sub" id="sub">Loading ...</p>

                        <div class="bar">
                            <div class="fill" id="fill"></div>
                        </div>

                        <div class="row">
                            <div class="file" id="file">-</div>
                            <div id="pct">0%</div>
                        </div>

                        <div class="row justify-content-end">
                            <div id="speed">0 KB/s</div>
                            <!--<div id="eta">—</div>-->
                        </div>
                    </div>
                </div>

                <script>
                    const elFill = document.getElementById('fill');
                    const elPct = document.getElementById('pct');
                    const elSub = document.getElementById('sub');
                    const elFile = document.getElementById('file');
                    const elSpeed = document.getElementById('speed');
                    //const elEta = document.getElementById('eta');

                    let total = 0, got = 0;
                    let targetPct = 0;
                    let smoothPct = 0;

                    let lastT = performance.now(), lastGot = 0;

                    function fmtBytes(n) {
                        const u=["B","KB","MB","GB"]; let i=0;
                        while(n>=1024 && i<u.length-1){n/=1024;i++;}
                        return `${n.toFixed(i?2:0)} ${u[i]}`;
                    }

                    function tickSpeed(){
                        const now = performance.now();
                        const dt = (now-lastT)/1000;
                        if(dt <= 0.10) return;
                        const dGot = got-lastGot;
                        const sp = dGot/dt;
                        elSpeed.textContent = `${fmtBytes(sp)}/s`;
                        
                        /*if(total>0 && sp>1){
                            const rem = Math.max(0,total-got);
                            const eta = rem/sp;
                            const m = Math.floor(eta/60), s = Math.floor(eta%60);
                            elEta.textContent = `${m}m ${s}s`;
                        }*/

                        lastT = now; lastGot = got;
                    }

                    setInterval(tickSpeed, 200);

                    function raf(){
                        smoothPct += (targetPct - smoothPct) * 0.15;
                        const pct = Math.max(0, Math.min(100, smoothPct));
                        elFill.style.width = pct.toFixed(2) + '%';
                        elPct.textContent = Math.round(pct) + '%';
                        requestAnimationFrame(raf);
                    }

                    requestAnimationFrame(raf);

                    window.__ompcef = {
                        manifest: (files, bytes) => {
                        total = bytes|0;
                        elSub.textContent = `${files} fichier(s) • ${fmtBytes(total)}`;
                    },
                    progress: (fileName, fileReceived, fileTotal, overallReceived, overallTotal) => {
                        got = overallReceived|0;
                        total = overallTotal|0;
                        targetPct = total ? (got/total)*100 : 0;

                        elFile.textContent = fileName || '—';
                        elSub.textContent = `${fmtBytes(fileReceived)} / ${fmtBytes(fileTotal)} • Total ${fmtBytes(got)} / ${fmtBytes(total)}`;
                    },
                    done: () => {
                        got = total;
                        targetPct = 100;
                            elFile.textContent = 'Done';
                            elSub.textContent = 'Loading ...';
                            //elEta.textContent = '—';
                        }
                    };
                </script>
            </body>
        </html>)HTML";
}

static std::string GetInternalYouTubeHtml()
{
    return R"HTML(<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <meta name="referrer" content="origin">
  <title>YouTube</title>
  <style>
    html,body{margin:0;height:100%;background:#000;overflow:hidden}
    iframe{border:0;width:100%;height:100%;display:block}
  </style>
</head>
<body>
  <iframe id="yt"
    sandbox="allow-scripts allow-same-origin allow-presentation allow-popups allow-forms"
    allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share"
    allowfullscreen
    referrerpolicy="origin"></iframe>

  <script>
    (function(){
      const u = new URL(location.href);
      const v = u.searchParams.get('v') || '';
      if(!v){ 
        document.body.innerHTML = '<div style="color:#fff;padding:12px;font-family:system-ui">Missing video id</div>'; 
        return; 
      }

      const autoplay = u.searchParams.get('autoplay') ?? '1';
      const mute = u.searchParams.get('mute') ?? '1';
      const controls = u.searchParams.get('controls') ?? '0';
      const rel = u.searchParams.get('rel') ?? '0';
      const start = u.searchParams.get('start');
      const end = u.searchParams.get('end');

      const p = new URLSearchParams();
      p.set('autoplay', autoplay);
      p.set('mute', mute);
      p.set('controls', controls);
      p.set('rel', rel);
      p.set('playsinline', '1');
      p.set('iv_load_policy', '3');
      p.set('modestbranding', '1');
      p.set('enablejsapi', '1');
      p.set('origin', 'http://cef');  // For 153 error
      if(start) p.set('start', start);
      if(end) p.set('end', end);

      const src = `https://www.youtube.com/embed/${encodeURIComponent(v)}?${p.toString()}`;
      const iframe = document.getElementById('yt');
      iframe.src = src;
      
      iframe.onerror = function() {
        console.error('Failed to load YouTube iframe');
      };
    })();
  </script>
</body>
</html>)HTML";
}

static std::string GetInternalTwitchHtml()
{
    return R"HTML(<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <meta name="referrer" content="origin">
  <title>Twitch</title>
  <style>
    html,body{margin:0;height:100%;background:#000;overflow:hidden}
    iframe{border:0;width:100%;height:100%;display:block}
  </style>
</head>
<body>
  <iframe id="tw"
    sandbox="allow-scripts allow-same-origin allow-presentation allow-popups allow-forms"
    allow="autoplay; fullscreen; picture-in-picture; encrypted-media"
    allowfullscreen
    referrerpolicy="origin"></iframe>

  <script>
    (function(){
      const u = new URL(location.href);
      const params = new URLSearchParams(u.search);

      const hostname = location.hostname || 'cef';
      params.set('parent', hostname);

      const channel = params.get('channel');
      const video = params.get('video');
      const clip = params.get('clip');
      
      if(!channel && !video && !clip){
        document.body.innerHTML = '<div style="color:#fff;padding:12px;font-family:system-ui">Missing Twitch channel/video/clip</div>';
        return;
      }

      let base = 'https://player.twitch.tv/';
      const finalParams = new URLSearchParams();
      
      if(channel) finalParams.set('channel', channel);
      if(video) finalParams.set('video', video);
      if(clip) finalParams.set('collection', clip);
      
      for(const [k, v] of params.entries()){
        if(k !== 'channel' && k !== 'video' && k !== 'clip'){
          finalParams.set(k, v);
        }
      }
      
      finalParams.set('parent', hostname);
      finalParams.set('autoplay', 'true');
      finalParams.set('muted', 'false');

      const src = base + '?' + finalParams.toString();
      const iframe = document.getElementById('tw');
      iframe.src = src;
      
      iframe.onerror = function() {
        console.error('Failed to load Twitch iframe');
      };
    })();
  </script>
</body>
</html>)HTML";
}

LocalSchemeHandlerFactory::LocalSchemeHandlerFactory(ResourceManager& resource_manager)
    : resource_manager_(resource_manager)
{
}

CefRefPtr<CefResourceHandler> LocalSchemeHandlerFactory::Create(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& scheme_name,
    CefRefPtr<CefRequest> request)
{
    CEF_REQUIRE_IO_THREAD();
    return new LocalResourceHandler(resource_manager_);
}

static const std::unordered_map<std::string, std::string>& GetMimeTypeMap()
{
    static const std::unordered_map<std::string, std::string> mime_map = {
        // Web content types
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".mjs", "application/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},

        // Image types
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".webp", "image/webp"},
        {".bmp", "image/bmp"},

        // Audio types
        {".mp3", "audio/mpeg"},
        {".ogg", "audio/ogg"},
        {".wav", "audio/wav"},
        {".m4a", "audio/mp4"},
        {".aac", "audio/aac"},
        {".flac", "audio/flac"},

        // Video types
        {".mp4", "video/mp4"},
        {".webm", "video/webm"},
        {".ogv", "video/ogg"},
        {".avi", "video/x-msvideo"},

        // Font types
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},
        {".eot", "application/vnd.ms-fontobject"},

        // Document/data types
        {".pdf", "application/pdf"},
        {".txt", "text/plain"},
        {".md", "text/markdown"},
    };

    return mime_map;
}

// Optimized MIME type resolver using cache
std::string GetMimeType(const std::string& path)
{
    size_t pos = path.rfind('.');
    if (pos == std::string::npos) {
        return "application/octet-stream";
    }

    std::string ext = path.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return std::tolower(c); });

    const auto& mime_map = GetMimeTypeMap();
    auto it = mime_map.find(ext);
    if (it != mime_map.end()) {
        return it->second;
    }

    return "application/octet-stream";
}

LocalResourceHandler::LocalResourceHandler(ResourceManager& resource_manager)
    : resource_manager_(resource_manager), read_offset_(0)
{
}

bool LocalResourceHandler::ProcessRequest(
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefCallback> callback)
{
    CEF_REQUIRE_IO_THREAD();

    std::string url = request->GetURL();
    std::string path_part;

    if (url.rfind("cef://", 0) == 0) {
        path_part = url.substr(6);
    }
    else if (url.rfind("http://cef/", 0) == 0) {
        path_part = url.substr(11);
    }
    else {
        LOG_DEBUG("[CEF] Unsupported URL scheme: {}", url);
        return false;
    }

    // Strip query parameters and fragments for file resolution
    size_t query_pos = path_part.find('?');
    if (query_pos != std::string::npos) {
        path_part = path_part.substr(0, query_pos);
    }

    size_t fragment_pos = path_part.find('#');
    if (fragment_pos != std::string::npos) {
        path_part = path_part.substr(0, fragment_pos);
    }

    size_t first_slash_pos = path_part.find('/');
    if (first_slash_pos == std::string::npos) {
        LOG_DEBUG("[CEF] Invalid URL format: {}", url);
        return false;
    }

    std::string resource_name = path_part.substr(0, first_slash_pos);
    std::string internal_path = path_part.substr(first_slash_pos + 1);

    // Internal embedded pages
    // http://cef/__internal/loading.html
    // http://cef/__internal/youtube.html?v=...
    // http://cef/__internal/twitch.html?...
    if (resource_name == "__internal")
    {
        if (internal_path == "loading.html")
        {
            const std::string html = GetInternalLoadingHtml();
            data_.assign(html.begin(), html.end());
            mime_type_ = "text/html";
            read_offset_ = 0;
            callback->Continue();
            return true;
        }

        if (internal_path == "youtube.html")
        {
            const std::string html = GetInternalYouTubeHtml();
            data_.assign(html.begin(), html.end());
            mime_type_ = "text/html";
            read_offset_ = 0;
            callback->Continue();
            return true;
        }

        if (internal_path == "twitch.html")
        {
            const std::string html = GetInternalTwitchHtml();
            data_.assign(html.begin(), html.end());
            mime_type_ = "text/html";
            read_offset_ = 0;
            callback->Continue();
            return true;
        }
    }

    // Block files without an extension
    size_t pos = internal_path.rfind('.');
    if (pos == std::string::npos) {
        LOG_WARN("[CEF] Blocked request for file with no extension: {}", internal_path);
        return false;
    }

    std::string ext = internal_path.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    // Block the request if the extension is not in the allowed types.
    const auto& mime_map = GetMimeTypeMap();
    if (mime_map.find(ext) == mime_map.end()) {
        LOG_WARN("[CEF] Blocked request for unallowed file type ({}): {}", ext, internal_path);
        return false;
    }

    // URL decode the path (handle spaces and special characters)
    CefString decoded = CefURIDecode(
        internal_path,
        true,
        static_cast<cef_uri_unescape_rule_t>(UU_SPACES | UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS)
    );

    internal_path = decoded.ToString();

    // Normalize path and prevent directory traversal
    std::replace(internal_path.begin(), internal_path.end(), '\\', '/');
    if (internal_path.find("..") != std::string::npos ||
        internal_path.find("~") != std::string::npos) {
        LOG_DEBUG("[CEF] Path traversal attempt blocked: {}", internal_path);
        return false;
    }

    // Attempt to load file content from ResourceManager
    if (resource_manager_.GetFileContent(resource_name, internal_path, data_)) {
        mime_type_ = GetMimeType(internal_path);
        read_offset_ = 0;

        LOG_DEBUG("[CEF] Successfully loaded: {} from resource '{}'", internal_path, resource_name);
        callback->Continue();
        return true;
    }

    LOG_DEBUG("[CEF] Resource not found: {}/{}", resource_name, internal_path);
    return false;
}

void LocalResourceHandler::GetResponseHeaders(
    CefRefPtr<CefResponse> response,
    int64_t& response_length,
    CefString& redirectUrl)
{
    CEF_REQUIRE_IO_THREAD();

    response->SetMimeType(mime_type_);
    response->SetStatus(200);
    response_length = static_cast<int64_t>(data_.size());
}

bool LocalResourceHandler::ReadResponse(
    void* data_out,
    int bytes_to_read,
    int& bytes_read,
    CefRefPtr<CefCallback> callback)
{
    CEF_REQUIRE_IO_THREAD();

    size_t remaining_bytes = data_.size() - read_offset_;
    if (remaining_bytes == 0) {
        bytes_read = 0;
        return false;
    }

    size_t bytes_to_copy = std::min(static_cast<size_t>(bytes_to_read), remaining_bytes);
    std::memcpy(data_out, data_.data() + read_offset_, bytes_to_copy);

    read_offset_ += bytes_to_copy;
    bytes_read = static_cast<int>(bytes_to_copy);

    return true;
}

void LocalResourceHandler::Cancel()
{
    CEF_REQUIRE_IO_THREAD();

    data_.clear();
    data_.shrink_to_fit();
    read_offset_ = 0;
}