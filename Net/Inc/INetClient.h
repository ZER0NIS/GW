#pragma once

#include <windows.h>

#ifdef NETCLIENT_EXPORTS
#	define NETCLIENT_API extern "C" __declspec(dllexport)
#else
#	define NETCLIENT_API extern "C" __declspec(dllimport)
#endif

namespace sbase
{
	class CMsg;
}

class INetClient
{
public:
	enum STATUS
	{
		STATUS_NONE,
		STATUS_CONNECTED,
		STATUS_DISCONNECTED,
	};

	virtual ULONG			Release(void)								= 0;
	virtual void			SetHost(const char* szIP, USHORT usPort)	= 0;
	virtual bool			Connect(void)								= 0;
	virtual void			DisConnect(void)							= 0;
	virtual void			Process( void )								= 0;
	virtual STATUS			GetStatus(void)	const						= 0;
	virtual sbase::CMsg*    PickMsg(void)								= 0;
 	virtual bool			SendMsg(const void* pBuf, int nSize)		= 0;
	virtual long			GetMsgNum()									= 0;
};


NETCLIENT_API INetClient* NetClientCreate(void);