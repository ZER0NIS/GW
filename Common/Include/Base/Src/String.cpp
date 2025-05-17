#include "..\inc\SvrBase.h"
#include "String.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace sbase
{
	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////
	sstring::sstring()
	{
	}

	//////////////////////////////////////////////////////////////////////
	sstring::sstring(const char* fmt, ...)
	{
		if (fmt)
		{
			try {
				char buffer[MAX_STRING] = "";
				vsprintf(buffer, fmt, (char*)((&fmt) + 1));
				this->assign(buffer);
			}
			catch (...)
			{
				//LogSave("Error: too big size of string in format.");
			}
		}
	}

	//////////////////////////////////////////////////////////////////////
	sstring::~sstring()
	{
	}

	//////////////////////////////////////////////////////////////////////
	void sstring::format(const char* fmt, ...)
	{
		if (!fmt)
			return;

		try {
			char buffer[MAX_STRING] = "";
			vsprintf(buffer, fmt, (char*)((&fmt) + 1));
			this->assign(buffer);
		}
		catch (...)
		{
			//LogSave("Error: too big size of string in format.");
		}
	}
}