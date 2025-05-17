#ifndef _DBGFUNC_H_
#define _DBGFUNC_H_

#include "windows.h"
#include <eh.h> 
#include "Exception.h"
#pragma warning(disable: 4200)	//nonstandard extension used : zero-sized array in struct/union

#pragma optimize( "y", off )	// 保证CALL FRAME不会被优化掉

#define TANSLATOR_CLASS(x)	class CTranslator##x{\
public:\
	static void s_TransFunction( unsigned int u, EXCEPTION_POINTERS* pExp )\
	{\
	throw sbase::Exception(pExp);\
	}\
	CTranslator##x():: CTranslator##x()\
	: OldFanc(NULL)	\
	{\
		OldFanc = _set_se_translator( CTranslator##x()::s_TransFunction );\
	}\
	CTranslator##x()::~CTranslator##x(){if(OldFanc) _set_se_translator(OldFanc);}\
	private:\
		_se_translator_function OldFanc;\
	}obj##x;

namespace sbase
{
	void TransFunction( unsigned int u, EXCEPTION_POINTERS* pExp );

	class CTranslator
	{
	public:	
		CTranslator();
		~CTranslator()					{if(OldFanc) _set_se_translator(OldFanc); }
	private:
		_se_translator_function OldFanc;
	};
}


#endif