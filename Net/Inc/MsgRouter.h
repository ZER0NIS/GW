#pragma once

#pragma warning(disable:4786)
#include <list>
#include <vector>
#include <set>
#include <string>
#include "..\..\Common\Include\Base/inc/SyncObjs.h"
#include "..\Inc\IServe.h"

namespace serve
{
	const int MAX_PORTMSG = 256 * 1024;

	class CRouterPort;

	class CMsgRouter : public IRouter
	{
		// static
	public:
		static CMsgRouter* CreateNew();

		// Constructor & Destructor
	protected:
		CMsgRouter(int nMaxPort);
		virtual ~CMsgRouter();
		bool 	Init(/*IAccessContrl* pAccessCtrl*/);

	public:
		unsigned long		Release(void);
		virtual IPort* QueryPort(ULONG nPort)	const;

		// Interface of IMessageAccess
	public:

		sbase::IMessage* TakeMsg(ULONG nPort);
		bool 	DispatchMsg(ULONG nPortDst, /*int nPortDstGroup,*/ ULONG nPortSrc, sbase::IMessage& msg);

		//20070108
		bool                 DispatchOutMsg(ULONG nPortDst, /*int nPortDstGroup,*/ ULONG nPortSrc, sbase::IMessage& msg);
		bool                 PushOutMsg(ULONG nPort, sbase::IMessage& msg);
		// Implementation
	protected:
		bool 	IsValidPort(ULONG nPort)	const;
		bool 	PushMsg(ULONG nPort, sbase::IMessage& msg);

		//Ïò¸ÂËÍ

	private:

		// port set
		typedef std::vector<CRouterPort*>		PORT_VEC;
		PORT_VEC  		m_setPort;
	};
}
