//========================================================
//
//    Copyright (c) 2006,欢乐连线工作室
//    All rights reserved.
//
//    文件名称 ： mempool.cpp
//    摘    要 ： 内存池
//
//    当前版本 ： 1.01
//    作    者 ： 李锋军
//    完成日期 ： 2007-05-30
//
//========================================================

#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <ctype.h>

#include "..\Inc\mempool.h"

#undef max
#define max(x,y) (((x)>(y)) ? (x) : (y))

MemoryPoolReportFunc_t CMemoryPool::g_ReportFunc = 0;

//-----------------------------------------------------------------------------
// 出错报告
//-----------------------------------------------------------------------------

void CMemoryPool::SetErrorReportFunc(MemoryPoolReportFunc_t func)
{
	g_ReportFunc = func;
}

//-----------------------------------------------------------------------------
// 构造
//-----------------------------------------------------------------------------

CMemoryPool::CMemoryPool(int blockSize, int numElements, int growMode)
{
	m_BlockSize = blockSize < sizeof(void*) ? sizeof(void*) : blockSize;
	m_BlocksPerBlob = numElements;
	m_GrowMode = growMode;
	Init();
	AddNewBlob();
}

//-----------------------------------------------------------------------------
// 析构
//-----------------------------------------------------------------------------
CMemoryPool::~CMemoryPool()
{
	if (m_BlocksAllocated > 0)
	{
		ReportLeaks();
	}
	Clear();
}

//-----------------------------------------------------------------------------
// 初始化内存池
//-----------------------------------------------------------------------------
void CMemoryPool::Init()
{
	m_NumBlobs = 0;
	m_BlocksAllocated = 0;
	m_pHeadOfFreeList = 0;
	m_BlobHead.m_pNext = m_BlobHead.m_pPrev = &m_BlobHead;
}

//-----------------------------------------------------------------------------
// 清理
//-----------------------------------------------------------------------------
void CMemoryPool::Clear()
{
	CBlob* pNext;
	for (CBlob* pCur = m_BlobHead.m_pNext; pCur != &m_BlobHead; pCur = pNext)
	{
		pNext = pCur->m_pNext;
		free(pCur);
	}
	Init();
}

//-----------------------------------------------------------------------------
// 报告内存泄漏
//-----------------------------------------------------------------------------

void CMemoryPool::ReportLeaks()
{
	if (!g_ReportFunc)
		return;

	g_ReportFunc("Memory leak: mempool blocks left in memory: %d\n", m_BlocksAllocated);

#ifdef _DEBUG
	// walk and destroy the free list so it doesn't intefere in the scan
	while (m_pHeadOfFreeList != NULL)
	{
		void* next = *((void**)m_pHeadOfFreeList);
		memset(m_pHeadOfFreeList, 0, m_BlockSize);
		m_pHeadOfFreeList = next;
	}

	g_ReportFunc("Dumping memory: \'");

	for (CBlob* pCur = m_BlobHead.m_pNext; pCur != &m_BlobHead; pCur = pCur->m_pNext)
	{
		// scan the memory block and dump the leaks
		char* scanPoint = (char*)pCur->m_Data;
		char* scanEnd = pCur->m_Data + pCur->m_NumBytes;
		bool needSpace = false;

		while (scanPoint < scanEnd)
		{
			// search for and dump any strings
			if (isprint(*scanPoint))
			{
				g_ReportFunc("%c", *scanPoint);
				needSpace = true;
			}
			else if (needSpace)
			{
				needSpace = false;
				g_ReportFunc(" ");
			}

			scanPoint++;
		}
	}

	g_ReportFunc("\'\n");
#endif // _DEBUG
}

//-----------------------------------------------------------------------------
// 添加
//-----------------------------------------------------------------------------
void CMemoryPool::AddNewBlob()
{
	int sizeMultiplier;

	if (m_GrowMode == GROW_SLOW)
	{
		sizeMultiplier = 1;
	}
	else if (m_GrowMode == GROW_NONE)
	{
		if (m_NumBlobs != 0)
		{
			return;
		}
	}

	sizeMultiplier = m_NumBlobs + 1;

	int nElements = m_BlocksPerBlob * sizeMultiplier;
	int blobSize = m_BlockSize * nElements;
	CBlob* pBlob = (CBlob*)malloc(sizeof(CBlob) + blobSize - 1);

	pBlob->m_NumBytes = blobSize;
	pBlob->m_pNext = &m_BlobHead;
	pBlob->m_pPrev = pBlob->m_pNext->m_pPrev;
	pBlob->m_pNext->m_pPrev = pBlob->m_pPrev->m_pNext = pBlob;

	m_pHeadOfFreeList = pBlob->m_Data;

	void** newBlob = (void**)m_pHeadOfFreeList;
	for (int j = 0; j < nElements - 1; j++)
	{
		newBlob[0] = (char*)newBlob + m_BlockSize;
		newBlob = (void**)newBlob[0];
	}

	// null terminate list
	newBlob[0] = NULL;
	m_NumBlobs++;
}

void* CMemoryPool::Alloc()
{
	return Alloc(m_BlockSize);
}

//-----------------------------------------------------------------------------
//分配
//-----------------------------------------------------------------------------
void* CMemoryPool::Alloc(unsigned int amount)
{
	sbase::CSingleLock lock(&m_cs);
	void* returnBlock;

	if (amount > (unsigned int)m_BlockSize)
		return NULL;

	if (!m_pHeadOfFreeList)
	{
		// returning NULL is fine in GROW_NONE
		if (m_GrowMode == GROW_NONE)
		{
			ASSERT(m_GrowMode == GROW_NONE);
			return NULL;
		}

		AddNewBlob();

		if (!m_pHeadOfFreeList)
		{
			return NULL;
		}
	}
	m_BlocksAllocated++;

	returnBlock = m_pHeadOfFreeList;

	ASSERT(m_pHeadOfFreeList);
	m_pHeadOfFreeList = *((void**)m_pHeadOfFreeList);

	return returnBlock;
}

//-----------------------------------------------------------------------------
//释放
//-----------------------------------------------------------------------------
void CMemoryPool::Free(void* memBlock)
{
	if (!memBlock)
		return;

#ifdef _DEBUG
	bool bOK = false;
	for (CBlob* pCur = m_BlobHead.m_pNext; pCur != &m_BlobHead; pCur = pCur->m_pNext)
	{
		if (memBlock >= pCur->m_Data && (char*)memBlock < (pCur->m_Data + pCur->m_NumBytes))
		{
			bOK = true;
		}
	}
#endif // _DEBUG

#ifdef _DEBUG
	memset(memBlock, 0xDD, m_BlockSize);
#endif

	sbase::CSingleLock lock(&m_cs);
	m_BlocksAllocated--;

	*((void**)memBlock) = m_pHeadOfFreeList;

	m_pHeadOfFreeList = memBlock;
}