//========================================================
//
//    Copyright (c) 2006,欢乐连线工作室
//    All rights reserved.
//
//    文件名称 ： GameServer.cpp
//    摘    要 ： 游戏服务器基础框架
//
//    替换版本 ： 1.00
//    作    者 ： 林德辉
//    完成日期 ： 2007-01-08
//
//    当前版本 ： 2.00
//    作    者 ： Fenjune Li
//    完成日期 ： 2008-06-15
//
//========================================================

#include "stdafx.h"
#include "World.h"
#include "GSMaster.h"
#include <MMSystem.h>
#pragma  comment(lib, "winmm.lib")

int _tmain(int, _TCHAR*)
{
	try {
		GSMaster GameServer;

		if (!GameServer.Init())
			return -1;

		// Control de tiempo para limitar uso de CPU
		DWORD lastFrameTime = timeGetTime();
		DWORD frameTime;
		const DWORD TARGET_FRAME_TIME = 20; // 50 FPS m醲imo (20ms)

		while (!CWorld::m_stopEvent)
		{
			GameServer.Run();

			// Limitar el uso de CPU dando tiempo a otros procesos
			frameTime = timeGetTime() - lastFrameTime;
			if (frameTime < TARGET_FRAME_TIME)
			{
				Sleep(TARGET_FRAME_TIME - frameTime);
			}
			lastFrameTime = timeGetTime();
		}

		// 释放资源
		GameServer.Release();
	}
	catch (const std::exception& ex) {
		std::cerr << "[EXCEPCI覰 C++] " << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (...) {
		std::cerr << "[EXCEPCI覰 DESCONOCIDA]" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}