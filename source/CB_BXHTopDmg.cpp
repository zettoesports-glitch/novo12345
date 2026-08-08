#include "stdafx.h"
#include "CB_BXHTopDmg.h"

#if(CB_BXHDMG)

#include "CBInterface.h"
#include "NewUISystem.h"
#include "NewUICommon.h"
#include "ZzzInfomation.h"
#include "Util.h"
#include "DSPlaySound.h"
#include "_TextureIndex.h"
#include "UIBaseDef.h"
#include "UIControls.h"

using namespace SEASON3B;

CB_BXHTopDmg* gCB_BXHTopDmg = 0;

namespace
{
	// Moldura Menu Custom (mesmo set do Mobile gDrawWindowCustom)
	enum
	{
		IMAGE_BXH_FRAME_TOP = BITMAP_INTERFACE_EX + 50,		// newui_item_back01
		IMAGE_BXH_FRAME_LEFT = BITMAP_INTERFACE_EX + 51,		// newui_item_back02-L
		IMAGE_BXH_FRAME_RIGHT = BITMAP_INTERFACE_EX + 52,	// newui_item_back02-R
		IMAGE_BXH_FRAME_BOTTOM = BITMAP_INTERFACE_EX + 53,	// newui_item_back03
		IMAGE_BXH_FRAME_CLOSE = BITMAP_INTERFACE_EX + 54,	// mini_map_ui_cancel
	};

	void EnsureBxhMenuCustomMoldura()
	{
		static bool s_loaded = false;
		if (s_loaded)
			return;

		LoadBitmap("Interface\\newui_item_back01.tga", IMAGE_BXH_FRAME_TOP, GL_LINEAR);
		LoadBitmap("Interface\\newui_item_back02-L.tga", IMAGE_BXH_FRAME_LEFT, GL_LINEAR);
		LoadBitmap("Interface\\newui_item_back02-R.tga", IMAGE_BXH_FRAME_RIGHT, GL_LINEAR);
		LoadBitmap("Interface\\newui_item_back03.tga", IMAGE_BXH_FRAME_BOTTOM, GL_LINEAR);
		LoadBitmap("Interface\\mini_map_ui_cancel.tga", IMAGE_BXH_FRAME_CLOSE, GL_LINEAR);
		s_loaded = true;
	}

	// Moldura completa do Menu Custom (cantos + laterais + fundo)
	bool DrawMenuCustomMoldura(float* StartX, float* StartY, float CuaSoW, float CuaSoH, const char* title)
	{
		if (!gInterface->Data[eWindowBXHDmg].OnShow)
			return false;

		EnsureBxhMenuCustomMoldura();

		CB_Interface::InterfaceObject& win = gInterface->Data[eWindowBXHDmg];

		if (!win.FirstLoad)
		{
			win.Width = CuaSoW;
			win.Height = CuaSoH;
			win.X = *StartX;
			win.Y = *StartY;
			win.AllowMove = true;
			win.FirstLoad = true;
		}
		else
		{
			*StartX = win.X;
			*StartY = win.Y;
			win.Width = CuaSoW;
			win.Height = CuaSoH;
		}

		if (win.AllowMove)
			gInterface->MoveWindows(eWindowBXHDmg);

		*StartX = win.X;
		*StartY = win.Y;

		const float x = win.X;
		const float y = win.Y;
		const float w = CuaSoW;
		const float h = CuaSoH;

		if (CheckMouseIn(x, y, w, h))
		{
			gInterface->SetBlockCur(TRUE);
			win.Hover = true;
		}
		else
		{
			win.Hover = false;
		}

		EnableAlphaTest();

		// Fundo escuro
		gInterface->DrawBarForm(x, y + 5.f, w, h - 10.f, 0.1f, 0.1f, 0.1f, 0.9f);

		// Barras horizontais (topo / base)
		for (float i = x + 10.f; i < (x + w) - 10.f; i += 10.f)
		{
			RenderImage(IMAGE_BXH_FRAME_TOP, i, y, 10.0f, 64.0f, 60.0f, 0.f);
			RenderImage(IMAGE_BXH_FRAME_BOTTOM, i, (y + h) - 45.0f, 10.0f, 45.0f, 60.0f, 0.f);
		}

		// Barras verticais (esquerda / direita)
		for (float doc = y + 10.f; doc < (y + h) - 10.f; doc += 10.f)
		{
			RenderImage(IMAGE_BXH_FRAME_LEFT, x, doc, 21.0f, 10.f, 0.f, 0.f);
			RenderImage(IMAGE_BXH_FRAME_RIGHT, (x + w) - 21.0f, doc, 21.0f, 10.f, 0.f, 0.f);
		}

		// Cantos
		RenderImage(IMAGE_BXH_FRAME_TOP, x, y, 60.0f, 64.0f, 0.f, 0.f);
		RenderImage(IMAGE_BXH_FRAME_TOP, (x + w) - 60.0f, y, 60.0f, 64.0f, 130.0f, 0.f);
		RenderImage(IMAGE_BXH_FRAME_BOTTOM, x, (y + h) - 45.0f, 60.0f, 45.0f, 0.f, 0.f);
		RenderImage(IMAGE_BXH_FRAME_BOTTOM, (x + w) - 60.0f, (y + h) - 45.0f, 60.0f, 45.0f, 130.0f, 0.f);

		// Botao fechar
		const float CloseX = x + w - 33.0f;
		const float CloseY = y;
		if (CheckMouseIn(CloseX, CloseY, 36.f, 29.f))
		{
			if ((GetKeyState(VK_LBUTTON) & 0x8000) && GetTickCount() - win.EventTick > 300)
			{
				PlayBuffer(SOUND_CLICK01);
				win.EventTick = GetTickCount();
				win.Close();
				EnableAlphaTest(false);
				return false;
			}
			RenderImage(IMAGE_BXH_FRAME_CLOSE, CloseX, CloseY, 36.f, 29.f, 0.f, 0.f, RGBA(255, 204, 20, 130));
		}
		else
		{
			RenderImage(IMAGE_BXH_FRAME_CLOSE, CloseX, CloseY, 36.0f, 29.0f);
		}

		// Titulo
		gInterface->TextDraw(g_hFontBold, (int)x, (int)(y + 10.f), 0xFFFFFFFF, 0x0, (int)w, 0, 3, "%s", title);

		glColor3f(1.f, 1.f, 1.f);
		EnableAlphaTest(false);
		return win.OnShow != 0;
	}
}

CB_BXHTopDmg::CB_BXHTopDmg()
{
	this->m_DataTopBXHDMG.clear();
	this->ClassMonster = -1;
	this->TotalDamage = 0;
}

CB_BXHTopDmg::~CB_BXHTopDmg()
{
}

void CB_BXHTopDmg::RecvProtocol(BYTE* Recv)
{
	if (Recv == 0)
	{
		return;
	}

	this->m_DataTopBXHDMG.clear();
	this->ClassMonster = -1;
	this->TotalDamage = 0;

	PMSG_BXH_DMG_HEAD_RECV* lpMsg = (PMSG_BXH_DMG_HEAD_RECV*)Recv;
	this->ClassMonster = lpMsg->ClassMonster;
	this->TotalDamage = lpMsg->TotalDamage;

	for (int n = 0; n < lpMsg->count && n < MAX_BXH_TOP; n++)
	{
		PMSG_BXH_DMG_INFO_RECV* lpInfo = (PMSG_BXH_DMG_INFO_RECV*)(
			((BYTE*)lpMsg) + sizeof(PMSG_BXH_DMG_HEAD_RECV) + (sizeof(PMSG_BXH_DMG_INFO_RECV) * n));
		this->m_DataTopBXHDMG.push_back(*lpInfo);
	}

	if (!this->m_DataTopBXHDMG.empty())
	{
		gInterface->Data[eWindowBXHDmg].OnShow = true;
	}
}

void CB_BXHTopDmg::DrawWindowMini()
{
	if (gInterface->CheckWindow(CB_Interface::MoveList)
		|| gInterface->CheckWindow(CB_Interface::Inventory)
		|| gInterface->CheckWindow(CB_Interface::Character)
		|| gInterface->CheckWindow(CB_Interface::FullMap)
		|| gInterface->CheckWindow(CB_Interface::CashShop)
		|| gInterface->CheckWindow(CB_Interface::SkillTree)
		|| gInterface->CheckWindow(CB_Interface::FastMenu))
	{
		return;
	}

	if (!gInterface->Data[eWindowBXHDmg].OnShow || this->m_DataTopBXHDMG.empty())
	{
		gInterface->Data[eWindowBXHDmg].OnShow = false;
		return;
	}

	// Altura minima da moldura custom (cantos 64+45)
	float WindowW = 190.f;
	float WindowH = 100.f + (float)(this->m_DataTopBXHDMG.size() * 14);
	if (WindowH < 130.f)
		WindowH = 130.f;

	float StartX = (float)(MAX_WIN_WIDTH - WindowW) - 12.f;
	float StartY = 70.f;

	const char* monName = getMonsterName(this->ClassMonster);
	if (monName == 0 || monName[0] == 0)
	{
		monName = "Boss";
	}

	if (!DrawMenuCustomMoldura(&StartX, &StartY, WindowW, WindowH, monName))
	{
		return;
	}

	// Conteudo (abaixo da barra superior da moldura ~y+40)
	const int headY = (int)(StartY + 42);
	gInterface->TextDraw(g_hFontBold, (int)(StartX + 18), headY, 0xFFC421FF, 0x0, 22, 0, 1, "#");
	gInterface->TextDraw(g_hFontBold, (int)(StartX + 38), headY, 0xFFC421FF, 0x0, 70, 0, 1, "Nome");
	gInterface->TextDraw(g_hFontBold, (int)(StartX + 110), headY, 0xFFC421FF, 0x0, 70, 0, 1, "Dano / %");

	const int textY = headY + 14;
	const DWORD total = (this->TotalDamage > 0) ? this->TotalDamage : 1;

	for (size_t i = 0; i < this->m_DataTopBXHDMG.size(); i++)
	{
		const int y = textY + (int)(i * 14);
		const int pct = (int)(((unsigned __int64)this->m_DataTopBXHDMG[i].damage * 100ull) / (unsigned __int64)total);
		char dmgBuf[32];
		sprintf_s(dmgBuf, "%d", this->m_DataTopBXHDMG[i].damage);

		const DWORD rankColor = (i == 0) ? 0xFFD700FF : 0xFFFFFFFF;

		gInterface->TextDraw(g_hFont, (int)(StartX + 18), y, rankColor, 0x0, 22, 0, 1, "%d", (int)(i + 1));
		gInterface->TextDraw(g_hFont, (int)(StartX + 38), y, rankColor, 0x0, 70, 0, 1, "%s", this->m_DataTopBXHDMG[i].Name);
		gInterface->TextDraw(g_hFont, (int)(StartX + 110), y, rankColor, 0x0, 70, 0, 1, "%s (%d%%)", dmgBuf, pct);
	}
}

#endif
