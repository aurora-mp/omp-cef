/*
	This is a SAMP (0.3.DL-1) API project file.
	Developers: LUCHARE <luchare.dev@gmail.com>, ARMOR
	
	See more here https://github.com/LUCHARE/SAMP-API
	
	Copyright (c) 2018 BlastHack Team <BlastHack.Net>. All rights reserved.
*/

#include "sampapi/0.3.DL-1/CCustomModels.h"

SAMPAPI_BEGIN_V03DL_1

SAMPAPI_VAR CCustomModels*& RefCustomModels() {
    return *(CCustomModels**)GetAddress(0x2ACA28);
}

CCustomModels::CCustomModels(IDirect3DDevice9* pDevice) {
    ((void(__thiscall*)(CCustomModels*, IDirect3DDevice9*))GetAddress(0xD890))(this, pDevice);
}

CCustomModels::~CCustomModels() {}

bool CCustomModels::InitDirectory() {
    return ((bool(__thiscall*)(CCustomModels*))GetAddress(0xBA40))(this);
}

bool CCustomModels::DoesDffExist(unsigned int nDffIndex) {
    return ((bool(__thiscall*)(CCustomModels*, unsigned int))GetAddress(0xBB10))(this, nDffIndex);
}

bool CCustomModels::DoesTxdExist(unsigned int nTxdIndex) {
    return ((bool(__thiscall*)(CCustomModels*, unsigned int))GetAddress(0xBBA0))(this, nTxdIndex);
}

bool CCustomModels::DeleteDff(unsigned int nDffIndex) {
    return ((bool(__thiscall*)(CCustomModels*, unsigned int))GetAddress(0xC100))(this, nDffIndex);
}

bool CCustomModels::DeleteTxd(unsigned int nTxdIndex) {
    return ((bool(__thiscall*)(CCustomModels*, unsigned int))GetAddress(0xC140))(this, nTxdIndex);
}

bool CCustomModels::DeleteTempFile(const char* szFileName) {
    return ((bool(__thiscall*)(CCustomModels*, const char*))GetAddress(0xC180))(this, szFileName);
}

bool CCustomModels::LoadModel(unsigned int nStandartModelId, unsigned int nCustomModelId, unsigned int nDffIndex, unsigned int nTxdIndex) {
    return ((bool(__thiscall*)(CCustomModels*, unsigned int, unsigned int, unsigned int, unsigned int))GetAddress(0xC1C0))(this, nStandartModelId, nCustomModelId, nDffIndex, nTxdIndex);
}

CCustomModelInfo* CCustomModels::GetModelInfo(unsigned int nId) {
    return ((CCustomModelInfo*(__thiscall*)(CCustomModels*, unsigned int))GetAddress(0xC410))(this, nId);
}

int CCustomModels::GetCount() {
    return ((int(__thiscall*)(CCustomModels*))GetAddress(0xC430))(this);
}

bool CCustomModels::ResizePool(unsigned int nId) {
    return ((bool(__thiscall*)(CCustomModels*, unsigned int))GetAddress(0xC4E0))(this, nId);
}

bool CCustomModels::DoesExist(unsigned int nId) {
    return ((bool(__thiscall*)(CCustomModels*, unsigned int))GetAddress(0xCE30))(this, nId);
}

void CCustomModels::PushBack(unsigned int nId, CCustomModelInfo* pModelInfo) {
    ((void(__thiscall*)(CCustomModels*, unsigned int, CCustomModelInfo*))GetAddress(0xCFF0))(this, nId, pModelInfo);
}

void CCustomModels::SetModelInfo(unsigned int nId, CCustomModelInfo* pModelInfo) {
    ((void(__thiscall*)(CCustomModels*, unsigned int, CCustomModelInfo*))GetAddress(0xD550))(this, nId, pModelInfo);
}

SAMPAPI_END
