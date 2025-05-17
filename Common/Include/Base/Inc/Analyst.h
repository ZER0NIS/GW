//	Analyst.h
//	效率分析模块, 在common工程之下, 其它工程之上
//

#ifndef _ANALYST_ANALYST_H_
#define _ANALYST_ANALYST_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <Mmsystem.h>
#pragma warning(disable:4786)

#include <vector>
#include <string>
#include <map>
#include "types.h"

#include "SyncObjs.h"
#include "AnalystDef.h"
#include "SysTimer.h"

namespace sbase
{
	class CAnalyst
	{
	public:
		CAnalyst();
		virtual ~CAnalyst();

		static CAnalyst& Instance();
	public:
		bool	AnalystAdd(int idx, int mask, const char* name);

	public:
		void	TicksAdd(int idx, I64 ticks);
		void	SizeAdd(int idx, I64 size);
		void	AmountAdd(int idx, I64 nAmount) {}

	public:		// 按name索引，消耗会大一些
		void	TicksAdd(const char* szKey, I64 ticks);
		void	SizeAdd(const char* szKey, I64 size);
		void	AmountAdd(const char* szKey, I64 nAmount);
	public:
		void ReStart(); 	// Clear all history
		void LogToDisk(); // Log to disk
		void LogToConsole(); // Log to console
	protected:
		bool Create();
		void LogTicksToString(Statis_t* pStatis, char* szBuf, int nBufSize);
		void LogSizeToString(Statis_t* pStatis, char* szBuf, int nBufSize);
		void ClearData(Statis_t* pStatis);
	private:
		typedef std::map<std::string, Statis_t*> STATIS_MAP;
		STATIS_MAP m_mapStatis;

		typedef std::vector<Statis_t*> STATIS_SET;
		STATIS_SET m_setStatis;

		sbase::CCriticalSection m_xLock;
	};

	class CAnalystTick
	{
	public:
		CAnalystTick(OBJID idx)
		{
 m_idx = idx;
 CSysTimer::Instance().Sys_TickInit();
		}
		~CAnalystTick()
		{
 CAnalyst::Instance().TicksAdd(m_idx, CSysTimer::Instance().Sys_GetTicks());
		}

	private:
		OBJID	m_idx;
	};
}

#endif //_Analyst_H_