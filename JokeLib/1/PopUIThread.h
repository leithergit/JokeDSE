#pragma once

#include <list>
#include <afxmt.h>
#include <process.h>
#include "SocketServer.h"



// CPopUIThread

class CPopUIThread : public CWinApp
{
	DECLARE_DYNCREATE(CPopUIThread)

protected:
	CPopUIThread();           // protected constructor used by dynamic creation
	virtual ~CPopUIThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	void StartThread();
	
	void StopThread();
	
	
	HANDLE	m_hThreadArray[4];
	bool	m_bSendThreadRun = false;
	static UINT __stdcall ThreadSendFile(void *p)
	{
		CPopUIThread *pThis = (CPopUIThread *)p;
		return pThis->SendFileRun();
	}
	UINT SendFileRun();
	

	bool SendFile(CString strFilePath, CString strFile, WORD nPort = 55555);
	
protected:
	DECLARE_MESSAGE_MAP()
};


