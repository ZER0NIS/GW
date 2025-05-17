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

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <io.h>
#include <tchar.h>

#pragma warning(disable : 4786)
#include <map>
#include <set>
#include <string>

#include "..\inc\SvrBase.h"
#include "..\Inc\SyncObjs.h"
//#include "BlackList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;

namespace sbase
{
	CCriticalSection Lock;

	const ULONG _MAX_LOGSIZE = 20 * 1024 * 1024;
	/////////////////////////////////////////////////////////////////////////////
	void
		sbase::LogSave(const char* pszName, const char* fmt, ...)
	{
		Lock.Lock();
		if (!pszName || !fmt)
		{
			Lock.Unlock();
			return;
		}

		//Beep( 750, 10 );
		static char buffer[2 * 4096] = "";
		va_list ap;
		va_start(ap, fmt);
		//::vsprintf( buffer, fmt, (char*) ((&fmt)+1) );
		vsnprintf(buffer, 2 * 4096, fmt, ap);
		va_end(ap);

		// Release
		time_t ltime;
		::time(&ltime);

		struct tm* pTime;
		pTime = ::localtime(&ltime); /* Convert to local time. */

		char szLogFile[2 * 4096];
		::sprintf(szLogFile, "..\\..\\Log\\%s(%d-%d-%d).log", pszName, pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday);

		// Open log file
		FILE* fp = ::fopen(szLogFile, "a+");
		if (!fp)
		{
			Lock.Unlock();
			return;
		}

		// Write log file
		::fprintf(fp, "%s -- %s\n", buffer, ::ctime(&ltime));

		DWORD dwLogSize = ::filelength(::fileno(fp));
		::fclose(fp);

		// Log file is too large
		if (dwLogSize >= _MAX_LOGSIZE)
		{
			char szBackupFile[2 * 4096] = "";
			::sprintf(szBackupFile, "..\\..\\Log\\%s(%d-%d-%d.%u).log", pszName, pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday, TimeGet(TIME_DAYTIME));

			::rename(szLogFile, szBackupFile);
		}

		Lock.Unlock();
	}

	/////////////////////////////////////////////////////////////////////////////
	void
		sbase::SysLogSave(const char* fmt, ...)
	{
		static char buffer[1024] = "";
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(buffer, 1024, fmt, ap);
		va_end(ap);

		sbase::LogSave("SYS", buffer);
	}

	/////////////////////////////////////////////////////////////////////////////
	void    LogException(const char* psbuf, ...)
	{
		static char buffer[1024] = "";
		va_list ap;
		va_start(ap, psbuf);
		vsnprintf(buffer, 1024, psbuf, ap);
		va_end(ap);

		sbase::LogSave("Exception", buffer);
	}

	void    ItemLog(const char* file, const char* psbuf, ...)
	{
		static char buffer[1024] = "";
		va_list ap;
		va_start(ap, psbuf);
		vsnprintf(buffer, 1024, psbuf, ap);
		va_end(ap);

		sbase::LogSave(file, buffer);
	}

	/////////////////////////////////////////////////////////////////////////////
	void
		sbase::GmLogSave(const char* fmt, ...)
	{
		sbase::LogSave("GM", fmt);
	}

	/////////////////////////////////////////////////////////////////////////////
	void
		sbase::ErrorMsg(const char* fmt, ...)
	{
		if (!fmt)
			return;

		char buffer[1024];
		::vsprintf(buffer, fmt, (char*)((&fmt) + 1));

		::MessageBox(NULL, buffer, "Error", MB_OK | MB_ICONERROR);
		//::exit(-1);
	}

	/////////////////////////////////////////////////////////////////////////////
	void
		sbase::DebugMsg(const char* fmt, ...)
	{
		if (!fmt)
			return;

		char buffer[1024];
		::vsprintf(buffer, fmt, (char*)((&fmt) + 1));

		::OutputDebugString(buffer);
		::OutputDebugString("\n");
	}

	/////////////////////////////////////////////////////////////////////////////
	void
		sbase::DebugLogMsg(const char* fmt, ...)
	{
#ifdef _DEBUG
		if (!fmt)
			return;

		char buffer[1024] = "";
		::vsprintf(buffer, fmt, (char*)((&fmt) + 1));

		sbase::LogSave("DEBUG", buffer);
#endif
	}

	//////////////////////////////////////////////////////////////////////
	time_t
		sbase::TimeGet(TIME_TYPE type/*=TIME_MILLISECOND*/)
	{
		time_t dwTime = 0;
		switch (type)
		{
		case TIME_SECOND:
			dwTime = ::time(NULL);
			break;

		case TIME_MINUTE:
		{
			time_t long_time;
			time(&long_time);                /* Get time as long integer. */

			struct tm* pTime;
			pTime = ::localtime(&long_time); /* Convert to local time. */

			dwTime = pTime->tm_year % 100 * 100000000 +
				(pTime->tm_mon + 1) * 1000000 +
				pTime->tm_mday * 10000 +
				pTime->tm_hour * 100 +
				pTime->tm_min;
		}
		break;

		case TIME_DAY:
		{
			time_t long_time;
			time(&long_time);                /* Get time as long integer. */

			struct tm* pTime;
			pTime = ::localtime(&long_time); /* Convert to local time. */

			dwTime = pTime->tm_year * 10000 +
				(pTime->tm_mon + 1) * 100 +
				pTime->tm_mday;
		}
		break;

		case TIME_DAYTIME:
		{
			time_t long_time;
			time(&long_time);                /* Get time as long integer. */

			struct tm* pTime;
			pTime = ::localtime(&long_time); /* Convert to local time. */

			dwTime = pTime->tm_hour * 10000 +
				pTime->tm_min * 100 +
				pTime->tm_sec;
		}
		break;

		case TIME_STAMP:
		{
			time_t long_time;
			time(&long_time);                /* Get time as long integer. */

			struct tm* pTime;
			pTime = ::localtime(&long_time); /* Convert to local time. */

			dwTime = (pTime->tm_mon + 1) * 100000000 +
				pTime->tm_mday * 1000000 +
				pTime->tm_hour * 10000 +
				pTime->tm_min * 100 +
				pTime->tm_sec;
		}
		break;

		default:
			dwTime = ::timeGetTime();
			break;
		}

		return dwTime;
	}

	//////////////////////////////////////////////////////////////////////
	int
		sbase::RandGet(INT nMax, bool bReset)
	{
		if (nMax <= 0)
			nMax = 1;

		if (bReset)
			::srand(::timeGetTime());

		return ::rand() % nMax;
	}

	//////////////////////////////////////////////////////////////////////
	double
		sbase::RandomRateGet(double dRange)
	{
		double pi = 3.1415926;

		int nRandom = sbase::RandGet(999, false) + 1;
		double a = ::sin(nRandom * pi / 1000);
		double b;
		if (nRandom >= 90)
			b = (1.0 + dRange) - ::sqrt(::sqrt(a)) * dRange;
		else
			b = (1.0 - dRange) + ::sqrt(::sqrt(a)) * dRange;

		return b;
	}

	//////////////////////////////////////////////////////////////////////
	bool
		sbase::StrCopy(char* pszTarget, const char* pszSource, UINT unBufSize)
	{
		if (!pszTarget || !pszSource || unBufSize <= 0)
			return false;

		if (::strlen(pszSource) > unBufSize)
		{
			::strncpy(pszTarget, pszSource, unBufSize);
			pszTarget[unBufSize] = 0;
		}
		else
			::strcpy(pszTarget, pszSource);

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	bool
		sbase::StrCheck(const char* pszString)
	{
		if (!pszString)
			return false;

		int nLen = (int)::strlen(pszString);
		for (int i = 0; i < nLen; i++)
		{
			unsigned char c = (unsigned char)pszString[i];
			if (c >= 0x81 && c <= 0xfe)
			{
				if (i + 1 >= nLen)
					return false;

				unsigned char c2 = (unsigned char)pszString[i + 1];
				if (c2 < 0x40 && c2 > 0x7e && c2 < 0x80 && c2 > 0xfe)
					return false;
				else
					i++;
			}
			else
			{
				if (c == 0x80 || c < ' ')
					return false;
			}
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	bool
		sbase::LegalStrCheck(const char* pszString)
	{
		if (!pszString)
			return false;

		int nLen = (int)::strlen(pszString);
		for (int i = 0; i < nLen; i++)
		{
			unsigned char c = pszString[i];
			switch (c)
			{
			case ' ':
			case ';':
			case ',':
			case '/':
			case '\\':
			case '=':
			case '%':
			case '@':
			case '\'':
			case '#':
				return false;
			}
		}

		//if (rade_sbase::BlackList::IsMember(pszString))
		//	return false;

		return sbase::StrCheck(pszString);
	}

	HANDLE	g_hStdOut = NULL;
	WORD	s_wConsoleWidth = 80;
	WORD	s_wConsoleHeight = 40;
	//////////////////////////////////////////////////////////////////////
	void
		sbase::ConsoleSetup(DWORD dwWindowSizeX, DWORD dwWindowSizeY)
	{
		s_wConsoleWidth = (WORD)__min(80, dwWindowSizeX);
		s_wConsoleHeight = (WORD)__min(80, dwWindowSizeY);

		g_hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);

		SMALL_RECT rcWindow = { 0, 0, s_wConsoleWidth - 1, s_wConsoleHeight - 1 };

		::SetConsoleWindowInfo(g_hStdOut, TRUE, &rcWindow);

		COORD crdBufferSize;
		crdBufferSize.X = s_wConsoleWidth;
		crdBufferSize.Y = s_wConsoleHeight;
		::SetConsoleScreenBufferSize(g_hStdOut, crdBufferSize);

		// Write a blank string first
// 		for( int i=rcWindow.Top; i<rcWindow.Bottom; i++ )
// 		{
// 			coord.Y = (WORD)i;
// 			::SetConsoleCursorPosition( g_hStdOut, coord );
//
// 			TCHAR strEmpty[255] = TEXT("                                                                                                                                                                                                                                                              ");
// 			::WriteConsole( g_hStdOut, strEmpty, rcWindow.Right + 1, &dwWritten, NULL );
// 		}
	}

	//////////////////////////////////////////////////////////////////////
	void
		sbase::ConsoleWriteStr(DWORD nCoordX, DWORD nCoordY, const char* fmt, ...)
	{
		char buf[2048] = "";
		_vstprintf(buf, fmt, (CHAR*)((&fmt) + 1));

		int nLen = (int)__max(1, ::strlen(buf));
		if (nLen > s_wConsoleWidth)
			buf[s_wConsoleWidth] = 0;
		else
		{
			for (int i = nLen; i < s_wConsoleWidth; i++)
				buf[i] = ' ';
		}

		// Write strBuffer at (0,nCoordY)
		DWORD dwWritten;
		COORD coord = { (WORD)nCoordX, (WORD)nCoordY };
		::WriteConsoleOutputCharacter(g_hStdOut, buf, s_wConsoleWidth, coord, &dwWritten);
	}

	void  ConsoleWriteColorText(DWORD nColor, const char* fmt, ...)
	{
		char buf[2048] = "";
		_vstprintf(buf, fmt, (CHAR*)((&fmt) + 1));

		SetConsoleTextAttribute(g_hStdOut, (WORD)nColor);
		printf(buf);
		printf("\n");
		SetConsoleTextAttribute(g_hStdOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}

	void  ConsoleWriteText(const char* fmt, ...)
	{
		char buf[2048] = "";
		_vstprintf(buf, fmt, (CHAR*)((&fmt) + 1));

		printf(buf);
		printf("\n");
	}

	void  SetConsoleFontColor(DWORD Color)
	{
		SetConsoleTextAttribute(g_hStdOut, (WORD)Color);
	}

	void    SetConsoleTitle(const char* fmt, ...)
	{
		char buf[2048] = "";
		_vstprintf(buf, fmt, (CHAR*)((&fmt) + 1));
		::SetConsoleTitle(buf);
	}

	//////////////////////////////////////////////////////////////////////
	//	CnStr.txt
	//////////////////////////////////////////////////////////////////////
	static int GetLine(FILE* fp, char* pszLine, int nBufSize)
	{
		IF_NOT(fp && pszLine && nBufSize > 0)
			return -2;

		memset(pszLine, 0L, nBufSize);

		int pt = 0;
		for (;;)
		{
			int c = fgetc(fp);
			if (c == EOF)
			{
				if (pt <= 0)
					return EOF;
				else
					return 0;
			}

			if (c == '\n')
			{
				return 0;
			}

			pszLine[pt++] = (char)c;
			IF_NOT(pt < nBufSize)
				return -3;
		}
	}

	//////////////////////////////////////////////////////////////////////
	static DWORD GetStrID(const char* pszLine)
	{
		IF_NOT(pszLine)
			return 0;

		DWORD dwKey = 0;
		if (1 == sscanf(pszLine, "#define IDS_%u", &dwKey))
			return dwKey;
		else
			return 0;
	}

	//////////////////////////////////////////////////////////////////////
	static bool TakeOutMsg(const char* pszLine, char* pszMsgOut)
	{
		IF_NOT(pszLine && pszMsgOut)
			return false;

		// search the first "
		const char* pszMsg = strstr(pszLine, "\"");
		IF_NOT(pszMsg && strlen(pszMsg) > 0)
			return false;

		pszMsg++;
		strcpy(pszMsgOut, pszMsg);

		int nLen = (int)strlen(pszMsgOut);
		if (nLen > 0)	// not empty line
			pszMsgOut[nLen - 1] = 0;	// delete the last "

		return true;
	}

	map<DWORD, string> setCnStr;
	//////////////////////////////////////////////////////////////////////
	bool
		sbase::CnStrInit()
	{
		// read txt infos
		const char szIniFile[] = "cnstr.txt";
		FILE* fp = fopen(szIniFile, "r");
		if (!fp)
		{
			ErrorMsg("Error: ini file %s not found.", szIniFile);
			return false;
		}

		for (;;)
		{
			char szLine[1024] = "";
			int rval = GetLine(fp, szLine, sizeof(szLine));
			if (0 != rval)
			{
				if (EOF != rval)
					ErrorMsg("error in get line of %s", szIniFile);
				break;
			}

			DWORD dwKey = GetStrID(szLine);
			if (0 == dwKey)
			{
				ErrorMsg("error in get key of line (%s) in %s", szLine, szIniFile);
				break;
			}

			char szMsg[1024] = "";
			if (!TakeOutMsg(szLine, szMsg))
			{
				ErrorMsg("error in take out msg of line (%s) in %s", szLine, szIniFile);
				break;
			}

			setCnStr[dwKey] = szMsg;
		}

		fclose(fp);

		// 保证最后一条IDS_101707正常，无遗漏.
		if (!CnStrGet("IDS_101713"))
			return false;

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	void
		sbase::CnStrDestroy()
	{
		setCnStr.clear();
	}

	//////////////////////////////////////////////////////////////////////
	char*
		sbase::CnStrGet(char* pszStrIndex)
	{
		IF_NOT(pszStrIndex)
			return NULL;

		DWORD dwKey = 0;
		if (1 != ::sscanf(pszStrIndex, "IDS_%u", &dwKey))
			return NULL;

		map<DWORD, string>::const_iterator iter = setCnStr.find(dwKey);
		if (iter == setCnStr.end())
			return NULL;

		return const_cast<char*>((*iter).second.c_str());
	}

	//HRESULT sbase::HresultFromLastError()
	//{
	//	DWORD dwErr = ::GetLastError();
	//	return HRESULT_FROM_WIN32(dwErr);
	//}
	bool sbase::LogStrCheck(char* pszString)
	{
		return true;
	}
	PBYTE	DumpExceptionAddress(int nLevel, char* pBuf, PEXCEPTION_POINTERS pException)
	{
		return (PBYTE)pBuf;
	}

	DWORD   CalcUserMoneyChksum(DWORD dwUserID, INT64 n64Money)
	{
		return 0;
	}
	DWORD   CalcUserTokenChksum(DWORD dwUserID, DWORD dwToken)
	{
		return 0;
	}
}