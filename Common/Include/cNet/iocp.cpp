#include "stdafx.h"
#include "IOCP.h"
#include "Socket.h"
#include <./assert.h>

#include "../Base/Inc/Ini.h"

//#define USE_LPFN_CONNECTEX

namespace cnet
{
	CIOCP::CIOCP() :m_bMonitorStop(false), m_bRecvStop(false), m_bSendStop(false)
	{
		m_hWorkerThread = NULL;
		m_hSendThread = NULL;
		m_MonitorThread = NULL;
		m_hCompletionPort = NULL;
		gCurrentConnections = 0;
		m_OutstandingSends = 0;
		gBytesRead = 0;
		gBytesSent = 0;
		gStartTime = 0;
		gBytesReadLast = 0;
		gBytesSentLast = 0;
		gStartTimeLast = 0;
		gTotalConnections = 0;
		gConnectionRefused = 0;
		m_SendBufferSize = 0;
		m_RecvBufferSize = 0;
		m_ServerIsOk = false;
		m_pFreeIODataList = NULL;
		m_FreeSocketList = NULL;
		m_ConnectionList = NULL;
		m_CloseSocketCache = NULL;
		InitializeCriticalSection(&m_CriFreeIOList);
		InitializeCriticalSection(&m_CriFreeSockList);
		InitializeCriticalSection(&m_CriClosedSockList);
		InitializeCriticalSection(&m_CriConnectedSockList);
	}

	CIOCP::~CIOCP()
	{
		m_bMonitorStop = m_bRecvStop = m_bSendStop = true;

		::PostQueuedCompletionStatus(m_hCompletionPort, -1, 0, 0);

		//等待线程结束 Fenjune Li[2009-07-27]
		if (WAIT_TIMEOUT == ::WaitForSingleObject(m_MonitorThread, 10000))
		{
 ::TerminateThread(m_MonitorThread, 4444);
		}

		if (m_MonitorThread)
 CloseHandle(m_MonitorThread);

		if (WAIT_TIMEOUT == ::WaitForSingleObject(m_hWorkerThread, 10000))
		{
 ::TerminateThread(m_hWorkerThread, 4444);
		}

		if (WAIT_TIMEOUT == ::WaitForSingleObject(m_hSendThread, 10000))
		{
 ::TerminateThread(m_hSendThread, 4444);
		}

		if (m_hWorkerThread)
 CloseHandle(m_hWorkerThread);

		if (m_hSendThread)
 CloseHandle(m_hSendThread);

		DeleteCriticalSection(&m_CriFreeIOList);
		DeleteCriticalSection(&m_CriFreeSockList);
		DeleteCriticalSection(&m_CriClosedSockList);
		DeleteCriticalSection(&m_CriConnectedSockList);

		PerIOData* pTemp;
		CSocket* pTempScok, * pCloseSock, * pConneted;

		//清除连接socket链表
		while (m_ConnectionList != NULL)
		{
 pConneted = m_ConnectionList->next;
 SAFE_DELETE(m_ConnectionList);
 m_ConnectionList = pConneted;
		}

		//清除关闭socket链表
		while (m_CloseSocketCache != NULL)
		{
 pCloseSock = m_CloseSocketCache->next;
 SAFE_DELETE(m_CloseSocketCache);
 m_CloseSocketCache = pCloseSock;
		}

		//清除空闲socket链表
		while (m_FreeSocketList != NULL)
		{
 pTempScok = m_FreeSocketList->next;
 SAFE_DELETE(m_FreeSocketList);
 m_FreeSocketList = pTempScok;
		}

		//清除空闲IO链表
		while (m_pFreeIODataList != NULL)
		{
 pTemp = m_pFreeIODataList->next;
 SAFE_DELETE(m_pFreeIODataList);
 m_pFreeIODataList = pTemp;
		}

		if (m_hCompletionPort)
		{
 CloseHandle(m_hCompletionPort);
		}

		WSACleanup();
	}

	addrinfo* CIOCP::ResolveAddress(char* addr, char* port, int af, int type, int proto)
	{
		addrinfo hints,
 * res = NULL;
		int             rc;

		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = ((addr) ? 0 : AI_PASSIVE);
		hints.ai_family = af;
		hints.ai_socktype = type;
		hints.ai_protocol = proto;

		rc = getaddrinfo(
 addr,
 port,
 &hints,
 &res
		);
		if (rc != 0)
		{
 printf("Invalid address %s, getaddrinfo failed: %d\n", addr, rc);
 return NULL;
		}
		return res;
	}

	CSocket* CIOCP::GetSocketObj(SOCKET s, int af)
	{
		CSocket* sockobj = NULL;

		sockobj = new CSocket(this);

		if (sockobj == NULL)
		{
 //fprintf(stderr, "GetSocketObj: HeapAlloc failed: %d\n", GetLastError());
 sbase::LogSave("Error", "GetSocketObj failed: %d,%s,%d", GetLastError(),
 	__FILE__, __LINE__);
 ExitProcess(0);
		}

		sockobj->s = s;
		sockobj->af = af;

		return sockobj;
	}

	CSocket* CIOCP::GetSocketObj(SOCKET sck)
	{
		EnterCriticalSection(&m_CriFreeSockList);
		CSocket* sockobj = NULL;

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
 sockobj->Initnalize(sck);
		}

		LeaveCriticalSection(&m_CriFreeSockList);

		return sockobj;
	}

	void CIOCP::FreeSocketObj(CSocket* obj)
	{
		EnterCriticalSection(&m_CriFreeSockList);
		obj->next = m_FreeSocketList;
		m_FreeSocketList = obj;
		LeaveCriticalSection(&m_CriFreeSockList);
	}

	void CIOCP::CloseScoket(CSocket* pSocket)
	{
		//关闭可用连接
		EnterCriticalSection(&m_CriConnectedSockList);
		CSocket* pTemp = m_ConnectionList, * pPrevScoket = NULL;;
		while (pTemp)
		{
 if (pTemp == pSocket)
 {
 	if (pPrevScoket)
 		pPrevScoket->next = pTemp->next;
 	else
 		m_ConnectionList = m_ConnectionList->next;

 	break;
 }

 pPrevScoket = pTemp;
 pTemp = pTemp->next;
		}
		LeaveCriticalSection(&m_CriConnectedSockList);

		//进入关闭作业
		EnterCriticalSection(&m_CriClosedSockList);
		pSocket->next = m_CloseSocketCache;
		m_CloseSocketCache = pSocket;
		LeaveCriticalSection(&m_CriClosedSockList);
	}

	CSocket* CIOCP::GetColsedSocket()
	{
		EnterCriticalSection(&m_CriClosedSockList);
		CSocket* pScoket = NULL, * pPrevScoket = NULL;
		pScoket = m_CloseSocketCache;
		if (NULL == pScoket)
		{
 LeaveCriticalSection(&m_CriClosedSockList);
 return NULL;
		}

		while (pScoket)
		{
 if (!pScoket->IsValid() &&
 	(pScoket->OutstandingRecv == 0) &&
 	(pScoket->OutstandingSend == 0))
 {
 	if (pPrevScoket)
 		pPrevScoket->next = pScoket->next;
 	else
 		m_CloseSocketCache = m_CloseSocketCache->next;

 	LeaveCriticalSection(&m_CriClosedSockList);
 	return pScoket;
 }

 pPrevScoket = pScoket;
 pScoket = pScoket->next;
		}

		LeaveCriticalSection(&m_CriClosedSockList);
		return NULL;
	}

	PerIOData* CIOCP::GetBufferObj(int buflen)
	{
		PerIOData* pIoData = NULL;
		EnterCriticalSection(&m_CriFreeIOList);

		if (m_pFreeIODataList == NULL)
		{
 pIoData = new PerIOData(buflen);
		}
		else
		{
 pIoData = m_pFreeIODataList;
 pIoData->buflen = buflen;

 m_pFreeIODataList = m_pFreeIODataList->next;
		}

		LeaveCriticalSection(&m_CriFreeIOList);

		return pIoData;
	}

	void CIOCP::FreeBufferObj(PerIOData* obj)
	{
		EnterCriticalSection(&m_CriFreeIOList);

		char* pBuf = obj->buf;

		memset(obj, 0, sizeof(PerIOData));

		obj->buf = pBuf;

		obj->next = m_pFreeIODataList;
		m_pFreeIODataList = obj;

		LeaveCriticalSection(&m_CriFreeIOList);
	}

	void CIOCP::InsertSocketObj(CSocket** head, CSocket* obj)
	{
		CSocket* end = NULL,
 * ptr = NULL;

		ptr = *head;
		if (ptr)
		{
 while (ptr->next)
 	ptr = ptr->next;
 end = ptr;
		}

		obj->next = NULL;
		obj->prev = end;

		if (end == NULL)
		{
 *head = obj;
		}
		else
		{
 end->next = obj;
 obj->prev = end;
		}
	}

	int CIOCP::PostConnect(CSocket* sock, PerIOData* connobj)
	{
		DWORD   bytes = 0;
		int     rc;

		connobj->operation = OP_CONNECT;

		rc = sock->lpfnConnectEx(
 sock->s,
 (SOCKADDR*)&connobj->addr,
 connobj->addrlen,
 connobj->buf,
 connobj->buflen,
 &bytes,
 &connobj->ol
		);
		if (rc == FALSE)
		{
 if (WSAGetLastError() != WSA_IO_PENDING)
 {
 	fprintf(stderr, "PostConnect: ConnectEx failed: %d\n",
 		WSAGetLastError());
 	return SOCKET_ERROR;
 }
		}

		return NO_ERROR;
	}

	DWORD WINAPI CIOCP::MonitorWorkerThread(LPVOID lpParam)
	{
		LONG StartTime, StartTimeLast;
		StartTime = StartTimeLast = GetTickCount();
		CIOCP* This = (CIOCP*)lpParam;
		CSocket* ClosedSocket = NULL;

		while (!This->m_bMonitorStop)
		{
 if ((ClosedSocket = This->GetColsedSocket()) != NULL)
 {
 	ClosedSocket->Finalize();
 	This->FreeSocketObj(ClosedSocket);
 }

 Sleep(50);
		}

		return 0;
	}

	DWORD WINAPI CIOCP::SendThread(LPVOID lpParam)
	{
		CIOCP* This = (CIOCP*)lpParam;
		while (!This->m_bSendStop)
		{
 CSocket* pSocket = This->m_ConnectionList;
 EnterCriticalSection(&This->m_CriConnectedSockList);
 while (pSocket)
 {
 	if (pSocket->IsValid())
 		pSocket->Write();
 	pSocket = pSocket->next;
 }
 LeaveCriticalSection(&This->m_CriConnectedSockList);
 Sleep(10);
		}

		return 0;
	}

	DWORD WINAPI CIOCP::WorkerThread(LPVOID lpParam)
	{
		CIOCP* This = (CIOCP*)lpParam;
		int          error, rc;
		DWORD        bytes, flags;
		CSocket* sockobj = NULL;
		PerIOData* buffobj = NULL;
		OVERLAPPED* lpOverlapped = NULL;
		while (!This->m_bRecvStop)
		{
 error = NO_ERROR;
 rc = GetQueuedCompletionStatus(
 	This->m_hCompletionPort,
 	&bytes,
 	(PULONG_PTR)&sockobj,
 	&lpOverlapped,
 	INFINITE
 );
 buffobj = CONTAINING_RECORD(lpOverlapped, PerIOData, ol);
 if (rc == 0)
 {
 	//fprintf(stderr, "GetQueuedCompletionStatus failed: %d\n", WSAGetLastError());
 	rc = WSAGetOverlappedResult(
 		sockobj->s,
 		lpOverlapped,
 		&bytes,
 		FALSE,
 		&flags
 	);

 	if (rc == FALSE)
 	{
 		//fprintf(stderr, "WSAGetOverlappedResult failed: %d\n", WSAGetLastError());
 		error = WSAGetLastError();
 	}
 }

 //通知退出
 if (bytes == -1 && !sockobj)
 {
 	break;
 }

 This->HandleIo(sockobj, buffobj, bytes, error);
		}

		return 0;
	}

	bool CIOCP::Init(char* pszIP, int nPort, CSocket** ppSocket, unsigned long RecvBufferSize, unsigned long SendBufferSize)
	{
		m_RecvBufferSize = RecvBufferSize;
		m_SendBufferSize = SendBufferSize;

		if (pszIP != NULL && nPort != 0)
		{
 strncpy(m_szServerIP, pszIP, IP_MAXSIZE);

 m_nServerPort = nPort;
		}

		if (!InitSocket())
		{
 return false;
		}

		if (m_hCompletionPort == NULL)
		{
 m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)NULL, 0);
 if (m_hCompletionPort == NULL)
 {
 	fprintf(stderr, "CreateIoCompletionPort failed: %d\n", GetLastError());
 	return false;
 }
		}

		if (m_hWorkerThread == NULL)
		{
 m_hWorkerThread = CreateThread(NULL, 0, WorkerThread, (LPVOID)this, 0, NULL);
 //rade_cnet::SysLogSave("Thread Create id=%d, %s, %d", GetThreadId( m_hWorkerThread ), __FILE__, __LINE__ );
 if (m_hWorkerThread == NULL)
 {
 	fprintf(stderr, "CreateThread failed: %d\n", GetLastError());
 	return false;
 }
 SetThreadAffinityMask(m_hWorkerThread, 12);
		}

		if (m_hSendThread == NULL)
		{
 m_hSendThread = CreateThread(NULL, 0, SendThread, (LPVOID)this, 0, NULL);
 if (m_hSendThread == NULL)
 {
 	fprintf(stderr, "CreateThread failed: %d\n", GetLastError());
 	return false;
 }
 SetThreadAffinityMask(m_hSendThread, 12);
		}

		if (m_MonitorThread == NULL)
		{
 m_MonitorThread = CreateThread(NULL, 0, MonitorWorkerThread, (LPVOID)this, 0, NULL);
 if (m_MonitorThread == NULL)
 {
 	fprintf(stderr, "CreateThread failed: %d\n", GetLastError());
 	return false;
 }
 SetThreadAffinityMask(m_MonitorThread, 12);
		}

		if (NULL != ppSocket)
		{
 return ConnectServer(ppSocket, m_szServerIP, m_nServerPort);
		}

		return true;
	} // Init()

	bool CIOCP::InitSocket()
	{
		WSADATA wsdata;

		if (WSAStartup(MAKEWORD(2, 2), &wsdata) != 0)
		{
 printf("WSAStartup Error \n");
 Beep(1000, 5);
 return false;
		}

		return true;
	} // InitSocket()

	bool CIOCP::ConnectServer(CSocket** ppSocket, char* IP, int Port)
	{
		SOCKET Sck = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == Sck)
		{
 return false;
		}

		// 设置SENDBUF
		const ULONG SNDBUF_SIZE = 20 * 1024;
		int optval = SNDBUF_SIZE;
		if (::setsockopt(Sck, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval)))
		{
 ::closesocket(Sck);
 return false;
		}

		// 设置RECVBUG
		if (setsockopt(Sck, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(optval)))
		{
 ::closesocket(Sck);
 return false;
		}

		SOCKADDR_IN server;

		if (NULL == IP)
		{
 server.sin_family = AF_INET;
 server.sin_port = htons((u_short)m_nServerPort);
 server.sin_addr.s_addr = inet_addr(m_szServerIP);
		}
		else
		{
 server.sin_family = AF_INET;
 server.sin_port = htons((u_short)Port);
 server.sin_addr.s_addr = inet_addr(IP);
		}

		if (-1 == connect(Sck, (SOCKADDR*)&server, sizeof(SOCKADDR)))
		{
 //printf("Connect Server Fail! ErrorID:%d \n", WSAGetLastError());
 ::closesocket(Sck);
 return false;
		}

		// 创建客户端句柄
		CSocket* ClientSck = GetSocketObj(Sck);

		if (!ClientSck)
		{
 ::closesocket(Sck);
 return false;
		}

		HANDLE hdl = CreateIoCompletionPort((HANDLE)Sck, m_hCompletionPort, (ULONG_PTR)ClientSck, 0);

		if (NULL == hdl)
		{
 cout << "CreateIoCompletionPort fail! Error:" << GetLastError() << endl;
 return false;
		}

		PerIOData* pIoData = GetBufferObj(DEFAULT_BUFFER_SIZE);

		if (NULL != pIoData)
		{
 if (ClientSck->PostRecv(pIoData) != NO_ERROR)
 {
 	FreeBufferObj(pIoData);
 	return false;
 }
		}

		*ppSocket = ClientSck;

		EnterCriticalSection(&m_CriConnectedSockList);
		ClientSck->next = m_ConnectionList;
		m_ConnectionList = ClientSck;
		LeaveCriticalSection(&m_CriConnectedSockList);

		m_ServerIsOk = true;

		return true;
	} // ConnectServer()

	void CIOCP::SetPort(int af, SOCKADDR* sa, USHORT port)
	{
		if (af == AF_INET)
		{
 ((SOCKADDR_IN*)sa)->sin_port = htons(port);
		}
		else if (af == AF_INET6)
		{
 ((SOCKADDR_IN6*)sa)->sin6_port = htons(port);
		}
	}

	int CIOCP::HandleIo(CSocket* sock, PerIOData* buf, DWORD BytesTransfered, DWORD error)
	{
		PerIOData* recvobj = NULL;

		if (error != NO_ERROR)
		{
 if (buf->operation == OP_READ)
 {
 	::InterlockedDecrement(&sock->OutstandingRecv);
 }
 else if (buf->operation == OP_WRITE)
 {
 	::InterlockedDecrement(&sock->OutstandingSend);
 }
 sock->Refresh();
 FreeBufferObj(buf);
 return SOCKET_ERROR;
		}
		else
		{
 if (buf->operation == OP_READ)
 {
 	InterlockedDecrement(&sock->OutstandingRecv);
 	if ((BytesTransfered > 0) && (!sock->m_BClosing))
 	{
 		InterlockedExchangeAdd(&gBytesRead, BytesTransfered);
 		InterlockedExchangeAdd(&gBytesReadLast, BytesTransfered);

 		buf->buflen = BytesTransfered;
 		sock->OnRead(buf);
 		recvobj = buf;
 		recvobj->buflen = DEFAULT_BUFFER_SIZE;
 		recvobj->pSocket = sock;

 		if (sock->PostRecv(recvobj) != NO_ERROR)
 		{
  printf("sock %d, PostRecvError!\n", sock->s);
  sock->Refresh();
  FreeBufferObj(recvobj);
 		}
 	}
 	else
 	{
 		sock->Refresh();
 		FreeBufferObj(buf);
 	}
 }
 else if (buf->operation == OP_WRITE)
 {
 	InterlockedDecrement(&sock->OutstandingSend);
 	InterlockedExchangeAdd(&gBytesSent, BytesTransfered);
 	InterlockedExchangeAdd(&gBytesSentLast, BytesTransfered);
 	if (BytesTransfered > 0)
 	{
 		sock->OnWrite();
 		if (sock->DoSends() != NO_ERROR)
  sock->Refresh();
 	}
 	else
 		sock->Refresh();
 	FreeBufferObj(buf);
 }
#ifdef TRANSMIT_FILE
 else if (buf->operation == OP_TRANSMIT)
 {
 	sock->OnFileTransmitCompleted();
 	sock->Refresh();
 	FreeBufferObj(buf);
 }
#endif
		}

		return 1;
	}

	HANDLE CIOCP::CreateTempFile(char* filename, DWORD size)
	{
		OVERLAPPED ol;
		HANDLE     hFile;
		DWORD      bytes2write,
 offset,
 nLeft,
 written,
 buflen = 1024;
		char       buf[1024];
		int        rc;
		hFile = CreateFile(
 filename,
 GENERIC_READ | GENERIC_WRITE,
 FILE_SHARE_READ,
 NULL,
 CREATE_ALWAYS,
 FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_OVERLAPPED |
 FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_DELETE_ON_CLOSE,
 NULL
		);
		if (hFile == INVALID_HANDLE_VALUE)
		{
 fprintf(stderr, "CreateTempFile failed: %d\n", GetLastError());
 return hFile;
		}

		memset(buf, '$', buflen);

		memset(&ol, 0, sizeof(ol));
		offset = 0;

		ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (ol.hEvent == FALSE)
		{
 fprintf(stderr, "CreateTempFile: CreateEvent failed: %d\n",
 	GetLastError());
 return INVALID_HANDLE_VALUE;
		}

		nLeft = size;
		while (nLeft > 0)
		{
 bytes2write = ((nLeft < buflen) ? nLeft : buflen);

 ol.Offset = offset;

 rc = WriteFile(
 	hFile,
 	buf,
 	bytes2write,
 	&written,
 	&ol
 );
 if (rc == 0)
 {
 	if (GetLastError() != ERROR_IO_PENDING)
 	{
 		fprintf(stderr, "CreateTempFile: WriteFile failed: %d\n",
  GetLastError());
 		return INVALID_HANDLE_VALUE;
 	}
 	else
 	{
 		rc = GetOverlappedResult(
  hFile,
  &ol,
  &written,
  TRUE
 		);
 		if (rc == 0)
 		{
  fprintf(stderr, "CreateTempFile: GetOverlappedResult failed: %d\n",
  	GetLastError());
  return INVALID_HANDLE_VALUE;
 		}
 	}
 }
 ResetEvent(ol.hEvent);

 offset += written;
 nLeft -= written;
		}

		printf("Created temp file of size: %d\n", offset);

		CloseHandle(ol.hEvent);

		return hFile;
	}
}