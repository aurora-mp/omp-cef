#pragma once

#include "samp/pools/game_pool.hpp"

enum class SampGameState
{
    Unknown = -1,
    WaitConnect = 0,
    Connecting = 1,
    Connected = 2,  
    WaitJoin = 3,
    Restarting = 4,
};

class INetGameView 
{
public:
	virtual ~INetGameView() = default;

	virtual std::string GetIp() const = 0;
	virtual int GetPort() const = 0;

    virtual SampGameState GetGameState() const = 0;

    bool IsConnected() const { 
        return GetGameState() == SampGameState::Connected; 
    }

	virtual int GetLocalPlayerId() const = 0;
	virtual std::string GetLocalPlayerName() const = 0;

	virtual IObjectPool* GetObjectPool() = 0;
    virtual IVehiclePool* GetVehiclePool() = 0;

	virtual CEntity* GetEntityFromObjectId(int objectId) = 0;
};