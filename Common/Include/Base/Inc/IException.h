//////////////////////////////////////////////////////////////////////////
#ifndef EXCEPTION_IEXCEPTION_H
#define EXCEPTION_IEXCEPTION_H

#include <windows.h>

namespace sbase
{
	//////////////////////////////////////////////////////////////////////
	bool  StackTrace(char* szBuf);                   //打印当前线程调用栈(注意是当前线程)
	bool  StackTrace(char* szBuf, DWORD dwThread);   //打印指定线程调用栈(当前线程打印指定的其他线程)

	//////////////////////////////////////////////////////////////////////
	class  IException 
	{
	public:
		virtual const char*	    GetName() const         = 0;
		virtual bool			TraceStack(char* buf)   = 0;
		virtual EXCEPTION_POINTERS* GetExceptionInfo(void) = 0;
	};
}

#endif