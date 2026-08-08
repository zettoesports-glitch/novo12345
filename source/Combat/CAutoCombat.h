#pragma once

/**
 * CAutoCombat — Auto HP + Auto PvP (client)
 *
 * Auto HP  : uses HP potions when Life < threshold% of MaxLife
 * Auto PvP : allow targeting players without holding Ctrl
 *
 * config.ini [Combat]
 *   AutoHP=0/1
 *   AutoPvP=0/1
 *   AutoHPPercent=95
 *   AutoHPDelayMs=500
 */

class CAutoCombat
{
public:
	static CAutoCombat& Instance();

	void Load();
	void Save() const;

	void Update();

	bool IsAutoHP() const { return m_bAutoHP; }
	void SetAutoHP(bool on);
	void ToggleAutoHP();

	int  GetHPPercent() const { return m_iHPPercent; }
	void SetHPPercent(int percent);

	bool IsAutoPvP() const { return m_bAutoPvP; }
	void SetAutoPvP(bool on);
	void ToggleAutoPvP();

	bool AllowPlayerTargetWithoutCtrl() const { return m_bAutoPvP; }

private:
	CAutoCombat();
	~CAutoCombat() {}

	CAutoCombat(const CAutoCombat&);
	CAutoCombat& operator=(const CAutoCombat&);

	bool TryUseHPPotion();
	int  FindHPPotionInventoryIndex() const;

	bool m_bAutoHP;
	bool m_bAutoPvP;
	int  m_iHPPercent;
	unsigned int m_uPotionDelayMs;
	unsigned int m_uLastPotionTick;
};

#define gAutoCombat (CAutoCombat::Instance())
