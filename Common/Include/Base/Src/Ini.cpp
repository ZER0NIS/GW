#pragma warning(disable:4786)
#include "..\inc\SvrBase.h"
#include "String.h"
#include "..\inc\Ini.h"

namespace sbase
{
	CIni::CIni(bool bCritical) : m_bCritical(bCritical)
	{
	}

	CIni::CIni(const char* pszFile, bool bCritical) : m_bCritical(bCritical)
	{
		bool bSucOpen = this->Open(pszFile);
		if (!bSucOpen && m_bCritical)
 sbase::ErrorMsg("%s open error.", pszFile);

		m_strFileName = pszFile;
	}

	CIni::~CIni()
	{
	}

	//////////////////////////////////////////////////////////////////////
	bool CIni::SearchSection(const char* pszSection) const
	{
		return (m_setSection.find(sbase::sstring(pszSection)) != m_setSection.end());
	}

	//////////////////////////////////////////////////////////////////////
	bool CIni::IsValidLine(const char* szLine) const
	{
		IF_NOT(szLine)
 return false;

		// length check
		if (::strlen(szLine) <= 2)
 return false;

		// first char check
		bool bValidLine = true;
		switch (szLine[0])
		{
		case '/':
		case '\\':
		case ';':
		case '=':
		case ' ':
		case '\t':
		case '\r':
		case 0x0a:
 bValidLine = false;
 break;

		default:
 break;
		}

		return bValidLine;
	}

	//////////////////////////////////////////////////////////////////////
	bool CIni::ParseSection(char* szLine, sbase::sstring& str) const
	{
		IF_NOT(szLine)
 return false;

		if ('[' != szLine[0])
 return false;

		int nStrLen = (int)::strlen(szLine);
		int i = 1;
		for (; i < nStrLen; i++)
		{
 if (']' == szLine[i])
 {
 	szLine[i] = '\0';
 	break;
 }
		}

		if (i >= nStrLen)	// not valid section line
 return false;

		// section line found!
		str = szLine + 1;

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	bool CIni::ParseContent(char* szLine, sbase::sstring& strIndex, sbase::sstring& strContent) const
	{
		// valid check
		if (!this->IsValidLine(szLine))
 return false;

		// parse index
		int nLen = (int)::strlen(szLine);
		int i = 0;
		for (; i < nLen; i++)
		{
 if ('=' == szLine[i])
 {
 	szLine[i] = '\0';

 	int idx = i;
 	while (idx > 0)
 	{
 		--idx;

 		int c = szLine[idx];
 		if (' ' != c && '\t' != c)
  break;
 		else
  szLine[idx] = '\0';
 	}

 	strIndex = szLine;
 	break;
 }
		}

		if (i >= nLen)		// no '=' found, not valid line
 return false;

		// string forward
		if (i + 1 >= nLen)	// empty content
 return true;

		szLine += (i + 1);	// now szLine is string behind '='

		// remove format char
		nLen = (int)::strlen(szLine);
		for (i = 0; i < nLen; i++)
		{
 int c = szLine[i];
 if (' ' != c && '\t' != c)
 	break;
		}

		szLine += i;		// now szLine is string after format char

		// parse content
		nLen = (int)::strlen(szLine);
		for (i = 0; i < nLen; i++)
		{
 if (/*' ' == szLine[i] ||*/ '\t' == szLine[i] || ';' == szLine[i]
 	|| '\r' == szLine[i] || 0x0a == szLine[i])
 {
 	szLine[i] = '\0';
 	break;
 }
		}

		strContent = szLine;
		return true;
	}

	//////////////////////////////////////////////////////////////////////
	bool CIni::Open(const char* pszIniFile)
	{
		IF_NOT(pszIniFile)
 return false;

		m_setSection.clear();

		FILE* fp = fopen(pszIniFile, "r");
		if (!fp)
 return false;

		SECTION section;
		sbase::sstring strTitle;

		char szLine[1024] = "";
		for (;;)
		{
 if (NULL == fgets(szLine, sizeof(szLine), fp))
 {
 	// save section info
 	if (!strTitle.empty())
 		m_setSection[strTitle] = section;

 	break;
 }

 // string length chk
 int nStrLen = (int)::strlen(szLine);
 if (nStrLen <= 2)
 	continue;

 // get rid of end char
 if (0x0a == szLine[nStrLen - 1])
 	szLine[nStrLen - 1] = 0;

 // parse now
 sbase::sstring str;
 if (this->ParseSection(szLine, str))
 {
 	// a new section found, keep the old one
 	if (!strTitle.empty())
 		m_setSection[strTitle] = section;

 	// replace section title string
 	strTitle = str;

 	// clear old section
 	section.setInfo.clear();
 }
 else
 {
 	if (!strTitle.empty())
 	{
 		// read section content
 		sbase::sstring strIndex, strContent;
 		if (this->ParseContent(szLine, strIndex, strContent))
 		{
  section.setInfo[strIndex] = strContent;
 		}
 	}
 }
		}

		fclose(fp);

		m_strFileName = pszIniFile;
		return true;
	}

	//////////////////////////////////////////////////////////////////////
	int CIni::GetData(const char* pszSection, const char* pszIndex) const
	{
		IF_NOT(pszSection && pszIndex)
 return 0;

		// search section
		std::map<sbase::sstring, SECTION>::const_iterator iter = m_setSection.find(sbase::sstring(pszSection));
		if (iter == m_setSection.end())
		{
 if (m_bCritical)
 	sbase::ErrorMsg("section[%s] not found in %s!", pszSection, m_strFileName.c_str());

 return 0;
		}

		// search content
		std::map<sbase::sstring, sbase::sstring>::const_iterator iter2 = (*iter).second.setInfo.find(sbase::sstring(pszIndex));
		if (iter2 == (*iter).second.setInfo.end())
		{
 if (m_bCritical)
 	sbase::ErrorMsg("section[%s], index[%s] not found in %s!", pszSection, pszIndex, m_strFileName.c_str());

 return 0;
		}

		return (*iter2).second;
	}

	// --------------------------------------------------------------------------
	// [3/23/2007 Fenjune]
	// --------------------------------------------------------------------------
	float CIni::GetFloatData(const char* pszSection, const char* pszIndex) const
	{
		const char* IniStr = GetString(pszSection, pszIndex);
		if (IniStr == NULL)
 return 0.0f;
		return (float)atof(IniStr);
	}

	//////////////////////////////////////////////////////////////////////
	const char* CIni::GetString(const char* pszSection, const char* pszIndex) const
	{
		IF_NOT(pszSection && pszIndex)
 return NULL;

		// search section
		std::map<sbase::sstring, SECTION>::const_iterator iter = m_setSection.find(sbase::sstring(pszSection));
		if (iter == m_setSection.end())
		{
 if (m_bCritical)
 	sbase::ErrorMsg("section[%s] not found in %s!", pszSection, m_strFileName.c_str());

 return NULL;
		}

		// search content
		std::map<sbase::sstring, sbase::sstring>::const_iterator iter2 = (*iter).second.setInfo.find(sbase::sstring(pszIndex));
		if (iter2 == (*iter).second.setInfo.end())
		{
 if (m_bCritical)
 	sbase::ErrorMsg("section[%s], index[%s] not found in %s!", pszSection, pszIndex, m_strFileName.c_str());

 return NULL;
		}

		return (*iter2).second;
	}

	//////////////////////////////////////////////////////////////////////
	// static
	//////////////////////////////////////////////////////////////////////
	CIni* CIni::CreateNew(const char* pszIniFile, bool bCritical)
	{
		IF_NOT(pszIniFile)
 return NULL;

		CIni* pMyIni = new CIni(bCritical);
		IF_NOT(pMyIni)
 return NULL;

		if (!pMyIni->Open(pszIniFile))
		{
 delete pMyIni;
 return NULL;
		}

		return pMyIni;
	}
}