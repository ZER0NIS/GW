#include "stdafx.h"
#include "PerIOData.h"

namespace snet
{
	PerIOData::PerIOData( int BufLen )
	{
 		m_Buf = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BYTE)*BufLen);
 		assert(m_Buf!=NULL);
 		if (m_Buf == NULL)
 		{
 sbase::SysLogSave("New PerIOData buffer failed %d!",BufLen);
 		}

		m_BufLen = BufLen;
		m_Next   = NULL;
		IoOrder  = 0;
//		m_AddrLen =  sizeof(m_Addr);
	}

	PerIOData::~PerIOData()
	{
		HeapFree(GetProcessHeap(), 0, m_Buf);
	}

   	void* PerIOData::operator new (size_t size)
   	{
   		return (void *)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );

   	}
   
   	void  PerIOData::operator delete(void* p)
   	{
   		if (!p) 
    return;
   
   		HeapFree(GetProcessHeap(), 0, p);
   	}

}