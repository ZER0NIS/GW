#ifndef _ZS_ILOGGER_H
#define _ZS_ILOGGER_H

//#include "../base/inc/SvrBase.h"

namespace sLog
{
	enum LOG_LEVEL
	{//LOG级别
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_INFO,
		LOG_LEVEL_WARN,
		LOG_LEVEL_ERROR,
	};

	enum LOG_APPENDER
	{//输出器
		LOG_APPENDER_NONE	= 0,		//
		LOG_APPENDER_CONSOLE= 1,		//输出到控制台
		LOG_APPENDER_FILE	= 2,		//输出到文件
		LOG_APPENDER_DB		= 4,		//输出到DB
	};

	const int MAX_LOGBUFFER = 4096;

	class ILogger
	{//LOG记录器
	public:
		virtual const char* GetName	() = 0;
		virtual void	SetLogLevel	(LOG_LEVEL eLevel) = 0;	

		//输出LOG，根据输出器输出到不同的终端，如果要输出到数据库，则需先增加数据库输出器并制定调用存储过程的格式化语句
		//文件句柄一直打开，直到变化时才重新关闭打开，提高输出效率 快速输出
		//pszEvent 事件类型 方便查询。format事件信息
		virtual void	OutDebug	(const char* pszEvent, const char* format, ...) = 0;
		virtual void	OutInfo		(const char* pszEvent, const char* format, ...) = 0;
		virtual void	OutWarn		(const char* pszEvent, const char* format, ...) = 0;
		virtual void	OutError	(const char* pszEvent, const char* format, ...) = 0;

		//对应四种不同的LOG_LEVEL
		//format事件信息
		virtual void	OutDebug	(const char* format, ...) = 0; //LOG_LEVEL_DEBUG
		virtual void	OutInfo		(const char* format, ...) = 0; //LOG_LEVEL_INFO
		virtual void	OutWarn		(const char* format, ...) = 0; //LOG_LEVEL_WARN
		virtual void	OutError	(const char* format, ...) = 0; //LOG_LEVEL_ERROR

		//增加和删除输出器
		virtual bool	AddAppender	(const LOG_APPENDER eAppenderType, const char* pszParam)	=0;
		virtual void	RemoveAppender (const LOG_APPENDER eAppenderType) =0; 
		virtual ~ILogger()	{}

	};


	ILogger*	GetLogger		(const char* pszLoggerName, const LOG_APPENDER eAppenderType, const char* pszParam);
//	bool		ReleaseLogger	(const char* pszLoggerName);
	bool		CreateLogDB		(const char* pszSer, const char* pszUser, const char* pszPsd, const char* pszDBName);

	//输出到数据库 由用户构造好sql语句。
	void		OutLog2DBDirect (const char* pszSQL);

	void		ReleaseLogSystem();
}

#endif//_ZS_ILOGGER_H
