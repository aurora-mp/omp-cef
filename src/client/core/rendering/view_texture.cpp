#include "view_texture.hpp"
#include "system/logger.hpp"
#include <game_sa/RenderWare.h>
#include <cstring>

ViewTexture::ViewTexture(LPDIRECT3DDEVICE9 device, int width, int height)
{
    Create(device, width, height);
}

ViewTexture::~ViewTexture()
{
    ReleaseResources();
}

void ViewTexture::ReleaseResources()
{
    if (texture_) 
    { 
        texture_->Release(); 
        texture_ = nullptr; 
    }

    if (rwTexture_) 
    { 
        RwTextureDestroy(rwTexture_); 
        rwTexture_ = nullptr; 
        rwRaster_ = nullptr; 
    }
}

bool ViewTexture::Create(LPDIRECT3DDEVICE9 device, int width, int height)
{
    ReleaseResources();

    device_ = device; width_ = width; height_ = height;
    LOG_DEBUG("[ViewTexture] Creating texture {}x{}", width, height);

    HRESULT hr = device_->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture_, nullptr);
    if (FAILED(hr)) 
    { 
        LOG_ERROR("[ViewTexture] Failed to create D3D9 texture ({}x{})", width, height); 
        return false; 
    }

    rwRaster_ = RwRasterCreate(width, height, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
    if (!rwRaster_) 
    { 
        LOG_ERROR("[ViewTexture] Failed to create RwRaster"); 
        ReleaseResources(); 
        return false; 
    }

    rwTexture_ = RwTextureCreate(rwRaster_);
    if (!rwTexture_) 
    { 
        LOG_ERROR("[ViewTexture] Failed to create RwTexture from RwRaster"); 
        RwRasterDestroy(rwRaster_); 
        rwRaster_ = nullptr; 
        ReleaseResources(); 
        return false; 
    }

    strncpy_s(rwTexture_->name, "CEF_BROWSER", sizeof(rwTexture_->name));
    return true;
}

void ViewTexture::Update(const void* pixels, int width, int height, bool updateRwRaster)
{
    if (!texture_ || !rwRaster_ || width != width_ || height != height_) 
    {
        if (width > 0 && height > 0) 
        { 
            LOG_WARN("[ViewTexture] Size mismatch or invalid state, recreating (old={}x{}, new={}x{})", width_, height_, width, height); 
            Create(device_, width, height); 
        }

        if (!texture_ || !rwRaster_) 
            return;
    }

    D3DLOCKED_RECT rect;
    if (SUCCEEDED(texture_->LockRect(0, &rect, nullptr, 0)))
    {
        const auto* src = static_cast<const uint8_t*>(pixels);
        auto* dst = static_cast<uint8_t*>(rect.pBits);

        if (rect.Pitch == width * 4) 
        { 
            memcpy(dst, src, static_cast<size_t>(width) * height * 4); 
        }
        else 
        {
            for (int y = 0; y < height; ++y) 
            {
                memcpy(dst + y * rect.Pitch, src + y * width * 4, static_cast<size_t>(width) * 4);
            }
        }

        texture_->UnlockRect(0);
    }

    if (!updateRwRaster)
        return;

    uint8_t* rasterPixels = RwRasterLock(rwRaster_, 0, rwRASTERLOCKWRITE);
    if (rasterPixels) 
    {
        const auto* src = static_cast<const uint8_t*>(pixels);
        const int rwPitch = rwRaster_->stride;
        const int srcPitch = width * 4;

        if (rwPitch == srcPitch) 
        { 
            memcpy(rasterPixels, src, static_cast<size_t>(height) * srcPitch); 
        }
        else 
        {
            for (int y = 0; y < height; ++y) 
            {
                memcpy(rasterPixels + y * rwPitch, src + y * srcPitch, srcPitch);
            }
        }

        RwRasterUnlock(rwRaster_);
    }
}

void ViewTexture::UpdatePartial(const void* pixels, int bufferWidth, int bufferHeight, int x, int y, int width, int height, bool updateRwRaster)
{
    if (!texture_ || !rwRaster_ || !pixels || width <= 0 || height <= 0) 
        return;

    RECT lockRect = { (LONG)x, (LONG)y, (LONG)(x + width), (LONG)(y + height) };
    D3DLOCKED_RECT rect;
    if (SUCCEEDED(texture_->LockRect(0, &rect, &lockRect, 0)))
    {
        const auto* src = static_cast<const uint8_t*>(pixels);
        auto* dst = static_cast<uint8_t*>(rect.pBits);

        for (int row = 0; row < height; ++row) 
        {
            const uint8_t* srcRow = src + ((y + row) * bufferWidth + x) * 4;
            uint8_t* dstRow = dst + (row * rect.Pitch);
            memcpy(dstRow, srcRow, static_cast<size_t>(width) * 4);
        }

        texture_->UnlockRect(0);
    }

    if (!updateRwRaster)
        return;

    uint8_t* rasterPixels = RwRasterLock(rwRaster_, 0, rwRASTERLOCKWRITE);
    if (rasterPixels) 
    {
        const int rwPitch = rwRaster_->stride;
        const auto* src = static_cast<const uint8_t*>(pixels);

        for (int row = 0; row < height; ++row) 
        {
            const uint8_t* srcRow = src + ((y + row) * bufferWidth + x) * 4;
            uint8_t* dstRow = rasterPixels + ((y + row) * rwPitch + x * 4);
            memcpy(dstRow, srcRow, static_cast<size_t>(width) * 4);
        }

        RwRasterUnlock(rwRaster_);
    }
}

void ViewTexture::Draw(int x, int y)
{
    if (!device_ || !texture_) 
        return;

    device_->SetTexture(0, texture_);
    device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    device_->SetRenderState(D3DRS_LIGHTING, FALSE);
    device_->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    device_->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    device_->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    device_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

    const float w = static_cast<float>(width_);
    const float h = static_cast<float>(height_);

    struct Vertex { float x, y, z, rhw; DWORD color; float u, v; };
    const Vertex quad[] = {
        { (float)x - 0.5f,       (float)y - 0.5f,       0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f },
        { (float)(x + w) - 0.5f, (float)y - 0.5f,       0.0f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f },
        { (float)x - 0.5f,       (float)(y + h) - 0.5f, 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 1.0f },
        { (float)(x + w) - 0.5f, (float)(y + h) - 0.5f, 0.0f, 1.0f, 0xFFFFFFFF, 1.0f, 1.0f },
    };

    device_->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    device_->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));
}

void ViewTexture::OnDeviceLost()
{
    LOG_DEBUG("[ViewTexture] Device lost, releasing D3D9 texture only");
    
    // Release ONLY the D3D9 texture (D3DPOOL_MANAGED resource)
    if (texture_) {
        texture_->Release();
        texture_ = nullptr;
    }
    
    isLost_ = true;
}

void ViewTexture::OnDeviceReset(LPDIRECT3DDEVICE9 device)
{
    if (!isLost_) 
    {
        LOG_DEBUG("[ViewTexture] OnDeviceReset called but device wasn't lost, skipping");
        return;
    }
    
    LOG_DEBUG("[ViewTexture] Device reset, recreating D3D9 texture {}x{}", width_, height_);
    
    device_ = device;
    
    // Recreate the D3D9 texture
    HRESULT hr = device_->CreateTexture(width_, height_, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture_, nullptr);
    if (FAILED(hr)) 
    {
        LOG_ERROR("[ViewTexture] Failed to recreate D3D9 texture after reset: HRESULT=0x{:08X}", static_cast<unsigned>(hr));
        return;
    }
    
    // Re-sync pixel data from RenderWare raster to D3D9 texture
    if (rwRaster_) 
    {
        uint8_t* rasterPixels = RwRasterLock(rwRaster_, 0, rwRASTERLOCKREAD);
        if (rasterPixels) 
        {
            D3DLOCKED_RECT rect;
            if (SUCCEEDED(texture_->LockRect(0, &rect, nullptr, 0))) 
            {
                const int rwPitch = rwRaster_->stride;
                const int srcPitch = width_ * 4;
                auto* dst = static_cast<uint8_t*>(rect.pBits);
                
                if (rect.Pitch == rwPitch) 
                {
                    memcpy(dst, rasterPixels, static_cast<size_t>(height_) * srcPitch);
                } 
                else 
                {
                    for (int y = 0; y < height_; ++y) 
                    {
                        memcpy(dst + y * rect.Pitch, rasterPixels + y * rwPitch, srcPitch);
                    }
                }

                texture_->UnlockRect(0);
                LOG_DEBUG("[ViewTexture] Successfully restored pixel data from RwRaster");
            }
            
            RwRasterUnlock(rwRaster_);
        } 
        else 
        {
            LOG_WARN("[ViewTexture] Could not lock RwRaster to restore pixel data");
        }
    }
    
    isLost_ = false;
    LOG_DEBUG("[ViewTexture] D3D9 texture recreation complete");
}