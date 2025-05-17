//========================================================
//
//    �ļ����� �� NetClient.h
//    ժ    Ҫ �� �ͻ����������
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
	// ��������£��˺���Ӧ����һ��������ָ���ӽ�������������������ݴ�С��һ����Ǵ������Ϣ����С����
	// �������-1�����رմ�socket��
	virtual int				OnRcvMsg		(const void* buf, int nLen);

	// socket���������ô˷�����
	virtual void			OnEstablishSck	(void)	
	{
		m_eStatus = STATUS_CONNECTED;
	}

	// socket�ر�ʱ����ô˷�����
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