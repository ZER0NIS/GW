#ifndef _FILE_IOCP_SERVER_H_
#define _FILE_IOCP_SERVER_H_

#include "stdafx.h"
#include "PerIOData.h"
#include "ListenSocket.h"
#include <deque>
#include <mutex>

namespace snet
{
	class CIOCP
	{
	public:
		CIOCP();
		~CIOCP();
		bool Init(char* Addr, char* Port, int AF, int Type, int Proto, long MaxCon, unsigned long RecvBufferSize = 64 * 1024, unsigned long SendBufferSize = 64 * 1024 * 2);

		addrinfo* ResolveAddress(char* Addr, char* Port, int AF, int Type, int Proto);
		int PrintAddress(SOCKADDR* Sa, int Salen);

		PerIOData* GetPerIOData(int BufLen);
		void FreePerIOData(PerIOData* PerIODat);

		static DWORD WINAPI CompletionThread(LPVOID lpParam);
		void HandleIo(ULONG_PTR Key, PerIOData* Buf, HANDLE CompPort, DWORD BytesTransfered, DWORD Error);
		void ShowStatus(bool flag);

		void FreeSocketObj(CSocket* Sockobj);
		CSocket* GetSocketObj(SOCKET Sock, int AF);
		static DWORD WINAPI MonitorThread(LPVOID lpParam);

		CSocket* PopNewConnect();
		void PushNewConnect(CSocket* newSocket);

		CSocket* PopCloseConnect();
		void PushNewClose(CSocket* newSocket);

	public:
		bool                m_PrintStatus;
		HANDLE              m_CompletionPort;
		CLisetenSocket* m_pLiSetenSock;
		addrinfo* m_Addrptr;
		SYSTEM_INFO         SysInfo;
		int                 m_WaitCount;
		unsigned long       m_SendBufferSize;
		unsigned long       m_RecvBufferSize;
		unsigned int        HaveSend;

		HANDLE              m_WaitEvents[MAX_COMPLETION_THREAD_COUNT];

		std::mutex          m_CriSection;
		std::mutex          m_BufferListCs;
		std::mutex          m_BSocketListCs;
		std::mutex          m_PendingCritSec;
		std::mutex          m_ConnectCritSec;
		std::mutex          m_CloseCritSec;

		volatile LONG       m_Connections;
		LONG                m_MaxConnections;
		volatile LONG       m_OutstandingSends;
		PerIOData* m_FreeBufferList;
		CSocket* m_FreeSocketList;

		std::deque<CSocket*> m_NewConnetct;
		std::deque<CSocket*> m_NewClose;

		volatile bool       m_bMonitorStop;
		volatile bool       m_bWorkStop;
	};
}
#endif