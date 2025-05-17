#ifndef _ZS_LOGSYSTEM_H
#define _ZS_LOGSYSTEM_H

#include "../../DB/Inc/IDB.h"
#include "Singleton.h"
#include "Logger.h"
#include <vector>

namespace sLog
{
	class CLogSystem
	{
	public:
		CLogSystem();
		static CLogSystem* Instance();
		static ULONG		Release();

		ILogger* GetLogger(const char* pszLoggerName, const LOG_APPENDER eAppenderType, const char* pszParam);
		bool		ReleaseLogger(const char* pszLoggerName);

		ILogger* FindLogger(const char* pszLoggerName);

		bool		CreateDbInstance(const char* pszSer, const char* pszUser, const char* pszPsd, const char* pszDBname);

		bool		Log2DB(const char* pszProcedure, const char* pszEventType, int nLogLevel, const char* pszContent);
		bool		Log2DB(const char* pszProcedure);
	private:
		~CLogSystem();

		typedef std::vector<ILogger*> VEC_LOGGER;
		VEC_LOGGER	m_vecLogger;

		rade_db::IDatabase* m_pDataBase;

		static CLogSystem* s_Instance;
	};
}

#endif//_ZS_LOGSYSTEM_H
