//========================================================
//
//    文件名称：Socket.h
//    摘   要： Socket封装类
//    
//    说    明：  1)Socket接受缓冲区改为环形缓冲区
//                2)修改粘包处理
//========================================================


#pragma once

/////////////////////////////////////////////

#ifndef _WINSOCK2API_           // 没有包含winsock2.h    
#define _WINSOCK2API_            // 避免再包含winsock2.h  
#endif     



/////////////////////////////////////////////

#include <string>
#include "../../Lib_Base/inc/Heap.h"
#include "../Inc/ISocket.h"
#include "CirBuffer.h"

#include <winsock2.h>  

namespace net
{
	
	// 接收缓冲区尺寸
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


