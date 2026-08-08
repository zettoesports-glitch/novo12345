#include "stdafx.h"
#include "CustomEventTime.h"
#include "CBInterface.h"
#include "Util.h"
#include "TextClien.h"
#include "NewUISystem.h"
#include "UIControls.h"
#include "wsclientinline.h"
#include "DSPlaySound.h"
#include "_TextureIndex.h"
#include "UIBaseDef.h"
#include <algorithm>

using namespace SEASON3B;

// ============================================================================
// Event board:
// - Menu Custom UI, large window (all events one page)
// - Sort: soonest first; Online after upcoming; Disabled last
// - Colors: <15min / <5min / Online / rest
// - Click row to move (gate) without "clique na linha" hint
// ============================================================================

namespace
{
	// Larger than MuMobile 420x390 — full list readable
	const float kMainW = 560.f;
	const float kMainH = 520.f;
	const float kRowH = 16.f;
	const float kListTopPad = 64.f; // below header
	const float kListBotPad = 28.f;

	// Time thresholds (seconds)
	const int kNear15Min = 15 * 60; // 900
	const int kNear5Min = 5 * 60;   // 300

	void EnsureMenuCustomTextures()
	{
		static bool s_loaded = false;
		if (s_loaded)
			return;
		LoadBitmap("Interface\\HUD\\bg250.tga", IMAGE_BG_SERVER_INTERFACE_250, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\timerserver-bar.tga", IMAGE_TITTLE_SERVER_INTERFACE, GL_LINEAR);
		LoadBitmap("Interface\\HUD\\button_close_navi.tga", IMAGE_BUTTON_CLOSE_NAVI, GL_LINEAR);
		s_loaded = true;
	}

	void DrawMenuCustomFrame(float x, float y, float w, float h, const char* title)
	{
		EnsureMenuCustomTextures();

		EnableAlphaTest(true);
		glColor4f(1.f, 1.f, 1.f, 1.f);

		RenderImageF(IMAGE_BG_SERVER_INTERFACE_250, x, y, w, h, 0.0, 0.0, 250.0, 250.0);

		const float titleW = 160.f;
		const float titleH = 24.f;
		RenderImageF(IMAGE_TITTLE_SERVER_INTERFACE,
			x + ((w - titleW) * 0.5f), y + 12.f, titleW, titleH, 0.0, 0.0, 223.0, 32.0);

		g_pRenderText->SetFont(g_hFontBold);
		g_pRenderText->SetBgColor(0);
		g_pRenderText->SetTextColor(CLRDW_YELLOW);
		g_pRenderText->RenderText(
			x + ((w - titleW) * 0.5f), y + 12.f, title, (int)titleW, (int)titleH, RT3_SORT_CENTER);

		const float closeX = x + w - 22.0f;
		const float closeY = y + 8.0f;
		RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, closeX, closeY, 16.0f, 16.0f, 0.0, 0.0, 15.0, 15.0);
		if (SEASON3B::CheckMouseIn(closeX, closeY, 16.0f, 16.0f))
		{
			glColor4f(0.55f, 0.55f, 0.55f, 1.0f);
			RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, closeX, closeY, 16.0f, 16.0f, 0.0, 0.0, 15.0, 15.0);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

			if (IsRelease(VK_LBUTTON))
			{
				gInterface->Data[eWindowEventTime].OnShow = false;
				PlayBuffer(SOUND_CLICK01);
			}
		}
	}

	// Sort key: soonest upcoming first → Online next → Disabled last
	// When event goes Online (0) it sinks; next soonest stays on top.
	bool EventTimeLess(const CUSTOM_EVENTTIME_DATA& a, const CUSTOM_EVENTTIME_DATA& b)
	{
		const int ta = a.time;
		const int tb = b.time;

		auto rank = [](int t) -> int
		{
			if (t > 0) return 0;   // upcoming
			if (t == 0) return 1;  // online / happening
			return 2;              // disabled
		};

		const int ra = rank(ta);
		const int rb = rank(tb);
		if (ra != rb)
			return ra < rb;

		if (ra == 0)
			return ta < tb; // soonest first among upcoming

		// Online / disabled: keep stable by name
		return _stricmp(a.NameEvent, b.NameEvent) < 0;
	}

	void SortEventList(std::vector<CUSTOM_EVENTTIME_DATA>& list)
	{
		std::stable_sort(list.begin(), list.end(), EventTimeLess);
	}

	// Time cell color
	// Online → green | <5min → orange/urgent | <15min → yellow | else white | disabled gray-orange
	DWORD TimeColor(int t)
	{
		if (t <= -1)
			return 0xFFB145B8; // disabled
		if (t == 0)
			return 0x45FF7AB8; // happening / online
		if (t < kNear5Min)
			return 0xFF8C00B8; // < 5 min — orange
		if (t < kNear15Min)
			return 0xFFD700B8; // < 15 min — gold
		return 0xFFFFFFB8;    // normal
	}

	// Name tint follows urgency a bit
	DWORD NameColor(int t)
	{
		if (t <= -1)
			return 0xAAAAAAB8;
		if (t == 0)
			return 0x45FF7AB8;
		if (t < kNear5Min)
			return 0xFFAA66B8;
		if (t < kNear15Min)
			return 0xFFE080B8;
		return 0xFFFF478A8;
	}
}

CCustomEventTime* CCustomEventTime::Instance()
{
	static CCustomEventTime s_Instance;
	return &s_Instance;
}

CCustomEventTime::CCustomEventTime()
{
	this->Page = 0;
	this->EventTimeLoad = 0;
	this->EventTimeEnable = 0;
	this->EventTimeTickCount = 0;
	this->count = 0;
	this->MaxListData = 0;
	this->Click = false;
	this->OpenTestDelay = 0;
	this->Arena = 0;
}

void CCustomEventTime::Init()
{
	this->Click = false;
	this->mNewDataEventTime.clear();
	this->MaxListData = 0;
	this->Page = 0;
	this->EventTimeEnable = 0;
}

void CCustomEventTime::Load(CUSTOM_EVENT_INFO* info) // OK
{
	for (int n = 0; n < MAX_EVENTTIME; n++)
	{
		this->SetInfo(info[n]);
	}
}

void CCustomEventTime::SetInfo(CUSTOM_EVENT_INFO info) // OK
{
	if (info.Index < 0 || info.Index >= MAX_EVENTTIME)
	{
		return;
	}

	this->m_CustomEventInfo[info.Index] = info;
}

void CCustomEventTime::ClearCustomEventTime() // OK
{
	for (int n = 0; n < MAX_EVENTTIME; n++)
	{
		gCustomEventTime[n].time = -1;
	}
	this->count = 0;
	this->EventTimeEnable = 0;
	this->mNewDataEventTime.clear();
}

void CCustomEventTime::OpenTestWindow(int Page)
{
	gInterface->Data[eWindowEventTime].OnShow = true;

	PMSG_CUSTOM_EVENTTIME_SEND pMsg;
	pMsg.header.set(0xF3, 0xE8, sizeof(pMsg));
	pMsg.Page = 0; // always full list (GS ignores paging)
	DataSend((BYTE*)&pMsg, pMsg.header.size);
}

void CCustomEventTime::OnOffWindow() // OK
{
	gInterface->Data[eWindowEventTime].OpenClose();
	if (gInterface->Data[eWindowEventTime].OnShow)
	{
		this->Page = 0;
		g_CustomEventTime->ClearCustomEventTime();
		g_CustomEventTime->OpenTestWindow(0);
	}
}

void CCustomEventTime::GCReqEventTime(PMSG_CUSTOM_EVENTTIME_RECV* lpMsg) // OK
{
	this->count = lpMsg->count;
	this->mNewDataEventTime.clear();
	this->MaxListData = lpMsg->MaxList;

	for (int n = 0; n < lpMsg->count; n++)
	{
		CUSTOM_EVENTTIME_DATA* lpInfo = (CUSTOM_EVENTTIME_DATA*)(((BYTE*)lpMsg)
			+ sizeof(PMSG_CUSTOM_EVENTTIME_RECV)
			+ (sizeof(CUSTOM_EVENTTIME_DATA) * n));

		// Nao lista eventos desativados (sem contagem / Desligado)
		if (lpInfo->time <= -1)
			continue;

		this->mNewDataEventTime.push_back(*lpInfo);
	}

	SortEventList(this->mNewDataEventTime);

	this->EventTimeEnable = 1;
	this->EventTimeTickCount = GetTickCount();
}

void CCustomEventTime::DrawEventTimePanelWindow()
{
	if (!gInterface->Data[eWindowEventTime].OnShow)
	{
		return;
	}

	// ESC also handled in NewUIHotKey (before system menu) — keep here as backup
	if (SEASON3B::IsPress(VK_ESCAPE))
	{
		gInterface->Data[eWindowEventTime].OnShow = false;
		PlayBuffer(SOUND_CLICK01);
		return;
	}

	float MainWidth = kMainW;
	float MainHeight = kMainH;
	float StartY = ((MAX_WIN_HEIGHT - 51.f) / 2.f) - (MainHeight / 2.f);
	float StartX = (MAX_WIN_WIDTH / 2.f) - (MainWidth / 2.f);
	if (StartY < 12.f)
		StartY = 12.f;

	gInterface->Data[eWindowEventTime].X = StartX;
	gInterface->Data[eWindowEventTime].Y = StartY;
	gInterface->Data[eWindowEventTime].Width = MainWidth;
	gInterface->Data[eWindowEventTime].Height = MainHeight;

	if (SEASON3B::CheckMouseIn(StartX, StartY, MainWidth, MainHeight))
	{
		gInterface->SetBlockCur(TRUE);
		gInterface->Data[eWindowEventTime].Hover = true;
	}
	else
	{
		gInterface->Data[eWindowEventTime].Hover = false;
	}

	DrawMenuCustomFrame(StartX, StartY, MainWidth, MainHeight, "Eventos");

	const float RowCol = MainWidth / 3.f;
	float StartBody = StartY;

	if (this->EventTimeEnable != 1)
	{
		const float MainCenter = StartX + RowCol;
		const char* loadMsg = "Carregando .";
		switch (this->EventTimeLoad)
		{
		case 1: loadMsg = "Carregando .."; this->EventTimeLoad = 2; break;
		case 2: loadMsg = "Carregando ..."; this->EventTimeLoad = 3; break;
		case 3: loadMsg = "Carregando ...."; this->EventTimeLoad = 4; break;
		case 4: loadMsg = "Carregando ....."; this->EventTimeLoad = 0; break;
		default: loadMsg = "Carregando ."; this->EventTimeLoad = 1; break;
		}
		gInterface->DrawFormat(eGold, (int)(MainCenter + 10.f), (int)(StartBody + 180.f), 120, 1, loadMsg);
		return;
	}

	// Header (sem retangulo/fundo atras das colunas)
	TextDraw(g_hFontBold, (int)(StartX + 8.f), (int)(StartBody + 44.f), 0xFFFFFFA8, 0x0, (int)RowCol, 0, 3, "Evento");
	TextDraw(g_hFontBold, (int)(StartX + 8.f + RowCol), (int)(StartBody + 44.f), 0xFFFFFFA8, 0x0, (int)RowCol, 0, 3, "Local");
	TextDraw(g_hFontBold, (int)(StartX + 8.f + RowCol * 2.f), (int)(StartBody + 44.f), 0xFFFFFFA8, 0x0, (int)RowCol, 0, 3, "Tempo");

	// Countdown + re-sort every second (Online sinks, next soonest rises)
	if ((GetTickCount() - this->EventTimeTickCount) > 1000)
	{
		for (size_t i = 0; i < this->mNewDataEventTime.size(); i++)
		{
			if (this->mNewDataEventTime[i].time > 0)
				this->mNewDataEventTime[i].time -= 1;
		}
		SortEventList(this->mNewDataEventTime);
		this->EventTimeTickCount = GetTickCount();
	}

	// How many rows fit
	const float listTop = StartBody + kListTopPad;
	const float listBottom = StartY + MainHeight - kListBotPad;
	const int maxRows = (int)((listBottom - listTop) / kRowH);
	const float hitW = MainWidth - 36.f;
	const float hitX = StartX + 12.f;

	char text2[40];
	int line = 0;

	for (size_t i = 0; i < this->mNewDataEventTime.size(); i++)
	{
		if (line >= maxRows)
			break;

		const int t = this->mNewDataEventTime[i].time;

		// Segurança: ignora desativados (time <= -1 = sem contagem)
		if (t <= -1)
			continue;

		const float rowY = listTop + (float)line * kRowH;
		const DWORD colTime = TimeColor(t);
		const DWORD colName = NameColor(t);

		if (t == 0)
			wsprintf(text2, "Ao vivo");
		else
		{
			const int hours = t / 3600;
			const int minutes = (t / 60) % 60;
			const int seconds = t % 60;
			if (hours > 23)
				wsprintf(text2, "%d dia(s)+", hours / 24);
			else
				wsprintf(text2, "%02d:%02d:%02d", hours, minutes, seconds);
		}

		// Subtle hover if can move (no tip text)
		const bool canMove = (this->mNewDataEventTime[i].NumberGate > 0);
		const bool hover = canMove && SEASON3B::CheckMouseIn(hitX, rowY - 1.f, hitW, kRowH);

		if (hover)
		{
			TextDraw((HFONT)g_hFont, (int)hitX, (int)(rowY - 1.f), 0xFFFFFFFF, 0x00FBFF40, (int)hitW, (int)kRowH, 1, " ");

			if (IsRelease(VK_LBUTTON))
			{
				XULY_CGPACKET pMsg;
				memset(&pMsg, 0, sizeof(pMsg));
				pMsg.header.set(0xD3, 0x01, sizeof(pMsg));
				pMsg.ThaoTac = (DWORD)this->mNewDataEventTime[i].NumberGate;
				DataSend((LPBYTE)&pMsg, pMsg.header.size);
				PlayBuffer(SOUND_CLICK01);
				gInterface->Data[eWindowEventTime].OnShow = false;
			}
		}

		TextDraw(g_hFontBold, (int)(StartX + 10.f), (int)rowY, colName, 0x0, (int)RowCol, 0, 3,
			TEXT(this->mNewDataEventTime[i].NameEvent));
		TextDraw(g_hFontBold, (int)(StartX + 10.f + RowCol), (int)rowY, 0x61FFD0A8, 0x0, (int)RowCol, 0, 3,
			TEXT(this->mNewDataEventTime[i].DesString));
		TextDraw(g_hFontBold, (int)(StartX + 10.f + RowCol * 2.f), (int)rowY, colTime, 0x0, (int)RowCol, 0, 3,
			TEXT(text2));

		line++;
	}
}
