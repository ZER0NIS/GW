#include "..\inc\Field.h"
#include "..\..\Base\inc\SvrBase.h"
#pragma warning(disable:4244)

namespace rade_db
{
	//////////////////////////////////////////////////////////////////////////////
	CField::CField(const FieldInfo& info)
		: m_info(info), m_bChanged(false)
	{
	}

	CField::CField(const CField& field)
		: m_info(field.m_info), m_bChanged(false)
	{
		m_i64Val = field.m_i64Val;
		//m_dVal	= field.m_dVal;
		m_strVal = field.m_strVal;
	}

	CField::~CField()
	{
	}

	CField&
		CField::operator=(const CField& field)
	{
		if (this == &field)
			return *this;

		if (m_info.unType != field.m_info.unType)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		//m_info		= field.m_info;
		m_i64Val = field.m_i64Val;
		//m_dVal	= field.m_dVal;
		m_strVal = field.m_strVal;
		m_bChanged = false;
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////////
	CField::operator bool() const
	{
		if (m_info.unType != FIELD_TYPE_TINY)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		if (m_i64Val != 0)
			return true;
		else
			return false;
	}

	CField::operator char() const
	{
		if ((UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_SHORT
			|| m_info.unType == FIELD_TYPE_LONG
			|| m_info.unType == FIELD_TYPE_LONGLONG
			|| m_info.unType == FIELD_TYPE_FLOAT
			|| m_info.unType == FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_i64Val;
	}

	CField::operator unsigned char() const
	{
		if (!(UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_SHORT
			|| m_info.unType == FIELD_TYPE_LONG
			|| m_info.unType == FIELD_TYPE_LONGLONG
			|| m_info.unType == FIELD_TYPE_FLOAT
			|| m_info.unType == FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_i64Val;
	}

	CField::operator short() const
	{
		if ((UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_LONG
			|| m_info.unType == FIELD_TYPE_LONGLONG
			|| m_info.unType == FIELD_TYPE_FLOAT
			|| m_info.unType == FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_i64Val;
	}

	CField::operator unsigned short() const
	{
		if (!(UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_LONG
			|| m_info.unType == FIELD_TYPE_LONGLONG
			|| m_info.unType == FIELD_TYPE_FLOAT
			|| m_info.unType == FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_i64Val;
	}

	CField::operator long() const
	{
		if ((UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_LONGLONG
			|| m_info.unType == FIELD_TYPE_FLOAT
			|| m_info.unType == FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_i64Val;
	}

	CField::operator unsigned long() const
	{
		if (!(UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_LONGLONG
			|| m_info.unType == FIELD_TYPE_FLOAT
			|| m_info.unType == FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_i64Val;
	}

	CField::operator int() const
	{
		if ((UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_LONGLONG
			|| m_info.unType == FIELD_TYPE_FLOAT
			|| m_info.unType == FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_i64Val;
	}

	CField::operator unsigned int() const
	{
		if (!(UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_LONGLONG
			|| m_info.unType == FIELD_TYPE_FLOAT
			|| m_info.unType == FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_i64Val;
	}

	CField::operator __int64() const
	{
		if ((UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_FLOAT
			|| m_info.unType == FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_i64Val;
	}

	CField::operator unsigned __int64() const
	{
		if (!(UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_FLOAT
			|| m_info.unType == FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_i64Val;
	}

	CField::operator float() const
	{
		if (m_info.unType != FIELD_TYPE_FLOAT)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_dVal;
	}

	CField::operator double() const
	{
		if (m_info.unType != FIELD_TYPE_DOUBLE
			&& m_info.unType != FIELD_TYPE_FLOAT)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		return m_dVal;
	}

	CField::operator char* () const
	{
		if (m_info.unType == FIELD_TYPE_STRING || m_info.unType == FIELD_TYPE_VAR_STRING ||
			m_info.unType == MYSQL_TYPE_TINY_BLOB || m_info.unType == MYSQL_TYPE_MEDIUM_BLOB ||
			m_info.unType == MYSQL_TYPE_LONG_BLOB || m_info.unType == MYSQL_TYPE_BLOB)
			return (char*)m_strVal.c_str();

		ASSERT(!("WARNNING: datatype mismatch , return unstable value"));
		return (char*)m_strVal.c_str();
	}

	CField::operator const char* () const
	{
		if (m_info.unType == FIELD_TYPE_STRING || m_info.unType == FIELD_TYPE_VAR_STRING ||
			m_info.unType == FIELD_TYPE_DATETIME || m_info.unType == FIELD_TYPE_TIME)
			return m_strVal.c_str();

		ASSERT(!("WARNNING: datatype mismatch , return unstable value"));
		return m_strVal.c_str();
	}

	//////////////////////////////////////////////////////////////////////////////
	IData&
		CField::operator = (bool bOp)
	{
		if (m_info.unType != (FIELD_TYPE_TINY))
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		//m_bVal = bOp;
		m_i64Val = bOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (char cOp)
	{
		if (UNSIGNED_FLAG & m_info.unAttr)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_i64Val = cOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (unsigned char ucOp)
	{
		if (!(UNSIGNED_FLAG & m_info.unAttr))
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_i64Val = ucOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (short sOp)
	{
		if ((UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_TINY)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_i64Val = sOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (unsigned short usOp)
	{
		if (!(UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_TINY)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_i64Val = usOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (long lOp)
	{
		if ((UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_TINY
			|| m_info.unType == FIELD_TYPE_SHORT)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_i64Val = lOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (unsigned long ulOp)
	{
		if (!(UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_TINY
			|| m_info.unType == FIELD_TYPE_SHORT)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_i64Val = ulOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (int iOp)
	{
		if ((UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_TINY
			|| m_info.unType == FIELD_TYPE_SHORT)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_i64Val = iOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (unsigned int uiOp)
	{
		if (!(UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_TINY
			|| m_info.unType == FIELD_TYPE_SHORT)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_i64Val = uiOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (__int64 i64Op)
	{
		if ((UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_TINY
			|| m_info.unType == FIELD_TYPE_SHORT
			|| m_info.unType == FIELD_TYPE_LONG)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_i64Val = i64Op;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (unsigned __int64 ui64Op)
	{
		if (!(UNSIGNED_FLAG & m_info.unAttr)
			|| m_info.unType == FIELD_TYPE_TINY
			|| m_info.unType == FIELD_TYPE_SHORT
			|| m_info.unType == FIELD_TYPE_LONG)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_i64Val = ui64Op;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (float fOp)
	{
		if (m_info.unType != FIELD_TYPE_FLOAT
			|| m_info.unType != FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_dVal = fOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (double dbOp)
	{
		if (m_info.unType != FIELD_TYPE_DOUBLE)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_dVal = dbOp;
		m_bChanged = true;
		return *this;
	}

	IData&
		CField::operator = (const char* pszVal)
	{
		if (m_info.unType != FIELD_TYPE_STRING
			&& m_info.unType != FIELD_TYPE_VAR_STRING)
			ASSERT(!("WARNNING: datatype mismatch , return unstable value"));

		m_strVal = pszVal;
		m_bChanged = true;
		return *this;
	}

	//////////////////////////////////////////////////////////////////
	bool CField::SetValue(const char* szValue)
	{
		switch (m_info.unType)
		{
			//case FIELD_TYPE_CHAR:
		case FIELD_TYPE_TINY:
		case FIELD_TYPE_SHORT:
		case FIELD_TYPE_LONG:
		case FIELD_TYPE_LONGLONG:
			m_i64Val = szValue ? _atoi64(szValue) : 0;
			break;

		case FIELD_TYPE_FLOAT:
		case FIELD_TYPE_DOUBLE:
			m_dVal = szValue ? atof(szValue) : 0.0f;
			break;

		case FIELD_TYPE_STRING:
		case FIELD_TYPE_VAR_STRING:
		case MYSQL_TYPE_TINY_BLOB:
		case MYSQL_TYPE_MEDIUM_BLOB:
		case MYSQL_TYPE_LONG_BLOB:
		case MYSQL_TYPE_BLOB:
		case FIELD_TYPE_TIME:
		case FIELD_TYPE_DATETIME:
			m_strVal = szValue ? szValue : "";
			break;

		default:
			sbase::LogSave("rade_db", "ERROR: CField::SetValue unknow field type:%u", m_info.unType);
			return false;
		}

		m_bChanged = true;
		return true;
	}
}