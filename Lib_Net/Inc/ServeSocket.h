
#pragma once


#pragma warning(disable:4786)
#include <map>

#include "../Inc/ISocket.h"
#include "ListenSocket.h"
#include "..\..\Lib_Base\Inc\SyncObjs.h"


namespace net
{

	const ULONG SOCKET_SENDBUFSIZE	= 10*1024;
	const ULONG SOCKET_RECVBUFSIZE	= 10*1024;
//	const ULONG MAX_SERVICE			= 2048;			    // 最大连接数量
	const ULONG MAX_SERVICE			= 20000;			// 最大连接数量
	
	class CSocket;
	class IEncryptor;
	class CServeSocket : public IServeSocket
	{
	// Constructor
	public:
		CServeSocket(IServeSocketEvent& iSocketEvent, IEncryptor* pEncryptorSnd, IEncryptor* pEncryptorRcv);
		virtual ~CServeSocket();
		
		bool					Init			(int nServePort,char* IP);
		
	// interface of IServeSocket
	public:
		virtual unsigned long	Release			(void)			{ delete this; return 0; }
		
		virtual bool			SendMsg			(SOCKET socket, const void* pBuf, int nSize);
		virtual void			Process			(void);

		virtual bool			CloseSocket		(SOCKET socket);
		void                    CleanUp         (SOCKET socket) { sbase::CSingleLock lock(&m_cs);  m_setSocket.erase(socket); };
		virtual void			RefuseConnect	(bool bEnable)	{ m_bRefuseConnect = bEnable; }
		
		virtual const char*		GetSocketIP		(SOCKET socket);
		virtual int				GetSocketAmount	(void)			{ return m_setSocket.size(); }
		
		virtual IEncryptor*		QueryEncryptor	(SOCKET socket, ENCRYPTOR_TYPE eType);
		virtual void			ChgEncryptor	(SOCKET socket, ENCRYPTOR_TYPE eType, IEncryptor* pEncryptor);
		
	// Implementation
	protected:  
		CSocket*				GetSocket		(SOCKET socket);
		void					CloseAll		(void);
		
	protected:
		IServeSocketEvent&		m_eventSocket;
		CListenSocket			m_objListen;
		bool					m_bRefuseConnect;
		
		typedef	std::map<SOCKET, CSocket*>	SOCKET_MAP;
		SOCKET_MAP				m_setSocket;

		IEncryptor*				m_pEncryptorSnd;
		IEncryptor*				m_pEncryptorRcv;

	private:
		sbase::CCriticalSection	m_cs;
	};

	
}	// end of namspace net

