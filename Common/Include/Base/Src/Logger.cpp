//#include "stdafx.h"
#include "..\inc\Logger.h"

#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <io.h>

#include "..\inc\LogSystem.h"
using namespace sLog;

//const ULONG _MAX_LOGSIZE = 20*1024*1024;
const ULONG _MAX_LOGSIZE = 1024 * 1024;

CLogger::CLogger(const char* pszLoggerName, LOG_APPENDER eAppenderType, const char* pszParam)
	:m_strLoggerName(pszLoggerName)
{
	m_nLogAppender = 0;
	m_eLogLevel = LOG_LEVEL_DEBUG;
	m_fd = -1;
	m_nMDayFileCreate = 0;

	this->AddAppender(eAppenderType, pszParam);
}

CLogger::~CLogger()
{
	this->Close();
}

void CLogger::Close()
{
	if (m_fd != -1)
	{
		::close(m_fd);
		m_fd = -1;
	}
}

bool CLogger::Open()
{
	if (m_strFileKey.empty())
		return false;

	this->Close();

	time_t ltime;
	::time(&ltime);

	struct tm* pTime;
	pTime = ::localtime(&ltime); /* Convert to local time. */

	char szLogFile[MAX_LOGBUFFER];
	::sprintf_s(szLogFile, MAX_LOGBUFFER, "..\\..\\Log\\%s(%d-%d-%d).log", m_strFileKey.c_str(), pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday);

	m_fd = ::open(szLogFile, O_CREAT | O_APPEND | O_WRONLY, _S_IREAD | _S_IWRITE);
	if (m_fd < 0)
	{
		::fprintf(stdout, "::open File(%s) Failed\n", szLogFile);
		return false;
	}

	m_strCurFileName = szLogFile;
	m_nMDayFileCreate = pTime->tm_mday;
	return true;
}

void CLogger::OutDebug(const char* pszEvent, const char* format, ...)
{
	if (!pszEvent)
		return;

	if (!format)
		return;

	char szBuf[MAX_LOGBUFFER] = { 0 };

	va_list args;
	va_start(args, format);
	vsprintf_s(szBuf, MAX_LOGBUFFER, format, args);
	va_end(args);

	this->OutLog(pszEvent, LOG_LEVEL_DEBUG, szBuf);
}

void CLogger::OutInfo(const char* pszEvent, const char* format, ...)
{
	if (!pszEvent)
		return;

	if (!format)
		return;

	char szBuf[MAX_LOGBUFFER] = { 0 };

	va_list args;
	va_start(args, format);
	vsprintf_s(szBuf, MAX_LOGBUFFER, format, args);
	va_end(args);

	this->OutLog(pszEvent, LOG_LEVEL_INFO, szBuf);
}

void CLogger::OutWarn(const char* pszEvent, const char* format, ...)
{
	if (!pszEvent)
		return;

	if (!format)
		return;

	char szBuf[MAX_LOGBUFFER] = { 0 };

	va_list args;
	va_start(args, format);
	vsprintf_s(szBuf, MAX_LOGBUFFER, format, args);
	va_end(args);

	this->OutLog(pszEvent, LOG_LEVEL_WARN, szBuf);
}

void CLogger::OutError(const char* pszEvent, const char* format, ...)
{
	if (!pszEvent)
		return;

	if (!format)
		return;

	char szBuf[MAX_LOGBUFFER] = { 0 };

	va_list args;
	va_start(args, format);
	vsprintf_s(szBuf, MAX_LOGBUFFER, format, args);
	va_end(args);

	this->OutLog(pszEvent, LOG_LEVEL_ERROR, szBuf);
}

void CLogger::OutDebug(const char* format, ...)
{
	if (!format)
		return;

	char szBuf[MAX_LOGBUFFER] = { 0 };

	va_list args;
	va_start(args, format);
	vsprintf_s(szBuf, MAX_LOGBUFFER, format, args);
	va_end(args);

	this->OutLog(m_strLoggerName.c_str(), LOG_LEVEL_DEBUG, szBuf);
}

void CLogger::OutInfo(const char* format, ...)
{
	if (!format)
		return;

	char szBuf[MAX_LOGBUFFER] = { 0 };

	va_list args;
	va_start(args, format);
	vsprintf_s(szBuf, MAX_LOGBUFFER, format, args);
	va_end(args);

	this->OutLog(m_strLoggerName.c_str(), LOG_LEVEL_INFO, szBuf);
}
void CLogger::OutWarn(const char* format, ...)
{
	if (!format)
		return;

	char szBuf[MAX_LOGBUFFER] = { 0 };

	va_list args;
	va_start(args, format);
	vsprintf_s(szBuf, MAX_LOGBUFFER, format, args);
	va_end(args);

	this->OutLog(m_strLoggerName.c_str(), LOG_LEVEL_WARN, szBuf);
}
void CLogger::OutError(const char* format, ...)
{
	if (!format)
		return;

	char szBuf[MAX_LOGBUFFER] = { 0 };

	va_list args;
	va_start(args, format);
	vsprintf_s(szBuf, MAX_LOGBUFFER, format, args);
	va_end(args);

	this->OutLog(m_strLoggerName.c_str(), LOG_LEVEL_ERROR, szBuf);
}

void CLogger::OutLog(const char* pszEvent, const LOG_LEVEL eLevel, char* pszMsg)
{
	if (!pszEvent)
		return;

	if (!pszMsg)
		return;

	if (eLevel < m_eLogLevel)
	{
		return;
	}

	// Release
	time_t ltime;
	::time(&ltime);

	struct tm* pTime;
	pTime = ::localtime(&ltime); /* Convert to local time. */

	sbase::CSingleLock lock(&m_Lock);

	if (this->IsOut2Console())
	{
		::fprintf(stdout, "%s LogLevel=%d-- %s\n", pszMsg, eLevel, ::ctime(&ltime));

#ifdef _DEBUG
		::OutputDebugStringA(pszMsg);
#endif
	}

	if (this->IsOut2File())
	{
#ifdef _SLOW_MODE
		char szLogFile[2 * 4096];
		::sprintf(szLogFile, "..\\..\\Log\\%s(%d-%d-%d).log", pszEvent, pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday);

		// Open log file
		FILE* fp = ::fopen(szLogFile, "a+");
		if (!fp)
		{
			return;
		}

		// Write log file
		::fprintf(fp, "%s LogLevel=%d-- %s\n", szBuf, eLevel, ::ctime(&ltime));

		DWORD dwLogSize = ::filelength(::fileno(fp));
		::fclose(fp);

		// Log file is too large
		if (dwLogSize >= _MAX_LOGSIZE)
		{
			char szBackupFile[2 * 4096] = "";
			::sprintf(szBackupFile, "..\\..\\Log\\%s(%d-%d-%d.%u).log", pszEvent, pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday, sbase::TimeGet(sbase::TIME_DAYTIME));

			::rename(szLogFile, szBackupFile);
		}
#else

		if (pTime->tm_mday != this->GetFileCreateDay())
		{
			if (!this->Open())
			{
				::fprintf(stdout, "%s Logger Open Failed\n", m_strLoggerName.c_str());
				return;
			}
		}
		char szLogBuf[MAX_LOGBUFFER] = { 0 };
		int nLen = sprintf_s(szLogBuf, MAX_LOGBUFFER, "Event:%s-- %s LogLevel=%d-- %s\n", pszEvent, pszMsg, eLevel, ::ctime(&ltime));
		if (nLen > 0)
		{
			::write(m_fd, szLogBuf, nLen);

			DWORD dwLogSize = ::filelength(m_fd);

			// Log file is too large
			if (dwLogSize >= _MAX_LOGSIZE)
			{
				char szBackupFile[MAX_LOGBUFFER] = "";
				::sprintf(szBackupFile, "..\\..\\Log\\%s(%d-%d-%d.%u).log", m_strFileKey.c_str(), pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday, sbase::TimeGet(sbase::TIME_DAYTIME));

				this->Close();
				::rename(m_strCurFileName.c_str(), szBackupFile);
				if (!this->Open())
				{
					::fprintf(stdout, "%s Logger Open Failed\n", m_strLoggerName.c_str());
					return;
				}
			}
		}
#endif
	}

	if (this->IsOut2DB())
	{
		if (!sbase::LogStrCheck(pszMsg))
		{
#ifdef _DEBUG
			char szOutput[MAX_LOGBUFFER] = { 0 };
			sprintf_s(szOutput, MAX_LOGBUFFER, "非法字符 Content:%s FILE :%s Line:%d\n", pszMsg, __FILE__, __LINE__);
			::OutputDebugStringA(szOutput);
#endif
			return;
		}
		if (m_strProcedureFormat.empty())
		{
#ifdef _DEBUG
			char szOutput[MAX_LOGBUFFER] = { 0 };
			sprintf_s(szOutput, MAX_LOGBUFFER, "日志调用存储过程失败 Content:%s FILE :%s Line:%d\n", pszMsg, __FILE__, __LINE__);
			::OutputDebugStringA(szOutput);
#endif
			return;
		}

		CLogSystem::Instance()->Log2DB(m_strProcedureFormat.c_str(), pszEvent, eLevel, pszMsg);
	}
}

//void CLogger::OutLog(int nEventType, const LOG_LEVEL eLevel, const char * format, ...)
//{
//
//	if (!format)
//		return;
//
//	char szBuf[MAX_LOGBUFFER] = {0};
//
//	va_list args;
//	va_start(args, format);
//
//	vsprintf_s(szBuf, MAX_LOGBUFFER, format, args);
//#ifdef _DEBUG
//	::OutputDebugStringA(szBuf);
//#endif
//}

bool CLogger::AddAppender(const LOG_APPENDER eAppenderType, const char* pszParam/* = NULL*/)
{
	m_nLogAppender |= eAppenderType;

	if (eAppenderType == LOG_APPENDER_DB && pszParam)
	{
		m_strProcedureFormat = pszParam;
	}
	else if (eAppenderType == LOG_APPENDER_FILE && pszParam)
	{
		m_strFileKey = pszParam;

		if (!this->Open())
		{
			return false;
		}
	}

	return true;
}