//========================================================
//
//    文件名称 ： KernelImpl.cpp
//    摘    要 ： 网络服务
//    
//========================================================

#include <tchar.h>
#include "..\inc\KernelImpl.h"
#include "..\inc\MsgRouter.h"
#include "..\inc\Serve.h"
#include "..\..\Lib_Base\inc\SvrBase.h"
#include "..\inc\NetServeImpl.h"
#include "..\..\Lib_Base\inc\Ini.h"
#include "..\Inc\KernelMsgDef.h"

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "winmm.lib" )



namespace serve
{

	CKernelImpl* 	CKernelImpl::s_pKernel = NULL;

   //----------------------------------------------------------------------------
	CKernelImpl*
		CKernelImpl::GetInstance(void)
	{
		if (NULL == s_pKernel)
			s_pKernel = new CKernelImpl();	

		return s_pKernel;
	}

   //----------------------------------------------------------------------------
	CKernelImpl::CKernelImpl()
		: m_eStatus(STATUS_NONE), m_pRouter(NULL)
	{
// 		const DWORD dwServeInterval = 1000;
// 		m_pThread = NULL;
// 		m_pThread = sbase::CThread::CreateNew(*this, sbase::CThread::RUN, dwServeInterval);
		b = 10;
	}

	
	bool	
		CKernelImpl::Init(void)
	{
		bool bRet = InitRouter();
		bRet &= InitServe();
		return bRet;
	}


	//----------------------------------------------------------------------------

	CKernelImpl::~CKernelImpl()
	{
		SERVE_VEC::iterator itor = m_setServe.begin();
		for (; itor != m_setServe.end(); itor++)
		{
			CServe* pServe = (*itor);
			if (pServe)
			{
				UnRegisterServe(pServe);
				SAFE_RELEASE(pServe);
			}
		}

		m_setServe.clear();
		m_setPortServe.clear();
		m_setPortServe.clear();
		SAFE_RELEASE(m_pRouter);

//		SAFE_DELETE(m_pThread);
	}

	//----------------------------------------------------------------------------
	bool
		CKernelImpl::InitRouter(void)
	{
		if (m_eStatus != STATUS_NONE)
			return true;

		// Init MsgRouter
		m_pRouter = CMsgRouter::CreateNew();
		IF_NOT (m_pRouter)
			return false;

		char szMsg[512] = "";
		sprintf_s(szMsg, "Initialize MsgRouter success!\r\n");
		this->UpdateInitInfo(szMsg);

		m_eStatus = STATUS_READY;
		return true;
	}


    //----------------------------------------------------------------------------
	bool
		CKernelImpl::InitServe(void)
	{
		//20070106

		if (STATUS_READY != m_eStatus || NULL == m_pRouter)
			return false;

		//sbase::CIni ini("config.ini", true);
		//USHORT usInterval	= ini.GetData("Serve","Interval");



		//消息接收线程
		IServe* pServe		= ns::ServeCreate(m_pRouter, 10, ns::RECEIVE);
		if (! pServe)
		{
				char szInfo[1024] = "";
				::sprintf_s(szInfo, "ServeCreate failed\n");
				this->UpdateInitInfo(szInfo);
				return false;
		}

		char szInfo[1024] = "";
		::sprintf_s(szInfo, "ServeCreate success!\n");
		this->UpdateInitInfo(szInfo);

		this->RegisterServe(pServe);

		// success
		m_eStatus = STATUS_NORMAL;


		//消息发送线程
		IServe* pSendServe		= ns::ServeCreate( m_pRouter, 10, ns::SEND );
		if (! pSendServe)
		{ 
			return false;
		}

		this->RegisterServe(pSendServe);
		// success
		m_eStatus = STATUS_NORMAL;


		/////////////////////////////////////////////////
		//消息发送线程
		//IServe* pSendServe1		= ns::ServeCreate( m_pRouter, 5, ns::SEND );
		//if (! pSendServe1)
		//{ 
		//	return false;
		//}

		//this->RegisterServe(pSendServe1);
		// success
		//m_eStatus = STATUS_NORMAL;
		////////////////////////////////////////////////

		//启动线程
		bool bet = RunServes();
		IF_NOT(bet)
			m_eStatus = STATUS_ERR;
		return bet;
	}

    //----------------------------------------------------------------------------
	bool	
		CKernelImpl::RunServes		()
	{
		SERVE_VEC::iterator it = m_setServe.begin();
		enum { S_PRECREATE, S_CREATE, S_RUN,};
		UCHAR uStatus = S_PRECREATE;
		bool bRet = true;
		std::string strErr;
		for (; it != m_setServe.end();)
		{
			CServe* pServe = *it;
			IF_NOT(pServe)
				return false;
			
			IServe* pIServe = pServe->QueryServe();
			IF_NOT(pIServe)
				return false;

			strErr = pIServe->GetName();
			if (uStatus == S_PRECREATE)
			{
				IF_NOT(pIServe->PreCreate())
				{
					strErr += "  -->Precreate Failed!!! ";
					bRet = false;
				}
				else
				{
					strErr += "   -->Precreate OK!!!";					
				}
			}
			else if (uStatus == S_CREATE)
			{
				IF_NOT(pIServe->Create())
				{
					bRet = false;
					strErr += "  -->Create Failed!!! ";
				}
				else
				{
					strErr += "  -->Create OK!!! ";
				}
			}
			else
			{
				IF_NOT(pServe->Run())
				{
					bRet = false;
					strErr += "  -->Run Failed!!! ";
				}
				else
				{
					strErr += "  -->Run OK!!! ";

				}
			}

			this->UpdateInitInfo(strErr.c_str());

			it++;

			if (it == m_setServe.end())
			{
				if (uStatus == S_PRECREATE)
				{
					uStatus = S_CREATE;
					it = m_setServe.begin();
				}
				else if (uStatus == S_CREATE)
				{
					uStatus = S_RUN;
					it = m_setServe.begin();
				}
				else
				{
					break;
				}
			}
		}

		return bRet;
	}

    //----------------------------------------------------------------------------
	void	
		CKernelImpl::DestroyServes	()
	{
		SERVE_VEC::iterator it = m_setServe.begin();
		enum { S_POSTDESTROY, S_DESTROY,};
		UCHAR uStatus = S_POSTDESTROY;
		std::string strErr;
		for (; it != m_setServe.end();)
		{
			CServe* pServe = *it;
			IServe* pIServe = pServe ? pServe->QueryServe() : NULL;
			IF_NOT(pServe)
			{
				goto next;
			}

			strErr = pIServe ? pIServe->GetName() : "Unknown Serve";

			if (uStatus == S_POSTDESTROY)
			{
				IF_OK(pIServe)
				{
					strErr += " PostDestoy!!!";
					pIServe->PostDestroy();
				}
			}
			else if (uStatus == S_DESTROY)
			{
				strErr += " Destroy!!!";
				pServe->Release();
				it = m_setServe.erase(it);
				continue;
			}

next:
			it++;

			if (it == m_setServe.end())
			{
				if (uStatus == S_POSTDESTROY)
				{
					uStatus = S_DESTROY;
					it = m_setServe.begin();
				}
				else
				{
					break;
				}
			}
		}
	}

	//----------------------------------------------------------------------------
	bool
		CKernelImpl::RegisterServe(IServe* pServe, HMODULE hModule)
	{
		IF_NOT (pServe)
			return false;

		CServe* pObjServe = new CServe(pServe, hModule);

		m_setPortServe.insert(SERVE_PORT_MAP::value_type(pObjServe->GetPort(), pObjServe));
		m_setNameServe.insert(SERVE_NAME_MAP::value_type(std::string(pObjServe->GetName()), pObjServe));

		m_setServe.push_back(pObjServe);
		return true;
	}

    //----------------------------------------------------------------------------
	IPort*	
		CKernelImpl::QueryShellPort(void) const
	{
		IF_NOT(m_pRouter)
			return NULL;

		return m_pRouter->QueryPort(PORT_SHELL);
	}


    //----------------------------------------------------------------------------
	bool
		CKernelImpl::UnRegisterServe(CServe* pServe)
	{
		IF_NOT (pServe)
			return false;

		SERVE_PORT_MAP::iterator itor = m_setPortServe.find(pServe->GetPort());
		if (itor == m_setPortServe.end())
			return false;

		m_setPortServe.erase(itor);

		m_setNameServe.erase(std::string(pServe->GetName()));
		return true;
	}

    //----------------------------------------------------------------------------
	void
		CKernelImpl::UpdateInitInfo(const char* pszInfo)
	{
		sbase::IMessage portMsg(sizeof(MSG_HEADER)+strlen(pszInfo), _KERNEL_MSG_SHELL_INIT, (char*)pszInfo, -1, -1);
		m_pRouter->DispatchMsg(PORT_SHELL, PORT_SHELL, portMsg);
	}

	//--------------------------------------------------------
	void
		CKernelImpl::CloseSocket(SOCKET socket)
	{
       IF_OK(m_setServe[0])
		   m_setServe[0]->CloseSock(socket);
	}

	//--------------------------------------------------------
	//网络服务创建
	//--------------------------------------------------------
	KERNEL_API IKernel*	
		serve::NetServiceCreate(serve::IPort** pPort,int * a)
	{
		CKernelImpl* pKernel = CKernelImpl::GetInstance();
		IF_NOT (pKernel)
			return NULL;

		IF_NOT ( pKernel->Init() )
		{
			SAFE_RELEASE(pKernel);
			return NULL;
		}

		(*pPort) = pKernel->m_pRouter->QueryPort(PORT_IN);
		a = &(pKernel->b);
		if (NULL == pPort)
		{
			return NULL;
		}
		::OutputDebugStr(_T("加载DLL成功!"));

		return pKernel;
	}

}