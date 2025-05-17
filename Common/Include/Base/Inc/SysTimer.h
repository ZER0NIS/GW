//	SysTimer.h
//	高精度ticks，需要硬件支持，也许早期处理器不支持
//

#ifndef _SYSTIMER_H_
#define _SYSTIMER_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "types.h"

namespace sbase
{
	class CSysTimer
	{
	public:
		CSysTimer();
		virtual ~CSysTimer();

	public:
		static	CSysTimer& Instance();
	public:
		bool	Sys_TickInit(bool bUseHighResolute = false);
		I64		Sys_GetTicks();
		I64		Sys_TicksToMS(I64 ticks);
		I64		Sys_Milliseconds();				// Get the milliseconds from init
	private:
		// Low-resolution ticks value of the application
		I64 ticks_start;
		// Store if a high-resolution performance counter exists on the system
		bool hi_res_timer_available;
		// The first high-resolution ticks value of the application 
		LARGE_INTEGER hi_res_start_ticks;
		// The number of ticks per second of the high-resolution performance counter
		LARGE_INTEGER hi_res_ticks_per_second;
	};	
}

#endif //_SYSTIMER_H_