#pragma once

#include "resource.h"
#include <list>
#include <afxmt.h>
#include <process.h>
#include "SocketServer.h"
#include <memory>
using namespace std;
using namespace std::tr1;

// CCopyFile dialog
typedef void(*AccessCallBack)(CString strFile, void *pParam);
// struct FileInfo
// {
// 	CString strDir;
// 	CString strFile;
// 	
// 	FileInfo(CString strDir1, CString strFile1)
// 		:strDir(strDir1),
// 		strFile(strFile1)
// 	{
// 	}
// };
// typedef shared_ptr<FileInfo> FileInfoPtr;

class CDialogCopyFile : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogCopyFile)

public:
	CDialogCopyFile(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogCopyFile();
// Dialog Data
	enum { IDD = IDD_DIALOG_COPYFILE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedButtonSend();
	HWND	m_hEditText = nullptr;
	CString m_strFilter;
	CString m_strDirectory;


	void AccessDirectory(CString szDirectory, AccessCallBack pACB,void *p);
	bool SendFile(CString strFilePath, CString strFile ,WORD nPort = 55555)
	{
		if (PathFileExists(strFile))
		{
			shared_ptr<CSockClient> pClient = make_shared<CSockClient>();
			if (pClient->ConnectServer(_T("192.168.1.7"), nPort,1500) != INVALID_SOCKET)
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
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
};
