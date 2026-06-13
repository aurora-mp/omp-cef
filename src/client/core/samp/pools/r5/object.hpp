#pragma once

#include "samp/pools/game_pool.hpp"
#include <sampapi/0.3.7-R5-1/CNetGame.h>
#include <sampapi/0.3.7-R5-1/CObject.h>
#include <sampapi/0.3.7-R5-1/CObjectPool.h>

class SampObject_R5 : public ISampObject 
{
public:
    SampObject_R5(sampapi::v037r5::CObject* obj) : m_sampObject(obj) {}

    CEntity* GetGameEntity() const override 
    {
        return m_sampObject ? reinterpret_cast<CEntity*>(m_sampObject->m_pGameEntity) : nullptr;
    }
private:
    sampapi::v037r5::CObject* m_sampObject;
};

class ObjectPool_R5 : public IObjectPool 
{
public:
    ObjectPool_R5() = default;

    std::unique_ptr<ISampObject> Get(int objectId) override 
    {
        auto* netGame = sampapi::v037r5::RefNetGame();
        if (!netGame || !netGame->m_pPools || !netGame->m_pPools->m_pObject)
            return nullptr;

        sampapi::v037r5::CObject* sampObject = netGame->m_pPools->m_pObject->Get(objectId);
        if (!sampObject)
            return nullptr;

        return std::make_unique<SampObject_R5>(sampObject);
    }
};