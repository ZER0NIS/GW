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
#ifndef _FILE_SOCKET_CLIENT_H
#define _FILE_SOCKET_CLIENT_H

#include "stdafx.h"
#include "CirBuffer.h"

namespace cnet
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
		SOCKET               s;
		int                  af;
		volatile int       m_BClosing;
		BOOL                 bConnected;
		LPFN_CONNECTEX       lpfnConnectEx;
		LPFN_TRANSMITFILE    lpfnTransmitFile;
		volatile LONG        OutstandingRecv;
		volatile LONG        OutstandingSend;
		PerIOData* Repost;
		CRITICAL_SECTION     SockCritSec;
		CSocket* next, * prev;
	public:
		void Initnalize(SOCKET Sock);
		void Finalize();

		void OnRead(PerIOData* pPerIOData);
		void Release() { delete this; }

		//对该socket投递操作
		int PostSend(PerIOData* Sendobj);
		int PostRecv(PerIOData* Recvobj);

		//服务器投递发送请求
		bool Write(void);
		void OnWrite();
		//游戏逻辑向缓冲区中写数据DEFAULT_BUFFER_SIZE
		bool PackMsg(char* pMsg, size_t iLen);
		void InsertPendingSend(PerIOData* Sendobj);
		int DoSends();

		//从缓冲区中读出已接受到的数据
		bool Read(char** Paket, int nMaxLen = DEFAULT_BUFFER_SIZE);
		//不验证包头合法
		long NHJ_Read(char** Paket);
		//游戏逻辑处理完毕，移动缓冲区指针
		void Remove(size_t nLen) { m_iBuffer.Remove(nLen); };
		bool IsSendOver() { return (!m_oBuffer.GetLength() && LastSendIssued == IoCountIssued && IoCountIssued == IOCompleted); }

		bool IsValid() { return s != INVALID_SOCKET && !m_BClosing; };
		void Refresh();
		void ZeroBufMem() { s = INVALID_SOCKET; m_iBuffer.Clear(); m_oBuffer.Clear(); };

		void* operator new (size_t size);
		void  operator delete(void* p);

		CIOCP* GetIOCP() { return m_pIOCP; }
		const char* GetPeerIP();

	private:

		CRITICAL_SECTION   StatusCritSec;
		volatile bool   m_bEncrypt;
		string			m_strIP;
		CIOCP* m_pIOCP;
		PerIOData* OutOfOrderSends;
		CircularBuffer	m_iBuffer;
		CircularBuffer	m_oBuffer;
		IEncryptor* m_Encryptor;
		volatile SOCKET_MODE m_eMode;
		int                m_Af;
		volatile LONG      PendingSend;
		ULONG  LastSendIssued, IoCountIssued, IOCompleted;
	};
}

#endif