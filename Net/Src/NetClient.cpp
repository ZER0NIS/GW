//========================================================
//
//    �ļ����� �� NetClient.cpp
//    ժ    Ҫ �� �ͻ����������
//
//========================================================

#include "..\inc\NetClient.h"
#include "..\..\Common\Include\Base\Inc\SvrBase.h"
#include "../inc/ISocket.h"
#include "../inc/TEncryptor.h"

#include <iostream>

//#pragma comment( lib, "user32.lib" )
//#pragma comment( lib, "winmm.lib" )
//
//#ifdef _DEBUG
//#pragma comment( lib, "../lib/BaseD.lib" )
//#else
//#pragma comment( lib, "../lib/SvrBase.lib" )
//
//#endif

using namespace std;

CNetClient* CNetClient::CreateNew()
{
	CNetClient* pClient = new CNetClient;
	return pClient;
}

CNetClient::~CNetClient()
{
	SAFE_RELEASE(m_pNetSocket);
}
CNetClient::CNetClient()
{
	m_pNetSocket = NULL;
	m_eStatus = STATUS_NONE;
}
ULONG CNetClient::Release(void)
{
	delete this;
	return 0;
}

void CNetClient::SetHost(const char* szIP, USHORT usPort)
{
	IF_OK(szIP && usPort)
	{
		m_strServerIP = szIP;
		m_usServerPort = usPort;
	}
}

bool CNetClient::Connect(void)
{
	//if (m_pNetSocket && m_eStatus == STATUS_DISCONNECTED)
	//{
	//	SAFE_RELEASE(m_pNetSocket);
	//}
	//else if (m_pNetSocket && m_eStatus == STATUS_CONNECTED)
	//{
	//      cout<<"�㲻���ظ���½..."<<endl;
	//   return false;
	//}

	//if (!m_pNetSocket)
	//{
	//	m_pNetSocket = net::ClientSocketCreate(*this, m_strServerIP.c_str(), m_usServerPort, 10000);

	//}

	//return m_pNetSocket;

	if (m_pNetSocket && m_eStatus == STATUS_DISCONNECTED)
	{
		SAFE_RELEASE(m_pNetSocket);
	}

	net::IEncryptor* pEncryptorSnd = new net::TEncryptClient<0x20, 0xFD, 0x07, 0x1F, 0x7A, 0xCF, 0xE5, 0x3F>;
	net::IEncryptor* pEncryptorRcv = new net::TEncryptClient<0x20, 0xFD, 0x07, 0x1F, 0x7A, 0xCF, 0xE5, 0x3F>;

	if (!m_pNetSocket)
	{
		m_pNetSocket = net::ClientSocketCreate(*this, m_strServerIP.c_str(), m_usServerPort, 1, pEncryptorSnd, pEncryptorRcv);
		if (m_pNetSocket != NULL)
		{
 m_eStatus = STATUS_CONNECTED;
		}
	}

	return !!m_pNetSocket;
}

void CNetClient::Process(void)
{
	if (m_pNetSocket)
	{
		m_pNetSocket->Process();
	}
}

sbase::CMsg*
CNetClient::PickMsg()
{
	if (m_pNetSocket)
	{
		m_pNetSocket->Process();
	}

	if (m_setMsg.size() > 0)
	{
		//cout<<"�ͻ�����Ϣ������"<<m_setMsg.size()<<endl;
		sbase::CMsg* pMsg = m_setMsg[0];
		m_setMsg.erase(m_setMsg.begin());

		return pMsg;
	}
	else
	{
		return NULL;
	}
}

void CNetClient::DisConnect(void)
{
	SAFE_RELEASE(m_pNetSocket);
	m_eStatus = STATUS_DISCONNECTED;
}

INetClient::STATUS		CNetClient::GetStatus(void)	const
{
	return m_eStatus;
}

bool CNetClient::SendMsg(const void* pBuf, int nSize)
{
	if (m_pNetSocket == NULL)
	{
		return false;
	}

	static  char Buf[2048];
	memcpy(Buf, pBuf, nSize);

	return  m_pNetSocket->SendMsg(Buf, ((sbase::MsgHead*)pBuf)->usSize);
}

int	 CNetClient::OnRcvMsg(const void* buf, int nLen)
{
	size_t BufNum = m_pNetSocket->GetBufByteNum();
	if (buf && BufNum >= 4)
	{
		sbase::MsgHead* pHead = (sbase::MsgHead*)buf;
		//���յ�һ��������
		if (pHead->usSize <= BufNum)
		{
 sbase::CMsg* pMsg = new sbase::CMsg();

 if (pMsg->Create(buf, pHead->usSize))
 {
 	//cout<<"��������Ϣ����..."<<endl;
 	m_setMsg.push_back(pMsg);
 	return pMsg->GetSize();
 }
 else
 {
 	SAFE_DELETE(pMsg);
 }
		}
		//�������
		else
		{
 return 0;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
//Extern

NETCLIENT_API INetClient* NetClientCreate(void)
{
	return CNetClient::CreateNew();
}