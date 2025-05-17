#include "../inc/Database.h"
#include "../inc/Recordset.h"
#include "../inc/Record.h"
#include "..\..\Base\inc\SvrBase.h"
#include "..\..\Base\inc\Ini.h"
#include <iostream>
#include <tchar.h>

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "winmm.lib" )

#ifdef _DEBUG
#pragma comment( lib, "..//..//lib//Debug//BaseD.lib" )
#else
#pragma comment( lib, "..//..//lib//Release//Base.lib" )

#endif

BOOL APIENTRY DllMain(HANDLE,
	DWORD  ul_reason_for_call,
	LPVOID
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

namespace rade_db
{
	DEFINE_FIXEDSIZE_ALLOCATOR(CDatabase, 1000, CMemoryPool::GROW_FAST);

	CDatabase::CDatabase()
		: m_hdbc(NULL), m_hMutex(NULL), m_bOpen(false)
		, m_hdbcAsync(NULL), m_pThread(NULL)
	{
		memset(m_szDBName, 0L, sizeof(m_szDBName));
		AsyncSQL_Request2.reserve(200);
		AsyncSQL_Request1.reserve(200);
		AsyncSQL_RequestOut = &AsyncSQL_Request2;
		AsyncSQL_RequestIn = &AsyncSQL_Request1;

		m_request_num = 0;
		m_deal_num = 0;
		m_result_num = 0;
	}

	CDatabase::~CDatabase()
	{
		this->Close();
		SAFE_DELETE(m_pThread);
	}

	bool CDatabase::Open(const char* szDBServer, const char* szLoginUser, const char* szPassword, const char* szDBName, bool)
	{
		IF_NOT(szDBServer)
 return false;

		IF_NOT(szLoginUser)
 return false;

		m_hMutex = ::CreateMutex(NULL, false, NULL);
		if (!m_hMutex)
		{
 ::MessageBox(NULL, "Create Mutex failed.", "Error", MB_OK | MB_ICONERROR);
 return false;
		}

		m_hdbc = this->Connect(szDBServer, szLoginUser, szPassword, szDBName);
		if (!m_hdbc)
		{
 ::MessageBox(NULL, "rade_db connect failed.", "Error", MB_OK | MB_ICONERROR);
 return false;
		}

		m_hdbcAsync = this->Connect(szDBServer, szLoginUser, szPassword, szDBName);
		if (!m_hdbcAsync)
		{
 ::MessageBox(NULL, "AsyncDB connect failed.", "Error", MB_OK | MB_ICONERROR);
 return false;
		}

		m_pThread = sbase::CThread::CreateNew(*this, 0, 3);
		//m_pThread->SetThreadAMask(12);
		if (!m_pThread)
		{
 ::MessageBox(NULL, "Create AsyncDB thread fail.", "Error", MB_OK | MB_ICONERROR);
 return false;
		}
		printf("MariaDB client version: %s\n", mysql_get_client_info());
		m_bOpen = true;
		strcpy(m_szDBName, szDBName);

		return true;
	}

	void
		CDatabase::Close(void)
	{
		if (m_hMutex)
		{
 ::CloseHandle(m_hMutex);
 m_hMutex = NULL;
		}

		if (m_hdbc != NULL)
		{
 mysql_close(m_hdbc);
 m_hdbc = NULL;
		}

		m_bOpen = false;
	}

	typedef unsigned long ulong;

	MYSQL* CDatabase::Connect(const char* szHost, const char* szUser, const char* szPasswd, const char* szDB,
		unsigned int uiPort, char* szSocket, unsigned int)
	{
		MYSQL* hdbc = mysql_init(NULL);
		if (hdbc == NULL)
		{
 sbase::LogSave("rade_db", "ERROR: rade_db init error.");
 return NULL;
		}
		//	mysql_options(hdbc, MYSQL_OPT_SSL_MODE, (void*)SSL_MODE_DISABLED);
		mysql_options(hdbc, MYSQL_READ_DEFAULT_GROUP, "");
		mysql_options(hdbc, MYSQL_OPT_CONNECT_TIMEOUT, "");
		hdbc->options.read_timeout = 1000;
		my_bool reconnect = 1;
		mysql_options(hdbc, MYSQL_OPT_RECONNECT, &reconnect);
		hdbc->free_me = 1;

		unsigned long  flag = CLIENT_MULTI_RESULTS | CLIENT_MULTI_STATEMENTS | CLIENT_FOUND_ROWS | CLIENT_INTERACTIVE;
		if (!mysql_real_connect(hdbc, szHost, szUser, szPasswd, szDB, uiPort, szSocket, flag))
		{
 mysql_close(hdbc);
 sbase::LogSave("rade_db", "ERROR: rade_db real connect failed.");
 return NULL;
		}

		// Forzar UTF-8 después de conectar (utf8mb4 soporta todos los caracteres Unicode)
		if (mysql_set_character_set(hdbc, "utf8") != 0)
		{
 sbase::LogSave("rade_db", "WARNING: Could not set UTF-8 charset: %s", mysql_error(hdbc));
		}

		mysql_set_server_option(hdbc, MYSQL_OPTION_MULTI_STATEMENTS_ON);

		strcpy(m_szDBName, szDB);
		strcpy(m_szHost, szHost);
		strcpy(m_szPasswd, szPasswd);
		strcpy(m_szUser, szUser);

		return hdbc;
	}

	I64
		CDatabase::GetLastInsertedID()
	{
		IF_NOT(this->IsOpen())
 return 0;

		return mysql_insert_id(m_hdbc);
	}

	MYSQL_RES*
		CDatabase::ExecuteSQL(const char* szSQL)
	{
		IF_NOT(this->IsOpen())
 return NULL;

		try
		{
 if (!m_hdbc)
 {
 	sbase::LogSave("rade_db", "ERROR: Database not init");
 	return NULL;
 }
 MYSQL_RES* pRes = NULL, * surplus = NULL;
 if (WAIT_OBJECT_0 == ::WaitForSingleObject(m_hMutex, _MAX_DBACCESSTIME))
 {
 	DWORD dwStart = ::clock();
 	m_hdbc->server_status &= ~SERVER_MORE_RESULTS_EXISTS;

 	if (mysql_query(m_hdbc, szSQL) != 0)
 	{
 		const char* error = mysql_error(m_hdbc);
 		sbase::LogSave("rade_db", "ERROR: Database ExecuteSQL(%s) occur mysql error(%s).", error, szSQL);
 		if (mysql_query(m_hdbc, szSQL) != 0)
 		{
  ::ReleaseMutex(m_hMutex);
  return NULL;
 		}
 	}
 	pRes = mysql_store_result(m_hdbc);
 	while (mysql_next_result(m_hdbc) == 0)
 	{
 		surplus = mysql_store_result(m_hdbc);
 		mysql_free_result(surplus);
 	}
 	::InterlockedExchangeAdd(&m_tmAccess, ::clock() - dwStart);
 	this->StatisticSQL(szSQL, m_tmAccess);
 }
 else
 {
 	sbase::LogSave("rade_db", "WARNING: Database ExecuteSQL(%s) overtime", szSQL);
 }

 ::ReleaseMutex(m_hMutex);
 return pRes;
		}
		catch (...)
		{
 sbase::LogSave("rade_db", "EXCEPTION: Database ExecuteSQL(%s)", szSQL);
 return NULL;
		}
	}

	bool
		CDatabase::ExecuteSyncSQL(const char* pszSQL)
	{
		MYSQL_RES* pRes = this->ExecuteSQL(pszSQL);
		if (pRes)
 return true;

		return false;
	}

	bool CDatabase::CallBackAsyncSQL()
	{
		sbase::CSingleLock  Lock(&m_xResultLock);

		while (AsyncSQL_Result.size() > 0)
		{
 PSQL_RESULT DealResult = AsyncSQL_Result.front();
 if (DealResult != NULL)
 {
 	if (DealResult->callback)
 	{
 		DealResult->callback(DealResult);
 	}
 	SAFE_RELEASE(DealResult);
 	DealResult = NULL;
 }
 AsyncSQL_Result.erase(AsyncSQL_Result.begin());
 m_result_num++;
		}

		return true;
	}

	bool CDatabase::ExecuteAsyncSQL(const char* pszSQL, void* pUser, SQL_RESULT_CALLBACK callback)
	{
		if (strlen(pszSQL) == 0)
		{
 sbase::LogSave("rade_db", "ERROR: Empty SQL string provided.");
 return false;
		}

		IF_NOT(this->IsOpen())
		{
 sbase::LogSave("rade_db", "ERROR: Database connection not open.");
 return false;
		}

		IF_NOT(pszSQL && strlen(pszSQL) > 0 && strlen(pszSQL) < _MAX_SQL)
		{
 sbase::LogSave("rade_db", "ERROR: SQL string too long (%d chars, max %d).",
 	strlen(pszSQL), _MAX_SQL);
 return false;
		}

		char szSQL[_MAX_SQL] = "";
		::strncpy(szSQL, pszSQL, _MAX_SQL - 1);
		szSQL[_MAX_SQL - 1] = '\0';

		SQL_REQUEST request;
		memset(request.Desrible, 0L, sizeof(request.Desrible));
		::strncpy(request.Desrible, pszSQL, sizeof(request.Desrible) - 1);
		request.Desrible[sizeof(request.Desrible) - 1] = '\0';
		request.callback = callback;
		request.pPalyer = pUser;

		sbase::LogSave("rade_db", "Queueing SQL [%.100s] for player %p",
 szSQL, pUser);

		if (m_pThread)
		{
 bool bOk = m_pThread->SetEvent();
 if (bOk)
 	sbase::LogSave("rade_db", "[INFO] Evento de trabajo activado correctamente.\n");
 else
 	sbase::LogSave("rade_db", "[ERROR] Falló al activar evento de trabajo.\n");
		}
		else
		{
 sbase::LogSave("rade_db", "[ERROR] Puntero m_pThread nulo.\n");
		}

		sbase::CSingleLock Lock(&m_xLock);
		AsyncSQL_RequestIn->push_back(request);
		m_request_num++;

		return true;
	}

	bool CDatabase::CheckSQL(const char* szSQL)
	{
		IF_NOT(szSQL)
 return false;

		char pszSQL[_MAX_SQL] = "";
		strcpy(pszSQL, szSQL);
		strupr(pszSQL);

		if (0 == strcmp(pszSQL, "UPDATE"))
		{
 if (!strstr(pszSQL, "WHERE") || !strstr(pszSQL, "LIMIT"))
 {
 	sbase::LogSave("rade_db", "ERROR: Invalid update SQL [%s].", pszSQL);
 	return false;
 }
		}

		if (0 == strcmp(pszSQL, "DELETE"))
		{
 if (!strstr(pszSQL, "WHERE"))
 {
 	sbase::LogSave("rade_db", "ERROR: Invalid SQL delete [%s].", pszSQL);
 	return false;
 }
		}

		return true;
	}

	void CDatabase::StatisticSQL(const char* pszSQL, DWORD dwRun)
	{
		if (!pszSQL)
 return;

		char szSQL[_MAX_SQL] = "";
		::strncpy(szSQL, pszSQL, ::strlen(pszSQL));
		::strlwr(szSQL);

		DWORD dwTotalSQL = m_infoDB.dwTotalSQL;
		if (m_tm.ToNextTime())
		{
 ::memset(&m_infoDB, 0L, sizeof(DB_INFO));
 m_infoDB.dwTotalSQL = dwTotalSQL;
		}

		m_infoDB.dwTotalSQL++;
		m_infoDB.dwSQLPer5Min++;
		m_infoDB.dwSQLRunPer5Min += dwRun;
		if (m_infoDB.dwSQLMaxPer5Min < dwRun)
 m_infoDB.dwSQLMaxPer5Min = dwRun;

		if (0 != ::strstr(szSQL, "update"))
		{
 m_infoDB.dwUpdatePer5Min++;
 m_infoDB.dwUpdateRunPer5Min += dwRun;
 if (m_infoDB.dwUpdateMaxPer5Min < dwRun)
 	m_infoDB.dwUpdateMaxPer5Min = dwRun;
		}
		else if (0 != ::strstr(szSQL, "select"))
		{
 m_infoDB.dwSelectPer5Min++;
 m_infoDB.dwSelectRunPer5Min += dwRun;
 if (m_infoDB.dwSelectMaxPer5Min < dwRun)
 	m_infoDB.dwSelectMaxPer5Min = dwRun;
		}
		else if (0 != ::strstr(szSQL, "insert"))
		{
 m_infoDB.dwInsertPer5Min++;
 m_infoDB.dwInsertRunPer5Min += dwRun;
 if (m_infoDB.dwInsertMaxPer5Min < dwRun)
 	m_infoDB.dwInsertMaxPer5Min = dwRun;
		}
		else if (0 != ::strstr(szSQL, "delete"))
		{
 m_infoDB.dwDeletePer5Min++;
 m_infoDB.dwDeleteRunPer5Min += dwRun;
 if (m_infoDB.dwDeleteMaxPer5Min < dwRun)
 	m_infoDB.dwDeleteMaxPer5Min = dwRun;
		}
	}

	void
		CDatabase::UpdateStatInfo(DB_INFO& infoDB)
	{
		::memset(&infoDB, 0L, sizeof(DB_INFO));

		DWORD dwTotalSQL = m_infoDB.dwTotalSQL;
		if (m_tm.ToNextTime())
		{
 m_infoDB.dwSQLAvgPer5Min += (m_infoDB.dwSQLPer5Min != 0) ? m_infoDB.dwSQLRunPer5Min * 1000 / m_infoDB.dwSQLPer5Min : 0;
 m_infoDB.dwSelectAvgPer5Min += (m_infoDB.dwSelectRunPer5Min != 0) ? m_infoDB.dwSelectRunPer5Min * 1000 / m_infoDB.dwSelectPer5Min : 0;
 m_infoDB.dwUpdateAvgPer5Min += (m_infoDB.dwUpdateRunPer5Min != 0) ? m_infoDB.dwUpdateRunPer5Min * 1000 / m_infoDB.dwUpdatePer5Min : 0;
 m_infoDB.dwInsertAvgPer5Min += (m_infoDB.dwInsertRunPer5Min != 0) ? m_infoDB.dwInsertRunPer5Min * 1000 / m_infoDB.dwInsertPer5Min : 0;
 m_infoDB.dwDeleteAvgPer5Min += (m_infoDB.dwDeleteRunPer5Min != 0) ? m_infoDB.dwDeleteRunPer5Min * 1000 / m_infoDB.dwDeletePer5Min : 0;
 infoDB = m_infoDB;

 ::memset(&m_infoDB, 0L, sizeof(DB_INFO));
 m_infoDB.dwTotalSQL = dwTotalSQL;
 return;
		}

		infoDB = m_infoDB;
	}

	IRecordset*
		CDatabase::CreateRecordset(const char* szSQL, MODE eMode)
	{
		IF_NOT(szSQL)
 return NULL;

		IF_NOT(this->IsOpen())
 return NULL;

		CRecordset* pRes = new CRecordset(*this, eMode);
		IF_NOT(pRes)
 return NULL;

		IF_NOT(pRes->Create(szSQL))
		{
 SAFE_DELETE(pRes);
 return NULL;
		}

		return pRes;
	}

	bool
		CDatabase::Execute(const char* szSQL, MODE)
	{
		IF_NOT(szSQL)
 return false;

		IF_NOT(this->IsOpen())
 return false;

		if (NULL == ExecuteSQL(szSQL))
		{
 return true;
		}
		return false;
	}

	IRecord*
		CDatabase::MakeDefaultRecord(const char* szTable, unsigned long)
	{
		IF_NOT(szTable)
 return NULL;

		IF_NOT(this->IsOpen())
 return NULL;

		CRecordset* pRes = new CRecordset(*this, MODE_EDIT);
		IF_NOT(pRes)
 return NULL;

		CRecord* pRec = pRes->MakeDefaultRecord(szTable);
		IF_NOT(pRec)
		{
 SAFE_DELETE(pRes);
 return NULL;
		}

		return pRec;
	}

	int CDatabase::OnThreadEvent(void)
	{
		return 0;
	}

	int	CDatabase::OnThreadProcess(void)
	{
		static int AsyncNum = 0;
		static int HdbcNum = 0;

		if (!m_hdbcAsync)
		{
 m_hdbcAsync = Connect(m_szHost, m_szUser, m_szPasswd, m_szDBName);
 if (NULL == m_hdbcAsync)
 {
 	++AsyncNum;
 	sbase::ConsoleWriteColorText(FOREGROUND_GREEN, "Connect MySQL database failed : %d.", AsyncNum);
 	if (AsyncNum == 10 || HdbcNum == 10)
 	{
 		sbase::LogSave("Error", "Server down with MySQL crash! ");
 		ExitProcess(0);
 	}
 	return 0;
 }
 else
 	AsyncNum = 0;
		}

		if (!m_hdbc)
		{
 m_hdbc = Connect(m_szHost, m_szUser, m_szPasswd, m_szDBName);
 if (NULL == m_hdbc)
 {
 	++HdbcNum;
 	sbase::ConsoleWriteColorText(FOREGROUND_GREEN, "Connect MySQL database failed : %d.", HdbcNum);
 	if (AsyncNum == 10 || HdbcNum == 10)
 	{
 		sbase::LogSave("Error", "Server down with MySQL crash! ");
 		ExitProcess(0);
 	}
 	return 0;
 }
 else
 	HdbcNum = 0;
		}

		std::string strSQL;
		MYSQL* DBHandle = NULL;
		size_t  Request_Num = AsyncSQL_RequestOut->size();

		while (Request_Num)
		{
 if (!m_hdbcAsync)
 {
 	m_hdbcAsync = Connect(m_szHost, m_szUser, m_szPasswd, m_szDBName);
 	if (NULL == m_hdbcAsync)
 		return 0;
 }

 if (!m_hdbc)
 {
 	m_hdbc = Connect(m_szHost, m_szUser, m_szPasswd, m_szDBName);
 	if (NULL == m_hdbc)
 		return 0;
 }

 strSQL.clear();

 SQL_REQUEST& request = AsyncSQL_RequestOut->front();
 strSQL = request.Desrible;

 if (!strSQL.empty())
 {
 	DWORD dwStart = ::clock();
 	m_hdbcAsync->server_status &= ~SERVER_MORE_RESULTS_EXISTS;
 	{
 		if (mysql_query(m_hdbcAsync, strSQL.c_str()) != 0)
 		{
  const char* error = mysql_error(m_hdbcAsync);
  sbase::LogSave("rade_db", "ERROR: Database ExecuteSQL(%s) occur mysql error(%s),[%d].", error, strSQL.c_str(), 0);
  mysql_close(m_hdbcAsync);
  m_hdbcAsync = NULL;
  if (mysql_query(m_hdbc, strSQL.c_str()) != 0)
  {
  	const char* error = mysql_error(m_hdbc);
  	sbase::LogSave("rade_db", "ERROR: Database ExecuteSQL(%s) occur mysql error(%s),[%d].", error, strSQL.c_str(), 1);
  	mysql_close(m_hdbc);
  	m_hdbc = NULL;

  	goto Dump;
  }
  else
  {
  	DBHandle = m_hdbc;
  }
 		}
 		else
 		{
  DBHandle = m_hdbcAsync;
 		}
 		PSQL_RESULT pResult = NULL;
 		MYSQL_RES* pSQL_RES = NULL, * pSQL_Surplus = NULL;
 		pSQL_RES = mysql_store_result(DBHandle);
 		while (mysql_next_result(DBHandle) == 0)
 		{
  pSQL_Surplus = mysql_store_result(DBHandle);
  mysql_free_result(pSQL_Surplus);
 		}

 		if (request.callback)
 		{
  pResult = new SQL_RESULT;
  memset(pResult, 0L, sizeof(SQL_RESULT));
 		}
 		else
 		{
  if (pSQL_RES)
  	sbase::LogSave("rade_db", "Warning: Database request callable function with [%s]", strSQL.c_str());

  goto Dump;
 		}

 		pResult->callback = request.callback;
 		pResult->Falg = true;
 		pResult->pPlayer = request.pPalyer;
 		if (NULL == pSQL_RES)
 		{
  pResult->pResult = NULL;
  goto Next;
 		}
 		CRecordset* pRes = new CRecordset(*this, rade_db::MODE_EDIT);
 		IF_NOT(pRes->Create(pSQL_RES))
 		{
  SAFE_DELETE(pRes);
 		}
 		pResult->pResult = pRes;
 	Next:
 		m_xResultLock.Lock();
 		AsyncSQL_Result.push_back(pResult);
 		m_xResultLock.Unlock();
 	Dump: AsyncSQL_RequestOut->erase(AsyncSQL_RequestOut->begin());
 		::InterlockedExchangeAdd(&m_tmAccess, ::clock() - dwStart);
 		strSQL.clear();
 		Sleep(1);
 		m_deal_num++;
 	}
 }
 Request_Num = AsyncSQL_RequestOut->size();
		}

		ChangeRequestVector();

		return 1;
	}

	void  CDatabase::ChangeRequestVector()
	{
		sbase::CSingleLock  Lock(&m_xLock);
		if (AsyncSQL_RequestIn == &AsyncSQL_Request1)
		{
 AsyncSQL_RequestIn = &AsyncSQL_Request2;
 AsyncSQL_RequestOut = &AsyncSQL_Request1;
		}
		else
		{
 AsyncSQL_RequestIn = &AsyncSQL_Request1;
 AsyncSQL_RequestOut = &AsyncSQL_Request2;
		}
	}

	int CDatabase::ConvertStr(char* pchNew, const char* pchOld)
	{
		if (!pchOld)
 return 0;

		MYSQL* ptem = m_hdbc ? m_hdbc : m_hdbcAsync;
		if (!ptem)
 return 0;

		if (!pchOld && !pchNew)
 return 0;

		int ulLen = mysql_real_escape_string(ptem, pchNew, pchOld, (int)strlen(pchOld));
		*(pchNew + ulLen) = '\0';

		return ulLen;
	}

	DB_API IDatabase*
		rade_db::DatabaseCreate(const char* szDBServer, const char* szLoginName, const char* szPassword, const char* szDBName, bool bEnableSQLChk)
	{
		rade_db::CDatabase* pDB = new rade_db::CDatabase;
		IF_NOT(pDB)
 return NULL;

		IF_NOT(pDB->Open(szDBServer, szLoginName, szPassword, szDBName, bEnableSQLChk))
		{
 pDB->Release();
 return NULL;
		}

		return pDB;
	}

	static	sbase::CCriticalSection	xLock;
	static rade_db::CDatabase* g_pDBTool;

	DB_API IDatabase& rade_db::InstanceGet(void)
	{
		sbase::CSingleLock xLock(&xLock);
		if (g_pDBTool == NULL)
		{
 g_pDBTool = new rade_db::CDatabase;
		}

		if (!g_pDBTool->IsOpen())
		{
 char szDBServer[64] = "";
 char szLoginName[64] = "";
 char szPassword[64] = "";
 char szDBName[64] = "";

 sbase::CIni ini("config.ini", false);
 strcpy(szDBServer, ini.GetString("Database", "DBServer"));
 strcpy(szLoginName, ini.GetString("Database", "LoginName"));
 strcpy(szPassword, ini.GetString("Database", "Password"));
 strcpy(szDBName, ini.GetString("Database", "DBName"));

 bool bEnableSQLChk = true;
 g_pDBTool->Open(szDBServer, szLoginName, szPassword, szDBName, bEnableSQLChk);
		}

		return *g_pDBTool;
	}

	DB_API void Destroy(void)
	{
		SAFE_RELEASE(g_pDBTool);
	}
}