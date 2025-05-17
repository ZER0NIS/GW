#include "..\inc\ClientSocket.h"
#include "..\inc\Socket.h"
#include "..\..\Common\Include\Base\inc\SvrBase.h"
#include <iostream>

using namespace std;

namespace net
{
	CClientSocket::CClientSocket(IClientSocketEvent& event, IEncryptor* pEncryptorSnd, IEncryptor* pEncryptorRcv)
		: m_eventSocket(event), m_pSocket(NULL), m_pEncryptorSnd(NULL), m_pEncryptorRcv(NULL)
	{
		if (pEncryptorSnd)
			m_pEncryptorSnd = pEncryptorSnd;

		if (pEncryptorRcv)
			m_pEncryptorRcv = pEncryptorRcv;
	}

	CClientSocket::~CClientSocket()
	{
		SAFE_DELETE(m_pSocket);

		SAFE_RELEASE(m_pEncryptorSnd);
		SAFE_RELEASE(m_pEncryptorRcv);
	}

	//////////////////////////////////////////////////////////////////////
	bool
		CClientSocket::Init(const char* pszIP, int nPort, unsigned long dwReconnectInterval)
	{
		IF_NOT(pszIP)
			return false;

		// 初始化socket
		if (!CSocket::SocketInit())
			return false;

		// 创建套接字
		SOCKET	sock = ::socket(PF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET)
		{
			CSocket::DumpError("CClientSocket::Init(), socket");
			return false;
		}

		// 设置SOCKET为KEEPALIVE
		int		optval = 1;
		if (::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, sizeof(optval)))
		{
			CSocket::DumpError("CClientSocket::Init(), setsockopt, keep-alive");
			::closesocket(sock);
			return false;
		}

		// 设置SENDBUF
		const ULONG SNDBUF_SIZE = 30 * 1024;
		optval = SNDBUF_SIZE;
		if (::setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval)))
		{
			CSocket::DumpError("CClientSocket::Init(), setsockopt, snd buf");
			::closesocket(sock);
			return false;
		}

		// 设置RECVBUG
		if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(optval)))
		{
			CSocket::DumpError("CClientSocket::Init(), setsockopt, rcv buf");
			::closesocket(sock);
			return false;
		}

		// 设置为非阻塞方式
		unsigned long	a = 1;
		if (::ioctlsocket(sock, FIONBIO, &a))
		{
			CSocket::DumpError("CClientSocket::Init(), ioctlsocket");
			::closesocket(sock);
			return false;
		}

		// 域名->IP地址
		UINT	nAddr = ::inet_addr(pszIP);			// 必须为 UINT, 以便与in_addr兼容
		if (nAddr == INADDR_NONE)
		{
			hostent* hp;
			hp = ::gethostbyname(pszIP);
			if (hp == 0)
			{
				CSocket::DumpError("CClientSocket::Init(), gethostbyname");
				::closesocket(sock);
				return false;
			}

			char* pIP = inet_ntoa(*(struct in_addr*)hp->h_addr_list[0]);

			nAddr = ::inet_addr(pIP/*hp->h_addr_list[0]*/);	// 或 h_addr。自动取第一网卡的IP。
		}

		// ...
		SOCKADDR_IN	addr_in;
		::memset((void*)&addr_in, 0, sizeof(addr_in));
		addr_in.sin_family = AF_INET;
		addr_in.sin_addr.s_addr = nAddr;
		addr_in.sin_port = ::htons(nPort);
		if (SOCKET_ERROR == ::connect(sock, (PSOCKADDR)&addr_in, sizeof(addr_in)))
		{
			if (WSAEWOULDBLOCK == ::WSAGetLastError())	// On a nonblocking socket
			{
				const int BLOCKSECONDS = 3;

				timeval timeout;
				timeout.tv_sec = BLOCKSECONDS;
				timeout.tv_usec = 0;

				fd_set writeset, exceptset;
				FD_ZERO(&writeset);
				FD_ZERO(&exceptset);
				FD_SET(sock, &writeset);
				FD_SET(sock, &exceptset);

				int ret = ::select(FD_SETSIZE, NULL, &writeset, &exceptset, &timeout);
				if (ret == 0)
				{
					CSocket::DumpError("CClientSocket::Init(), select, connect timeout");
					::closesocket(sock);
					return false;
				}
				else if (ret < 0)
				{
					CSocket::DumpError("CClientSocket::Init(), select, connect failed");
					::closesocket(sock);
					return false;
				}
				else // ret > 0
				{
					if (FD_ISSET(sock, &exceptset))
					{
						DWORD dwError = ::WSAGetLastError();
						CSocket::DumpError("CClientSocket::Init(), select, connect exception");
						::closesocket(sock);
						return false;
					}
					else
					{
						if (!FD_ISSET(sock, &writeset))
						{
							CSocket::DumpError("CClientSocket::Init(), select, connect error");
							::closesocket(sock);
							return false;
						}
						else
						{// 连接成功
							m_pSocket = new CSocket(sock, m_pEncryptorSnd, m_pEncryptorRcv);
							IF_NOT(CSocket::IsValidPt(m_pSocket))
								return false;

							// 初始化成员
							m_strIP = pszIP;
							m_nPort = nPort;
							m_ulReconnectInterval = dwReconnectInterval;

							m_eventSocket.OnEstablishSck();

							return true;
						}
					}
				}
			}
		}

		CSocket::DumpError("CClientSocket::Init(), connect");
		::closesocket(sock);
		return false;
	}

	//////////////////////////////////////////////////////////////////////
	void
		CClientSocket::Close(void)
	{
		if (m_pSocket)
		{
			m_eventSocket.OnCloseSck();
			m_pSocket->Close();
		}
	}

	//////////////////////////////////////////////////////////////////////
	bool
		CClientSocket::SendMsg(const void* buf, int nSize)
	{
		// 		IF_NOT (m_pSocket)
		// 			return false;
		if (m_pSocket == NULL)
		{
			//#define OutputDebugStr  OutputDebugString VS2010 没有这个定义

			//::OutputDebugStr("客户端连接断开...\n");
			return false;
		}
		if (!m_pSocket->Send(buf, nSize))
		{
			m_eventSocket.OnCloseSck();

			// auto connect socket
			//if (m_tmAutoConnect.GetInterval() != 0)
			m_tmAutoConnect.Update();

			return false;
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	void
		CClientSocket::Process(void)
	{
		if (CSocket::IsValidPt(m_pSocket) && m_pSocket->IsOpen())
		{
			int nLen = 0;
			const char* buf = m_pSocket->Recv(nLen, true);
			if (buf)
			{
				if (nLen > 0)
				{
					//添加TCP粘包处理
					int nClearLen;
					bool HaveIntactPack = true;
					while (HaveIntactPack)
					{
						nClearLen = 0;
						nClearLen = m_eventSocket.OnRcvMsg(m_pSocket->GetBufStart(), nLen);
						if (nClearLen < 0)
						{
							this->Close();

							if (m_ulReconnectInterval != 0)	// auto connect socket
								m_tmAutoConnect.Startup(m_ulReconnectInterval);
						}
						else if (nClearLen > 0)
						{
							if (CSocket::IsValidPt(m_pSocket))
								m_pSocket->ClrPacket(nClearLen);
						}
						else
						{
							HaveIntactPack = false;
						}
					}
				}
			}
			else	// error
			{
				m_eventSocket.OnCloseSck();

				if (m_ulReconnectInterval != 0)	// auto connect socket
					m_tmAutoConnect.Startup(m_ulReconnectInterval);
			}
		}
		else	// do auto connect
		{
			if (m_tmAutoConnect.IsActive() && m_tmAutoConnect.ToNextTime())
			{
				SAFE_DELETE(m_pSocket);

				//if (this->Init(m_strIP.c_str(), m_nPort, m_ulReconnectInterval))
				{
					// stop connect timer
					m_tmAutoConnect.Clear();
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////
	IEncryptor*
		CClientSocket::QueryEncryptor(ENCRYPTOR_TYPE nType)
	{
		if (m_pSocket)
		{
			return m_pSocket->QueryEncryptor(nType);
		}
		else
		{
			if (ENCRYPTOR_SND == nType)
				return m_pEncryptorSnd;
			else if (ENCRYPTOR_RCV == nType)
				return m_pEncryptorRcv;
			else
				return NULL;
		}
	}

	size_t  CClientSocket::GetBufByteNum(void)
	{
		return m_pSocket->GetBufByteNum();
	};

	//////////////////////////////////////////////////////////////////////
	void
		CClientSocket::ChgEncryptor(ENCRYPTOR_TYPE nType, IEncryptor* pEncryptor)
	{
		if (NULL == pEncryptor)
			return;

		// change or copy
		switch (nType)
		{
		case ENCRYPTOR_SND:
			if (m_pEncryptorSnd)
			{
				SAFE_RELEASE(m_pEncryptorSnd);
				m_pEncryptorSnd = pEncryptor;
			}
			break;

		case ENCRYPTOR_RCV:
			if (m_pEncryptorRcv)
			{
				SAFE_RELEASE(m_pEncryptorRcv);
				m_pEncryptorRcv = pEncryptor;
			}
			break;

		default:
		{
			if (m_pEncryptorSnd)
			{
				SAFE_RELEASE(m_pEncryptorSnd);
				m_pEncryptorSnd = pEncryptor;				// keep original copy
			}

			if (m_pEncryptorRcv)
			{
				SAFE_RELEASE(m_pEncryptorRcv);
				m_pEncryptorRcv = pEncryptor->Duplicate();	// make a new copy
			}
		}
		break;
		}

		// change
		IF_OK(m_pSocket)
			m_pSocket->ChgEncryptor(nType, pEncryptor);
	}

	//////////////////////////////////////////////////////////////////////
	// extern
	//////////////////////////////////////////////////////////////////////
	IClientSocket*
		net::ClientSocketCreate(IClientSocketEvent& iSocketEvent, const char* pszIP, int nPort, unsigned long dwReconnectInterval/* = 0*/, IEncryptor* pEncryptorSnd/* = NULL*/, IEncryptor* pEncryptorRcv/* = NULL*/)
	{
		IF_NOT(pszIP)
			return NULL;

		CClientSocket* pSocket = new CClientSocket(iSocketEvent, pEncryptorSnd, pEncryptorRcv);
		IF_NOT(pSocket)
		{
			SAFE_RELEASE(pEncryptorSnd);
			SAFE_RELEASE(pEncryptorRcv);
			return NULL;
		}

		if (!pSocket->Init(pszIP, nPort, dwReconnectInterval))
		{
			SAFE_DELETE(pSocket);
			return NULL;
		}

		return pSocket;
	}
}