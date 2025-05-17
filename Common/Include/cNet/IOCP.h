//========================================================
//
//    Copyright (c) 2007,欢乐连线工作室
//    All rights reserved.
//
//    文件名称：CirBuffer.h
//    摘  要：  CirBuffer封装类
//
//    当前版本：1.01
//    作   者： 李锋军
//    完成日期：2007-12-15
//    说   明： TCP粘包处理
//
//========================================================
#ifndef _FILE_IOCP_CLIENT_H_
#define _FILE_IOCP_CLIENT_H_

#include "PerIOData.h"

#define IP_MAXSIZE	32

namespace cnet
{
	class CSocket;

	class  CIOCP
	{
		friend class CSocket;
	public:
		unsigned long   	m_SendBufferSize;
		unsigned long       m_RecvBufferSize;
	public:
		CIOCP();
		~CIOCP();

		addrinfo* ResolveAddress(char* addr, char* port, int af, int type, int proto);

		CSocket* GetSocketObj(SOCKET s, int af);
		void FreeSocketObj(CSocket* obj);

		CSocket* GetColsedSocket();
		void CloseScoket(CSocket* pSocket);

		PerIOData* GetBufferObj(int buflen);
		void FreeBufferObj(PerIOData* obj);

		void InsertSocketObj(CSocket** head, CSocket* obj);
		int PostConnect(CSocket* sock, PerIOData* connobj);

		static DWORD WINAPI WorkerThread(LPVOID lpParam);
		static DWORD WINAPI SendThread(LPVOID lpParam);
		static DWORD WINAPI MonitorWorkerThread(LPVOID lpParam);

		//		int PostSend(CSocket *sock, PerIOData *sendobj);
		void SetPort(int af, SOCKADDR* sa, USHORT port);

		//		int PostRecv(CSocket *sock, PerIOData *recvobj);

		int HandleIo(CSocket* sock, PerIOData* buf, DWORD BytesTransfered, DWORD error);

		HANDLE CreateTempFile(char* filename, DWORD size);

		CSocket* GetSocketObj(SOCKET sck);

		bool Init(char* pszIP, int nPort, CSocket** ppSocket = NULL, unsigned long RecvBufferSize = 64 * 1024, unsigned long SendBufferSize = 64 * 1024 * 2);
		bool InitSocket(void);
		bool ConnectServer(CSocket** ppSocket, char* IP = NULL, int Port = 0);

		LONG GetTotalRecv() { return gBytesRead; }
		LONG GetTotalSend() { return gBytesSent; }
		bool GetServerState() { return m_ServerIsOk; }

	protected:
	private:
		HANDLE           m_hWorkerThread;
		HANDLE           m_MonitorThread;
		HANDLE           m_hSendThread;
		HANDLE           m_hCompletionPort;
		volatile LONG    gCurrentConnections;
		volatile LONG    gBytesRead;
		volatile LONG    gBytesSent;
		volatile LONG 	 gStartTime;
		volatile LONG 	 gBytesReadLast;
		volatile LONG 	 gBytesSentLast;
		volatile LONG 	 gStartTimeLast;
		volatile LONG 	 gTotalConnections;
		volatile LONG 	 gConnectionRefused;
		volatile LONG    m_OutstandingSends;

		CSocket* m_FreeSocketList;
		CSocket* m_CloseSocketCache;
		CSocket* m_ConnectionList;
		PerIOData* m_pFreeIODataList;				// 空闲链表

		SOCKET			 m_conSck;
		char			 m_szServerIP[IP_MAXSIZE];		// 服务器ip
		int				 m_nServerPort;					// 服务器port
		bool			 m_ServerIsOk;					// 服务器是否关闭

		CRITICAL_SECTION m_CriFreeIOList;
		CRITICAL_SECTION m_CriFreeSockList;
		CRITICAL_SECTION m_CriClosedSockList;
		CRITICAL_SECTION m_CriConnectedSockList;
		volatile bool  m_bMonitorStop;
		volatile bool  m_bRecvStop;
		volatile bool  m_bSendStop;
	};
}

#endif