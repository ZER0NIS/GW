#include "stdafx.h"
#include "./PerIOData.h"

namespace cnet
{
	PerIOData::PerIOData(int Len)
	{
		buf = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BYTE) * Len);
		if (buf == NULL)
		{
 //fprintf(stderr, "GetBufferObj: HeapAlloc failed: %d\n", GetLastError());
 sbase::LogSave("Error", "HeapAlloc failed: %d,%s,%d", GetLastError(),
 	__FILE__, __LINE__);
 ExitProcess(0);
		}

		buflen = Len;
		addrlen = sizeof(addr);
		next = NULL;
	}

	PerIOData::~PerIOData()
	{
		HeapFree(GetProcessHeap(), 0, buf);
	}

	void* PerIOData::operator new (size_t size)
	{
		void* newobj = (void*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
		if (newobj == NULL)
		{
 //fprintf(stderr, "GetBufferObj: HeapAlloc failed: %d\n", GetLastError());
 sbase::LogSave("Error", "HeapAlloc failed: %d,%s,%d", GetLastError(),
 	__FILE__, __LINE__);
 ExitProcess(0);
		}

		return newobj;
	}

	void  PerIOData::operator delete(void* p)
	{
		if (!p)
 return;

		HeapFree(GetProcessHeap(), 0, p);
	}
}