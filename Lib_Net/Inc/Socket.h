//========================================================
//
//    �ļ����ƣ�Socket.h
//    ժ   Ҫ�� Socket��װ��
//    
//    ˵    ����  1)Socket���ܻ�������Ϊ���λ�����
//                2)�޸�ճ������
//========================================================


#pragma once

/////////////////////////////////////////////

#ifndef _WINSOCK2API_           // û�а���winsock2.h    
#define _WINSOCK2API_            // �����ٰ���winsock2.h  
#endif     



/////////////////////////////////////////////

#include <string>
#include "../../Lib_Base/inc/Heap.h"
#include "../Inc/ISocket.h"
#include "CirBuffer.h"

#include <winsock2.h>  

namespace net
{
	
	// ���ջ������ߴ�
	const int	RCV_BUFFER_SIZE	=15*64*1024;
	
	class IEncryptor;
	class CSocket
	{
	public:
		CSocket(SOCKET sock, IEncryptor* pEncryptorSnd, IEncryptor* pEncryptorRcv);
		virtual ~CSocket();
		
	private:
		CSocket(): m_iBuffer( RCV_BUFFER_SIZE ) {}
		
	// interface
	public:
		bool			Send			(const void* buf, int nSize);
		const char*		Recv			(int& nLen, bool bDetectData = false);

		void			Close			(void);
		void			ClrPacket		(int nLen);
		int				SetBufSize		(int nSendBuf, int nRecvBuf);
		const char*		GetPeerIP		(void);
		
		bool			IsOpen			(void)	{ return (m_sock != INVALID_SOCKET); }
		bool			HaveData		(void)	{ return (m_nLen > 0); }
		SOCKET			Socket			(void)	{ return m_sock; }
        bool            ResolvePacket   (void) ;
        const char*     GetBufStart     (void) { return m_iBuffer.GetStart();}
        size_t          GetBufByteNum   (void) { return m_iBuffer.GetLength(); }
 		//int				DumpError		(const char* pszInfo);
		
	private:
		SOCKET			m_sock;	
		fd_set			m_fdset;
		
		int				m_nLen;
        CircularBuffer  m_iBuffer;
		//char			m_bufMsg[RCV_BUFFER_SIZE];
 
		std::string		m_strIP;


	// Encryptor
	public:
		IEncryptor*		QueryEncryptor	(ENCRYPTOR_TYPE nType);
		void			ChgEncryptor	(ENCRYPTOR_TYPE nType, IEncryptor* pEncryptor);
		
	private:
		IEncryptor*		m_pEncryptorSnd;
		IEncryptor*		m_pEncryptorRcv;
		
	// statics
	public:
		static bool		s_bInit;

		static bool		SocketInit		(void);
		static void		SocketDestroy	(void);
		static int		DumpError		(const char* pszInfo);
		
		
	// heap manage
	public:
		MYHEAP_DECLARATION(s_heap)
	};
	
}


