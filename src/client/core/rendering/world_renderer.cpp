#include "world_renderer.hpp"
#include <game_sa/CEntity.h>
#include <game_sa/RenderWare.h>
#include "render_manager.hpp"
#include "system/logger.hpp"

WorldRenderer::~WorldRenderer() = default;

WorldRenderer::WorldRenderer(const std::string& textureName, float width, float height)
    : textureName_(textureName), width_(static_cast<int>(width)), height_(static_cast<int>(height))
{
    LPDIRECT3DDEVICE9 device = RenderManager::Instance().GetDevice();
    if (!device) 
    {
        LOG_ERROR("[WorldRenderer] Direct3D device is null, cannot create texture.");
        width_ = height_ = 0; 
        return;
    }

    viewTexture_ = std::make_unique<ViewTexture>(device, width_, height_);
    rwReplacement_ = viewTexture_->GetRwTexture();
}

void WorldRenderer::OnPaint(const void* buffer, int width, int height)
{
    if (!viewTexture_ || !buffer) 
        return;

    viewTexture_->Update(buffer, width, height);
}

void WorldRenderer::ReplaceTexturesInAtomic(RpAtomic* atomic)
{
    if (!atomic || !atomic->geometry) 
        return;

    for (int i = 0; i < atomic->geometry->matList.numMaterials; ++i) 
    {
        RpMaterial* mat = atomic->geometry->matList.materials[i];
        if (mat && mat->texture && textureName_ == mat->texture->name) 
        {
            backup_.emplace_back(&mat->texture, mat->texture);
            mat->texture = rwReplacement_;
        }
    }
}

void WorldRenderer::SwapTexture(CEntity* entity)
{
    backup_.clear();

    if (!entity || !rwReplacement_ || !entity->m_pRwObject) 
        return;

    RwObject* rwObj = entity->m_pRwObject;
    if (rwObj->type == 1) { // RpAtomic
        ReplaceTexturesInAtomic(reinterpret_cast<RpAtomic*>(rwObj));
    } else if (rwObj->type == 2) { // RpClump
        RpClump* clump = reinterpret_cast<RpClump*>(rwObj);
        RpClumpForAllAtomics(clump, AtomicTextureSwapCallback, this);
    }
}

void WorldRenderer::RestoreTexture()
{
    for (auto& entry : backup_) *(entry.first) = entry.second;
    backup_.clear();
}

RpAtomic* WorldRenderer::AtomicTextureSwapCallback(RpAtomic* atomic, void* userData)
{
    auto* self = reinterpret_cast<WorldRenderer*>(userData);
    self->ReplaceTexturesInAtomic(atomic);
    return atomic;
}

IDirect3DTexture9* WorldRenderer::GetD3DTexture() const
{
    return viewTexture_ ? viewTexture_->GetD3DTexture() : nullptr;
}

void WorldRenderer::OnDeviceLost()
{
    LOG_DEBUG("[WorldRenderer] OnDeviceLost for texture '{}'", textureName_);

    if (viewTexture_)
        viewTexture_->OnDeviceLost();
}

void WorldRenderer::OnDeviceReset(LPDIRECT3DDEVICE9 device)
{
    LOG_DEBUG("[WorldRenderer] OnDeviceReset for texture '{}'", textureName_);

    if (viewTexture_)
        viewTexture_->OnDeviceReset(device);
}