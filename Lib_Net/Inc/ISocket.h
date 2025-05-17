
#pragma once

#ifdef NET_EXPORTS
	#define NET_API extern "C" __declspec(dllexport)
#else
	#define NET_API extern "C" __declspec(dllimport)
#endif

#ifndef SOCKET
typedef unsigned int	SOCKET;
#endif

namespace net
{

	// ����������ö��
	enum ENCRYPTOR_TYPE{ ENCRYPTOR_ALL = 0, ENCRYPTOR_SND, ENCRYPTOR_RCV };

	//////////////////////////////////////////////////////////////////
	// ����������Ӧ���ڶ��̻߳����������Ͷ���������Խ��ѽ����ڴ����
	//////////////////////////////////////////////////////////////////
	class IEncryptor
	{
	public:
		// �ͷŴ˼������ӿ�
		virtual unsigned long	Release			(void)							= 0;

		// ��ָ�����ȼ�������
		virtual void			Encrypt			(unsigned char* buf, int nLen)	= 0;

		// ��ָ�����Ƚ�������	
		virtual void			Decrypt			(unsigned char* buf, int nLen)	= 0;
	
		// �˷������԰�ָ�����ȵĽ����������¼��ܣ��ָ�������ǰ��״̬
		virtual void			Rencrypt		(unsigned char* buf, int nLen)	= 0;

		// �ı����������
		virtual void			ChangeCode		(unsigned long ulCode)			= 0;
		virtual void			ChangeCode		(const char* pszKey)			= 0;

		// ����һ���µļ������ӿ�
		virtual IEncryptor*		Duplicate		(void)							= 0;
	};


	//////////////////////////////////////////////////////////////////
	// �ͻ���socket�¼��ӿڣ�Ӧ�ö���Ӧ�̳д˽ӿ�
	//////////////////////////////////////////////////////////////////
	class IClientSocketEvent
	{
	public:
		// ��������£��˺���Ӧ����һ��������ָ���ӽ�������������������ݴ�С��һ����Ǵ������Ϣ����С����
		// �������-1�����رմ�socket��
		virtual int				OnRcvMsg		(const void* buf, int nLen)		= 0;
		
		// socket���������ô˷�����
		virtual void			OnEstablishSck	(void)							= 0;

		// socket�ر�ʱ����ô˷�����
		virtual void			OnCloseSck		(void)							= 0;
	};


	//////////////////////////////////////////////////////////////////
	// �ͻ���socket�ӿڣ�Ӧ�ö���Ӧ������ӵ�д˽ӿڣ������ڵ���Process�Բ���IClientSocketEvent���á�
	//////////////////////////////////////////////////////////////////
	class IClientSocket
	{
	public:
		virtual unsigned long	Release			(void)							= 0;

		// ������Ϣ
		virtual bool			SendMsg			(const void* pBuf, int nSize)	= 0;

		// �˷����������Ե��ã���������IClientSocketEvent��
		virtual void			Process			(void)							= 0;

		// �ر�socket
		virtual void			Close			(void)							= 0;

		// ��ѯ������
		virtual IEncryptor*		QueryEncryptor	(ENCRYPTOR_TYPE nType)			= 0;
		
		// �Ļ���������ԭ���ļ��������Զ��ͷš�ע�������Ӧ����new�ķ�������
		virtual void			ChgEncryptor	(ENCRYPTOR_TYPE nType, IEncryptor* pEncryptor) = 0;
         
        //�õ������������ݴ�С
        virtual size_t          GetBufByteNum  (void)                           = 0;
	};


	//////////////////////////////////////////////////////////////////
	// ����socket�¼��ӿڣ�Ӧ�ö���Ӧ�̳д˽ӿ�
	//////////////////////////////////////////////////////////////////
	class IServeSocketEvent
	{
	public:
		// ��������£��˺���Ӧ����һ��������ָ���ӽ�������������������ݴ�С��һ����Ǵ������Ϣ����С����
		// �������-1�����رմ�socket��
		virtual int				OnRcvMsg		(SOCKET socket, const char* buf, int nLen)	= 0;
		
		// ����(listen)��socket�½���һ����socket�����ô˺���������false��ص���socket�������ᵼ�������onCloseSck���ã�
		virtual bool			OnAcceptSck		(SOCKET socket)					= 0;

		// �µ�socket���������ô˷�����
		virtual void			OnEstablishSck	(SOCKET socket)					= 0;

		// socket���ر�ʱ����ô˷�����
		virtual void			OnCloseSck		(SOCKET socket)					= 0;
	};


	//////////////////////////////////////////////////////////////////
	// ����socket�ӿڣ�Ӧ�ö���Ӧ������ӵ�д˽ӿڣ������ڵ���Process�Բ���IServeSocketEvent���á�
	//////////////////////////////////////////////////////////////////
	class IServeSocket
	{
	public:
		virtual unsigned long	Release			(void)							= 0;

		// ������Ϣ��socket
		virtual bool			SendMsg			(SOCKET socket, const void* pBuf, int nSize)	= 0;

		// �����Ե��ô˷���������IServeSocketEvent��
		virtual void			Process			(void)							= 0;

		// �ر�ָ����socket
		virtual bool			CloseSocket		(SOCKET socket)					= 0;

		// �����Ƿ�����µ�����
		virtual void			RefuseConnect	(bool bEnable = true)			= 0;

		// ȡ��ָ��socket��ip��ַ
		virtual const char*		GetSocketIP		(SOCKET socket)					= 0;

		// ȡ����socket������
		virtual int				GetSocketAmount	(void)							= 0;

		// ��ѯ������
		virtual IEncryptor*		QueryEncryptor	(SOCKET socket, ENCRYPTOR_TYPE nType) = 0;
		
		// �Ļ���������ԭ���ļ��������Զ��ͷš�ע�������Ӧ����new�ķ�������
		virtual void			ChgEncryptor	(SOCKET socket, ENCRYPTOR_TYPE nType, IEncryptor* pEncryptor) = 0;
	};


	//////////////////////////////////////////////////////////////////
	// ���ɿͻ���socket�ӿڣ�ʧ�ܷ���NULL��
	// ����������Ӧ��new���Ķ����������ʧ�ܣ������IEncryptor���Զ��ͷš�
	// ���������ͷ�(Release)�˽ӿ�ʱ���Զ��ͷš�
	// dwReconnectInterval��socket������ʱ���������λ���룻�����Ϊ�㣬IClientSocket�ᰴָ����ʱ������ԭ���Ĳ������½���socket���ӡ����Ϊ�㣬�򲻻��Զ�������
	//////////////////////////////////////////////////////////////////
	 IClientSocket* ClientSocketCreate(IClientSocketEvent& iSocketEvent, const char* pszIP, int nPort, unsigned long dwReconnectInterval , IEncryptor* pEncryptorSnd , IEncryptor* pEncryptorRcv );


	//////////////////////////////////////////////////////////////////
	// ���ɷ���socket�ӿڣ�ʧ�ܷ���NULL;
	// ����������Ӧ��new���Ķ����������ʧ�ܣ������IEncryptor���Զ��ͷš�
	// ���������ͷ�(Release)�˽ӿ�ʱ���Զ��ͷš�
	//////////////////////////////////////////////////////////////////
    IServeSocket* ServeSocketCreate(IServeSocketEvent& iSocketEvent, int nServePort,char* IP, IEncryptor* pEncryptorSnd = NULL, IEncryptor* pEncryptorRcv = NULL);
}
	