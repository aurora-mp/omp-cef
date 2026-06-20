#include "netgame.hpp"

sampapi::v037r1::CNetGame* NetGameView_R1::GetNetGame() const
{
    return sampapi::v037r1::RefNetGame();
}

IObjectPool* NetGameView_R1::GetObjectPool() 
{
    return &object_pool_wrapper_;
}

IVehiclePool* NetGameView_R1::GetVehiclePool() 
{
    return &vehicle_pool_wrapper_;
}

std::string NetGameView_R1::GetIp() const
{
    if (const auto* netGame = GetNetGame()) {
        return std::string{ netGame->m_szHostAddress };
    }

    return "";
}

int NetGameView_R1::GetPort() const 
{
    if (const auto* netGame = GetNetGame()) {
        return netGame->m_nPort;
    }

    return -1;
}

SampGameState NetGameView_R1::GetGameState() const
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

int NetGameView_R1::GetLocalPlayerId() const
{
    auto* netGame = GetNetGame();
    if (netGame && netGame->GetPlayerPool()) {
        return netGame->GetPlayerPool()->m_localInfo.m_nId;
    }

    return -1;
}

std::string NetGameView_R1::GetLocalPlayerName() const
{
    auto* netGame = GetNetGame();
    if (netGame && netGame->GetPlayerPool()) {
        return netGame->GetPlayerPool()->m_localInfo.m_szName;
    }

    return "";
}

CEntity* NetGameView_R1::GetEntityFromObjectId(int objectId) 
{
    auto pool = GetObjectPool();
    if (!pool)
        return nullptr;

    auto sampObject = pool->Get(objectId);
    if (!sampObject)
        return nullptr;

    return sampObject->GetGameEntity();
}

IPlayerPool* NetGameView_R1::GetPlayerPool()
{
    return &playerPool_;
}

bool NetGameView_R1::PlayerPoolImpl::GetPlayerPos(int playerId, float& x, float& y, float& z)
{
    auto* netGame = view_->GetNetGame();
    if (!netGame || !netGame->m_pPools || !netGame->m_pPools->m_pPlayer)
        return false;

    auto* playerPool = netGame->m_pPools->m_pPlayer;
    if (playerId == playerPool->m_localInfo.m_nId || playerId == view_->GetLocalPlayerId())
    {
        auto* localPlayer = playerPool->GetLocalPlayer();
        if (localPlayer && localPlayer->m_pPed)
        {
            sampapi::CMatrix mat;
            localPlayer->m_pPed->GetMatrix(&mat);
            x = mat.pos.x;
            y = mat.pos.y;
            z = mat.pos.z;
            return true;
        }
    }
    else
    {
        auto* remotePlayer = playerPool->GetPlayer(playerId);
        if (remotePlayer && remotePlayer->m_pPed)
        {
            sampapi::CMatrix mat;
            remotePlayer->m_pPed->GetMatrix(&mat);
            x = mat.pos.x;
            y = mat.pos.y;
            z = mat.pos.z;
            return true;
        }
    }
    return false;
}

bool NetGameView_R1::PlayerPoolImpl::GetPlayerName(int playerId, std::string& name)
{
    auto* netGame = view_->GetNetGame();
    if (!netGame || !netGame->m_pPools || !netGame->m_pPools->m_pPlayer)
        return false;

    auto* playerPool = netGame->m_pPools->m_pPlayer;
    if (playerId == playerPool->m_localInfo.m_nId || playerId == view_->GetLocalPlayerId())
    {
        name = playerPool->m_localInfo.m_szName;
        return true;
    }
    
    if (playerPool->m_bNotEmpty[playerId])
    {
        name = playerPool->GetName(playerId);
        return true;
    }
    return false;
}

bool NetGameView_R1::PlayerPoolImpl::IsPlayerStreamedIn(int playerId)
{
    auto* netGame = view_->GetNetGame();
    if (!netGame || !netGame->m_pPools || !netGame->m_pPools->m_pPlayer)
        return false;

    auto* playerPool = netGame->m_pPools->m_pPlayer;
    if (playerId == playerPool->m_localInfo.m_nId || playerId == view_->GetLocalPlayerId())
        return true;

    auto* remotePlayer = playerPool->GetPlayer(playerId);
    return remotePlayer && remotePlayer->m_pPed != nullptr;
}