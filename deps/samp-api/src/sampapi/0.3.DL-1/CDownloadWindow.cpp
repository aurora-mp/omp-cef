/*
	This is a SAMP (0.3.DL-1) API project file.
	Developers: LUCHARE <luchare.dev@gmail.com>, ARMOR
	
	See more here https://github.com/LUCHARE/SAMP-API
	
	Copyright (c) 2018 BlastHack Team <BlastHack.Net>. All rights reserved.
*/

#include "sampapi/0.3.DL-1/CDownloadWindow.h"

SAMPAPI_BEGIN_V03DL_1

SAMPAPI_VAR CDownloadWindow*& RefDownloadWindow() {
    return *(CDownloadWindow**)GetAddress(0x2AC9E8);
}

CDownloadWindow::CDownloadWindow(IDirect3DDevice9* pDevice) {
    ((void(__thiscall*)(CDownloadWindow*, IDirect3DDevice9*))GetAddress(0x6AA10))(this, pDevice);
}

CDownloadWindow::~CDownloadWindow() {
    ((void(__thiscall*)(CDownloadWindow*))GetAddress(0x6A6E0))(this);
}

void CDownloadWindow::GetScreenRect(CRect* pRect) {
    ((void(__thiscall*)(CDownloadWindow*, CRect*))GetAddress(0x6A280))(this, pRect);
}

void CDownloadWindow::DoesExist(int nFileId) {
    ((void(__thiscall*)(CDownloadWindow*, int))GetAddress(0x6A610))(this, nFileId);
}

void CDownloadWindow::AddEntry(char nFileType, int nFileId, char nFileState, int nFileDownloaded, int nFileRemainDownload, int a7) {
    ((void(__thiscall*)(CDownloadWindow*, char, int, char, int, int, int))GetAddress(0x6A720))(this, nFileType, nFileId, nFileState, nFileDownloaded, nFileRemainDownload, a7);
}

void CDownloadWindow::Show() {
    ((void(__thiscall*)(CDownloadWindow*))GetAddress(0x6A2B0))(this);
}

void CDownloadWindow::ResetDialogControl(CDXUTDialog* pDialog) {
    ((void(__thiscall*)(CDownloadWindow*, CDXUTDialog*))GetAddress(0x6A6E0))(this, pDialog);
}

void CDownloadWindow::Draw() {
    ((void(__thiscall*)(CDownloadWindow*))GetAddress(0x6A350))(this);
}

void CDownloadWindow::FreePool() {
    ((void(__thiscall*)(CDownloadWindow*))GetAddress(0x6A6E0))(this);
}

void CDownloadWindow::PushBack(DownloadedFileInfo* pInfo) {
    ((void(__thiscall*)(CDownloadWindow*, DownloadedFileInfo*))GetAddress(0x6A650))(this, pInfo);
}

void CDownloadWindow::DeleteFile(unsigned int nId) {
    ((void(__thiscall*)(CDownloadWindow*, unsigned int))GetAddress(0x6A6B0))(this, nId);
}

void CDownloadWindow::ResizePool(unsigned int nId) {
    ((void(__thiscall*)(CDownloadWindow*, unsigned int))GetAddress(0x6A450))(this, nId);
}

int CDownloadWindow::GetFilesCount() {
    return ((int(__thiscall*)(CDownloadWindow*))GetAddress(0x6A440))(this);
}


DownloadedFileInfo* CDownloadWindow::GetFileInfo(unsigned int nId) {
    return ((DownloadedFileInfo*(__thiscall*)(CDownloadWindow*, unsigned int))GetAddress(0x6A420))(this, nId);
}

SAMPAPI_END