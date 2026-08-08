#pragma once

#include "Defined_Global.h"

#if(CB_BXHDMG)

#include "Protocol.h"
#include <vector>

#pragma pack(push, 1)
struct PMSG_BXH_DMG_HEAD_RECV
{
	PSWMSG_HEAD header;
	BYTE count;
	WORD ClassMonster;
	DWORD TotalDamage;
};

struct PMSG_BXH_DMG_INFO_RECV
{
	char Name[11];
	int damage;
};
#pragma pack(pop)

class CB_BXHTopDmg
{
public:
	CB_BXHTopDmg();
	~CB_BXHTopDmg();

	void RecvProtocol(BYTE* Recv);
	void DrawWindowMini();

	int ClassMonster;
	DWORD TotalDamage;
	std::vector<PMSG_BXH_DMG_INFO_RECV> m_DataTopBXHDMG;
};

extern CB_BXHTopDmg* gCB_BXHTopDmg;

#endif
