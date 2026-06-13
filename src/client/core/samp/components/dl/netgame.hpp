#pragma once

#include "samp/components/netgame_view.hpp"

#include "samp/pools/dl/object.hpp"
#include "samp/pools/dl/vehicle.hpp"

#include <sampapi/0.3.DL-1/CNetGame.h>
#include <sampapi/0.3.DL-1/CVehiclePool.h>

class NetGameView_DL : public INetGameView 
{
public:
    NetGameView_DL() = default;
    ~NetGameView_DL() override = default;

    std::string GetIp() const override;
    int GetPort() const override;

    SampGameState GetGameState() const override;

    int GetLocalPlayerId() const override;
    std::string GetLocalPlayerName() const override;

    IObjectPool* GetObjectPool() override;
    IVehiclePool* GetVehiclePool() override;

    CEntity* GetEntityFromObjectId(int objectId) override;

private:
    sampapi::v03dl::CNetGame* GetNetGame() const;

    ObjectPool_DL object_pool_wrapper_;
    VehiclePool_DL vehicle_pool_wrapper_;

    enum class RawMode : int
    {
        WaitConnect = 0x1,
        Connecting = 0x2,
        Connected = 0x5,
        WaitJoin = 0x6,
        Restarting = 0xB,
    };
};