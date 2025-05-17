// ********************************************************************
//
// Copyright (c) 2004
// All Rights Reserved
//
// Author: zeng cs
// Created: 2005/12/1
//
// ********************************************************************

//#include <memory.h>
#include "..\inc\Msg.h"
#include "..\..\..\..\Common\MsgTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace sbase
{
	//////////////////////////////////////////////////////////////////////
	void
		CMsg::Init()
	{
		::memset(m_bufMsg, 0L, sizeof(m_bufMsg));
		m_head.usSize = 0;
		m_head.usType = _MSG_INVALID;
	}

	//////////////////////////////////////////////////////////////////////
	bool
		CMsg::IsValid(void) const
	{
		if (this->GetHeadSize() >= this->GetSize())
			return false;

		if (_MSG_INVALID == this->GetType())
			return false;

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	// virtual
	//////////////////////////////////////////////////////////////////////
	bool
		CMsg::Create(const void* pszBufMsg, USHORT nsMsgSize)
	{
		if (!pszBufMsg)
			return false;

		MsgHead* pHead = (MsgHead*)pszBufMsg;
		if ((USHORT)nsMsgSize != pHead->usSize)
			return false;

		if (_MSG_INVALID == pHead->usType)
			return false;

		if (nsMsgSize > 2048)
			ASSERT(nsMsgSize <= 2048);

		::memcpy(m_bufMsg, pszBufMsg, nsMsgSize);

		return true;
	}
}