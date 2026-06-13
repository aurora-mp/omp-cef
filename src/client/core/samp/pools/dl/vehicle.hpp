#pragma once

#include "samp/pools/game_pool.hpp"
#include <sampapi/0.3.DL-1/CVehicle.h>
#include <sampapi/0.3.DL-1/CVehiclePool.h>

class VehiclePool_DL : public IVehiclePool 
{
public:
    VehiclePool_DL() = default;

    int Find(CVehicle* vehiclePtr) override 
    {
        auto* netGame = sampapi::v03dl::RefNetGame();
        if (!netGame || !netGame->m_pPools || !netGame->m_pPools->m_pVehicle) {
            return -1;
        }

        return netGame->m_pPools->m_pVehicle->Find(vehiclePtr);
    }
};