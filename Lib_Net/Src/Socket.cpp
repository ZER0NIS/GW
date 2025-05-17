
#include "..\inc\Socket.h"
#include "..\..\Lib_Base\inc\SvrBase.h"
#include <iostream>
using namespace std;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "winmm.lib" )
#pragma comment (lib,"WS2_32.lib")




BOOL APIENTRY DllMain( HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


namespace net
{
	// static
	bool CSocket::s_bInit = false;
	MYHEAP_IMPLEMENTATION(CSocket, s_heap)
		//////////////////////////////////////////////////////////////////////
		// Construction/Destruction
		//////////////////////////////////////////////////////////////////////
		CSocket::CSocket(SOCKET sock, IEncryptor* pEncryptorSnd, IEncryptor* pEncryptorRcv): m_iBuffer( RCV_BUFFER_SIZE )
		//: m_sock(sock), m_pEncryptorSnd(NULL), m_pEncryptorRcv(NULL)
	{
		m_sock = sock;
		m_pEncryptorSnd = NULL;
		m_pEncryptorRcv = NULL;
		m_nLen = 0;
		//memset(m_bufMsg, 0L, sizeof(m_bufMsg));

		if (pEncryptorSnd)
			m_pEncryptorSnd = pEncryptorSnd->Duplicate();

		if (pEncryptorRcv)
			m_pEncryptorRcv = pEncryptorRcv->Duplicate();
	}

	CSocket::~CSocket(void)
	{
		this->Close();

		SAFE_RELEASE (m_pEncryptorSnd);
		SAFE_RELEASE (m_pEncryptorRcv);
	}


	/////////////////////////////////////////////////////////////////////////////////////////
	bool 
		CSocket::Send(const void* buf, int nSize)
	{
		CONST ULONG _MAX_MSGBUF = 16*1024;
		if (!buf || nSize <= 0 || nSize >= _MAX_MSGBUF)
			return false;

		if ( INVALID_SOCKET == m_sock)
		{
			return false;
		}
		// 探测可写性
		FD_ZERO(&m_fdset);
		FD_SET(m_sock, &m_fdset);

		timeval timeout = { 0, 0 };
		if (SOCKET_ERROR == ::select(FD_SETSIZE, (fd_set *)NULL, &m_fdset, (fd_set *)NULL, &timeout))
		{
			CSocket::DumpError("CSocket::Send(), call ::select() error");

			this->Close();
			return false;
		}

		if (!FD_ISSET(m_sock, &m_fdset))
		{ // 不可写，关闭此socket
			this->Close();
			return false;
		}

		// 	// 加密消息
		// 	if (m_pEncryptorSnd)
		// 	{
		// 		char	bufEn[_MAX_MSGBUF];		//???
		// 		::memcpy(bufEn, buf, nSize);	//???
		// 
		// 		buf = bufEn;					//???
		// 		m_pEncryptorSnd->Encrypt((unsigned char*)buf, nSize);
		// 	}
		// 加密消息

		if (m_pEncryptorSnd)
		{
			//加密
			//m_pEncryptorSnd->Encrypt((unsigned char*)buf, nSize);
		}


		// 发送
		int ret = ::send(m_sock, (const char*)buf, nSize, 0);
		if (ret == nSize)
		{
			// sbase::DebugLogMsg("CSocket::Send() success，SOCKET[%d]!", this->Socket());
			//			printf("Socket[%d] Send\n",m_sock);
			return true;
		}
		else
		{	
			int err = CSocket::DumpError("CSocket::Send(), Client close socket exceptional");
			if (err != WSAEWOULDBLOCK)
			{
				printf("Socket[%d] close __Send\n",m_sock);
				this->Close();
				return false;
			}

			return true;
		}
	}

	bool  CSocket::ResolvePacket( ) 
	{
		//能取到包头
		if ( m_iBuffer.GetLength() >= 4 )
		{
			return true;
		}

		return false;
	}
	//////////////////////////////////////////////////////////////////////
	const char*
		CSocket::Recv(int& nLen, bool bDetectData/*= false*/)
	{
		IF_NOT (INVALID_SOCKET != m_sock)
			return NULL;

		if (bDetectData)
		{
			//fd_set readset;
			FD_ZERO(&m_fdset);
			FD_SET(m_sock, &m_fdset);

			timeval timeout = { 0, 0 };
			if (SOCKET_ERROR == ::select(FD_SETSIZE, &m_fdset, (fd_set *)NULL, (fd_set *)NULL, &timeout))
			{	// closed or network
				CSocket::DumpError("CSocket::Recv(), call ::select() error");

				//this->Close();
				return NULL;
			}

			if (!FD_ISSET(m_sock, &m_fdset))	// no data
			{
				nLen = m_iBuffer.GetLength();
				return m_iBuffer.GetStart();;
				//return m_bufMsg;
			}
		}

		// 接收
		nLen = 0;
		char buf[RCV_BUFFER_SIZE];
		int ret = ::recv(m_sock, buf, RCV_BUFFER_SIZE, 0);
		if (ret > 0)
		{// 解密消息

			//解密
			//if (m_pEncryptorRcv)
			  //m_pEncryptorRcv->Decrypt((unsigned char*)(buf), ret);


			m_nLen += ret;
			if ( !m_iBuffer.Write( buf, ret ) )
			{
				assert(0);
				::MessageBox( NULL, "网络通讯失败！0204", "错误", MB_OK );
				return NULL;
			}
		}
		else if (ret == 0)
		{
			sbase::DebugMsg("Client close the socket.");

			//this->Close();
			return NULL;
		}
		else
		{

			int err = CSocket::DumpError("CSocket::Recv(), Client close socket exceptional");
			if (err != WSAEWOULDBLOCK)
			{
				printf("Socket[%d] close __Recv\n",m_sock);
				this->Close();
				return NULL;
			}
		}

		nLen = m_iBuffer.GetLength();
		return m_iBuffer.GetStart();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	void 
		CSocket::ClrPacket	(int nLen)
	{
		if (nLen <= 0)
			return;

		if (nLen >= m_nLen)
			m_nLen = 0;
		else
		{
			//::memcpy(m_bufMsg, m_bufMsg + nLen, m_nLen - nLen);
			m_nLen -= nLen;
		}
    
       m_iBuffer.Remove( nLen );
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	void 
		CSocket::Close(void)
	{
		if (m_sock == INVALID_SOCKET)
			return;

		int err = ::closesocket(m_sock);
		if (SOCKET_ERROR == err)
		{
			CSocket::DumpError("CSocket::Close(), call ::closesocket() error");
		}

		m_sock = INVALID_SOCKET;
		m_nLen = 0;

		printf("Socket[%d] closed \n",m_sock);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	const char*	
		CSocket::GetPeerIP()
	{
		if (m_strIP.empty())
		{
			sockaddr_in	inAddr;
			::memset(&inAddr, 0, sizeof(inAddr));
			int		nLen = sizeof(inAddr);
			if (::getpeername(m_sock, (sockaddr *)&inAddr, &nLen))
			{
				CSocket::DumpError("CSocket::GetPeerIP(), call ::getpeername() error");

				this->Close();
				return NULL;
			}

			char *	pszIP = ::inet_ntoa(inAddr.sin_addr);
			if (!pszIP)
				return NULL;

			m_strIP = pszIP;
		}

		return m_strIP.c_str();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	int	
		CSocket::SetBufSize(int nSndBuf, int nRcvBuf)
	{
		// 设置SENDBUG
		int		optval = nSndBuf;
		if (nSndBuf && ::setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char *) &optval, sizeof(optval)))
		{
			CSocket::DumpError("CSocket::SetBufSize(), call ::setsockopt(snd buf) error");

			this->Close();
			return false;
		}

		// 设置RECVBUG
		optval = nRcvBuf;
		if (nRcvBuf && ::setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char *) &optval, sizeof(optval)))
		{
			CSocket::DumpError("CSocket::SetBufSize(), setsockopt, rcv buf");

			this->Close();
			return false;
		}

		return true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	bool 
		CSocket::SocketInit(void)
	{
		if (CSocket::s_bInit)
			return true;

		// 初始化网络
		WSADATA		wsaData;
		int ret = ::WSAStartup( MAKEWORD( 2,2 ), &wsaData);
		if (ret != 0)
		{
			sbase::LogSave("Net", "ERROR: Init WSAStartup() failed.");
			return false;
		}

		// 检查版本
		if (LOBYTE(wsaData.wVersion) != 0x02 || HIBYTE(wsaData.wVersion) != 0x02)
		{
			sbase::LogSave("Net", "ERROR: WSAStartup Version not match 2.0");
			return false;
		}

		CSocket::s_bInit = true;
		return true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	void 
		CSocket::SocketDestroy(void)
	{
		::WSACleanup();
		CSocket::s_bInit = false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	int 
		CSocket::DumpError(const char* pszInfo)
	{
		int err = WSAGetLastError();
		if (WSAEWOULDBLOCK != err)
			sbase::LogSave("Net", "DUMP: WSABASEERR+%d, from %s", err-WSABASEERR, pszInfo ? pszInfo : "");

		return err;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	IEncryptor*	
		CSocket::QueryEncryptor	(ENCRYPTOR_TYPE nType)
	{
		if (ENCRYPTOR_SND == nType)
			return m_pEncryptorSnd;
		else if (ENCRYPTOR_RCV == nType)
			return m_pEncryptorRcv;
		else
			return NULL;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	void 
		CSocket::ChgEncryptor(ENCRYPTOR_TYPE nType, IEncryptor* pEncryptor)
	{
		IF_NOT (pEncryptor)
			return;

		switch(nType)
		{
		case ENCRYPTOR_SND:
			if (m_pEncryptorSnd)
			{
				SAFE_RELEASE (m_pEncryptorSnd);
				m_pEncryptorSnd = pEncryptor->Duplicate();
			}
			break;

		case ENCRYPTOR_RCV:
			if (m_pEncryptorRcv)
			{
				// rencrypt the overflow buf


				SAFE_RELEASE (m_pEncryptorRcv);
				m_pEncryptorRcv = pEncryptor->Duplicate();

				// decrypt the overflow buf with the new decryptor
#ifdef _ENCRYPT_
				if (m_nLen > 0)
					m_pEncryptorRcv->Decrypt((unsigned char*)m_bufMsg, m_nLen);
#endif
			}
			break;

		default:
			{
				if (m_pEncryptorSnd)
				{
					SAFE_RELEASE (m_pEncryptorSnd);
					m_pEncryptorSnd = pEncryptor->Duplicate();
				}

				if (m_pEncryptorRcv)
				{
					// rencrypt the overflow buf
#ifdef _ENCRYPT_
					if (m_nLen > 0)
						m_pEncryptorRcv->Rencrypt((unsigned char*)m_bufMsg, m_nLen);
#endif

					SAFE_RELEASE (m_pEncryptorRcv);
					m_pEncryptorRcv = pEncryptor->Duplicate();

					// decrypt the overflow buf with the new decryptor
#ifdef _ENCRYPT_
					if (m_nLen > 0)
						m_pEncryptorRcv->Decrypt((unsigned char*)m_bufMsg, m_nLen);
#endif
				}
			}
			break;
		}
	}
}
