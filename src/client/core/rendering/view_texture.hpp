#pragma once

#include <d3d9.h>
#include <game_sa/rw/rwcore.h>

// Bridges a D3D9 texture with a RenderWare texture so the same pixel buffer
// can be used both for OSR (CEF) and by the GTA:SA engine.
class ViewTexture
{
public:
    ViewTexture() = default;
    ~ViewTexture();

    ViewTexture(const ViewTexture&) = delete;
    ViewTexture& operator=(const ViewTexture&) = delete;
    ViewTexture(ViewTexture&&) = delete;
    ViewTexture& operator=(ViewTexture&&) = delete;

    ViewTexture(LPDIRECT3DDEVICE9 device, int width, int height);

    bool Create(LPDIRECT3DDEVICE9 device, int width, int height);
    void Update(const void* pixels, int width, int height, bool updateRwRaster = true);
    void UpdatePartial(const void* pixels, int bufferWidth, int bufferHeight, int x, int y, int width, int height, bool updateRwRaster = true);
    void Draw(int x, int y);

    LPDIRECT3DTEXTURE9 GetD3DTexture() const { return texture_; }
    RwTexture* GetRwTexture() { return rwTexture_; }

    void OnDeviceLost();
    void OnDeviceReset(LPDIRECT3DDEVICE9 device);

private:
    void ReleaseResources();

    LPDIRECT3DDEVICE9 device_ = nullptr;
    LPDIRECT3DTEXTURE9 texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;

    RwTexture* rwTexture_ = nullptr;
    RwRaster* rwRaster_ = nullptr;

    bool isLost_ = false;
};