//========================================================
//
//    文件名称 ： NetClient.h
//    摘    要 ： 客户端网络服务
//
//========================================================


#pragma once

#include "INetClient.h"
#include "ISocket.h"
#include "..\..\Lib_Base\Inc\Msg.h"
#include <vector>
#include <string>

class CNetClient : public INetClient, net::IClientSocketEvent
{
public:
	static CNetClient* CreateNew();
	~CNetClient();

protected:
	CNetClient();

public:
	//Interface of INetclient
	virtual ULONG			Release(void)								;
	virtual void			SetHost(const char* szIP, USHORT usPort)	;
	virtual bool			Connect(void)								;
	virtual void			DisConnect(void)							;
	virtual void			Process( void )								;
	virtual STATUS			GetStatus(void)	const						;
	virtual sbase::CMsg*    PickMsg(void)								;
	virtual bool			SendMsg(const void* pBuf, int nSize)		;
	virtual long			GetMsgNum(){return m_setMsg.size();}

	//Interface of net::IClientSocketEvent
	// 正常情况下，此函数应返回一个正整数指明从接受数据区清除掉的数据大小（一般就是处理的消息包大小）；
	// 如果返回-1，则会关闭此socket。
	virtual int				OnRcvMsg		(const void* buf, int nLen);

	// socket建立后会调用此方法。
	virtual void			OnEstablishSck	(void)	
	{
		m_eStatus = STATUS_CONNECTED;
	}

	// socket关闭时会调用此方法。
	virtual void			OnCloseSck		(void)
	{
		m_eStatus = STATUS_DISCONNECTED;
	}


protected:
	net::IClientSocket*		m_pNetSocket;
	ULONG					m_ulID;
	std::string				m_strServerIP;
	USHORT					m_usServerPort;
	STATUS					m_eStatus;
	std::vector<sbase::CMsg *>      m_setMsg;
};