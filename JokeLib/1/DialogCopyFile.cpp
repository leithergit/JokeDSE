// CopyFile.cpp : implementation file
//

#include "stdafx.h"
#include "AssistLib.h"
#include "DialogCopyFile.h"
#include "afxdialogex.h"
//#include "MyInjection.h"


// CCopyFile dialog

IMPLEMENT_DYNAMIC(CDialogCopyFile, CDialogEx)

CDialogCopyFile::CDialogCopyFile(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDialogCopyFile::IDD, pParent)
{

}

CDialogCopyFile::~CDialogCopyFile()
{
}

void CDialogCopyFile::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDialogCopyFile, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CDialogCopyFile::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CDialogCopyFile::OnBnClickedButtonSend)
	ON_WM_DESTROY()
END_MESSAGE_MAP()



// CCopyFile message handlers

void CDialogCopyFile::OnBnClickedButtonBrowse()
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
		SetDlgItemText(IDC_EDIT_PATH, strFilePath);
	}
}


void AccessFile(CString strFile,void *pParam)
{
	CDialogCopyFile *pThis = (CDialogCopyFile *)pParam;
	CString strFileupper = strFile;
	strFileupper.MakeUpper();
	CString strExt = PathFindExtension(strFileupper);
	int nPos = pThis->m_strFilter.Find(strExt);
	bool bSend = false;
	if (nPos >= 0)
	{
		nPos += strExt.GetLength();
		if (pThis->m_strFilter[nPos] != ' ' && pThis->m_strFilter[nPos] != ';')
			bSend = true;
	}
	else
		bSend = true;
	if (bSend)
	{
 //		CString strReletivePath = strFile.Right(strFile.GetLength() - pThis->m_nDirLength);
// 		pThis->m_csList.Lock();
// 		pThis->m_FileList.push_back(make_shared<FileInfo>(strReletivePath, strFile));
// 		pThis->m_csList.Unlock();
// 		
		ScrollEdit(pThis->m_hEditText, _T("%s.\r\n"), (LPCTSTR)strReletivePath);
	}	
}

void CDialogCopyFile::AccessDirectory(CString strDirectory, AccessCallBack pACB,void *p)
{
	CFileFind finder;
 
	if (strDirectory.GetAt(m_nDirLength - 1) == _T('\\'))
		strDirectory.SetAt(m_nDirLength - 1, _T('\\'));
	CString strFilePath = strDirectory + _T("\\*.*");
	BOOL bWorking = finder.FindFile(strFilePath);
	m_strFilter.MakeUpper();
	CString strSVN = _T(".svn");
	CString strGit = _T(".git");

	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		CString strFile = finder.GetFilePath();
		if (finder.IsDirectory())
		{
			if (strFile.Find(strGit) >=0
				||strFile.Find(strSVN) >=0)
				continue;
			AccessDirectory(finder.GetFilePath(),pACB,p);
		}
		else
		{
			//TraceMsgA("%s File:%s\n", __FUNCTION__, finder.GetFilePath());
			pACB(finder.GetFilePath(), p);
		}
	}
}
void CDialogCopyFile::OnBnClickedButtonSend()
{
	GetDlgItemText(IDC_EDIT_FILTER, m_strFilter);
	GetDlgItemText(IDC_EDIT_PATH, m_strDirectory);
// 	m_nDirLength = m_strDirectory.GetLength();
// 	if (m_strDirectory != _T("") && m_strDirectory.GetLength() >= 3)
// 	{
// 		m_bSendThreadRun = true;
// 		for (int i = 0; i < sizeof(m_hThreadArray) / sizeof(HANDLE); i++)
// 		{
// 			m_hThreadArray[i] = (HANDLE)_beginthreadex(0, 0, ThreadSendFile, this, 0, 0);
// 		}
// 		AccessDirectory(m_strDirectory, AccessFile,this);
// 	}
}


BOOL CDialogCopyFile::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SendDlgItemMessage(IDC_EDIT_PATH, EM_SETLIMITTEXT, 0, 0x7FFFFFFF);
	m_hEditText = GetDlgItem(IDC_EDIT_TEXT)->GetSafeHwnd();
	m_strFilter = _T(".obj .aps .opt .sbr .res .pdb .bsc .pch .idb .ncb .plg .ilk .exe .nlb .sdf .exp .tlog .log .tlb .dep BuildLog.htm .suo .user .lastbuildstate");
	SetDlgItemText(IDC_EDIT_FILTER, m_strFilter);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CDialogCopyFile::OnDestroy()
{
	CDialogEx::OnDestroy();

// 	m_bSendThreadRun = false;
// 	WaitForMultipleObjects(sizeof(m_hThreadArray) / sizeof(HANDLE), m_hThreadArray,TRUE, INFINITE);
}
