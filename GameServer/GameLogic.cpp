#include "stdafx.h"
#include <mmsystem.h>
#include "GSMaster.h"
#include "World.h"

#include "DataCore.h"
#include "GameCore\Region.h"
#include "GameCore\RepRegion.h"
#include "GameCore\GameObject.h"
#include "GameCore\Player.h"
#include "GameCore\NPC.h"
#include "GameCore\Monster.h"
#include "GameCore\QuestManager.h"
#include "GameCore/RoleRelation.h"

#include "..\Common\MsgDefine.h"
#include "..\Common\MsgTypes.h"
#include "ServiceLoader.h"

extern MSG_SCENE_CHANGE msg_scene_change;

BYTE GSMaster::GetTotalPlayerNum()
{
	float per = (float)m_pWorld->GetNowPlayer() / (float)m_pWorld->GetAllowedOnline();
	BYTE status;
	if (per <= 0.60000f && per >= 0.00000f)
	{
		status = 1;
	}
	else if (per <= 0.800000f && per >= 0.60000f)
	{
		status = 2;
	}
	else
	{
		status = 3;
	}
	return status;
}

void   CWorld::OnNewEnterGameWorld()
{
	snet::CSocket* pNewSocket = m_pSrvIocp->PopNewConnect();

	while (NULL != pNewSocket)
	{
		assert(pNewSocket->m_pIOCP);
		CPlayer* pNewPlayer = JoinPlayer();

		pNewPlayer->SetSocket(pNewSocket);

		g_setPlayer.insert(make_pair(pNewPlayer, false));

		NewSocketProc(pNewPlayer);

		pNewSocket = m_pSrvIocp->PopNewConnect();
	}
}

void CWorld::NewSocketProc(CPlayer* pPlayer)
{
	char pszBuff[4096] = { 0 };

	snet::CSocket* pSocket = pPlayer->GetSocket();

	if (pSocket->IsValid())
	{
		while (pSocket->Read((char**)&pszBuff, 4096))
		{
			switch (((MsgHead*)&pszBuff)->usType)
			{
			case _MSG_LOGIN_GAMESERVER:

				m_pSocketHandle->ResloveGamePacket(pszBuff, pPlayer);

				break;
			default:
				return;
			}
		}
	}
	else
	{
		m_pSrvIocp->PushNewClose(pSocket);
		DelPlayer(pPlayer);
		pPlayer->DelayExit(0);
		if (pPlayer->IsMove())
		{
			printf("%s刚刚登录游戏服务器就退出游戏!", pPlayer->GetName());
		}
	}
}

void   CWorld::OnReadlyForGame()
{
	MAP_PLAYER::iterator itor = g_setPlayer.begin();

	while (itor != g_setPlayer.end())
	{
		CPlayer* p = itor->first;

		if (!(itor->first->OnRead()))
		{
			itor = g_setPlayer.erase(itor);
			if (p->IsMove())
			{
				printf("%s刚准备进入就异常退出游戏！\n", p->GetName());
				break;
			}
		}

		if (CheckPlayer(itor->first))
		{
			itor = g_setPlayer.erase(itor);
			break;
		}

		if (!(itor->first->OnSend()))
		{
			itor = g_setPlayer.erase(itor);

			if (p->IsMove())
			{
				printf("%s刚准备进入就异常退出游戏！\n", p->GetName());
				break;
			}
		}

		itor++;
	}
}

long CWorld::GameLogic(void)
{
	OnSceneChange();

	EntityAI();

	QuestManager::Instance()->UpData();

	g_pConsortiaManager->UpData();

	OnTradeDangerousPlayer();

	return 0;
}

long  CWorld::BackUpPlayer(void)
{
	static int RegionID = 0;
	int WillRegionID = RegionID % (g_lMainCity + g_lCity + g_lField);
	GetRegion(WillRegionID)->BackUpPlayerData();
	RegionID++;

	return 1;
}

long CWorld::BackUpConsortia(void)
{
	g_pConsortiaManager->BackUp_Consortia_info();
	return 1;
}

void CWorld::OnSceneChange()
{
	if (g_listSceneChange.size() != 0)
	{
		for (list< tarSceneChange* >::iterator it = g_listSceneChange.begin(); it != g_listSceneChange.end(); it++)
		{
			tarSceneChange* p = *it;
			p->pPlayer->ClearPath();
			p->pPlayer->SetfX(p->x);
			p->pPlayer->SetfZ(p->z);
			GetRegion(p->lFromSceneID)->RemoveObject(p->pPlayer->GetID());
			p->pPlayer->m_eState = OBJECT_IDLE;

			p->pPlayer->SetlRegionID(GetRegion(p->lToSceneID)->GetLogicID());
			msg_scene_change.uiUserID = p->pPlayer->GetID();
			msg_scene_change.X = p->pPlayer->GetPosX();
			msg_scene_change.Y = 0.0f;
			msg_scene_change.Z = p->pPlayer->GetPosZ();
			msg_scene_change.usCurMapID = (USHORT)GetRegion(p->lFromSceneID)->GetID();
			msg_scene_change.usNextMapID = (USHORT)GetRegion(p->lToSceneID)->GetID();
			SendMsgToClient(&msg_scene_change, p->pPlayer->GetSocket());
			g_setPlayer.insert(make_pair(p->pPlayer, false));

			SAFE_DELETE(p);
		}

		g_listSceneChange.clear();
	}
}

void CWorld::UpdateGameLoginQueue()
{
	sbase::CSingleLock xLock1(&m_xLock);
	std::vector<CPlayer*>::iterator itor = g_QueuePlayerVec.begin();
	int Index = 0;
	for (; itor != g_QueuePlayerVec.end(); )
	{
		CPlayer* pPlayer = *itor;
		if (!pPlayer->RefreshLoginQueue(Index))
		{
			itor = g_QueuePlayerVec.erase(itor);
		}
		else
		{
			itor++;
		}

		Index++;
	}
}

void CWorld::OnTradeDangerousPlayer()
{
	if (g_listKickPlayer.size() != 0)
	{
		for (list< CPlayer* >::iterator it = g_listKickPlayer.begin(); it != g_listKickPlayer.end(); it++)
		{
			CPlayer* pPlayer = *it;
			if (pPlayer->GetlFaction() == 1)
			{
				pPlayer->SetlRegionID(pPlayer->GetRegion()->GetAthensRevival()->MapID);
				pPlayer->SetfX(pPlayer->GetRegion()->GetAthensRevival()->X);
				pPlayer->SetfZ(pPlayer->GetRegion()->GetAthensRevival()->Z);
			}
			else if (pPlayer->GetlFaction() == 0)
			{
				pPlayer->SetlRegionID(pPlayer->GetRegion()->GetSpartaRevival()->MapID);
				pPlayer->SetfX(pPlayer->GetRegion()->GetSpartaRevival()->X);
				pPlayer->SetfZ(pPlayer->GetRegion()->GetSpartaRevival()->Z);
			}
			CloseSocket(pPlayer);
		}
		g_listKickPlayer.clear();
	}
}

void  CWorld::GMStat()
{
}

void   CWorld::EntityAI()
{
	static DWORD nAIFPSCount = 0;
	static DWORD nFPSCount = 0;
	static long lUpdateMagicCount = 0;

	if (m_TimerArray[TIMER_LOGIC].IsExpire())
	{
		nAIFPS = nAIFPSCount;
		nAIFPSCount = 0;
		nFPS = nFPSCount;
		nFPSCount = 0;
		m_TimerArray[TIMER_LOGIC].Update();
	}

	nFPSCount++;

	if (m_TimerArray[TIMER_LOGIC].IsExpire((nAIFPSCount + 1) * LOGIC_TIMEOUT))
	{
		nAIFPSCount++;

		for (int i = 0; i < g_lAllScene; i++)
			g_pRegion[i].AI();

		g_pRegion[lUpdateMagicCount].UpdateMagicRegion();
		lUpdateMagicCount++;
		if (lUpdateMagicCount >= g_lAllScene)
			lUpdateMagicCount = 0;

		for (int i = 0; i < g_lDuplicate; i++)
		{
			GetRepRegion(i)->AI();
		}
	}
}