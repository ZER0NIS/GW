
#pragma warning(disable:4786)
//#define _WINSOCKAPI_	/* Prevent inclusion of winsock.h in windows.h*/
//#include <winsock2.h>

#include "..\inc\ServeSocket.h"
#include "..\inc\Socket.h"
#include "..\..\lib_Base\inc\SvrBase.h"
#include "..\..\lib_Base\inc\SyncObjs.h"
#include <tchar.h>
#include <iostream>
using namespace std;


namespace net
{


	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////
	CServeSocket::CServeSocket(IServeSocketEvent& eventSocket, IEncryptor* pEncryptorSnd, IEncryptor* pEncryptorRcv)
		: m_eventSocket(eventSocket), m_bRefuseConnect(false), m_pEncryptorSnd(NULL), m_pEncryptorRcv(NULL)
	{	
		if (pEncryptorSnd)
			m_pEncryptorSnd = pEncryptorSnd;

		if (pEncryptorRcv)
			m_pEncryptorRcv = pEncryptorRcv;

		m_setSocket.clear();
	}

	CServeSocket::~CServeSocket()
	{
		this->CloseAll();

		SAFE_RELEASE (m_pEncryptorSnd);
		SAFE_RELEASE (m_pEncryptorRcv);
	}

	//////////////////////////////////////////////////////////////////////
	bool 
		CServeSocket::Init(int nServePort,char* IP)
	{
		if(!CSocket::SocketInit())
			return false;

		if (!m_objListen.Listen(nServePort,IP, SOCKET_SENDBUFSIZE, SOCKET_RECVBUFSIZE))
			return false;

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	bool 
		CServeSocket::CloseSocket(SOCKET socket)
	{
		CSocket* pSocket = this->GetSocket(socket);
		if (!pSocket)
			return false;

		pSocket->Close();
		return true;
	}

	//////////////////////////////////////////////////////////////////////
	void 
		CServeSocket::CloseAll(void)
	{
		SOCKET_MAP::iterator itor;
		for (itor = m_setSocket.begin(); itor != m_setSocket.end(); itor++)
		{
			CSocket* pSocket = (*itor).second;
			if (!pSocket)
				continue;

			pSocket->Close();
			SAFE_DELETE (pSocket);
		}

		m_setSocket.clear();
	}

	//////////////////////////////////////////////////////////////////////
	CSocket* 
		CServeSocket::GetSocket(SOCKET socket)
	{
		SOCKET_MAP::iterator itor = m_setSocket.find(socket);
//        cout<<"Socket："<<socket<<endl;
//		cout<<"连接数量："<<m_setSocket.size()<<endl;
		if (itor == m_setSocket.end())
			return NULL;

		return (*itor).second;
	}

	//////////////////////////////////////////////////////////////////////
	const char*	
		CServeSocket::GetSocketIP(SOCKET socket)
	{
		CSocket* pSocket = this->GetSocket(socket);
		if (!pSocket)
			return NULL;

		return pSocket->GetPeerIP();
	}

	//////////////////////////////////////////////////////////////////////
	bool 
		CServeSocket::SendMsg (SOCKET socket, const void* pBuf, int nSize)
	{
		 sbase::CSingleLock lock(&m_cs);

		IF_NOT (pBuf && nSize > 0)
		{
           cout<<"IF_NOT (pBuf && nSize > 0)."<<endl;
           return false;
		}
		///////////////////////////////////////////	
		if ( 0 == socket)
		{
           SOCKET_MAP::iterator itor = m_setSocket.begin();
           for ( ; itor!=m_setSocket.end(); itor++ )
           {
               CSocket* ptempSocket = (*itor).second;
			   IF_OK(ptempSocket)
                 ptempSocket->Send(pBuf, nSize);
           }
		   int a= m_setSocket.size();
		   char buffer[128] = "";
		   sprintf_s( buffer, "完成发送%d条消息！", a );
		   ::OutputDebugStr(buffer);
           return true;
		}
		/////////////////////////////////////////
       
		CSocket* pSocket = this->GetSocket(socket);
		if (!pSocket)
		{
           cout<<"this->GetSocket(socket)"<<endl;
           return false;
		}
			
        
		return pSocket->Send(pBuf, nSize);
	}

	//////////////////////////////////////////////////////////////////////
	void 
		CServeSocket::Process(void)
	{
		// process listen socket
		if (!m_bRefuseConnect && m_setSocket.size() < MAX_SERVICE)
		{
			SOCKET sockNew = m_objListen.Accept();
			if (INVALID_SOCKET != sockNew)
			{
				if (m_eventSocket.OnAcceptSck(sockNew))
				{
					CSocket* pSocket = new CSocket(sockNew, m_pEncryptorSnd, m_pEncryptorRcv);
					IF_NOT (pSocket)
					{
						::closesocket(sockNew);
						cout<<"new CSocket failed!"<<endl;
					}
					else
					{
						m_setSocket[sockNew] = pSocket;
						///////////////////////////////
						const char * IP= GetSocketIP(sockNew);
						///////////////////////////////
						cout<<"Accept connect ,IP:"<<IP<<",Connection count:"<<m_setSocket.size()<<";"<<endl;
					 //  ::OutputDebugStr(_T("接受连接请求..."));
						m_eventSocket.OnEstablishSck(sockNew);	
						
					}
				}
				else
					::closesocket(sockNew);
			}
		}

		// add socket info to fd_set
		fd_set	readmask;
		FD_ZERO(&readmask);

		SOCKET_MAP::iterator itor;
		for (itor = m_setSocket.begin(); itor != m_setSocket.end(); itor++)
		{
			CSocket* pSocket = (*itor).second;
			if (pSocket && pSocket->IsOpen())
				FD_SET(pSocket->Socket(), &readmask);
		}

		// get status of all socket
		struct timeval	timeout = { 0, 0 };
		int ret = ::select(MAX_SERVICE, &readmask, (fd_set *) 0, (fd_set *) 0, &timeout);

		// process sockets now
		SOCKET_MAP::reverse_iterator ritor;
		for (ritor = m_setSocket.rbegin(); ritor != m_setSocket.rend(); ritor++)
		{
			CSocket* pSocket = (*ritor).second;
			IF_NOT (pSocket)
			{
                CleanUp(ritor->first);
				// size of map can't be zero here
				if (m_setSocket.size() == 0)
					break;

				continue;
			}

			// socket got data
			if (pSocket->IsOpen() 
				&& (pSocket->HaveData() || FD_ISSET(pSocket->Socket(), &readmask)))
			{
				// process data
				int	nBufLen = 0;
				const char* pBuf = pSocket->Recv(nBufLen);
				if (pBuf && nBufLen > 0)
				{
					int nLen = m_eventSocket.OnRcvMsg(pSocket->Socket(), pBuf, nBufLen);
					if (nLen < 0)
						pSocket->Close();
					else
						pSocket->ClrPacket(nLen);
				}
				else if (!pBuf)	//socket break;
				{
					m_eventSocket.OnCloseSck(pSocket->Socket());
					pSocket->Close();
				}
			}

			// socket still open?
			if (!pSocket->IsOpen())
			{
				m_eventSocket.OnCloseSck(ritor->first);
                 
                CleanUp(ritor->first); 
				SAFE_DELETE (pSocket);

				// size of map can't be zero here
				if (m_setSocket.size() == 0)
					break;
			}
		}
	}


	//////////////////////////////////////////////////////////////////////
	IEncryptor* 
		CServeSocket::QueryEncryptor(SOCKET socket, ENCRYPTOR_TYPE eType)
	{
		CSocket* pSocket = this->GetSocket(socket);
		if (!pSocket)
			return NULL;

		return pSocket->QueryEncryptor(eType);
	}


	//////////////////////////////////////////////////////////////////////
	void 
		CServeSocket::ChgEncryptor(SOCKET socket, ENCRYPTOR_TYPE eType, IEncryptor* pEncryptor)
	{
		IF_NOT (pEncryptor)
			return;

		CSocket* pSocket = this->GetSocket(socket);
		if (!pSocket)
			return;

		pSocket->ChgEncryptor(eType, pEncryptor);
	}


//---------------------------------------------------------------
	 IServeSocket* 
		net::ServeSocketCreate(IServeSocketEvent& iSocketEvent, int nServePort,char* IP, IEncryptor* pEncryptorSnd/* = NULL*/, IEncryptor* pEncryptorRcv/* = NULL*/)
	{
		CServeSocket* pSocket = new CServeSocket(iSocketEvent, pEncryptorSnd, pEncryptorRcv);
		IF_NOT (pSocket)
		{
			SAFE_RELEASE (pEncryptorSnd);
			SAFE_RELEASE (pEncryptorRcv);
			return NULL;
		}

		IF_NOT (pSocket->Init(nServePort,IP))
		{
			SAFE_DELETE (pSocket);
			return NULL;
		}	

		return pSocket;
	}
}
