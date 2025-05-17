#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "..\inc\SvrBase.h"
#include "..\inc\IniFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace sbase
{
	CIniFile::CIniFile(const char* pFilename, const char* pSection)
	{
		if (!pFilename || ::strlen(pFilename) > _MAX_INISIZE)
		{
 m_szFilename[0] = 0;
 ASSERT("CIniFile::CIniFile() parameter error.");
 return;
		}

		sbase::StrCopy(m_szFilename, pFilename, (UINT)::strlen(pFilename));

		if (!pSection || strlen(pSection) > _MAX_INISIZE)
		{
 m_szSection[0] = 0;
 ASSERT("CIniFile::CIniFile() parameter error.");
 return;
		}

		sbase::StrCopy(m_szSection, pSection, (UINT)::strlen(pSection));
	}

	CIniFile::~CIniFile()
	{
	}

	//////////////////////////////////////////////////////////////////////
	bool
		CIniFile::GetString(const char* pKey, char* szBuf, int nBufSize)
	{
		if (!szBuf)
		{
 ASSERT("CIniFile::GetString() parameter error.");
 return false;
		}

		szBuf[0] = 0;

		int	nKeyLen = (int)::strlen(pKey);
		ASSERT(nKeyLen > 0);
		ASSERT(nKeyLen < _MAX_INISIZE);

		FILE* pFile = ::fopen(m_szFilename, "r");
		if (!pFile)
		{
 ASSERT("CIniFile::GetString() open file fail!");
 return false;
		}

		int	nSectLen = (int)::strlen(m_szSection);
		char buf[_MAX_INISIZE * 2];
		while (!feof(pFile))
		{
 // read a line
 ::fgets(buf, _MAX_INISIZE * 2 - 1, pFile);

 // find '[SECTION]'
 char* pBuf = ::strchr(buf, '[');
 if (NULL == pBuf || ';' == *pBuf)
 	continue;

 int i = 1;
 for (; i < _MAX_INISIZE * 2 - 1; i++)
 	if (0x20 != pBuf[i]) break;

 if ((0 > ::strnicmp(pBuf + i, m_szSection, nSectLen))
 	|| (']' != pBuf[nSectLen + 1]))
 	continue;

 // find KEY
 do
 {
 	// read a line
 	::fgets(buf, _MAX_INISIZE * 2 - 1, pFile);

 	// find next '[SECTION]'
 	char* pBuf = ::strchr(buf, '[');
 	if (pBuf)
 		break;

 	// find pKey
 	pBuf = ::strchr(buf, '=');
 	if (NULL == pBuf || ';' == *pBuf)
 		continue;

 	for (i = 1; i < _MAX_INISIZE * 2 - 1; i++)
 		if (' ' != *(pBuf - i))  break;

 	if (0 != ::strnicmp(pBuf - nKeyLen - i + 1, pKey, nKeyLen))
 		continue;

 	// begin to get str
 	sbase::StrCopy(szBuf, pBuf + 1, nBufSize - 1);
 	//20070109
 	szBuf[::strlen(szBuf) - 1] = 0;

 	// close file
 	::fclose(pFile);
 	return true;
 } while (!feof(pFile));

 break;
		}

		::fclose(pFile);
		return false;
	}

	//////////////////////////////////////////////////////////////////////
	int
		CIniFile::GetInt(const char* pKey)
	{
		char buf[_MAX_INISIZE];
		if (this->GetString(pKey, buf, _MAX_INISIZE))
 return ::atoi(buf);

		return 0;
	}
}