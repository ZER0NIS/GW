#ifndef _FILE_GAMEOBJ_H
#define _FILE_GAMEOBJ_H
#pragma once

#pragma warning( disable:4244 )

#include "Obj.h"
#include "GameData.h"
#include "..\..\Common/Include/DB\inc\idb.h"
#include ".\TimeElems.h"
#include ".\Region.h"
#include "Map.h"
#include "..\ErrorMessage.h"
#include <mmsystem.h>

class CStatus;
class CSkillManager;
class CWorld;

#define SET_DATA_ACCESSOR( x, y )     inline void SetBase##y( x t )  {  m_BaseData.m_##y = t; };\
                                 inline void SetSpecialty##y( x t )  {  m_Specialty.m_##y = t; };\
                                 inline void SetEquip##y( x t )  {  m_EquipData.m_##y = t; };\
  		 inline void SetStatus##y( x t )  {  m_StatusData.m_##y = t; };\
  		 inline void SetAltar##y( x t )  {  m_AltarData.m_##y = t; };
#define GET_DATA_ACCESSOR( x, y)      inline x GetBase##y() {  return (x)m_BaseData.m_##y; };\
  		 inline x GetSpecialty##y() {  return (x)m_Specialty.m_##y; };\
  		 inline x GetEquip##y() {  return (x)m_EquipData.m_##y; };\
  		 inline x GetStatus##y() {  return (x)m_StatusData.m_##y; };\
  		 inline x GetAltar##y() {  return (x)m_AltarData.m_##y; };\
                                 inline x GetAll##y() {  return (x)(m_BaseData.m_##y + m_Specialty.m_##y + m_EquipData.m_##y + m_StatusData.m_##y + m_AltarData.m_##y );};
#define GET_SET_DATA_ACCESSOR( x, y )   SET_DATA_ACCESSOR( x, y) GET_DATA_ACCESSOR( x, y)

#define SET_ACCESSOR( x, y, z)       inline void Set##y( x t )  {  z.m_##y = t; };
#define GET_ACCESSOR( x, y, z )       inline x Get##y() {  return (x)z.m_##y; };
#define GET_SET_ACCESSOR( x, y, z )   SET_ACCESSOR( x, y, z ) GET_ACCESSOR( x, y, z )

#define RESUME_TIME_INTERVAL 5
#define RELATION_TIME_INTERVAL 30
#define EXP_INTERVAL 100
#define MAX_AVATAR	12

struct tarWalk
{
	float x, z, fatan;
};
// Usar unique_ptr en lugar de punteros crudos
typedef std::list<std::unique_ptr<tarWalk>>::iterator WalkIterator;

struct tarPath
{
	float		x, y, z;
	float		end_x, end_y, end_z;
	float		OffsetX, OffsetY, OffsetZ;
	unsigned int nEndTick;
	unsigned int nTick;
};
// Usar unique_ptr en lugar de punteros crudos
typedef std::list<std::unique_ptr<tarPath>>::iterator PathIterator;

#define MAX_BUFF 20
#define MAX_DEBUFF 20
class CBuff;

enum PhysiacalAttack
{
	ATTACK_UBAEMED,
	ATTACK_HAMMER,
	ATTACK_CHOP,
	ATTACK_STICK,
	ATTACK_SHOT,
	ATTACK_STAFF,
};

enum Magic_Attack
{
	MAGIC_PHY,
	MAGIC_MAGIC,
	MAGIC_IATRIC,
	MAGIC_FLY,
};

enum  TIME_STYLE {
	TIME_STYLE_CAST,
	TIME_STYLE_COOL,
	TIME_STYLE_NONE
};

enum {
	STYLE_DECREASE = 0,
	STYLE_INCREASE
};

enum HandType
{
	HTNULL,
	HTOne,
	HTTwo,
	HTDoubule
};

enum ATTACK_TYPE
{
	ATTACK_FORCE,
	ATTACK_HIT,
	ATTACK_MISS,
};

typedef struct  ACTIVE_INFO
{
	TIME_STYLE  Time_style;
	time_t      Time_start;
	UINT        Time_interval;
} ACTIVEINFO;

typedef struct PASSIVE_INFO
{
	UINT  Grade;
	USHORT EquipLV;
} PASSIVEINFO;

struct DAMAGE_INFO
{
	int		Type;
	int		HP;
	int		MP;
};

enum eAvatar {
	AVATAR_CAP = 0,
	AVATAR_HAIR,
	AVATAR_HEAD,
	AVATAR_EYE,
	AVATAR_BODY,
	AVATAR_GLOVES,
	AVATAR_BOOTS,
	AVATAR_SLEEVES,
	AVATAR_LEGGINS,
	AVATAR_WEAPONONEHAND,
	AVATAR_WEAPONTWOHAND,
};

static char* SQL_SELECT_USER_BY_ID = "SELECT curHP,name,curMP FROM character_base WHERE id = %d LIMIT 1";
static char* SQL_DELETE_COOLTIME_BY_ID = "DELETE  FROM user_magic WHERE user_id= %d ";
static char* SQL_INSERT_COOLTIME_BY_ID = "INSERT INTO user_magic VALUES ( %d,%d,%d)";

class CGameObject;
class CCell;

struct tarEnmity
{
	CGameObject* pObject;
	long lValue;

	bool operator == (const tarEnmity& right) const
	{
		return pObject == right.pObject && lValue == right.lValue;
	}
};

typedef list< tarEnmity > EnmityList;
typedef list< tarEnmity >::iterator EnmityIterator;
typedef struct
{
	int     HP_A;
	float   HP_B;
	float   HP_C;
	float	HP_D;
	int     MP_A;
	float   MP_B;
	float   MP_C;
	int     Attack_A;
	float	Attack_B;
	float	Attack_C;
	float	Attack_D;
	int	    Defend_A;
	float	Defend_B;
	float	Defend_C;
	float	Defend_D;
	int		Hit_A;
	int		Hit_B;
	float	Hit_C;
	int		Miss_A;
	int		Miss_B;
	float	Miss_C;
	int		MagicExempt_A;
	int		MagicExempt_B;
	float	MagicExempt_C;

	int	    MagicResist_A;
	float	MagicResist_B;
} PARAMETER_LIST;

typedef struct
{
	int     Defalult_Hit;
	int	    Defalult_Dodge;
	float	Default_Resume_HP;
	float   Default_Resume_MP;
} DEFAULT_LIST;

struct ScriptObject
{
	ScriptObject()
		:
		ID(-1),
		Rank(0)
	{
	}

	int ID;
	int	Rank;
};

typedef struct
{
	float  PhyAttack_A_1;
	float  PhyAttack_A_2;
	float  PhyAttack_A_3;
	float  PhyAttack_B_1;
	float  PhyAttack_B_2;
	float  PhyAttack_B_3;
	float  PhyAttack_B_4;
	float  PhyAttack_C;

	float  MagicAttack_A_1;
	float  MagicAttack_A_2;
	float  MagicAttack_A_3;
	float  MagicAttack_B_1;
	float  MagicAttack_B_2;
	float  MagicAttack_B_3;
	float  MagicAttack_B_4;
	float  MagicAttack_C;
} ATTACK_PARAM;

class CGameObject
{
	typedef  void (CGameObject::* pFAddr)(float, bool, int);
public:
	CGameObject(void);
	virtual ~CGameObject(void);

	virtual bool IsActive(void) { return m_bActive; }
	virtual void Release(void);
	virtual void AI(void);
	virtual CGameObject* GetTarget(void) { return m_targetObj; }
	virtual CGameObject* GetSkillTarget(void) { return m_skillTargetObj; }
	virtual void  SetTarget(CGameObject* pObject) { m_targetObj = pObject; }
	virtual void  SetSkillTarget(CGameObject* pObject) { m_skillTargetObj = pObject; }

	virtual void CalculateMoney(CGameObject* pObj);
	virtual AttackRadius GetAttackRadius();
	virtual  long GetID(void) const { return m_lID; }
	void SetID(long lID) { m_lID = lID; }
	void SetCurrentCell(CCell* pCell) { m_pCurrentCell = pCell; }
	CCell* GetCurrentCell() const { return m_pCurrentCell; }
	void Activate(void);
	void SetRegion(CRegion* pRegion) { m_pRegion = pRegion; m_ObjectData.m_lRegionID = (USHORT)m_pRegion->GetLogicID(); };
	CRegion* GetRegion(void) { return m_pRegion; }
	float GetPosX() { return m_ObjectData.m_fX; }
	float GetPosY() { return m_ObjectData.m_fY; }
	float GetPosZ() { return m_ObjectData.m_fZ; }
	float GetAtan2() { return m_fatan2; }
	float GetOffsetX(void) { return m_fOffsetX; }
	float GetOffsetZ(void) { return m_fOffsetZ; }
	CCell* SetPos(float x, float y, float z, bool IsFirst = false);
	virtual void StartFight(CGameObject* pObject);
	virtual void EndFight();
	void SetFight(bool value) { m_bIsFight = value; }
	bool GetFight(void) { return m_bIsFight; }
	void LeaveFight(void) {};
	bool IsAttack(void);
	long SetActive(bool active);
	eObjectType	GetType(void) { return m_eType; }
	void ClearPath(void);
	void GetScriptObject(ScriptObject* obj);
	void SetScriptObject(ScriptObject* obj);
	virtual HandType GetHandType();
	virtual void Dead(CGameObject* pObj);
	virtual bool IsDead();

	virtual bool IsOwner(const char*)
	{
		return false;
	}

	EquipData* GetEquipData()
	{
		return &m_EquipData;
	}
	AltarData* GetEquiAltarpData()
	{
		return &m_AltarData;
	}
	StatusData* GetStatusData()
	{
		return &m_StatusData;
	}

	ATTACK_TYPE JudgeAttackStyle(CGameObject* pTarget);
	ATTACK_TYPE JudgeAppendStatusStyle(CGameObject* pTarget, const MagicData* pMagicData);
	virtual bool InitObjData(long  ID) = 0;
	virtual void AynObjToObj(CCell* pOldCell, CCell* pNewCell);
	virtual void AynMeToOther(CCell* pOldCell, CCell* pNewCell);
	virtual bool CalculatePassivePracticeDegree(PhysiacalAttack Type, ATTACK_TYPE eAttackType, bool IsDead = false) = 0;
	virtual void CalculatePassivePracticeDegree(int MagicID, ATTACK_TYPE eAttackType, bool IsDead = false) = 0;
	virtual void BePhysicalAttackedPassivePracticeDegree(PhysiacalAttack, ATTACK_TYPE) {};
	virtual void BeMagicAttackPassivePracticeDegree(UINT, ATTACK_TYPE) {};
	virtual bool CalculateExp(bool TarIsDead, int exp = 1);
	virtual bool ChanegeDegree(int ucSkillID, int Buffer) = 0;
	virtual bool PassiveSkillUpgrade(UINT  SkillID, int* Rank = NULL) = 0;
	virtual void AddPassiveSkill(int ucSkillID, int iRank = 1, int EXP = 0) = 0;
	virtual	bool IsActiveSkillCooling(int ucSkillID);
	virtual	bool IsGodSkillTimeOut(int MagicID);
	virtual	bool SetActiveSkillStartTime(int ucSkillID);
	virtual	void SetCurActiveSkillID(int ucActiveSkillID);
	virtual	void ResetTimer(float  IntervalTime, TIME_STYLE timeStyle);
	virtual	void ChangeActiveSkillStatus(int ucSkillID, TIME_STYLE status);
	virtual	bool IsRefresh(TIME_STYLE timeStyle);
	virtual INT CalculateAttackDamage(CGameObject& pObj, float iPower1 = 0, int iPower2 = 0);
	virtual void CalculateExpAndLevel(CGameObject& pTarObj);
	virtual INT	CalculateAttackDamage(int MagicID, CGameObject& pObj);
	virtual UINT  GetRank() = 0;
	virtual bool DelPassiveSkill(int ucSkillID);
	virtual bool FindPassiveSkill(int ucSkillID, UINT* Degree = NULL);
	virtual bool FindActiveSkill(int ucSkillID);
	virtual bool AddActiveSkill(int ucSkillID);
	virtual void DelActiveSkill(int ucSkillID);
	bool DelStatus(CStatus* pStatus);
	bool AddStatus(CStatus* pStatus);
	bool CanAddStatus(CStatus* pStatus);
	void ResolveStatus();
	size_t GetStatus(UINT StatusArr[]);
	virtual bool IsHeavyLoricae() { return false; };
	virtual bool IsFabircLoricae() { return false; };
	virtual	int GetCurActiveSkillID();
	virtual	void AddEnmity(CGameObject* pObject, long lValue);
	virtual	void DecEnmity(CGameObject* pObject, long lValue);
	virtual	void RemoveEnmity(CGameObject* pObject);
	virtual	void ClearEnmity();
	virtual	void ChainEnmityList(CGameObject* obj, int value);
	virtual void CoagentEnmityList(CGameObject* obj, int value);
	virtual bool IsClearEnmity();
	virtual	CGameObject* GetMaxEnmity();
	virtual ULONG GetEXP() { return 0; };
	void AddHP(const int hp);
	void AddMP(const int mp);
	virtual ULONG GetDodge() = 0;
	virtual UINT  CalculateAttack();
	virtual UINT  CalculateDefend() const;
	int	GetMaxHP(void) const;
	int	GetMaxMP(void) const;
	UINT GetSkillPoint() { return m_ConvertPoint; };
	UINT GetSkillExp() { return m_WinExp; };

	void SetSkillPoint(UINT uiPoint) { m_ConvertPoint = uiPoint; };
	void SetSkillExp(UINT uiExp) { m_WinExp = uiExp; };
	virtual void  SynGameData(bool) {};
	DAMAGE_INFO		GetDamageInfo(void) const;
	void	SetDamageInfo(const DAMAGE_INFO dmgInfo);
	void	ClearDamageInfo(void);
	void AddDanger(int iDander = 1) { m_lDanger += iDander; }
	long GetDanger(void) { return m_lDanger; }
	eObjectState GetState(void) { return m_eState; }
	void SetState(eObjectState  eState) { m_eState = eState; }
	void ResetCastState();

	virtual char* GetName() { return m_ObjectData.m_strName; }
	virtual void  SetName(const char* RoleName) { strcpy(m_ObjectData.m_strName, RoleName); };

	LONG   GetlHP() { return m_ObjectData.m_lHP + m_StatusData.m_AddHP - m_StatusData.m_DecHP; };
	LONG   GetlMP() { return m_ObjectData.m_lMP + m_StatusData.m_AddMP - m_StatusData.m_DecMP; };
	void   SetlHP(LONG HP) { m_ObjectData.m_lHP = HP; };
	void   SetlMP(LONG MP) { m_ObjectData.m_lMP = MP; };
	GET_SET_ACCESSOR(BYTE, Gender, m_ObjectData);
	GET_SET_ACCESSOR(USHORT, lRegionID, m_ObjectData);
	GET_SET_ACCESSOR(FLOAT, fX, m_ObjectData);
	GET_SET_ACCESSOR(FLOAT, fY, m_ObjectData);
	GET_SET_ACCESSOR(FLOAT, fZ, m_ObjectData);
	GET_SET_ACCESSOR(FLOAT, fSpeed, m_ObjectData);
	GET_SET_ACCESSOR(USHORT, lStyle, m_ObjectData);
	GET_SET_ACCESSOR(BYTE, lFaction, m_ObjectData);
	GET_SET_ACCESSOR(USHORT, lPrestige, m_ObjectData);
	GET_SET_ACCESSOR(BYTE, cKnight, m_ObjectData);
	GET_SET_ACCESSOR(USHORT, cUnion, m_ObjectData);
	GET_SET_ACCESSOR(BYTE, cUnionBusiness, m_ObjectData);
	GET_SET_ACCESSOR(UINT, uiUionContribute, m_ObjectData);
	GET_SET_ACCESSOR(UINT, uiStoreNum, m_ObjectData);
	GET_SET_ACCESSOR(UINT, uiBagNum, m_ObjectData);
	GET_SET_ACCESSOR(BYTE, lClass, m_ObjectData);
	GET_SET_ACCESSOR(BYTE, cRank, m_ObjectData);
	GET_SET_ACCESSOR(UINT, cExp, m_ObjectData);
	GET_SET_ACCESSOR(USHORT, usAttackDistance, m_ObjectData);
	GET_SET_ACCESSOR(USHORT, usAttackSpeed, m_ObjectData);
	GET_SET_ACCESSOR(BYTE, cHairStyle, m_ObjectData);
	GET_SET_ACCESSOR(BYTE, cPupilColor, m_ObjectData);
	GET_SET_ACCESSOR(USHORT, usBelief, m_ObjectData);
	GET_SET_ACCESSOR(UINT, lMoney, m_ObjectData);
	GET_SET_ACCESSOR(UINT, lStone, m_ObjectData);
	GET_SET_ACCESSOR(UINT, AttackDistance, m_EquipData);
	GET_SET_ACCESSOR(UINT, AttackSpeed, m_EquipData);

	GET_SET_DATA_ACCESSOR(LONG, lMaxHP);
	GET_SET_DATA_ACCESSOR(LONG, lMaxMP);
	GET_SET_DATA_ACCESSOR(LONG, lResumeHP);
	GET_SET_DATA_ACCESSOR(LONG, lResumeMP);
	GET_SET_DATA_ACCESSOR(LONG, lAttack);
	GET_SET_DATA_ACCESSOR(LONG, lDefend);
	GET_SET_DATA_ACCESSOR(LONG, lMagicAttack);
	GET_SET_DATA_ACCESSOR(LONG, lMagicDefend);
	GET_SET_DATA_ACCESSOR(LONG, lHit);
	GET_SET_DATA_ACCESSOR(LONG, lDodge);
	GET_SET_DATA_ACCESSOR(LONG, lCritAppend);
	GET_SET_DATA_ACCESSOR(LONG, lCritDefend);
	GET_SET_DATA_ACCESSOR(FLOAT, fPhyDamageAppend);
	GET_SET_DATA_ACCESSOR(FLOAT, fMagicDamageAppend);
	GET_SET_DATA_ACCESSOR(LONG, lDamageSorb);
	GET_SET_DATA_ACCESSOR(FLOAT, fBeCure);
	GET_SET_DATA_ACCESSOR(FLOAT, fCure);
	GET_SET_DATA_ACCESSOR(LONG, lStatusHit);
	GET_SET_DATA_ACCESSOR(LONG, lStatudDodge);

	GET_SET_ACCESSOR(FLOAT, HaltIntonate, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, ReboundDamage, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, NonMoving, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, NonMagicUsing, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, NonZSUsing, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, NonAttackUsing, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, NonPropUsing, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, ReturnDamage, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, ChangeIntonateTime, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, BeMagicDamage, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, CreateMagicDamage, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, BeAttackDamage, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, CreateAttackDamage, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, GetEXP, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, AddHP, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, DecHP, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, AddMP, m_StatusData);
	GET_SET_ACCESSOR(FLOAT, DecMP, m_StatusData);

	static void RouterHandler(pFAddr* Addr, int ID);
	void Handler_HaltIntonate(float value, bool isresume, int StatusID);
	void Handler_ReboundDamage(float value, bool isresume, int StatusID);
	void Handler_NonMoving(float value, bool isresume, int StatusID);
	void Handler_NonMagicUsing(float value, bool isresume, int StatusID);
	void Handler_NonZSUsing(float value, bool isresume, int StatusID);
	void Handler_NonAttackUsing(float value, bool isresume, int StatusID);
	void Handler_NonPropUsing(float value, bool isresume, int StatusID);
	void Handler_ReturnDamage(float value, bool isresume, int StatusID);
	void Handler_ChangeIntonateTime(float value, bool isresume, int StatusID);
	void Handler_BeCure(float value, bool isresume, int StatusID);
	void Handler_UseCure(float value, bool isresume, int StatusID);
	void Handler_BeMagicDamage(float value, bool isresume, int StatusID);
	void Handler_CreateMagicDamage(float value, bool isresume, int StatusID);
	void Handler_BeAttackDamage(float value, bool isresume, int StatusID);
	void Handler_CreateAttackDamage(float value, bool isresume, int StatusID);
	void Handler_GetEXP(float value, bool isresume, int StatusID);
	void Handler_MPMax(float value, bool isresume, int StatusID);
	void Handler_HPMax(float value, bool isresume, int StatusID);
	void Handler_Hit(float value, bool isresume, int StatusID);
	void Handler_Duck(float value, bool isresume, int StatusID);
	void Handler_Defend(float value, bool isresume, int StatusID);
	void Handler_Attack(float value, bool isresume, int StatusID);
	void Handler_CirtAdd(float value, bool isresume, int StatusID);
	void Handler_CirtDefend(float value, bool isresume, int StatusID);
	void Handler_MagicAttack(float value, bool isresume, int StatusID);
	void Handler_MagicDefend(float value, bool isresume, int StatusID);
	void Handler_AddHP(float value, bool isresume, int StatusID);
	void Handler_DecHP(float value, bool isresume, int StatusID);
	void Handler_AddMP(float value, bool isresume, int StatusID);
	void Handler_DecMP(float value, bool isresume, int StatusID);
	void Handler_ChangeMagicSuccedOdds(float value, bool isresume, int StatusID);
	void Handler_ChangeMagicDefendOdds(float value, bool isresume, int StatusID);

	void Handler_PS_HPMax(SKillData* Skill, AttributeType Type);
	void Handler_PS_MPMax(SKillData* Skill, AttributeType Type);
	void Handler_PS_ResumeHP(SKillData* Skill, AttributeType Type);
	void Handler_PS_ResumeMP(SKillData* Skill, AttributeType Type);
	void Handler_PS_Attack(SKillData* Skill, AttributeType Type);
	void Handler_PS_Defend(SKillData* Skill, AttributeType Type);
	void Handler_PS_MagicAttack(SKillData* Skill, AttributeType Type);
	void Handler_PS_MagicDefend(SKillData* Skill, AttributeType Type);
	void Handler_PS_Hit(SKillData* Skill, AttributeType Type);
	void Handler_PS_Dodge(SKillData* Skill, AttributeType Type);
	void Handler_PS_CritAppend(SKillData* Skill, AttributeType Type);
	void Handler_PS_DecPhysicalDamage(SKillData* Skill, AttributeType Type);
	void Handler_PS_CritDefen(SKillData* Skill, AttributeType Type);
	void Handler_PS_PhyDamageAppend(SKillData* Skill, AttributeType Type);
	void Handler_PS_MagicDamageAppend(SKillData* Skill, AttributeType Type);
	void Handler_PS_DamageSorb(SKillData* Skill, AttributeType Type);
	void Handler_PS_BeCure(SKillData* Skill, AttributeType Type);
	void Handler_PS_Cure(SKillData* Skill, AttributeType Type);
	void Handler_PS_OneStatusAppend(SKillData* Skill, AttributeType Type);
	void Handler_PS_OneStatudDefend(SKillData* Skill, AttributeType Type);
protected:
	void ResetResumeTimer() { m_Resume_Timer.TimeOver(); m_Resume_Timer.Startup(RESUME_TIME_INTERVAL); };
	void ResetRelationTimer() { m_Relation_Timer.TimeOver(); m_Relation_Timer.Startup(RELATION_TIME_INTERVAL); };

private:

public:

	EnmityList         	m_listEnmity;
	AttackRadius       	m_AttackRadius;
	long               	m_lID;
	volatile  bool      m_bActive;
	float              	m_fOffsetX, m_fOffsetY, m_fOffsetZ;
	float              	m_fDestinationX, m_fDestinationY, m_fDestinationZ;
	float              	m_fDist;
	float              	m_fDest;
	float              	m_fatan2;
	float              	m_fX, m_fY, m_fZ;
	bool               	m_bIsFight;
	eObjectType        	m_eType;
	eObjectState       	m_eState;
	sbase::CTimer      	m_timeFight;
	sbase::CTimerMS	   	m_timeAttack;
	CGameObject* m_targetObj;
	CGameObject* m_skillTargetObj;
	std::list<std::unique_ptr<tarPath>> m_queuePath;
	std::list<std::unique_ptr<tarWalk>> m_queueWalk;
	short int          	m_nAvatar[MAX_AVATAR];
	CBuff* m_pBuff;
	CBuff* m_pDeBuff;
	CRegion* m_pRegion;

private:
	DAMAGE_INFO	        m_DamageInfo;
protected:

	ObjectData	                m_ObjectData;
	BaseData                    m_BaseData;
	SkillData                   m_Specialty;
	EquipData                	m_EquipData;
	StatusData                  m_StatusData;
	AltarData                   m_AltarData;

	map< UINT, PASSIVEINFO >   m_PassiveSkill;
	map< UINT, ACTIVEINFO >    m_ActiveSkill;
	map< UINT, UINT >          m_Altar;
	map< UINT, TTimeElement<CStatus*, time_t> > m_StatusMap;
	map< UINT, StatusData > m_StatusSelfData;

	short                       m_ucCurActiveSkill;
	UINT                        m_uiSkillEffect;
	BYTE                        m_lRefreshTime;
	BYTE                        m_ucActiveSkillCastTime;
	USHORT                      m_usActiveSkillCoolTime;
	sbase::CTimerMS             m_ActiveSkillCast_Timer;
	sbase::CTimer               m_ActiveSkillCool_Timer;
	sbase::CTimer               m_Resume_Timer;
	sbase::CTimer               m_Relation_Timer;
	sbase::CTimer               m_Team_Timer;
	time_t                      m_GodLastTime;
	CCell* m_pCurrentCell;
	UINT                        m_WinExp;
	UINT                        m_ConvertPoint;
	DWORD  dTimeStart;

private:
	long m_lDanger;
public:

	bool IsPublicCDOK() { return(timeGetTime() - dTimeStart >= 1000); }
	void StartPublicCD() { dTimeStart = timeGetTime(); }

public:

	static CWorld* s_World;
};

#endif
