#include "netgame.hpp"

sampapi::v03dl::CNetGame* NetGameView_DL::GetNetGame() const
{
    return sampapi::v03dl::RefNetGame();
}

IObjectPool* NetGameView_DL::GetObjectPool() 
{
    return &object_pool_wrapper_;
}

IVehiclePool* NetGameView_DL::GetVehiclePool() 
{
    return &vehicle_pool_wrapper_;
}

std::string NetGameView_DL::GetIp() const
{
    if (const auto* netGame = GetNetGame()) {
        return std::string{ netGame->m_szHostAddress };
    }

    return "";
}

int NetGameView_DL::GetPort() const 
{
    if (const auto* netGame = GetNetGame()) {
        return netGame->m_nPort;
    }

    return -1;
}

SampGameState NetGameView_DL::GetGameState() const
{
    auto* netGame = GetNetGame();
    if (!netGame)
        return SampGameState::Unknown;

    switch (static_cast<RawMode>(netGame->GetState()))
    {
        case RawMode::WaitConnect: 
            return SampGameState::WaitConnect;
        case RawMode::Connecting:  
            return SampGameState::Connecting;
        case RawMode::Connected:   
            return SampGameState::Connected;
        case RawMode::WaitJoin:    
            return SampGameState::WaitJoin;
        case RawMode::Restarting:  
            return SampGameState::Restarting;
        default:                   
            return SampGameState::Unknown;
    }
}

int NetGameView_DL::GetLocalPlayerId() const
{
    auto* netGame = GetNetGame();
    if (netGame && netGame->GetPlayerPool()) {
        return netGame->GetPlayerPool()->m_nLocalPlayerId;
    }

    return -1;
}

std::string NetGameView_DL::GetLocalPlayerName() const
{
    auto* netGame = GetNetGame();
    if (netGame && netGame->GetPlayerPool()) {
        return netGame->GetPlayerPool()->m_szLocalPlayerName;
    }

    return "";
}

CEntity* NetGameView_DL::GetEntityFromObjectId(int objectId) 
{
    auto pool = GetObjectPool();
    if (!pool)
        return nullptr;

    auto sampObject = pool->Get(objectId);
    if (!sampObject)
        return nullptr;

    return sampObject->GetGameEntity();
}