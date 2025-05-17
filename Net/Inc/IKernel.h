
#pragma once


#ifdef KERNEL_EXPORTS
#define KERNEL_API extern "C" __declspec(dllexport)
#else
#define KERNEL_API extern "C" __declspec(dllimport)
#endif


namespace serve
{
	class IPort;
	class IKernel
	{
	public:
		virtual USHORT	Release(void) 	= 0;
		virtual IPort*	QueryShellPort(void) const	= 0;
		virtual void    CloseSocket(SOCKET socket)    = 0;
	};

	KERNEL_API IKernel*	NetServiceCreate(serve::IPort** pPort,int * a);
}
