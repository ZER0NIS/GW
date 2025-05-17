//========================================================
//
//    文件名称 ： RouterPort.h
//    摘    要 ： 消息接受和发送管理
//
//========================================================

#pragma once

#include "..\..\Common\Include\Base\inc\Heap.h"
#include "MsgRouter.h"

namespace serve
{
	class CRouterPort : public IPort
	{
	public:
		CRouterPort(CMsgRouter& router, int nPort)
			: m_Router(router), m_usPort(nPort), m_bEnable(true) {
		}
		virtual ~CRouterPort();

	private:
		CMsgRouter& m_Router;
		USHORT					m_usPort;
		USHORT					m_usGroup;
		sbase::CCriticalSection	m_cs;

		//消息接受链表
		typedef std::list<sbase::IMessage*>	MSG_LST;
		MSG_LST					m_setMsg;

		//消息发送链表
		MSG_LST                 m_outMsg;

		bool					m_bEnable;

		// Interface of IPort
	public:
		virtual USHORT		Release(void);
		virtual USHORT		GetPort(void) const { return m_usPort; }
		virtual USHORT		GetGroup(void) const { return m_usGroup; }
		virtual bool		IsValid(void) const { return true; }
		//从接受链表中取出消息
		virtual sbase::IMessage* TakeMsg(void);
		//接受链表中追加数据
		virtual bool		PushMsg(sbase::IMessage* pMsg);
		//向接受链表中写入消息
		virtual bool		SendMsg(sbase::IMessage& msg, int nPortDes);

		virtual void		CleanMsg(void);
		virtual void		Stop(void) { m_bEnable = false; }
		virtual void		Start(void) { m_bEnable = true; }
		virtual void                DiscardMsg(sbase::IMessage* pMsg)
		{
			SAFE_RELEASE(pMsg);
		};

		//从发送链表中取出消息
		virtual sbase::IMessage* TakeMsgFromOut(void);
		//向发送链表中写入消息
		virtual bool                    SendMsgToOut(sbase::IMessage& msg, int nPortDes);
		//向发送链表中追加消息
		bool	                        PushOutMsg(sbase::IMessage* pMsg);

		// heap manage
	public:
		MYHEAP_DECLARATION(s_heap)
	};
}
