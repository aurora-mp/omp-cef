#include "screen_capture.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <utility>

#include "system/logger.hpp"

namespace
{
    constexpr int kDefaultWidth = 640;
    constexpr int kDefaultHeight = 360;
    constexpr int kDefaultFps = 15;
    constexpr int kMinDimension = 16;
    constexpr int kMaxWidth = 1280;
    constexpr int kMaxHeight = 720;
    constexpr int kMinFps = 1;
    constexpr int kMaxFps = 30;
    constexpr int kMaxPixelsPerSecond = 1280 * 720 * 15;
}

ScreenCaptureManager::~ScreenCaptureManager()
{
    ReleaseResources();
}

ScreenCaptureManager::Configuration ScreenCaptureManager::Sanitize(int width, int height, int fps)
{
    Configuration config;
    config.width = std::clamp(width > 0 ? width : kDefaultWidth, kMinDimension, kMaxWidth);
    config.height = std::clamp(height > 0 ? height : kDefaultHeight, kMinDimension, kMaxHeight);
    config.fps = std::clamp(fps > 0 ? fps : kDefaultFps, kMinFps, kMaxFps);
    const int pixel_rate_fps = std::max(
        kMinFps,
        kMaxPixelsPerSecond / (config.width * config.height));
    config.fps = std::min(config.fps, pixel_rate_fps);
    return config;
}

ScreenCaptureManager::Configuration ScreenCaptureManager::Start(
    int browser_id,
    int width,
    int height,
    int fps)
{
    const Configuration config = Sanitize(width, height, fps);

    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto& subscription = subscriptions_[browser_id];
    subscription.config = config;
    subscription.generation = next_generation_++;
    subscription.next_capture_ms = 0;
    subscription.sequence = 0;
    subscription.frame_pending = false;
    return config;
}

void ScreenCaptureManager::Stop(int browser_id)
{
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_.erase(browser_id);
}

void ScreenCaptureManager::StopAll()
{
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_.clear();
}

bool ScreenCaptureManager::HasSubscriptions() const
{
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    return !subscriptions_.empty();
}

bool ScreenCaptureManager::IsCurrent(int browser_id, uint64_t generation) const
{
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    const auto it = subscriptions_.find(browser_id);
    return it != subscriptions_.end() && it->second.generation == generation;
}

void ScreenCaptureManager::CompleteFrame(int browser_id, uint64_t generation)
{
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    const auto it = subscriptions_.find(browser_id);
    if (it != subscriptions_.end() && it->second.generation == generation)
        it->second.frame_pending = false;
}

uint64_t ScreenCaptureManager::ResolutionKey(int width, int height) noexcept
{
    return (static_cast<uint64_t>(static_cast<uint32_t>(width)) << 32) |
        static_cast<uint32_t>(height);
}

ScreenCaptureManager::CaptureResources* ScreenCaptureManager::GetOrCreateResources(
    IDirect3DDevice9* device,
    int width,
    int height)
{
    if (!device)
        return nullptr;

    if (resource_device_ != device)
    {
        ReleaseResources();
        resource_device_ = device;
    }

    const uint64_t key = ResolutionKey(width, height);
    auto existing = resources_.find(key);
    if (existing != resources_.end())
        return &existing->second;

    CaptureResources resources;
    HRESULT hr = device->CreateRenderTarget(
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE,
        0,
        FALSE,
        &resources.render_target,
        nullptr);

    if (FAILED(hr) || !resources.render_target)
    {
        LOG_ERROR("[ScreenCapture] CreateRenderTarget failed: 0x{:08X}", static_cast<unsigned int>(hr));
        return nullptr;
    }

    hr = device->CreateOffscreenPlainSurface(
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM,
        &resources.system_surface,
        nullptr);

    if (FAILED(hr) || !resources.system_surface)
    {
        LOG_ERROR("[ScreenCapture] CreateOffscreenPlainSurface failed: 0x{:08X}", static_cast<unsigned int>(hr));
        resources.render_target->Release();
        return nullptr;
    }

    return &resources_.emplace(key, resources).first->second;
}

bool ScreenCaptureManager::CapturePixels(
    IDirect3DDevice9* device,
    int width,
    int height,
    std::vector<uint8_t>& rgba)
{
    auto* resources = GetOrCreateResources(device, width, height);
    if (!resources)
        return false;

    IDirect3DSurface9* back_buffer = nullptr;
    HRESULT hr = device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
    if (FAILED(hr) || !back_buffer)
        return false;

    hr = device->StretchRect(back_buffer, nullptr, resources->render_target, nullptr, D3DTEXF_LINEAR);
    back_buffer->Release();
    if (FAILED(hr))
        return false;

    hr = device->GetRenderTargetData(resources->render_target, resources->system_surface);
    if (FAILED(hr))
        return false;

    D3DLOCKED_RECT locked{};
    hr = resources->system_surface->LockRect(&locked, nullptr, D3DLOCK_READONLY);
    if (FAILED(hr) || !locked.pBits)
        return false;

    const size_t row_bytes = static_cast<size_t>(width) * 4;
    rgba.resize(row_bytes * static_cast<size_t>(height));

    const auto* source = static_cast<const uint8_t*>(locked.pBits);
    auto* destination = rgba.data();
    for (int y = 0; y < height; ++y)
    {
        const auto* source_row = source + static_cast<ptrdiff_t>(y) * locked.Pitch;
        auto* destination_row = destination + static_cast<size_t>(y) * row_bytes;

        for (int x = 0; x < width; ++x)
        {
            const size_t offset = static_cast<size_t>(x) * 4;
            destination_row[offset + 0] = source_row[offset + 2];
            destination_row[offset + 1] = source_row[offset + 1];
            destination_row[offset + 2] = source_row[offset + 0];
            destination_row[offset + 3] = 255;
        }
    }

    resources->system_surface->UnlockRect();
    return true;
}

std::vector<std::shared_ptr<CapturedScreenFrame>> ScreenCaptureManager::CaptureDueFrames(
    IDirect3DDevice9* device,
    uint64_t now_ms)
{
    std::vector<DueCapture> due;
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        due.reserve(subscriptions_.size());

        for (auto& [browser_id, subscription] : subscriptions_)
        {
            if (subscription.frame_pending || now_ms < subscription.next_capture_ms)
                continue;

            subscription.frame_pending = true;
            subscription.next_capture_ms = now_ms + std::max<uint64_t>(1, 1000u / subscription.config.fps);
            due.push_back(DueCapture{
                browser_id,
                subscription.config,
                subscription.generation,
                ++subscription.sequence
            });
        }
    }

    using Resolution = std::pair<int, int>;
    std::map<Resolution, std::vector<DueCapture>> grouped;
    for (const auto& capture : due)
        grouped[{capture.config.width, capture.config.height}].push_back(capture);

    std::vector<std::shared_ptr<CapturedScreenFrame>> frames;
    frames.reserve(due.size());

    for (const auto& [resolution, captures] : grouped)
    {
        auto pixels = std::make_shared<std::vector<uint8_t>>();
        if (!CapturePixels(device, resolution.first, resolution.second, *pixels))
        {
            for (const auto& capture : captures)
                CompleteFrame(capture.browser_id, capture.generation);
            continue;
        }

        for (const auto& capture : captures)
        {
            auto frame = std::make_shared<CapturedScreenFrame>();
            frame->browser_id = capture.browser_id;
            frame->width = resolution.first;
            frame->height = resolution.second;
            frame->sequence = capture.sequence;
            frame->generation = capture.generation;
            frame->timestamp_ms = static_cast<double>(now_ms);
            frame->rgba = pixels;
            frames.push_back(std::move(frame));
        }
    }

    return frames;
}

void ScreenCaptureManager::ReleaseResources()
{
    for (auto& [key, resources] : resources_)
    {
        if (resources.system_surface)
            resources.system_surface->Release();
        if (resources.render_target)
            resources.render_target->Release();
    }

    resources_.clear();
    resource_device_ = nullptr;
}

void ScreenCaptureManager::OnDeviceLost()
{
    ReleaseResources();
}
