#pragma once

#include <game_sa/CEntity.h>
#include <game_sa/CVehicle.h>

class ISampObject 
{
public:
    virtual ~ISampObject() = default;
    virtual CEntity* GetGameEntity() const = 0;
};

class IObjectPool 
{
public:
    virtual ~IObjectPool() = default;
    virtual std::unique_ptr<ISampObject> Get(int objectId) = 0;
};

class IVehiclePool 
{
public:
    virtual ~IVehiclePool() = default;
    virtual int Find(CVehicle* pVehicle) = 0;
};