#include "stdafx.h"
#include "WSclient.h"
#include "UIControls.h"
#include "ZzzInventory.h"
#include "wsclientinline.h"
#include "NewUIInventoryJewel.h"
#include "NewUISystem.h"
#include "NewUICustomMessageBox.h"
#include "NewUIQuestProgress.h"
#include "NewUICommon.h"
#include "TextClien.h"
#include "CGMFrame.h"
#include "CGMProtect.h"
#include "CGMItemMng.h"
#include "DSPlaySound.h"
#include "_TextureIndex.h"
#include "GlobalText.h"
#include "ZzzInventory.h"
#include "ScriptItem.h"
#include "GameShop/NewUIInGameShop.h"
#include "UIBaseDef.h"

#define MAX_JEWEL_INVENTORY		6

SEASON3B::CNewUIInventoryJewel::CNewUIInventoryJewel()
{
	Initialize();
}

SEASON3B::CNewUIInventoryJewel::~CNewUIInventoryJewel()
{
	Release();
}

bool SEASON3B::CNewUIInventoryJewel::Create(CNewUIManager* pNewUIMng, int x, int y)
{
	if (!pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(INTERFACE_INVENTORY_JEWEL, this);

	// Menu Custom chrome + Event-style page L/R buttons
	LoadBitmap("Interface\\HUD\\bg250.tga", IMAGE_BG_SERVER_INTERFACE_250, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\timerserver-bar.tga", IMAGE_TITTLE_SERVER_INTERFACE, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\button_close_navi.tga", IMAGE_BUTTON_CLOSE_NAVI, GL_LINEAR);
	LoadBitmap("Interface\\InGameShop\\IGS_Storage_Page.tga", CNewUIInGameShop::IMAGE_IGS_STORAGE_PAGE, GL_LINEAR);
	LoadBitmap("Interface\\InGameShop\\ingame_Bt_page_L.tga", CNewUIInGameShop::IMAGE_IGS_PAGE_LEFT, GL_LINEAR);
	LoadBitmap("Interface\\InGameShop\\ingame_Bt_page_R.tga", CNewUIInGameShop::IMAGE_IGS_PAGE_RIGHT, GL_LINEAR);


	SetPos(x, y);
	InitButtons();
	return true;
}

void SEASON3B::CNewUIInventoryJewel::Initialize()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;
	m_dwCurIndex = -1;
	m_dwSelIndex = -1;
	m_nSelPage = 0;
	m_nMaxPage = 0;
	m_nOptionCurrent = 0;
	memset(&m_nRectItem, 0, sizeof(m_nRectItem));

	m_bItems.push_back({ 0, 6159, -1, 250000 });
	m_bItems.push_back({ 1, 7181, -1, 250000 });
	m_bItems.push_back({ 2, 7182, -1, 250000 });
	m_bItems.push_back({ 3, 7184, -1, 250000 });
	m_bItems.push_back({ 4, 7190, -1, 250000 });
	m_bItems.push_back({ 5, 7199, -1, 250000 });
	m_bItems.push_back({ 6, 7210, -1, 250000 });
}

void SEASON3B::CNewUIInventoryJewel::Release()
{
	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
	m_bItems.clear();
}

void SEASON3B::CNewUIInventoryJewel::CenterWindow()
{
	SetPos((int)PositionX_The_Mid((float)WINDOW_W), (int)PositionY_The_Mid((float)WINDOW_H));
}

void SEASON3B::CNewUIInventoryJewel::SetPos(int x, int y)
{
	m_Pos.x = x;
	m_Pos.y = y;

	// Selected jewel preview (compact, no gold square)
	m_nRectItem.left = m_Pos.x + (WINDOW_W - PREVIEW_SIZE) / 2;
	m_nRectItem.top = m_Pos.y + 40;

	// Qty option row is rendered manually

	// Page L/R — same style as Event (20x23 InGameShop page buttons)
	const int pageY = m_Pos.y + 368;
	const int pageLX = m_Pos.x + WINDOW_W / 2 - 52;
	const int pageRX = m_Pos.x + WINDOW_W / 2 + 32;
	m_ButtonBack.SetPos(pageLX, pageY);
	m_ButtonNext.SetPos(pageRX, pageY);
	m_ButtonBack.ChangeButtonInfo(pageLX, pageY, 20, 23);
	m_ButtonNext.ChangeButtonInfo(pageRX, pageY, 20, 23);
}

void SEASON3B::CNewUIInventoryJewel::InitButtons()
{
	// Same page arrows as Event board
	m_ButtonBack.ChangeButtonImgState(true, CNewUIInGameShop::IMAGE_IGS_PAGE_LEFT, true);
	m_ButtonNext.ChangeButtonImgState(true, CNewUIInGameShop::IMAGE_IGS_PAGE_RIGHT, true);
	const int pageY = m_Pos.y + 368;
	const int pageLX = m_Pos.x + WINDOW_W / 2 - 52;
	const int pageRX = m_Pos.x + WINDOW_W / 2 + 32;
	m_ButtonBack.ChangeButtonInfo(pageLX, pageY, 20, 23);
	m_ButtonNext.ChangeButtonInfo(pageRX, pageY, 20, 23);
}

void SEASON3B::CNewUIInventoryJewel::GetDepositRect(float& x, float& y, float& w, float& h) const
{
	// Action plates
	w = 110.f;
	h = 28.f;
	x = m_Pos.x + 28.f;
	y = m_Pos.y + (float)WINDOW_H - 48.f;
}

void SEASON3B::CNewUIInventoryJewel::GetWithdrawRect(float& x, float& y, float& w, float& h) const
{
	w = 110.f;
	h = 28.f;
	x = m_Pos.x + (float)WINDOW_W - 28.f - w;
	y = m_Pos.y + (float)WINDOW_H - 48.f;
}

bool SEASON3B::CNewUIInventoryJewel::HitCircle(float x, float y, float w, float h) const
{
	// Rectangular hit for Deposit/Withdraw plates
	return SEASON3B::IsRelease(VK_LBUTTON) && SEASON3B::CheckMouseIn(x, y, w, h);
}

bool SEASON3B::CNewUIInventoryJewel::Render()
{
	EnableAlphaTest();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	RenderFrame();
	RenderInter();
	RenderTexts();
	RenderButtons();
	RenderHoly();

	DisableAlphaBlend();
	return true;
}

bool SEASON3B::CNewUIInventoryJewel::Update()
{
	return true;
}

bool SEASON3B::CNewUIInventoryJewel::UpdateMouseEvent()
{
	const float listL = ListLeft();
	const float listT = ListTop();
	const float listW = ListWidth();

	// Close (X)
	if (SEASON3B::IsRelease(VK_LBUTTON) &&
		SEASON3B::CheckMouseIn(m_Pos.x + WINDOW_W - 22.f, m_Pos.y + 6.f, 16.f, 16.f))
	{
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_INVENTORY_JEWEL);
		PlayBuffer(SOUND_CLICK01);
		return false;
	}

	// Qty 1 / 10 / 20 / 30 (below separator)
	float optX = m_Pos.x + 40.f;
	float optY = m_Pos.y + 128.f;
	for (int i = 0; i < 4; i++)
	{
		if (SEASON3B::IsRelease(VK_LBUTTON) && SEASON3B::CheckMouseIn(optX + (i * 70.f), optY, 50.f, 22.f))
		{
			m_nOptionCurrent = i;
			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}

	m_dwCurIndex = -1;
	size_t index = (size_t)(m_nSelPage * MAX_JEWEL_INVENTORY);
	float rowY = listT;

	for (size_t i = index; i < m_bItems.size() && i < index + MAX_JEWEL_INVENTORY; i++)
	{
		if (SEASON3B::CheckMouseIn(listL, rowY, listW, (float)ROW_H - 2.f))
		{
			m_dwCurIndex = (int)i;
			if (SEASON3B::IsRelease(VK_LBUTTON))
			{
				m_dwSelIndex = (int)i;
				PlayBuffer(SOUND_CLICK01);
				return false;
			}
		}
		rowY += (float)ROW_H;
	}

	if (m_ButtonNext.UpdateMouseEvent())
	{
		if (m_nSelPage < m_nMaxPage)
			m_nSelPage++;
		if (m_nSelPage == m_nMaxPage)
			m_ButtonNext.Lock();
		if (m_nSelPage > 0)
			m_ButtonBack.UnLock();
		return false;
	}

	if (m_ButtonBack.UpdateMouseEvent())
	{
		if (m_nSelPage > 0)
			m_nSelPage--;
		if (m_nSelPage == 0)
			m_ButtonBack.Lock();
		if (m_nSelPage < m_nMaxPage)
			m_ButtonNext.UnLock();
		return false;
	}

	// Deposit (store) — circular bau icon
	{
		float bx, by, bw, bh;
		GetDepositRect(bx, by, bw, bh);
		if (HitCircle(bx, by, bw, bh))
		{
			if (m_dwSelIndex != -1)
				SendRequestAddJewelOfInventory(m_dwSelIndex, 0, 0);
			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}

	// Withdraw — circular bau icon
	{
		float bx, by, bw, bh;
		GetWithdrawRect(bx, by, bw, bh);
		if (HitCircle(bx, by, bw, bh))
		{
			check_budget(1);
			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}

	// Eat clicks on panel
	if (SEASON3B::CheckMouseIn((float)m_Pos.x, (float)m_Pos.y, (float)WINDOW_W, (float)WINDOW_H))
		return false;

	return true;
}

bool SEASON3B::CNewUIInventoryJewel::UpdateKeyEvent()
{
	if (IsVisible() && SEASON3B::IsPress(VK_ESCAPE))
	{
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_INVENTORY_JEWEL);
		return false;
	}
	return true;
}

float SEASON3B::CNewUIInventoryJewel::GetLayerDepth()
{
	return 6.5f; // above inventory depth when standalone
}

bool SEASON3B::CNewUIInventoryJewel::CheckExpansionInventory()
{
	if (IsVisible())
		return CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_W, WINDOW_H) != false;
	return false;
}

void SEASON3B::CNewUIInventoryJewel::OpenningProcess()
{
	CenterWindow();
	m_nSelPage = 0;
	m_dwSelIndex = -1;
	m_dwCurIndex = -1;
	m_ButtonBack.Lock();
	m_ButtonNext.UnLock();
	m_nOptionCurrent = 0;
}

void SEASON3B::CNewUIInventoryJewel::ClosingProcess()
{
	m_nSelPage = 0;
	m_dwSelIndex = -1;
	m_dwCurIndex = -1;
	m_ButtonBack.Lock();
	m_ButtonNext.UnLock();
}

void SEASON3B::CNewUIInventoryJewel::RenderFrame()
{
	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;
	const float w = (float)WINDOW_W;
	const float h = (float)WINDOW_H;

	// Menu Custom UI chrome
	SEASON3B::RenderImageF(IMAGE_BG_SERVER_INTERFACE_250, x, y, w, h, 0.0, 0.0, 250.0, 250.0);

	// Title bar
	SEASON3B::RenderImageF(IMAGE_TITTLE_SERVER_INTERFACE, x + (w - 140.f) * 0.5f, y + 12.f, 140.f, 22.f, 0.0, 0.0, 223.0, 32.0);

	// Close
	SEASON3B::RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, x + w - 22.f, y + 6.f, 16.f, 16.f, 0.0, 0.0, 15.0, 15.0);
	if (SEASON3B::CheckMouseIn(x + w - 22.f, y + 6.f, 16.f, 16.f))
	{
		glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
		SEASON3B::RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, x + w - 22.f, y + 6.f, 16.f, 16.f, 0.0, 0.0, 15.0, 15.0);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// No gold square around selected jewel — icon only (see RenderHoly)

	// Separator line between stock count and Qty chips
	{
		const float lineY = y + 124.f;
		const float lineL = x + 28.f;
		const float lineW = w - 56.f;
		EnableAlphaTest(true);
		glColor4ub(180, 150, 60, 200); // soft gold line
		RenderColor(lineL, lineY, lineW, 1.f, 0.0, 0);
		glColor4ub(80, 70, 40, 120);
		RenderColor(lineL, lineY + 1.f, lineW, 1.f, 0.0, 0);
		EndRenderColor();
		glColor4f(1.f, 1.f, 1.f, 1.f);
	}

	// List panel background strip
	EnableAlphaTest(true);
	glColor4ub(0, 0, 0, 120);
	RenderColor(ListLeft() - 2.f, ListTop() - 4.f, ListWidth() + 4.f, (float)(LIST_ROWS * ROW_H) + 8.f, 0.0, 0);
	EndRenderColor();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	// Page plate
	SEASON3B::RenderImageF(CNewUIInGameShop::IMAGE_IGS_STORAGE_PAGE,
		x + (w - 58.f) * 0.5f, y + 364.f, 58.f, 22.f, 0.0, 0.0, 80.0, 30.0);
}

void SEASON3B::CNewUIInventoryJewel::RenderTexts()
{
	char strText[128];
	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;
	const float w = (float)WINDOW_W;

	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetTextColor(CLRDW_YELLOW);

	// Title
	sprintf_s(strText, "%s", gTextClien.TextClien_Khac[3]);
	if (strText[0] == 0)
		strcpy_s(strText, "Banco de Joias");
	g_pRenderText->RenderText(x + (w - 140.f) * 0.5f, y + 12.f, strText, 140, 22, RT3_SORT_CENTER);

	// Selected item name + stock under compact preview (55px)
	// Preview top y+40 size 55 → text right under icon
	g_pRenderText->SetTextColor(CLRDW_WHITE);
	if (m_dwSelIndex >= 0 && m_dwSelIndex < (int)m_bItems.size())
	{
		WareHoly* item = &m_bItems[m_dwSelIndex];
		Script_Item* item_info = GMItemMng->find(item->GetIndex());
		if (item_info && item_info->Name[0] != '\0')
			g_pRenderText->RenderText(x, y + 96.f, item_info->Name, (int)w, 14, RT3_SORT_CENTER);
		else
			g_pRenderText->RenderText(x, y + 96.f, "Joia", (int)w, 14, RT3_SORT_CENTER);

		g_pRenderText->SetTextColor(CLRDW_GOLD);
		sprintf_s(strText, "x %lld", item->GetValue());
		g_pRenderText->RenderText(x, y + 108.f, strText, (int)w, 14, RT3_SORT_CENTER);
	}
	else
	{
		g_pRenderText->SetTextColor(180, 180, 180, 255);
		g_pRenderText->RenderText(x, y + 100.f, "Selecione uma joia", (int)w, 14, RT3_SORT_CENTER);
	}

	// Qty label (below separator at y+124)
	g_pRenderText->SetTextColor(CLRDW_WHITE);
	g_pRenderText->RenderText(x + 20.f, y + 130.f, "Qtd:", 40, 14, RT3_SORT_LEFT);

	// List names
	const float listL = ListLeft();
	float rowY = ListTop();
	size_t index = (size_t)(m_nSelPage * MAX_JEWEL_INVENTORY);

	for (size_t i = index; i < m_bItems.size() && i < index + MAX_JEWEL_INVENTORY; i++)
	{
		WareHoly* item = &m_bItems[i];
		Script_Item* item_info = GMItemMng->find(item->GetIndex());

		sprintf_s(strText, "x %lld", item->GetValue());
		if (item->GetValue() <= 0)
			g_pRenderText->SetTextColor(160, 160, 160, 255);
		else
			g_pRenderText->SetTextColor(CLRDW_GOLD);

		const char* name = (item_info && item_info->Name[0] != '\0') ? item_info->Name : "Joia desconhecida";
		g_pRenderText->SetTextColor(CLRDW_WHITE);
		g_pRenderText->RenderText(listL + 36.f, rowY + 4.f, name, (int)ListWidth() - 100, 14, RT3_SORT_LEFT);

		if (item->GetValue() <= 0)
			g_pRenderText->SetTextColor(160, 160, 160, 255);
		else
			g_pRenderText->SetTextColor(CLRDW_GOLD);
		g_pRenderText->RenderText(listL + 36.f, rowY + 18.f, strText, (int)ListWidth() - 50, 14, RT3_SORT_LEFT);

		rowY += (float)ROW_H;
	}

	// Page number
	g_pRenderText->SetTextColor(CLRDW_WHITE);
	sprintf_s(strText, "%d / %d", m_nSelPage + 1, m_nMaxPage + 1);
	g_pRenderText->RenderText(x + (w - 58.f) * 0.5f, y + 364.f, strText, 58, 22, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIInventoryJewel::RenderInter()
{
	const float listL = ListLeft();
	const float listW = ListWidth();
	float rowY = ListTop();
	size_t index = (size_t)(m_nSelPage * MAX_JEWEL_INVENTORY);

	EnableAlphaTest(true);

	for (size_t i = index; i < m_bItems.size() && i < index + MAX_JEWEL_INVENTORY; i++)
	{
		// Alternating row tint
		if (((i - index) % 2) == 0)
			glColor4ub(20, 22, 40, 160);
		else
			glColor4ub(12, 14, 28, 140);
		RenderColor(listL, rowY, listW, (float)ROW_H - 3.f, 0.0, 0);
		rowY += (float)ROW_H;
	}
	EndRenderColor();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	rowY = ListTop();
	if (m_dwCurIndex != -1 && m_dwCurIndex >= (int)index && m_dwCurIndex < (int)index + MAX_JEWEL_INVENTORY)
	{
		FrameTarget(listL, rowY + ((m_dwCurIndex - (int)index) * (float)ROW_H), listW, (float)ROW_H - 3.f, RGBA(218, 180, 60, 0xff));
	}
	if (m_dwSelIndex != -1 && m_dwSelIndex != m_dwCurIndex &&
		m_dwSelIndex >= (int)index && m_dwSelIndex < (int)index + MAX_JEWEL_INVENTORY)
	{
		FrameTarget(listL, rowY + ((m_dwSelIndex - (int)index) * (float)ROW_H), listW, (float)ROW_H - 3.f, RGBA(98, 223, 31, 0xff));
	}
}

void SEASON3B::CNewUIInventoryJewel::RenderButtons()
{
	// Old ugly text plates removed — Deposit/Withdraw = bau jewel medallion
	RenderActionButtons();
	m_ButtonNext.Render();
	m_ButtonBack.Render();
	render_option_group();
}

void SEASON3B::CNewUIInventoryJewel::RenderActionButtons()
{
	// Deposit = green plate | Withdraw = red plate
	float dx, dy, dw, dh;
	float wx, wy, ww, wh;
	GetDepositRect(dx, dy, dw, dh);
	GetWithdrawRect(wx, wy, ww, wh);

	const bool hoverDep = SEASON3B::CheckMouseIn(dx, dy, dw, dh);
	const bool hoverWdr = SEASON3B::CheckMouseIn(wx, wy, ww, wh);

	EnableAlphaTest(true);

	// Depositar — green
	if (hoverDep)
		glColor4ub(40, 160, 50, 235);
	else
		glColor4ub(25, 120, 35, 215);
	RenderColor(dx, dy, dw, dh, 0.0, 0);
	glColor4ub(90, 220, 100, 255);
	RenderColor(dx, dy, dw, 1.f, 0.0, 0);
	RenderColor(dx, dy + dh - 1.f, dw, 1.f, 0.0, 0);
	RenderColor(dx, dy, 1.f, dh, 0.0, 0);
	RenderColor(dx + dw - 1.f, dy, 1.f, dh, 0.0, 0);
	EndRenderColor();

	// Retirar — red
	if (hoverWdr)
		glColor4ub(180, 40, 40, 230);
	else
		glColor4ub(140, 25, 25, 210);
	RenderColor(wx, wy, ww, wh, 0.0, 0);
	glColor4ub(220, 80, 80, 255);
	RenderColor(wx, wy, ww, 1.f, 0.0, 0);
	RenderColor(wx, wy + wh - 1.f, ww, 1.f, 0.0, 0);
	RenderColor(wx, wy, 1.f, wh, 0.0, 0);
	RenderColor(wx + ww - 1.f, wy, 1.f, wh, 0.0, 0);
	EndRenderColor();

	glColor4f(1.f, 1.f, 1.f, 1.f);

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetTextColor(hoverDep ? CLRDW_YELLOW : CLRDW_WHITE);
	g_pRenderText->RenderText(dx, dy, "Depositar", (int)dw, (int)dh, RT3_SORT_CENTER);

	g_pRenderText->SetTextColor(hoverWdr ? CLRDW_YELLOW : CLRDW_WHITE);
	g_pRenderText->RenderText(wx, wy, "Retirar", (int)ww, (int)wh, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIInventoryJewel::RenderHoly()
{
	SEASON3B::begin3D();

	const float listL = ListLeft();
	float rowY = ListTop();
	size_t index = (size_t)(m_nSelPage * MAX_JEWEL_INVENTORY);

	for (size_t i = index; i < m_bItems.size() && i < index + MAX_JEWEL_INVENTORY; i++)
	{
		WareHoly* item = &m_bItems[i];
		Render2Item3D(listL + 6.f, rowY + 5.f, 24.f, 24.f, item->GetIndex(), item->GetLevel(), 0, 0, false);
		rowY += (float)ROW_H;
	}

	if (m_dwSelIndex != -1 && m_dwSelIndex < (int)m_bItems.size())
	{
		WareHoly* item = &m_bItems[m_dwSelIndex];
		// IMPORTANT: Render2Item3D() ignores Width/Height (always uses inventory 1x1 slot).
		// Use RenderItem3D with explicit box size + Scale so the jewel actually grows.
		const float sx = (float)m_nRectItem.left;
		const float sy = (float)m_nRectItem.top;
		const float box = (float)PREVIEW_SIZE;

		float scale = 0.f;
		if (g_pNewItemScript)
		{
			ITEM_POSITION* rp = g_pNewItemScript->FindItemRelative(item->GetIndex());
			if (rp)
			{
				// 20 ≈ normal inventory 1-slot cell used by Render2Item3D
				const float baseCell = 20.f;
				scale = rp->scale * (box / baseCell);
			}
		}
		if (scale <= 0.f)
			scale = 0.0028f; // compact fallback (~half of previous large preview)

		RenderItem3D(sx, sy, box, box,
			item->GetIndex(), item->GetLevel(), 0, 0, false,
			0.f, 0.f, 0.f, 0.f, scale);
	}

	SEASON3B::endrender3D();
}

void SEASON3B::CNewUIInventoryJewel::FrameTarget(float iPos_x, float iPos_y, float width, float height, DWORD color)
{
	EnableAlphaTest(true);
	BYTE red = GetRed(color);
	BYTE green = GetGreen(color);
	BYTE blue = GetBlue(color);

	glColor4ub(red, green, blue, 40);
	RenderColor(iPos_x, iPos_y, width, height, 0.0, 0);

	glColor4ub(red, green, blue, 255);
	RenderColor(iPos_x, iPos_y, width, 1.f, 0.0, 0);
	RenderColor(iPos_x, iPos_y + height, width, 1.f, 0.0, 0);
	RenderColor(iPos_x, iPos_y + 1.f, 1.f, height - 1.f, 0.0, 0);
	RenderColor(iPos_x + width - 1.f, iPos_y + 1.f, 1.f, height - 1.f, 0.0, 0);

	EndRenderColor();
	glColor4f(1.f, 1.f, 1.f, 1.f);
}

void SEASON3B::CNewUIInventoryJewel::render_option_group()
{
	const char* _num[] = { "1", "10", "20", "30" };
	float optX = m_Pos.x + 40.f;
	float optY = m_Pos.y + 128.f; // below separator line

	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetFont(g_hFontBold);

	for (int i = 0; i < 4; i++)
	{
		const bool sel = (m_nOptionCurrent == i);
		// Chip background
		if (sel)
			glColor4ub(60, 90, 40, 200);
		else
			glColor4ub(30, 32, 50, 180);
		EnableAlphaTest(true);
		RenderColor(optX + i * 70.f, optY, 50.f, 22.f, 0.0, 0);
		EndRenderColor();
		glColor4f(1.f, 1.f, 1.f, 1.f);

		if (sel)
			g_pRenderText->SetTextColor(CLRDW_YELLOW);
		else
			g_pRenderText->SetTextColor(CLRDW_WHITE);
		g_pRenderText->RenderText(optX + i * 70.f, optY, _num[i], 50, 22, RT3_SORT_CENTER);
	}
}

void SEASON3B::CNewUIInventoryJewel::RemoveData()
{
	m_bItems.clear();
}

void SEASON3B::CNewUIInventoryJewel::InsertData(BYTE Index, short ItemIndex, short ItemLevel, __int64 count)
{
	if (ItemIndex >= ITEM_WING && ItemIndex < MAX_ITEM)
	{
		m_bItems.push_back({ Index, ItemIndex, ItemLevel, count });
	}
	m_nMaxPage = (int)(m_bItems.size() / MAX_JEWEL_INVENTORY);
}

bool SEASON3B::CNewUIInventoryJewel::check_budget(DWORD _iCount)
{
	if (m_dwSelIndex >= 0 && m_dwSelIndex < (int)m_bItems.size())
	{
		SendRequestRemoveJewelOfInventory(m_dwSelIndex, m_nOptionCurrent, _iCount);
	}
	return false;
}
