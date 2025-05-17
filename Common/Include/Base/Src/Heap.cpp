#include "..\Inc\heap.h"
#include "..\inc\SvrBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace sbase
{
	CHeap::CHeap(const char* pszClassName)
	{
		m_hHeap = ::HeapCreate(0, 0, 0);

		m_lNumAlloc = 0;
		m_lMaxAlloc = 0;

		if (pszClassName)
 m_strName = pszClassName;
	}

	//////////////////////////////////////////////////////////////////////
	CHeap::~CHeap()
	{
		if (m_hHeap)
 ::HeapDestroy(m_hHeap);

		if (m_lNumAlloc != 0)
		{
 if (!m_strName.empty())
 	sbase::SysLogSave("Class [%s] have [%d] memory which did not release", m_strName.c_str(), m_lNumAlloc);
 else
 	sbase::SysLogSave("Have [%d] memory which did not release", m_lNumAlloc);
		}
	}

	//////////////////////////////////////////////////////////////////////
	void*
		CHeap::Alloc(size_t size)
	{
		if (!m_hHeap)
 return NULL;

		if (size == 0)
 return false;

		// alloc mem for new obj
		void* p = ::HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, size);
		if (p)
		{
 ::InterlockedIncrement(&m_lNumAlloc);

 if (m_lNumAlloc > m_lMaxAlloc)
 	::InterlockedExchange(&m_lMaxAlloc, m_lNumAlloc);
		}

		return p;
	}

	//////////////////////////////////////////////////////////////////////
	void
		CHeap::Free(void* p)
	{
		if (!p)
 return;

		if (!m_hHeap)
 return;

		if (m_lNumAlloc < 0)
 sbase::SysLogSave("Memory amount of Class [%s] is error!", m_strName.c_str());

		// free it...
		if (::HeapFree(m_hHeap, 0, p))
		{
 ::InterlockedDecrement(&m_lNumAlloc);
		}
	}

	//////////////////////////////////////////////////////////////////////
	bool
		CHeap::IsValidPt(void* p)
	{
		if (!m_hHeap || !p)
 return false;

		// user address range from 1M--2G-64k
		if ((intptr_t)p < 0x00010000 || (intptr_t)p >= 0x7FFEFFFF)
 return false;

		if (::IsBadCodePtr((FARPROC)p))
 return false;

		if (::HeapValidate(m_hHeap, 0, p))
 return true;

		return false;
	}
}