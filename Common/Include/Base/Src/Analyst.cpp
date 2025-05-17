#include <stdio.h>
#include <time.h>
#include "../inc/Analyst.h"
#include "../inc/SysTimer.h"
#include "../inc/SvrBase.h"
using namespace sbase;
////////////////////////////////////////////////////////////////////////////
// CONST
const char szAnalystLog[] = "analyst";

CAnalyst&
CAnalyst::Instance()
{
	static CAnalyst objAnalyst;
	return objAnalyst;
}
////////////////////////////////////////////////////////////////////////////
CAnalyst::CAnalyst()
{
	Create();
}

////////////////////////////////////////////////////////////////////////////
CAnalyst::~CAnalyst()
{
#ifdef KXJ_STREAM
	this->LogToDisk();
#endif
	for (int i = 0; i < m_setStatis.size(); i++) {
		Statis_t* pStatis = m_setStatis[i];
		if (pStatis->name)
 delete[]pStatis->name;
		delete pStatis;
	}

	STATIS_MAP::iterator iter;
	for (iter = m_mapStatis.begin(); iter != m_mapStatis.end(); ++iter) {
		Statis_t* pStatis = iter->second;
		if (pStatis != NULL) {
 if (pStatis->name)
 	delete[]pStatis->name;
 delete pStatis;
		}
	}
}

////////////////////////////////////////////////////////////////////////////
bool CAnalyst::Create()
{
	for (int i = 0; i < ANALYST_MAX_ITEM; i++) {
		Statis_t* pStatis = new Statis_t;
		if (pStatis == NULL)
 return false;
		memset(pStatis, 0, sizeof(Statis_t));
		m_setStatis.push_back(pStatis);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////
void CAnalyst::ReStart()
{
	// lock
	sbase::CSingleLock  lock(&m_xLock);

	for (int i = 0; i < m_setStatis.size(); i++) {
		Statis_t* pStatis = m_setStatis[i];
		if (pStatis)
 ClearData(pStatis);
	}

	STATIS_MAP::iterator iter;
	for (iter = m_mapStatis.begin(); iter != m_mapStatis.end(); ++iter) {
		Statis_t* pStatis = iter->second;
		if (pStatis)
		{
 ClearData(pStatis);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
void CAnalyst::ClearData(Statis_t* pStatis)
{
	if (pStatis == NULL)
		return;

	if (pStatis->mask & ANALYST_MASK_TICKS) {
		pStatis->totalTicks = 0;
		pStatis->maxTicks = 0;
		pStatis->countTicks = 0;
	}

	if (pStatis->mask & ANALYST_MASK_SIZE) {
		pStatis->totalSize = 0;
		pStatis->maxSize = 0;
		pStatis->countSize = 0;
	}
}

////////////////////////////////////////////////////////////////////////////
bool CAnalyst::AnalystAdd(int idx, int mask, const char* name)
{
	IF_NOT(idx >= 0 && idx < ANALYST_MAX_ITEM)
	{
		return false;
	}
	IF_NOT(mask != ANALYST_MASK_INVALID)
	{
		return false;
	}
	IF_NOT(name != NULL)
	{
		return false;
	}
	IF_NOT(strlen(name) <= 100)
	{
		return false;
	}

	Statis_t* pStatis = m_setStatis[idx];

	// mask
	IF_NOT(pStatis->mask == ANALYST_MASK_INVALID)
	{
		return false;
	}
	pStatis->mask = mask;

	// name
	int len = strlen(name) + 1;
	pStatis->name = new char[len];
	strcpy(pStatis->name, name);
	pStatis->name[len - 1] = 0;

	return true;
}

////////////////////////////////////////////////////////////////////////////
void CAnalyst::TicksAdd(int idx, I64 ticks)
{
	// lock
	sbase::CSingleLock  lock(&m_xLock);

	if (idx < 0 || idx >= ANALYST_MAX_ITEM)
		return;

	Statis_t* pStatis = m_setStatis[idx];

	// Is ticks counter
	if (!(pStatis->mask & ANALYST_MASK_TICKS))
		return;

	// Count ticks
	pStatis->totalTicks += ticks;
	pStatis->countTicks++;

	if (ticks > pStatis->maxTicks)
	{
		pStatis->maxTicks = ticks;
	}
}

////////////////////////////////////////////////////////////////////////////
void CAnalyst::SizeAdd(int idx, I64 size)
{
	// lock
	sbase::CSingleLock  lock(&m_xLock);

	if (idx < 0 || idx >= ANALYST_MAX_ITEM)
		return;

	Statis_t* pStatis = m_setStatis[idx];

	// Is size counter
	if (!(pStatis->mask & ANALYST_MASK_SIZE))
		return;

	// Inc size
	//InterlockedExchangeAdd(&(pStatis->totalSize), size);
	pStatis->totalSize += size;
	pStatis->countSize++;
}

////////////////////////////////////////////////////////////////////////////
// Not thread safe.
void CAnalyst::LogTicksToString(Statis_t* pStatis, char* szBuf, int nBufSize)
{
	IF_NOT(pStatis->mask & ANALYST_MASK_TICKS)
	{
		return;
	}

	memset(szBuf, 0, nBufSize);

	// Total and average and the max ms
	I64 totalMS = CSysTimer::Instance().Sys_TicksToMS(pStatis->totalTicks);
	I64 averageMS = totalMS;
	if (pStatis->countTicks > 0)
		averageMS = totalMS / pStatis->countTicks;
	I64 maxMS = CSysTimer::Instance().Sys_TicksToMS(pStatis->maxTicks);

	sprintf(szBuf, "%s£ºcount[%I64u], total used[%I64u]ms, average[%I64u]ms, max[%I64u]ms.\n", pStatis->name,
		pStatis->countTicks, totalMS, averageMS, maxMS);
}

////////////////////////////////////////////////////////////////////////////
// Not thread safe.
void CAnalyst::LogSizeToString(Statis_t* pStatis, char* szBuf, int nBufSize)
{
	IF_NOT(pStatis->mask & ANALYST_MASK_SIZE)
	{
		return;
	}

	memset(szBuf, 0, nBufSize);

	I64 totalSize = pStatis->totalSize;
	I64 average = totalSize;
	if (pStatis->countSize > 0)
		average = totalSize / pStatis->countSize;
	I64 max = pStatis->maxSize;

	sprintf(szBuf, "%s£ºcount[%I64u], size[%I64u], average[%I64u], max[%I64u].\n",
		pStatis->name, pStatis->countSize, totalSize, average, max);
}

////////////////////////////////////////////////////////////////////////////
// Log to disk.
// Not thread safe.
void CAnalyst::LogToConsole()
{
}

//////////////////////////////////////////////////////////////////////////////////
void CAnalyst::LogToDisk()
{
	const int BUFLEN = 1024;
	char szBuf[BUFLEN];

	time_t ltime;
	::time(&ltime);
	tm* pTm = localtime(&ltime);
	sprintf(szBuf, "==============%02d:%02d:%02d==============\n",
		pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
	sbase::LogSave(szAnalystLog, szBuf);

	I64 nTotalTick = 0;
	for (int i = 0; i < m_setStatis.size(); i++) {
		Statis_t* pStatis = m_setStatis[i];
		if (pStatis->mask == ANALYST_MASK_INVALID)
 continue;

		// Ticks
		if (pStatis->mask & ANALYST_MASK_TICKS) {
 LogTicksToString(pStatis, szBuf, BUFLEN);
 sbase::LogSave(szAnalystLog, szBuf);
 nTotalTick += pStatis->totalTicks;
		}

		// Size
		if (pStatis->mask & ANALYST_MASK_SIZE) {
 LogSizeToString(pStatis, szBuf, BUFLEN);
 sbase::LogSave(szAnalystLog, szBuf);
		}
	}

	STATIS_MAP::iterator iter;
	for (iter = m_mapStatis.begin(); iter != m_mapStatis.end(); ++iter) {
		Statis_t* pStatis = iter->second;

		if (pStatis != NULL) {
 // Ticks
 if (pStatis->mask & ANALYST_MASK_TICKS) {
 	LogTicksToString(pStatis, szBuf, BUFLEN);
 	sbase::LogSave(szAnalystLog, szBuf);
 	nTotalTick += pStatis->totalTicks;
 }

 // Size
 if (pStatis->mask & ANALYST_MASK_SIZE) {
 	LogSizeToString(pStatis, szBuf, BUFLEN);
 	sbase::LogSave(szAnalystLog, szBuf);
 }
		}
	}

	sprintf(szBuf, "Total used: %I64d ms.\n", CSysTimer::Instance().Sys_TicksToMS(nTotalTick));
	sbase::LogSave(szAnalystLog, szBuf);
}

//////////////////////////////////////////////////////////////////////////////////
void CAnalyst::TicksAdd(const char* szKey, I64 ticks)
{
	// lock
	sbase::CSingleLock  lock(&m_xLock);

	Statis_t* pStatis = m_mapStatis[szKey];
	if (pStatis == NULL) {
		pStatis = new Statis_t;
		memset(pStatis, 0, sizeof(Statis_t));

		// Name
		int len = strlen(szKey) + 1;
		pStatis->name = new char[len];
		strcpy(pStatis->name, szKey);
		pStatis->name[len - 1] = 0;

		m_mapStatis[szKey] = pStatis;
	}

	if (ticks > 1795540000)
	{
		INT k = 0;
	}
	// Analyst type allow count ticks
	pStatis->mask |= ANALYST_MASK_TICKS;

	pStatis->totalTicks += ticks;
	++pStatis->countTicks;

	if (ticks > pStatis->maxTicks)
		pStatis->maxTicks = ticks;
}

//////////////////////////////////////////////////////////////////////////////////
void CAnalyst::SizeAdd(const char* szKey, I64 size)
{
	// lock
	sbase::CSingleLock  lock(&m_xLock);

	Statis_t* pStatis = m_mapStatis[szKey];
	if (pStatis == NULL) {
		pStatis = new Statis_t;
		memset(pStatis, 0, sizeof(Statis_t));

		// Name
		int len = strlen(szKey) + 1;
		pStatis->name = new char[len];
		strcpy(pStatis->name, szKey);
		pStatis->name[len - 1] = 0;

		m_mapStatis[szKey] = pStatis;
	}

	// Analyst type allow count size
	pStatis->mask |= ANALYST_MASK_SIZE;

	pStatis->totalSize += size;
	++pStatis->countSize;
	if (size > pStatis->maxSize)
		pStatis->maxSize = size;
}

void CAnalyst::AmountAdd(const char* szKey, I64 nAmount)
{
}