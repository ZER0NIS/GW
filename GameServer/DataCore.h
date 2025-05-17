#pragma once

#define PLAYER_BEGIN 	0
#define NPC_BEGIN 		5000
#define MONSTER_BEGIN 	10000

#define MAX_REGION 		24
#define MAX_REP_REGION 	1

#define ACADEMIC_PLAYER_MEM_ID 		0
#define ACADEMIC_NPC_MEM_ID  5000
#define ACADEMIC_MONSTER_MEM_ID 		10000
#define ACADEMIC_BOSS_MEM_ID 		100

#define ACTUAL_PLAYER_ONLINE	    1500
#define ACTUAL_NPC_ONLINE		    3000
#define ACTUAL_MONSTER_ONLINE		20000
#define ACTUAL_BOSS_ONLINE 100

#define	CORPSE_LOST_TIME 5
#define CORPSE_ITEM_TIME 20

#define FIGHT_TIME 		10
#define FIGHT_INTERVAL 	1700
#define REFRESH_TIME 	30
#define MONSTER_AI_TIME 	20

#define MONSTER_FACTION 	2

#define ERROR_JOIN_PLAYER_FULL 	2
#define ERROR_JOIN_PLAYER_OUT 	3
#define ERROR_REMOVE_PLAYER_OUT 	3

class CRegion;
class CRepRegion;
class CGameObject;
class CPlayer;
class CNPC;
class CMonster;
class CObstacle;

struct tarSceneChange
{
	CPlayer* pPlayer;
	long lFromSceneID;
	long lToSceneID;
	float x, y, z;
};
