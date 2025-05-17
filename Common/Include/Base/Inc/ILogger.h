#ifndef _ZS_ILOGGER_H
#define _ZS_ILOGGER_H

//#include "../base/inc/SvrBase.h"

namespace sLog
{
	enum LOG_LEVEL
	{//LOG����
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_INFO,
		LOG_LEVEL_WARN,
		LOG_LEVEL_ERROR,
	};

	enum LOG_APPENDER
	{//�����
		LOG_APPENDER_NONE	= 0,		//
		LOG_APPENDER_CONSOLE= 1,		//���������̨
		LOG_APPENDER_FILE	= 2,		//������ļ�
		LOG_APPENDER_DB		= 4,		//�����DB
	};

	const int MAX_LOGBUFFER = 4096;

	class ILogger
	{//LOG��¼��
	public:
		virtual const char* GetName	() = 0;
		virtual void	SetLogLevel	(LOG_LEVEL eLevel) = 0;	

		//���LOG������������������ͬ���նˣ����Ҫ��������ݿ⣬�������������ݿ���������ƶ����ô洢���̵ĸ�ʽ�����
		//�ļ����һֱ�򿪣�ֱ���仯ʱ�����¹رմ򿪣�������Ч�� �������
		//pszEvent �¼����� �����ѯ��format�¼���Ϣ
		virtual void	OutDebug	(const char* pszEvent, const char* format, ...) = 0;
		virtual void	OutInfo		(const char* pszEvent, const char* format, ...) = 0;
		virtual void	OutWarn		(const char* pszEvent, const char* format, ...) = 0;
		virtual void	OutError	(const char* pszEvent, const char* format, ...) = 0;

		//��Ӧ���ֲ�ͬ��LOG_LEVEL
		//format�¼���Ϣ
		virtual void	OutDebug	(const char* format, ...) = 0; //LOG_LEVEL_DEBUG
		virtual void	OutInfo		(const char* format, ...) = 0; //LOG_LEVEL_INFO
		virtual void	OutWarn		(const char* format, ...) = 0; //LOG_LEVEL_WARN
		virtual void	OutError	(const char* format, ...) = 0; //LOG_LEVEL_ERROR

		//���Ӻ�ɾ�������
		virtual bool	AddAppender	(const LOG_APPENDER eAppenderType, const char* pszParam)	=0;
		virtual void	RemoveAppender (const LOG_APPENDER eAppenderType) =0; 
		virtual ~ILogger()	{}

	};


	ILogger*	GetLogger		(const char* pszLoggerName, const LOG_APPENDER eAppenderType, const char* pszParam);
//	bool		ReleaseLogger	(const char* pszLoggerName);
	bool		CreateLogDB		(const char* pszSer, const char* pszUser, const char* pszPsd, const char* pszDBName);

	//��������ݿ� ���û������sql��䡣
	void		OutLog2DBDirect (const char* pszSQL);

	void		ReleaseLogSystem();
}

#endif//_ZS_ILOGGER_H
