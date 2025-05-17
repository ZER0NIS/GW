#include <algorithm>
#include "../inc/Recordset.h"
#include "../inc/Record.h"
#include "../inc/Database.h"
#include "..\..\Base\inc\SvrBase.h"

namespace rade_db
{
	MYHEAP_IMPLEMENTATION(CRecordset, s_heap)

		const UINT INVALID_KEY = UINT_MAX;

	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////
	CRecordset::CRecordset(CDatabase& rade_db, MODE eMode)
		: m_db(rade_db), m_nCursor(0), m_unKeyIndex(INVALID_KEY), m_unAutoIncIndex(INVALID_KEY), m_eMode(eMode)
	{
		::memset(m_szSQL, 0L, sizeof(m_szSQL));

		m_setFieldInfo.clear();
		m_setRecord.clear();
	}

	///////////////////////////////////////////////////////////////////////
	CRecordset::~CRecordset()
	{
		std::vector<CRecord*>::iterator itor = m_setRecord.begin();
		for (; itor != m_setRecord.end(); ++itor)
		{
			CRecord* pRec = *itor;
			if (NULL != pRec)
				SAFE_RELEASE(pRec);
		}

		m_nCursor = 0;
		m_unKeyIndex = INVALID_KEY;
		m_eMode = MODE_NONE;
		m_setFieldInfo.clear();
		m_setRecord.clear();
	}

	///////////////////////////////////////////////////////////////////////
	bool
		CRecordset::Create(const char* szSQL)
	{
		IF_NOT(szSQL)
			return false;

		IF_NOT(m_db.IsOpen())
			return false;

		MYSQL_RES* pRES = m_db.ExecuteSQL(szSQL);

		// save SQL statement
		::memcpy(m_szSQL, szSQL, strlen(szSQL));
		::strupr(m_szSQL);

		return this->Create(pRES);
	}

	bool
		CRecordset::Create(MYSQL_RES* pRES)
	{
		IF_NOT(pRES)
			return false;

		// 填写字段信息
		MYSQL_FIELD* fields = mysql_fetch_fields(pRES);
		IF_NOT(fields)
		{
			mysql_free_result(pRES);
			return false;
		}

		UINT unNumFields = mysql_num_fields(pRES);
		for (UINT i = 0; i < unNumFields; i++)
		{
			FieldInfo fieldInfo;
			fieldInfo.strName = fields[i].name;
			fieldInfo.unType = fields[i].type;
			fieldInfo.unAttr = fields[i].flags;
			fieldInfo.unLen = fields[i].length;

			m_setFieldInfo.push_back(fieldInfo);

			if (fields[i].flags & PRI_KEY_FLAG)
			{
				m_unKeyIndex = i;
			}
			if (fields[i].flags & AUTO_INCREMENT_FLAG)
			{
				m_unAutoIncIndex = i;
			}
		}

		// 填写记录数据
		ULONGLONG ulNumRecord = mysql_num_rows(pRES);
		//ULONGLONG ulNumRecord = mysql_affected_rows(m_db.GetDBHandle());
		for (ULONG r = 0; r < ulNumRecord; r++)
		{
			// 移动到第i个记录
			mysql_data_seek(pRES, r);

			// 取第i个记录数据
			MYSQL_ROW	row = mysql_fetch_row(pRES);
			IF_NOT(row)
				continue;

			CRecord* pRec = new CRecord(*this, row, unNumFields);

			m_setRecord.push_back(pRec);
		}

		// move first!
		this->Move(0);

		// get table name
		::strcpy(m_szTableName, fields[0].table);

		// free MYSQL_RES

		mysql_free_result(pRES);

		return true;
	}

	///////////////////////////////////////////////////////////////////////
	void
		CRecordset::Remove(CRecord* pRec)
	{
		std::vector<CRecord*>::iterator itor = std::find(m_setRecord.begin(), m_setRecord.end(), pRec);
		if (itor != m_setRecord.end())
			*itor = NULL;
	}

	///////////////////////////////////////////////////////////////////////
	CRecord*
		CRecordset::MakeDefaultRecord(const char* pszTable)
	{
		IF_NOT(pszTable)
			return NULL;

		IF_NOT(m_db.IsOpen())
			return NULL;

		char szSQL[100];
		::sprintf(szSQL, "SELECT * FROM %s LIMIT 0", pszTable);
		MYSQL_RES* pRES = m_db.ExecuteSQL(szSQL);
		IF_NOT(pRES)
			return NULL;

		// 填写字段信息
		MYSQL_FIELD* fields = mysql_fetch_fields(pRES);
		IF_NOT(fields)
			return NULL;

		UINT unNumFields = mysql_num_fields(pRES);
		for (UINT i = 0; i < unNumFields; i++)
		{
			FieldInfo fieldInfo;
			fieldInfo.strName = fields[i].name;
			fieldInfo.unType = fields[i].type;
			fieldInfo.unAttr = fields[i].flags;
			fieldInfo.unLen = fields[i].length;

			m_setFieldInfo.push_back(fieldInfo);

			if (fields[i].flags & PRI_KEY_FLAG)
			{
				m_unKeyIndex = i;
			}
			if (fields[i].flags & AUTO_INCREMENT_FLAG)
			{
				m_unAutoIncIndex = i;
			}
		}

		// 填写缺省记录数据
		CRecord* pDefRec = new CRecord(*this, fields, unNumFields);
		m_setRecord.push_back(pDefRec);

		// move first!
		this->Move(0);

		// save SQL statement
		::memcpy(m_szSQL, szSQL, strlen(szSQL));
		::strupr(m_szSQL);

		// get table name
		::strcpy(m_szTableName, fields[0].table);

		// free MYSQL_RES
		mysql_free_result(pRES);
		return pDefRec;
	}

	///////////////////////////////////////////////////////////////////////
	// Interface of IRecordset
	///////////////////////////////////////////////////////////////////////
	IRecord*
		CRecordset::GetRecord(void) const
	{
		if (0 > m_nCursor)
			return NULL;

		return m_setRecord[m_nCursor];
	}

	///////////////////////////////////////////////////////////////////////
	void
		CRecordset::Move(unsigned int index)
	{
		if (m_setRecord.empty())
		{
			m_nCursor = -1;
			return;
		}

		if (index < 0)
			m_nCursor = 0;
		else if (index >= m_setRecord.size())
			m_nCursor = int(m_setRecord.size() - 1);
		else
			m_nCursor = index;
	}

	///////////////////////////////////////////////////////////////////////
	bool
		CRecordset::Update(bool bSync/* = true*/)
	{
		std::vector<CRecord*>::iterator itor = m_setRecord.begin();
		for (; itor != m_setRecord.end(); ++itor)
		{
			CRecord* pRec = *itor;
			if (CRecord::IsValidPt(pRec))
				continue;

			this->UpdateRecord(pRec, bSync);
		}

		return true;
	}

	///////////////////////////////////////////////////////////////////////
	void
		CRecordset::ClsEditFlag(void)
	{
		for (UINT i = 0; i < m_setRecord.size(); i++)
		{
			CRecord* pRec = m_setRecord[i];
			if (!pRec)
				continue;

			pRec->ClsEditFlag();
		}
	}

	///////////////////////////////////////////////////////////////////////
	IRecord*
		CRecordset::MakeDefRecord(void)
	{
		IF_NOT(m_db.IsOpen())
			return NULL;

		if (MODE_EDIT != m_eMode)
			return NULL;

		// new a default record, but it isn`t in rade_db
		UINT unAmountField = (UINT)m_setFieldInfo.size();
		CRecord* pRec = new CRecord(*this, unAmountField);
		if (!pRec)
			return NULL;

		return pRec;
	}

	///////////////////////////////////////////////////////////////////////
	void
		CRecordset::BuildSQLCondition(char* pszConditionSQL)
	{
		if (!pszConditionSQL)
			return;

		::memset(pszConditionSQL, 0L, sizeof(pszConditionSQL));
		pszConditionSQL[0] = '\0';

		CRecord* pRec = m_setRecord[m_nCursor];
		if (!pRec)
			return;

		sprintf(pszConditionSQL, "%s=%s", pRec->KeyName(), pRec->Key());
	}

	///////////////////////////////////////////////////////////////////////
	void
		CRecordset::BuildSQLOperation(char* pszOperationSQL)
	{
		if (!pszOperationSQL)
			return;

		::memset(pszOperationSQL, 0L, sizeof(pszOperationSQL));
		pszOperationSQL[0] = '\0';

		CRecord* pRec = m_setRecord[m_nCursor];
		if (!pRec)
			return;

		if (MODE_EDIT == m_eMode)
			pRec->BuildSQLOperation(pszOperationSQL);
	}

	///////////////////////////////////////////////////////////////////////
	bool
		CRecordset::UpdateRecord(CRecord* pRec, bool)
	{
		if (!m_db.IsOpen())
			return false;

		if (MODE_EDIT != m_eMode)
			return false;

		IF_NOT(CRecord::IsValidPt(pRec))
			return false;

		// build sql operate
		char szOperationSQL[1024];
		if (!pRec->BuildSQLOperation(szOperationSQL))
			return false;

		// build sql condition
		char szConditionSQL[128];
		pRec->BuildSQLCondition(szConditionSQL);

		char szSQL[2048] = "";
		sprintf(szSQL, "UPDATE %s SET %s WHERE %s LIMIT 1", m_szTableName, szOperationSQL, szConditionSQL);

		bool ret = false;
		//20070511 		if (bSync)
		// 			ret = m_db.ExecuteSyncSQL(szSQL);
		// 		else
		// 			ret = m_db.ExecuteAsyncSQL(szSQL);

				// clear edit flag
		if (ret)
			this->ClsEditFlag();

		return ret;
	}

	/////////////////////////////////////////////////////////////////////////
	bool
		CRecordset::DeleteRecord(CRecord* pRec, bool bArchive/* = true*/)
	{
		if (!m_db.IsOpen())
			return false;

		if (MODE_EDIT != m_eMode)
			return false;

		if (bArchive)
		{
			// build operate sql
			char szOperationSQL[1024];
			pRec->BuildSQLOperation(szOperationSQL);

			char szConditionSQL[128];
			pRec->BuildSQLCondition(szConditionSQL);

			char szSQL[2048];
			sprintf(szSQL, "UPDATE %s SET %s WHERE %s LIMIT 1", m_szTableName, szOperationSQL, szConditionSQL);
			if (!m_db.ExecuteSyncSQL(szSQL))
				return false;
		}
		else
		{
			// build cdffition sql
			char szConditionSQL[128];
			pRec->BuildSQLCondition(szConditionSQL);

			char szSQL[1024];
			sprintf(szSQL, "DELETE FROM %s WHERE %s LIMIT 1", m_szTableName, szConditionSQL);
			if (!m_db.ExecuteSyncSQL(szSQL))
				return false;
		}

		std::vector<CRecord*>::iterator itor = std::find(m_setRecord.begin(), m_setRecord.end(), pRec);
		if (itor != m_setRecord.end())
			m_setRecord.erase(itor);

		SAFE_RELEASE(pRec);
		return true;
	}

	///////////////////////////////////////////////////////////////////////
	bool
		CRecordset::InsertRecord(CRecord* pRec)
	{
		if (NULL == pRec)
			return NULL;

		IF_NOT(m_db.IsOpen())
			return NULL;

		if (MODE_EDIT != m_eMode)
			return NULL;

		char szOperationSQL[1024];
		pRec->BuildSQLOperation(szOperationSQL);

		// INSERT a new record into table
		char szSQL[128];
		sprintf(szSQL, "INSERT INTO %s SET %s", m_szTableName, szOperationSQL);
		if (m_db.ExecuteSyncSQL(szSQL))
			return NULL;

		if (m_unAutoIncIndex != INVALID_KEY)
		{
			//pRec->Field(m_unAutoIncIndex) = (unsigned int)m_db.GetLastInsertedID();
			pRec->Field(m_unAutoIncIndex) = (long)m_db.GetLastInsertedID();
		}

		m_setRecord.push_back(pRec);
		return true;
	}
}