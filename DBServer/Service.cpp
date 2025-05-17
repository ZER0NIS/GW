#include "stdafx.h"
#include "Service.h"
#include "../Common/Include/Base/Inc/Ini.h"

#include "../Common/Include/sNet/IOCP.h"
#include "Master.h"

typedef ipc_np::CPipe* (*PipeServerCreate)(const char*);

PipeService::PipeService(DBService* DB) :m_Pipe(NULL), m_DBMgr(DB)
{
	InitializeCriticalSection(&m_CriSec);
}

PipeService::~PipeService()
{
	SAFE_DELETE(m_pThread);
	DeleteCriticalSection(&m_CriSec);

	m_Pipe->Release();
	FreeLibrary(m_PipeDll);
}

bool PipeService::Init()
{
	printf("Initialize PipeServer...\n");
	m_PipeDll = LoadLibrary("Pipe.dll");
	if (NULL == m_PipeDll)
	{
		return 0;
	}

	PipeServerCreate PipeServ = (PipeServerCreate)::GetProcAddress(m_PipeDll, "PipeServCreate");
	if (!PipeServ)
	{
		FreeLibrary(m_PipeDll);
		return false;
	}

	m_Pipe = PipeServ("IPC_BACKUP");

	if (m_Pipe == NULL)
		return false;

	if (!m_Pipe->Init())
	{
		printf("Initialize PipeServer Failed!\n");
		return false;
	}

	Start();

	m_pThread = sbase::CThread::CreateNew(*this, 0, 11);
	if (!m_pThread)
	{
		::MessageBox(NULL, "Create NetWorker thread fail.", "Error", MB_OK | MB_ICONERROR);

		return false;
	}

	return  true;
}

void PipeService::Stop()
{
}

void PipeService::Release()
{
}

void PipeService::Run()
{
	if (!IsOpen() || !m_Pipe->GetStatus())
	{
		return;
	}

	OnRead();

	if (m_QueSql.empty())
	{
		return;
	}

	string strSql = PopQueSql();

	if (!strSql.empty())
	{
		do
		{
			m_DBMgr->WriteToDB(strSql.c_str());

			if (m_QueSql.empty())
			{
				return;
			}

			strSql = PopQueSql();

			Sleep(10);
		} while (!strSql.empty());
	}
}

int PipeService::OnThreadProcess()
{
	m_Pipe->Recv(NULL);

	return 1;
}

const string PipeService::PopQueSql()
{
	EnterCriticalSection(&m_CriSec);

	string strSql = m_QueSql.front();
	m_QueSql.pop_front();

	LeaveCriticalSection(&m_CriSec);

	return strSql;
}

void PipeService::PushQueSql(string strSql)
{
	EnterCriticalSection(&m_CriSec);

	m_QueSql.push_back(strSql);

	LeaveCriticalSection(&m_CriSec);
}

bool PipeService::OnRead()
{
	string strRecv;

	BACKUP_MSG* pPipeMsg = NULL;

	char temp[4096] = { 0 };

	while (m_Pipe->ReadBuf((char**)&pPipeMsg))
	{
		memset(temp, 0, sizeof(temp));

		memcpy(temp, pPipeMsg->SQL, pPipeMsg->Head.usSize - sizeof(MsgHead));

		strRecv = temp;

#ifdef _DEBUG
		static int num = 0;

		memset(temp, 0, sizeof(temp));

		sprintf(temp, "%d ->Len:%d  Data:%s<-\n", ++num, pPipeMsg->Head.usSize, strRecv.c_str());

		printf(temp);
		fstream ofile;

		ofile.open("log.txt", ios_base::out | ios_base::app);

		ofile.write(temp, strlen(temp));

		ofile.close();

#endif
		PushQueSql(strRecv);

		m_Pipe->Remove(pPipeMsg->Head.usSize);
	}
	return true;
}

CommandService::CommandService(CMaster* pMaster) :lKeyCount(0), m_pMaster(pMaster)
{
	memset(strKeyBuffer, 0L, sizeof(strKeyBuffer));
}

CommandService::~CommandService()
{
}

bool CommandService::Init()
{
	m_Timer.Startup(COMMAND_TIMER);
	Start();

	printf("\nDBServer> ");
	return true;
}

void CommandService::Stop()
{
	IService::Stop();
}

void CommandService::Release()
{
}

void CommandService::Run()
{
	if (!IsOpen())
		return;

	if (m_Timer.IsExpire())
	{
		if (Command() == -1)
			exit(1);

		m_Timer.Update();
	}
}

long CommandService::ParseCommand(char* Argcommand)
{
	unsigned int x;
	size_t l = strlen(Argcommand);
	if (l < 2)
	{
		return 0;
	}

	for (x = 0; x < AllCmds; x++)
		if (!memcmp(Commands[x].cmd, Argcommand, Commands[x].cmdLen))
		{
			(this->*Commands[x].pfun)(&Argcommand[Commands[x].cmdLen], &::printf);
			break;
		}
	if (x == AllCmds)
	{
		sbase::ConsoleWriteColorText(FOREGROUND_RED, "		...Unknown command!\n ");
	}
	return 1;
}

long CommandService::Command(void)
{
	if (kbhit())
	{
		int key = getch();
		switch (key)
		{
		case 13:
		{
			if (lKeyCount == 0)
				break;

			ParseCommand(strKeyBuffer);
			lKeyCount = 0;
			memset(strKeyBuffer, 0, 256);
		}
		break;
		case 8:
		{
			if (lKeyCount > 0)
			{
				printf("\b \b");
				strKeyBuffer[lKeyCount] = 0;
				lKeyCount--;
			}
		}
		break;
		case 27:
		{
			lKeyCount = 0;
			memset(strKeyBuffer, 0, 256);
			printf("   ...[Cancel]\nCommand: ");
		}
		break;
		default:
		{
			if (lKeyCount >= 255)
			{
				break;
			}
			strKeyBuffer[lKeyCount] = (char)key;
			lKeyCount++;
			printf("%c", key);
		}
		break;
		}
	}

	return 1;
}

void  CommandService::Key_Help(char*, pFunPrintf pPrintf)
{
	printf("\n");
	for (int a = 0; a < AllCmds; a++)
	{
		printf("%-16s  %-20s%-25s\n", Commands[a].cmd, "-", Commands[a].Note);
	}
	pPrintf("\nDBServer> ");
}

void CommandService::Key_ShutdownServ(char* command, pFunPrintf pPrintf)
{
	time_t time = 0;
	char* StrTime;
	int x = 0;
	while (command[x] == ' ')
		x++;
	StrTime = &command[x];
	if (!strlen(StrTime))
	{
		m_pMaster->ShutDownServ(0);
		sbase::ConsoleWriteColorText(FOREGROUND_RED, "\nDB Server  shutdown immediately!\x0d\x0a", time);
		return;
	}

	time = atol(StrTime);
	if (time != 0)
	{
		m_pMaster->ShutDownServ(time);
		sbase::ConsoleWriteColorText(FOREGROUND_RED, "\nDB Server will shutdown in %d seconds!\x0d\x0a", time);
	}
	pPrintf("\nDBServer> ");
}

DBService::DBService()
{
}

DBService::~DBService()
{
}

bool DBService::Init()
{
	char szDBServer[64] = "";
	char szLoginName[64] = "";
	char szPassword[64] = "";
	char szDBName[64] = "";

	sbase::CIni ini("config.ini", true);
	strcpy(szDBServer, ini.GetString("Database", "DBServer"));
	strcpy(szLoginName, ini.GetString("Database", "LoginName"));
	strcpy(szPassword, ini.GetString("Database", "Password"));
	strcpy(szDBName, ini.GetString("Database", "DBName"));

	for (int a = 0; a < MAX_THREAD_DB; a++)
	{
		m_pDB[a] = rade_db::DatabaseCreate(szDBServer, szLoginName, szPassword, szDBName);
		if (NULL == m_pDB[a])
		{
			return false;
		}
	}

	Start();

	return true;
}

rade_db::IDatabase* DBService::GetDBAddr()
{
	int Min = 0;
	size_t Num = m_pDB[0]->GetRequestSize();
	for (int a = 0; a < MAX_THREAD_DB; a++)
	{
		if (m_pDB[a]->GetRequestSize() < Num)
		{
			Min = a;
			Num = m_pDB[a]->GetRequestSize();
		}
	}
	return m_pDB[Min];
}

void DBService::Stop()
{
}

void DBService::Release()
{
	delete this;
}

void DBService::Run()
{
	if (!IsOpen())
		return;
}

bool  DBService::WriteToDB(const char* SQL)
{
	rade_db::IDatabase* pDB = GetDBAddr();

	if (pDB->ExecuteAsyncSQL(SQL, NULL, NULL))
	{
		return true;
	}
	else
	{
		return false;
	}
}

CBackService::CBackService(DBService* DB) :m_pSrvIocp(new snet::CIOCP()), m_DBMgr(DB)

{
	InitializeCriticalSection(&m_CriSec);
}

CBackService::~CBackService()
{
	SAFE_DELETE(m_pSrvIocp);

	SAFE_DELETE(m_pThread);
	DeleteCriticalSection(&m_CriSec);
}

bool CBackService::Init()
{
	if (!LoadNetService())
		return false;

	m_pThread = sbase::CThread::CreateNew(*this, 0, 10);
	if (m_pThread == NULL)
		return false;

	cout << "Initialize Successfully!\n" << endl;
	sbase::SetConsoleFontColor(FOREGROUND_INTENSITY);

	Start();

	return true;
}

void CBackService::Stop()
{
}

void CBackService::Release()
{
}

void CBackService::Run()
{
	if (!IsOpen())
	{
		return;
	}

	snet::CSocket* pNewSocket = m_pSrvIocp->PopNewConnect();

	while (NULL != pNewSocket)
	{
		assert(pNewSocket->m_pIOCP);

		CacheAccounts(pNewSocket);

		NewSocketProc(pNewSocket);

		pNewSocket = m_pSrvIocp->PopNewConnect();
	}

	if (m_QueSql.empty())
	{
		return;
	}

	string strSql = PopQueSql();

	if (!strSql.empty())
	{
		do
		{
			m_DBMgr->WriteToDB(strSql.c_str());

			if (m_QueSql.empty())
			{
				return;
			}

			strSql = PopQueSql();

			Sleep(1);
		} while (!strSql.empty());
	}
}

bool CBackService::LoadNetService()
{
	char szIP[32] = { 0 };
	char szPort[12] = { 0 };
	int  PreNew = 0;

	sbase::CIni _ini("config.ini", false);
	strcpy(szIP, _ini.GetString("System", "IP"));
	strcpy(szPort, _ini.GetString("System", "ListenPort"));
	PreNew = _ini.GetData("System", "MaxOnline");

	printf(">>Initialize Net...\n");
	if (!m_pSrvIocp->Init(szIP, szPort, 0, 1, 6, PreNew))
	{
		printf("Initialize Net Failed!\n");
		return false;
	}

	return true;
}

int CBackService::Process()
{
	OnRead();
	return 0;
}

void CBackService::NewSocketProc(snet::CSocket* pSocket)
{
	BACKUP_MSG pszBuff;
	memset(&pszBuff, 0L, sizeof(BACKUP_MSG));

	if (pSocket->IsValid())
	{
		while (pSocket->Read((char**)&pszBuff, sizeof(pszBuff)))
		{
			switch (pszBuff.Head.usType)
			{
			case _MSG_LOGIN:
			{
				MSG_PIPESERVER_INFO  Login_info;
				Login_info.Head.usSize = sizeof(MSG_PIPESERVER_INFO);
				Login_info.Head.usType = _MSG_LOGIN_RETURN_INFO;

				Login_info.ucInfo = 1;

				SendMsg(&Login_info, pSocket, Login_info.Head.usSize);
				pSocket->Write();
				printf("DB Client Connected!!");
			}
			break;
			default:
				return;
			}

			memset(&pszBuff, 0L, sizeof(BACKUP_MSG));
		}
	}
	else
	{
		m_pSrvIocp->PushNewClose(pSocket);
	}
}

void CBackService::OnRead()
{
	snet::CSocket* pSocket = NULL;
	BACKUP_MSG pszBuff;
	memset(&pszBuff, 0L, sizeof(BACKUP_MSG));

	sbase::CSingleLock xLock(&m_xLock);

	SESSION_VEC::iterator itor = m_AccountsSet.begin();
	while (itor != m_AccountsSet.end())
	{
		pSocket = static_cast<snet::CSocket*>(*itor);

		if (pSocket->IsValid())
		{
			while (pSocket->Read((char**)&pszBuff, sizeof(pszBuff)))
			{
				ReslovePacket(&pszBuff, pSocket);

				memset(&pszBuff, 0L, sizeof(BACKUP_MSG));
			}
		}
		else
		{
			pSocket->GetIOCP()->PushNewClose(pSocket);
			m_AccountsSet.erase(itor);
			break;
		}
		itor++;
	}
}

void CBackService::OnWrite()
{
	snet::CSocket* pSocket = NULL;

	SESSION_VEC::iterator itor = m_AccountsSet.begin();
	while (itor != m_AccountsSet.end())
	{
		pSocket = static_cast<snet::CSocket*>(*itor);

		if (pSocket->IsValid())
		{
			pSocket->Write();
		}
		else
		{
			pSocket->GetIOCP()->PushNewClose(pSocket);
			m_AccountsSet.erase(itor);
			break;
		}
		itor++;
	}
}

void CBackService::ReslovePacket(const void* pPacket, snet::CSocket* pSocket)
{
	string strRecv;

	char temp[4096] = { 0 };

	BACKUP_MSG* pszBuff = (BACKUP_MSG*)pPacket;

	switch (pszBuff->Head.usType)
	{
	case 	TYPE_ROLE_ATTRIBUTE:
	case 	TYPE_ROLE_ACTIVESKILL:
	case 	TYPE_ROLE_PASSIVESKILL:
	case 	TYPE_ROLE_KITBAG1:
	case 	TYPE_ROLE_KITBAG2:
	case 	TYPE_ROLE_KITBAG3:
	case 	TYPE_ROLE_KITBAG4:
	case 	TYPE_ROLE_EQUIP:
	case 	TYPE_ROLE_STORAGE:
	case 	TYPE_ROLE_QUEST:
	case 	TYPE_ROLE_FRIEND:
	case 	TYPE_ROLE_BLACKLIST:
	case 	TYPE_ROLE_CONSORTIA:
	case 	TYPE_ROLE_ALTAR:
	case 	TYPE_MGR_CONSORTIA:
	{
		memset(temp, 0, 4096);
		memcpy(temp, pszBuff->SQL, pszBuff->Head.usSize - sizeof(MsgHead));
		strRecv = temp;
		PushQueSql(strRecv);

#ifdef _DEBUG_
		static int num = 0;

		sprintf(temp, "[%d] ->Len:%d  Data:%s<-\n", ++num, pszBuff->Head.usSize, strRecv.c_str());

		fstream ofile;

		ofile.open("log.txt", ios_base::out | ios_base::app);

		ofile.write(temp, strlen(temp));

		ofile.close();
#endif

		break;
	}

	default:
		break;
	}
}

void CBackService::SendMsg(void* buf, void* pSocket, int nLen) const
{
	snet::CSocket* pSendSocket = static_cast<snet::CSocket*>(pSocket);
	pSendSocket->PackMsg((char*)buf, nLen);
}

void CBackService::CacheAccounts(void* socket)
{
	sbase::CSingleLock xLock(&m_xLock);
	m_AccountsSet.push_back(socket);
}

bool CBackService::FindAccount(void* socket)
{
	SESSION_VEC::iterator itor = m_AccountsSet.begin();
	sbase::CSingleLock xLock(&m_xLock);
	while (itor != m_AccountsSet.end())
	{
		if (*itor == socket)
		{
			return true;
		}
		itor++;
	}

	return false;
}

void CBackService::ClearAccount(void* socket)
{
	SESSION_VEC::iterator itor = m_AccountsSet.begin();
	sbase::CSingleLock xLock(&m_xLock);
	while (itor != m_AccountsSet.end())
	{
		if (*itor == socket)
		{
			m_AccountsSet.erase(itor);
			return;
		}
		itor++;
	}
}

const string CBackService::PopQueSql()
{
	EnterCriticalSection(&m_CriSec);

	string strSql = m_QueSql.front();
	m_QueSql.pop_front();

	LeaveCriticalSection(&m_CriSec);

	return strSql;
}

void CBackService::PushQueSql(string strSql)
{
	EnterCriticalSection(&m_CriSec);

	m_QueSql.push_back(strSql);

	LeaveCriticalSection(&m_CriSec);
}