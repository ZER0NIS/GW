
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
		bool		Listen		(int nPort,char* IP, int nSndBuf = 0, int nRcvBuf = 0);		// 0 : ���޸�ϵͳ��ȱʡֵ��
		SOCKET		Accept		(void);
		void		Close		(void);
		SOCKET		Socket		(void)		{ return m_sock; }
		
		// data 
	protected:
		// ̽��socket��д�ԣ�����1��ʾ��д��0��ʾsocket��δ������Ҫ�ȴ���-1��ʾsocket����
		int Detect		(SOCKET sock);
		
		SOCKET 	m_sock; // ����socket
		std::list<SOCKET>	m_setSock;		// ��δ������socket��
	};

}

