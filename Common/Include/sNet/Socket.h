//========================================================
//
//    Copyright (c) 2008,�������߹�����
//    All rights reserved.
//
//    �ļ����� �� Socket.h
//    ժ    Ҫ �� ��ͨSocketͨѶ��ͷ�ļ�
//
//    ��ǰ�汾 �� 1.00
//    ��    �� �� ����
//    ������� �� 2008-05-01
//
//========================================================

#ifndef _FILE_SOCKET_H_
#define _FILE_SOCKET_H_

#include "stdafx.h"
#include "CirBuffer.h"
#include "FileTransfer.h"

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

		//���֪ͨ����
		void OnRead(PerIOData* pPerIOData);
		void OnWrite();

		//�Ը�socketͶ�ݲ���
		int PostSend(PerIOData* Sendobj);
		int PostRecv(PerIOData* Recvobj);

		//������Ͷ�ݷ�������
		bool Write(int* line = NULL);
		//��Ϸ�߼��򻺳�����д����
		bool PackMsg(char* pMsg, size_t iLen);

		//�ӻ������ж����ѽ��ܵ�������
		bool Read(char** Paket, int nMaxLen = DEFAULT_BUFFER_SIZE);
		long NHJ_Read(char** Paket);
		//��Ϸ�߼�������ϣ��ƶ�������ָ��
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

		bool PrepareReceiveFile(LPCTSTR lpszFilename, DWORD dwFileSize);
		bool PrepareSendFile(LPCTSTR lpszFilename, UINT& size);
		bool FileTransmit();
		void OnFileTransmitCompleted();
	public:
		CIOCP* m_pIOCP;
		SOCKET             m_Sock;
		int                m_Af;
		volatile int       m_BClosing;
		volatile LONG      OutstandingRecv;
		volatile LONG      OutstandingSend;
		volatile LONG      PendingSend;
		CRITICAL_SECTION   SockCritSec;
		CRITICAL_SECTION   StatusCritSec;
		SOCKADDR_IN		   m_addrRemote;
		CSocket* next;
		ULONG           LastSendIssued, IoCountIssued, IOCompleted;
		CircularBuffer  m_iBuffer;
		CircularBuffer  m_oBuffer;
	private:
#ifdef TRANSMIT_FILE
		FTransfer* m_FileTransfer;
#endif
		volatile  bool   m_bEncrypt;
		PerIOData* OutOfOrderSends;
		IEncryptor* m_Encryptor;
		volatile SOCKET_MODE m_eMode;
		char			m_IP[NI_MAXHOST];
		char			m_PORT[NI_MAXSERV];
	};
}

#endif