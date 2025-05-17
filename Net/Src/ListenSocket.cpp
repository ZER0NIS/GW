#include "..\..\Common\Include\Base/inc/SvrBase.h"
#include "../inc/ListenSocket.h"
#include <iostream>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace net
{
	//////////////////////////////////////////////////////////////////////
	bool
		CListenSocket::Listen(int nPort, char* IP, int nSndBuf/*= 0*/, int nRcvBuf/*= 0*/)
	{
		if (m_sock != INVALID_SOCKET)
 this->Close();

		// 创建主套接字(侦听socket)
		m_sock = ::socket(AF_INET, SOCK_STREAM, 0);
		if (m_sock == INVALID_SOCKET)
		{
 CSocket::DumpError("CListenSocket::Listen(), socket, failed.");
 return false;
		}

		// 设置SOCKET为KEEPALIVE
		int	nOptVal = 1;
		if (::setsockopt(m_sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&nOptVal, sizeof(nOptVal)))
		{
 CSocket::DumpError("CListenSocket::Listen(), setsockopt, set SO_KEEPALIVE failed.");
 ::closesocket(m_sock);
 return false;
		}

		// 设置SENDBUF
		nOptVal = nSndBuf;
		if (nSndBuf && ::setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char*)&nOptVal, sizeof(nOptVal)))
		{
 CSocket::DumpError("CListenSocket::Listen(), setsockopt, set SO_SNDBUF failed.");
 ::closesocket(m_sock);
 return false;
		}

		// 设置RECVBUF
		nOptVal = nRcvBuf;
		if (nRcvBuf && ::setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char*)&nOptVal, sizeof(nOptVal)))
		{
 CSocket::DumpError("CListenSocket::Listen(), setsockopt, set SO_RCVBUF failed.");
 ::closesocket(m_sock);
 return false;
		}

		// 设置SOCKET为可重复使用
		//if(::setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char *) &nOptVal, sizeof(nOptVal)))
		//{
		//	CSocket::DumpError("CListenSocket::Listen(), setsockopt(), set SO_REUSEADDR failed.");
		//	::closesocket(m_sock);
		//	return false;
		//}

		// 设置为非阻塞方式
		unsigned long	i = 1;
		if (::ioctlsocket(m_sock, FIONBIO, &i))
		{
 CSocket::DumpError("CListenSocket::Listen(), ioctlsocket, set nonBlocking socket failed.");
 ::closesocket(m_sock);
 return false;
		}

		// 绑定
		SOCKADDR_IN		addr_in;
		addr_in.sin_family = AF_INET;
		addr_in.sin_addr.S_un.S_addr = inet_addr(IP);
		addr_in.sin_port = ::htons(nPort);
		if (::bind(m_sock, (LPSOCKADDR)&addr_in, sizeof(addr_in)))
		{
 CSocket::DumpError("CListenSocket::Listen(), bind, failed.");
 ::closesocket(m_sock);
 return false;
		}

		// 监听端口
		if (::listen(m_sock, SOMAXCONN))
		{
 CSocket::DumpError("CListenSocket::Listen(), listen, failed.");
 ::closesocket(m_sock);
 return false;
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	int
		CListenSocket::Detect(SOCKET sock)
	{
		fd_set	writemask;
		FD_ZERO(&writemask);
		FD_SET(sock, &writemask);

		int rval = 0;
		struct timeval	timeout = { 0, 0 };
		if (SOCKET_ERROR == ::select(FD_SETSIZE, (fd_set*)0, &writemask, (fd_set*)0, &timeout))
		{
 switch (::WSAGetLastError())
 {
 case WSAEWOULDBLOCK:
 	rval = 0;
 	break;

 default:
 	rval = -1;
 	break;
 }
		}
		else
		{
 if (FD_ISSET(sock, &writemask))
 	rval = 1;
 else
 	rval = 0;
		}

		return rval;
	}

	//////////////////////////////////////////////////////////////////////
	SOCKET
		CListenSocket::Accept()
	{
		fd_set	readmask;
		FD_ZERO(&readmask);
		FD_SET(m_sock, &readmask);

		struct timeval	timeout = { 0, 0 };
		if (::select(FD_SETSIZE, &readmask, (fd_set*)0, (fd_set*)0, &timeout) > 0)
		{
 cout << "CListenSocket::Select !" << endl;
 if (FD_ISSET(m_sock, &readmask))
 {
 	struct sockaddr_in	addr;
 	int	   len = sizeof(addr);

 	//SOCKET ttt = WSAAccept(m_sock, (sockaddr *) &addr, (int *) &len,NULL,NULL);
 	SOCKET sockNew = accept(m_sock, (sockaddr*)&addr, (int*)&len);
 	if (INVALID_SOCKET == sockNew)
 	{
 		CSocket::DumpError("CListenSocket::Accept(), accept, failed.");
 	}
 	else
 	{// 探测socket的可写性，如果没有错误或者是阻塞，加入socket等待队列
 		switch (this->Detect(sockNew))
 		{
 		case 0:		// 阻塞，需要等待
  sbase::DebugMsg("found block socket:%u", sockNew);
  m_setSock.push_back(sockNew);
  break;

 		case -1:	// 错误，直接关闭
  sbase::DebugMsg("found error socket:%u", sockNew);
  ::closesocket(sockNew);
  break;

 		default:	// 正常
  return sockNew;
 		}
 	}
 }
		}

		// 扫描socket等待序列，返回可写的socket
		std::list<SOCKET>::iterator iter = m_setSock.begin();
		for (; iter != m_setSock.end(); iter++)
		{
 SOCKET sock = *iter;
 switch (this->Detect(sock))
 {
 case 0:		// 需要等待
 	break;

 case -1:	// 错误，直接关闭
 	CSocket::DumpError("CListenSocket::Accept(), select, failed.");
 	::closesocket(sock);
 	break;

 default:	// 正常
 {
 	sbase::DebugMsg("socket:%u ready now", sock);
 	m_setSock.erase(iter);
 	return sock;
 }
 break;
 }
		}

		return INVALID_SOCKET;
	}

	//////////////////////////////////////////////////////////////////////
	void
		CListenSocket::Close()
	{
		if (m_sock != INVALID_SOCKET)
		{
 ::closesocket(m_sock);
 m_sock = INVALID_SOCKET;
		}

		std::list<SOCKET>::iterator iter = m_setSock.begin();
		for (; iter != m_setSock.end(); iter++)
		{
 ::closesocket(*iter);
		}

		m_setSock.clear();
	}
}