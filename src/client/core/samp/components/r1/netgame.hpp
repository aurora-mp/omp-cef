#pragma once

#include "samp/components/netgame_view.hpp"

#include "samp/pools/r1/object.hpp"
#include "samp/pools/r1/vehicle.hpp"

#include <sampapi/0.3.7-R1/CNetGame.h>
#include <sampapi/0.3.7-R1/CVehiclePool.h>

class NetGameView_R1 : public INetGameView 
{
public:
    NetGameView_R1() = default;
    ~NetGameView_R1() override = default;

    std::string GetIp() const override;
    int GetPort() const override;

    SampGameState GetGameState() const override;

    int GetLocalPlayerId() const override;
    std::string GetLocalPlayerName() const override;

    IObjectPool* GetObjectPool() override;
    IVehiclePool* GetVehiclePool() override;
    IPlayerPool* GetPlayerPool() override;

    CEntity* GetEntityFromObjectId(int objectId) override;

private:
    sampapi::v037r1::CNetGame* GetNetGame() const;

private:
    class PlayerPoolImpl : public IPlayerPool
    {
    public:
        PlayerPoolImpl(NetGameView_R1* view) : view_(view) {}

        bool GetPlayerPos(int playerId, float& x, float& y, float& z) override;
        bool GetPlayerName(int playerId, std::string& name) override;
        bool IsPlayerStreamedIn(int playerId) override;
    private:
        NetGameView_R1* view_;
    };

    PlayerPoolImpl playerPool_{this};

    ObjectPool_R1 object_pool_wrapper_;
    VehiclePool_R1 vehicle_pool_wrapper_;

    enum class RawMode : int
    {
        WaitConnect = 9,
        Connecting = 13,
        Connected = 14,
        WaitJoin = 15,
        Restarting = 18,
    };
};