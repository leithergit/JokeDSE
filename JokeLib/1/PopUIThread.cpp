// PopUIThread.cpp : implementation file
//

#include "stdafx.h"
#include "PopUIThread.h"
#include "DialogCopyFile.h"
#include "AssistLib.h"

// CPopUIThread
extern AFX_MODULE_STATE *g_pModule_State;
IMPLEMENT_DYNCREATE(CPopUIThread, CWinApp)

CPopUIThread::CPopUIThread()
{
	m_hThread = nullptr;
}

CPopUIThread::~CPopUIThread()
{
}

BOOL CPopUIThread::InitInstance()
{
	CWinApp::InitInstance();
	AfxSetModuleState(g_pModule_State);
	//StartThread();
	CDialogCopyFile dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();
	return TRUE;
}

int CPopUIThread::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CPopUIThread, CWinApp)
END_MESSAGE_MAP()


// CPopUIThread message handlers
void CPopUIThread::StartThread()
{
	m_bSendThreadRun = true;
	for (int i = 0; i < sizeof(m_hThreadArray) / sizeof(HANDLE); i++)
	{
		m_hThreadArray[i] = (HANDLE)_beginthreadex(0, 0, ThreadSendFile, this, 0, 0);
	}
}

void CPopUIThread::StopThread()
{
	m_bSendThreadRun = false;
	::WaitForMultipleObjects(sizeof(m_hThreadArray) / sizeof(HANDLE), m_hThreadArray, TRUE, INFINITE);
	for (int i = 0; i < sizeof(m_hThreadArray) / sizeof(HANDLE); i++)
	{
		CloseHandle(m_hThreadArray[i]);
		m_hThreadArray[i] = nullptr;
	}
}

UINT CPopUIThread::SendFileRun()
{
	FileInfoPtr pFileInfp;
	while (m_bSendThreadRun)
	{
		theApp.m_csList.Lock();
		if (theApp.m_FileList.size() > 0)
		{
			pFileInfp = theApp.m_FileList.front();
			theApp.m_FileList.pop_front();

		}
		theApp.m_csList.Unlock();
		if (pFileInfp)
		{
			SendFile(pFileInfp->strDir, pFileInfp->strFile);
			pFileInfp.reset();
		}

		Sleep(10);
	}
	return 0;
}

bool CPopUIThread::SendFile(CString strFilePath, CString strFile, WORD nPort)
{
	if (PathFileExists(strFile))
	{
		shared_ptr<CSockClient> pClient = make_shared<CSockClient>();
		if (pClient->ConnectServer(_T("192.168.1.7"), nPort, 1500) != INVALID_SOCKET)
		{
			CFile fileRead(strFile, CFile::modeRead);
			CHAR  szHeader[2048] = { 0 };
			CHAR szPath[1024] = { 0 };
			strcpy_s(szPath, 1024, (LPCTSTR)strFilePath);

			sprintf_s(szHeader, 2048, "HeaderLength:%08x;FileLength:%08x;FileDSE:%s;####\n", 1024, (UINT)fileRead.GetLength(), szPath);
			int nHeaderLen = strlen(szHeader);
			sprintf_s(szHeader, 2048, "HeaderLength:%08x;FileLength:%08x;FileDSE:%s;####\n", nHeaderLen, (UINT)fileRead.GetLength(), szPath);
			int nHeaderLength = 0, nFileLength = 0;

			CHAR  *szBuffer = new CHAR[(int)fileRead.GetLength() + nHeaderLen];
			shared_ptr<char> pBuffPtr(szBuffer);
			memcpy(szBuffer, szHeader, nHeaderLen);
			int nSize = fileRead.Read(&szBuffer[nHeaderLen], fileRead.GetLength());
			if (pClient->Send((byte *)szBuffer, fileRead.GetLength() + nHeaderLen) == (fileRead.GetLength() + nHeaderLen))
			{
				TraceMsgA("%s File Length = %d\tFile:%s .\n", __FUNCTION__, (int)(fileRead.GetLength() + nHeaderLen), strFile);
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;
}