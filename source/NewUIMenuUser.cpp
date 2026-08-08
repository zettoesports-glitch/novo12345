#include "stdafx.h"
#include "CGMProtect.h"
#include "NewUISystem.h"
#include "DSPlaySound.h"
#include "CGMFrame.h"
#include "UIControls.h"
#include "CustomEventTime.h"
#include "NewUIMenuUser.h"
#include "NewUISecurityPanel.h"
#include "Protocol.h"
#include "_TextureIndex.h"

SEASON3B::CNewUIMenuUser::CNewUIMenuUser()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;
}

SEASON3B::CNewUIMenuUser::~CNewUIMenuUser()
{
	Release();
}

bool SEASON3B::CNewUIMenuUser::Create(CNewUIManager* pNewUIMng, float x, float y)
{
	if (!pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(INTERFACE_CUSTOM_MENU, this);
	RegisterPhotoModeKeepUI(this);

	LoadImages();
	SetPos(x, y);
	Show(false);
	return true;
}

void SEASON3B::CNewUIMenuUser::Release()
{
	UnloadImages();
	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUIMenuUser::SetPos(float x, float y)
{
	m_Pos.x = (LONG)x;
	m_Pos.y = (LONG)y;
}

void SEASON3B::CNewUIMenuUser::LoadImages()
{
	LoadBitmap("Interface\\HUD\\bg250.tga", IMAGE_BG_SERVER_INTERFACE_250, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\timerserver-bar.tga", IMAGE_TITTLE_SERVER_INTERFACE, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\button_close_navi.tga", IMAGE_BUTTON_CLOSE_NAVI, GL_LINEAR);

	// 80x80 .tga/.ozt from Imagem pack
	LoadBitmap("Interface\\HUD\\.btn\\jewel.tga", IMAGE_MENU_BTN_JEWEL, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	LoadBitmap("Interface\\HUD\\.btn\\vip.tga", IMAGE_MENU_BTN_VIP, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	LoadBitmap("Interface\\HUD\\.btn\\event.tga", IMAGE_MENU_BTN_EVENT, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	LoadBitmap("Interface\\HUD\\.btn\\rank.tga", IMAGE_MENU_BTN_RANK, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	LoadBitmap("Interface\\HUD\\.btn\\lock.tga", IMAGE_MENU_BTN_LOCK, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	LoadBitmap("Interface\\HUD\\.btn\\senha.tga", IMAGE_MENU_BTN_SENHA, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
}

void SEASON3B::CNewUIMenuUser::UnloadImages()
{
	DeleteBitmap(IMAGE_MENU_BTN_JEWEL);
	DeleteBitmap(IMAGE_MENU_BTN_VIP);
	DeleteBitmap(IMAGE_MENU_BTN_EVENT);
	DeleteBitmap(IMAGE_MENU_BTN_RANK);
	DeleteBitmap(IMAGE_MENU_BTN_LOCK);
	DeleteBitmap(IMAGE_MENU_BTN_SENHA);
}

bool SEASON3B::CNewUIMenuUser::UpdateKeyEvent()
{
	if (IsVisible() && SEASON3B::IsPress(VK_ESCAPE))
	{
		g_pNewUISystem->Hide(INTERFACE_CUSTOM_MENU);
		return false;
	}
	return true;
}

// 3x2 grid of circular icons
bool SEASON3B::CNewUIMenuUser::GetBtnRect(int index, float& outX, float& outY, float& outW, float& outH) const
{
	if (index < 0 || index >= BTN_COUNT)
		return false;

	const float s = (float)ICON_BTN;
	const float gapX = 12.f;
	const float gapY = 22.f; // space for label
	const float totalW = s * (float)BTN_COLS + gapX * (float)(BTN_COLS - 1);
	const float startX = m_Pos.x + ((float)WINDOW_W - totalW) * 0.5f;
	const float startY = m_Pos.y + 48.f;

	const int col = index % BTN_COLS;
	const int row = index / BTN_COLS;

	outW = outH = s;
	outX = startX + col * (s + gapX);
	outY = startY + row * (s + gapY);
	return true;
}

GLuint SEASON3B::CNewUIMenuUser::GetBtnTexture(int index) const
{
	switch (index)
	{
	case BTN_JEWEL: return IMAGE_MENU_BTN_JEWEL;
	case BTN_VIP:   return IMAGE_MENU_BTN_VIP;
	case BTN_EVENT: return IMAGE_MENU_BTN_EVENT;
	case BTN_RANK:  return IMAGE_MENU_BTN_RANK;
	case BTN_LOCK:  return IMAGE_MENU_BTN_LOCK;
	case BTN_SENHA: return IMAGE_MENU_BTN_SENHA;
	default: return IMAGE_MENU_BTN_JEWEL;
	}
}

const char* SEASON3B::CNewUIMenuUser::GetBtnLabel(int index) const
{
	switch (index)
	{
	case BTN_JEWEL: return "Joias";
	case BTN_VIP:   return "VIP";
	case BTN_EVENT: return "Eventos";
	case BTN_RANK:  return "Ranking";
	case BTN_LOCK:  return g_bAccountItemsLocked ? "Locked" : "Lock item";
	case BTN_SENHA: return "Senha"; // senha da CONTA (login)
	default: return "";
	}
}

int SEASON3B::CNewUIMenuUser::HitTestBtn() const
{
	if (!SEASON3B::IsRelease(VK_LBUTTON))
		return -1;

	const float mx = (float)MouseX;
	const float my = (float)MouseY;

	for (int i = 0; i < BTN_COUNT; ++i)
	{
		float x, y, w, h;
		if (!GetBtnRect(i, x, y, w, h))
			continue;
		const float cx = x + w * 0.5f;
		const float cy = y + h * 0.5f;
		const float r = w * 0.5f;
		const float dx = mx - cx;
		const float dy = my - cy;
		if ((dx * dx + dy * dy) <= (r * r))
			return i;
	}
	return -1;
}

bool SEASON3B::CNewUIMenuUser::IsClickClose() const
{
	const float cx = m_Pos.x + (float)WINDOW_W - 20.0f;
	const float cy = m_Pos.y + 5.0f;
	return SEASON3B::CheckMouseIn(cx, cy, 16.0f, 16.0f) && SEASON3B::IsRelease(VK_LBUTTON);
}

bool SEASON3B::CNewUIMenuUser::UpdateMouseEvent()
{
	if (!SEASON3B::CheckMouseIn((float)m_Pos.x, (float)m_Pos.y, (float)WINDOW_W, (float)WINDOW_H))
		return true;

	if (IsClickClose())
	{
		g_pNewUISystem->Hide(INTERFACE_CUSTOM_MENU);
		PlayBuffer(SOUND_CLICK01);
		return false;
	}

	const int hit = HitTestBtn();
	if (hit >= 0)
	{
		g_pNewUISystem->Hide(INTERFACE_CUSTOM_MENU);
		PlayBuffer(SOUND_CLICK01);

		switch (hit)
		{
		case BTN_JEWEL:
			g_pNewUISystem->Toggle(SEASON3B::INTERFACE_INVENTORY_JEWEL);
			break;
		case BTN_VIP:
			if (gmProtect->WindowsVipShop)
				g_pNewUISystem->Show(SEASON3B::INTERFACE_SHOW_VIP);
			break;
		case BTN_EVENT:
			if (g_CustomEventTime)
				g_CustomEventTime->OnOffWindow();
			else if (gmProtect->WindowsEventTime)
				g_pNewUISystem->Show(SEASON3B::INTERFACE_EVENT_TIME);
			break;
		case BTN_RANK:
			if (gmProtect->WindowsRankTop)
				g_pNewUISystem->Show(SEASON3B::INTERFACE_RANKING_TOP);
			break;
		case BTN_LOCK:
			if (g_pNewUISecurity)
				g_pNewUISecurity->OpenMode(CNewUISecurityPanel::MODE_LOCK);
			break;
		case BTN_SENHA:
			if (g_pNewUISecurity)
				g_pNewUISecurity->OpenMode(CNewUISecurityPanel::MODE_PASS);
			break;
		}
		return false;
	}

	return false;
}

bool SEASON3B::CNewUIMenuUser::Render()
{
	EnableAlphaTest(true);
	glColor4f(1.f, 1.f, 1.f, 1.f);

	RenderFrame();
	RenderMenuButtons();

	DisableAlphaBlend();
	return true;
}

bool SEASON3B::CNewUIMenuUser::Update()
{
	return true;
}

float SEASON3B::CNewUIMenuUser::GetLayerDepth()
{
	return 10.0f;
}

void SEASON3B::CNewUIMenuUser::OpenningProcess()
{
	m_Pos.x = (LONG)PositionX_The_Mid((float)WINDOW_W);
	m_Pos.y = (LONG)PositionY_The_Mid((float)WINDOW_H);
}

void SEASON3B::CNewUIMenuUser::ClosingProcess()
{
}

void SEASON3B::CNewUIMenuUser::RenderFrame()
{
	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;
	const float w = (float)WINDOW_W;
	const float h = (float)WINDOW_H;

	SEASON3B::RenderImageF(IMAGE_BG_SERVER_INTERFACE_250, x, y, w, h, 0.0, 0.0, 250.0, 250.0);
	SEASON3B::RenderImageF(IMAGE_TITTLE_SERVER_INTERFACE, x + ((w - 105.f) / 2.f), y + 12.f, 105.f, 22.f, 0.0, 0.0, 223.0, 32.0);

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetTextColor(CLRDW_YELLOW);
	g_pRenderText->RenderText(x + ((w - 105.f) / 2.f), y + 12.f, "Menu", 105, 22, RT3_SORT_CENTER);

	SEASON3B::RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, x + w - 20.0f, y + 5.0f, 16.0f, 16.0f, 0.0, 0.0, 15.0, 15.0);
	if (SEASON3B::CheckMouseIn(x + w - 20.0f, y + 5.0f, 16.0f, 16.0f))
	{
		glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
		SEASON3B::RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, x + w - 20.0f, y + 5.0f, 16.0f, 16.0f, 0.0, 0.0, 15.0, 15.0);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void SEASON3B::CNewUIMenuUser::RenderMenuButtons()
{
	const float mx = (float)MouseX;
	const float my = (float)MouseY;

	glColor4f(1.f, 1.f, 1.f, 1.f);
	EnableAlphaTest(true);

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0);

	for (int i = 0; i < BTN_COUNT; ++i)
	{
		float bx, by, bw, bh;
		if (!GetBtnRect(i, bx, by, bw, bh))
			continue;

		const float cx = bx + bw * 0.5f;
		const float cy = by + bh * 0.5f;
		const float r = bw * 0.5f;
		const float dx = mx - cx;
		const float dy = my - cy;
		const bool hover = (dx * dx + dy * dy) <= (r * r);

		SEASON3B::RenderImageF(GetBtnTexture(i), bx, by, bw, bh, 0.0, 0.0, (float)ICON_TEX, (float)ICON_TEX);

		// Badge vermelho no icone de Travar quando itens estao travados
		if (i == BTN_LOCK && g_bAccountItemsLocked)
		{
			EnableAlphaTest(true);
			glColor4ub(220, 40, 40, 230);
			RenderColor(bx + bw - 18.f, by + 4.f, 14.f, 14.f, 0.0f, 0);
			EndRenderColor();
			glColor4f(1.f, 1.f, 1.f, 1.f);
			g_pRenderText->SetTextColor(CLRDW_WHITE);
			g_pRenderText->RenderText(bx + bw - 18.f, by + 4.f, "!", 14, 14, RT3_SORT_CENTER);
		}

		if (i == BTN_LOCK && g_bAccountItemsLocked)
			g_pRenderText->SetTextColor(hover ? CLRDW_ORANGE : CLRDW_RED);
		else
			g_pRenderText->SetTextColor(hover ? CLRDW_YELLOW : CLRDW_WHITE);
		g_pRenderText->RenderText(bx, by + bh + 2.f, GetBtnLabel(i), (int)bw, 14, RT3_SORT_CENTER);
	}
}
