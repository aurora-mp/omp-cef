/*
	This is a SAMP (0.3.DL-1) API project file.
	Developers: LUCHARE <luchare.dev@gmail.com>, ARMOR
	
	See more here https://github.com/LUCHARE/SAMP-API
	
	Copyright (c) 2018 BlastHack Team <BlastHack.Net>. All rights reserved.
*/

#pragma once

#include "sampapi/sampapi.h"
#include "sampapi/CRect.h"
#include "sampapi/0.3.DL-1/CDownloadedFilesPool.h"

SAMPAPI_BEGIN_PACKED_V03DL_1

class SAMPAPI_EXPORT CDownloadWindow {
public:
    CDownloadedFilesPool* m_pFilesPool;
    int m_nDownloadedFilesCount;
    IDirect3DDevice9* m_pDevice;
    unsigned int m_position[2];
    int m_nWindowWidth;
    int m_nWindowHeight;
    int field_1C;
    int field_20;
    int field_24;
    CDXUTDialog* m_pDialog;
    CDXUTListBox* m_pListbox;
    int m_bIsWindowActive;
    int m_nRowCount;
    char m_szType[129];
    char m_szId[129];
    char m_szProcess[129];
    char m_szUnused[129];
    int m_nSpaceSizeForIdText;
    int m_nSpaceSizeForProgressText;
    int field_244;
    int field_248;
    int m_nDownloadedFilesCount_1;
    int m_nFilesSum;
    int field_254;
    int field_258;
    float m_fDownloadSpeed;

    CDownloadWindow(IDirect3DDevice9* pDevice);
    ~CDownloadWindow();

    void GetScreenRect(CRect* pRect);
    void DoesExist(int nFileId);
    void Show();
    void AddEntry(char nFileType, int nFileId, char nFileState, int nFileDownloaded, int nFileRemainDownload, int a7);
    void Draw();
    void ResetDialogControl(CDXUTDialog* pDialog);
    void FreePool();
    void PushBack(DownloadedFileInfo* pInfo);
    void DeleteFile(unsigned int nId);
    void ResizePool(unsigned int nId);
    int GetFilesCount();
    DownloadedFileInfo* GetFileInfo(unsigned int nId);
    

};

SAMPAPI_EXPORT SAMPAPI_VAR CDownloadWindow*& RefDownloadWindow();

SAMPAPI_END_PACKED
