// AssistLib.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "AssistLib.h"
#include "SocketServer.h"
#include "Runlog.h"

#pragma comment(lib,"winmm.lib")

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

#define  _StartEvent	_T("Global\\{97D34EFC-A044-493C-9156-420849F34179}")
#define  _StopEvent		_T("Global\\{EBDEB499-B8F2-4DE3-9C01-95A5D3A11778}")
#define	 _NamedMutex	_T("Global\\{B3435272-9755-4D1A-A0D1-2341C5A1B2B0}")

#pragma data_seg("MySection")
// HANDLE hEventExcute = nullptr;
// HANDLE hEventJokeRun = nullptr;
bool bJokeMain = false;
HMODULE hJokeLib = nullptr;
char szSharedSourcePath[1024] = { 0 };
char szSharedFilter[4096] = { 0 };
#pragma data_seg()
#pragma comment(linker, "/SECTION:MySection,RWS")

// CAssistLibApp

BEGIN_MESSAGE_MAP(CAssistLibApp, CWinApp)
END_MESSAGE_MAP()


CAssistLibApp::CAssistLibApp()
{
	m_pRunlog = make_shared<CRunlog>("Joke");
}


CAssistLibApp theApp;

// CAssistLibApp initialization
void CALLBACK TimerCallBack(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	CAssistLibApp *pThis = (CAssistLibApp*)dwUser;
	timeKillEvent(pThis->m_hTimer);
	/*pThis->m_hJokeRun = (HANDLE)*/_beginthreadex(nullptr, 0, pThis->ThreadJoke, pThis, 0, 0);
}

AFX_MODULE_STATE *g_pModule_State = nullptr;
void WaitforDebug(bool bWait = false)
{
	do 
	{
		Sleep(100);
	} while (bWait);
}
BOOL CAssistLibApp::InitInstance()
{
	CWinApp::InitInstance();
	m_hModule = AfxGetInstanceHandle();

	BOOL bWait = FALSE;
	WaitforDebug(bWait);
	hJokeLib = m_hModule;
	CSocketServer::InitializeWinsock();
	
	hEventStop = OpenEvent(EVENT_ALL_ACCESS, FALSE, _StopEvent);
	hEventStart = OpenEvent(EVENT_ALL_ACCESS, FALSE, _StartEvent);
	if (hEventStop || hEventStart)
	{
		timeSetEvent(100, 1, (LPTIMECALLBACK)TimerCallBack, (DWORD_PTR)this, TIME_ONESHOT | TIME_CALLBACK_FUNCTION);
	}
		
	return TRUE;
}

bool CAssistLibApp::CheckFilter(CString strFile, bool bExt)
{
	CString strFileupper = strFile;
	strFileupper.MakeUpper();
	CString strCompare = "";
	bool bMatched = false;
	if (!bExt)
	{
		int nPos = strFileupper.ReverseFind('\\');
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
			if ((m_strFilter[nPos] == ' ' || m_strFilter[nPos] == ';')
				|| m_strFilter.GetLength() == nPos)
				bMatched = true;
		}
	}
	return bMatched;
}

void CAssistLibApp::AccessFile(CString strFile, void *pParam)
{
	CAssistLibApp *pThis = (CAssistLibApp *)pParam;
	CString strReletivePath = strFile.Right(strFile.GetLength() - pThis->m_nDirLength);
	if (!pThis->CheckFilter(strFile) &&
		!pThis->CheckFilter(strFile, false))
	{
		if (SendFile(strReletivePath, strFile,pParam,55555,false))
		{
			pThis->m_nSendFiles++;
		}
	}
	else
	{
		pThis->m_pRunlog->Runlog("%s %s is filted.\r\n", __FUNCTION__, (LPCTSTR)strFile);
		SendFile(strReletivePath, strFile,pParam, 55555, true);	
		pThis->m_nFiltedFiles++;
	}
}

UINT CAssistLibApp::JokeRun()
{
	m_strDirectory = szSharedSourcePath;
	m_strFilter = szSharedFilter;
	m_nDirLength = m_strDirectory.GetLength();
	if (m_nDirLength > 0)
		AccessDirectory(m_strDirectory, AccessFile, this);

	m_pRunlog->Runlog("%s FoundFiles = %d\tFiltered Files = %d\tSent Files = %d.\r\n", __FUNCTION__, m_nFoundFiles, m_nFiltedFiles, m_nSendFiles);
	//WaitForSingleObject(m_hJokeRun, INFINITE);
	SetEvent(hEventStop);
	
	Sleep(1000);
	CloseHandle(hEventStart);
	CloseHandle(hEventStop);
	//::FreeLibrary(m_hModule);
	
	return 0;
}

void CAssistLibApp::AccessDirectory(CString strDirectory, AccessCallBack pACB, void *p)
{
	CFileFind finder;

	if (strDirectory.GetAt(m_nDirLength - 1) == _T('\\'))
		strDirectory.SetAt(m_nDirLength - 1, _T('\\'));
	CString strFilePath = strDirectory + _T("\\*.*");
	BOOL bWorking = finder.FindFile(strFilePath);
	m_strFilter.MakeUpper();

	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		CString strFile = finder.GetFilePath();
		if (finder.IsDirectory())
		{
			strFile.MakeUpper();
			if (CheckFilter(strFile) || 
				CheckFilter(strFile,false))
				continue;

			AccessDirectory(finder.GetFilePath(), pACB, p);
		}
		else
		{
			m_nFoundFiles++;
			m_pRunlog->Runlog(_T("%s Found files %d.\n"), __FUNCTION__, m_nFoundFiles);
			pACB(finder.GetFilePath(), p);
		}
	}
}

bool CAssistLibApp::SendFile(CString strFilePath/*相对路径*/, CString strFile,void *p, WORD nPort,bool bSkipped)
{
	CAssistLibApp *pThis = (CAssistLibApp *)p;
	if (PathFileExists(strFile))
	{
		shared_ptr<CSockClient> pClient = make_shared<CSockClient>();
		do 
		{
			if (pClient->ConnectServer(_T("127.0.0.1"), nPort, 500) != INVALID_SOCKET)
			{
				try
				{
					CFile fileRead(strFile, CFile::modeRead);
					CHAR  szHeader[2048] = { 0 };
					CHAR szPath[1024] = { 0 };
					strcpy_s(szPath, 1024, (LPCTSTR)strFilePath);
					int nHeaderLen = 0;
					// 写入固定的长度值
					sprintf_s(szHeader, 2048, "FoundFiles:%08x;SendFiles:%08xFiltedFiles:%08x;HeaderLength:%08x;Skipped:%d;FileLength:%08x;FileDSE:%s;####\n", 
							pThis->m_nFoundFiles,pThis->m_nSendFiles + 1,pThis->m_nFiltedFiles,nHeaderLen, bSkipped, (UINT)fileRead.GetLength(), szPath);
					nHeaderLen = strlen(szHeader);
					// 写入真实的长度值
					sprintf_s(szHeader, 2048, "FoundFiles:%08x;SendFiles:%08xFiltedFiles:%08x;HeaderLength:%08x;Skipped:%d;FileLength:%08x;FileDSE:%s;####\n",
						pThis->m_nFoundFiles, pThis->m_nSendFiles + 1, pThis->m_nFiltedFiles, nHeaderLen, bSkipped, (UINT)fileRead.GetLength(), szPath);
					nHeaderLen = strlen(szHeader);
					int nHeaderLength = 0, nFileLength = 0;
					if (bSkipped)
					{
						return  (pClient->Send((byte *)szHeader, nHeaderLen) == nHeaderLen);
					}
					else
					{
						int nHeaderLength = 0, nFileLength = 0;
						CHAR  *szBuffer = new CHAR[(int)fileRead.GetLength() + nHeaderLen + 1];
						shared_ptr<char> pBuffPtr(szBuffer);
						memcpy(szBuffer, szHeader, nHeaderLen);
						int nSize = fileRead.Read(&szBuffer[nHeaderLen], fileRead.GetLength());
						if (nSize != fileRead.GetLength())
						{
							pThis->m_pRunlog->Runlog("Read Files faied:%s.\n", (LPCTSTR)strFile);
						}
						return  (pClient->Send((byte *)szBuffer, fileRead.GetLength() + nHeaderLen) == (fileRead.GetLength() + nHeaderLen));
					}
					
				}
				catch (CMemoryException* e)
				{
					TCHAR szError[1024] = { 0 };
					e->GetErrorMessage(szError, 1024);
					pThis->m_pRunlog->Runlog("%s %s.\n", __FUNCTION__, szError);
					return false;
				}
				catch (CFileException* e)
				{
					TCHAR szError[1024] = { 0 };
					e->GetErrorMessage(szError, 1024);
					pThis->m_pRunlog->Runlog("%s %s.\n", __FUNCTION__, szError);
					return false;
				}
				catch (CException* e)
				{
					TCHAR szError[1024] = { 0 };
					e->GetErrorMessage(szError, 1024);
					pThis->m_pRunlog->Runlog("%s %s.\n", __FUNCTION__, szError);
					return false;
				}
			}
			else
			{
				//TraceMsgA("%s Send File %s Failed.\n", __FUNCTION__, (LPCTSTR)strFile);
				continue;
			}
		} while (true);
	}
	else
		return false;
}

void EnableJoke()
{
	bJokeMain = true;
}
void ExcuteJoke(char *szSource,char *szFilter1,HMODULE &hJokeLibRemote)
{
	strcpy_s(szSharedSourcePath, 1024, szSource);
	strcpy_s(szSharedFilter, 4096, szFilter1);
	hJokeLibRemote = hJokeLib;
//	SetEvent(hEventExcute);
}

// void StopJoke()
// {
// 	SetEvent(hEventJokeRun);
// }

int CAssistLibApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}
