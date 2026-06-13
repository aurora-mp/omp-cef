#pragma once

#include "samp/pools/game_pool.hpp"
#include <sampapi/0.3.7-R1/CNetGame.h>
#include <sampapi/0.3.7-R1/CObject.h>
#include <sampapi/0.3.7-R1/CObjectPool.h>

class SampObject_R1 : public ISampObject 
{
public:
    SampObject_R1(sampapi::v037r1::CObject* obj) : m_sampObject(obj) {}

    CEntity* GetGameEntity() const override 
    {
        return m_sampObject ? reinterpret_cast<CEntity*>(m_sampObject->m_pGameEntity) : nullptr;
    }
private:
    sampapi::v037r1::CObject* m_sampObject;
};

class ObjectPool_R1 : public IObjectPool 
{
public:
    ObjectPool_R1() = default;

    std::unique_ptr<ISampObject> Get(int objectId) override 
    {
        auto* netGame = sampapi::v037r1::RefNetGame();
        if (!netGame || !netGame->m_pPools || !netGame->m_pPools->m_pObject)
            return nullptr;

        sampapi::v037r1::CObject* sampObject = netGame->m_pPools->m_pObject->Get(objectId);
        if (!sampObject)
            return nullptr;

        return std::make_unique<SampObject_R1>(sampObject);
    }
};