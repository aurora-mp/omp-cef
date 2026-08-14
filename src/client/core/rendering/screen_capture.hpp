#pragma once

#include <d3d9.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

struct CapturedScreenFrame
{
    int browser_id = -1;
    int width = 0;
    int height = 0;
    uint32_t sequence = 0;
    uint64_t generation = 0;
    double timestamp_ms = 0.0;
    std::shared_ptr<std::vector<uint8_t>> rgba;
};

class ScreenCaptureManager
{
public:
    struct Configuration
    {
        int width = 640;
        int height = 360;
        int fps = 15;
    };

    ScreenCaptureManager() = default;
    ~ScreenCaptureManager();

    ScreenCaptureManager(const ScreenCaptureManager&) = delete;
    ScreenCaptureManager& operator=(const ScreenCaptureManager&) = delete;

    Configuration Start(int browser_id, int width, int height, int fps);
    void Stop(int browser_id);
    void StopAll();

    bool HasSubscriptions() const;
    bool IsCurrent(int browser_id, uint64_t generation) const;
    void CompleteFrame(int browser_id, uint64_t generation);

    std::vector<std::shared_ptr<CapturedScreenFrame>> CaptureDueFrames(
        IDirect3DDevice9* device,
        uint64_t now_ms);

    void OnDeviceLost();

private:
    struct Subscription
    {
        Configuration config;
        uint64_t generation = 0;
        uint64_t next_capture_ms = 0;
        uint32_t sequence = 0;
        bool frame_pending = false;
    };

    struct DueCapture
    {
        int browser_id = -1;
        Configuration config;
        uint64_t generation = 0;
        uint32_t sequence = 0;
    };

    struct CaptureResources
    {
        IDirect3DSurface9* render_target = nullptr;
        IDirect3DSurface9* system_surface = nullptr;
    };

    static Configuration Sanitize(int width, int height, int fps);
    static uint64_t ResolutionKey(int width, int height) noexcept;

    CaptureResources* GetOrCreateResources(IDirect3DDevice9* device, int width, int height);
    bool CapturePixels(
        IDirect3DDevice9* device,
        int width,
        int height,
        std::vector<uint8_t>& rgba);
    void ReleaseResources();

private:
    mutable std::mutex subscriptions_mutex_;
    std::unordered_map<int, Subscription> subscriptions_;
    uint64_t next_generation_ = 1;

    IDirect3DDevice9* resource_device_ = nullptr;
    std::unordered_map<uint64_t, CaptureResources> resources_;
};
