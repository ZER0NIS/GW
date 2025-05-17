
#pragma once

#include <windows.h>

//////////////////////////////////////////////////////////////////////

const int _MAX_MSGBUF	= 1024;

enum 
{	

	_KERNEL_MSG_BASE				= 0,
		
	// NetworkServe Msg
	_KERNEL_MSG_SCK_SWITCH			= _KERNEL_MSG_BASE+1,
	_KERNEL_MSG_SCK_CHGPORT,
	_KERNEL_MSG_SCK_CHGENCRYPTOR,
	_KERNEL_MSG_SCK_CLOSE,
	
	// GameServe Msg
	_KERNEL_MSG_CONNECT_SETUP		= _KERNEL_MSG_BASE+10,		
	_KERNEL_MSG_CONNECT_CLOSE,
	_KERNEL_MSG_CONNECT_CLOSEALL,

	// ShellServe Msg
	_KERNEL_MSG_SHELL_INIT			= _KERNEL_MSG_BASE+20,
	_KERNEL_MSG_SHELL_DB,
	_KERNEL_MSG_SHELL_SCK,
	_KERNEL_MSG_SHELL_LOGIN,
	_KERNEL_MSG_SHELL_MODULE,
	
	// AIServe Msg
	_KERNEL_MSG_AI_SETUP			= _KERNEL_MSG_BASE+30,		
	_KERNEL_MSG_AI_CLOSE,

	_KERNEL_MSG_GAME		= 1000,
};

//////////////////////////////////////////////////////////////////////
struct MSG_HEADER
{
	WORD	wSize;
	WORD	wType;
};


//////////////////////////////////////////////////////////////////////
// internal msg define
//////////////////////////////////////////////////////////////////////
struct MSG_INTERFACE_LOG
{
	char	szTxt[256];
};

struct MSG_LOGIN_INFO
{
	DWORD dwLogin;
	DWORD dwOnline;
};

struct MSG_DB_INFO
{
	DWORD dwTotalSQL;
	DWORD dwSQLPer5Min;
	DWORD dwSumPer5Min;
	DWORD dwAvgPer5Min;
	DWORD dwMaxPer5Min;
};

struct MSG_SCK_INFO
{
	DWORD dwBytesSnd;
	DWORD dwTotalBytesSnd;
	DWORD dwBytesRcv;
	DWORD dwTotalBytesRcv;
	DWORD dwSckPerAccept;
	DWORD dwSckPerClose;
	DWORD dwSckOnline;
	DWORD dwSckTotal;
};

struct MSG_SCK_CHGPORT
{
	SOCKET	socket;
	int		port;
};

struct MSG_SCK_CHGENCRYPTOR
{
	SOCKET	socket;
	int		nType;
	unsigned int uCode;
};

struct MSG_SCK_SWITCH
{
	bool	bRefuseConnect;
};

struct MSG_SCK_CLOSE
{
	SOCKET	socket;
};

struct MSG_CONNECT_SETUP
{
	SOCKET	socket;
};

struct MSG_CONNECT_CLOSE
{
	SOCKET	socket;
};

struct MSG_AI_SETUP
{
	SOCKET	socket;
};

struct MSG_AI_CLOSE
{
	SOCKET	socket;
};
