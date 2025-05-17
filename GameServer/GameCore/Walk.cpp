#include "StdAfx.h"
#include "Region.h"
#include "GameObject.h"
#include "Player.h"
#include "NPC.h"
#include "Monster.h"
#include "Map.h"

#include "..\..\Common\MsgDefine.h"
#include "..\..\Common\MsgTypes.h"
#include "..\errormessage.h"

extern MSG_WALK_BEGIN msg_walk_begin;
extern MSG_WALK_END msg_walk_end;
extern MSG_WALK msg_walk;

long CRegion::Walk(CPlayer* pPlayer, float x, float z, float fatan, long lState)
{
	if (!pPlayer)
		return 0;

	auto p = std::make_unique<tarWalk>();
	p->x = x;
	p->z = z;
	p->fatan = fatan;

	pPlayer->m_fatan2 = fatan;
	pPlayer->m_queueWalk.push_back(std::move(p));

	msg_walk.uiID = pPlayer->GetID();
	msg_walk.usMapID = pPlayer->GetRegion()->GetID();
	msg_walk.fatan = fatan;
	msg_walk.x = x;
	msg_walk.z = z;
	msg_walk.lState = lState;
	SendAreaMsgOneToOther(pPlayer->GetCurrentCell(), &msg_walk);

	static float fPX = 0.0f;
	static float fPZ = 0.0f;
	static int loop = 0;
	if (loop > 100)
	{
		// Validación anti-cheat: comprobar si el movimiento es demasiado grande (posible hack de velocidad)
		if (!pPlayer->m_queueWalk.empty())
		{
			auto it = pPlayer->m_queueWalk.end();
			it--;
			fPX = x - (*it)->x;
			fPZ = z - (*it)->z;
			float Dist = sqrt(fPX * fPX + fPZ * fPZ);
			if (Dist > 0.3f)
			{
				pPlayer->AddDanger();
				sbase::LogException("Message type Exception ,Accounts : %s, Role: %s,[_MSG_WALK] Type:Walk.", pPlayer->GetAccounts(), pPlayer->GetName());
			}
		}

		loop = 0;
	}
	else
		loop++;

	return 0;
}

eError CRegion::WalkBegin(CPlayer* pPlayer, float x, float, float z, float offset_x, float, float offset_z, unsigned int tick)
{
	if (pPlayer->m_eState == OBJECT_DEAD)
		return NO_MSG_ERRO;

	// Crear un nuevo path usando unique_ptr
	auto pPath = std::make_unique<tarPath>();

	pPath->x = x;
	pPath->y = 0.0f;
	pPath->z = z;
	pPath->end_x = x;
	pPath->end_y = 0.0f;
	pPath->end_z = z;
	pPath->nTick = 0;
	pPath->nEndTick = 0;
	pPath->OffsetX = offset_x;
	pPath->OffsetZ = offset_z;

	PathIterator it;
	tarPath* pPerPath = nullptr;

	if (pPlayer->m_queuePath.size() == 1)
	{
		it = pPlayer->m_queuePath.begin();
		pPerPath = it->get();

		if (tick > pPerPath->nTick)
		{
			pPerPath->nEndTick = tick;
			pPath->end_x = x;
			pPath->end_z = z;
		}
		else if (tick == pPerPath->nTick)
		{
			pPlayer->SetPos(x, 0.0f, z);
			pPlayer->m_queuePath.erase(it);
		}
		else
		{
			// Movimiento hacia atrás en el tiempo, ajustar posición en base a offset
			float temp_x = pPlayer->GetPosX();
			float temp_z = pPlayer->GetPosZ();

			// Limitar el número de iteraciones para evitar bucles infinitos o largos
			unsigned int maxIterations = 100; // Valor razonable para prevenir CPU excesivo
			unsigned int iterations = min(pPerPath->nTick - tick, maxIterations);

			for (unsigned int i = 0; i < iterations; i++)
			{
				temp_x -= pPerPath->OffsetX;
				temp_z -= pPerPath->OffsetZ;
			}

			pPlayer->SetPos(x, 0.0f, z);
			pPlayer->m_queuePath.erase(it); // unique_ptr libera automáticamente
		}
	}
	else if (pPlayer->m_queuePath.size() > 1)
	{
		it = pPlayer->m_queuePath.end();
		it--;
		pPerPath = it->get();
		pPerPath->nEndTick = tick;
		pPerPath->end_x = x;
		pPerPath->end_z = z;

		if (pPerPath->nEndTick == 0)
			pPlayer->m_queuePath.erase(it);
	}
	else
	{
		pPlayer->m_fatan2 = atan2(offset_x, offset_z);

		if (pPlayer->m_queuePath.empty())
		{
			return NO_MSG_ERRO; // Ya no es necesario SAFE_DELETE, unique_ptr se encarga
		}
	}

	// Mover (transferir propiedad) de pPath a la cola
	pPlayer->m_queuePath.push_back(std::move(pPath));

	if (pPlayer->m_eState == OBJECT_IDLE)
		pPlayer->m_eState = OBJECT_RUN;

	if (pPlayer->m_queuePath.size() == 1)
	{
		msg_walk_begin.X = pPath->x;
		msg_walk_begin.Y = 0.0f;
		msg_walk_begin.Z = pPath->z;
		msg_walk_begin.uiTick = tick;
		msg_walk_begin.OffsetX = pPath->OffsetX;
		msg_walk_begin.OffsetY = 0.0f;
		msg_walk_begin.OffsetZ = pPath->OffsetZ;
		msg_walk_begin.uiID = pPlayer->GetID();
		msg_walk_begin.usMapID = (USHORT)this->GetID();
		SendAreaMsgOneToOther(pPlayer->GetCurrentCell(), &msg_walk_begin);
	}

	return NO_MSG_ERRO;
}

long CRegion::WalkEnd(CPlayer* pPlayer, float x, float, float z, float atan2, unsigned int tick, unsigned int)
{
	// Early return para simplificar el código
	if (!pPlayer || pPlayer->m_queuePath.empty())
		return 0;

	PathIterator it;
	tarPath* pPerPath = nullptr;

	if (pPlayer->m_queuePath.size() == 1)
	{
		it = pPlayer->m_queuePath.begin();
		pPerPath = it->get();

		if (tick > pPerPath->nTick)
		{
			pPerPath->nEndTick = tick;
			pPerPath->end_x = x;
			pPerPath->end_z = z;
		}
		else if (tick == pPerPath->nTick)
		{
			pPlayer->m_queuePath.erase(it);

			if (pPlayer->m_eState == OBJECT_RUN)
				pPlayer->m_eState = OBJECT_IDLE;

			pPlayer->SetPos(x, 0.0f, z);

			msg_walk_end.X = pPlayer->GetPosX();
			msg_walk_end.Y = 0.0f;
			msg_walk_end.Z = pPlayer->GetPosZ();
			msg_walk_end.uiTick = tick;
			msg_walk_end.fAtan2 = pPlayer->m_fatan2;
			msg_walk_end.uiID = pPlayer->GetID();
			msg_walk_end.usMapID = (USHORT)this->GetID();
			SendAreaMsgOneToOther(pPlayer->GetCurrentCell(), &msg_walk_end);
		}
		else
		{
			float temp_x, temp_z;
			temp_x = pPlayer->GetPosX();
			temp_z = pPlayer->GetPosZ();

			// Limitar el número máximo de iteraciones para prevenir bucles largos
			unsigned int maxIterations = 100;
			unsigned int iterations = min(pPerPath->nTick - tick, maxIterations);

			for (unsigned int i = 0; i < iterations; i++)
			{
				temp_x -= pPerPath->OffsetX;
				temp_z -= pPerPath->OffsetZ;
			}
			pPlayer->SetPos(x, 0.0f, z);

			pPlayer->m_queuePath.erase(it); // unique_ptr se destruye automáticamente

			if (pPlayer->m_eState == OBJECT_RUN)
				pPlayer->m_eState = OBJECT_IDLE;

			msg_walk_end.X = pPlayer->GetPosX();
			msg_walk_end.Y = 0.0f;
			msg_walk_end.Z = pPlayer->GetPosZ();
			msg_walk_end.uiTick = tick;
			msg_walk_end.fAtan2 = pPlayer->m_fatan2;
			msg_walk_end.uiID = pPlayer->GetID();
			msg_walk_end.usMapID = (USHORT)this->GetID();
			SendAreaMsgOneToOther(pPlayer->GetCurrentCell(), &msg_walk_end);
		}
	}
	else if (pPlayer->m_queuePath.size() > 1)
	{
		it = pPlayer->m_queuePath.end();
		it--;
		pPerPath = it->get();
		pPerPath->nEndTick = tick;
		pPerPath->end_x = x;
		pPerPath->end_z = z;
	}
	else
	{
		pPlayer->m_fatan2 = pPlayer->m_fDest = atan2;
	}

	return 0;
}