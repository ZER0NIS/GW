#pragma warning(disable:4786)
#include "..\inc\MsgRouter.h"
#include "..\inc\RouterPort.h"
#include "..\..\Common\Include\Base\inc\SvrBase.h"

namespace serve
{
	//////////////////////////////////////////////////////////////////////
	CMsgRouter*
		CMsgRouter::CreateNew()
	{
		//这里只需要创建一个网络Router，包含发送和接受两个链表
		CMsgRouter* pRouter = new CMsgRouter(PORT_ALL);
		IF_NOT(pRouter)
			return NULL;

		IF_NOT(pRouter->Init(/*pAccessCtrl*/))
		{
			SAFE_DELETE(pRouter);
			return NULL;
		}

		return pRouter;
	}

	//////////////////////////////////////////////////////////////////////
	CMsgRouter::CMsgRouter(int nMaxPort)
	{
		for (int i = 0; i < nMaxPort; i++)
		{
			CMsgRouter& router = *this;
			CRouterPort* pPort = new CRouterPort(router, i);
			m_setPort.push_back(pPort);
		}
	}

	//////////////////////////////////////////////////////////////////////
	CMsgRouter::~CMsgRouter()
	{
		PORT_VEC::iterator it = m_setPort.begin();
		for (; it != m_setPort.end(); it++)
		{
			if (*it)
			{
				(*it)->CleanMsg();
			}
			SAFE_DELETE(*it);
		}

		m_setPort.clear();
	}

	//////////////////////////////////////////////////////////////////////
	unsigned long
		CMsgRouter::Release(void)
	{
		delete this;
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	bool
		CMsgRouter::IsValidPort(ULONG nPort) const
	{
		if (nPort >= 0 && nPort <= m_setPort.size())
			return true;

		return false;
	}

	//////////////////////////////////////////////////////////////////////
	bool
		CMsgRouter::PushMsg(ULONG nPort, sbase::IMessage& msg)
	{
		// param check
		IF_NOT(this->IsValidPort(nPort))
			return false;

		// duplicate msg
		sbase::IMessage* pNewMsg = new sbase::IMessage(msg/*msg.GetBuf(), msg.GetFrom(), msg.GetTo()*/);
		IF_NOT(pNewMsg)
			return false;

		return m_setPort[nPort] ? m_setPort[nPort]->PushMsg(pNewMsg) : false;
	}

	//--------------------------------------------------------------------------------
	bool
		CMsgRouter::PushOutMsg(ULONG nPort, sbase::IMessage& msg)
	{
		// param check
		IF_NOT(this->IsValidPort(nPort))
			return false;

		// duplicate msg
		sbase::IMessage* pNewMsg = new sbase::IMessage(msg/*msg.GetBuf(), msg.GetFrom(), msg.GetTo()*/);
		IF_NOT(pNewMsg)
			return false;

		return m_setPort[nPort] ? m_setPort[nPort]->PushOutMsg(pNewMsg) : false;
	}

	//////////////////////////////////////////////////////////////////////
	sbase::IMessage*
		CMsgRouter::TakeMsg(ULONG nPort)
	{
		// param check
		IF_NOT(this->IsValidPort(nPort))
			return NULL;

		return m_setPort[nPort] ? m_setPort[nPort]->TakeMsg() : NULL;
	}

	//------------------------------------------------------------------------
	//接受队列
	//------------------------------------------------------------------------
	bool
		CMsgRouter::DispatchMsg(ULONG nPortDst, /*int nPortDstGroup,*/ ULONG nPortSrc, sbase::IMessage& msg)
	{
		// param check
		if (PORT_ALL != nPortDst)
		{
			IF_NOT(this->IsValidPort(nPortDst))
				return false;
		}

		IF_NOT(this->IsValidPort(nPortSrc))
			return false;

		return this->PushMsg(nPortDst, msg);
	}

	//------------------------------------------------------------------------
	//发送对列
	//------------------------------------------------------------------------
	bool
		CMsgRouter::DispatchOutMsg(ULONG nPortDst, ULONG nPortSrc, sbase::IMessage& msg)
	{
		// param check
		if (PORT_ALL != nPortDst)
		{
			IF_NOT(this->IsValidPort(nPortDst))
				return false;
		}

		IF_NOT(this->IsValidPort(nPortSrc))
			return false;

		return this->PushOutMsg(nPortDst, msg);
	}

	//////////////////////////////////////////////////////////////////////
	IPort*
		CMsgRouter::QueryPort(ULONG nPort) const
	{
		IF_NOT(this->IsValidPort(nPort))
			return NULL;

		return m_setPort[nPort];
	}

	//////////////////////////////////////////////////////////////////////
	bool
		CMsgRouter::Init(/*IAccessContrl* pAccessCtrl*/)
	{
		return true;
	}
}