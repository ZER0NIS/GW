#pragma once

#include ".\GameCore\GameData.h"
#include "..\Common\MsgTypes.h"
#include "./GameCore/player.h"
class CDial
{
public:
	CDial();
	~CDial();

	struct DialItemData
	{
		ItemData item_data;
		UINT DialID;
		BYTE Nonsuch;
		UINT RandInDial;
		UINT Probability;
		UINT ItemID;
		int LV;
	};
	class DialDefaultItemData :public ItemData
	{
	public:
		UINT DialID;
		UINT ItemID;
	};
	void Init();
	static  CDial* Instance();
	void RandomItemInDial(UINT base_id);
	int RandomItem(UINT base_id);
	UINT DefaultItem(UINT base_id);
	DialDefaultItemData& CDial::GetDefaultItemData(UINT base_id);
	UINT* GetInDialItemID();
	void SetPlayer(CPlayer* pPlayer) { pLocalPlayer = pPlayer; }
	UINT SecondItem(UINT base_id);
	eError  isGoldBoxIDExist(BYTE Iter, UINT base_id);
	bool  isMsgWrong(MSG_GOLD_BOX* pGoldBoxMsg);
	std::map<UINT, DialDefaultItemData>m_DefaultItemDataMap;

private:
	CPlayer* pLocalPlayer;
	std::map<UINT, std::map<UINT, DialItemData>>m_ItemDataMap;
	std::map<UINT, std::map<UINT, DialItemData>>m_NonsuchItemDataMap;
	std::map<UINT, UINT> m_SumOfRandInDial;
	std::map<UINT, UINT> m_SumOfNonsuchInDial;
	DialItemData m_DialItemData[DIALITEMCOUNT];
	static CDial* m_This;
};