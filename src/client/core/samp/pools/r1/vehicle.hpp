#pragma once

#include "samp/pools/game_pool.hpp"
#include <sampapi/0.3.7-R1/CVehicle.h>
#include <sampapi/0.3.7-R1/CVehiclePool.h>

class VehiclePool_R1 : public IVehiclePool
{
public:
    VehiclePool_R1() = default;

    int Find(CVehicle* vehiclePtr) override 
    {
        auto* netGame = sampapi::v037r1::RefNetGame();
        if (!netGame || !netGame->m_pPools || !netGame->m_pPools->m_pVehicle) {
            return -1;
        }

        return netGame->m_pPools->m_pVehicle->Find(vehiclePtr);
    }
};