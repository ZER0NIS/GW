// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// �� Windows ͷ���ų�����ʹ�õ�����

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h > 
#include <mswsock.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <iostream>
#include "../Base/Inc\SvrBase.h"
using namespace std;

#pragma comment(lib,"Ws2_32.lib")
namespace snet
{
	#define MAX_COMPLETION_THREAD_COUNT 32 
	#define DEFAULT_OVERLAPPED_COUNT	8
	#define DEFAULT_BUFFER_SIZE			1024
	#define	BURST_ACCEPT_COUNT			50
	#define MAX_OVERLAPPED_ACCEPTS      500
	#define MAX_OVERLAPPED_SENDS		200
    #define MAX_OVERLAPPED_RECVS        200
	#define SINGLE_BUFF_SIZE			1024
	#define OP_ACCEPT					0                  
	#define OP_READ						1                  
	#define OP_WRITE					2 

}


