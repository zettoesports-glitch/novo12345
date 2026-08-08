#include "stdafx.h"
#include "Combat/CAutoCombat.h"
#include "ZzzInfomation.h"
#include "ZzzInventory.h"
#include "ZzzCharacter.h"
#include "NewUISystem.h"
#include "NewUIMyInventory.h"
#include "NewUIChatLogWindow.h"
#include "wsclientinline.h"
#include "MapManager.h"

static void NotifyChat(const char* msg)
{
	if (g_pChatListBox)
		g_pChatListBox->AddText("", msg, SEASON3B::TYPE_SYSTEM_MESSAGE);
}

CAutoCombat& CAutoCombat::Instance()
{
	static CAutoCombat s;
	return s;
}

CAutoCombat::CAutoCombat()
{
	m_bAutoHP = false;
	m_bAutoPvP = false;
	m_iHPPercent = 95;
	m_uPotionDelayMs = 150;
	m_uLastPotionTick = 0;
}

void CAutoCombat::Load()
{
	m_bAutoHP = (GetPrivateProfileIntA("Combat", "AutoHP", 0, ".\\config.ini") != 0);
	m_bAutoPvP = (GetPrivateProfileIntA("Combat", "AutoPvP", 0, ".\\config.ini") != 0);
	m_iHPPercent = GetPrivateProfileIntA("Combat", "AutoHPPercent", 95, ".\\config.ini");
	m_uPotionDelayMs = (unsigned int)GetPrivateProfileIntA("Combat", "AutoHPDelayMs", 500, ".\\config.ini");

	if (m_iHPPercent < 10)
		m_iHPPercent = 10;
	if (m_iHPPercent > 99)
		m_iHPPercent = 99;
	if (m_uPotionDelayMs < 50)
		m_uPotionDelayMs = 50;
	if (m_uPotionDelayMs > 3000)
		m_uPotionDelayMs = 3000;
}

void CAutoCombat::Save() const
{
	char buf[32];

	WritePrivateProfileStringA("Combat", "AutoHP", m_bAutoHP ? "1" : "0", ".\\config.ini");
	WritePrivateProfileStringA("Combat", "AutoPvP", m_bAutoPvP ? "1" : "0", ".\\config.ini");

	sprintf_s(buf, "%d", m_iHPPercent);
	WritePrivateProfileStringA("Combat", "AutoHPPercent", buf, ".\\config.ini");

	sprintf_s(buf, "%u", m_uPotionDelayMs);
	WritePrivateProfileStringA("Combat", "AutoHPDelayMs", buf, ".\\config.ini");
}

void CAutoCombat::SetAutoHP(bool on)
{
	if (m_bAutoHP == on)
		return;
	m_bAutoHP = on;
	if (on)
		m_uLastPotionTick = 0;
	Save();
	NotifyChat(m_bAutoHP ? "Auto HP On" : "Auto HP Off");
}

void CAutoCombat::ToggleAutoHP()
{
	SetAutoHP(!m_bAutoHP);
}

void CAutoCombat::SetHPPercent(int percent)
{
	if (percent < 10)
		percent = 10;
	if (percent > 99)
		percent = 99;
	m_iHPPercent = percent;
	Save();
}

void CAutoCombat::SetAutoPvP(bool on)
{
	if (m_bAutoPvP == on)
		return;
	m_bAutoPvP = on;
	Save();
	NotifyChat(m_bAutoPvP ? "Auto PvP On" : "Auto PvP Off");
}

void CAutoCombat::ToggleAutoPvP()
{
	SetAutoPvP(!m_bAutoPvP);
}

int CAutoCombat::FindHPPotionInventoryIndex() const
{
	if (!g_pMyInventory)
		return -1;

	static const int kHpTypes[] = {
		ITEM_POTION + 3,
		ITEM_POTION + 2,
		ITEM_POTION + 1,
	};

	for (int i = 0; i < (int)(sizeof(kHpTypes) / sizeof(kHpTypes[0])); ++i)
	{
		const int idx = g_pMyInventory->FindItemReverseIndex(kHpTypes[i]);
		if (idx != -1)
			return idx;
	}
	return -1;
}

bool CAutoCombat::TryUseHPPotion()
{
	const unsigned int now = (unsigned int)timeGetTime();
	if ((now - m_uLastPotionTick) < m_uPotionDelayMs)
		return false;

	const int slot = FindHPPotionInventoryIndex();
	if (slot < 0)
		return false;

	SendRequestUse(slot, 0);
	m_uLastPotionTick = now;
	return true;
}

void CAutoCombat::Update()
{
	if (!m_bAutoHP)
		return;

	if (SceneFlag != MAIN_SCENE)
		return;

	if (!CharacterAttribute || !Hero)
		return;

	if (CharacterAttribute->Life <= 0)
		return;

	TryUseHPPotion();
}
