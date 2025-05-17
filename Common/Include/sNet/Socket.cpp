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
#define ENCRYPT_KEY1				0xa61fce5e	// A = 0x20, B = 0xFD, C = 0x07, first = 0x1F, key = a61fce5e
#define ENCRYPT_KEY2				0x443ffc04	// A = 0x7A, B = 0xCF, C = 0xE5, first = 0x3F, key = 443ffc04
		typedef	TEncryptServer <ENCRYPT_KEY1, ENCRYPT_KEY2>	CEncryptor;
		m_Encryptor = new CEncryptor;
	}

	CSocket::~CSocket()
	{
		SAFE_RELEASE(m_Encryptor);
		m_Sock = INVALID_SOCKET;
		::DeleteCriticalSection(&SockCritSec);
		::DeleteCriticalSection(&StatusCritSec);
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
#ifdef TRANSMIT_FILE
		if (m_eMode == MODE_FILE)
		{
			m_FileTransfer->RecveiveFile(pPerIOData, pPerIOData->m_BufLen);
		}
		else
#endif
		{
			Decrypt((char*)pPerIOData->m_Buf, pPerIOData->m_BufLen);

			m_iBuffer.Write(pPerIOData->m_Buf, pPerIOData->m_BufLen);
		}
	}

	void CSocket::OnWrite()
	{
		EnterCriticalSection(&SockCritSec);
		IOCompleted++;
		//printf("已经完成的:%d\n",IOCompleted );
		LeaveCriticalSection(&SockCritSec);
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
		//m_iBuffer.Initnalize();
	//	m_oBuffer.Initnalize();
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
			ptr = ptr->m_Next;
			m_pIOCP->FreePerIOData(prev);
		}
		OutOfOrderSends = NULL;
#ifdef TRANSMIT_FILE
		delete m_FileTransfer;
#endif
		LeaveCriticalSection(&SockCritSec);
	};

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

		LeaveCriticalSection(&SockCritSec);

		return ret;
	}

	void CSocket::InsertPendingSend(PerIOData* Sendobj)
	{
		PerIOData* ptr = NULL;
		EnterCriticalSection(&SockCritSec);
		Sendobj->m_Next = NULL;
		ptr = OutOfOrderSends;
		OutOfOrderSends = Sendobj;
		Sendobj->m_Next = ptr;
		IoCountIssued++;
		LeaveCriticalSection(&SockCritSec);
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

		EnterCriticalSection(&SockCritSec);

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
				//printf("PostSend: WSASend* failed: %d [internal = %d]\n", err, Sendobj->m_OL.Internal);
				printf("SendBuf failed in server socket with errorCode:%d\n", err);
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

		ZeroMemory(&Recvobj->m_OL, sizeof(WSAOVERLAPPED));

		Recvobj->m_iOperation = OP_READ;

		wbuf.buf = Recvobj->m_Buf;
		wbuf.len = Recvobj->m_BufLen;

		flags = 0;

		EnterCriticalSection(&SockCritSec);

		// 		Recvobj->IoOrder = IoCountIssued;
		// 		IoCountIssued++;

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
				//dbgprint("PostRecv: WSARecv* failed: %d\n", WSAGetLastError());
				rc = SOCKET_ERROR;
			}
		}
		if (rc != SOCKET_ERROR)
		{
			InterlockedIncrement(&OutstandingRecv);
		}

		LeaveCriticalSection(&SockCritSec);

		return rc;
	}

	bool CSocket::Write(int* line)
	{
		bool result = true;

		if (line)
			*line = __LINE__;

		EnterCriticalSection(&SockCritSec);
		int OLen = 0;

		if (OutstandingSend > 30)
		{
			LeaveCriticalSection(&SockCritSec);
			return false;
		}

		if (line)
			*line = __LINE__;

		while ((OLen = (int)m_oBuffer.GetLength()) > 0)
		{
			PerIOData* pPerIOData = m_pIOCP->GetPerIOData(DEFAULT_BUFFER_SIZE);
			if (NULL == pPerIOData)
			{
				LeaveCriticalSection(&SockCritSec);
				return false;
			}

			if (line)
				*line = __LINE__;

			int len = (OLen - DEFAULT_BUFFER_SIZE >= 0) ? DEFAULT_BUFFER_SIZE : OLen;
			pPerIOData->m_iOperation = OP_WRITE;
			//memcpy( pPerIOData->m_Buf,m_oBuffer.GetStart(),len);
			m_oBuffer.Read(pPerIOData->m_Buf, len);
			pPerIOData->m_BufLen = len;
			pPerIOData->m_Socket = this;
			pPerIOData->IoOrder = IoCountIssued;

			if (line)
				*line = __LINE__;

			InsertPendingSend(pPerIOData);
			//m_oBuffer.Read( NULL, len );

			if (line)
				*line = __LINE__;
		}

		if (line)
			*line = __LINE__;

		if (IoCountIssued > LastSendIssued && LastSendIssued == IOCompleted)
			result = (DoSends() != NO_ERROR) ? false : true;

		if (line)
			*line = __LINE__;

		LeaveCriticalSection(&SockCritSec);

		return result;
	}

	bool CSocket::PackMsg(char* pMsg, size_t iLen)
	{
		EnterCriticalSection(&SockCritSec);

		char* pChar = Encrypt(pMsg, iLen);

		if (m_oBuffer.Space() < iLen)
			Write();

		bool  res = m_oBuffer.Write(pChar, iLen);

		LeaveCriticalSection(&SockCritSec);

		return res;
	}

	void CSocket::Refresh()
	{
		EnterCriticalSection(&SockCritSec);
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
		LeaveCriticalSection(&SockCritSec);
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
			//rade_sbase::LogSave("Debug","Type:%d,Size:%d",pHead->usType,pHead->usSize);
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
		EnterCriticalSection(&StatusCritSec);
		m_bEncrypt = bFlag;

		if (m_iBuffer.GetLength() > 0)
			Decrypt(m_iBuffer.GetStart(), m_iBuffer.GetLength());

		LeaveCriticalSection(&StatusCritSec);
	}

	char* CSocket::Decrypt(char* pMsg, size_t iLen)
	{
		EnterCriticalSection(&StatusCritSec);

		if (m_bEncrypt)
		{
			m_Encryptor->Decrypt((unsigned char*)pMsg, (int)iLen);
			LeaveCriticalSection(&StatusCritSec);
			return pMsg;
		}

		LeaveCriticalSection(&StatusCritSec);
		return pMsg;
	}

	char* CSocket::Encrypt(char* pMsg, size_t iLen)
	{
		static char pBuf[10000] = { 0 };

		EnterCriticalSection(&StatusCritSec);

		if (m_bEncrypt)
		{
			memcpy(pBuf, pMsg, iLen);
			m_Encryptor->Encrypt((unsigned char*)pBuf, (int)iLen);
			LeaveCriticalSection(&StatusCritSec);
			return pBuf;
		}

		LeaveCriticalSection(&StatusCritSec);
		return pMsg;
	}
#ifdef TRANSMIT_FILE
	bool CSocket::ChangeMode(SOCKET_MODE iMode)
	{
		if (MODE_FILE == iMode && MODE_TEXT == m_eMode)
		{
			if (NULL == m_FileTransfer)
			{
				m_FileTransfer = new FTransfer;
			}
		}
		m_eMode = iMode;

		return (m_FileTransfer != NULL);
	}

	bool CSocket::FileTransmit()
	{
		PerIOData* pPerIOData = m_pIOCP->GetPerIOData(DEFAULT_BUFFER_SIZE);
		pPerIOData->m_iOperation = OP_WRITE;
		return m_FileTransfer->TransmitFile(m_Sock, pPerIOData);
	}

	bool CSocket::PrepareReceiveFile(LPCTSTR lpszFilename, DWORD dwFileSize)
	{
		EnterCriticalSection(&SockCritSec);
		if (!ChangeMode(MODE_FILE))
		{
			LeaveCriticalSection(&SockCritSec);
			return false;
		}
		bool ret = m_FileTransfer->PrepareReceiveFile(lpszFilename, dwFileSize);
		LeaveCriticalSection(&SockCritSec);
		return ret;
	}

	bool CSocket::PrepareSendFile(LPCTSTR lpszFilename, UINT& size)
	{
		EnterCriticalSection(&SockCritSec);
		if (!ChangeMode(MODE_FILE))
		{
			LeaveCriticalSection(&SockCritSec);
			return false;
		}
		bool ret = m_FileTransfer->PrepareSendFile(lpszFilename, size);
		LeaveCriticalSection(&SockCritSec);
		return ret;
	}

	void CSocket::OnFileTransmitCompleted()
	{
		EnterCriticalSection(&SockCritSec);
		delete m_FileTransfer;
		m_FileTransfer = NULL;
		m_eMode = MODE_TEXT;
		LeaveCriticalSection(&SockCritSec);
	}
#endif
}