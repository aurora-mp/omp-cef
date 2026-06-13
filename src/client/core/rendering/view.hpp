#pragma once

#include "rendering/view_texture.hpp"
#include "include/internal/cef_types.h"
#include "include/cef_render_handler.h"
#include <d3d9.h>

// Manages a D3D9 texture as a render target for a single CEF browser view
class View
{
public:
    View();
    explicit View(int id);

    void Initialize(LPDIRECT3DDEVICE9 device);

    void Create(int width, int height);
    void Resize(int width, int height);

    // Updates the texture using CEF dirty rectangles for optimized partial updates
    void UpdateTexture(const uint8_t* data, const cef_rect_t* dirtyRects, size_t count);

    // Performs a simple full texture update from a raw buffer
    void OnPaint(const void* buffer, int width, int height);

    // Renders the texture at its current position as a 2D overlay
    void Draw();

    // Returns the view's bounding rectangle for CEF
    cef_rect_t rect() const;

    void SetPosition(int x, int y) noexcept { posX_ = x; posY_ = y; }
    void SetActive(bool active) noexcept { active_ = active; }
    void SetFocused(bool focused) noexcept { focused_ = focused; }
    bool IsFocused() const noexcept { return focused_; }

    LPDIRECT3DTEXTURE9 GetTexture() const noexcept {
        return wrapper_ ? wrapper_->GetD3DTexture() : nullptr;
    }

    void OnDeviceLost();
    void OnDeviceReset(LPDIRECT3DDEVICE9 device);

private:
    void InternalCreateTexture(int width, int height);

    int id_ = -1;
    int posX_ = 0;
    int posY_ = 0;
    int width_ = 0;
    int height_ = 0;
    bool active_ = true;
    bool focused_ = false;

    LPDIRECT3DDEVICE9 device_ = nullptr;
    std::unique_ptr<ViewTexture> wrapper_;
};