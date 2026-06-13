#pragma once

#include <game_sa/RenderWare.h>
#include <game_sa/CEntity.h>
#include "rendering/view_texture.hpp"

// Replaces a model's texture (by name) with a dynamic CEF rendered texture
class WorldRenderer
{
public:
    WorldRenderer(const std::string& textureName, float width, float height);
    ~WorldRenderer();

    // Push new pixels from CEF into the texture
    void OnPaint(const void* buffer, int width, int height);

    // Swap matching materials' textures to our browser texture before draw
    void SwapTexture(CEntity* entity);

    // Restore original textures after draw.
    void RestoreTexture();

    const std::string& GetTextureName() const { return textureName_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    IDirect3DTexture9* GetD3DTexture() const;

    void OnDeviceLost();
    void OnDeviceReset(LPDIRECT3DDEVICE9 device);

private:
    void ReplaceTexturesInAtomic(RpAtomic* atomic);
    static RpAtomic* AtomicTextureSwapCallback(RpAtomic* atomic, void* userData);

    std::string textureName_;
    int width_ = 0;
    int height_ = 0;

    std::unique_ptr<ViewTexture> viewTexture_;
    RwTexture* rwReplacement_ = nullptr;
    std::vector<std::pair<RwTexture**, RwTexture*>> backup_;
};