
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

	// 加密器类型枚举
	enum ENCRYPTOR_TYPE{ ENCRYPTOR_ALL = 0, ENCRYPTOR_SND, ENCRYPTOR_RCV };

	//////////////////////////////////////////////////////////////////
	// 加密器，如应用于多线程环境，此类型对象必须由自建堆进行内存管理
	//////////////////////////////////////////////////////////////////
	class IEncryptor
	{
	public:
		// 释放此加密器接口
		virtual unsigned long	Release			(void)							= 0;

		// 按指定长度加密数据
		virtual void			Encrypt			(unsigned char* buf, int nLen)	= 0;

		// 按指定长度解密数据	
		virtual void			Decrypt			(unsigned char* buf, int nLen)	= 0;
	
		// 此方法可以把指定长度的解密数据重新加密，恢复到解密前的状态
		virtual void			Rencrypt		(unsigned char* buf, int nLen)	= 0;

		// 改变加密器编码
		virtual void			ChangeCode		(unsigned long ulCode)			= 0;
		virtual void			ChangeCode		(const char* pszKey)			= 0;

		// 产生一个新的加密器接口
		virtual IEncryptor*		Duplicate		(void)							= 0;
	};


	//////////////////////////////////////////////////////////////////
	// 客户端socket事件接口，应用对象应继承此接口
	//////////////////////////////////////////////////////////////////
	class IClientSocketEvent
	{
	public:
		// 正常情况下，此函数应返回一个正整数指明从接受数据区清除掉的数据大小（一般就是处理的消息包大小）；
		// 如果返回-1，则会关闭此socket。
		virtual int				OnRcvMsg		(const void* buf, int nLen)		= 0;
		
		// socket建立后会调用此方法。
		virtual void			OnEstablishSck	(void)							= 0;

		// socket关闭时会调用此方法。
		virtual void			OnCloseSck		(void)							= 0;
	};


	//////////////////////////////////////////////////////////////////
	// 客户端socket接口，应用对象应产生，拥有此接口，并周期调用Process以产生IClientSocketEvent调用。
	//////////////////////////////////////////////////////////////////
	class IClientSocket
	{
	public:
		virtual unsigned long	Release			(void)							= 0;

		// 发送消息
		virtual bool			SendMsg			(const void* pBuf, int nSize)	= 0;

		// 此方法需周期性调用，用于驱动IClientSocketEvent。
		virtual void			Process			(void)							= 0;

		// 关闭socket
		virtual void			Close			(void)							= 0;

		// 查询加密器
		virtual IEncryptor*		QueryEncryptor	(ENCRYPTOR_TYPE nType)			= 0;
		
		// 改换加密器，原来的加密器会自动释放。注意加密器应该用new的方法参数
		virtual void			ChgEncryptor	(ENCRYPTOR_TYPE nType, IEncryptor* pEncryptor) = 0;
         
        //得到缓冲区中数据大小
        virtual size_t          GetBufByteNum  (void)                           = 0;
	};


	//////////////////////////////////////////////////////////////////
	// 服务socket事件接口，应用对象应继承此接口
	//////////////////////////////////////////////////////////////////
	class IServeSocketEvent
	{
	public:
		// 正常情况下，此函数应返回一个正整数指明从接受数据区清除掉的数据大小（一般就是处理的消息包大小）；
		// 如果返回-1，则会关闭此socket。
		virtual int				OnRcvMsg		(SOCKET socket, const char* buf, int nLen)	= 0;
		
		// 监听(listen)的socket新接受一个新socket后会调用此函数，返回false会关掉此socket。（不会导致下面的onCloseSck调用）
		virtual bool			OnAcceptSck		(SOCKET socket)					= 0;

		// 新的socket建立后会调用此方法。
		virtual void			OnEstablishSck	(SOCKET socket)					= 0;

		// socket被关闭时会调用此方法。
		virtual void			OnCloseSck		(SOCKET socket)					= 0;
	};


	//////////////////////////////////////////////////////////////////
	// 服务socket接口，应用对象应产生，拥有此接口，并周期调用Process以产生IServeSocketEvent调用。
	//////////////////////////////////////////////////////////////////
	class IServeSocket
	{
	public:
		virtual unsigned long	Release			(void)							= 0;

		// 发送消息到socket
		virtual bool			SendMsg			(SOCKET socket, const void* pBuf, int nSize)	= 0;

		// 周期性调用此方法以驱动IServeSocketEvent。
		virtual void			Process			(void)							= 0;

		// 关闭指定的socket
		virtual bool			CloseSocket		(SOCKET socket)					= 0;

		// 开关是否接受新的连接
		virtual void			RefuseConnect	(bool bEnable = true)			= 0;

		// 取得指定socket的ip地址
		virtual const char*		GetSocketIP		(SOCKET socket)					= 0;

		// 取所有socket的数量
		virtual int				GetSocketAmount	(void)							= 0;

		// 查询加密器
		virtual IEncryptor*		QueryEncryptor	(SOCKET socket, ENCRYPTOR_TYPE nType) = 0;
		
		// 改换加密器，原来的加密器会自动释放。注意加密器应该用new的方法参数
		virtual void			ChgEncryptor	(SOCKET socket, ENCRYPTOR_TYPE nType, IEncryptor* pEncryptor) = 0;
	};


	//////////////////////////////////////////////////////////////////
	// 生成客户端socket接口，失败返回NULL；
	// 加密器参数应是new出的对象，如果函数失败，带入的IEncryptor会自动释放。
	// 加密器在释放(Release)此接口时会自动释放。
	// dwReconnectInterval是socket重连的时间参数，单位毫秒；如果不为零，IClientSocket会按指定的时间间隔按原来的参数重新建立socket连接。如果为零，则不会自动重连。
	//////////////////////////////////////////////////////////////////
	 IClientSocket* ClientSocketCreate(IClientSocketEvent& iSocketEvent, const char* pszIP, int nPort, unsigned long dwReconnectInterval , IEncryptor* pEncryptorSnd , IEncryptor* pEncryptorRcv );


	//////////////////////////////////////////////////////////////////
	// 生成服务socket接口，失败返回NULL;
	// 加密器参数应是new出的对象，如果函数失败，带入的IEncryptor会自动释放。
	// 加密器在释放(Release)此接口时会自动释放。
	//////////////////////////////////////////////////////////////////
    IServeSocket* ServeSocketCreate(IServeSocketEvent& iSocketEvent, int nServePort,char* IP, IEncryptor* pEncryptorSnd = NULL, IEncryptor* pEncryptorRcv = NULL);
}
	