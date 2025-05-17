//========================================================
//    Copyright (c) 2006,福州天盟工作室
//    All rights reserved.
//
//    文件名称 ： IPCChannel.cpp
//    摘    要 ： 信道服务
//
//    当前版本 ： 1.0
//    作    者 ： 李锋军
//    完成日期 ： 2007-01-16
//========================================================

#include "..\inc\IPCChannel.h"
#include <stdio.h>

namespace ipc
{
	//---------------------------------------------------------
	CNamePipe::CNamePipe(TYPE ChannelType) :m_Start(false)
	{
		m_Type = ChannelType;
	}

	//---------------------------------------------------------
	CNamePipe::~CNamePipe()
	{
		Finalize();
	}

	//---------------------------------------------------------
	//接受连接
	//---------------------------------------------------------
	bool CNamePipe::Accept()
	{
		//等待客户端连接
		if (ConnectNamedPipe(m_PipeHandle, NULL) == 0)
		{
 CloseHandle(m_PipeHandle);
 return false;
		}
		return true;
	}

	//---------------------------------------------------------
	//信道服务初始化
	//---------------------------------------------------------
	bool
		CNamePipe::Init(const char* cName)
	{
		memset(m_SendBuffer, 0L, sizeof(m_SendBuffer));
		memset(m_ReadBuffer, 0L, sizeof(m_ReadBuffer));

		if (NAMEPIPE_SERVER == m_Type)
		{
 if ((m_PipeHandle = CreateNamedPipe(cName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, PIPE_UNLIMITED_INSTANCES, 0, 0, 1000, NULL)) == INVALID_HANDLE_VALUE)
 {
 	return false;
 }
		}
		else
		{
 //检查管道实例
 if (WaitNamedPipe(cName, NMPWAIT_WAIT_FOREVER) == 0)
 {
 	m_Start = false;
 	return false;
 }

 if ((m_PipeHandle = CreateFile(cName, GENERIC_READ | GENERIC_WRITE, 0, (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL)) == INVALID_HANDLE_VALUE)
 {
 	m_Start = false;
 	return false;
 }
		}

		m_Start = true;
		return true;
	}

	//---------------------------------------------------------
	//从管道读出信息
	//---------------------------------------------------------
	void*
		CNamePipe::Read(int& iBytesRead)
	{
		DWORD dwBytesRead = 0;
		memset(m_ReadBuffer, 0L, BUF_SIZE);
		if (ReadFile(m_PipeHandle, m_ReadBuffer, sizeof(m_ReadBuffer), &dwBytesRead, NULL) <= 0)
		{
 int Error = GetLastError();
 if (Error == ERROR_BROKEN_PIPE || Error == ERROR_PIPE_NOT_CONNECTED)
 {
 	iBytesRead = -1;
 }
 return NULL;
		}
		else
 iBytesRead = dwBytesRead;
		return m_ReadBuffer;
	}

	//---------------------------------------------------------
	//向管道写入信息
	//---------------------------------------------------------
	void*
		CNamePipe::Write(int& iBytesSend)
	{
		DWORD iBytesRead = 0;
		if (WriteFile(m_PipeHandle, m_SendBuffer, m_size, &iBytesRead, NULL) == 0)
		{
 CloseHandle(m_PipeHandle);
 iBytesSend = -1;
 return NULL;
		}
		else
 iBytesSend = iBytesRead;
		return  m_SendBuffer;
	}

	bool
		CNamePipe::Finalize()
	{
		if (NAMEPIPE_SERVER == m_Type)
		{
 if (DisconnectNamedPipe(m_PipeHandle) == 0)
 {
 	printf("Disconnect failed!");
 	return false;
 }
 else
 {
 	printf("Client closed!\n");
 }
		}

		if (m_Start)
		{
 CloseHandle(m_PipeHandle);
 m_Start = false;
		}

		return true;
	}

	bool   CNamePipe::Refresh()
	{
		if (DisconnectNamedPipe(m_PipeHandle) == 0)
		{
 printf("Disconnect failed!\n");
 return false;
		}
		else
		{
 printf("Client closed!\n");
		}

		return true;
	}

	void
		CNamePipe::SetSendBuf(void* pSend, int size)
	{
		memset(m_SendBuffer, 0L, BUF_SIZE);
		memcpy(m_SendBuffer, pSend, size);
		m_size = size;
	}

	char* CNamePipe::GetRecvBuf()
	{
		return m_ReadBuffer;
	}
}