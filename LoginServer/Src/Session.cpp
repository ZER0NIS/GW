#include "..\Inc\Session.h"
#include "..\inc\LoginLogic.h"

// -------------------------------------------------------
// Inicializa el pool de memoria para CSession
// -------------------------------------------------------
DEFINE_FIXEDSIZE_ALLOCATOR(CSession, 2000, CMemoryPool::GROW_FAST);

// -------------------------------------------------------
CSession::CSession(const char* Name, const char* PassWord, void* Socket, login::CLogin& Logion) :
	m_Logion(Logion), m_bGS(false), m_bDelAccout(false)
{
	strcpy(m_Name, Name);
	strcpy(m_PassWord, PassWord);
	m_Socket = Socket;

	printf("[CSession] Nueva sesión creada. Cuenta: %s, IP: %s\n", m_Name, static_cast<snet::CSocket*>(Socket)->GetPeerIP());
}

CSession::~CSession()
{
	printf("[CSession] Sesión destruida para la cuenta: %s\n", m_Name);
}

// -------------------------------------------------------
// Consulta a la base de datos
// -------------------------------------------------------
void CSession::OnDBRequest() const
{
	char szSQL[128];
	snet::CSocket* pSocket = static_cast<snet::CSocket*>(m_Socket);
	sprintf(szSQL, SQL_SELECT_ACCOUNT_INFO, m_Name, m_PassWord, pSocket->GetPeerIP());

	printf("[DBRequest] Ejecutando consulta SQL: %s\n", szSQL);
	if (NULL != m_Logion.m_pDB)
		m_Logion.m_pDB->ExecuteAsyncSQL(szSQL, (void*)this, &CSession::OnDBResponse);
}

// -------------------------------------------------------
// Procesamiento de respuesta de la base de datos
// -------------------------------------------------------
void CSession::OnDBResponse(rade_db::PSQL_RESULT result)
{
	printf("[DBResponse] Recibida respuesta de la base de datos\n");

	IF_NOT(result)
	{
		printf("[DBResponse] Error: resultado nulo\n");
		return;
	}
	CSession* pSession = static_cast<CSession*>(result->pPlayer);
	if (pSession == NULL)
	{
		printf("[DBResponse] Error: puntero a sesión nulo\n");
		return;
	}

	rade_db::IRecordset* pRes = result->pResult;
	if (!pRes)
	{
		printf("[DBResponse] Error: recordset nulo\n");
		SAFE_DELETE(pSession);
		return;
	}

	int RecordCount = pRes->CountRecord();
	if (RecordCount == 0)
	{
		printf("[DBResponse] Error: cuenta no encontrada para usuario: %s\n", pSession->m_Name);
		SAFE_DELETE(pSession);
		return;
	}

	pRes->Move(0);
	rade_db::IRecord* Record = pRes->GetRecord();
	IF_NOT(Record)
	{
		printf("[DBResponse] Error: no se pudo obtener el registro\n");
		SAFE_DELETE(pSession);
		return;
	}

	enum { AC_EXIT, AC_BAN, IP_BAN };
	char Result = Record->Field(UINT(AC_EXIT));
	char ban_account = Record->Field(UINT(AC_BAN));
	char ban_ip = Record->Field(UINT(IP_BAN));

	printf("[DBResponse] Resultado de cuenta: %d, BanAcc: %d, BanIP: %d\n", Result, ban_account, ban_ip);

	// Construcción del mensaje de retorno
	MSG_LOGIN_RETURN_INFO Login_info;
	Login_info.Head.usSize = sizeof(MSG_LOGIN_RETURN_INFO);
	Login_info.Head.usType = _MSG_LOGIN_RETURN_INFO;
	Login_info.ucInfo = Result;

	if (Result == 1)
	{
		if (ban_account == 1)
			Login_info.ucInfo++;

		if (ban_ip == 1)
			Login_info.ucInfo += 2;
	}

	printf("[DBResponse] Enviando resultado de login: ucInfo=%d a la cuenta %s\n", Login_info.ucInfo, pSession->m_Name);

	pSession->m_Logion.SendMsg(&Login_info, pSession->m_Socket, Login_info.Head.usSize);

	// Cachear sesión en memoria
	pSession->m_Logion.CacheAccounts(pSession->GetAccount(), pSession);

	if (Result == 1) // Login exitoso
	{
		printf("[DBResponse] Login exitoso, enviando lista de servidores...\n");

		login::CLogin::GAMESERVER_ID::iterator itor;
		for (itor = pSession->m_Logion.m_GameServerMap.begin(); itor != pSession->m_Logion.m_GameServerMap.end(); itor++)
		{
			MSG_GAMESERVER_INFO GameSer = *(*itor).second;
			memset(GameSer.cIP, 0L, sizeof(GameSer.cIP));
			GameSer.uiPort = 0;

			printf("[DBResponse] Enviando servidor [%s]: IP=..., Puerto=%u\n", GameSer.cIP, GameSer.uiPort);

			pSession->m_Logion.SendMsg(&GameSer, pSession->m_Socket, GameSer.Head.usSize);
		}
	}
}