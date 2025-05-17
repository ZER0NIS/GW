#include <windows.h>
#include <mmsystem.h>
#include "../inc/SysTimer.h"
#include <stdio.h>

namespace sbase
{
	CSysTimer& CSysTimer::Instance()
	{
		static CSysTimer objSysTimer;
		return objSysTimer;
	}

	CSysTimer::CSysTimer()
	{
		ticks_start = 0;
		hi_res_timer_available = false;
	}

	CSysTimer::~CSysTimer()
	{
	}

	bool CSysTimer::Sys_TickInit(bool bUseHighResolute/* = false*/)
	{
		// Detect the hardware is support high-resolution perfermance
		if (bUseHighResolute && QueryPerformanceFrequency(&hi_res_ticks_per_second) == TRUE)
		{
 hi_res_timer_available = true;
 QueryPerformanceCounter(&hi_res_start_ticks);
 FILE* fp = fopen("syslog\\system.txt", "w");
 if (fp)
 {
 	fprintf(fp, "CPU frequency:%u", hi_res_ticks_per_second.QuadPart);
 	fclose(fp);
 }
		}
		else
		{
 hi_res_timer_available = false;
 ticks_start = timeGetTime();
 return false;
		}
		return true;
	}

	I64 CSysTimer::Sys_GetTicks()
	{
		I64 ticks = 0;
		if (hi_res_timer_available) {
 LARGE_INTEGER hi_res_now;
 QueryPerformanceCounter(&hi_res_now);
 hi_res_now.QuadPart -= hi_res_start_ticks.QuadPart;
 ticks = (I64)(hi_res_now.QuadPart);
		}
		else {
 I64 now = timeGetTime();
 ticks = now - ticks_start;
		}

		return ticks;
	}

	I64 CSysTimer::Sys_TicksToMS(I64 ticks)
	{
		// Hi-resolution
		if (hi_res_timer_available) {
 LARGE_INTEGER largeTicks;
 largeTicks.QuadPart = ticks;
 largeTicks.QuadPart *= 1000;
 largeTicks.QuadPart /= hi_res_ticks_per_second.QuadPart;
 return (I64)largeTicks.QuadPart;
		}

		// Low resolution is ms already.
		return ticks;
	}

	I64 CSysTimer::Sys_Milliseconds()
	{
		if (hi_res_timer_available)
		{
 return Sys_TicksToMS(Sys_GetTicks());
		}

		return Sys_GetTicks();
	}
}