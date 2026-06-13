#pragma once

#include "samp/components/netgame_view.hpp"

#include "samp/pools/r5/object.hpp"
#include "samp/pools/r5/vehicle.hpp"

#include <sampapi/0.3.7-R5-1/CNetGame.h>
#include <sampapi/0.3.7-R5-1/CVehiclePool.h>

class NetGameView_R5 : public INetGameView 
{
public:
    NetGameView_R5() = default;
    ~NetGameView_R5() override = default;

    std::string GetIp() const override;
    int GetPort() const override;

    SampGameState GetGameState() const override;

    int GetLocalPlayerId() const override;
    std::string GetLocalPlayerName() const override;

    IObjectPool* GetObjectPool() override;
    IVehiclePool* GetVehiclePool() override;

    CEntity* GetEntityFromObjectId(int objectId) override;

private:
    sampapi::v037r5::CNetGame* GetNetGame() const;

    ObjectPool_R5 object_pool_wrapper_;
    VehiclePool_R5 vehicle_pool_wrapper_;

    enum class RawMode : int
    {
        WaitConnect = 0x1,
        Connecting = 0x2,
        Connected = 0x5,
        WaitJoin = 0x6,
        Restarting = 0xB,
    };
};