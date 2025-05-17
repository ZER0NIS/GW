#include "stdafx.h"
#include "ListenSocket.h"
#include "IOCP.h"

namespace snet
{
	int CLisetenSocket::PostAccept(PerIOData* acceptobj)
	{
		DWORD   bytes;
		int     rc;

		ZeroMemory(&acceptobj->m_OL, sizeof(WSAOVERLAPPED));
		acceptobj->m_iOperation = OP_ACCEPT;
		acceptobj->m_Client = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == acceptobj->m_Client)
		{
			return SOCKET_ERROR;
		}

		BOOL bKeepAlive = TRUE;
		int nRet = ::setsockopt(acceptobj->m_Client, SOL_SOCKET, SO_KEEPALIVE, (char*)&bKeepAlive, sizeof(bKeepAlive));
		if (nRet != 0)
		{
			return SOCKET_ERROR;
		}

		tcp_keepalive inKeepAlive = { 0 };
		unsigned long ulInLen = sizeof(tcp_keepalive);

		tcp_keepalive outKeepAlive = { 0 };
		unsigned long ulOutLen = sizeof(tcp_keepalive);

		unsigned long ulBytesReturn = 0;

		inKeepAlive.onoff = 1;
		inKeepAlive.keepaliveinterval = 1000;
		inKeepAlive.keepalivetime = 3;

		nRet = WSAIoctl(acceptobj->m_Client,
			SIO_KEEPALIVE_VALS,
			(LPVOID)&inKeepAlive,
			ulInLen,
			(LPVOID)&outKeepAlive,
			ulOutLen,
			&ulBytesReturn,
			NULL,
			NULL);
		if (SOCKET_ERROR == nRet)
		{
			return SOCKET_ERROR;
		}

		rc = m_lpfnAcceptEx(
			m_Socket,
			acceptobj->m_Client,
			acceptobj->m_Buf,
			acceptobj->m_BufLen - ((sizeof(SOCKADDR_STORAGE) + 16) * 2),
			sizeof(SOCKADDR_STORAGE) + 16,
			sizeof(SOCKADDR_STORAGE) + 16,
			&bytes,
			&acceptobj->m_OL
		);
		if (rc == FALSE)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf("PostAccept: AcceptEx failed: %d\n",
					WSAGetLastError());
				if (acceptobj->m_Buf == NULL)
					printf("acceptobj->m_Buf");

				return SOCKET_ERROR;
			}
		}

		return NO_ERROR;
	}

	void CLisetenSocket::InsertPendingAccept(PerIOData* BufObj)
	{
		BufObj->m_Next = NULL;

		{
				std::lock_guard<std::mutex> lock(ListenCritSec);

			if (m_PendingAccepts == NULL)
			{
				m_PendingAccepts = BufObj;
			}
			else
			{
				BufObj->m_Next = m_PendingAccepts;
				m_PendingAccepts = BufObj;
			}

			InterlockedIncrement(&m_PendingAcceptCount);

		}
	}

	void CLisetenSocket::RemovePendingAccept(PerIOData* BufObj)
	{
		PerIOData* ptr = NULL, * prev = NULL;

		{
			std::lock_guard<std::mutex> lock(ListenCritSec);

		ptr = m_PendingAccepts;
		while ((ptr) && (ptr != BufObj))
		{
			prev = ptr;
			ptr = ptr->m_Next;
		}
		if (prev)
		{
			prev->m_Next = BufObj->m_Next;
		}
		else
		{
			m_PendingAccepts = BufObj->m_Next;
		}

		BufObj->m_Next = NULL;

		InterlockedDecrement(&m_PendingAcceptCount);

		}
	}

	bool CLisetenSocket::Init(addrinfo* Ptr, CIOCP* pIOCP)
	{
		GUID  GUIDAcceptEx = WSAID_ACCEPTEX,
			DUIDGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS,
			GUIDTransmitFile = WSAID_TRANSMITFILE;
		DWORD Bytes;


		m_AddressFamily = Ptr->ai_family;

		m_Socket = socket(Ptr->ai_family, Ptr->ai_socktype, Ptr->ai_protocol);
		if (INVALID_SOCKET == m_Socket)
		{
			fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
			return false;
		}

		m_HAcceptEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (NULL == m_HAcceptEvent)
		{
			fprintf(stderr, "CreateEvent failed: %d\n", GetLastError());
			return false;
		}

		m_HRepostAccept = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (NULL == m_HRepostAccept)
		{
			fprintf(stderr, "CreateEvent failed: %d\n", GetLastError());
			return false;
		}

		pIOCP->m_WaitEvents[pIOCP->m_WaitCount++] = m_HAcceptEvent;
		pIOCP->m_WaitEvents[pIOCP->m_WaitCount++] = m_HRepostAccept;

		HANDLE hrc = CreateIoCompletionPort((HANDLE)m_Socket, pIOCP->m_CompletionPort, (ULONG_PTR)this, 0);
		if (hrc == NULL)
		{
			fprintf(stderr, "CreateIoCompletionPort failed: %d\n", GetLastError());
			return false;
		}

		int rc = bind(m_Socket, Ptr->ai_addr, (int)Ptr->ai_addrlen);
		if (SOCKET_ERROR == rc)
		{
			fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
			return false;
		}

		rc = WSAIoctl(
			m_Socket,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GUIDAcceptEx,
			sizeof(GUIDAcceptEx),
			&m_lpfnAcceptEx,
			sizeof(m_lpfnAcceptEx),
			&Bytes,
			NULL,
			NULL
		);
		if (SOCKET_ERROR == rc)
		{
			fprintf(stderr, "WSAIoctl: SIO_GET_EXTENSION_FUNCTION_POINTER failed: %d\n",
				WSAGetLastError());
			return false;
		}

		rc = WSAIoctl(
			m_Socket,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&DUIDGetAcceptExSockaddrs,
			sizeof(DUIDGetAcceptExSockaddrs),
			&m_lpfnGetAcceptExSockaddrs,
			sizeof(m_lpfnGetAcceptExSockaddrs),
			&Bytes,
			NULL,
			NULL
		);
		if (SOCKET_ERROR == rc)
		{
			fprintf(stderr, "WSAIoctl: SIO_GET_EXTENSION_FUNCTION_POINTER faled: %d\n",
				WSAGetLastError());
			return false;
		}

		rc = WSAIoctl(
			m_Socket,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GUIDTransmitFile,
			sizeof(GUIDTransmitFile),
			&m_pfnTransmitFile,
			sizeof(m_pfnTransmitFile),
			&Bytes,
			NULL,
			NULL
		);
		if (rc == SOCKET_ERROR)
		{
			fprintf(stderr, "WSAIoctl: SIO_GET_EXTENSION_FUNCTION_POINTER faled:\
				%d\n", WSAGetLastError());
			return false;
		}
		rc = listen(m_Socket, 200);
		if (SOCKET_ERROR == rc)
		{
			fprintf(stderr, "listen failed: %d\n", WSAGetLastError());
			return false;
		}

		rc = WSAEventSelect(m_Socket, m_HAcceptEvent, FD_ACCEPT);
		if (SOCKET_ERROR == rc)
		{
			fprintf(stderr, "WSAEventSelect failed: %d\n", WSAGetLastError());
			return false;
		}

		m_pIOCP = pIOCP;
		return true;
	}
}