#include "..\inc\Timer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace sbase
{
	//////////////////////////////////////////////////////////////////////
	// CTimer 带时间积累的定时时钟
	//////////////////////////////////////////////////////////////////////
	bool
		CTimer::ToNextTick(int nSecs)
	{
		if (this->IsExpire(nSecs))
		{
 if (::time(NULL) >= m_tUpdateTime + nSecs * 2)
 	return this->Update(), true;
 else
 	return (m_tUpdateTime += nSecs), true;
		}
		else
 return false;
	}

	bool	CTimer::IsExpire(void)
	{
		time_t daa = time(NULL);
		return daa >= m_tUpdateTime + m_nInterval;
	}

	bool	CTimer::TimeOver(void)
	{
		if (IsActive() && IsExpire())
 return Clear(), true;
		return false;
	}

	//////////////////////////////////////////////////////////////////////
	// CTimerMS 带时间积累的定时时钟
	//////////////////////////////////////////////////////////////////////
	bool
		CTimerMS::ToNextTick(int nMilliSecs)
	{
		if (this->IsExpire(nMilliSecs))
		{
 if (::clock() >= m_tUpdateTime + nMilliSecs * 2)
 	return this->Update(), true;
 else
 	return (m_tUpdateTime += nMilliSecs), true;
		}
		else
 return false;
	}

	DWORD	CTimer::GetRemainSecsToZero()
	{
		return 0;
	}
}