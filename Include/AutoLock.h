#pragma once

#include <assert.h>
#include <Windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include "CriticalSectionAgent.h"
#ifdef Release_D
#undef assert
#define assert	((void)0)
#endif
/// @brief 驴脡脪脭脳脭露炉录脫脣酶潞脥陆芒脣酶脕脵陆莽脟酶碌脛脌脿
///
/// 脢脢脫脙脫脷陆芒脣酶脤玫录镁赂麓脭脫,虏禄路陆卤茫脢脰露炉陆芒脣酶脕脵陆莽脟酶麓煤脗毛碌脛鲁隆潞脧
///
/// @code
///	CRITICAL_SECTION cs;
/// CAutoLock  lock(&cs,true);
/// @endcode 
///
#define _OuputLockTime
#define _LockOverTime	100


#define defA(B,C) B##C
#define defLineVar(B,C) defA(B,C)

#define LineLock(cs)			CAutoLock defLineVar(lock,__LINE__)(cs,false,__FILE__,__FUNCTION__,__LINE__);

#define AutoLock(cs)			CAutoLock Lock(cs,false,__FILE__,__FUNCTION__,__LINE__);

#define AutoLockAgent(CSAgent)			CAutoLock LockAgent(CSAgent.Get(),false,__FILE__,__FUNCTION__,__LINE__);
#define UnlockAgent()					LockAgent.Unlock();

// 脭脷虏禄脨猫脪陋脢脰露炉脤谩脟掳陆芒脣酶碌脛碌脴路陆拢卢陆篓脪茅脠芦虏驴脢鹿脫脙LineLockAgent
#define LineLockAgent(CSAgent)  CAutoLock  defLineVar(Lock,__LINE__)(CSAgent.Get(),false,__FILE__,__FUNCTION__,__LINE__);

class CAutoLock
{
public:
	CRITICAL_SECTION *m_pCS;
	bool	m_bAutoDelete;
	bool	m_bLocked;
#ifdef _LockOverTime
	DWORD	m_dwLockTime;
	CHAR   *m_pszFile;
	char   *m_pszFunction;
	int		m_nLockLine;
#endif
	explicit CAutoLock():m_pCS(NULL),m_bAutoDelete(false)
	{
	}
public:
	CAutoLock(CRITICAL_SECTION *pCS, bool bAutoDelete = false, const CHAR *szFile = nullptr, char *szFunction = nullptr, int nLine = 0)
	{
		ZeroMemory(this, sizeof(CAutoLock));
		assert(pCS != NULL);
		if (pCS)
		{
#ifdef _LockOverTime
			m_dwLockTime = timeGetTime();
			if (szFile)
			{
				m_pszFile = new CHAR[strlen(szFile) + 1];
				strcpy(m_pszFile, szFile);
				m_pszFunction = new char[strlen(szFunction) + 1];
				strcpy(m_pszFunction, szFunction);
				m_nLockLine = nLine;
			}
#endif
			m_pCS = pCS;
			::EnterCriticalSection(m_pCS);
			m_bLocked = true;
			if (timeGetTime() - m_dwLockTime >= _LockOverTime)
			{
				CHAR szOuput[1024] = { 0 };
				if (szFile)
				{
					sprintf(szOuput, "Wait Lock @File:%s:%d(%s),Waittime = %d.\n", m_pszFile, m_nLockLine, m_pszFunction , timeGetTime() - m_dwLockTime);
				}
				else
					sprintf(szOuput, "Wait Lock Waittime = %d.\n", timeGetTime() - m_dwLockTime);
				OutputDebugStringA(szOuput);
			}
		}
	}

	CAutoLock(CCriticalSectionAgent *pCS, bool bAutoDelete = false, const CHAR *szFile = nullptr, char *szFunction = nullptr, int nLine = 0)
	{
		ZeroMemory(this, sizeof(CAutoLock));
		assert(pCS != NULL);
		if (pCS)
		{
#ifdef _LockOverTime
			m_dwLockTime = timeGetTime();
			if (szFile)
			{
				m_pszFile = new CHAR[strlen(szFile) + 1];
				strcpy(m_pszFile, szFile);
				m_pszFunction = new char[strlen(szFunction) + 1];
				strcpy(m_pszFunction, szFunction);
				m_nLockLine = nLine;
			}
#endif
			m_pCS = pCS->Get();
			::EnterCriticalSection(m_pCS);
			m_bLocked = true;
			if (timeGetTime() - m_dwLockTime >= _LockOverTime)
			{
				CHAR szOuput[1024] = { 0 };
				if (szFile)
				{
					sprintf(szOuput, "Wait Lock @File:%s:%d(%s),Waittime = %d.\n", m_pszFile, m_nLockLine, m_pszFunction, timeGetTime() - m_dwLockTime);
				}
				else
					sprintf(szOuput, "Wait Lock Waittime = %d.\n", timeGetTime() - m_dwLockTime);
				OutputDebugStringA(szOuput);
			}
		}
	}
	void Lock()
	{
		if (m_bLocked)
			return;
#if _LockOverTime
		m_dwLockTime = timeGetTime();
#endif
		::EnterCriticalSection(m_pCS);
		m_bLocked = true;
		if (timeGetTime() - m_dwLockTime >= _LockOverTime)
		{
			CHAR szOuput[1024] = { 0 };
			if (m_pszFile)
			{
				sprintf(szOuput, "Wait Lock @File:%s:%d(%s),Waittime = %d.\n", m_pszFile, m_nLockLine, m_pszFunction, timeGetTime() - m_dwLockTime);
			}
			else
				sprintf(szOuput, "Wait Lock Waittime = %d.\n", timeGetTime() - m_dwLockTime);
			OutputDebugStringA(szOuput);
		}
		
		
	}
	void Unlock()
	{
		if (!m_bLocked)
			return;
		if (m_pCS)
		{
			::LeaveCriticalSection(m_pCS);
			m_bLocked = false;
#ifdef _OuputLockTime
			CHAR szOuput[1024] = { 0 };
			if ((timeGetTime() - m_dwLockTime) > _LockOverTime)
			{
				if (m_pszFile)
				{
					sprintf(szOuput, "Lock @File:%s:%d(%s),locktime = %d.\n", m_pszFile, m_nLockLine, m_pszFunction, timeGetTime() - m_dwLockTime);
				}
				else
					sprintf(szOuput, "Lock locktime = %d.\n", timeGetTime() - m_dwLockTime);
				OutputDebugStringA(szOuput);
			}
		
#endif
			
		}
	}
	~CAutoLock()
	{
		if (m_bLocked)
			Unlock();
		
		if (m_pszFile)
		{
			delete[]m_pszFile;
			m_pszFile = nullptr;
		}
		if (m_pszFunction)
		{
			delete[]m_pszFunction;
			m_pszFunction = nullptr;
		}
		
		if (m_bAutoDelete)
			::DeleteCriticalSection((CRITICAL_SECTION *)m_pCS);
	}
};


class CTryLock:public CAutoLock
{
public:
	explicit CTryLock()
	{
	}

	CTryLock(CRITICAL_SECTION *pCS, bool bAutoDelete = false, const CHAR *szFile = nullptr, char *szFunction = nullptr, int nLine = 0) 
	{
		assert(pCS != NULL);
		if (pCS)
		{
#ifdef _LockOverTime
			m_dwLockTime = timeGetTime();
			if (szFile)
			{
				m_pszFile = new CHAR[strlen(szFile) + 1];
				strcpy(m_pszFile, szFile);
				m_pszFunction = new char[strlen(szFunction) + 1];
				strcpy(m_pszFunction, szFunction);
				m_nLockLine = nLine;
			}
#endif
			m_pCS = pCS;
			m_bLocked = ::TryEnterCriticalSection(m_pCS);
			if (!m_bLocked)
				throw std::exception("Failed in trying lock!");
			if (timeGetTime() - m_dwLockTime >= _LockOverTime)
			{
				CHAR szOuput[1024] = { 0 };
				if (szFile)
				{
					sprintf(szOuput, "Wait Lock @File:%s:%d(%s),Waittime = %d.\n", m_pszFile, m_nLockLine, m_pszFunction, timeGetTime() - m_dwLockTime);
				}
				else
					sprintf(szOuput, "Wait Lock Waittime = %d.\n", timeGetTime() - m_dwLockTime);
				OutputDebugStringA(szOuput);
			}
		}
		else
			throw std::exception("Invalid parameters!");
	}

	CTryLock(CCriticalSectionAgent *pCS, bool bAutoDelete = false, const CHAR *szFile = nullptr, char *szFunction = nullptr, int nLine = 0)
	{
		ZeroMemory(this, sizeof(CAutoLock));
		assert(pCS != NULL);
		if (pCS)
		{
#ifdef _LockOverTime
			m_dwLockTime = timeGetTime();
			if (szFile)
			{
				m_pszFile = new CHAR[strlen(szFile) + 1];
				strcpy(m_pszFile, szFile);
				m_pszFunction = new char[strlen(szFunction) + 1];
				strcpy(m_pszFunction, szFunction);
				m_nLockLine = nLine;
			}
#endif
			m_pCS = pCS->Get();
			m_bLocked = ::TryEnterCriticalSection(m_pCS);
			if (!m_bLocked)
				throw std::exception("Failed in trying lock!");
			if (timeGetTime() - m_dwLockTime >= _LockOverTime)
			{
				CHAR szOuput[1024] = { 0 };
				if (szFile)
				{
					sprintf(szOuput, "Wait Lock @File:%s:%d(%s),Waittime = %d.\n", m_pszFile, m_nLockLine, m_pszFunction, timeGetTime() - m_dwLockTime);
				}
				else
					sprintf(szOuput, "Wait Lock Waittime = %d.\n", timeGetTime() - m_dwLockTime);
				OutputDebugStringA(szOuput);
			}
		}
		else
			throw std::exception("Invalid parameters!");
	}

};

#define LineTryLock(cs)				CTryLock	defLineVar(Trylock,__LINE__)(cs,false,__FILE__,__FUNCTION__,__LINE__);
#define AutoTrylock(cs)				CTryLock	Trylock(cs,false,__FILE__,__FUNCTION__,__LINE__);
#define AutoTrylockAgent(CSAgent)	CTryLock	LockAgent(CSAgent.Get(),false,__FILE__,__FUNCTION__,__LINE__);
#define LineTrylockAgent(CSAgent)	CTryLock	defLineVar(LockAgent,__LINE__)(CSAgent.Get(),false,__FILE__,__FUNCTION__,__LINE__);