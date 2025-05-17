#ifndef _ZS_LOGGER_H
#define _ZS_LOGGER_H

#include "ILogger.h"
#include "SyncObjs.h"
#include <windows.h>
#include <string>

namespace sLog
{

	class CLogger : public ILogger
	{
	public:
		CLogger(const char* pszLoggerName, LOG_APPENDER eAppenderType, const char* pszParam);

		virtual const char*	GetName	()	{ return m_strLoggerName.c_str(); }
		virtual void	SetLogLevel(LOG_LEVEL eLevel)	{ m_eLogLevel = eLevel; }


		virtual void	OutDebug	(const char* pszEvent, const char* format, ...);
		virtual void	OutInfo		(const char* pszEvent, const char* format, ...);
		virtual void	OutWarn		(const char* pszEvent, const char* format, ...);
		virtual void	OutError	(const char* pszEvent, const char* format, ...);

		virtual void	OutDebug	(const char* format, ...);
		virtual void	OutInfo		(const char* format, ...);
		virtual void	OutWarn		(const char* format, ...);
		virtual void	OutError	(const char* format, ...);

		virtual bool	AddAppender(const LOG_APPENDER eAppenderType, const char* pszParam);
		virtual void	RemoveAppender (const LOG_APPENDER eAppenderType){ m_nLogAppender &= ~eAppenderType; }
	private:	
		virtual ~CLogger()	;

		void	OutLog(const char* pszEvent, const LOG_LEVEL eLevel, char* pszMsg);


		bool	IsOut2Console	()	{ return (m_nLogAppender & LOG_APPENDER_CONSOLE)!=0;  }
		bool	IsOut2File		()	{ return (m_nLogAppender & LOG_APPENDER_FILE)!=0; }
		bool	IsOut2DB		()	{ return (m_nLogAppender & LOG_APPENDER_DB)!=0; }

		void	Close			();
		bool	Open			();

		int		GetFileCreateDay	()	{ return m_nMDayFileCreate; }

		int	m_nLogAppender;
		std::string m_strProcedureFormat;
		std::string m_strLoggerName;

		LOG_LEVEL	m_eLogLevel;

		int m_fd;
		std::string m_strFileKey;
		std::string m_strCurFileName;

		int m_nMDayFileCreate; //记录器创建的时间 --以天计  


		sbase::CCriticalSection m_Lock;
	};

}


#endif//_ZS_LOGGER_H
