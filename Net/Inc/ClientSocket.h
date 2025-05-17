#pragma once

#include <string>
#include "..\Inc\ISocket.h"
#include "..\..\Common\Include\Base\inc\Timer.h"

namespace net
{
	class CSocket;
	class CClientSocket : public IClientSocket
	{
	public:
		CClientSocket(IClientSocketEvent& event, IEncryptor* pEncryptorSnd, IEncryptor* pEncryptorRcv);
		virtual ~CClientSocket();
		//得到缓冲区中数据大小
		virtual size_t  GetBufByteNum(void);
		bool Init(const char* pszIP, int nPort, unsigned long dwReconnectInterval);

		// interface of IClientSocket
	public:
		unsigned long	Release(void) { delete this; return 0; }

		bool SendMsg(const void* buf, int nSize);
		void Process(void);
		void Close(void);

		IEncryptor* QueryEncryptor(ENCRYPTOR_TYPE eType);
		void ChgEncryptor(ENCRYPTOR_TYPE eType, IEncryptor* pEncryptor);

	protected:
		IClientSocketEvent& m_eventSocket;
		CSocket* m_pSocket;

		std::string 	m_strIP;
		int  m_nPort;

		unsigned long m_ulReconnectInterval;
		sbase::CTimerMS m_tmAutoConnect;

		IEncryptor* m_pEncryptorSnd;
		IEncryptor* m_pEncryptorRcv;
	};
}
