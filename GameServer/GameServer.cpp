//========================================================
//
//    Copyright (c) 2006,�������߹�����
//    All rights reserved.
//
//    �ļ����� �� GameServer.cpp
//    ժ    Ҫ �� ��Ϸ�������������
//
//    �滻�汾 �� 1.00
//    ��    �� �� �ֵ»�
//    ������� �� 2007-01-08
//
//    ��ǰ�汾 �� 2.00
//    ��    �� �� Fenjune Li
//    ������� �� 2008-06-15
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
		const DWORD TARGET_FRAME_TIME = 20; // 50 FPS m�ximo (20ms)

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

		// �ͷ���Դ
		GameServer.Release();
	}
	catch (const std::exception& ex) {
		std::cerr << "[EXCEPCI�N C++] " << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (...) {
		std::cerr << "[EXCEPCI�N DESCONOCIDA]" << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}