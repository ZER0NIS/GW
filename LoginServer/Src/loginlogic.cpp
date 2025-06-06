﻿#include "..\Inc\stdafx.h"
#include "..\inc\LoginLogic.h"
#include "..\..\Common/Include/Base\Inc\IMessage.h"
#include "..\..\Common/Include/Base\inc\Ini.h"
#include "..\inc\Session.h"
#include "..\..\Common/Include/sNet\Socket.h"

#include <mmsystem.h>

#define   GODSWAR_TESTSWITCH_ON

using namespace std;

#define OUT_PUT(x) #x

enum STATE_TYPE { OFFLINE, NORMAL, BUSY, FULL };

namespace login
{
	char CLogin::ValidateCode[25];

	CLogin::CLogin() : m_pThread(nullptr)
	{
		sbase::CIni ini("config.ini", true);
		m_version = ini.GetData("System", "Version");

		//m_pThread = new sbase::CThread();

		m_pDB = NULL;
		m_GameServerInfo.clear();

		m_pSrvIocp = new snet::CIOCP();
	}

	bool CLogin::LoadDBSercice()
	{
		constexpr size_t BUFFER_SIZE = 128;
		char szDBServer[BUFFER_SIZE] = { 0 };
		char szLoginName[BUFFER_SIZE] = { 0 };
		char szPassword[BUFFER_SIZE] = { 0 };
		char szDBName[BUFFER_SIZE] = { 0 };

		sbase::CIni ini("config.ini", false);
		strncpy(szDBServer, ini.GetString("Database", "DBServer"), BUFFER_SIZE - 1);
		strncpy(szLoginName, ini.GetString("Database", "LoginName"), BUFFER_SIZE - 1);
		strncpy(szPassword, ini.GetString("Database", "Password"), BUFFER_SIZE - 1);
		strncpy(szDBName, ini.GetString("Database", "DBName"), BUFFER_SIZE - 1);

		// Asegurar que todas las cadenas estén terminadas correctamente
		szDBServer[BUFFER_SIZE - 1] = '\0';
		szLoginName[BUFFER_SIZE - 1] = '\0';
		szPassword[BUFFER_SIZE - 1] = '\0';
		szDBName[BUFFER_SIZE - 1] = '\0';

		m_pDB = rade_db::DatabaseCreate(szDBServer, szLoginName, szPassword, szDBName);
		if (!m_pDB) {
 printf("[ERROR] No se pudo conectar a la base de datos\n");
 return false;
		}

		return true;
	}
	CLogin::~CLogin()
	{
		SAFE_DELETE(m_pThread);
		SAFE_DELETE(m_pSrvIocp);

		GAMESERVER_SOCKET::iterator it = m_GameServerInfo.begin();
		for (; it != m_GameServerInfo.end(); it++)
		{
 SAFE_DELETE((*it).second);
		}
		m_GameServerInfo.clear();
		m_GameServerMap.clear();

		SAFE_RELEASE(m_pDB);
	}

	bool CLogin::CheckDB(const char* Name, const char* password, void* socket)
	{
		if (!m_pDB)
 return false;

		CSession* pSession = new CSession(Name, password, socket, *this);
		if (pSession != NULL)
		{
 printf("[DB] Creando nueva sesión para usuario: %s\n", Name);
 pSession->OnDBRequest();
 return true;
		}

		return false;
	}

	void  CLogin::Close()
	{
	}
	char* CLogin::DecryptionAccount(const char* account)
	{
		static char output[32] = { "\0" };
		static char* p = "435kfa3fjaf03fakf3fa3220fasjf3we322f2fjaf92f2fsf92fl1h1535rb7nw4\0";
		strcpy(output, account);
		int key = 0;
		int len = strlen(output);

		for (int i = 0; i < len; i++)
		{
 if (output[i] >= 'A' && output[i] <= 'Z')
 {
 	key = (p[len] % 26 + 26) % 26;
 	output[i] = (output[i] - 'A' - key + 26) % 26 + 'A';
 }
 else if (output[i] >= 'a' && output[i] <= 'z')
 {
 	key = (p[len] % 26 + 26) % 26;
 	output[i] = (output[i] - 'a' - key + 26) % 26 + 'a';
 }
 else if (output[i] >= '0' && output[i] <= '9')
 {
 	key = (p[len] % 10 + 10) % 10;
 	output[i] = (output[i] - '0' - key + 10) % 10 + '0';
 }
		}

		return output;
	}

	bool CLogin::LoadNetService()
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
		m_pSrvIocp->ShowStatus(true);
		return true;
	}

	int CLogin::Process()
	{
		snet::CSocket* pNewSocket = m_pSrvIocp->PopNewConnect();

		while (NULL != pNewSocket)
		{
 printf("[NET] Nueva conexión entrante\n");
 NewSocketProc(pNewSocket);
 pNewSocket = m_pSrvIocp->PopNewConnect();
		}

		OnRead();
		OnWrite();

		if (m_pDB)
		{
 m_pDB->CallBackAsyncSQL();
		}

		return 0;
	}

	bool CLogin::Run()
	{
		if (!LoadNetService())
 return false;

#ifdef GODSWAR_TESTSWITCH_ON
		if (!LoadDBSercice())
 return false;
#endif

		try {
 m_pThread = sbase::CThread::CreateNew(*this, 0, 10);
 if (m_pThread == nullptr)
 	return false;
		}
		catch (std::exception& e) {
 printf("Error al crear thread: %s\n", e.what());
 return false;
		}

		cout << "Initialize Successfully!\n" << endl;
		sbase::SetConsoleFontColor(FOREGROUND_INTENSITY);

		return true;
	}

	char* CLogin::CreateValidateCode()
	{
		int RanCheckNum = 0;
		char keytemp[5] = { 0 };
		memset(CLogin::ValidateCode, 0x30, 18);
		srand((unsigned)timeGetTime());
		for (int i = 0; i < 6; i++)
		{
 RanCheckNum = rand();
 _itoa(RanCheckNum, keytemp, 16);
 memcpy(&CLogin::ValidateCode[i * 4], keytemp, strlen(keytemp));
		}
		CLogin::ValidateCode[24] = 0x00;
		return CLogin::ValidateCode;
	}

	void CLogin::SendMsg(void* buf, void* pSocket, int nLen) const
	{
		snet::CSocket* pSendSocket = static_cast<snet::CSocket*>(pSocket);
		printf("[NET] Enviando paquete de tipo %d, tamaño %d\n", ((MsgHead*)buf)->usType, nLen);
		pSendSocket->PackMsg((char*)buf, nLen);
	}

	bool CLogin::JudgeValidStr(MSG_LOGIN* pMsgLogin, void* pSocket)
	{
		MSG_LOGIN_RETURN_INFO  Login_info;
		Login_info.Head.usSize = sizeof(MSG_LOGIN_RETURN_INFO);
		Login_info.Head.usType = _MSG_LOGIN_RETURN_INFO;

		for (int a = 0; a < (int)strlen(pMsgLogin->Name); a++)
		{
 if (!(isalpha(pMsgLogin->Name[a]) || isdigit(pMsgLogin->Name[a]) || pMsgLogin->Name[a] == '_'))
 {
 	printf("[AUTH] Nombre de usuario inválido: %s\n", pMsgLogin->Name);
 	Login_info.ucInfo = 0;
 	SendMsg(&Login_info, pSocket, Login_info.Head.usSize);
 	static_cast<snet::CSocket*>(pSocket)->Write();
 	return false;
 }
		}

		return true;
	}

	void CLogin::ReslovePacket(const void* pPacket, snet::CSocket* pSocket)
	{
		MsgHead* pszBuff = (MsgHead*)pPacket;
		printf("[NET] Paquete recibido - Tipo: %d, Tamaño: %d\n", pszBuff->usType, pszBuff->usSize);

		switch (pszBuff->usType)
		{
		case _MSG_GAMESERVER_INFO:
		{
 MSG_GAMESERVER_INFO* Server = (MSG_GAMESERVER_INFO*)pszBuff;
 printf("[GS] Info de servidor recibida - ID: %d, Estado: %d\n", Server->cID, Server->cState);
 switch (Server->cState)
 {
 case OFFLINE:
 case NORMAL:
 case BUSY:
 case FULL:
 {
 	GAMESERVER_ID::iterator itor = m_GameServerMap.find(Server->cID);
 	if (itor != m_GameServerMap.end())
 	{
 		MSG_GAMESERVER_INFO* pServer = (*itor).second;
 		pServer->cState = Server->cState;
 		printf("[GS] Actualizado estado del servidor %d a %d\n", Server->cID, Server->cState);
 	}
 	else
 	{
 		MSG_GAMESERVER_INFO* msg_Register = new MSG_GAMESERVER_INFO;
 		memcpy(msg_Register, pszBuff, sizeof(MSG_GAMESERVER_INFO));
 		m_GameServerInfo[pSocket] = msg_Register;
 		m_GameServerMap[msg_Register->cID] = msg_Register;
 		printf("[GS] Nuevo servidor registrado - ID: %d, Nombre: %s\n",
  msg_Register->cID, msg_Register->ServerName);
 	}
 }
 break;
 }
		}
		break;
		case _MSG_REQUEST_GAMESERVER:
		{
 MSG_REQUEST_GAMESERVER* Choose = (MSG_REQUEST_GAMESERVER*)pszBuff;
 printf("[GS] Solicitud de servidor de juego - Usuario: %s, Servidor ID: %d\n",
 	Choose->Name, Choose->cGameServerID);
 CSession* pPlayerSession = FindAccount(Choose->Name);
 if (NULL == pPlayerSession)
 {
 	printf("[GS] Error: Sesión no encontrada para usuario %s\n", Choose->Name);
 	break;
 }

 if (pPlayerSession->GetGSCheckFlag())
 {
 	printf("[GS] Error: Bandera GS ya activada para usuario %s\n", Choose->Name);
 	break;
 }

 GAMESERVER_ID::iterator serveritor;
 serveritor = m_GameServerMap.find(Choose->cGameServerID);
 if (serveritor != m_GameServerMap.end())
 {
 	MSG_GAMESERVER_INFO* temp = (*serveritor).second;
 	GAMESERVER_SOCKET::iterator servesocket = m_GameServerInfo.begin();
 	for (; servesocket != m_GameServerInfo.end(); servesocket++)
 	{
 		if ((*servesocket).second == temp)
 		{
  MSG_RESPONSE_GAMESERVER Serverok;
  Serverok.Head.usSize = sizeof(MSG_RESPONSE_GAMESERVER);
  Serverok.Head.usType = _MSG_RESPONSE_GAMESERVER;

  if (serveritor->second->cState == FULL)
  {
  	printf("[GS] Servidor %d lleno para usuario %s\n",
  		Choose->cGameServerID, Choose->Name);
  	Serverok.cGameServerID = 0;
  	Serverok.uiPort = 0;
  	strcpy(Serverok.cIP, "");
  	Serverok.cLoginError = 1;
  }
  else
  {
  	printf("[GS] Asignando servidor %d a usuario %s\n",
  		Choose->cGameServerID, Choose->Name);
  	Serverok.cGameServerID = temp->cID;
  	Serverok.uiPort = temp->uiPort;
  	strcpy(Serverok.cIP, temp->cIP);
  	Serverok.cLoginError = 0;
  	strcpy(Serverok.cCheckOutText, CreateValidateCode());

  	MSG_VALIDATE_GAMESERVER   gameserver_valid;
  	gameserver_valid.Head.usType = _MSG_VALIDATE_GAMESERVER;
  	gameserver_valid.Head.usSize = sizeof(MSG_VALIDATE_GAMESERVER);
  	strcpy(gameserver_valid.cCheckOutText, Serverok.cCheckOutText);
  	strcpy(gameserver_valid.Accounts, Choose->Name);

  	printf("[GS] Enviando validación al GS para usuario %s\n", Choose->Name);
  	SendMsg(&gameserver_valid, (*servesocket).first, gameserver_valid.Head.usSize);
  	pPlayerSession->GSCheckFlag();
  }
  break;
 		}
 	}
 }
		}
		break;
		case _MSG_VALIDATE_GAMESERVER:
		{
 MSG_VALIDATE_GAMESERVER* key = (MSG_VALIDATE_GAMESERVER*)pszBuff;
 printf("[GS] Validación recibida para usuario %s\n", key->Accounts);
 CSession* pPlayerSession = FindAccount(key->Accounts);
 if (NULL != pPlayerSession)
 {
 	pPlayerSession->GSCheckFlag();
 	GAMESERVER_SOCKET::iterator serveritor;
 	serveritor = m_GameServerInfo.find(pSocket);
 	if (serveritor != m_GameServerInfo.end())
 	{
 		MSG_RESPONSE_GAMESERVER Serverok;
 		Serverok.Head.usSize = sizeof(MSG_RESPONSE_GAMESERVER);
 		Serverok.Head.usType = _MSG_RESPONSE_GAMESERVER;
 		if (serveritor->second->cState == FULL)
 		{
  printf("[GS] Error: Servidor lleno al validar usuario %s\n", key->Accounts);
  Serverok.cGameServerID = 0;
  Serverok.uiPort = 0;
  strcpy(Serverok.cIP, "");
  Serverok.cLoginError = 1;
 		}
 		else
 		{
  printf("[GS] Validación exitosa para usuario %s\n", key->Accounts);
  Serverok.cGameServerID = serveritor->second->cID;
  Serverok.uiPort = serveritor->second->uiPort;
  strcpy(Serverok.cIP, serveritor->second->cIP);
  Serverok.cLoginError = 0;
  strcpy(Serverok.cCheckOutText, key->cCheckOutText);
 		}

 		SendMsg(&Serverok, (void*)pPlayerSession->GetSocket(), Serverok.Head.usSize);
 		pPlayerSession->AccountCheckFlag();
 	}
 }
		}
		break;
		case _MSG_BAN_PLAYER:
 printf("[ADMIN] Paquete de ban recibido\n");
 break;
		default:
 printf("[NET] Mensaje desconocido recibido - Tipo: %d\n", pszBuff->usType);
 sbase::LogSave("GS", "UnKnown Msg From Gs\n");
 break;
		}
	}

	void CLogin::NewSocketProc(snet::CSocket* pSocket)
	{
		char pszBuff[4096] = { 0 };
		if (pSocket->IsValid())
		{
 while (pSocket->Read((char**)pszBuff, 4096))
 {
 	printf("[NET] Procesando nuevo paquete de conexión - Tipo: %d\n", ((MsgHead*)pszBuff)->usType);
 	switch (((MsgHead*)pszBuff)->usType)
 	{
 	case _MSG_LOGIN:
 	{
 		MSG_LOGIN* msg_Login = (MSG_LOGIN*)pszBuff;
 		printf("[AUTH] Intento de login - Usuario: %s\n", msg_Login->Name);

 		char cAccount[ACCOUNTS_LENGTH] = { 0 };
 		strcpy(cAccount, DecryptionAccount(msg_Login->Name));
 		memset(msg_Login->Name, 0L, sizeof(msg_Login->Name));
 		strcpy(msg_Login->Name, cAccount);

 		/*if (msg_Login->fVersion != m_version)
 		{
  printf("[AUTH] Error: Versión incorrecta - Esperada: %.2f, Recibida: %.2f\n",
  	m_version, msg_Login->fVersion);
  MSG_LOGIN_RETURN_INFO  Login_info;
  Login_info.Head.usSize = sizeof(MSG_LOGIN_RETURN_INFO);
  Login_info.Head.usType = _MSG_LOGIN_RETURN_INFO;
  Login_info.ucInfo = 0xff;
  SendMsg(&Login_info, pSocket, Login_info.Head.usSize);
  pSocket->Write();
  break;
 		}*/

 		if (!JudgeValidStr(msg_Login, pSocket))
 		{
  printf("[AUTH] Error: Nombre de usuario inválido\n");
  break;
 		}

 		bool Isfind = true;

 		printf("[AUTH] Usuario %s enviado a DB para validación\n", msg_Login->Name);

 		//	sprintf(passwordsecond, "%s%s", m_MD5.MDString(PassWord), "");

 		if (CheckDB(cAccount, msg_Login->cPassWord, pSocket))
 		{
  Isfind = true;
 		}
 		else
 		{
  printf("[AUTH] Error al enviar usuario %s a DB\n", msg_Login->Name);
 		}
 	}
 	break;
 	case _MSG_GAMESERVER_INFO:
 	{
 		MSG_GAMESERVER_INFO* Server = (MSG_GAMESERVER_INFO*)pszBuff;
 		printf("[GS] Info de servidor recibida en nueva conexión - ID: %d, Estado: %d\n",
  Server->cID, Server->cState);
 		switch (Server->cState)
 		{
 		case OFFLINE:
 		case NORMAL:
 		case BUSY:
 		case FULL:
 		{
  GAMESERVER_ID::iterator itor = m_GameServerMap.find(Server->cID);
  if (itor != m_GameServerMap.end())
  {
  	MSG_GAMESERVER_INFO* pServer = (*itor).second;
  	pServer->cState = Server->cState;
  	printf("[GS] Actualizado estado del servidor %d a %d\n", Server->cID, Server->cState);
  }
  else
  {
  	MSG_GAMESERVER_INFO* msg_Register = new MSG_GAMESERVER_INFO;
  	memcpy(msg_Register, pszBuff, sizeof(MSG_GAMESERVER_INFO));
  	m_GameServerInfo[pSocket] = msg_Register;
  	m_GameServerMap[msg_Register->cID] = msg_Register;
  	printf("[GS] Nuevo servidor registrado - ID: %d, Nombre: %s\n",
  		msg_Register->cID, msg_Register->ServerName);
  }
 		}
 		break;
 		default:
  ASSERT(0);
  break;
 		}
 	}
 	break;
 	default:
 		printf("[NET] Mensaje desconocido en nueva conexión - Tipo: %d\n", ((MsgHead*)pszBuff)->usType);
 		return;
 	}

 	//    pSocket->Remove(pszBuff->usSize);
 }
		}
		else
		{
 printf("[NET] Conexión no válida, cerrando\n");
 m_pSrvIocp->PushNewClose(pSocket);
		}
	}

	void CLogin::OnRead()
	{
		snet::CSocket* pSocket = NULL;
		char pszBuff[4096] = { 0 };
		sbase::CSingleLock xLock(&m_xLock);

		//printf("[NET] Procesando lectura de sockets...\n");

		SESSION_MAP::iterator itor = m_AccountsMap.begin();
		while (itor != m_AccountsMap.end())
		{
 pSocket = static_cast<snet::CSocket*>(itor->second->GetSocket());

 if (!pSocket)
 {
 	itor++;
 	continue;
 }

 if (pSocket->IsValid())
 {
 	while (pSocket->Read((char**)&pszBuff, 4096))
 	{
 		printf("[NET] Paquete recibido de usuario %s\n", itor->second->GetAccount());
 		ReslovePacket(pszBuff, pSocket);

 		//    pSocket->Remove(pszBuff->usSize);
 	}
 }
 else
 {
 	printf("[NET] Socket no válido para usuario %s, cerrando\n", itor->second->GetAccount());
 	pSocket->GetIOCP()->PushNewClose(pSocket);
 	ClearAccount(itor->second->GetAccount());
 	break;
 }
 itor++;
		}

		GAMESERVER_SOCKET::iterator pos = m_GameServerInfo.begin();
		while (pos != m_GameServerInfo.end())
		{
 pSocket = pos->first;

 if (pSocket->IsValid())
 {
 	bool ret = pSocket->Read((char**)&pszBuff);
 	while (ret)
 	{
 		printf("[NET] Paquete recibido de GameServer ID: %d\n", pos->second->cID);
 		ReslovePacket(pszBuff, pSocket);

 		//pSocket->Remove(pszBuff->usSize);

 		ret = pSocket->Read((char**)&pszBuff);
 	}
 }
 else
 {
 	printf("[NET] Socket no válido para GameServer ID: %d, cerrando\n", pos->second->cID);
 	pSocket->GetIOCP()->PushNewClose(pSocket);

 	GAMESERVER_ID::iterator it = m_GameServerMap.find(pos->second->cID);
 	if (it != m_GameServerMap.end())
 	{
 		SAFE_DELETE((*it).second);
 		m_GameServerMap.erase(it);
 	}
 	m_GameServerInfo.erase(pos);
 	break;
 }

 pos++;
		}
	}

	void CLogin::OnWrite()
	{
		snet::CSocket* pSocket = NULL;
		sbase::CSingleLock xLock(&m_xLock);

		//printf("[NET] Procesando escritura de sockets...\n");

		SESSION_MAP::iterator itor = m_AccountsMap.begin();
		while (itor != m_AccountsMap.end())
		{
 pSocket = static_cast<snet::CSocket*>(itor->second->GetSocket());

 if (!pSocket)
 {
 	itor++;
 	continue;
 }

 if (pSocket->IsValid())
 {
 	//printf("[NET] Escribiendo datos para usuario %s\n", itor->second->GetAccount());
 	pSocket->Write();

 	if (itor->second->GetAccountFlag())
 	{
 		printf("[NET] Limpiando cuenta %s por bandera de cuenta\n", itor->second->GetAccount());
 		ClearAccount(itor->second->GetAccount());

 		break;
 	}
 }
 else
 {
 	printf("[NET] Socket no válido para usuario %s, cerrando\n", itor->second->GetAccount());
 	pSocket->GetIOCP()->PushNewClose(pSocket);
 	ClearAccount(itor->second->GetAccount());
 	break;
 }
 itor++;
		}

		GAMESERVER_SOCKET::iterator pos = m_GameServerInfo.begin();
		while (pos != m_GameServerInfo.end())
		{
 pSocket = pos->first;

 if (pSocket->IsValid())
 {
 	//	printf("[NET] Escribiendo datos para GameServer ID: %d\n", pos->second->cID);
 	pSocket->Write();
 }
 else
 {
 	printf("[NET] Socket no válido para GameServer ID: %d, cerrando\n", pos->second->cID);
 	pSocket->GetIOCP()->PushNewClose(pSocket);

 	GAMESERVER_ID::iterator it = m_GameServerMap.find(pos->second->cID);
 	if (it != m_GameServerMap.end())
 	{
 		SAFE_DELETE((*it).second);
 		m_GameServerMap.erase(it);
 	}
 	m_GameServerInfo.erase(pos);
 	break;
 }

 pos++;
		}
	}

	void CLogin::CacheAccounts(std::string StrName, CSession* pSession)
	{
		sbase::CSingleLock xLock(&m_xLock);
		printf("[SESSION] Cacheando nueva cuenta: %s\n", StrName.c_str());
		m_AccountsMap[StrName] = pSession;
	}

	CSession* CLogin::FindAccount(std::string StrName)
	{
		SESSION_MAP::iterator  itor = m_AccountsMap.find(StrName);
		if (itor != m_AccountsMap.end())
		{
 printf("[SESSION] Cuenta encontrada: %s\n", StrName.c_str());
 return itor->second;
		}

		printf("[SESSION] Cuenta NO encontrada: %s\n", StrName.c_str());
		return NULL;
	}

	void CLogin::ClearAccount(std::string StrName)
	{
		sbase::CSingleLock xLock(&m_xLock);

		SESSION_MAP::iterator  itor = m_AccountsMap.find(StrName);

		if (itor != m_AccountsMap.end())
		{
 printf("[SESSION] Limpiando cuenta: %s\n", StrName.c_str());
 SAFE_DELETE(itor->second);
 m_AccountsMap.erase(itor);
		}
	}

	void CLogin::ShowServerList()
	{
		printf("\n               ¡ï=============ÓÎ Ï· ·þ Îñ Æ÷ ÁÐ ±í=============¡ï\n");
		GAMESERVER_ID::iterator itor = m_GameServerMap.begin();
		while (itor != m_GameServerMap.end())
		{
 printf(" ID¡¾%d¡¿ %-25s %-16s:%d\t×´Ì¬¡¾", itor->second->cID,
 	itor->second->ServerName, itor->second->cIP, itor->second->uiPort);

 switch (itor->second->cState)
 {
 case 0:
 	printf("Æô¶¯¡¿\n");
 	break;
 case 1:
 	printf("Õý³£¡¿\n");
 	break;
 case 2:
 	printf("·±Ã¦¡¿\n");
 	break;
 case 3:
 	printf("ÒÑÂú¡¿\n");
 	break;
 default:
 	break;
 }

 itor++;
		}
	}
}