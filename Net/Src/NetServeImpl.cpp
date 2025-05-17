#include <tchar.h>
#include <iostream>
#include "../Inc/NetServeImpl.h"
#include "../Inc/TEncryptor.h"
#include "..\..\Common\Include\Base/inc/SvrBase.h"
#include "..\..\Common\Include\Base/inc/IniFile.h"
#include "../Inc/IServe.h"
//#include "../Include/INetServe.h"

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "winmm.lib" )

using namespace std;

namespace ns
{
	net::IServeSocket* CNetworkServeImpl::pCommonSocketServer = NULL;
	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////
	CNetworkServeImpl::CNetworkServeImpl(serve::IPort* pPort, USHORT usServeInterval, TYPE netType)
		: m_pPort(pPort), m_usServeInterval(usServeInterval), m_pServeSocket(NULL)
	{
		m_setPort.clear();
		m_Type = netType;
		::memset(&m_infoSck, 0L, sizeof(MSG_SCK_INFO));
		m_tm.Startup(1);
	}

	CNetworkServeImpl::~CNetworkServeImpl()
	{
		if (pCommonSocketServer != NULL)
		{
 SAFE_RELEASE(m_pServeSocket);
 m_pPort->CleanMsg();
 m_pPort->Stop();
 m_setPort.clear();
 pCommonSocketServer = NULL;
		}
	}

	//////////////////////////////////////////////////////////////////////
	// this function will release encrytor if failed.
	//////////////////////////////////////////////////////////////////////
	bool CNetworkServeImpl::Init(int nServePort, char* cIP)
	{
		// 2012.12 wu
		// HL AccountServer
//#define LOGIN_KEY1		0xa61fce5e	// A = 0x20, B = 0xFD, C = 0x07, first = 0x1F, key = a61fce5e
//#define LOGIN_KEY2		0x443ffc04	// A = 0x7A, B = 0xCF, C = 0xE5, first = 0x3F, key = 443ffc04

		// CQ AccountServer
#define LOGIN_KEY1		0xb29d3c84	// A = 0xFA, B = 0x0F, C = 0x13, first = 0x9D, key = b29d3c84
#define LOGIN_KEY2		0xcc624ada	// A = 0xA4, B = 0x79, C = 0x6D, first = 0x62, key = cc624ada

		typedef	net::TEncryptServer <LOGIN_KEY1, LOGIN_KEY2>	CEncryptor;

		// create serve socket
		if (m_Type == RECEIVE)
		{
 m_pServeSocket = net::ServeSocketCreate(*this, nServePort, cIP, new CEncryptor, new CEncryptor);
 if (NULL == pCommonSocketServer)
 	pCommonSocketServer = m_pServeSocket;
		}
		else if (m_Type == SEND)
		{
 if (NULL != pCommonSocketServer)
 	m_pServeSocket = pCommonSocketServer;
		}

		IF_NOT(m_pServeSocket)
 return false;

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// Interface of IServeSocketEvent
	//////////////////////////////////////////////////////////////////////
	bool
		CNetworkServeImpl::OnAcceptSck(SOCKET socket)
	{
		// statistic socket info
		m_infoSck.dwSckPerAccept++;
		return true;
	}

	//////////////////////////////////////////////////////////////////////
	void
		CNetworkServeImpl::OnEstablishSck(SOCKET socket)
	{
		m_setPort[socket] = serve::PORT_IN;

		// inform GameServe setup virtual connect
		MSG_CONNECT_SETUP info = { socket };
		//sbase::IMessage msg(sizeof(MSG_HEADER) + sizeof(info), _KERNEL_MSG_CONNECT_SETUP, (char*)&info, serve::PORT_IN, socket);
		sbase::IMessage msg(sizeof(MSG_HEADER) + sizeof(info), _KERNEL_MSG_CONNECT_SETUP, (char*)&info, serve::PORT_IN, reinterpret_cast<void*>(socket));

		this->SendMsg(msg, serve::PORT_IN);

		// statistic socket info
		m_infoSck.dwSckTotal++;
		m_infoSck.dwSckOnline = m_setPort.size();
	}

	//////////////////////////////////////////////////////////////////////
	void
		CNetworkServeImpl::OnCloseSck(SOCKET socket)
	{
		PORT_MAP::iterator iter = m_setPort.find(socket);
		if (iter == m_setPort.end())
 return;

		// inform GameServe closed virtual connect
		MSG_CONNECT_CLOSE info = { socket };
		//	sbase::IMessage msg(sizeof(MSG_HEADER) + sizeof(info), _KERNEL_MSG_CONNECT_CLOSE, (char*)&info, -1, socket);
		sbase::IMessage msg(sizeof(MSG_HEADER) + sizeof(info), _KERNEL_MSG_CONNECT_CLOSE, (char*)&info, -1, reinterpret_cast<void*>(socket));

		m_pPort->SendMsg(msg, serve::PORT_IN);

		ASSERT(m_pServeSocket);
		m_pServeSocket->CloseSocket(socket);

		// remove from port set
		m_setPort.erase(iter);

		// statistic socket info
		m_infoSck.dwSckPerClose++;
		m_infoSck.dwSckOnline = m_setPort.size();
	}

	//////////////////////////////////////////////////////////////////////
	int
		CNetworkServeImpl::OnRcvMsg(SOCKET socket, const char* buf, int nLen)
	{
		IF_NOT(socket != INVALID_SOCKET)
 return 0;

		IF_NOT(buf || nLen >= sizeof(WORD))
 return 0;

		IF_NOT(m_pPort->IsValid())
 return 0;

		// integral chk
		int nSize = *((WORD*)buf);
		if (nLen < nSize)
 return 0;

		// statistic socket info
		m_infoSck.dwBytesRcv += nLen;
		m_infoSck.dwTotalBytesRcv += nLen;

		/*
		static int nMsgCount = 0;
		this->MsgDump(++nMsgCount, (void*)buf, nSize);
		*/

		// search des port
		PORT_MAP::iterator iter = m_setPort.find(socket);
		if (iter == m_setPort.end())
 return 0;

		// send msg to des port
		//sbase::IMessage msg(buf, serve::PORT_NETWORK, socket);
		//sbase::IMessage msg(buf, -1, socket);
		sbase::IMessage msg(buf, -1, reinterpret_cast<void*>(socket));

		this->SendMsg(msg, (*iter).second);

		return msg.GetSize();
	}

	//////////////////////////////////////////////////////////////////////
	// Interface of IServe
	//////////////////////////////////////////////////////////////////////
	void
		CNetworkServeImpl::SendMsg(sbase::IMessage& msg, int nPortDes/*, int nPortDesGroup = PORTGROUP_ALL*/)
	{
		//		if (serve::PORT_INVALID != nPortDes)
		m_pPort->SendMsg(msg, nPortDes/*, nPortDesGroup*/);
	}

	//////////////////////////////////////////////////////////////////////
	int
		CNetworkServeImpl::OnProcess(void)
	{
		IF_NOT(m_pServeSocket)
		{
 return 0;
		}
		//::OutputDebugStr(_T("OnProcess\n"));
		///cout<<"OnProcess"<<endl;

		if (RECEIVE == m_Type)
		{
 // serve socket process
 //::OutputDebugStr(_T("接受线程!\n"));
 m_pServeSocket->Process();
		}
		else if (SEND == m_Type)
		{
 //::OutputDebugStr(_T("发送线程!\n"));
 // take msg from port
 if (!m_pPort->IsValid())
 	return 0;

 // process msg
 sbase::IMessage* pPortMsg = NULL;
 //从发送链表中取出消息

 while ((pPortMsg = m_pPort->TakeMsg()) != NULL)
 {
 	// 	cout<<"收到数据...."<<endl;
 	switch (pPortMsg->usMsgType)
 	{
 	case _KERNEL_MSG_SCK_CLOSE:
 	{
 		MSG_SCK_CLOSE* pMsg = (MSG_SCK_CLOSE*)(pPortMsg->bufMsg + sizeof(MSG_HEADER));
 		m_pServeSocket->CloseSocket(pMsg->socket);
 		m_setPort.erase(pMsg->socket);
 	}
 	break;

 	default:
 	{
 		IF_OK(m_pServeSocket)
 		{
  //if (m_pServeSocket->SendMsg(pPortMsg->GetTo(), pPortMsg->GetBuf(), pPortMsg->GetSize()))
  if (m_pServeSocket->SendMsg(reinterpret_cast<SOCKET>(pPortMsg->GetTo()), pPortMsg->GetBuf(), pPortMsg->GetSize()))

  {
  	//  cout<<"已经发送.."<<pPortMsg->GetSize()<<endl;
  }

  // statistic socket info
  m_infoSck.dwBytesSnd += pPortMsg->GetSize();
  m_infoSck.dwTotalBytesSnd += pPortMsg->GetSize();
 		}
 	}
 	break;
 	}

 	//SAFE_RELEASE (pPortMsg);
 }
		}

		// update socket info
		if (m_tm.ToNextTime())
		{
 m_infoSck.dwSckPerAccept = 0;
 m_infoSck.dwSckPerClose = 0;
		}

		//---------导致内存使用量不断增大-----------
		//this->UpdateStatInfo();

		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	void
		CNetworkServeImpl::UpdateStatInfo(void)
	{
		sbase::IMessage portMsg(sizeof(MSG_HEADER) + sizeof(m_infoSck), _KERNEL_MSG_SHELL_SCK, &m_infoSck, -1, reinterpret_cast<void*>(-1));

		this->SendMsg(portMsg, serve::PORT_SHELL);
	}

	//------------------------------------------------------------------------------------
	serve::IServe* ServeCreate(serve::IRouter* pRouter, USHORT usInterval, TYPE netType)
	{
		serve::IPort* pPort = NULL;
		if (RECEIVE == netType)
		{
 pPort = pRouter->QueryPort(serve::PORT_IN);
 IF_NOT(pPort)
 	return NULL;
		}
		else if (SEND == netType)
		{
 pPort = pRouter->QueryPort(serve::PORT_OUT);
 IF_NOT(pPort)
 	return NULL;
		}

		CNetworkServeImpl* pServe = new CNetworkServeImpl(pPort, usInterval, netType);
		IF_NOT(pServe)
 return NULL;

		if (RECEIVE == netType)
		{
 sbase::CIniFile ini("./config.ini", "System");
 const int nListenPort = ini.GetInt("ListenPort");
 char IP[32] = "";
 ini.GetString("IP", IP, sizeof(IP));

 IF_NOT(pServe->Init(nListenPort, IP))
 {
 	SAFE_DELETE(pServe);
 	return NULL;
 }
		}
		else if (SEND == netType)
		{
 sbase::CIniFile ini("./config.ini", "System");
 const int nListenPort = 6500;
 IF_NOT(pServe->Init(nListenPort, "127.0.0.1"))
 {
 	SAFE_DELETE(pServe);
 	return NULL;
 }
		}

		return pServe;
	}
}