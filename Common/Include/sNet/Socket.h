#ifndef _FILE_SOCKET_H_
#define _FILE_SOCKET_H_

#include "stdafx.h"
#include "CirBuffer.h"
#include "FileTransfer.h"
#include <mutex>

namespace snet
{
	enum SOCKET_MODE
	{
		MODE_TEXT,
		MODE_FILE,
	};
	class PerIOData;
	class CIOCP;
	class IEncryptor;

	class CSocket
	{
	public:
		CSocket(CIOCP* pIOCP);
		~CSocket();

		void OnRead(PerIOData* pPerIOData);
		void OnWrite();

		int PostSend(PerIOData* Sendobj);
		int PostRecv(PerIOData* Recvobj);

		bool Write(int* line = NULL);
		bool PackMsg(char* pMsg, size_t iLen);

		bool Read(char** Paket, int nMaxLen = DEFAULT_BUFFER_SIZE);
		long NHJ_Read(char** Paket);
		void Remove(size_t nLen) { m_iBuffer.Remove(nLen); };

		bool IsValid() { return m_Sock != INVALID_SOCKET && !m_BClosing; };
		void Refresh();
		void Initnalize(SOCKET Sock, int Af);
		void Finalize();
		void Release() { delete this; };

		int DoSends();
		void InsertPendingSend(PerIOData* Sendobj);

		void* operator new	(size_t size);
		void   operator delete(void* p);

		size_t GetL() { return m_oBuffer.GetLength(); }

		snet::CIOCP* GetIOCP() { return m_pIOCP; }

		void SetPeerInfo(SOCKADDR* sa, int salen);
		const char* GetPeerIP();

		size_t GetiSpace() { return m_iBuffer.Space(); }
		size_t Get0Space() { return m_oBuffer.Space(); }

		void SetEncrypt(bool bFlag);
		char* Decrypt(char* pMsg, size_t iLen);
		char* Encrypt(char* pMsg, size_t iLen);
		bool ChangeMode(SOCKET_MODE eMode);
		SOCKET_MODE GetMode() { return m_eMode; }

	public:
		CIOCP* m_pIOCP;
		SOCKET             m_Sock;
		int                m_Af;
		volatile int       m_BClosing;
		volatile LONG      OutstandingRecv;
		volatile LONG      OutstandingSend;
		volatile LONG      PendingSend;

		std::mutex SockMutex;
		std::mutex StatusMutex;

		SOCKADDR_IN		   m_addrRemote;
		CSocket* next;
		ULONG           LastSendIssued, IoCountIssued, IOCompleted;
		CircularBuffer  m_iBuffer;
		CircularBuffer  m_oBuffer;
	private:
		volatile  bool   m_bEncrypt;
		PerIOData* OutOfOrderSends;
		IEncryptor* m_Encryptor;
		volatile SOCKET_MODE m_eMode;
		char			m_IP[NI_MAXHOST];
		char			m_PORT[NI_MAXSERV];
	};
}

#endif