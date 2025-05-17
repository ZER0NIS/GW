
#pragma once

#include "../../Lib_Base/inc/Types.h"
#include "..\..\Lib_Base\Inc\IMessage.h"
#include "../../Lib_Base/inc/IModule.h"

#ifdef KERNEL_EXPORTS
	#define KERNEL_API extern "C" __declspec(dllexport)
#else
	#define KERNEL_API extern "C" __declspec(dllimport)
#endif

namespace serve
{
	
	//////////////////////////////////////////////////////////////////////
	//enum PORT		{	PORT_INVALID = (USHORT) -1, PORT_SHELL = 0, PORT_AI, PORT_NETWORK, PORT_ACCOUNT, 
	//					PORT_RESERVED0, PORT_RESERVED1, PORT_RESERVED2, PORT_RESERVED3,PORT_RESERVED4,PORT_RESERVED5,
	//					PORT_GAME0, PORT_GAME1, PORT_GAME2, PORT_GAME3, PORT_GAME4, PORT_GAME5, PORT_GAME6, 
	//					PORT_ALL , };
	
	enum PORT {PORT_IN,PORT_OUT,PORT_SHELL,PORT_ALL};
	//////////////////////////////////////////////////////////////////////
	class IPort
	{
	public:
		// get port num
		virtual USHORT		GetPort		(void) const	= 0;
		
			
		// validation chk
		virtual bool		IsValid		(void) const	= 0;
		
		// take out a msg from msg port, u should release this msg by ur self
		virtual sbase::IMessage*	TakeMsg	(void)	= 0;
		
		// send a msg to specified msg port
		virtual bool		SendMsg		(sbase::IMessage& msg, int nPortDes) = 0;

		
		virtual sbase::IMessage*        TakeMsgFromOut(void) = 0;
		virtual bool                    SendMsgToOut(sbase::IMessage& msg, int nPortDes) = 0;


		virtual void		CleanMsg	(void)			= 0;
		virtual void		Stop		(void)			= 0;
		virtual void		Start		(void)			= 0;
		virtual void        DiscardMsg  (sbase::IMessage* pMsg)=0;
	};
	
	//////////////////////////////////////////////////////////////////////
	class IServe : public sbase::IModule
	{
	public:
		virtual USHORT		GetInterval	(void) const					= 0;
		virtual USHORT		GetPort		(void) const					= 0;
		virtual void		SendMsg		(sbase::IMessage& msg, int nPortDes)= 0;
		virtual int			OnProcess	(void)							= 0;
		virtual void        CloseSock   (SOCKET socket)                 = 0;
	};

	class IRouter
	{
	public:
		virtual serve::IPort*		QueryPort(ULONG usPort)	const	= 0;
	};
}

typedef serve::IServe*(*funServeCreate)(serve::IRouter* pRouter, USHORT usInterval);

