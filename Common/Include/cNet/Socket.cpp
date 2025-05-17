#include "stdafx.h"
#include "./Socket.h"
#include "IOCP.h"
#include "PerIOData.h"
#include "./Encryptor.h"
#include "../Base/Inc/Msg.h"
#include "FileTransfer.h"

namespace cnet
{
	CSocket::CSocket(CIOCP* pIOCP) :m_iBuffer(pIOCP->m_RecvBufferSize), m_oBuffer(pIOCP->m_SendBufferSize), m_pIOCP(pIOCP),
		m_bEncrypt(false)
	{
		typedef	TEncryptClient <0xEE, 0x17, 0x33, 0x50, 0x82, 0x23, 0x61, 0x33>	CEncryptor;	// 2012.12 wu changed
		m_Encryptor = new CEncryptor;
	}

	CSocket::~CSocket()
	{
		SAFE_RELEASE(m_Encryptor);
		::DeleteCriticalSection(&StatusCritSec);
		::DeleteCriticalSection(&SockCritSec);
	}

	void* CSocket::operator new (size_t size)
	{
		return (void*)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	}

	void CSocket::operator delete(void* p)
	{
		::HeapFree(GetProcessHeap(), 0, p);
	}

	int CSocket::PostSend(PerIOData* Sendobj)
	{
		WSABUF  wbuf;
		DWORD   bytes;
		int     rc, err;

		ZeroMemory(&Sendobj->ol, sizeof(WSAOVERLAPPED));

		Sendobj->operation = OP_WRITE;

		wbuf.buf = Sendobj->buf;
		wbuf.len = Sendobj->buflen;

		EnterCriticalSection(&SockCritSec);

		LastSendIssued++;

		rc = WSASend(
			s,
			&wbuf,
			1,
			&bytes,
			0,
			&Sendobj->ol,
			NULL
		);

		if (rc == SOCKET_ERROR)
		{
			rc = NO_ERROR;
			if ((err = WSAGetLastError()) != WSA_IO_PENDING)
			{
#ifdef _DEBUG
				if (err == WSAENOBUFS)
					DebugBreak();
#endif
				printf("ErrorCode:%d\n", err);
				rc = SOCKET_ERROR;
			}
		}
		if (rc == NO_ERROR)
		{
			InterlockedIncrement(&OutstandingSend);
			InterlockedIncrement(&m_pIOCP->m_OutstandingSends);
		}
		LeaveCriticalSection(&SockCritSec);

		return rc;
	}

	int CSocket::PostRecv(PerIOData* Recvobj)
	{
		WSABUF  wbuf;
		DWORD   bytes, flags;
		int     rc;

		ZeroMemory(&Recvobj->ol, sizeof(WSAOVERLAPPED));
		Recvobj->operation = OP_READ;

		wbuf.buf = Recvobj->buf;
		wbuf.len = Recvobj->buflen;

		flags = 0;

		EnterCriticalSection(&SockCritSec);

		rc = WSARecv(
			s,
			&wbuf,
			1,
			&bytes,
			&flags,
			&Recvobj->ol,
			NULL
		);

		if (rc == SOCKET_ERROR)
		{
			rc = NO_ERROR;
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				//dbgprint("PostRecv: WSARecv* failed: %d\n", WSAGetLastError());
				rc = SOCKET_ERROR;
			}
		}
		if (rc == NO_ERROR)
		{
			InterlockedIncrement(&OutstandingRecv);
		}

		LeaveCriticalSection(&SockCritSec);

		return rc;
	}

	void CSocket::OnRead(PerIOData* pPerIOData)
	{
		//Decrypt((char*)pPerIOData->buf, pPerIOData->buflen);

		m_iBuffer.Write(pPerIOData->buf, pPerIOData->buflen);
	}

	void CSocket::OnWrite()
	{
		EnterCriticalSection(&SockCritSec);
		IOCompleted++;
		LeaveCriticalSection(&SockCritSec);
	}

	bool CSocket::Write(void)
	{
		bool result = true;
		EnterCriticalSection(&SockCritSec);
		int OLen = 0;
		while ((OLen = (int)m_oBuffer.GetLength()) != 0)
		{
			PerIOData* pPerIOData = m_pIOCP->GetBufferObj(DEFAULT_BUFFER_SIZE);
			if (NULL == pPerIOData)
			{
				LeaveCriticalSection(&SockCritSec);
				return false;
			}

			int len = (OLen - DEFAULT_BUFFER_SIZE >= 0) ? DEFAULT_BUFFER_SIZE : OLen;
			pPerIOData->operation = OP_WRITE;
			m_oBuffer.Read(pPerIOData->buf, len);
			//memcpy( pPerIOData->buf,m_oBuffer.GetStart(),len);
			pPerIOData->buflen = len;
			pPerIOData->pSocket = this;
			pPerIOData->IoOrder = IoCountIssued;

			InsertPendingSend(pPerIOData);
			//m_oBuffer.Remove(len);

			if (IoCountIssued > LastSendIssued && LastSendIssued == IOCompleted)
				result = (DoSends() != NO_ERROR) ? false : true;
		}
		LeaveCriticalSection(&SockCritSec);

		return result;
	}

	void CSocket::InsertPendingSend(PerIOData* Sendobj)
	{
		PerIOData* ptr = NULL;
		EnterCriticalSection(&SockCritSec);
		Sendobj->next = NULL;
		ptr = OutOfOrderSends;
		OutOfOrderSends = Sendobj;
		Sendobj->next = ptr;
		IoCountIssued++;
		LeaveCriticalSection(&SockCritSec);
	}

	int CSocket::DoSends()
	{
		PerIOData* sendobj = NULL, * prev = NULL;
		int         ret;

		ret = NO_ERROR;

		EnterCriticalSection(&SockCritSec);
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
					prev->next = sendobj->next;
				}

				if (PostSend(sendobj) != NO_ERROR)
				{
					m_pIOCP->FreeBufferObj(sendobj);
					ret = SOCKET_ERROR;
				}
				break;
			}
			prev = sendobj;
			sendobj = sendobj->next;
		}

		LeaveCriticalSection(&SockCritSec);

		return ret;
	}

	bool CSocket::PackMsg(char* pMsg, size_t iLen)
	{
		//char* pChar = Encrypt(pMsg, iLen);

		if (m_oBuffer.Space() < iLen)
			Write();

		return m_oBuffer.Write(pMsg, iLen);
	}

	void CSocket::Refresh()
	{
		if (s != INVALID_SOCKET)
		{
			closesocket(s);
		}

		s = INVALID_SOCKET;
		m_Af = 0;
		m_BClosing = 1;
		OutstandingRecv = 0;
		OutstandingSend = 0;
		PendingSend = 0;
		//next            = NULL;
	}

	bool CSocket::Read(char** Paket, int nMaxLen)
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

		//size_t nLen = m_iBuffer.GetLength();
		//char* pChar = m_iBuffer.GetStart();
		//if ((nLen > sizeof(sbase::MsgHead)) && pChar)
		//{
		//	sbase::MsgHead* pHead = (sbase::MsgHead*)pChar;
		//	//rade_cnet::LogSave("Debug","Type:%d,Size:%d",pHead->usType,pHead->usSize);
		//	if (nLen >= pHead->usSize && pHead->usSize <= nMaxLen)
		//	{
		//		return m_iBuffer.Read((char*)Paket, pHead->usSize);
		//	}
		//	else
		//	{
		//		*Paket = NULL;
		//		return false;
		//	}
		//}
		//else
		//{
		//	*Paket = NULL;
		//	return false;
		//}
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

	const char* CSocket::GetPeerIP()
	{
		if (m_strIP.empty())
		{
			sockaddr_in	inAddr;
			::memset(&inAddr, 0, sizeof(inAddr));
			int		nLen = sizeof(inAddr);
			if (::getpeername(s, (sockaddr*)&inAddr, &nLen))
			{
				//CSocket::DumpError("CSocket::GetPeerIP(), call ::getpeername() error");
				printf("CSocket::GetPeerIP(), call ::getpeername() error");

				//this->Close();
				return NULL;
			}

			char* pszIP = ::inet_ntoa(inAddr.sin_addr);
			if (!pszIP)
				return NULL;

			m_strIP = pszIP;
		}

		return m_strIP.c_str();
	}

	void CSocket::Initnalize(SOCKET Sock)
	{
		//m_eMode = MODE_TEXT;
		OutstandingRecv = 0;
		OutstandingSend = 0;
		LastSendIssued = 0;
		IoCountIssued = 0;
		IOCompleted = 0;
		m_iBuffer.Clear();
		m_oBuffer.Clear();
		s = Sock;
		m_bEncrypt = false;
		m_BClosing = false;
		bConnected = FALSE;
		Repost = NULL;
		next = NULL;
		prev = NULL;
		OutOfOrderSends = NULL;
		m_Encryptor->Refresh();
		::InitializeCriticalSection(&SockCritSec);
		::InitializeCriticalSection(&StatusCritSec);
	}

	void CSocket::Finalize()
	{
		EnterCriticalSection(&SockCritSec);
		PerIOData* ptr = NULL, * prev = NULL;
		ptr = OutOfOrderSends;
		while (ptr)
		{
			prev = ptr;
			ptr = ptr->next;
			m_pIOCP->FreeBufferObj(prev);
		}
		OutOfOrderSends = NULL;
		LeaveCriticalSection(&SockCritSec);
	}
}