
#pragma once

#include "Socket.h"
#include <list>

namespace net
{

	class CListenSocket  
	{
	public:
		CListenSocket()  { m_sock = INVALID_SOCKET; }
		virtual ~CListenSocket() { this->Close(); }
		
		// interface
	public:
		bool		Listen		(int nPort,char* IP, int nSndBuf = 0, int nRcvBuf = 0);		// 0 : 不修改系统的缺省值。
		SOCKET		Accept		(void);
		void		Close		(void);
		SOCKET		Socket		(void)		{ return m_sock; }
		
		// data 
	protected:
		// 探测socket可写性，返回1表示可写，0表示socket还未就绪需要等待，-1表示socket错误
		int Detect		(SOCKET sock);
		
		SOCKET 	m_sock; // 侦听socket
		std::list<SOCKET>	m_setSock;		// 还未就绪的socket集
	};

}

