//	Analyst.h
//	效率分析模块
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
	const int ANALYST_MASK_INVALID	= 0;		// 无效
	const int ANALYST_MASK_TICKS	= 1;		// 统计ticks消耗
	const int ANALYST_MASK_SIZE		= 2;		// 统计size/bytes
	const int ANALYST_MASK_AMOUNT	= 4;		// 统计数量

	struct CriticalSection 
	{
		CRITICAL_SECTION handle;
		operator CRITICAL_SECTION* ()	{ return &handle; } 
		CriticalSection()				{ InitializeCriticalSection(&handle); }
		~CriticalSection()				{ DeleteCriticalSection(&handle); }
	};

	typedef struct 
	{
		I64			totalTicks;		// 总共ticks消耗
		I64			maxTicks;		// 最大ticks消耗
		I64			totalSize;		// 总共size/bytes
		I64			maxSize;		// 最大size/bytes
		I64			amount;			// 数量
		I64			countTicks;		// ticks 统计次数
		I64			countSize;		// size 统计次数
		I64			countAmount;	// amount 统计次数
		I64			mask;			// the mask of type
		char*		name;			// alloc/free by hand
	} Statis_t;

	// 允许最多分析数量
	const int ANALYST_MAX_ITEM = 100;
}

#endif //_Analyst_H_