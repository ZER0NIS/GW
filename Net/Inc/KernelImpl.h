//========================================================
//
//    文件名称 ： KernelImpl.h
//    摘    要 ： 网络服务
//
//========================================================

#pragma once

#pragma warning(disable:4786)
#include <string>
#include <vector>
#include <map>
#include "IServe.h"
#include "IKernel.h"
#include "..\..\Common\Include\Base/inc/SyncObjs.h"
#include "..\..\Common\Include\Base/inc/Thread.h"

namespace serve
{
	class CServe;
	class CMsgRouter;
	class CKernelImpl : public IKernel
	{
		// --------------- Begin of define (Singleton Pattern) ---------------
	public:
		static CKernelImpl* GetInstance(void);
	private:
		static CKernelImpl* s_pKernel;

	protected:
		CKernelImpl();
		virtual ~CKernelImpl();
		// --------------- End of define (Singleton Pattern) ---------------

		// --------------- Implementation ---------------
	public:
		int b;
		CMsgRouter* m_pRouter;
		virtual USHORT	Release(void) { delete this; return 0; }
		virtual IPort* QueryShellPort(void) const;
		virtual void    CloseSocket(SOCKET socket);
		virtual bool	Init(void);

	protected:
		virtual bool	InitRouter(void);
		virtual bool	InitServe(void);

		virtual bool	RunServes();
		virtual void	DestroyServes();

	private:

		enum STATUS { STATUS_NONE, STATUS_READY, STATUS_NORMAL, STATUS_ERR, };
		STATUS			m_eStatus;

	protected:
		void			UpdateInitInfo(const char* pszInfo);

		// Register
	public:
		bool			RegisterServe(IServe* pServe, HMODULE hModule = NULL);
		bool			UnRegisterServe(CServe* pServe);

	private:
		typedef std::vector<CServe*>			SERVE_VEC;
		SERVE_VEC			m_setServe;
		typedef std::map<USHORT, CServe*>		SERVE_PORT_MAP;
		SERVE_PORT_MAP		m_setPortServe;

		typedef std::map<std::string, CServe*>	SERVE_NAME_MAP;
		SERVE_NAME_MAP		m_setNameServe;
	};
}
