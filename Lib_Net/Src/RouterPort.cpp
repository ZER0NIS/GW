//========================================================
//
//    文件名称 ： RouterPort.cpp
//    摘    要 ： 消息接受和发送管理
//
//========================================================


#include "..\inc\RouterPort.h"

#include "..\..\Lib_Base\inc\SvrBase.h"
#include "..\..\Lib_Base\inc\SyncObjs.h"
#include  <iostream>
using namespace std;

namespace serve
{

	MYHEAP_IMPLEMENTATION(CRouterPort, s_heap)
	
	//---------------------------------------------------------
	USHORT
		CRouterPort::Release(void)
	{ 
		delete this;
		return 0; 
	}

    //----------------------------------------------------------
	CRouterPort::~CRouterPort()
	{
		CleanMsg();
	}

    //----------------------------------------------------------
	sbase::IMessage*	
		CRouterPort::TakeMsg(void)
	{ 
		sbase::CSingleLock lock(&m_cs);
		if (m_setMsg.size() == 0)
		{
			return NULL;
		}
		//cout<<"接受队列消息:"<<m_setMsg.size()<<"条"<<endl;
		//char hint[128];
	     sbase::IMessage* pMsg= m_setMsg.front();
		 m_setMsg.pop_front();
		//sprintf_s(hint,"[取出]消息数量:%d;\n",m_setMsg.size());
		//::OutputDebugString(_T(hint));
		return pMsg;
	}

    //----------------------------------------------------------
	//
	//----------------------------------------------------------
	bool
		CRouterPort::SendMsg(sbase::IMessage& msg, int nPortDes/*, int nPortDesGroup*/)  
	{ 
		//  sbase::DebugLogMsg("RouterPort send msg[%d] from Port[%d] to Port[%d]!", msg.GetType(), this->GetPort(), nPortDes);
		return m_Router.DispatchMsg(nPortDes, m_usPort, msg);
	}


    //----------------------------------------------------------
	bool		
		CRouterPort::PushMsg(sbase::IMessage* pMsg) 
	{
		sbase::CSingleLock lock(&m_cs);
		if (m_bEnable)
		{
			m_setMsg.push_back(pMsg);
			char hint[128];
			sprintf_s(hint,"[压入]消息数量:%d;\n",m_setMsg.size());
			::OutputDebugString(_T(hint));

			return true;
		}
		else
		{
			return false;
		}
	}

   //-----------------------------------------------------
	bool		
		CRouterPort::PushOutMsg(sbase::IMessage* pMsg) 
	{
		sbase::CSingleLock lock(&m_cs);
		if (m_bEnable)
		{
			m_outMsg.push_back(pMsg);
			return true;
		}
		else
		{
			return false;
		}
	}

	//---------------------------------------------------------------------------
	void		
		CRouterPort::CleanMsg	(void)
	{
		//清理消息接受链表
		sbase::CSingleLock lock(&m_cs);
		MSG_LST::iterator it = m_setMsg.begin();
		for (; it != m_setMsg.end(); it++)
		{
			sbase::IMessage* pMsg = *it;
			SAFE_RELEASE(pMsg);
		}
		m_setMsg.clear();

		//清理消息发送链表
		MSG_LST::iterator itor = m_outMsg.begin();
		for (; itor != m_outMsg.end(); itor++)
		{
			sbase::IMessage* pMsgout = *itor;
			SAFE_RELEASE(pMsgout);
		}
		m_outMsg.clear();
	}

	//------------------------------------------------------------------------------
	sbase::IMessage*
		CRouterPort::TakeMsgFromOut(void)
	{
		sbase::CSingleLock lock(&m_cs);
		if (m_outMsg.size() == 0)
		{
			return NULL;
		}

		sbase::IMessage* pMsg = m_outMsg.front();
		m_outMsg.pop_front();
		return pMsg;
	}

    //-------------------------------------------------------------------------------
    bool 
		CRouterPort::SendMsgToOut(sbase::IMessage& msg, int nPortDes)
	{
        return m_Router.DispatchOutMsg(nPortDes, m_usPort, msg);
	}

}
