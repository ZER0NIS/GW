
#pragma once

#include "..\..\Lib_Base\inc\Thread.h"
#include "..\Inc\IServe.h"


namespace serve
{
	class CServe : private sbase::IThreadEvent
	{
		friend class CKernelImpl;
	public:
		CServe(IServe* serve, HMODULE hModule = NULL);
		virtual ~CServe();
		
		USHORT			Release	(void)				{ delete this; return 0; }
		USHORT			GetPort	(void)	const		{ return m_serve->GetPort(); }
		const char*		GetName(void) const		    { return m_serve->GetName(); }
		void            CloseSock   (SOCKET socket) { m_serve->CloseSock(socket);return ;};               
		bool			Run(void);

	protected:
		IServe*	m_serve;
		
		// -------------- interface of IThreadEvent ------------------
	protected:
		IServe*			QueryServe(void)const	{ return m_serve;}
		virtual int		OnThreadCreate	(void)	{ return 0; }
		virtual int		OnThreadDestroy	(void)	{ return 0; }
		virtual int		OnThreadEvent	(void)	{ return 0; }
		virtual int		OnThreadProcess	(void)	{ return m_serve->OnProcess(); }
		
	private:
		//消息接受线程
		sbase::CThread*	m_pThread;
		HMODULE			m_hModule;
	};

}
