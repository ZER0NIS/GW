//////////////////////////////////////////////////////////////////////////
#ifndef EXCEPTION_IEXCEPTION_H
#define EXCEPTION_IEXCEPTION_H

#include <windows.h>

namespace sbase
{
	//////////////////////////////////////////////////////////////////////
	bool  StackTrace(char* szBuf);                   //��ӡ��ǰ�̵߳���ջ(ע���ǵ�ǰ�߳�)
	bool  StackTrace(char* szBuf, DWORD dwThread);   //��ӡָ���̵߳���ջ(��ǰ�̴߳�ӡָ���������߳�)

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