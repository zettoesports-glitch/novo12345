#include "stdafx.h"
#include "CGMProtect.h"
#include "NewUISystem.h"
#include "wsclientinline.h"
#include "NewUIPurcharseVip.h"
#include "DSPlaySound.h"
#include "UIBaseDef.h"
#include "_TextureIndex.h"

using namespace SEASON3B;

SEASON3B::CNewUIPurcharseVip::CNewUIPurcharseVip()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;
	targetIndex = 0;
}

SEASON3B::CNewUIPurcharseVip::~CNewUIPurcharseVip()
{
	Release();
}

bool SEASON3B::CNewUIPurcharseVip::Create(CNewUIManager* pNewUIMng, float x, float y)
{
	if (!pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(INTERFACE_SHOW_VIP, this);

	this->LoadImages();
	this->SetPos(x, y);
	this->SetInfo();
	this->Show(false);
	return true;
}

void SEASON3B::CNewUIPurcharseVip::Release()
{
	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		this->UnloadImages();
		shopList.clear();
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUIPurcharseVip::SetPos(float x, float y)
{
	m_Pos.x = (LONG)x;
	m_Pos.y = (LONG)y;
}

void SEASON3B::CNewUIPurcharseVip::SetInfo()
{
	targetIndex = 0;
	shopList.resize(GMProtect->CustomBuyVip.size());
	std::copy(GMProtect->CustomBuyVip.begin(), GMProtect->CustomBuyVip.end(), shopList.begin());
}

void SEASON3B::CNewUIPurcharseVip::LoadImages()
{
	// Menu Custom chrome
	LoadBitmap("Interface\\HUD\\bg250.tga", IMAGE_BG_SERVER_INTERFACE_250, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\timerserver-bar.tga", IMAGE_TITTLE_SERVER_INTERFACE, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\button_close_navi.tga", IMAGE_BUTTON_CLOSE_NAVI, GL_LINEAR);

	// VIP content
	LoadBitmap("Interface\\HUD\\membership_data.tga", IMAGE_MEMBERSHIP_DATA, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\button_arrow_L.tga", IMAGE_button_arrow_l, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\button_arrow_R.tga", IMAGE_button_arrow_r, GL_LINEAR);
}

void SEASON3B::CNewUIPurcharseVip::UnloadImages()
{
	DeleteBitmap(IMAGE_MEMBERSHIP_DATA);
	DeleteBitmap(IMAGE_button_arrow_l);
	DeleteBitmap(IMAGE_button_arrow_r);
}

float SEASON3B::CNewUIPurcharseVip::BuyBtnX() const
{
	return (float)m_Pos.x + ((float)WINDOW_W - BuyBtnW()) * 0.5f;
}

float SEASON3B::CNewUIPurcharseVip::BuyBtnY() const
{
	return (float)m_Pos.y + (float)WINDOW_H - 55.f;
}

float SEASON3B::CNewUIPurcharseVip::BuyBtnW() const
{
	return 140.f;
}

float SEASON3B::CNewUIPurcharseVip::BuyBtnH() const
{
	return 30.f;
}

bool SEASON3B::CNewUIPurcharseVip::UpdateKeyEvent()
{
	if (IsVisible() == true)
	{
		if (SEASON3B::IsPress(VK_ESCAPE))
		{
			g_pNewUISystem->Hide(INTERFACE_SHOW_VIP);
			return false;
		}

		if (this->Checktarget())
		{
			if (SEASON3B::IsRelease(VK_LEFT))
			{
				if (targetIndex > 0)
					targetIndex--;
				return false;
			}
			if (SEASON3B::IsRelease(VK_RIGHT))
			{
				if (targetIndex < shopList.size() - 1)
					targetIndex++;
				return false;
			}
		}
	}
	return true;
}

bool SEASON3B::CNewUIPurcharseVip::UpdateMouseEvent()
{
	const float w = (float)WINDOW_W;
	const float h = (float)WINDOW_H;
	const float RenderFrameX = (float)m_Pos.x;
	const float RenderFrameY = (float)m_Pos.y;

	if (!SEASON3B::CheckMouseIn(RenderFrameX, RenderFrameY, w, h))
		return true;

	// Close (Menu Custom)
	const float closeX = RenderFrameX + w - 22.f;
	const float closeY = RenderFrameY + 8.f;
	if (SEASON3B::CheckMouseIn(closeX, closeY, 16.f, 16.f) && SEASON3B::IsRelease(VK_LBUTTON))
	{
		g_pNewUISystem->Hide(INTERFACE_SHOW_VIP);
		PlayBuffer(SOUND_CLICK01);
		return false;
	}

	// Prev / Next arrows
	const float midY = RenderFrameY + 70.f;
	if (targetIndex > 0)
	{
		if (SEASON3B::CheckMouseIn(RenderFrameX + 18.f, midY, 30.f, 48.f) && SEASON3B::IsRelease(VK_LBUTTON))
		{
			targetIndex--;
			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}
	if (targetIndex + 1 < shopList.size())
	{
		if (SEASON3B::CheckMouseIn(RenderFrameX + w - 48.f, midY, 30.f, 48.f) && SEASON3B::IsRelease(VK_LBUTTON))
		{
			targetIndex++;
			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}

	if (this->Checktarget())
	{
		TEMPLATE_MEMBERSHIP* Data = &shopList[targetIndex];

		if ((CharacterMachine->AccountLevel < Data->AccountLevel)
			&& (Data->Coin[0] != 0 || Data->Coin[1] != 0 || Data->Coin[2] != 0))
		{
			if (SEASON3B::CheckMouseIn(BuyBtnX(), BuyBtnY(), BuyBtnW(), BuyBtnH())
				&& SEASON3B::IsRelease(VK_LBUTTON))
			{
				CreateOkCancelMessageBox(GlobalText[2897], -1, 3.f, ButtonOK);
				return false;
			}
		}
	}

	return false; // block click-through
}

bool SEASON3B::CNewUIPurcharseVip::Render()
{
	EnableAlphaTest(true);
	glColor4f(1.f, 1.f, 1.f, 1.f);

	RenderFrame();

	if (this->Checktarget())
	{
		RenderButtons();
		RenderTexte();
	}

	DisableAlphaBlend();
	return true;
}

bool SEASON3B::CNewUIPurcharseVip::Update()
{
	return true;
}

float SEASON3B::CNewUIPurcharseVip::GetLayerDepth()
{
	return 10.0f;
}

void SEASON3B::CNewUIPurcharseVip::OpenningProcess()
{
	targetIndex = 0;
	SetPos(PositionX_The_Mid((float)WINDOW_W), PositionY_The_Mid((float)WINDOW_H));
	SetInfo();
}

void SEASON3B::CNewUIPurcharseVip::ClosingProcess()
{
	targetIndex = 0;
}

int SEASON3B::CNewUIPurcharseVip::GetLevelPurcharse()
{
	if (!Checktarget())
		return 0;
	return shopList[targetIndex].Index;
}

bool SEASON3B::CNewUIPurcharseVip::Checktarget()
{
	return (targetIndex < shopList.size());
}

void SEASON3B::CNewUIPurcharseVip::RenderFrame()
{
	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;
	const float w = (float)WINDOW_W;
	const float h = (float)WINDOW_H;

	// === Menu Custom UI chrome ===
	RenderImageF(IMAGE_BG_SERVER_INTERFACE_250, x, y, w, h, 0.0, 0.0, 250.0, 250.0);

	const float titleW = 140.f;
	const float titleH = 24.f;
	RenderImageF(IMAGE_TITTLE_SERVER_INTERFACE,
		x + ((w - titleW) * 0.5f), y + 12.f, titleW, titleH, 0.0, 0.0, 223.0, 32.0);

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetTextColor(CLRDW_YELLOW);
	g_pRenderText->RenderText(
		x + ((w - titleW) * 0.5f), y + 12.f, "VIP", (int)titleW, (int)titleH, RT3_SORT_CENTER);

	// Close
	const float closeX = x + w - 22.f;
	const float closeY = y + 8.f;
	RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, closeX, closeY, 16.0f, 16.0f, 0.0, 0.0, 15.0, 15.0);
	if (SEASON3B::CheckMouseIn(closeX, closeY, 16.0f, 16.0f))
	{
		glColor4f(0.55f, 0.55f, 0.55f, 1.0f);
		RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, closeX, closeY, 16.0f, 16.0f, 0.0, 0.0, 15.0, 15.0);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// Membership badge (larger)
	if (this->Checktarget())
	{
		const float iconW = 90.f;
		const float iconH = 75.f;
		const float iconX = x + (w - iconW) * 0.5f;
		const float iconY = y + 55.f;

		const int lvl = shopList[targetIndex].AccountLevel;
		float su = 2.f;
		if (lvl == 2)
			su = 177.f;
		else if (lvl == 3)
			su = 353.f;

		if (lvl >= 1 && lvl <= 3)
		{
			RenderImageF(IMAGE_MEMBERSHIP_DATA, iconX, iconY, iconW, iconH, su, 2.f, 170.f, 149.f);
		}
	}
}

void SEASON3B::CNewUIPurcharseVip::RenderTexte()
{
	char psText[MAX_PATH];
	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;
	const float w = (float)WINDOW_W;
	const float cx = x + w * 0.5f;

	TEMPLATE_MEMBERSHIP* Data = &shopList[targetIndex];

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0);

	// VIP plan name
	g_pRenderText->SetTextColor(CLRDW_CYAN);
	g_pRenderText->RenderText(cx, y + 145.f, Data->text_name, 0, 0, RT3_WRITE_CENTER);

	// Benefits
	g_pRenderText->SetFont(g_hFont);
	g_pRenderText->SetTextColor(CLRDW_WHITE);

	const float infoX = x + 50.f;
	sprintf_s(psText, TextGlobal[40], Data->CntExp);
	g_pRenderText->RenderText(infoX, y + 175.f, psText, 0, 0, RT3_SORT_LEFT);

	sprintf_s(psText, TextGlobal[41], Data->CntDrop);
	g_pRenderText->RenderText(infoX, y + 192.f, psText, 0, 0, RT3_SORT_LEFT);

	sprintf_s(psText, TextGlobal[42], Data->CntDays);
	g_pRenderText->RenderText(infoX, y + 209.f, psText, 0, 0, RT3_SORT_LEFT);

	// Price / buy label
	if ((CharacterMachine->AccountLevel < Data->AccountLevel)
		&& (Data->Coin[0] != 0 || Data->Coin[1] != 0 || Data->Coin[2] != 0))
	{
		const float bx = BuyBtnX();
		const float by = BuyBtnY();
		const float bw = BuyBtnW();
		const float bh = BuyBtnH();

		g_pRenderText->SetFont(g_hFontBold);
		g_pRenderText->SetTextColor(0x0, 0x0, 0x0, 0xFF);

		if (Data->Coin[0] != 0 && Data->Coin[1] == 0 && Data->Coin[2] == 0)
		{
			sprintf_s(psText, "%d %s", Data->Coin[0], TextGlobal[58]);
			g_pRenderText->RenderText(bx, by, psText, (int)bw, (int)bh, RT3_SORT_CENTER);
		}
		else if (Data->Coin[0] == 0 && Data->Coin[1] != 0 && Data->Coin[2] == 0)
		{
			sprintf_s(psText, "%d %s", Data->Coin[1], TextGlobal[59]);
			g_pRenderText->RenderText(bx, by, psText, (int)bw, (int)bh, RT3_SORT_CENTER);
		}
		else if (Data->Coin[0] == 0 && Data->Coin[1] == 0 && Data->Coin[2] != 0)
		{
			sprintf_s(psText, "%d %s", Data->Coin[2], TextGlobal[60]);
			g_pRenderText->RenderText(bx, by, psText, (int)bw, (int)bh, RT3_SORT_CENTER);
		}
		else
		{
			sprintf_s(psText, "[%d %s][%d %s][%d %s]",
				Data->Coin[0], TextGlobal[58],
				Data->Coin[1], TextGlobal[59],
				Data->Coin[2], TextGlobal[60]);

			g_pRenderText->SetTextColor(CLRDW_GOLD);
			g_pRenderText->RenderText(cx, by - 16.f, psText, 0, 0, RT3_WRITE_CENTER);

			g_pRenderText->SetTextColor(0x0, 0x0, 0x0, 0xFF);
			g_pRenderText->RenderText(bx, by, GlobalText[2293], (int)bw, (int)bh, RT3_SORT_CENTER);
		}
	}
	else
	{
		g_pRenderText->SetFont(g_hFont);
		g_pRenderText->SetTextColor(CLRDW_GRAY);
		g_pRenderText->RenderText(cx, BuyBtnY() + 6.f, "—", 0, 0, RT3_WRITE_CENTER);
	}
}

void SEASON3B::CNewUIPurcharseVip::RenderButtons()
{
	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;
	const float w = (float)WINDOW_W;
	const float midY = y + 70.f;

	TEMPLATE_MEMBERSHIP* Data = &shopList[targetIndex];

	if (targetIndex > 0)
	{
		RenderImage(IMAGE_button_arrow_l, x + 18.f, midY, 30.f, 48.f);
	}
	if (targetIndex + 1 < shopList.size())
	{
		RenderImage(IMAGE_button_arrow_r, x + w - 48.f, midY, 30.f, 48.f);
	}

	if ((CharacterMachine->AccountLevel < Data->AccountLevel)
		&& (Data->Coin[0] != 0 || Data->Coin[1] != 0 || Data->Coin[2] != 0))
	{
		RenderButton(BuyBtnX(), BuyBtnY(), BuyBtnW(), BuyBtnH());
		EndRenderColor();
	}
}

void SEASON3B::CNewUIPurcharseVip::RenderButton(float x, float y, float width, float height)
{
	if (SEASON3B::CheckMouseIn(x, y, width, height))
	{
		glColor4ub(0xfb, 0xdd, 0x1f, 0xFF);
		if (MouseLButton)
			glColor4ub(0xcc, 0xb2, 0x0f, 0xFF);
	}
	else
	{
		glColor4ub(0xe9, 0xcd, 0x1a, 0xFF);
	}

	RenderColor(x, y, width, height, 0.0, 0);

	glColor4ub(0xa8, 0x92, 0x08, 0xFF);
	RenderColor(x, y, width, 1, 0.0, 0);
	RenderColor(x, y, 1, height, 0.0, 0);
	RenderColor(x, y + height - 1, width, 1, 0.0, 0);
	RenderColor(x + width - 1, y, 1, height, 0.0, 0);
}

SEASON3B::CALLBACK_RESULT SEASON3B::CNewUIPurcharseVip::ButtonOK(CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam)
{
	int LevelPurcharse = g_pNewUILevelPurcharse->GetLevelPurcharse();

	SendRequestPurcharseMembership(LevelPurcharse);

	PlayBuffer(SOUND_CLICK01);

	g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);

	return CALLBACK_BREAK;
}
