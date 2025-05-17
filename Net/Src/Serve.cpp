#include "../inc/Serve.h"
#include "../inc/KernelImpl.h"
#include "..\..\Common\Include\Base\inc\SvrBase.h"
#include <tchar.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace serve
{
	CServe::CServe(IServe* serve, HMODULE hModule)
		: m_serve(serve), m_pThread(NULL), m_hModule(NULL)
	{
	}

	CServe::~CServe()
	{
		SAFE_DELETE(m_pThread);
		if (NULL != m_serve)
		{
			m_serve->Release();
		}
	}

	bool CServe::Run()

	{
		const DWORD dwServeInterval = m_serve->GetInterval();

		m_pThread = sbase::CThread::CreateNew(*this, 0, dwServeInterval);
		ASSERT(m_pThread != NULL);
		return !!m_pThread;
	}
}