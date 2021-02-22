
// MyInjectionDlg.h : header file
//

#pragma once
#include "resource.h"
#include "SocketUtils.h"
#include "Utility.h"
#include "../JokeAssist/FileHeader.h"
#include <Shlwapi.h>
#include <afxmt.h>
#include <vector>
#include "CriticalSectionAgent.h"
#include "AutoLock.h"
#include "asio.hpp"
#include <thread>
using namespace std;
using namespace asio;
using asio::ip::tcp;

#pragma comment(lib,"shlwapi.lib")
#include "../JokeAssist/JokeLib.h"
#pragma comment(lib,"../debug/JokeAssistX.lib")

typedef BOOL (WINAPI *FreeLibraryProc)(_In_ HMODULE hLibModule);
typedef HMODULE (WINAPI *LoadLibraryWProc)(_In_ LPCWSTR lpLibFileName);
typedef WINBASEAPI FARPROC (WINAPI *GetProcAddressProc)(_In_ HMODULE hModule,_In_ LPCSTR lpProcName);


struct FileItem
{
	CString strSourceFile;
	CString strDestFile;
	UINT	nFileSize;
	UINT	nRecved = 0;
	bool	nStatus;
	FileItem(CString str1,CString str2,int nSize)
	{
		strSourceFile = str1;
		strDestFile = str2;
		nFileSize = nSize;
		nStatus = false;
	}
};
typedef shared_ptr<FileItem> FileItemPtr;
class CFileClient :public CSockClient
{
public:
	int nHeaderLength = 0;
	CHAR szFilePath[1024];
	CString strFileName;
	int nFileSize = 0;
	int nSavedLength = 0;
	int nFoundFiles = 0;
	int nDecryptedFiles = 0;
	int nFiltedFiles = 0;
	int nSkipped = 0;
	CFile FileSave;
	FileItemPtr pFileItem = nullptr;
	int		nRecvCount = 0;
	byte	*pBuffer = nullptr;
	CFileClient(SOCKET s, sockaddr_in AddrIn)
		:CSockClient(s, AddrIn)
	{
		//pBuffer = new byte[1024 * 1024];
	}

	~CFileClient()
	{
		if (FileSave.m_hFile != INVALID_HANDLE_VALUE)
			FileSave.Close();
// 		if (pBuffer)
// 		{
// 			delete[]pBuffer;
// 			pBuffer = nullptr;
// 		}
	}
};
class CJokeDSEDlg;
class session
	: public std::enable_shared_from_this<session>
{
public:
	session(tcp::socket socket, CJokeDSEDlg *pDlg)
		: socket_(std::move(socket))
		, pMainDlg(pDlg)
	{
		pTempdata = new char[max_length];
		ZeroMemory(pTempdata, max_length);
		pBuffer = new char[nBufferSize];
		ZeroMemory(pBuffer, nBufferSize);
	}
	~session()
	{
		if (pTempdata)
			delete[]pTempdata;
		if (pBuffer)
			delete[]pBuffer;
	}

	void start()
	{
		do_read();
	}
	
	int nHeaderLength = 0;
	CHAR szFilePath[1024];
	CString strFileName;
	int nFileSize = 0;
	int nSavedLength = 0;
	int nFoundFiles = 0;
	int nDecryptedFiles = 0;
	int nFiltedFiles = 0;
	int nSkipped = 0;
	CFile FileSave;
	FileItemPtr pFileItem = nullptr;
	int		nRecvCount = 0;
	void Parser();
private:
	void do_read();
	
	void GetFile(int& nOffset);
	void CloseFile();

	tcp::socket socket_;
	enum { max_length = 8192 };
	char* pTempdata = nullptr;
	CCriticalSectionAgent csBuffer;
	char* pBuffer = nullptr;
	int nReadCount = 0;
	int nDataLength = 0;
	int nBufferSize = 128 * 1024;
	CJokeDSEDlg* pMainDlg = nullptr;
};
typedef shared_ptr<session> sessionPtr;
// CMyInjectionDlg dialog
class CJokeDSEDlg : public CDialogEx,CSocketServer
{
// Construction
public:
	
	CJokeDSEDlg(CWnd* pParent = NULL);	// standard constructor
	shared_ptr<tcp::acceptor> m_pAcceptor = nullptr;
	shared_ptr<io_context> m_pIO_context = nullptr;
	shared_ptr<std::thread> m_pThread = nullptr;
	list <sessionPtr> m_listSesstion;
	void do_accept()
	{
		m_pAcceptor->async_accept(
			[this](std::error_code ec, tcp::socket socket)
			{
				m_csClients.Lock();
				m_nClients++;
				m_csClients.Unlock();
				if (!ec)
				{
					
					std::string strClientIP = socket.remote_endpoint().address().to_string();
					unsigned short nClientPort = socket.remote_endpoint().port();
					TraceMsgA("%s Accept Client %s:%d.\n", __FUNCTION__, strClientIP.c_str(), nClientPort);

					sessionPtr pSession = std::make_shared<session>(std::move(socket),this);
					//m_listSesstion.push_back(pSession);
					pSession->start();
				}

				do_accept();
			});
	}
	void Start(int nPort)
	{
		if (!m_pThread)
		{
			m_pIO_context = make_shared<io_context>();
			m_pAcceptor = make_shared<tcp::acceptor>(*m_pIO_context, tcp::endpoint(tcp::v4(), nPort));
			do_accept();
			m_pThread = make_shared<std::thread>([&]() 
				{
					m_pIO_context->run();
				});
		}
		
	}
	void Stop()
	{
		if (m_pIO_context)
			m_pIO_context->stop();
		
		if (m_pThread)
			m_pThread->join();
	}

// Dialog Data
	enum { IDD = IDD_JOKEDSE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	virtual int OnAccept(CSockClient *pClientBase)
	{
		m_csClients.Lock();
		m_nClients++;
		m_csClients.Unlock();
		CFileClient *pClient = (CFileClient *)pClientBase;
		sockaddr_in &ClientAddr = pClient->ClientAddr;
		TraceMsgA("%s Client %d.%d.%d.%d:%d Connected.\n", __FUNCTION__,
			ClientAddr.sin_addr.S_un.S_un_b.s_b1,
			ClientAddr.sin_addr.S_un.S_un_b.s_b2,
			ClientAddr.sin_addr.S_un.S_un_b.s_b3,
			ClientAddr.sin_addr.S_un.S_un_b.s_b4,
			ClientAddr.sin_port);
// 		int nRecv = pClient->Recv();
// 		if (pClient->nBufferLength > 0)
// 		{
// 			ParserData(pClient);
// 		}
		return 0;
	}
	
	virtual CSockClient *CreateClient(SOCKET s, sockaddr_in AddrIn)
	{
		return (CSockClient *)new CFileClient(s, AddrIn);
	}

	virtual void DeleteClient(CSockClient *pClient)
	{
		if (pClient)
			delete (CFileClient *)pClient;
	}

	void GetFile(CSockClient *pClientBase, int &nOffset)
	{
		CFileClient *pClient = (CFileClient *)pClientBase;
		int nToSave = 0;
		while ((pClient->nDataLength - nOffset) >= sizeof(FilebufHeader))
		{
			FilebufHeader *pHeader = (FilebufHeader *)&pClient->pRecvBuffer[nOffset];
			if (pHeader->CheckPreFix() &&
				pHeader->CheckCRC() && 
				pClient->nDataLength > (pHeader->HeaderSize() + nOffset))
			{
				if (m_nFoundFiles < pHeader->nFoundFiles)
					m_nFoundFiles = pHeader->nFoundFiles;

				if (m_nSkippedFiles < pHeader->nFilterFiles)
					m_nSkippedFiles = pHeader->nFilterFiles;

				if (m_nDecrypedFiles < pHeader->nDecryptFiles)
					m_nDecrypedFiles = pHeader->nDecryptFiles;

				strncpy_s(pClient->szFilePath, 1024, pHeader->GetFileName(), pHeader->nNameSize);
				pClient->nFileSize = pHeader->nFileSize;
				pClient->strFileName = m_strSavePath + _UnicodeString(pClient->szFilePath, CP_ACP);
				int nPos = pClient->strFileName.ReverseFind(_T('\\'));
				CString strDir = pClient->strFileName.Left(nPos);
				if (!PathFileExists((LPTSTR)(LPCTSTR)strDir))
					CreateDirectoryTree((LPTSTR)(LPCTSTR)strDir);

				m_csList.Lock();
				pClient->pFileItem = make_shared<FileItem>(_UnicodeString(pClient->szFilePath, CP_ACP), pClient->strFileName, pClient->nFileSize);
				
				m_vecFileTemp.push_back(pClient->pFileItem);
				m_csList.Unlock();

				pClient->FileSave.Open(pClient->strFileName, CFile::modeCreate | CFile::modeWrite);

				int nDataSize = pClient->nDataLength - nOffset - pHeader->HeaderSize();
				nToSave = (nDataSize <= pClient->nFileSize) ? nDataSize : pClient->nFileSize;
				
				pClient->FileSave.Write(&pClient->pRecvBuffer[nOffset + pHeader->HeaderSize()], nToSave);
				pClient->nSavedLength = nToSave;
				pClient->pFileItem->nRecved = nToSave;
				nOffset += ( pHeader->HeaderSize()+ nToSave);
					
				if (pClient->nSavedLength == pClient->nFileSize)
				{
					pClient->nSavedLength = 0;
					pClient->nFileSize = 0;
					ZeroMemory(pClient->szFilePath, sizeof(pClient->szFilePath));
					pClient->FileSave.Close();
					CString strItem;
					m_csList.Lock();
					m_nSavedFiles++;
					pClient->pFileItem->nStatus = true;
					m_csList.Unlock();
				}
			}
			else
				break;
		}
	}
	void ParserData(CSockClient *pClientBase)
	{
		CFileClient *pClient = (CFileClient *)pClientBase;
		char *pBuffer = pClient->pRecvBuffer;
		int	nWriteLine = 0;
		int nBlockSize = 0;
		int nToSave = 0;
		CString strMessage;
		int nOffset = 0;

		try
		{
			if (pClient->nFileSize )
			{
				nWriteLine = __LINE__;
				nBlockSize = pClient->nDataLength;
				if (pClient->nSavedLength < pClient->nFileSize)
				{
					int nRemainedFileLen = pClient->nFileSize - pClient->nSavedLength;
					nToSave = (nRemainedFileLen >= pClient->nDataLength) ? pClient->nDataLength : nRemainedFileLen;

					pClient->FileSave.Write(pBuffer, nToSave);
					nOffset += nToSave;
					pClient->nSavedLength += nToSave;
					pClient->pFileItem->nRecved = pClient->nSavedLength;
					
				}

				if (pClient->nSavedLength == pClient->nFileSize)
				{
					pClient->pFileItem->nRecved = pClient->nFileSize;
					pClient->nFileSize = 0;
					pClient->nSavedLength = 0;
					ZeroMemory(pClient->szFilePath, sizeof(pClient->szFilePath));
					pClient->FileSave.Close();
					CString strItem;
					m_csList.Lock();
					m_nSavedFiles++;
					pClient->pFileItem->nStatus = true;
					m_csList.Unlock();
				}
				
				if (pClient->nDataLength > 0)
				{
					GetFile(pClientBase,nOffset);
					pClient->nDataLength -= nOffset;
				}
			}
			else
			{
				GetFile(pClientBase,nOffset);
				pClient->nDataLength -= nOffset;
			}
			if (pClient->nDataLength > nOffset)
			{
				memmove(pBuffer, &pBuffer[nOffset], pClient->nDataLength - nOffset);
				pClient->nDataLength -= nOffset;
			}
			else
				pClient->nDataLength = 0;
		}
		catch (CMemoryException* e)
		{
			TCHAR szDateTime[32] = { 0 };
			GetDateTime(szDateTime, 32);
			TCHAR szError[1024] = { 0 };
			e->GetErrorMessage(szError, 1024);
			strMessage.Format(_T("%s Catch a MemoryException@%d,Block size = %d:%s"), szDateTime, nWriteLine,nBlockSize, szError);
			m_csMessage.Lock();
			m_vecMessage.push_back(strMessage);
			m_nExceptions++;
			m_csMessage.Unlock();
			
		}
		catch (CFileException* e)
		{
			TCHAR szDateTime[32] = { 0 };
			GetDateTime(szDateTime, 32);
			TCHAR szError[1024] = { 0 };
			e->GetErrorMessage(szError, 1024);
			strMessage.Format(_T("%s Catch a FileException@%d,Block size = %d:%s"), szDateTime, nWriteLine, nBlockSize, szError);
			m_csMessage.Lock();
			m_vecMessage.push_back(strMessage);
			m_nExceptions++;
			m_csMessage.Unlock();
		}
		catch (CException* e)
		{
			TCHAR szDateTime[32] = { 0 };
			GetDateTime(szDateTime, 32);
			TCHAR szError[1024] = { 0 };
			e->GetErrorMessage(szError, 1024);
			strMessage.Format(_T("%s Catch a Exception@%d,Block size = %d:%s"), szDateTime, nWriteLine, nBlockSize, szError);
			m_csMessage.Lock();
			m_vecMessage.push_back(strMessage);
			m_nExceptions++;
			m_csMessage.Unlock();
		}
	}

	virtual int OnConnect(CSockClient *pClient)
	{
		return 0;
	}

	virtual int OnRecv(CSockClient *pClientBase)
	{
		CFileClient *pClient = (CFileClient *)pClientBase;
		pClient->nRecvCount++;
		int nRecv = pClient->Recv();
		m_nTotalBytes = pClient->m_nTotalRecv;
		if (pClient->nDataLength > 0)
		{
			 ParserData(pClient);
			//TraceMsgA("%s Total Recv Bytes:%I64u.\n", __FUNCTION__, pClient->m_nTotalRecv);
			// pClient->nDataLength = 0;
		}
		return nRecv;
	}

	virtual int OnWrite(CSockClient *pClientBase)
	{
		CFileClient *pClient = (CFileClient *)pClientBase;
		sockaddr_in &ClientAddr = pClient->ClientAddr;
		return 0;
	}

	virtual int OnDisConnect(CSockClient *pClientBase)
	{
		CFileClient *pClient = (CFileClient *)pClientBase;
		sockaddr_in &ClientAddr = pClient->ClientAddr;
		//TraceMsgA("%s Client %d.%d.%d.%d:%d Disconnected,RecvCount = %d.\n", __FUNCTION__,
		//	ClientAddr.sin_addr.S_un.S_un_b.s_b1,
		//	ClientAddr.sin_addr.S_un.S_un_b.s_b2,
		//	ClientAddr.sin_addr.S_un.S_un_b.s_b3,
		//	ClientAddr.sin_addr.S_un.S_un_b.s_b4,
		//	ClientAddr.sin_port,
		//	pClient->nRecvCount);
		if (pClient->nDataLength > 0)
		{
			ParserData(pClient);
			//TraceMsgA("%s Recv Bytes:%d.\n", __FUNCTION__, pClient->nDataLength);
			pClient->nDataLength = 0;
		}
		m_csClients.Lock();
		m_nClients--;
		m_csClients.Unlock();
		return 0;
	}
	//LONGLONG nRecvBytes = 0;
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonInject();
	CCriticalSectionAgent m_csListSession;
	list<session *> m_listSession;
	volatile bool m_bThreadSalveFile = false;
	shared_ptr<std::thread> m_pThreadSaveFile = nullptr;
	void ThreadSaveFile()
	{
		session* pSession = nullptr;
		while (m_bThreadSalveFile)
		{
			while (!m_csListSession.TryLock())
			{
				Sleep(20);
			}

			if (m_listSession.size())
			{
				pSession = m_listSession.front();
				m_listSession.pop_front();
			}
			m_csListSession.Unlock();
			if (pSession)
			{
				pSession->Parser();
				pSession = nullptr;
			}
				
			Sleep(20);
		}
	}
	void EnableJoke();
	BOOL InitInjection();
	BOOL UnInjection(bool bClearAll);
	CCriticalSection m_csList;
	CListCtrl m_ctlFileList;
	HANDLE m_hJokeEvent = nullptr;
	HANDLE m_hEventFinished = nullptr;
	HANDLE m_hEventStop = nullptr;
	HANDLE m_hRemoteThreadLoadLibrary = nullptr;
	HANDLE	m_hInjectProcess;
	HMODULE m_hRemoteJokeLib = nullptr;
	CString m_strSavePath;
	CString m_strSourcePath;
	CString m_strFilter;
	CCriticalSection m_csClients;
	UINT	m_nClients = 0;
	CCriticalSectionAgent m_csSaveFiles;
	int m_nSavedFiles = 0;
	int m_nTotalBytes = 0;

	int m_nFoundFiles = 0;
	int m_nDecrypedFiles = 0;
	int m_nSkippedFiles = 0;
	int m_vecFileSize = 0;
	int m_nConnections = 0;
	CCriticalSectionAgent m_csConnections;
	int	m_nExceptions = 0;
	CCriticalSectionAgent m_csExceptions;
	vector<FileItemPtr> m_vecFileTemp;
	vector<FileItemPtr> m_vecFileList;
	CCriticalSectionAgent m_csMessage;
	vector<CString>m_vecMessage;
	afx_msg void OnBnClickedButtonT();
	void			*m_pRemoteBuffer;
	LoadLibraryWProc pLoadLibraryW = nullptr;
	FreeLibraryProc  pFreeLibrary = nullptr;
	HMODULE m_hJokeModule = nullptr;
	afx_msg void OnBnClickedButtonSavecache();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedButtonBrowse2();

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtont();
	afx_msg void OnLvnGetdispinfoListFile(NMHDR *pNMHDR, LRESULT *pResult);
};
