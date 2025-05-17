#include "stdafx.h"
#include "Socket.h"
#include "PerIOData.h"
#include "IOCP.h"
#include "../Base/Inc/Msg.h"
#include "./Encryptor.h"

namespace snet
{
	CSocket::CSocket(CIOCP* pIOCP) :m_iBuffer(pIOCP->m_RecvBufferSize), m_oBuffer(pIOCP->m_SendBufferSize), m_pIOCP(pIOCP),
		m_bEncrypt(false)
	{
#define ENCRYPT_KEY1				0xa61fce5e
#define ENCRYPT_KEY2				0x443ffc04
		typedef	TEncryptServer <ENCRYPT_KEY1, ENCRYPT_KEY2>	CEncryptor;
		m_Encryptor = new CEncryptor;
	}

	CSocket::~CSocket()
	{
		SAFE_RELEASE(m_Encryptor);
		m_Sock = INVALID_SOCKET;
	}

	void* CSocket::operator new(size_t size)
	{
		return (void*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	}

	void   CSocket::operator delete(void* p)
	{
		if (!p)
			return;

		::HeapFree(GetProcessHeap(), 0, p);
	}

	void CSocket::OnRead(PerIOData* pPerIOData)
	{
		Decrypt((char*)pPerIOData->m_Buf, pPerIOData->m_BufLen);

		m_iBuffer.Write(pPerIOData->m_Buf, pPerIOData->m_BufLen);
	}

	void CSocket::OnWrite()
	{
		std::lock_guard<std::mutex> lock(SockMutex);

		IOCompleted++;
	}

	void CSocket::Initnalize(SOCKET Sock, int Af)
	{
#ifdef TRANSMIT_FILE
		m_FileTransfer = NULL;
#endif
		m_eMode = MODE_TEXT;
		m_Sock = Sock;
		m_Af = Af;
		memset(m_IP, 0l, NI_MAXHOST);
		memset(m_PORT, 0l, NI_MAXSERV);
		next = NULL;
		OutOfOrderSends = NULL;
		OutstandingRecv = OutstandingSend = 0;
		m_BClosing = 0;
		LastSendIssued = 0;
		IoCountIssued = 0;
		IOCompleted = 0;
		m_bEncrypt = false;
		m_Encryptor->Refresh();
	}

	void CSocket::Finalize()
	{
		std::lock_guard<std::mutex> lock(SockMutex);

		PerIOData* ptr = NULL, * prev = NULL;
		ptr = OutOfOrderSends;
		while (ptr)
		{
			prev = ptr;
			ptr = ptr->m_Next;
			m_pIOCP->FreePerIOData(prev);
		}
		OutOfOrderSends = NULL;
	};

	int CSocket::DoSends()
	{
		PerIOData* sendobj = NULL, * prev = NULL;
		int         ret;

		ret = NO_ERROR;

		std::lock_guard<std::mutex> lock(SockMutex);

		sendobj = OutOfOrderSends;

		while ((sendobj))
		{
			if (sendobj->IoOrder == LastSendIssued)
			{
				if (prev == NULL)
				{
					OutOfOrderSends = NULL;
				}
				else
				{
					prev->m_Next = sendobj->m_Next;
				}

				if (PostSend(sendobj) != NO_ERROR)
				{
					m_pIOCP->FreePerIOData(sendobj);
					ret = SOCKET_ERROR;
				}
				break;
			}
			prev = sendobj;
			sendobj = sendobj->m_Next;
		}

		return ret;
	}

	void CSocket::InsertPendingSend(PerIOData* Sendobj)
	{
		PerIOData* ptr = NULL;
		std::lock_guard<std::mutex> lock(SockMutex);

		Sendobj->m_Next = NULL;
		ptr = OutOfOrderSends;
		OutOfOrderSends = Sendobj;
		Sendobj->m_Next = ptr;
		IoCountIssued++;
	}

	int CSocket::PostSend(PerIOData* Sendobj)
	{
		WSABUF  wbuf;
		DWORD   bytes;
		int     rc, err;

		ZeroMemory(&Sendobj->m_OL, sizeof(WSAOVERLAPPED));

		Sendobj->m_iOperation = OP_WRITE;

		wbuf.buf = Sendobj->m_Buf;
		wbuf.len = Sendobj->m_BufLen;

		std::lock_guard<std::mutex> lock(SockMutex);

		LastSendIssued++;

		rc = WSASend(
			m_Sock,
			&wbuf,
			1,
			&bytes,
			0,
			&Sendobj->m_OL,
			NULL
		);
		if (rc == SOCKET_ERROR)
		{
			rc = NO_ERROR;
			if ((err = WSAGetLastError()) != WSA_IO_PENDING)
			{
				if (err == WSAENOBUFS)
#ifdef _DEBUG

					DebugBreak();
#else
					ASSERT(WSAENOBUFS != WSA_IO_PENDING);
#endif
				printf("SendBuf failed in server socket with errorCode:%d\n", err);
				rc = SOCKET_ERROR;
			}
		}
		if (rc == NO_ERROR)
		{
			InterlockedIncrement(&OutstandingSend);
			InterlockedIncrement(&m_pIOCP->m_OutstandingSends);
		}

		return rc;
	}

	int CSocket::PostRecv(PerIOData* Recvobj)
	{
		WSABUF  wbuf;
		DWORD   bytes, flags;
		int     rc;

		ZeroMemory(&Recvobj->m_OL, sizeof(WSAOVERLAPPED));

		Recvobj->m_iOperation = OP_READ;

		wbuf.buf = Recvobj->m_Buf;
		wbuf.len = Recvobj->m_BufLen;

		flags = 0;

		std::lock_guard<std::mutex> lock(SockMutex);

		rc = WSARecv(
			m_Sock,
			&wbuf,
			1,
			&bytes,
			&flags,
			&Recvobj->m_OL,
			NULL
		);

		if (rc == SOCKET_ERROR)
		{
			rc = NO_ERROR;
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				rc = SOCKET_ERROR;
			}
		}
		if (rc != SOCKET_ERROR)
		{
			InterlockedIncrement(&OutstandingRecv);
		}

		return rc;
	}

	bool CSocket::Write(int* line)
	{
		bool result = true;

		if (line)
			*line = __LINE__;

		std::lock_guard<std::mutex> lock(SockMutex);

		int OLen = 0;

		if (OutstandingSend > 30)
		{
			return false;
		}

		if (line)
			*line = __LINE__;

		while ((OLen = (int)m_oBuffer.GetLength()) > 0)
		{
			PerIOData* pPerIOData = m_pIOCP->GetPerIOData(DEFAULT_BUFFER_SIZE);
			if (NULL == pPerIOData)
			{
				return false;
			}

			if (line)
				*line = __LINE__;

			int len = (OLen - DEFAULT_BUFFER_SIZE >= 0) ? DEFAULT_BUFFER_SIZE : OLen;
			pPerIOData->m_iOperation = OP_WRITE;
			m_oBuffer.Read(pPerIOData->m_Buf, len);
			pPerIOData->m_BufLen = len;
			pPerIOData->m_Socket = this;
			pPerIOData->IoOrder = IoCountIssued;

			if (line)
				*line = __LINE__;

			InsertPendingSend(pPerIOData);
			if (line)
				*line = __LINE__;
		}

		if (line)
			*line = __LINE__;

		if (IoCountIssued > LastSendIssued && LastSendIssued == IOCompleted)
			result = (DoSends() != NO_ERROR) ? false : true;

		if (line)
			*line = __LINE__;

		return result;
	}

	bool CSocket::PackMsg(char* pMsg, size_t iLen)
	{
		std::lock_guard<std::mutex> lock(SockMutex);

		char* pChar = Encrypt(pMsg, iLen);

		if (m_oBuffer.Space() < iLen)
			Write();

		bool  res = m_oBuffer.Write(pChar, iLen);

		return res;
	}

	void CSocket::Refresh()
	{
		std::lock_guard<std::mutex> lock(SockMutex);

		if (m_Sock != INVALID_SOCKET)
		{
			closesocket(m_Sock);
		}

		m_Sock = INVALID_SOCKET;
		m_Af = 0;
		m_BClosing = 1;
		OutstandingRecv = 0;
		OutstandingSend = 0;
		PendingSend = 0;
		next = NULL;
	}

	long CSocket::NHJ_Read(char** Paket)
	{
		size_t nLen = m_iBuffer.GetLength();
		if (nLen > 0 && m_iBuffer.GetStart())
		{
			*Paket = m_iBuffer.GetStart();
			return (long)nLen;
		}
		else
		{
			*Paket = NULL;
			return 0;
		}
	}

	bool CSocket::Read(char** Paket, int nMaxLen)
	{
		size_t nLen = m_iBuffer.GetLength();
		char* pChar = m_iBuffer.GetStart();
		if ((nLen > sizeof(sbase::MsgHead)) && pChar)
		{
			sbase::MsgHead* pHead = (sbase::MsgHead*)pChar;
			if (nLen >= pHead->usSize && pHead->usSize <= nMaxLen)
			{
				return m_iBuffer.Read((char*)Paket, pHead->usSize);
			}
			else
			{
				*Paket = NULL;
				return false;
			}
		}
		else
		{
			*Paket = NULL;
			return false;
		}
	}

	const char* CSocket::GetPeerIP()
	{
		return m_IP;
	}

	void CSocket::SetPeerInfo(SOCKADDR* sa, int salen)
	{
		int rc = getnameinfo(
			sa,
			salen,
			m_IP,
			NI_MAXHOST,
			m_PORT,
			NI_MAXSERV,
			NI_NUMERICHOST | NI_NUMERICSERV
		);
		if (rc != 0)
		{
			fprintf(stderr, "%s: getnameinfo failed: %d\n", __FILE__, rc);
		}
	}

	void CSocket::SetEncrypt(bool bFlag)
	{
		std::lock_guard<std::mutex> lock(StatusMutex);

		m_bEncrypt = bFlag;

		if (m_iBuffer.GetLength() > 0)
			Decrypt(m_iBuffer.GetStart(), m_iBuffer.GetLength());
	}

	char* CSocket::Decrypt(char* pMsg, size_t iLen)
	{
		std::lock_guard<std::mutex> lock(StatusMutex);

		if (m_bEncrypt)
		{
			m_Encryptor->Decrypt((unsigned char*)pMsg, (int)iLen);

			return pMsg;
		}

		return pMsg;
	}

	char* CSocket::Encrypt(char* pMsg, size_t iLen)
	{
		static char pBuf[10000] = { 0 };

		std::lock_guard<std::mutex> lock(StatusMutex);
		if (m_bEncrypt)
		{
			memcpy(pBuf, pMsg, iLen);
			m_Encryptor->Encrypt((unsigned char*)pBuf, (int)iLen);
			return pBuf;
		}

		return pMsg;
	}
}