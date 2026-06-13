#pragma once

#include "samp/pools/game_pool.hpp"
#include <sampapi/0.3.DL-1/CNetGame.h>
#include <sampapi/0.3.DL-1/CObject.h>
#include <sampapi/0.3.DL-1/CObjectPool.h>

class SampObject_DL : public ISampObject 
{
public:
    SampObject_DL(sampapi::v03dl::CObject* obj) : m_sampObject(obj) {}

    CEntity* GetGameEntity() const override 
    {
        return m_sampObject ? reinterpret_cast<CEntity*>(m_sampObject->m_pGameEntity) : nullptr;
    }
private:
    sampapi::v03dl::CObject* m_sampObject;
};

class ObjectPool_DL : public IObjectPool 
{
public:
    ObjectPool_DL() = default;

    std::unique_ptr<ISampObject> Get(int objectId) override 
    {
        auto* netGame = sampapi::v03dl::RefNetGame();
        if (!netGame || !netGame->m_pPools || !netGame->m_pPools->m_pObject)
            return nullptr;

        sampapi::v03dl::CObject* sampObject = netGame->m_pPools->m_pObject->Get(objectId);
        if (!sampObject)
            return nullptr;

        return std::make_unique<SampObject_DL>(sampObject);
    }
};