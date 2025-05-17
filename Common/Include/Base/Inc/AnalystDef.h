//	Analyst.h
//	Ч�ʷ���ģ��
//

#ifndef _ANALYST_ANALYSTDEF_H_
#define _ANALYST_ANALYSTDEF_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#pragma warning(disable:4786)
namespace sbase
{
	const int ANALYST_MASK_INVALID	= 0;		// ��Ч
	const int ANALYST_MASK_TICKS	= 1;		// ͳ��ticks����
	const int ANALYST_MASK_SIZE		= 2;		// ͳ��size/bytes
	const int ANALYST_MASK_AMOUNT	= 4;		// ͳ������

	struct CriticalSection 
	{
		CRITICAL_SECTION handle;
		operator CRITICAL_SECTION* ()	{ return &handle; } 
		CriticalSection()				{ InitializeCriticalSection(&handle); }
		~CriticalSection()				{ DeleteCriticalSection(&handle); }
	};

	typedef struct 
	{
		I64			totalTicks;		// �ܹ�ticks����
		I64			maxTicks;		// ���ticks����
		I64			totalSize;		// �ܹ�size/bytes
		I64			maxSize;		// ���size/bytes
		I64			amount;			// ����
		I64			countTicks;		// ticks ͳ�ƴ���
		I64			countSize;		// size ͳ�ƴ���
		I64			countAmount;	// amount ͳ�ƴ���
		I64			mask;			// the mask of type
		char*		name;			// alloc/free by hand
	} Statis_t;

	// ��������������
	const int ANALYST_MAX_ITEM = 100;
}

#endif //_Analyst_H_