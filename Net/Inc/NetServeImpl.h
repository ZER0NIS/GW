#pragma once

#pragma warning(disable:4786)
#include <map>
#include "..\..\Common\Include\Base/inc/imessage.h"
#include "ISocket.h"
#include "IServe.h"
#include "KernelMsgDef.h"
#include "..\..\Common\Include\Base/inc/Timer.h"

namespace ns
{
	static const char* SERVE_NAME = "NET_SERVE";
	enum TYPE { RECEIVE, SEND };

	class CNetworkServeImpl : public serve::IServe, private net::IServeSocketEvent
	{
	public:
		static  net::IServeSocket* pCommonSocketServer;

	public:
		CNetworkServeImpl(serve::IPort* pPort, USHORT usServeInterval, TYPE netType = RECEIVE);
		virtual ~CNetworkServeImpl();

		bool Init(int nServePort, char* cIP);
	private:
		TYPE            m_Type;
		serve::IPort* m_pPort;
		USHORT m_usServeInterval;

		typedef std::map<SOCKET, serve::PORT>	PORT_MAP;
		PORT_MAP		m_setPort;

		// Interface of IModule
	public:
		virtual STATUS		QueryStatus(void)	const { return m_status; }
		virtual ULONG		Release(void) { delete this; return 0; }
		virtual const char* GetName(void)		const { return SERVE_NAME; }
		virtual bool		PreCreate(void) { return true; }
		virtual bool		Create(void) { return true; }
		virtual bool		Run(void) { return true; }
		virtual void		PostDestroy(void) { return; }
		virtual void		Destroy(void) { return; }

		// Interface of IServe
	public:
		virtual USHORT	GetInterval(void) const { return m_usServeInterval; }
		virtual USHORT	GetPort(void) const { return m_pPort->GetPort(); }
		virtual void	SendMsg(sbase::IMessage& msg, int nPortDes/*, int nPortDesGroup = serve::PORTGROUP_ALL*/);
		virtual int		OnProcess(void);
		virtual void    CloseSock(SOCKET socket) { OnCloseSck(socket); return; };

		// Interface of IServeSocketEvent
	protected:
		virtual bool	OnAcceptSck(SOCKET socket);
		virtual void	OnEstablishSck(SOCKET socket);
		virtual void	OnCloseSck(SOCKET socket);
		virtual int		OnRcvMsg(SOCKET socket, const char* buf, int nLen);
	private:
		net::IServeSocket* m_pServeSocket;

		// Implementation
	public:
		void UpdateStatInfo(void);
	private:
		MSG_SCK_INFO		m_infoSck;
		sbase::CTimer		m_tm;

		STATUS 	m_status;
	};

	serve::IServe* ServeCreate(serve::IRouter* pRouter, USHORT usInterval, TYPE netType = RECEIVE);
}