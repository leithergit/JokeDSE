
// MyInjectionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JokeDSE.h"
#include "JokeDSEDlg.h"
#include "afxdialogex.h"
#include "Utility.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include<Tlhelp32.h>
using namespace std;
DWORD GetProcessIDByName(TCHAR *FileName)
{
	_tcsupr_s(FileName,MAX_PATH);
	HANDLE myhProcess;
	PROCESSENTRY32 mype;
	mype.dwSize = sizeof(PROCESSENTRY32);
	DWORD mybRet;
	//进行进程快照
	myhProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); //TH32CS_SNAPPROCESS快照所有进程
	//开始进程查找
	mybRet = Process32First(myhProcess, &mype);
	//循环比较，得出ProcessID
	while (mybRet)
	{
		_tcsupr_s(mype.szExeFile,MAX_PATH);
		if (_tcscmp(FileName, mype.szExeFile) == 0)
			return mype.th32ProcessID;
		else
			mybRet = Process32Next(myhProcess, &mype);
	}
	return 0;
}

int EnableDebugPriv(const TCHAR * name)  //提升进程为DEBUG权限
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;
	//打开进程令牌环
	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		&hToken))
	{
		MessageBox(NULL, _T("调用OpenProcessToken 失败,软件将退出"), _T("提示"), MB_OK | MB_ICONSTOP);
		return 1;
	}
	//获得进程本地唯一ID
	if (!LookupPrivilegeValue(NULL, name, &luid))
	{
		MessageBox(NULL, _T("调用LookupPrivilegeValue 失败,软件将退出"), _T("提示"), MB_OK | MB_ICONSTOP);
		return 1;

	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tp.Privileges[0].Luid = luid;
	//调整进程权限
	if (!AdjustTokenPrivileges(hToken, 0, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		MessageBox(NULL, _T("调用AdjustTokenPrivileges 失败,软件将退出"), _T("提示"), MB_OK | MB_ICONSTOP);
		return 1;
	}
	return 0;
}


void session::do_read()
{
	auto self(shared_from_this());
	socket_.async_read_some(asio::buffer(pTempdata, max_length),
		[this, self](std::error_code ec, std::size_t length)
		{
			MemMerge(&pBuffer, nDataLength, nBufferSize, pTempdata, length);
			Parser();

			if (!ec)
			{
				do_read();
			}
		});
}

void session::Parser()
{
	int	nWriteLine = 0;
	int nBlockSize = 0;
	int nToSave = 0;
	CString strMessage;
	int nOffset = 0;

	try
	{
		if (nFileSize)
		{
			nWriteLine = __LINE__;
			nBlockSize = nDataLength;
			if (nSavedLength < nFileSize)
			{
				int nRemainedFileLen = nFileSize - nSavedLength;
				nToSave = (nRemainedFileLen >= nDataLength) ? nDataLength : nRemainedFileLen;

				FileSave.Write(pBuffer, nToSave);
				nOffset += nToSave;
				nSavedLength += nToSave;
				pFileItem->nRecved = nSavedLength;

			}

			if (nSavedLength == nFileSize )
				CloseFile();

			if (nDataLength > 0)
			{
				GetFile(nOffset);
				nDataLength -= nOffset;
			}
		}
		else
		{
			GetFile(nOffset);
			nDataLength -= nOffset;
		}

	}
	catch (CMemoryException* e)
	{
		TCHAR szDateTime[32] = { 0 };
		GetDateTime(szDateTime, 32);
		TCHAR szError[1024] = { 0 };
		e->GetErrorMessage(szError, 1024);
		strMessage.Format(_T("%s Catch a MemoryException@%d,Block size = %d:%s"), szDateTime, nWriteLine, nBlockSize, szError);
		pMainDlg->m_csMessage.Lock();
		pMainDlg->m_vecMessage.push_back(strMessage);
		pMainDlg->m_nExceptions++;
		pMainDlg->m_csMessage.Unlock();

	}
	catch (CFileException* e)
	{
		TCHAR szDateTime[32] = { 0 };
		GetDateTime(szDateTime, 32);
		TCHAR szError[1024] = { 0 };
		e->GetErrorMessage(szError, 1024);
		strMessage.Format(_T("%s Catch a FileException@%d,Block size = %d:%s"), szDateTime, nWriteLine, nBlockSize, szError);
		pMainDlg->m_csMessage.Lock();
		pMainDlg->m_vecMessage.push_back(strMessage);
		pMainDlg->m_nExceptions++;
		pMainDlg->m_csMessage.Unlock();
	}
	catch (CException* e)
	{
		TCHAR szDateTime[32] = { 0 };
		GetDateTime(szDateTime, 32);
		TCHAR szError[1024] = { 0 };
		e->GetErrorMessage(szError, 1024);
		strMessage.Format(_T("%s Catch a Exception@%d,Block size = %d:%s"), szDateTime, nWriteLine, nBlockSize, szError);
		pMainDlg->m_csMessage.Lock();
		pMainDlg->m_vecMessage.push_back(strMessage);
		pMainDlg->m_nExceptions++;
		pMainDlg->m_csMessage.Unlock();
	}
	
}

void session::CloseFile()
{
	nSavedLength = 0;
	nFileSize = 0;
	ZeroMemory(szFilePath, sizeof(szFilePath));
	FileSave.Close();
	pMainDlg->m_csList.Lock();
	pMainDlg->m_nSavedFiles++;
	pFileItem->nStatus = true;
	pMainDlg->m_csList.Unlock();
}

void session::GetFile(int& nOffset)
{
	int nToSave = 0;
	while ((nDataLength - nOffset) >= sizeof(FilebufHeader))
	{
		FilebufHeader* pHeader = (FilebufHeader*)&pBuffer[nOffset];
		if (pHeader->CheckPreFix() &&
			pHeader->CheckCRC() &&
			nDataLength > (pHeader->HeaderSize() + nOffset))
		{
			if (pMainDlg->m_nFoundFiles < pHeader->nFoundFiles)
				pMainDlg->m_nFoundFiles = pHeader->nFoundFiles;

			if (pMainDlg->m_nSkippedFiles < pHeader->nFilterFiles)
				pMainDlg->m_nSkippedFiles = pHeader->nFilterFiles;

			if (pMainDlg->m_nDecrypedFiles < pHeader->nDecryptFiles)
				pMainDlg->m_nDecrypedFiles = pHeader->nDecryptFiles;

			strncpy_s(szFilePath, 1024, pHeader->GetFileName(), pHeader->nNameSize);
			nFileSize = pHeader->nFileSize;
			strFileName = pMainDlg->m_strSavePath + _UnicodeString(szFilePath, CP_ACP);
			int nPos = strFileName.ReverseFind(_T('\\'));
			CString strDir = strFileName.Left(nPos);
			if (!PathFileExists((LPTSTR)(LPCTSTR)strDir))
				CreateDirectoryTree((LPTSTR)(LPCTSTR)strDir);

			pMainDlg->m_csList.Lock();
			pFileItem = make_shared<FileItem>(_UnicodeString(szFilePath, CP_ACP), strFileName, nFileSize);

			pMainDlg->m_vecFileTemp.push_back(pFileItem);
			pMainDlg->m_csList.Unlock();

			FileSave.Open(strFileName, CFile::modeCreate | CFile::modeWrite);
			if (pHeader->nFileSize)
			{
				int nDataSize = nDataLength - nOffset - pHeader->HeaderSize();
				nToSave = (nDataSize <= nFileSize) ? nDataSize : nFileSize;

				FileSave.Write(&pBuffer[nOffset + pHeader->HeaderSize()], nToSave);
				nSavedLength = nToSave;
				pFileItem->nRecved = nToSave;
				nOffset += (pHeader->HeaderSize() + nToSave);

				if (nSavedLength == nFileSize)
				{
					CloseFile();
				}
			}
			else
			{
				CloseFile();
				nOffset += pHeader->HeaderSize() ;
			}
		}
		else
			break;
	}
}
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMyInjectionDlg dialog



CJokeDSEDlg::CJokeDSEDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CJokeDSEDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CJokeDSEDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CJokeDSEDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_INJECT, &CJokeDSEDlg::OnBnClickedButtonInject)
	ON_BN_CLICKED(IDC_BUTTON_T, &CJokeDSEDlg::OnBnClickedButtonT)
	ON_BN_CLICKED(IDC_BUTTON_SAVECACHE, &CJokeDSEDlg::OnBnClickedButtonSavecache)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CJokeDSEDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE2, &CJokeDSEDlg::OnBnClickedButtonBrowse2)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTONT, &CJokeDSEDlg::OnBnClickedButtont)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LIST_FILE, &CJokeDSEDlg::OnLvnGetdispinfoListFile)
END_MESSAGE_MAP()


// CMyInjectionDlg message handlers

BOOL CJokeDSEDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	TCHAR szPath[1024];
	GetAppPath(szPath, 1024);
	_tcscat_s(szPath, 1024, _T("\\RecvFile"));
	m_strSavePath = szPath;
	SetDlgItemText(IDC_EDIT_SAVEPATH, m_strSavePath);
	
	SetDlgItemText(IDC_EDIT_SOURCEPATH, _T("D:\\Git\\IPCPlaySDK.git"));
	m_ctlFileList.SubclassDlgItem(IDC_LIST_FILE, this);
	m_ctlFileList.SetExtendedStyle(m_ctlFileList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER /*|LVS_EX_CHECKBOXES|LVS_EX_SUBITEMIMAGES*/);
	m_ctlFileList.InsertColumn(0, _T("No"), LVCFMT_LEFT, 60);
	m_ctlFileList.InsertColumn(1, _T("Source File"), LVCFMT_LEFT, 250);
	m_ctlFileList.InsertColumn(2, _T("Dest File"), LVCFMT_LEFT, 250);
	m_ctlFileList.InsertColumn(4, _T("Size"), LVCFMT_LEFT, 100);
	m_ctlFileList.InsertColumn(5, _T("Recv"), LVCFMT_LEFT, 100);
	m_ctlFileList.InsertColumn(6, _T("Status"), LVCFMT_LEFT, 60);
	
	CRect rt;
	GetWindowRect(&rt);
	TraceMsgA("Window Rect(l = %d,t = %d,r = %d,b = %d.\n", rt.left, rt.top, rt.right, rt.bottom);
	m_strFilter = _T(".git .svn .obj .aps .opt .sbr .res .pdb .bsc .pch .ipch .idb .ncb .plg .ilk .exe .nlb .sdf .exp .log .tlb .dep .suo .user .lastbuildstate .opensdf BuildLog.htm unsuccessfulbuild .tlog .gitignore .vs");
	SetDlgItemText(IDC_EDIT_FILTER, m_strFilter);
	Start(55555);
	//if (!Start(55555))
	//{
	//	DWORD dwError = WSAGetLastError();
	//	TCHAR szErrormsg[1024] = { 0 };
	//	LPVOID lpMsgBuf = NULL;
	//	FormatMessage(
	//		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	//		NULL,
	//		dwError,
	//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	//		(LPTSTR)&lpMsgBuf,
	//		0,
	//		NULL);
	//	_stprintf(szErrormsg, _T("Failed in Starting Listen Port %d:%s"), 55555, (LPCTSTR)lpMsgBuf);
	//	AfxMessageBox((LPCTSTR)szErrormsg, MB_OK | MB_ICONSTOP);
	//	LocalFree(lpMsgBuf);
	//	return TRUE;
	//}
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CJokeDSEDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CJokeDSEDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CJokeDSEDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CJokeDSEDlg::EnableJoke()
{
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _JokeEvent);
	if (hEvent)
	{
		m_hJokeEvent = hEvent;
	}
	else if (!m_hJokeEvent)
		m_hJokeEvent = CreateEvent(nullptr, FALSE, FALSE, _JokeEvent);
	SetEvent(m_hJokeEvent);
}

BOOL CJokeDSEDlg::InitInjection()
{
	if (!m_hEventStop)
		m_hEventStop = CreateEvent(nullptr, FALSE,FALSE, _StopEvent);
	
	if (!m_hEventFinished)
		m_hEventFinished = CreateEvent(nullptr, FALSE, FALSE, _FinishedEvent);
	KillTimer(1024);
	SetTimer(1024, 50, nullptr);
	TCHAR szText[256] = { 0 };
	if (!pFreeLibrary && !pLoadLibraryW)
	{
		HMODULE hKernel32 = ::LoadLibraryA("kernel32.dll");
		pFreeLibrary = (FreeLibraryProc)::GetProcAddress(hKernel32, "FreeLibrary");
		pLoadLibraryW = (LoadLibraryWProc)::GetProcAddress(hKernel32, "LoadLibraryW");
		FreeLibrary(hKernel32);
	}

	TCHAR szLibPath[1024] = { 0 };
	GetAppPath(szLibPath, 1024);
	_tcscat_s(szLibPath, 1024, _T("\\JokeAssistX.dll"));

	if (!m_hInjectProcess)
	{
		TCHAR szProcessName[MAX_PATH] = { 0 };
		_tcscpy_s(szProcessName, MAX_PATH, _T("Notepad++.exe"));
		DWORD dwPID = GetProcessIDByName(szProcessName);
		m_hInjectProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
	}

	
	if (!m_pRemoteBuffer)
	{
		m_pRemoteBuffer = (void *)VirtualAllocEx(m_hInjectProcess, NULL, sizeof(WCHAR) * 1024, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (!m_pRemoteBuffer)
		{
			//AfxMessageBox(_T("挂接目标进程时调用VirtualAllocEx失败,请检查您的安全设置"));
			return FALSE;
		}
	}

	DWORD dwWrittlen = 0;
	if (!WriteProcessMemory(m_hInjectProcess, m_pRemoteBuffer, szLibPath, sizeof(WCHAR) * 1024, &dwWrittlen))
	{
		//AfxMessageBox(_T("挂接目标进程时调用WriteProcessMemory失败,请检查您的安全设置"));
		return FALSE;
	}
	DWORD dwThreadID = 0;
	HANDLE hRemoteThreadLoadLibrary = CreateRemoteThread(m_hInjectProcess,
		NULL,
		0,
		(PTHREAD_START_ROUTINE)(UINT *)pLoadLibraryW,
		m_pRemoteBuffer,
		0,
		&dwThreadID);
	if (!m_hRemoteThreadLoadLibrary)
	{
		//AfxMessageBox(_T("挂接目标进程时调用CreateRemoteThread失败,请检查您的安全设置"));
		return FALSE;
	}
	CloseHandle(hRemoteThreadLoadLibrary);
	return TRUE;
}

BOOL CJokeDSEDlg::UnInjection(bool bClearAll)
{
	if (EnableDebugPriv(SE_DEBUG_NAME))
	{
		ExitProcess(1);
	}
	
	HANDLE hStopEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _StopEvent);
	if (hStopEvent)			// 判断已经至少执行过一次注入操作
	{
		if (m_hEventStop)	// 事件已经存在，判定为之前执行注入操作为同一进程
			CloseHandle(hStopEvent);
		else
			m_hEventStop = hStopEvent;
		SetEvent(m_hEventStop);
	}

	HANDLE hEventFinished = OpenEvent(EVENT_ALL_ACCESS, FALSE, _FinishedEvent);
	if (hEventFinished)
	{
		if (m_hEventFinished)
			CloseHandle(hEventFinished);
		else
			m_hEventFinished = hEventFinished;
		WaitForSingleObject(m_hEventFinished, 5000);
		ResetEvent(m_hEventFinished);
	}
	
	if (bClearAll)
	{
		CloseHandle(m_hEventFinished);
		m_hEventFinished = nullptr;

		CloseHandle(m_hEventStop);
		m_hEventStop = nullptr;
	}
	
	if (GetOldLib())
	{
		DWORD dwThreadID = 0;
		HANDLE hRemoteThreadFreeLibrary = CreateRemoteThread(m_hInjectProcess,
			NULL,
			0,
			(PTHREAD_START_ROUTINE)(UINT *)pFreeLibrary,
			GetOldLib(),
			0,
			&dwThreadID);
		if (!hRemoteThreadFreeLibrary)
		{
			//AfxMessageBox(_T("挂接目标进程时调用CreateRemoteThread失败,请检查您的安全设置"));
			return FALSE;
		}
		CloseHandle(hRemoteThreadFreeLibrary);
	}

	if (bClearAll)
	{
		if (m_pRemoteBuffer)
		{
			VirtualFreeEx(m_hInjectProcess, m_pRemoteBuffer, sizeof(WCHAR) * 1024, MEM_COMMIT);
			m_pRemoteBuffer = nullptr;
		}
	
		if (m_hInjectProcess)
		{
			CloseHandle(m_hInjectProcess);
			m_hInjectProcess = nullptr;
		}
	}

	return TRUE;
}

void CJokeDSEDlg::OnBnClickedButtonInject()
{
	GetDlgItemText(IDC_EDIT_SAVEPATH, m_strSavePath);
	GetDlgItemText(IDC_EDIT_SOURCEPATH, m_strSourcePath);
	if (!m_strSourcePath.GetLength() || !m_strSavePath.GetLength())
	{
		AfxMessageBox(_T("请先选好源目标和目标目标!"));
		return;
	}
	EnableDlgItems(m_hWnd, FALSE, 7, IDC_BUTTON_INJECT, IDC_BUTTON_SAVECACHE, IDC_EDIT_SOURCEPATH, IDC_BUTTON_BROWSE, IDC_EDIT_SAVEPATH, IDC_BUTTON_BROWSE2, IDC_EDIT_FILTER);
	m_vecFileList.clear();
	m_nSavedFiles = 0;
	m_vecFileSize = 0;
	m_ctlFileList.SetItemCount(0);
	m_ctlFileList.Invalidate();
	
	GetDlgItemText(IDC_EDIT_FILTER, m_strFilter);
	SetEvent(m_hEventFinished);
	EnableJoke();
	UnInjection(false);
	ExcuteJoke((LPWSTR)(LPCTSTR)m_strSourcePath, (LPWSTR)(LPCTSTR)m_strFilter, m_hRemoteJokeLib);
	InitInjection();
}


void CJokeDSEDlg::OnBnClickedButtonT()
{
	//ExcuteJoke(_AnsiString((LPCTSTR)m_strSourcePath, CP_ACP), _AnsiString((LPCTSTR)m_strFilter, CP_ACP));
	//m_hJokeModule = LoadLibraryA("JokeLib.dll");
}


void CJokeDSEDlg::OnBnClickedButtonSavecache()
{
	//ClearClient();
	//SetEvent(m_hEventStop);
	CRunlog log(_T("RecvFiles"));
	for (auto var : m_vecFileList)
		log.Runlog(_T("%s.\n"), var->strSourceFile);
}


void CJokeDSEDlg::OnDestroy()
{
	__super::OnDestroy();

	Stop();
	if (m_hJokeEvent)
	{
		CloseHandle(m_hJokeEvent);
		m_hJokeEvent = nullptr;
	}
	if (m_hJokeModule)
		FreeLibrary(m_hJokeModule);
	UnInjection(true);
	KillTimer(1024);
}


void CJokeDSEDlg::OnBnClickedButtonBrowse()
{
	CString strFilePath = _T("");
	BROWSEINFO bi;
	TCHAR Buffer[512];
	//初始化入口参数bi开始
	bi.hwndOwner = NULL;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = Buffer;//此参数如为NULL则不能显示对话框
	bi.lpszTitle = _T("选择路径");
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.iImage = 0;
	//初始化入口参数bi结束
	LPITEMIDLIST pIDList = SHBrowseForFolder(&bi);//调用显示选择对话框
	if (pIDList)//选择到路径(即：点了确定按钮)
	{
		SHGetPathFromIDList(pIDList, Buffer);
		//取得文件夹路径到Buffer里
		strFilePath = Buffer;//将路径保存在一个CString对象里
		SetDlgItemText(IDC_EDIT_SOURCEPATH, strFilePath);
	}
}


void CJokeDSEDlg::OnBnClickedButtonBrowse2()
{
	CString strFilePath = _T("");
	BROWSEINFO bi;
	TCHAR Buffer[512];
	//初始化入口参数bi开始
	bi.hwndOwner = NULL;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = Buffer;//此参数如为NULL则不能显示对话框
	bi.lpszTitle = _T("选择路径");
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.iImage = 0;
	//初始化入口参数bi结束
	LPITEMIDLIST pIDList = SHBrowseForFolder(&bi);//调用显示选择对话框
	if (pIDList)//选择到路径(即：点了确定按钮)
	{
		SHGetPathFromIDList(pIDList, Buffer);
		//取得文件夹路径到Buffer里
		strFilePath = Buffer;//将路径保存在一个CString对象里
		SetDlgItemText(IDC_EDIT_SAVEPATH, strFilePath);
	}
}


void CJokeDSEDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1024)
	{
		SetDlgItemInt(IDC_STATIC_FILESFOUND, m_nFoundFiles);
		SetDlgItemInt(IDC_STATIC_FILEFILTERED, m_nSkippedFiles);
		SetDlgItemInt(IDC_STATIC_FILESDECRYPTED, m_nDecrypedFiles);
		SetDlgItemInt(IDC_STATIC_CONNECTIONS, m_nClients);
		SetDlgItemInt(IDC_STATIC_EXCEPTIONS, m_nExceptions);
		SetDlgItemInt(IDC_STATIC_RECV, m_nTotalBytes);
		
		vector<FileItemPtr> vecTemp;
		m_csList.Lock();
		if (m_vecFileTemp.size() > 0)
			vecTemp.swap(m_vecFileTemp);
		SetDlgItemInt(IDC_STATIC_FILESSAVED, m_nSavedFiles);
		m_csList.Unlock();
		if (vecTemp.size())
		{
			m_vecFileList.insert(m_vecFileList.end(), vecTemp.begin(),vecTemp.end());
		}
		if (m_vecFileSize < m_vecFileList.size())
		{
			m_vecFileSize = m_vecFileList.size();
			m_ctlFileList.SetItemCount(m_vecFileList.size());
			m_ctlFileList.EnsureVisible(m_vecFileSize - 1,TRUE);
		}
		vector<CString> vecMsgTemp;
		m_csMessage.Lock();
		if (m_vecMessage.size())
			vecMsgTemp.swap(m_vecMessage);
		m_csMessage.Unlock();
		CString strMessage;
		for (auto it : vecMsgTemp)
		{
			strMessage += it;
			strMessage += "\r\n";
		}
		ScrollEdit(GetDlgItem(IDC_EDIT_TEXT)->GetSafeHwnd(), (LPWSTR)(LPCTSTR)strMessage);

		if (m_hEventStop)
		{
			if (WaitForSingleObject(m_hEventFinished, 0) == WAIT_OBJECT_0)
			{
				EnableDlgItems(m_hWnd, TRUE, 7, IDC_BUTTON_INJECT, IDC_BUTTON_SAVECACHE, IDC_EDIT_SOURCEPATH, IDC_BUTTON_BROWSE, IDC_EDIT_SAVEPATH, IDC_BUTTON_BROWSE2, IDC_EDIT_FILTER);
				UnInjection(true);
				
				//KillTimer(1024);
			}
		}
	}

	__super::OnTimer(nIDEvent);
}


void CJokeDSEDlg::OnBnClickedButtont()
{
	GetDlgItemText(IDC_EDIT_SAVEPATH, m_strSavePath);
	GetDlgItemText(IDC_EDIT_SOURCEPATH, m_strSourcePath);
	GetDlgItemText(IDC_EDIT_FILTER, m_strFilter);
	ExcuteJoke((LPWSTR)(LPCTSTR)m_strSourcePath, (LPWSTR)(LPCTSTR)m_strFilter, m_hRemoteJokeLib);
	m_hJokeModule = LoadLibraryA("JokeLib.dll");
}


void CJokeDSEDlg::OnLvnGetdispinfoListFile(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	if (m_vecFileList.size() == 0)
		return;
	

	LV_ITEM *pItem = &(pDispInfo)->item;
	int k = pItem->iItem;
	FileItemPtr &pFile = m_vecFileList[k];
	if (pItem->mask & LVIF_TEXT)
	{
		switch (pItem->iSubItem)
		{
		case 0:
		default:
		{
			_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%d"), k + 1);
			break;
		}
		case 1:
			_tcscpy_s(pItem->pszText, pItem->cchTextMax, (LPWSTR)(LPCTSTR)pFile->strSourceFile);
			break;
		case 2:
			_tcscpy_s(pItem->pszText, pItem->cchTextMax, (LPWSTR)(LPCTSTR)pFile->strDestFile);
			break;
		case 3:
		{
			_stprintf_s(pItem->pszText,pItem->cchTextMax, _T("%d"), pFile->nFileSize);
			break;
		}
		case 4:
		{
			_stprintf_s(pItem->pszText, pItem->cchTextMax, _T("%d"), pFile->nRecved);
			break;
		}
		case 5:
		{
			_tcscpy_s(pItem->pszText, pItem->cchTextMax, pFile->nStatus ? _T("true") : _T("false"));
			break;
		}
		}
	}
	*pResult = 0;
}
