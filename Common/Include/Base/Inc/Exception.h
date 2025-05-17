#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include <windows.h>
#include "IException.h"
namespace sbase
{
	class Exception : public IException
	{
	public:	
		const char*		GetName() const;
		bool TraceStack(char* buf);
		EXCEPTION_POINTERS* GetExceptionInfo(void);
	public:
		Exception(EXCEPTION_POINTERS * m_pExp);	
		Exception(const Exception& se) : m_pExp(se.m_pExp) {}
		~Exception() {}
		
	private:
		EXCEPTION_POINTERS * m_pExp;		
	};

}
#endif