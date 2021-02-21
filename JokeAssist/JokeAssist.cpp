// JokeAssist.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "JokeAssist.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CJokeAssistApp
#include "SocketUtils.h"

#pragma comment(lib,"winmm.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma data_seg("MySection")
bool bJoked = false;
bool bJokeInjectioned = false;
HMODULE g_hJokeLib = nullptr;
TCHAR szSourcePath[1024] = { 0 };
TCHAR szFilter[4096] = { 0 };
#pragma data_seg()
#pragma comment(linker, "/SECTION:MySection,RWS")

AFX_MODULE_STATE *g_pModule_State = nullptr;

void WaitForAttach(bool bWait = false)
{
	while (bWait)
	{
		Sleep(50);
	}
}

void CALLBACK TimerCallBack(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	CJokeAssistApp *pThis = (CJokeAssistApp*)dwUser;
	timeKillEvent(pThis->m_hTimer);
	pThis->m_hJokeRun = (HANDLE)_beginthreadex(nullptr, 0, pThis->ThreadFindFiles, pThis, 0, 0);
}


BEGIN_MESSAGE_MAP(CJokeAssistApp, CWinApp)
END_MESSAGE_MAP()


// CJokeAssistApp construction

CJokeAssistApp::CJokeAssistApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CJokeAssistApp object

CJokeAssistApp theApp;


// CJokeAssistApp initialization

BOOL CJokeAssistApp::InitInstance()
{
	CWinApp::InitInstance();
	m_hModule = AfxGetInstanceHandle();
	
	CSocketServer::InitializeWinsock();
	HANDLE hJokeEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _JokeEvent);
	if (!hJokeEvent)
		return TRUE;
	if (WaitForSingleObject(hJokeEvent,0) == WAIT_OBJECT_0)
	{
		g_hJokeLib = m_hModule;
		bJoked = true;
		hEventStop = OpenEvent(EVENT_ALL_ACCESS, FALSE, _StopEvent);
		hEventFinished = OpenEvent(EVENT_ALL_ACCESS, FALSE, _FinishedEvent);
		if (hEventFinished)
			timeSetEvent(100, 1, (LPTIMECALLBACK)TimerCallBack, (DWORD_PTR)this, TIME_ONESHOT | TIME_CALLBACK_FUNCTION);
	}
	CloseHandle(hJokeEvent);
	hJokeEvent = nullptr;

	return TRUE;
}

bool CJokeAssistApp::CheckFilter(CString strFile, bool bExt)
{
	CString strFileupper = strFile;
	strFileupper.MakeUpper();
	CString strCompare = _T("");
	bool bMatched = false;
	if (!bExt)
	{
		int nPos = strFileupper.ReverseFind(L'\\');
		// 找到'\'并且不是最后一个字符
		if (nPos >= 0 && nPos != (strFileupper.GetLength() - 2))
			strCompare = strFileupper.Right(nPos);
	}
	else
		strCompare = PathFindExtension(strFileupper);

	if (strCompare.GetLength() > 0)
	{
		int nPos = m_strFilter.Find(strCompare);
		if (nPos >= 0)
		{
			nPos += strCompare.GetLength();
			int nLength = m_strFilter.GetLength();
			if ((m_strFilter[nPos] == L' ' || m_strFilter[nPos] == L';')
				|| m_strFilter.GetLength() == nPos)
				bMatched = true;
		}
	}
	return bMatched;
}

//void CJokeAssistApp::AccessFile(CString strFile, void *pParam)
//{
//	CJokeAssistApp *pThis = (CJokeAssistApp *)pParam;
//	CString strReletivePath = strFile.Right(strFile.GetLength() - pThis->m_nDirLength);
//	
//	//SendFile(pThis, strReletivePath, strFile, 55555);
//	
//}

#define  SendThreads	(1)
UINT CJokeAssistApp::FindFiles()
{
	WaitForAttach(/*true*/);
	m_bSendThreadRun = true;
	HANDLE	hThreadArray[SendThreads] = { 0 };
	for (int i = 0; i < SendThreads; i++)
		hThreadArray[i] = (HANDLE)_beginthreadex(nullptr, 0, ThreadSendFiles, this, 0, 0);

	m_strDirectory = szSourcePath;
	m_strFilter = szFilter;
	m_nDirLength = m_strDirectory.GetLength();
	ULONGLONG nTotalFileSize = 0;
	if (m_nDirLength > 0)
		AccessDirectory(m_strDirectory/*, AccessFile, this*/);
	// 必须等待所有发送线程结束
	TraceMsgA("%s TotalFileSize = %I64u.\n", __FUNCTION__, m_nTotalFileSize);
	WaitForMultipleObjects(SendThreads, hThreadArray, TRUE, INFINITE);
	for (int i = 0; i < SendThreads; i++)
		CloseHandle(hThreadArray[i]);
	SetEvent(hEventFinished);
	return 0;
}

UINT CJokeAssistApp::SendThread()
{
	CSockClientPtr pClient = make_shared<CSockClient>();
	USHORT nPort = 55555;
	if (pClient->Connect(_T("127.0.0.1"), nPort, 5000) == INVALID_SOCKET)
		return -1;
	pClient->EnableTCPDelay(TRUE);
	int nBufferSize = 1 * 1024 * 1024;
	shared_ptr<byte>pBuffer = shared_ptr<byte>(new byte[nBufferSize]);
	while (m_bSendThreadRun)
	{
		do
		{
			try
			{
				AutoTrylockAgent(m_csFileListtoSend);
				if (m_FileListtoSend.size())
				{
					FileInfoPtr pFile = m_FileListtoSend.front();
					m_FileListtoSend.pop_front();
					UnlockAgent();
					if (!SendFile(pFile->strDir, pFile->strFile, pClient, pBuffer.get(), nBufferSize))
					{
						TraceMsgW(L"%s Failed in sending file:%s.\n", __FUNCTIONW__, pFile->strFile);
					}
				}
				else
					Sleep(10);
			}
			catch (std::exception & e)
			{
			}
			
		} while (0);
		Sleep(20);
	}
	TraceMsgA("%s Send Bytes = %I64u", __FUNCTION__, pClient->m_nTotalSend);
	return 0;
}

void CJokeAssistApp::AccessDirectory(CString strDirectory)
{
	CFileFind finder;
	
	if (strDirectory.GetAt(m_nDirLength - 1) == _T('\\'))
		strDirectory.SetAt(m_nDirLength - 1, _T('\\'));
	CString strFilePath = strDirectory + _T("\\*.*");
	BOOL bWorking = finder.FindFile(strFilePath);
	m_strFilter.MakeUpper();

	while (bWorking)
	{
		if (WaitForSingleObject(hEventStop,0) == WAIT_OBJECT_0)
			break;
		bWorking = finder.FindNextFile();
		if (finder.IsDots())
			continue;

		CString strFile = finder.GetFilePath();
		CString strTemp = strFile;
		strTemp.MakeUpper();
		m_nFoundFiles++;
		if (CheckFilter(strTemp) ||
			CheckFilter(strTemp, false))
		{// 忽略目录
			m_nFiltedFiles++;
			continue;
		}
		if (finder.IsDirectory())
		{
			AccessDirectory(finder.GetFilePath());
		}
		else
		{
			m_nTotalFileSize += finder.GetLength();
			CString strReletivePath = strFile.Right(strFile.GetLength() - m_nDirLength);
			m_csFileListtoSend.Lock();
			m_FileListtoSend.push_back(make_shared<FileInfo>(strReletivePath, strFile));
			m_csFileListtoSend.Unlock();
			//DWORD dwT1 = timeGetTime();
			//bool bSent = SendFile(strReletivePath, strFile, );
			//DWORD dwTimeSpan = timeGetTime() - dwT1;
			//TraceMsgW(L"[%05d][%d][%04d] = %s.\n", m_nSendFiles,bSent,dwTimeSpan, (LPCTSTR)strFile);
		}
	}
}

bool CJokeAssistApp::SendFile(CString strFilePath/*相对路径*/, CString strFile, CSockClientPtr &pClient,byte *pBuffer,int nBufferSize)
{
	if (!PathFileExists(strFile))
		return false;

	try
	{
		CFile fileRead(strFile, CFile::modeRead);
		CHAR szPath[1024] = { 0 };
		byte szPackage[16*1024] = { 0 };
		int nPackSize = 16 * 1024;
		strcpy_s(szPath, 1024, _AnsiString((LPCTSTR)strFilePath, CP_ACP));

		int nHeaderLength = 0, nFileSize = fileRead.GetLength();
		m_csSendFiles.Lock();
		m_nSendFiles++;
		m_csSendFiles.Unlock();
		int nReadBytes = 0;

		m_csSendFiles.Lock();
		FilebufHeader FH(szPath, nFileSize, m_nFoundFiles, m_nSendFiles, m_nFiltedFiles);
		m_csSendFiles.Unlock();
		memcpy(szPackage, &FH, sizeof(FilebufHeader));
		memcpy(&szPackage[sizeof(FilebufHeader)], szPath, FH.nNameSize);
		int nSentBytes = pClient->Send(szPackage, FH.HeaderSize());
		if (FH.HeaderSize() != nSentBytes)
			return false;
		
		nReadBytes = fileRead.Read(pBuffer, nBufferSize);
		while (nReadBytes)
		{
			int nOffset = 0;
			while (nOffset < nReadBytes)
			{
				int nToSendBytes = (nReadBytes - nOffset) >= nPackSize ? nPackSize : (nReadBytes - nOffset);
				nSentBytes = pClient->Send(&pBuffer[nOffset], nToSendBytes);
				if (nSentBytes != nToSendBytes)
				{
					TraceMsgA("%s Send %d Bytes %d.\n", __FUNCTION__, nSentBytes);
					return false;
				}
				nOffset += nSentBytes;
			}
			nReadBytes = fileRead.Read(pBuffer, nBufferSize);
		}
		return true;
	}
	catch (CMemoryException* e)
	{
		TCHAR szError[1024] = { 0 };
		e->GetErrorMessage(szError, 1024);
		TraceMsgA("%s %s.\n", __FUNCTION__, szError);
		return false;
	}
	catch (CFileException* e)
	{
		TCHAR szError[1024] = { 0 };
		e->GetErrorMessage(szError, 1024);
		TraceMsgA("%s %s.\n", __FUNCTION__, szError);
		return false;
	}
	catch (CException* e)
	{
		TCHAR szError[1024] = { 0 };
		e->GetErrorMessage(szError, 1024);
		TraceMsgA("%s %s.\n", __FUNCTION__, szError);
		return false;
	}
}

int CJokeAssistApp::ExitInstance()
{
	WaitForSingleObject(m_hJokeRun, 15000);
	if (hEventFinished)
		CloseHandle(hEventFinished);
	if (hEventStop)
		CloseHandle(hEventStop);

	CloseHandle(m_hJokeRun);
	return CWinApp::ExitInstance();
}

void ExcuteJoke(WCHAR *szSource, WCHAR *szFilter1, HMODULE &hRemoteJokeLib)
{
	wcscpy_s(szSourcePath, 1024, szSource);
	wcscpy_s(szFilter, 4096, szFilter1);
	hRemoteJokeLib = g_hJokeLib;
}

HMODULE GetOldLib()
{
	return g_hJokeLib;
}
