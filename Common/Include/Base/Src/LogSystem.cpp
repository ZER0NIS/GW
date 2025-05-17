//#include "stdafx.h"
#include "..\inc\LogSystem.h"
#include <stdarg.h>
#include <time.h>

using namespace sLog;

const int MAX_LOGGER = 100;

//////////////////////////////////////////////////////////////////////////
//static
CLogSystem* CLogSystem::s_Instance = NULL;

CLogSystem* CLogSystem::Instance()
{
	if (s_Instance == NULL)
	{
		s_Instance = new CLogSystem;
	}

	return s_Instance;
}

ULONG	CLogSystem::Release()
{
	if (s_Instance != NULL)
	{
		SAFE_DELETE(s_Instance);
	}
	return 0;
}

CLogSystem::CLogSystem()
{
	m_vecLogger.reserve(MAX_LOGGER);

	m_pDataBase = NULL;
}

CLogSystem::~CLogSystem()
{
	for (int i = 0; i < m_vecLogger.size(); i++)
	{
		SAFE_DELETE(m_vecLogger[i]);
	}

	m_vecLogger.clear();

	SAFE_RELEASE(m_pDataBase);
}

ILogger* CLogSystem::GetLogger(const char* pszLoggerName, const LOG_APPENDER eAppenderType, const char* pszParam)
{
	if (!pszLoggerName)
	{
		return NULL;
	}

	ILogger* pLogger = this->FindLogger(pszLoggerName);
	if (!pLogger)
	{
		pLogger = new CLogger(pszLoggerName, eAppenderType, pszParam);

		if (pLogger)
		{
 m_vecLogger.push_back(pLogger);
		}
	}

	return pLogger;
}

bool	 CLogSystem::ReleaseLogger(const char* pszLoggerName)
{
	return true;
}

ILogger* CLogSystem::FindLogger(const char* pszLoggerName)
{
	if (!pszLoggerName)
		return NULL;

	for (int i = 0; i < m_vecLogger.size(); ++i)
	{
		if (stricmp(pszLoggerName, m_vecLogger[i]->GetName()) == 0)
		{
 return m_vecLogger[i];
		}
	}

	return NULL;
}

bool	CLogSystem::CreateDbInstance(const char* pszSer, const char* pszUser, const char* pszPsd, const char* pszDBname)
{
	if (!pszSer || !pszUser || !pszPsd || !pszDBname)
		return false;

	SAFE_RELEASE(m_pDataBase);

	m_pDataBase = rade_db::DatabaseCreate(pszSer, pszUser, pszPsd, pszDBname);
	assert(m_pDataBase);

	if (!m_pDataBase)
		return false;

	return true;
}

bool	CLogSystem::Log2DB(const char* pszProcedure, const char* pszEventType, int nLogLevel, const char* pszContent)
{
	if (!pszProcedure)
		return false;

	if (!pszEventType)
		return false;

	if (!pszContent)
		return false;

	if (!m_pDataBase)
		return false;

	char szBuf[MAX_LOGBUFFER] = { 0 };
	if (sprintf_s(szBuf, MAX_LOGBUFFER, pszProcedure, pszEventType, nLogLevel, pszContent, ::time(NULL)) > 0)
	{
		m_pDataBase->ExecuteAsyncSQL(szBuf, NULL, NULL);
	}

	return true;
}

bool	CLogSystem::Log2DB(const char* pszProcedure)
{
	if (!pszProcedure)
		return false;

	if (!m_pDataBase)
		return false;

	m_pDataBase->ExecuteAsyncSQL(pszProcedure, NULL, NULL);
	return true;
}

ILogger*
sLog::GetLogger(const char* pszLoggerName, const LOG_APPENDER eAppenderType, const char* pszParam)
{
	return CLogSystem::Instance()->GetLogger(pszLoggerName, eAppenderType, pszParam);
}

//bool
//log::ReleaseLogger(const char* pszLoggerName)
//{
//	return  CLogSystem::Instance()->ReleaseLogger(pszLoggerName);
//}

bool
sLog::CreateLogDB(const char* pszSer, const char* pszUser, const char* pszPsd, const char* pszDBName)
{
	return CLogSystem::Instance()->CreateDbInstance(pszSer, pszUser, pszPsd, pszDBName);
}

void
sLog::ReleaseLogSystem()
{
	CLogSystem::Instance()->Release();
}

void
sLog::OutLog2DBDirect(const char* pszSQL)
{
	CLogSystem::Instance()->Log2DB(pszSQL);
}