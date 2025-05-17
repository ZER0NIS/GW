#include <winsock2.h>
#include <windows.h>
#include "../inc/Record.h"
#include "../inc/Field.h"
#include "../inc/Recordset.h"
#include "..\..\Base\inc\SvrBase.h"

namespace rade_db
{
	MYHEAP_IMPLEMENTATION(CRecord, s_heap)
		CRecord::CRecord(CRecordset& res, UINT unAmountField)
		: m_res(res)
	{
		for (UINT i = 0; i < unAmountField; i++)
		{
			CField objField(res.GetFieldInfo(i));
			m_setFields.push_back(objField);
		}
	}

	CRecord::CRecord(CRecordset& res, MYSQL_ROW row, unsigned int& unNumFields)
		: m_res(res)
	{
		for (UINT i = 0; i < unNumFields; i++)
		{
			CField objField(res.GetFieldInfo(i));
			objField.SetValue(row[i]);
			objField.TagChanged(false);

			m_setFields.push_back(objField);
		}
	}

	CRecord::CRecord(CRecordset& res, MYSQL_FIELD* fields, unsigned int& unNumFields)
		: m_res(res)
	{
		for (UINT i = 0; i < unNumFields; i++)
		{
			CField objField(res.GetFieldInfo(i));
			objField.SetValue(fields[i].def);
			objField.TagChanged(false);

			m_setFields.push_back(objField);
		}
	}

	CRecord::~CRecord()
	{
		m_res.Remove(this);
		m_setFields.clear();
	}

	IData& CRecord::Field(unsigned int unIndex)
	{
		if (unIndex > m_setFields.size())
		{
			sbase::LogSave("rade_db", "ERROR: index[%u] out of Record.", unIndex);
			unIndex = 0;
		}

		return m_setFields[unIndex];
	}

	IData& CRecord::Field(const char* pszName)
	{
		if (pszName)
		{
			for (UINT i = 0; i < m_setFields.size(); i++)
			{
				CField& field = m_setFields[i];
				if (0 == strcmp(field.GetName(), pszName))
					return field;
			}

			sbase::LogSave("rade_db", "ERROR:  not found field[%s] in Record", pszName);
		}

		return this->Field(static_cast<unsigned int>(0));
	}

	IData& CRecord::Key(void)
	{
		return this->Field(m_res.m_unKeyIndex);
	}

	const char* CRecord::KeyName(void)
	{
		CField& field = m_setFields[m_res.m_unKeyIndex];
		return field.GetName();
	}

	void CRecord::ClsEditFlag(void)
	{
		for (UINT i = 0; i < m_setFields.size(); i++)
			m_setFields[i].TagChanged(false);
	}

	bool CRecord::BuildSQLOperation(char* pszOperationSQL)
	{
		pszOperationSQL[0] = '\0';

		char szFormat[1024] = "";
		BOOL bFirst = true;
		BOOL bFlag = true;
		for (UINT i = 0; i < m_setFields.size(); i++)
		{
			CField& field = m_setFields[i];
			if (!field.IsChanged())
				continue;

			switch (field.GetType())
			{
			case FIELD_TYPE_STRING:
			case FIELD_TYPE_VAR_STRING:
				if (field.m_strVal.length() <= 0)
					bFlag = false;
				else
					sprintf(szFormat, "='%s'", field.m_strVal.c_str());
				break;

			case FIELD_TYPE_FLOAT:
				sprintf(szFormat, "=%.2f", field.m_dVal);
				break;

			case FIELD_TYPE_DOUBLE:
				sprintf(szFormat, "=%.2f", field.m_dVal);
				break;

			case FIELD_TYPE_TINY:
				if ((field.GetAttr() & UNSIGNED_FLAG) != 0)
					sprintf(szFormat, "=%u", field.m_i64Val);
				else
					sprintf(szFormat, "=%d", field.m_i64Val);
				break;

			case FIELD_TYPE_SHORT:
				if ((field.GetAttr() & UNSIGNED_FLAG) != 0)
					sprintf(szFormat, "=%u", field.m_i64Val);
				else
					sprintf(szFormat, "=%d", field.m_i64Val);
				break;

			case FIELD_TYPE_LONG:
				if ((field.GetAttr() & UNSIGNED_FLAG) != 0)
					sprintf(szFormat, "=%u", field.m_i64Val);
				else
					sprintf(szFormat, "=%ld", field.m_i64Val);
				break;

			case FIELD_TYPE_LONGLONG:
				if ((field.GetAttr() & UNSIGNED_FLAG) != 0)
					sprintf(szFormat, "=%u", field.m_i64Val);
				else
					sprintf(szFormat, "=%ld", field.m_i64Val);
				break;

			default:
				sbase::LogSave("rade_db", "Error: unknow type in CRecord::BuildUpdateOpration()");
				break;
			}

			if (bFlag)
			{
				if (!bFirst)
					strcat(pszOperationSQL, ",");
				else
					bFirst = false;

				strcat(pszOperationSQL, field.GetName());
				strcat(pszOperationSQL, szFormat);
			}
			else
				bFlag = true;
		}

		if (pszOperationSQL[0] == '\0')
			return false;

		return true;
	}

	void CRecord::BuildSQLCondition(char* pszConditionSQL)
	{
		::memset(pszConditionSQL, 0L, sizeof(pszConditionSQL));
		pszConditionSQL[0] = '\0';

		sprintf(pszConditionSQL, "%s=%ld", this->KeyName(), (long)this->Key());
	}
}