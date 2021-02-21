#pragma once
#include <TCHAR.H>
#include <windows.h>
#include <time.h>
#include <assert.h>
#include <MMSystem.h>
#include <map>
using namespace std;
//#include <thread>
//#include <chrono>
#ifdef Release_D
#undef assert
#define assert	((void)0)
#endif

// using namespace std::this_thread;
// using namespace std::chrono;
#ifdef _UNICODE
#define GetDateTime			GetDateTimeW
#define	_DateTime			DateTimeW
#define UTC2DateTimeString	UTC2DateTimeStringW
#else
#define GetDateTime			GetDateTimeA
#define	_DateTime			DateTimeA
#define UTC2DateTimeString	UTC2DateTimeStringA
#endif


int		GetDateTimeA(CHAR *szDateTime,int nSize);
int		GetDateTimeW(WCHAR *szDateTime,int nSize);
LPCSTR	DateTimeA();
LPCWSTR	DateTimeW();
bool	IsLeapYear(UINT nYear);
//UINT64	DateTimeString2UTC(TCHAR *szTime,UINT64 &nTime);
void	UTC2DateTimeStringA(UINT64 nTime,CHAR *szTime,int nSize);
void	UTC2DateTimeStringW(UINT64 nTime,WCHAR *szTime,int nSize);
BOOL	SystemTime2UTC(SYSTEMTIME *pSystemTime,UINT64 *pTime);
BOOL	UTC2SystemTime(UINT64 *pTime,SYSTEMTIME *pSystemTime);
void	StringFromSystemTimeA(CHAR* szTimeString,int nSize,bool bMillionSecond = true);
void	StringFromSystemTimeW(WCHAR* szTimeString,int nSize,bool bMillionSecond = true);

// NTPУʱ��
struct   NTP_Packet
{
	int			Control_Word;   
	int			root_delay;   
	int			root_dispersion;   
	int			reference_identifier;   
	__int64		reference_timestamp;   
	__int64		originate_timestamp;   
	__int64		receive_timestamp;   
	int			transmit_timestamp_seconds;   
	int			transmit_timestamp_fractions;   
};

/************************************************************************/
/* ����˵��:�Զ���ʱ�������ͬ������
/* ����˵��:��
/* �� �� ֵ:�ɹ�����TRUE��ʧ�ܷ���FALSE
/************************************************************************/
BOOL NTPTiming(const char* szTimeServer);


#define TimeSpan(t)		(time(NULL) - (time_t)t)
#define TimeSpanEx(t)	(GetExactTime() - t)
#define PerfTimeSpan(t)	(GetPerfTime() - t)
#define MMTimeSpan(t)	(long)(timeGetTime() - t)
typedef struct __ExactTimeBase
{
	LONGLONG	dfFreq;
	LONGLONG	dfCounter;
	time_t		nBaseClock;
	double		dfMilliseconds;
	__ExactTimeBase()
	{
		ZeroMemory(this,sizeof(ETB));
		SYSTEMTIME systime1;
		SYSTEMTIME systime2;
		ZeroMemory(&systime1, sizeof(SYSTEMTIME));
		ZeroMemory(&systime2, sizeof(SYSTEMTIME));

		HANDLE hProcess			= GetCurrentProcess();
		HANDLE hThread			= GetCurrentThread();

		DWORD dwPriorityClass	= GetPriorityClass(hProcess);		
		DWORD dwThreadPriority	= GetThreadPriority(hThread);

		DWORD dwError			= 0;
		// �ѽ������ȼ�������ʵʱ��
		if (!SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS))
		{
			dwError = GetLastError();
			if (!SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
			{
				dwError = GetLastError();
				if (!SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS))
				{
					dwError = GetLastError();
				}
			}
		}
		// ���߳����ȼ�������ʵʱ��
		if (!SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL))
		{
			dwError = GetLastError();
			if (!SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST))
			{
				dwError = GetLastError();
				if (!SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL))
				{
					dwError = GetLastError();
				}
			}
		}
		GetSystemTime(&systime1);
		// У׼��׼ʱ��
		while (true)
		{
			GetSystemTime(&systime2);
			if (memcmp(&systime1, &systime2, sizeof(SYSTEMTIME)) != 0)
				break;
		}
		// �ָ��̺߳ͽ��̵����ȼ�
		SetThreadPriority(hThread, dwThreadPriority);
		SetPriorityClass(hProcess, dwPriorityClass);

		SystemTime2UTC(&systime2,(UINT64 *)&nBaseClock);
		dfMilliseconds = (double)(systime2.wMilliseconds /1000);

#ifdef _DEBUG
		CHAR szText[64] = {0};
		sprintf_s(szText,"BaseClock of ETB = %I64d.\n",nBaseClock);
		OutputDebugStringA(szText);
#endif
		LARGE_INTEGER LarInt;		
		QueryPerformanceFrequency(&LarInt);	
		dfFreq = LarInt.QuadPart;
		QueryPerformanceCounter(&LarInt);
		dfCounter = LarInt.QuadPart;
	}

	~__ExactTimeBase()
	{
	}

}ETB;
extern ETB g_etb;
#define	InitPerformanceClock	

/// @brief ȡϵͳ��ȷʱ��,��λ��,����Ϊ25΢������
double  GetExactTime();
double  GetPerfTime();

/// @brief �߳����߷�ʽ
// struct CThreadSleep
// {
// 	enum SleepWay
// 	{
// 		Sys_Sleep = 0,		///< ֱ�ӵ���ϵͳ����Sleep
// 		Wmm_Sleep = 1,		///< ʹ�ö�ý��ʱ����߾���
// 		Std_Sleep = 2		///< ʹC++11�ṩ���߳����ߺ���
// 	};
// 	CThreadSleep()
// 	{
// 		double dfSumSpan1 = 0.0f, dfSumSpan2 = 0.0f, dfSumSpan3 = 0.0f;
// 		double dfT1 = 0.0f;
// 		for (int i = 0; i < 32; i++)
// 		{
// 			dfT1 = GetExactTime();
// 			Sleep(1);
// 			dfSumSpan1 += TimeSpanEx(dfT1);
// 
// 			dfT1 = GetExactTime();
// 			timeBeginPeriod(1); //���þ���Ϊ1����
// 			::Sleep(1); //��ǰ�̹߳���һ����
// 			timeEndPeriod(1); //������������
// 			dfSumSpan2 += TimeSpanEx(dfT1);
// 
// 			dfT1 = GetExactTime();
// 			//std::this_thread::sleep_for(std::chrono::nanoseconds(1));
// 			Sleep(1);
// 			dfSumSpan3 += TimeSpanEx(dfT1);
// 		}
// 		double dfSpanSum = dfSumSpan1;
// 		
// 		if (dfSumSpan1 <= dfSumSpan2)
// 		{
// 			nSleepWay = Sys_Sleep;
// 		}
// 		else
// 		{
// 			dfSpanSum = dfSumSpan2;
// 			nSleepWay = Wmm_Sleep;
// 		}
// 		if (dfSumSpan3 < dfSpanSum)
// 		{
// 			nSleepWay = Std_Sleep;
// 			dfSpanSum = dfSumSpan3;
// 		}
// 		nSleepPrecision = (DWORD)(round(1000 * dfSpanSum)/32);
// 		if (nSleepPrecision == 0)
// 			nSleepPrecision = 1;
// 	}
// 	void operator ()(DWORD nTimems)
// 	{
// 		switch(nSleepWay)
// 		{
// 		case Sys_Sleep:
// 			::Sleep(nTimems);
// 			break;		
// 		case Wmm_Sleep:
// 		{
// 			timeBeginPeriod(1); //���þ���Ϊ1����
// 			::Sleep(nTimems);	//��ǰ�̹߳���һ����
// 			timeEndPeriod(1);	//������������
// 		}
// 			break;
// 		case Std_Sleep:
// 			//std::this_thread::sleep_for(std::chrono::nanoseconds(nTimems*1000));
// 			Sleep(1);
// 			break;
// 		default:
// 			assert(false);
// 			break;
// 		}
// 	}
// 	inline DWORD GetPrecision()
// 	{
// 		return nSleepPrecision;
// 	}
// 
// private:
// 	SleepWay	nSleepWay;
// 	DWORD		nSleepPrecision;
// };
// 
// extern CThreadSleep ThreadSleep;
#define GetSleepPricision()	ThreadSleep.GetPrecision();
#define  SaveWaitTime()	CWaitTime WaitTime(__FILE__,__LINE__,__FUNCTION__);

class CWaitTime
{
	DWORD dwTimeEnter;
	char szFile[512];
	int nLine;
	char szFunction[256];
public:
	CWaitTime(char *szInFile, int nInLine, char *szInFunction)
	{
		dwTimeEnter = timeGetTime();
		strcpy(szFile, szInFile);
		nLine = nInLine;
		strcpy(szFunction, szInFunction);
	}
	~CWaitTime()
	{
		if ((timeGetTime() - dwTimeEnter) > 200)
		{
			char szText[1024] = { 0 };
			sprintf(szText, "Wait Timeout @File:%s %d(%s) WaitTime = %d(ms).\n", szFile, nLine, szFunction, (timeGetTime() - dwTimeEnter));
			OutputDebugStringA(szText);
		}
	}
};

/// ��ʱ�����У��������ý�嶨ʱ������û�ж�ý�嶨ʱ����������ƿ��
class CTimerQueue
{
private:
	HANDLE m_hTimerQueue;
	CRITICAL_SECTION m_csTimerList;
	map<HANDLE,HANDLE> m_TimerMap;
public:
	CTimerQueue()
	{
		InitializeCriticalSection(&m_csTimerList);
		m_hTimerQueue = CreateTimerQueue();
	}
	~CTimerQueue()
	{
		if (m_hTimerQueue)
		{
			DeleteTimerQueue(m_hTimerQueue);
			m_hTimerQueue = nullptr;
		}
		EnterCriticalSection(&m_csTimerList);
		for (auto it = m_TimerMap.begin(); it != m_TimerMap.end();)
		{
			DeleteTimerQueueTimer(m_hTimerQueue, it->first, nullptr);
			it = m_TimerMap.erase(it);
		}
		LeaveCriticalSection(&m_csTimerList);
		DeleteCriticalSection(&m_csTimerList);
	}

	inline HANDLE TimerQueue()
	{
		return m_hTimerQueue;
	}

	/// ������ʱ��
	/// pTimerRoutine	��ʱ���ص�����������Ϊ:
	///					VOID CALLBACK WaitOrTimerCallback(IN PVOID   pContext,IN bool TimerOrWaitFired);
	/// pContext		���ݸ���ʱ���ص������Ĳ���
	/// dwDueTime		��ʱ���ɹ������󣬶�ʱ����һ����Ӧ�����ʱ��(��λ����)������ʱ��Ϊ0����ʱ��������Ӧ
	/// dwPeriod		��ʱ����Ӧ���ڣ�����ֵΪ0����ʱ��ֻ��Ӧһ�Σ�����ÿ����Ӧʱ�����Ӧһ�Σ�ֱ��ȡ����ʱ��
	HANDLE CreateTimer(WAITORTIMERCALLBACK pTimerRoutine, void *pContext, DWORD dwDueTime, DWORD dwPeriod)
	{
		HANDLE hTimer = nullptr;
		if (!CreateTimerQueueTimer(&hTimer, m_hTimerQueue, pTimerRoutine, pContext, dwDueTime, dwPeriod, 0))
		{
			return nullptr;
		}
		else
		{
			EnterCriticalSection(&m_csTimerList);
			m_TimerMap.insert(pair<HANDLE,HANDLE>(hTimer,hTimer));
			LeaveCriticalSection(&m_csTimerList);
			return hTimer;
		}
			
	}

	/// ɾ����ʱ��
	/// hTimer			��ʱ�����
	/// hCompleteEvent	��ʱ��ɾ���ɹ�֪ͨ�¼���ϵͳ�Ѿ�ȡ����ʱ��ʱ���Ҷ�ʱ���ص��������ʱ������¼����ò�����ΪNULL,
	///					ȡֵΪINVALID_HANDLE_VALUEʱ��DeleteTimer�ȴ����������еĶ�ʱ���ص�������ɺ�ŷ��أ�
	///					ȡֵΪNULLʱ��DeleteTimer��Ϊ��ʱ������ɾ����ǲ��������أ�����ʱ����ʧЧ����ʱ���ص���������
	///					�н�����Ȼ�󲻻����κ�֪ͨ���������������߲���ʹ�ø�ѡ�����Ӧ�ȴ���ʱ���ص��������н�����Ȼ��
	///					ִ�������������������
	void DeleteTimer(HANDLE &hTimer, HANDLE hCompleteEvent = nullptr)
	{
		if (hTimer)
		{
			DeleteTimerQueueTimer(m_hTimerQueue, hTimer, hCompleteEvent);
			EnterCriticalSection(&m_csTimerList);
			m_TimerMap.erase(hTimer);
			LeaveCriticalSection(&m_csTimerList);
			hTimer = nullptr;
		}
	}
};