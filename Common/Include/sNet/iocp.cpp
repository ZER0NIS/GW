#include "stdafx.h"
#include "IOCP.h"

namespace snet
{
	CIOCP::CIOCP() :m_FreeBufferList(NULL), m_FreeSocketList(NULL), m_Addrptr(NULL),
		m_pLiSetenSock(NULL), m_RecvBufferSize(0), m_SendBufferSize(0), m_WaitCount(0), HaveSend(0),
		m_Connections(0), m_OutstandingSends(0), m_MaxConnections(0),
		m_PrintStatus(false), m_bWorkStop(false), m_bMonitorStop(false)
	{
	}

	CIOCP::~CIOCP()
	{
		m_bWorkStop = m_bMonitorStop = true;

		Sleep(10000);

		for (int index = 0; index < m_WaitCount; index++)
		{
			if (index < (int)SysInfo.dwNumberOfProcessors * 2)
			{
				if (::WaitForSingleObject(m_WaitEvents[index], 1000))
				{
					::TerminateThread(m_WaitEvents[index], 4444);
				}

				if (m_WaitEvents[index])
				{
					CloseHandle(m_WaitEvents[index]);
					m_WaitEvents[index] = NULL;
				}
			}
		}

		CloseHandle(m_pLiSetenSock->m_HAcceptEvent);
		CloseHandle(m_pLiSetenSock->m_HRepostAccept);

		{
			std::lock_guard<std::mutex> lock(m_BufferListCs);
			PerIOData* pTemp;
			while (m_FreeBufferList != NULL)
			{
				pTemp = m_FreeBufferList->m_Next;
				SAFE_DELETE(m_FreeBufferList);
				m_FreeBufferList = pTemp;
			}
		}

		CSocket* pTempSock;
		while (m_FreeSocketList != NULL)
		{
			pTempSock = m_FreeSocketList->next;
			SAFE_DELETE(m_FreeSocketList);
			m_FreeSocketList = pTempSock;
		}

		while (!m_NewConnetct.empty())
		{
			CSocket* Sock = m_NewConnetct.front();
			m_NewConnetct.pop_front();
			SAFE_RELEASE(Sock);
		}

		while (!m_NewClose.empty())
		{
			CSocket* Sock = m_NewClose.front();
			m_NewClose.pop_front();
			SAFE_RELEASE(Sock);
		}

		if (m_CompletionPort)
		{
			CloseHandle(m_CompletionPort);
		}

		WSACleanup();
	}

	void CIOCP::FreePerIOData(PerIOData* PerIODat)
	{
		std::lock_guard<std::mutex> lock(m_BufferListCs);

		char* pChar = PerIODat->m_Buf;
		memset(PerIODat, 0, sizeof(PerIOData));
		PerIODat->m_Buf = pChar;

		PerIODat->m_Next = m_FreeBufferList;
		m_FreeBufferList = PerIODat;
	}

	PerIOData* CIOCP::GetPerIOData(int BufLen)
	{
		PerIOData* NewIOData = NULL;
		std::lock_guard<std::mutex> lock(m_BufferListCs);

		if (NULL == m_FreeBufferList)
		{
			NewIOData = new PerIOData(BufLen);
			if (NewIOData == NULL || NewIOData->m_Buf == NULL)
			{
				int error = WSAGetLastError();
				char   buff[128];
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, (void*)FORMAT_MESSAGE_FROM_STRING, error, 0, buff, 128, 0);
				sbase::SysLogSave("PerIOData malloc failed with error:%s", buff);
				return NULL;
			}
		}
		else
		{
			NewIOData = m_FreeBufferList;
			m_FreeBufferList = NewIOData->m_Next;
			NewIOData->m_Next = NULL;
			NewIOData->m_BufLen = BufLen;
			NewIOData->IoOrder = 0;
		}

		return NewIOData;
	}

	DWORD WINAPI CIOCP::CompletionThread(LPVOID lpParam)
	{
		ULONG_PTR    Key;
		SOCKET       s;
		PerIOData* bufobj = NULL;
		OVERLAPPED* lpOverlapped = NULL;
		DWORD        BytesTransfered,
			Flags;
		int          rc, error;
		CIOCP* This = (CIOCP*)lpParam;
		while (!This->m_bWorkStop)
		{
			error = NO_ERROR;
			rc = GetQueuedCompletionStatus(
				This->m_CompletionPort,
				&BytesTransfered,
				(PULONG_PTR)&Key,
				&lpOverlapped,
				INFINITE
			);

			bufobj = CONTAINING_RECORD(lpOverlapped, PerIOData, m_OL);

			if (rc == FALSE)
			{
				if (bufobj->m_iOperation == OP_ACCEPT)
				{
					s = ((CLisetenSocket*)Key)->m_Socket;
				}
				else
				{
					s = ((CSocket*)Key)->m_Sock;
				}

				rc = WSAGetOverlappedResult(
					s,
					&bufobj->m_OL,
					&BytesTransfered,
					FALSE,
					&Flags
				);
				if (rc == FALSE)
				{
					error = WSAGetLastError();
				}
			}

			if (BytesTransfered == -1 && !Key)
			{
				sbase::LogSave("Error", "ExitThread %d,%d,%s!", GetCurrentThreadId(), __LINE__, __FILE__);
				break;
			}

			This->HandleIo(Key, bufobj, This->m_CompletionPort, BytesTransfered, error);
		}

		ExitThread(0);
	}

	void CIOCP::FreeSocketObj(CSocket* Sockobj)
	{
		std::lock_guard<std::mutex> lock(m_BSocketListCs);

		Sockobj->next = m_FreeSocketList;
		m_FreeSocketList = Sockobj;
		InterlockedDecrement(&m_Connections);
	}

	CSocket* CIOCP::GetSocketObj(SOCKET Sock, int AF)
	{
		CSocket* sockobj = NULL;
		std::lock_guard<std::mutex> lock(m_BSocketListCs);

		if (m_FreeSocketList == NULL)
		{
			sockobj = new CSocket(this);
		}
		else
		{
			sockobj = m_FreeSocketList;
			m_FreeSocketList = sockobj->next;
			sockobj->next = NULL;
		}

		if (sockobj)
		{
			sockobj->Initnalize(Sock, AF);
			InterlockedIncrement(&m_Connections);
		}

		return sockobj;
	}

	void CIOCP::ShowStatus(bool flag)
	{
		m_PrintStatus = flag;
	}

	void CIOCP::HandleIo(ULONG_PTR Key, PerIOData* Buf, HANDLE CompPort, DWORD BytesTransfered, DWORD Error)
	{
		CLisetenSocket* Listenobj = NULL;
		CSocket* Sockobj = NULL, * Clientobj = NULL;
		PerIOData* Recvobj = NULL;
		BOOL   bCleanupSocket = false;

		if (NO_ERROR != Error)
		{
			if (Buf->m_iOperation != OP_ACCEPT)
			{
				Sockobj = (CSocket*)Key;
				if (Buf->m_iOperation == OP_READ)
				{
					if ((InterlockedDecrement(&Sockobj->OutstandingRecv) == 0) &&
						(Sockobj->OutstandingSend == 0))
					{
						printf("%s closed\n", Sockobj->GetPeerIP());
						Sockobj->Refresh();
					}
				}
				else if (Buf->m_iOperation == OP_WRITE)
				{
					if ((InterlockedDecrement(&Sockobj->OutstandingSend) == 0) &&
						(Sockobj->OutstandingRecv == 0))
					{
						printf("%s closed\n", Sockobj->GetPeerIP());
						Sockobj->Refresh();
					}
					InterlockedDecrement(&m_OutstandingSends);
				}
			}
			else
			{
				std::lock_guard<std::mutex> lock(Listenobj->ListenCritSec);
				Listenobj = (CLisetenSocket*)Key;

				closesocket(Buf->m_Client);
				Buf->m_Client = INVALID_SOCKET;
				Listenobj->RemovePendingAccept(Buf);
			}

			FreePerIOData(Buf);

			return;
		}

		if (Buf->m_iOperation == OP_ACCEPT)
		{
			if (m_PrintStatus)
				sbase::SysLogSave("OP_ACCEPT");
			HANDLE  hrc;
			SOCKADDR_STORAGE* LocalSockaddr = NULL,
				* RemoteSockaddr = NULL;
			int  LocalSockaddrLen, RemoteSockaddrLen;
			Listenobj = (CLisetenSocket*)Key;

			Listenobj->RemovePendingAccept(Buf);

			if (m_Connections >= m_MaxConnections)
			{
				if (m_PrintStatus)
					sbase::SysLogSave("m_Connections", m_Connections);
				printf("Warning:Connection[%d] is full!\n", m_Connections);
				closesocket(Buf->m_Client);
				Buf->m_Client = INVALID_SOCKET;
				FreePerIOData(Buf);
			}
			else
			{
				Listenobj->m_lpfnGetAcceptExSockaddrs(
					Buf->m_Buf,
					Buf->m_BufLen - ((sizeof(SOCKADDR_STORAGE) + 16) * 2),
					sizeof(SOCKADDR_STORAGE) + 16,
					sizeof(SOCKADDR_STORAGE) + 16,
					(SOCKADDR**)&LocalSockaddr,
					&LocalSockaddrLen,
					(SOCKADDR**)&RemoteSockaddr,
					&RemoteSockaddrLen
				);

				Clientobj = GetSocketObj(Buf->m_Client, Listenobj->m_AddressFamily);

				Clientobj->SetPeerInfo((SOCKADDR*)RemoteSockaddr, RemoteSockaddrLen);

				if (Clientobj)
				{
					hrc = CreateIoCompletionPort(
						(HANDLE)Clientobj->m_Sock,
						CompPort,
						(ULONG_PTR)Clientobj,
						0
					);
					if (hrc == NULL)
					{
						fprintf(stderr, "CompletionThread: CreateIoCompletionPort failed: %d\n",
							GetLastError());
						FreeSocketObj(Clientobj);
						return;
					}

					Buf->m_BufLen = BytesTransfered;
					Clientobj->OnRead(Buf);

					Recvobj = Buf;
					Recvobj->m_BufLen = DEFAULT_BUFFER_SIZE;
					Recvobj->m_Socket = Clientobj;

					if (Clientobj->PostRecv(Recvobj) != NO_ERROR)
					{
						closesocket(Buf->m_Client);
						Buf->m_Client = INVALID_SOCKET;
						FreeSocketObj(Clientobj);
						FreePerIOData(Recvobj);
					}
					else
					{
						PushNewConnect(Clientobj);
					}
				}
				else
				{
					closesocket(Buf->m_Client);
					Buf->m_Client = INVALID_SOCKET;
					FreePerIOData(Buf);
				}

				if (NO_ERROR != Error)
				{
					EnterCriticalSection(&Clientobj->SockCritSec);
					if ((Clientobj->OutstandingSend == 0) &&
						(Clientobj->OutstandingRecv == 0))
					{
						closesocket(Clientobj->m_Sock);
						Clientobj->m_Sock = INVALID_SOCKET;
						Clientobj->Refresh();
					}
					else
					{
						Clientobj->m_BClosing = TRUE;
					}

					Error = NO_ERROR;
					LeaveCriticalSection(&Clientobj->SockCritSec);
				}
			}

			InterlockedIncrement(&Listenobj->m_RepostCount);
			SetEvent(Listenobj->m_HRepostAccept);
		}
		else if (Buf->m_iOperation == OP_READ)
		{
			if (m_PrintStatus)
				sbase::SysLogSave("OP_READ");

			Sockobj = (CSocket*)Key;

			InterlockedDecrement(&Sockobj->OutstandingRecv);
			if (BytesTransfered > 0)
			{
				Buf->m_BufLen = BytesTransfered;
				Sockobj->OnRead(Buf);

				Recvobj = Buf;
				Recvobj->m_BufLen = DEFAULT_BUFFER_SIZE;
				Recvobj->m_Socket = Sockobj;

				if (NULL != Recvobj)
				{
					if (Sockobj->PostRecv(Recvobj) != NO_ERROR)
					{
						FreePerIOData(Recvobj);
						Error = (DWORD)SOCKET_ERROR;
						if ((Sockobj->OutstandingSend == 0) &&
							(Sockobj->OutstandingRecv == 0))
						{
							bCleanupSocket = TRUE;
						}
					}
				}
			}
			else
			{
				EnterCriticalSection(&Sockobj->SockCritSec);
				Sockobj->m_BClosing = TRUE;
				FreePerIOData(Buf);
				if ((Sockobj->OutstandingSend == 0) &&
					(Sockobj->OutstandingRecv == 0))
				{
					bCleanupSocket = TRUE;
				}
				LeaveCriticalSection(&Sockobj->SockCritSec);
			}
		}
		else if (Buf->m_iOperation == OP_WRITE)
		{
			if (m_PrintStatus)
				sbase::SysLogSave("OP_WRITE");

			Sockobj = (CSocket*)Key;
			InterlockedDecrement(&Sockobj->OutstandingSend);
			InterlockedDecrement(&m_OutstandingSends);

			if (BytesTransfered > 0)
			{
				if (BytesTransfered < (DWORD)Buf->m_BufLen)
				{
					ASSERT(BytesTransfered < (DWORD)Buf->m_BufLen);
				}
				HaveSend += BytesTransfered;
				Sockobj->OnWrite();
			}
			else
			{
				EnterCriticalSection(&Sockobj->SockCritSec);
				Sockobj->m_BClosing = TRUE;
				if ((Sockobj->OutstandingSend == 0) &&
					(Sockobj->OutstandingRecv == 0))
				{
					bCleanupSocket = TRUE;
				}
				LeaveCriticalSection(&Sockobj->SockCritSec);
			}

			FreePerIOData(Buf);

			if (!Sockobj->m_BClosing && Sockobj->DoSends() != NO_ERROR)
			{
				Error = (DWORD)SOCKET_ERROR;
				if ((Sockobj->OutstandingSend == 0) &&
					(Sockobj->OutstandingRecv == 0))
				{
					bCleanupSocket = TRUE;
				}
			}
		}

		if (Sockobj)
		{
			if (NO_ERROR != Error)
			{
				Sockobj->m_BClosing = true;
			}

			if (bCleanupSocket && Sockobj->m_BClosing)
			{
				Sockobj->Refresh();
			}
		}

		return;
	}

	DWORD WINAPI CIOCP::MonitorThread(LPVOID lpParam)
	{
		LONG StartTime, StartTimeLast;
		StartTime = StartTimeLast = GetTickCount();
		int rc = 0;
		CIOCP* This = (CIOCP*)lpParam;
		CLisetenSocket* listenobj;
		PerIOData* acceptobj;
		CSocket* ClosedSocket = NULL;

		while (!This->m_bMonitorStop)
		{
			while ((ClosedSocket = This->PopCloseConnect()) != NULL)
			{
				if (NULL != ClosedSocket)
				{
					ClosedSocket->Finalize();
					This->FreeSocketObj(ClosedSocket);
				}
			}
			rc = WSAWaitForMultipleEvents(
				This->m_WaitCount,
				This->m_WaitEvents,
				FALSE,
				1000,
				FALSE
			);
			if (rc == WSA_WAIT_FAILED)
			{
				fprintf(stderr, "WSAWaitForMultipleEvents failed: %d\n", WSAGetLastError());
				break;
			}
			else if (rc == WSA_WAIT_TIMEOUT)
			{
				int optval, optlen;
				listenobj = This->m_pLiSetenSock;

				while (listenobj)
				{
					//EnterCriticalSection(&listenobj->ListenCritSec);
					std::lock_guard<std::mutex> lock(listenobj->ListenCritSec);

					acceptobj = listenobj->m_PendingAccepts;
					int Num = 0;
					while (acceptobj)
					{
						Num++;
						optlen = sizeof(optval);
						rc = getsockopt(
							acceptobj->m_Client,
							SOL_SOCKET,
							SO_CONNECT_TIME,
							(char*)&optval,
							&optlen
						);
						if (rc == SOCKET_ERROR)
						{
							fprintf(stderr, "getsockopt: SO_CONNECT_TIME failed: %d\n",
								WSAGetLastError());
						}
						else
						{
							if ((optval != 0xFFFFFFFF) && (optval > 10))
							{
								closesocket(acceptobj->m_Client);
								acceptobj->m_Client = INVALID_SOCKET;
							}
						}

						if (Num == listenobj->m_PendingAcceptCount && acceptobj->m_Next != NULL)
						{
							acceptobj->m_Next = NULL;
						}

						acceptobj = acceptobj->m_Next;

						Sleep(10);
					}

					//	LeaveCriticalSection(&listenobj->ListenCritSec);

					listenobj = listenobj->Next;
					Sleep(10);
				}
			}
			else
			{
				int index = rc - WAIT_OBJECT_0;

				for (; index < This->m_WaitCount; index++)
				{
					rc = WaitForSingleObject(This->m_WaitEvents[index], 0);
					if (rc == WAIT_FAILED || rc == WAIT_TIMEOUT)
					{
						continue;
					}

					if (index < (int)This->SysInfo.dwNumberOfProcessors * 2)
					{
						sbase::LogSave("Error", "CIOCP::MonitorThread: %d,%s,%d",
							GetLastError(), __FILE__, __LINE__);
						ExitProcess(0);
					}
					else
					{
						listenobj = This->m_pLiSetenSock;
						while (listenobj)
						{
							if ((listenobj->m_HAcceptEvent == This->m_WaitEvents[index]) ||
								(listenobj->m_HRepostAccept == This->m_WaitEvents[index]))
								break;
							listenobj = listenobj->Next;
							Sleep(10);
						}

						if (listenobj)
						{
							WSANETWORKEVENTS ne;
							int              limit = 0;

							if (listenobj->m_HAcceptEvent == This->m_WaitEvents[index])
							{
								rc = WSAEnumNetworkEvents(
									listenobj->m_Socket,
									listenobj->m_HAcceptEvent,
									&ne
								);
								if (rc == SOCKET_ERROR)
								{
									fprintf(stderr, "WSAEnumNetworkEvents failed: %d,%d\n",
										listenobj->m_Socket, WSAGetLastError());
								}
								if ((ne.lNetworkEvents & FD_ACCEPT) == FD_ACCEPT)
								{
									limit = BURST_ACCEPT_COUNT;
								}
							}
							else if (listenobj->m_HRepostAccept == This->m_WaitEvents[index])
							{
								limit = InterlockedExchange(&listenobj->m_RepostCount, 0);

								ResetEvent(listenobj->m_HRepostAccept);
							}

							int i = 0;
							while ((i++ < limit) && (listenobj->m_PendingAcceptCount < MAX_OVERLAPPED_ACCEPTS))
							{
								acceptobj = This->GetPerIOData(DEFAULT_BUFFER_SIZE);
								if (acceptobj)
								{
									acceptobj->m_HPostAccept = listenobj->m_HAcceptEvent;

									listenobj->InsertPendingAccept(acceptobj);

									if (listenobj->PostAccept(acceptobj) == SOCKET_ERROR)
									{
										listenobj->RemovePendingAccept(acceptobj);
										This->FreePerIOData(acceptobj);
									};
								}
							}
						}
					}
				}
			}
			Sleep(10);
		}
		return 0;
	}

	addrinfo* CIOCP::ResolveAddress(char* Addr, char* Port, int AF, int Type, int Proto)
	{
		addrinfo Hints, * Res = NULL;
		int Rc;

		memset(&Hints, 0, sizeof(Hints));
		Hints.ai_flags = ((Addr) ? 0 : AI_PASSIVE);
		Hints.ai_family = AF;
		Hints.ai_socktype = Type;
		Hints.ai_protocol = Proto;

		Rc = getaddrinfo(Addr, Port, &Hints, &Res);
		if (Rc != 0)
		{
			printf("Invalid address %s, getaddrinfo failed: %d\n", Addr, Rc);
			return NULL;
		}
		return Res;
	}

	int CIOCP::PrintAddress(SOCKADDR* Sa, int Salen)
	{
		char host[NI_MAXHOST], serv[NI_MAXSERV];
		int  hostlen = NI_MAXHOST,
			servlen = NI_MAXSERV,
			rc;

		rc = getnameinfo(
			Sa,
			Salen,
			host,
			hostlen,
			serv,
			servlen,
			NI_NUMERICHOST | NI_NUMERICSERV
		);
		if (rc != 0)
		{
			fprintf(stderr, "%s: getnameinfo failed: %d\n", __FILE__, rc);
			return rc;
		}

		if (strcmp(serv, "0") != 0)
		{
			if (Sa->sa_family == AF_INET)
				printf("[%s]:%s", host, serv);
			else
				printf("%s:%s", host, serv);
		}
		else
			printf("%s", host);

		return NO_ERROR;
	}

	bool CIOCP::Init(char* Addr, char* Port, int AF, int Type, int Proto, long MaxCon, unsigned long RecvBufferSize, unsigned long SendBufferSize)
	{
		WSADATA  Wsd;
		addrinfo* Res = NULL, * Ptr = NULL;
		CLisetenSocket* listenobj = NULL;
		PerIOData* AcceptObj = NULL;

		m_MaxConnections = MaxCon;

		if (WSAStartup(MAKEWORD(2, 2), &Wsd) != 0)
		{
			fprintf(stderr, "unable to load Winsock!\n");
			return false;
		}

		m_CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)NULL, 0);
		if (NULL == m_CompletionPort)
		{
			fprintf(stderr, "CreateIoCompletionPort failed: %d\n", GetLastError());
			return false;
		}

		GetSystemInfo(&SysInfo);
		if (SysInfo.dwNumberOfProcessors > MAX_COMPLETION_THREAD_COUNT)
		{
			SysInfo.dwNumberOfProcessors = MAX_COMPLETION_THREAD_COUNT;
		}

		m_RecvBufferSize = RecvBufferSize;
		m_SendBufferSize = SendBufferSize;

		printf("RecvBuffer size = %lu, SendBuffer Size = %lu  (page size = %lu)\n", m_RecvBufferSize, m_SendBufferSize, SysInfo.dwPageSize);

		for (; m_WaitCount < (int)SysInfo.dwNumberOfProcessors * 2; m_WaitCount++)
		{
			m_WaitEvents[m_WaitCount] = CreateThread(NULL, 0, CompletionThread, (LPVOID)this, 0, NULL);
			if (m_WaitEvents[m_WaitCount] == NULL)
			{
				fprintf(stderr, "CreatThread failed: %d\n", GetLastError());
				return false;
			}
			SetThreadAffinityMask(m_WaitEvents[m_WaitCount], 12);
		}

		Res = ResolveAddress(Addr, Port, AF, Type, Proto);
		if (NULL == Res)
		{
			fprintf(stderr, "ResolveAddress failed to return any addresses!\n");
			return false;
		}

		m_Addrptr = Ptr = Res;

		for (int a = 0; a < m_MaxConnections; a++)
		{
			CSocket* pSock = new CSocket(this);
			FreeSocketObj(pSock);
		}
		InterlockedExchange(&m_Connections, 0);

		while (Ptr)
		{
			printf("Listening address: ");
			PrintAddress(Ptr->ai_addr, (int)Ptr->ai_addrlen);
			printf("\n");

			listenobj = (CLisetenSocket*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLisetenSocket));
			if (listenobj == NULL)
			{
				fprintf(stderr, "Out of memory!\n");
				return false;
			}

			listenobj->m_iHiWaterMark = DEFAULT_OVERLAPPED_COUNT;
			if (!listenobj->Init(Ptr, this))
				return false;

			for (int i = 0; i < DEFAULT_OVERLAPPED_COUNT; i++)
			{
				AcceptObj = GetPerIOData(DEFAULT_BUFFER_SIZE);
				if (NULL == AcceptObj)
				{
					fprintf(stderr, "Out of memory!\n");
					return false;
				}

				AcceptObj->m_HPostAccept = listenobj->m_HAcceptEvent;
				listenobj->InsertPendingAccept(AcceptObj);
				listenobj->PostAccept(AcceptObj);
			}

			if (NULL == m_pLiSetenSock)
			{
				m_pLiSetenSock = listenobj;
			}
			else
			{
				listenobj->Next = m_pLiSetenSock;
				m_pLiSetenSock = listenobj;
			}

			Ptr = Ptr->ai_next;
		}

		freeaddrinfo(Res);

		m_WaitEvents[m_WaitCount++] = CreateThread(NULL, 0, MonitorThread, (LPVOID)this, 0, NULL);
		if (m_WaitEvents[m_WaitCount - 1] == NULL)
		{
			fprintf(stderr, "CreatThread failed: %d\n", GetLastError());
			return false;
		}

		SetThreadAffinityMask(m_WaitEvents[m_WaitCount - 1], 12);

		return true;
	}

	CSocket* CIOCP::PopNewConnect()
	{
		CSocket* NewSock = NULL;
		std::lock_guard<std::mutex> lock(m_ConnectCritSec);

		if (!m_NewConnetct.empty())
		{
			NewSock = m_NewConnetct.front();
			m_NewConnetct.pop_front();
		}
		return NewSock;
	}

	void CIOCP::PushNewConnect(CSocket* newSocket)
	{
		std::lock_guard<std::mutex> lock(m_ConnectCritSec);

		m_NewConnetct.push_back(newSocket);
	}

	CSocket* CIOCP::PopCloseConnect()
	{
		CSocket* NewSock = NULL;

		if (!m_NewClose.empty())
		{
			std::lock_guard<std::mutex> lock(m_CloseCritSec);

			NewSock = m_NewClose.front();
			if (NewSock->IsValid())
			{
				NewSock = NULL;
			}
			else
			{
				m_NewClose.pop_front();
			}
		}

		return NewSock;
	}

	void CIOCP::PushNewClose(CSocket* newSocket)
	{
		std::lock_guard<std::mutex> lock(m_CloseCritSec);

		m_NewClose.push_back(newSocket);
	}
}