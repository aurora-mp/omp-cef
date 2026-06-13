/*
	This is a SAMP (0.3.DL-1) API project file.
	Developers: LUCHARE <luchare.dev@gmail.com>, ARMOR
	
	See more here https://github.com/LUCHARE/SAMP-API
	
	Copyright (c) 2018 BlastHack Team <BlastHack.Net>. All rights reserved.
*/

#pragma once

#include "sampapi/sampapi.h"

SAMPAPI_BEGIN_PACKED_V03DL_1

class SAMPAPI_EXPORT CCustomModelInfo {
public:
    bool m_nIsDff;
    bool m_nIsTxd;
    unsigned short field_2;
    char field_4;
    char field_5;
    char field_6;
    char field_7;
    unsigned short field_8;
    unsigned short fiend_0A;
    int nStandartModelId;
    int nCustomModelId;
    char field_14[50];
    int m_nDffModelIndex;
    int m_nTxdModelIndex;
};

class SAMPAPI_EXPORT CCustomModelsPool {
public:
    CCustomModelInfo* m_pObject[20000];
};

SAMPAPI_END_PACKED
