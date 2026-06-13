/*
	This is a SAMP (0.3.DL-1) API project file.
	Developers: LUCHARE <luchare.dev@gmail.com>, ARMOR
	
	See more here https://github.com/LUCHARE/SAMP-API
	
	Copyright (c) 2018 BlastHack Team <BlastHack.Net>. All rights reserved.
*/

#pragma once

#include "sampapi/sampapi.h"
#include "sampapi/CCustomModelsPool.h"

SAMPAPI_BEGIN_PACKED_V03DL_1

class SAMPAPI_EXPORT CCustomModels {
public:
    CCustomModelsPool* m_pModelsPool;
    int m_nModelsCount;
    bool m_bIsPathInit;
    char* m_szPathName;
    char field_D[257];
    char* m_szLocalPath;
    char field_112[8027];

    CCustomModels(IDirect3DDevice9* pDevice);
    ~CCustomModels();

    bool InitDirectory();
    bool DoesDffExist(unsigned int nDffIndex);
    bool DoesTxdExist(unsigned int nTxdIndex);
    bool DeleteDff(unsigned int nDffIndex);
    bool DeleteTxd(unsigned int nTxdIndex);
    bool DeleteTempFile(const char* szFileName);
    bool LoadModel(unsigned int nStandartModelId, unsigned int nCustomModelId, unsigned int nDffIndex, unsigned int nTxdIndex);
    CCustomModelInfo* GetModelInfo(unsigned int nId);
    int GetCount();
    bool ResizePool(unsigned int nId);
    bool DoesExist(unsigned int nId);
    void PushBack(unsigned int nId, CCustomModelInfo* pModelInfo);
    void SetModelInfo(unsigned int nId, CCustomModelInfo* pModelInfo);
};

SAMPAPI_EXPORT SAMPAPI_VAR CCustomModels*& RefCustomModels();

SAMPAPI_END_PACKED
