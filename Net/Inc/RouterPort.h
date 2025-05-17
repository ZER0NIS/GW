//========================================================
//
//    �ļ����� �� RouterPort.h
//    ժ    Ҫ �� ��Ϣ���ܺͷ��͹���
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

		//��Ϣ��������
		typedef std::list<sbase::IMessage*>	MSG_LST;
		MSG_LST					m_setMsg;

		//��Ϣ��������
		MSG_LST                 m_outMsg;

		bool					m_bEnable;

		// Interface of IPort
	public:
		virtual USHORT		Release(void);
		virtual USHORT		GetPort(void) const { return m_usPort; }
		virtual USHORT		GetGroup(void) const { return m_usGroup; }
		virtual bool		IsValid(void) const { return true; }
		//�ӽ���������ȡ����Ϣ
		virtual sbase::IMessage* TakeMsg(void);
		//����������׷������
		virtual bool		PushMsg(sbase::IMessage* pMsg);
		//�����������д����Ϣ
		virtual bool		SendMsg(sbase::IMessage& msg, int nPortDes);

		virtual void		CleanMsg(void);
		virtual void		Stop(void) { m_bEnable = false; }
		virtual void		Start(void) { m_bEnable = true; }
		virtual void                DiscardMsg(sbase::IMessage* pMsg)
		{
			SAFE_RELEASE(pMsg);
		};

		//�ӷ���������ȡ����Ϣ
		virtual sbase::IMessage* TakeMsgFromOut(void);
		//����������д����Ϣ
		virtual bool                    SendMsgToOut(sbase::IMessage& msg, int nPortDes);
		//����������׷����Ϣ
		bool	                        PushOutMsg(sbase::IMessage* pMsg);

		// heap manage
	public:
		MYHEAP_DECLARATION(s_heap)
	};
}
