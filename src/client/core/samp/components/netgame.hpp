#pragma once

#include "component.hpp"
#include "netgame_view.hpp"

class NetGameComponent : public ISampComponent 
{
public:
    void Initialize() override;

    std::string GetIp() const;
    int GetPort() const;

    int GetState() const;
    bool IsConnected() const;

    int GetLocalPlayerId() const;
    std::string GetLocalPlayerName() const;

    IObjectPool* GetObjectPool();
    IVehiclePool* GetVehiclePool();

    CEntity* GetEntityFromObjectId(int objectId);

private:
    std::unique_ptr<INetGameView> view_;
};
