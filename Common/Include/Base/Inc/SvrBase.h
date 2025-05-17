//========================================================
//
//    Copyright (c) 2006,欢乐连线工作室
//    All rights reserved.
//
//    文件名称 ： Player.h
//    摘    要 ： 动态对象功能模块
//
//    当前版本 ： 1.01
//    作    者 ： 李锋军
//    完成日期 ： 2007-01-17
//
//    修    改 ： zeng cs
//    完成日期 ： 2004/11/03
//
//    备注:  1)增加了Release版本的Log输出
//========================================================

#pragma once

#pragma warning(disable:4996)
#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include "Types.h"
#include "Heap.h"
#include "String.h"
#include "ILogger.h"
#include "IException.h"
//-------------------Memory leak--------------------------------
#define _CRTDBG_MAP_ALLOC
#include<stdlib.h>
#include<crtdbg.h>
//--------------------------------------------------------------

// macro...
#undef	SAFE_DELETE
#define SAFE_DELETE(ptr)		{ if(ptr){ try{ delete ptr; }catch(...){ LOGSYS->OutError("CATCH: *** SAFE_DELETE() crash! *** at %s, %d", __FILE__, __LINE__); } ptr = 0; } }

#undef	SAFE_RELEASE
#define SAFE_RELEASE(ptr)		{ if(ptr){ try{ ptr->Release(); }catch(...){ LOGSYS->OutError("CATCH: *** SAFE_RELEASE() crash! *** at %s, %d", __FILE__, __LINE__); } ptr = 0; } }

#undef	SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY( p )		{ if(( p ) != NULL) { try{ delete[] ( p ); }catch(...){ LOGSYS->OutError("CATCH: *** SAFE_RELEASE() crash! *** at %s, %d", __FILE__, __LINE__); } (p)=0; } }

#undef	ASSERT
// #undef	CHECK
#undef	IF_OK
#undef	IF_NOT
#undef	PURE_VIRTUAL

#define LOG(name)		sLog::GetLogger(name, sLog::LOG_APPENDER_FILE, name)
#define LOGSYS			LOG("SYS")
#define LOGERR			LOG("Error_Logic")
#define LOGMSG			LOG("Msg")
#define LOGDATA			LOG("Data")
#define LOGGM			LOG("GM")
#define LOGMONEY		LOG("Money")
#define LOGGOLD			LOG("Gold")

#ifdef	_DEBUG
#define		ASSERT			assert

//	#define		IF_OK(x)		if( ((x)) ? 1 : ( assert(!("IF_OK: " #x)),0 ) )
//	#define		IF_NOT(x)		if( (!(x)) ? ( assert(!("IF_NOT: " #x)),1 ) : 0 )

#define		IF_OK(x)		if( ((x)) ? 1 : ( LOGSYS->OutError("IF_OK", "★IF_OK(%s)★ in %s, %d", #x, __FILE__, __LINE__),0 ) )
#define		IF_NOT(x)		if( (!(x)) ? ( LOGSYS->OutError("IF_NOT", "★IF_NOT(%s)★ in %s, %d", #x, __FILE__, __LINE__),1 ) : 0 )

#define		PURE_VIRTUAL	= 0;
#else
#define		ASSERT(x)		 if(!(x)) { LOGSYS->OutError("ASSERT", "★ASSERT(%s)★ in %s, %d", #x, __FILE__, __LINE__);  }

#define		IF_OK(x)		if( ((x)) ? 1 : ( LOGSYS->OutError("IF_OK", "★IF_OK(%s)★ in %s, %d", #x, __FILE__, __LINE__),0 ) )
#define		IF_NOT(x)		if( (!(x)) ? ( LOGSYS->OutError("IF_NOT", "★IF_NOT(%s)★ in %s, %d", #x, __FILE__, __LINE__),1 ) : 0 )

#define		PURE_VIRTUAL	{ ASSERT(!"PURE_VIRTUAL_FUNCTION"); }
#endif

#define		DEAD_LOOP_BREAK(x,n){ if(++(x) > (n)){ ASSERT(!"DEAD_LOCK_BREAK"); break; } }

#define _EXCEPTION_TRY     //采用try异常处理方式
//#define _DUMPMINI_FILE		//异常发生时打印dump文件

#ifndef _EXCEPTION_TRY
#define		DEBUG_TRY
#define		DEBUG_CATCH(s)
#else
#define		DEBUG_TRY		try{
#define		DEBUG_CATCH(s)		}catch(std::exception& ex){char szFuncDump[1024];LOGSYS->OutInfo( #s,"CATCH(%s): * %s %d crash! %s*", ex.what(), __FILE__, __LINE__, sbase::DumpExceptionAddress(6, szFuncDump));}catch(sbase::IException& ex){char szFuncDump[1024];LOGSYS->OutInfo(#s, "CATCH(...): crash! %s * %s %d *", sbase::DumpExceptionAddress(6, szFuncDump, ex.GetExceptionInfo()), __FILE__, __LINE__ ); char szBuf[4096]; ex.TraceStack(szBuf);	LOGSYS->OutInfo(ex.GetName(),"*** Crash Begin! *** \n%s *** Crash End! ***",szBuf); }catch(...){ char szFuncDump[1024];LOGSYS->OutInfo( #s,"CATCH(...): *** %s %d crash! %s***", __FILE__, __LINE__, sbase::DumpExceptionAddress(6, szFuncDump)); }

#endif

#define		STACK_TRACE {char szBuf[4096]; sbase::StackTrace(szBuf);	LOGSYS->OutInfo("*** stack top *** \nSTACK_TRACE\n%s *** stack bottom ***", szBuf);}
#define		STACK_TRACE_ONCE {static bool bFirstTime = true; if(bFirstTime) {bFirstTime = false; char szBuf[4096]; wd::StackTrace(szBuf);	LOGSYS->OutInfo("*** stack top *** \nSTACK_TRACE\n%s *** stack bottom ***", szBuf);}}

// functions...
namespace sbase
{
	void	LogSave(const char* pszName, const char* fmt, ...);
	void	SysLogSave(const char* fmt, ...);
	void	GmLogSave(const char* fmt, ...);
	void	ErrorMsg(const char* fmt, ...);
	void	DebugMsg(const char* fmt, ...);
	void	DebugLogMsg(const char* fmt, ...);
	void    LogException(const char* psbuf, ...);
	void    ItemLog(const char* file, const char* psbuf, ...);

	PBYTE	DumpExceptionAddress(int nLevel, char* pBuf, PEXCEPTION_POINTERS pException = NULL);

	enum TIME_TYPE { TIME_MILLISECOND = 0, TIME_SECOND, TIME_MINUTE, TIME_DAY, TIME_DAYTIME, TIME_STAMP };
	time_t	TimeGet(TIME_TYPE type = TIME_MILLISECOND);

	int		RandGet(INT nMax, bool bRest = false);
	double	RandomRateGet(double dRange);

	bool	StrCopy(char* pszTarget, const char* pszSource, UINT unBufSize);
	LPCTSTR	DumpBuffer(const char* buf, UINT nSize);
	bool	StrCheck(const char* pszString);
	bool	LegalStrCheck(const char* pszString);

	bool	LogStrCheck(char* pszString);

	void	ConsoleWriteStr(DWORD nCoordX, DWORD nCoordY, const char* fmt, ...);
	void	ConsoleSetup(DWORD dwWindowSizeX = 80, DWORD dwWindowSizeY = 80);
	void    ConsoleWriteColorText(DWORD nColor, const char* fmt, ...);
	void    ConsoleWriteText(const char* fmt, ...);
	void    SetConsoleFontColor(DWORD Color = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
	void    SetConsoleTitle(const char* fmt, ...);

	bool	CnStrInit(void);
	void	CnStrDestroy(void);
	char* CnStrGet(char* pszStrIndex);

	ULONG	MaxAxisOffsetGet(OBJPOS posThis, OBJPOS posThat);
	bool	IsValidDistance(OBJPOS lhs, OBJPOS rhs, UINT unDistance);

	DWORD   CalcUserMoneyChksum(DWORD dwUserID, INT64 n64Money);
	DWORD   CalcUserTokenChksum(DWORD dwUserID, DWORD dwToken);
}
//
//#define LOG		sbase::LogSave
//#define LOGSYS	sbase::SysLogSave
//#define LOGERR	sbase::ErrorMsg
//#define LOGDBG	sbase::DebugMsg
//#define LOGGM	sbase::GmLogSave
//
