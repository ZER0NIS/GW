#pragma once

#include "GameObject.h"
#include "Item.h"
#include "./DropItemMgr.h"

#define AI_TYPE_INITIATIVE 1
#define AI_TYPE_PASSIVITY 2
#define AI_MONSTER_WALKTIME 500
#define MAX_OWNER 5
#define MAX_SKILLS 4

enum {
	MONSTER_STATE_LIVE = 0,
	MONSTER_STATE_DEAD,
	MONSTER_STATE_LOST,
	MONSTER_STATE_LEAVEFIGHT,

	MONSTER_ACTIVITY_IDLE,
	MONSTER_ACTIVITY_WALK,
	MONSTER_ACTIVITY_ESCAPE,
};

enum {
	MONSTER_MODE_PASSIVITY = 1,
	MONSTER_MODE_INITIATIVE,
};

enum {
	MONSTER_MODE_CRAVEN = 0,
	MONSTER_MODE_VALOROUS,
	MONSTER_MODE_DEEP,
};

typedef struct
{
	float           Exp;
	float           HP;
	float           HPResume;
	float           PhyDefend;
	float           MagicDefend;
	float           Hit;
	float           Miss;
	float           FrenzyHit;
	float           FrenzyMiss;
	float           PhyDamage;
	float           MagicDamage;
	float           DamageSorb;
	float           StatusHit;
	float           statusMiss;
} PARAM;

struct SkillInfo
{
	int		SkillID;
	int		SkillPro;
	float	SkillCoe;
	int		Attack;
	DWORD	TimePos;
};

class CMonster : public CGameObject
{
public:
	long 	m_lState;
	long 	m_lActivity;
	long 	m_lMode;
	float 	m_fIniX, m_fIniY, m_fIniZ;
	long 	m_lRefreshTime;
	int 		m_Level;
	int 		m_MonsterEquiplv;
	int 		m_Quality;
	float 	m_fActivityRange;
	float 	m_fEyeshot;
	float               m_fAngle;

public:
	CMonster(void);
	~CMonster(void);

public:

	bool 	IsLeaveFight() { return m_lState == MONSTER_STATE_LEAVEFIGHT; }

	UINT                CalculateAttack() const;
	UINT                CalculateDefend() const;
	void 	Activate(void);
	void 	AI(void);
	void 	Run(void);

	long 	GetState(void) { return m_lState; }
	void 	SetMode(long lMode) { m_lMode = lMode; }
	long 	GetMode(void) { return m_lMode; }

	void 	SetRefreshTime(long value) { m_lRefreshTime = value; }
	void 	Refresh(void);
	void 	ResetTimer(void);
	bool 	IsRefresh(void);

	void 	ResetCorpseTimer(void);
	bool 	IsCorpseTimer(void);

	void 	LeaveFight(void);

	bool 	WalkTo(float x, float y, float z);
	bool 	FollowTo(float x, float y, float z, BYTE distance = 2);
	bool 	StopWalk(void);

	void 	SetDropConfig(const DropConfig& drop);
	DropConfig& GetDropConfig();

	Item* GetFreeItem();

	float 	GetDistWithTarget();
	float 	GetDist(float x, float z);

	CGameObject* GetNearestPartner();

	virtual	void		AddEnmity(CGameObject* pObject, long lValue);
	virtual void		RemoveEnmity(CGameObject* pObject);

	virtual bool		IsOwner(const char* name);

	bool 	IsSkillCooling(int index);
	void 	Cooldown(int index);
	void 	UseSkill(int skillID);

	void 	Dead(CGameObject* pObj);

	void 	SetFightPosition();

	void                SynGameData(bool);
	virtual void		EndFight();

public:

	int 		m_QuestItem[MAX_DROPS];
	int 		m_QuestID;

	ItemData m_DropItem[MAX_DROPS];

	SkillInfo m_SkillInfos[MAX_SKILLS];

	CGameObject* m_ItemOwner[MAX_OWNER];
	string 	m_ItemOwnerName[MAX_OWNER];

	DropConfig m_DropConfig;
	ConfirmDrop         m_ConfirmConfig;
	CDropItemMgr        m_DropItemMgr;

	int 		m_Drops;
	Item 	m_DropItems[MAX_DROPS];

	int 		m_WalkTime;

	long 	m_lAIType;
	long 	m_Kidney;

	bool 	m_Escaped;
	bool 	m_Escaping;

	sbase::CTimer		m_Timer;
	sbase::CTimer		m_CorpseTimer;
	sbase::CTimer		m_timerAI;

	unsigned int		m_nRunTick;
	int 		m_nUnTick;
	float 	m_fDestinationX, m_fDestinationY, m_fDestinationZ;

	bool 	DoSomething();
	void                SetRandMagicAttack(UINT MagicAttack);

	virtual             UINT                CalculateAttack();
	bool CalculatePassivePracticeDegree(PhysiacalAttack Type, ATTACK_TYPE eAttackType, bool IsDead = false);
	void CalculatePassivePracticeDegree(int MagicID, ATTACK_TYPE eAttackType, bool IsDead = false);
	virtual             bool                ChanegeDegree(int ucSkillID, int Buffer);
	virtual             bool                PassiveSkillUpgrade(UINT  SkillID, int* Rank = NULL);
	virtual             void                AddPassiveSkill(int ucSkillID, int iRank = 1, int EXP = 0);
	virtual             INT                CalculateAttackDamage(CGameObject& obj, float iPower = 0);
	virtual             INT                CalculateAttackDamage(int MagicID);
	virtual void		AynObjToObj(CCell* pOldCell, CCell* pNewCell);
	virtual void		AynMeToOther(CCell* pOldCell, CCell* pNewCell);

	ULONG 	GetEXP();
	virtual UINT		GetRank() { return m_Level; };
	int 		GetMoney();

	ULONG CMonster::GetHit();
	ULONG CMonster::GetDodge();
	static PARAM 	Param;
	static              ATTACK_PARAM        AttackParam;

	bool                InitObjData(long ID);

private:

	int 		RandomSkill();

	int 		m_SkillCount;
	int 		m_SkillRandom;

	float 	m_fFightX;
	float 	m_fFightZ;
};
