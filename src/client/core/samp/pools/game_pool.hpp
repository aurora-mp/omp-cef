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

class IPlayerPool
{
public:
    virtual ~IPlayerPool() = default;
    virtual bool GetPlayerPos(int playerId, float& x, float& y, float& z) = 0;
    virtual bool GetPlayerName(int playerId, std::string& name) = 0;
    virtual bool IsPlayerStreamedIn(int playerId) = 0;
};