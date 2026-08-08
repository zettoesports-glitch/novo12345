// NewUIOptionWindow.cpp: implementation of the CNewUIOptionWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NewUIOptionWindow.h"
#include "NewUISystem.h"
#include "ZzzTexture.h"
#include "DSPlaySound.h"
#include "Input.h"
#include "ZzzOpenData.h"
#include "ZzzInterface.h"
#include "ZzzOpenglUtil.h"
#include "pugixml.hpp"
#include "./ExternalObject/leaf/regkey.h"
#include "TextClien.h"
#include "Time/CFPSController.h"
#include "Combat/CAutoCombat.h"
#include "CGMProtect.h"
#include "Winmain.h"
#include "ZzzScene.h"

extern "C" void wzAudioSetVolume(int numVolume);
extern "C" void wzAudioStop(void);
using namespace SEASON3B;

extern int targetIndexFPS;

// =============================================================================
// Two-column layout — wide X, short Y
// LEFT:  DISPLAY + GRAPHICS (+ Effect limitation) + AUDIO (volume)
// RIGHT: GAME + PERFORMANCE + COMBAT
// =============================================================================
namespace OptLayout
{
	const float WIN_W = 420.f;
	const float WIN_H = 310.f;
	const float TITLE_Y = 8.f;

	const float ROW_H = 15.f;
	const float CHECK_SZ = 15.f;

	// --- LEFT ---
	const float L_X = 16.f;
	const float L_COL_W = 190.f;

	const float DISP_HDR_Y = 26.f;
	const float DISP_RES_Y = 44.f;
	const float DISP_FONT_Y = 68.f;
	const float DROP_BOX_X = 90.f;
	const float DROP_W = 112.f;
	const float DROP_H = 18.f;
	const float DROP_ROW_H = 18.f;
	const int   DROP_MAX_VISIBLE = 8;
	// Modal list panel (centered over options window)
	const float DROP_MODAL_W = 210.f;
	const float DROP_MODAL_TITLE_H = 24.f;
	const float DROP_MODAL_PAD = 10.f;

	const float GFX_HDR_Y = 100.f;
	const float GFX_ROW_Y = 116.f;
	const float GFX_LOW_X = 16.f;
	const float GFX_LOW_TX = 34.f;
	const float GFX_MED_X = 78.f;
	const float GFX_MED_TX = 96.f;
	const float GFX_HIGH_X = 140.f;
	const float GFX_HIGH_TX = 158.f;

	// Effect limitation under GRAPHICS
	const float FX_LABEL_Y = 142.f;
	const float FX_BAR_Y = 158.f;
	const float FX_BAR_X = 16.f;
	const float FX_BAR_W = 188.f;
	const float FX_BAR_H = 28.f;

	// AUDIO at end of col1 (after effect bar)
	const float AUDIO_HDR_Y = 198.f;
	const float MUSIC_LABEL_Y = 214.f;
	const float MUSIC_BAR_Y = 230.f;
	const float SOUND_LABEL_Y = 250.f;
	const float SOUND_BAR_Y = 266.f;
	const float VOL_BAR_X = 16.f;
	const float VOL_BAR_W = 188.f;

	// --- RIGHT ---
	const float R_X = 220.f;
	const float R_COL_W = 185.f;
	const float R_CHECK = 386.f;
	const float R_HIT_W = 180.f;

	const float GAME_HDR_Y = 26.f;
	const float GAME_ROW0 = 42.f;
	const float GAME_ROW1 = 58.f;
	const float GAME_ROW2 = 74.f;
	const float GAME_ROW3 = 90.f;
	const float GAME_ROW4 = 106.f;
	const float GAME_ROW5 = 122.f;
	const float GAME_ROW6 = 138.f;
	const float GAME_ROW7 = 154.f; // Modo Foto
	const float GAME_ROW8 = 170.f; // Show player names

	const float PERF_HDR_Y = 190.f;
	const float PERF_FPS_Y = 206.f;
	const float PERF_VSYNC_Y = 222.f;
	// FPS dropdown box (right column) — same style as Window/Font
	const float FPS_DROP_X = 268.f;
	const float FPS_DROP_W = 115.f;

	const float COMBAT_HDR_Y = 242.f;
	const float COMBAT_HP_Y = 258.f;
	const float COMBAT_PVP_Y = 274.f;
}

// Section title centered inside [x, x+boxW]
static void RenderSectionHeader(float x, float y, float boxW, const char* title)
{
	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetTextColor(CLRDW_GOLD);
	g_pRenderText->SetBgColor(0, 0, 0, 0);
	// RT3_SORT_CENTER: text centered in the given box width
	g_pRenderText->RenderText((int)x, (int)y, title, (int)boxW, 0, RT3_SORT_CENTER);
	g_pRenderText->SetFont(g_hFont);
}

static void RenderSectionLine(float x, float y, float w)
{
	// Alpha 0 keeps glColor (gold); Flag 0 + Alpha>0 would force white
	glColor4f(0.72f, 0.58f, 0.22f, 1.f);
	RenderColor(x, y, w, 1.2f, 0.f, 0);
	EndRenderColor();
	glColor4f(1.f, 1.f, 1.f, 1.f);
}

// Label + checkbox on the SAME row (same Y). Text box height = CHECK_SZ.
static void RenderRowLabel(float x, float y, const char* text)
{
	g_pRenderText->SetFont(g_hFont);
	g_pRenderText->SetBgColor(0); // transparent text plate
	g_pRenderText->SetTextColor(230, 230, 230, 255);
	// width=0 auto, height=15 → same vertical band as 15x15 checkbox
	g_pRenderText->RenderText((int)x, (int)y, text, 0, (int)OptLayout::ROW_H);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SEASON3B::CNewUIOptionWindow::CNewUIOptionWindow()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;

	m_bAutoAttack = true;
	m_bWhisperSound = false;
	m_bSlideHelp = true;
	m_iVolumeLevel = 0;
	m_iRenderLevel = 4;

	m_RenderEffect = true;
	m_RenderEquipment = true;
	m_RenderTerrain = true;
	m_RenderObjects = true;
	m_bPhotoMode = false;
	m_bShowPlayerName = false; // names off by default
	m_iGraphicQuality = 2; // High default
	m_Resolution = 0;
	m_iFpsPreset = 2; // 120 default
	m_bVSync = false;
	m_iSoundVolume = m_iVolumeLevel;
	m_iMusicVolume = m_iVolumeLevel;

	m_iSoundVolume = GetPrivateProfileIntA("Audio", "SoundVolume", m_iVolumeLevel, ".\\config.ini");
	m_iMusicVolume = GetPrivateProfileIntA("Audio", "MusicVolume", m_iVolumeLevel, ".\\config.ini");
	if (m_iSoundVolume < 0) m_iSoundVolume = 0;
	if (m_iSoundVolume > 10) m_iSoundVolume = 10;
	if (m_iMusicVolume < 0) m_iMusicVolume = 0;
	if (m_iMusicVolume > 10) m_iMusicVolume = 10;

	m_iResIndex = 0;
	m_iFontIndex = 0;
	m_bDropRes = false;
	m_bDropFont = false;
	m_bDropFps = false;
	m_iDropScrollRes = 0;
	m_iDropScrollFont = 0;
	m_iDropScrollFps = 0;
	m_fpsOptions.clear();
	m_fpsOptions.push_back("30");
	m_fpsOptions.push_back("60");
	m_fpsOptions.push_back("120");
	m_fpsOptions.push_back("MAX");

	LoadGraphicQualityIni();
	// Only load values here — Apply runs in OpenningProcess / after GL+VSync init
	{
		int preset = GetPrivateProfileIntA("Performance", "FpsPreset", 2, ".\\config.ini");
		int vsync = GetPrivateProfileIntA("Performance", "VSync", 0, ".\\config.ini");
		if (preset < 0 || preset > 3) preset = 2;
		m_iFpsPreset = preset;
		m_bVSync = (vsync != 0);
		m_bShowPlayerName = GetPrivateProfileIntA("Game", "ShowPlayerName", 0, ".\\config.ini") != 0;
		extern bool g_bGMObservation;
		g_bGMObservation = m_bShowPlayerName;
	}
}

SEASON3B::CNewUIOptionWindow::~CNewUIOptionWindow()
{
	Release();
}

bool SEASON3B::CNewUIOptionWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
	if (NULL == pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_OPTION, this);
	RegisterPhotoModeKeepUI(this);

	SetPos(x, y);

	LoadImages();

	SetButtonInfo();

	Show(false);

	return true;
}

void SEASON3B::CNewUIOptionWindow::SetPhotoMode(bool bOn)
{
	m_bPhotoMode = bOn;
	SetPhotoModeHideHud(bOn);

	// Entering photo mode: close Options so the screen is clean for screenshots
	if (bOn && g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_OPTION))
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_OPTION);

	if (g_pChatListBox)
	{
		if (bOn)
			g_pChatListBox->AddText("", "Modo Foto ON — ESC para sair", SEASON3B::TYPE_SYSTEM_MESSAGE);
		else
			g_pChatListBox->AddText("", "Modo Foto OFF", SEASON3B::TYPE_SYSTEM_MESSAGE);
	}
}

void SEASON3B::CNewUIOptionWindow::SetShowPlayerName(bool bOn)
{
	m_bShowPlayerName = bOn;
	extern bool g_bGMObservation;
	g_bGMObservation = bOn; // keep legacy name gate in sync
	WritePrivateProfileStringA("Game", "ShowPlayerName", bOn ? "1" : "0", ".\\config.ini");
}

void SEASON3B::CNewUIOptionWindow::SetButtonInfo()
{
	LoadResolution("Data\\Resolutions.xml");
}

void SEASON3B::CNewUIOptionWindow::CloseAllDropdowns()
{
	m_bDropRes = false;
	m_bDropFont = false;
	m_bDropFps = false;
	m_iDropScrollRes = 0;
	m_iDropScrollFont = 0;
	m_iDropScrollFps = 0;
}

void SEASON3B::CNewUIOptionWindow::Release()
{
	UnloadImages();

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUIOptionWindow::SetPos(int x, int y)
{
	using namespace OptLayout;
	// Center wide panel on 640 design width: (640 - 420) / 2 = 110
	m_Pos.x = pos_center((640.f - WIN_W) * 0.5f);
	m_Pos.y = PositionY_In_The_Mid(0) + y;
	(void)x;
}

void SEASON3B::CNewUIOptionWindow::RenderDropdownBox(float x, float y, float w, float h, const char* text, bool open)
{
	// Same look for Janela / Fonte / FPS.
	// Alpha 0 keeps glColor (soft dark fill). Flag 1 pure black was harsh behind short numbers like "13".
	EnableAlphaTest();
	glColor4f(0.10f, 0.10f, 0.12f, 0.90f);
	RenderColor(x, y, w, h, 0.f, 0);
	EndRenderColor();

	// Gold border
	glColor4f(0.72f, 0.58f, 0.22f, 1.f);
	RenderColor(x, y, w, 1.f, 0.f, 0);
	RenderColor(x, y + h - 1.f, w, 1.f, 0.f, 0);
	RenderColor(x, y, 1.f, h, 0.f, 0);
	RenderColor(x + w - 1.f, y, 1.f, h, 0.f, 0);
	EndRenderColor();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	// Label (leave room on the right for login-style arrow 15x13)
	const float arrowW = 15.f;
	const float arrowH = 13.f;
	const float arrowX = x + w - arrowW - 3.f;
	const float arrowY = y + (h - arrowH) * 0.5f;

	// Transparent text plate (0,0,0,0) — SetBgColor(0) alone can still leave a black strip
	// on some font sizes when width>0; use width=0 auto size for all three dropdowns.
	g_pRenderText->SetFont(g_hFont);
	g_pRenderText->SetBgColor(0, 0, 0, 0);
	g_pRenderText->SetTextColor(235, 235, 235, 255);
	g_pRenderText->RenderText((int)(x + 6), (int)(y + 3), text ? text : "-", 0, 0, RT3_SORT_LEFT);

	// Same images as login account expand (ctskill up/down)
	RenderImage(open ? IMAGE_OPTION_ARROW_UP : IMAGE_OPTION_ARROW_DOWN,
		arrowX, arrowY, arrowW, arrowH);
}

void SEASON3B::CNewUIOptionWindow::RenderDropdownModal(int kind)
{
	using namespace OptLayout;
	const float bx = m_Pos.x;
	const float by = m_Pos.y;

	const std::vector<std::string>* pItems = NULL;
	int* pScroll = NULL;
	int curSel = 0;
	const char* title = "";
	if (kind == 0) { pItems = &m_resOptions; pScroll = &m_iDropScrollRes; curSel = m_iResIndex; title = "Janela"; }
	else if (kind == 1) { pItems = &m_fontOptions; pScroll = &m_iDropScrollFont; curSel = m_iFontIndex; title = "Fonte"; }
	else { pItems = &m_fpsOptions; pScroll = &m_iDropScrollFps; curSel = m_iFpsPreset; title = "FPS"; }

	if (!pItems || pItems->empty())
		return;

	const std::vector<std::string>& items = *pItems;
	const int count = (int)items.size();
	int& scroll = *pScroll;
	const int visible = (count < DROP_MAX_VISIBLE) ? count : DROP_MAX_VISIBLE;
	if (scroll < 0) scroll = 0;
	if (scroll > count - visible) scroll = max(0, count - visible);

	// RenderColor(Alpha>0, Flag): 0=WHITE, 1=BLACK — must use Flag=1 for dark UI
	// Alpha==0 keeps the current glColor (for gold borders / highlights)

	EnableAlphaTest();

	// 1) Dark veil over options window
	RenderColor(bx, by, WIN_W, WIN_H, 0.82f, 1);
	EndRenderColor();

	// 2) Centered solid black panel
	const float panelW = DROP_MODAL_W;
	const float panelH = DROP_MODAL_TITLE_H + DROP_MODAL_PAD + visible * DROP_ROW_H + DROP_MODAL_PAD;
	const float px = bx + (WIN_W - panelW) * 0.5f;
	const float py = by + (WIN_H - panelH) * 0.5f;

	RenderColor(px, py, panelW, panelH, 0.96f, 1);
	EndRenderColor();

	// Gold border (Alpha 0 → use glColor)
	glColor4f(0.85f, 0.70f, 0.25f, 1.f);
	RenderColor(px, py, panelW, 1.5f, 0.f, 0);
	RenderColor(px, py + panelH - 1.5f, panelW, 1.5f, 0.f, 0);
	RenderColor(px, py, 1.5f, panelH, 0.f, 0);
	RenderColor(px + panelW - 1.5f, py, 1.5f, panelH, 0.f, 0);
	EndRenderColor();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	// Title
	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0, 0, 0, 0);
	g_pRenderText->SetTextColor(255, 220, 80, 255);
	g_pRenderText->RenderText((int)px, (int)(py + 5.f), title, (int)panelW, 0, RT3_SORT_CENTER);
	g_pRenderText->SetFont(g_hFont);

	// Rows
	const float listY = py + DROP_MODAL_TITLE_H;
	for (int i = 0; i < visible; ++i)
	{
		const int idx = scroll + i;
		if (idx >= count) break;
		const float ry = listY + i * DROP_ROW_H;
		const bool sel = (idx == curSel);
		const bool hover = CheckMouseIn(px + 4.f, ry, panelW - 8.f, DROP_ROW_H);

		if (sel || hover)
		{
			// Selected / hover strip (dark gold via glColor + Alpha 0)
			if (sel)
				glColor4f(0.45f, 0.35f, 0.12f, 1.f);
			else
				glColor4f(0.22f, 0.20f, 0.14f, 1.f);
			RenderColor(px + 4.f, ry, panelW - 8.f, DROP_ROW_H - 1.f, 0.f, 0);
			EndRenderColor();
			glColor4f(1.f, 1.f, 1.f, 1.f);
		}

		// High-contrast text — no black plate behind numbers (Fonte 11/12/13…)
		g_pRenderText->SetBgColor(0, 0, 0, 0);
		if (sel)
			g_pRenderText->SetTextColor(255, 230, 120, 255);
		else
			g_pRenderText->SetTextColor(240, 240, 240, 255);
		g_pRenderText->RenderText((int)(px + 12.f), (int)(ry + 3.f), items[idx].c_str(),
			0, 0, RT3_SORT_LEFT);
	}
}

bool SEASON3B::CNewUIOptionWindow::UpdateDropdownModal(int kind)
{
	using namespace OptLayout;
	const float bx = m_Pos.x;
	const float by = m_Pos.y;

	const std::vector<std::string>* pItems = NULL;
	int* pScroll = NULL;
	if (kind == 0) { pItems = &m_resOptions; pScroll = &m_iDropScrollRes; }
	else if (kind == 1) { pItems = &m_fontOptions; pScroll = &m_iDropScrollFont; }
	else { pItems = &m_fpsOptions; pScroll = &m_iDropScrollFps; }

	if (!pItems || pItems->empty())
	{
		CloseAllDropdowns();
		return true;
	}

	const std::vector<std::string>& items = *pItems;
	const int count = (int)items.size();
	int& scroll = *pScroll;
	const int visible = (count < DROP_MAX_VISIBLE) ? count : DROP_MAX_VISIBLE;
	if (scroll < 0) scroll = 0;
	if (scroll > count - visible) scroll = max(0, count - visible);

	const float panelW = DROP_MODAL_W;
	const float panelH = DROP_MODAL_TITLE_H + DROP_MODAL_PAD + visible * DROP_ROW_H + DROP_MODAL_PAD;
	const float px = bx + (WIN_W - panelW) * 0.5f;
	const float py = by + (WIN_H - panelH) * 0.5f;
	const float listY = py + DROP_MODAL_TITLE_H;

	// Scroll while over panel
	if (CheckMouseIn(px, py, panelW, panelH))
	{
		if (MouseWheel > 0) { MouseWheel = 0; if (scroll > 0) scroll--; }
		else if (MouseWheel < 0) { MouseWheel = 0; if (scroll < count - visible) scroll++; }
	}

	if (SEASON3B::IsPress(VK_LBUTTON))
	{
		bool hitItem = false;
		for (int i = 0; i < visible; ++i)
		{
			const int idx = scroll + i;
			if (idx >= count) break;
			if (CheckMouseIn(px + 4.f, listY + i * DROP_ROW_H, panelW - 8.f, DROP_ROW_H))
			{
				if (kind == 0)
				{
					m_iResIndex = idx;
					change_resolution();
				}
				else if (kind == 1)
				{
					m_iFontIndex = idx;
					change_fontsize();
				}
				else
				{
					if (idx < 0) m_iFpsPreset = 0;
					else if (idx > 3) m_iFpsPreset = 3;
					else m_iFpsPreset = idx;
					ApplyPerformanceSettings();
					SavePerformanceSettings();
				}
				CloseAllDropdowns();
				PlayBuffer(SOUND_CLICK01);
				hitItem = true;
				break;
			}
		}
		// Click outside the list (on black veil) → cancel
		if (!hitItem && !CheckMouseIn(px, py, panelW, panelH))
		{
			CloseAllDropdowns();
			PlayBuffer(SOUND_CLICK01);
		}
	}

	// Consume all clicks while modal is open
	return true;
}

bool SEASON3B::CNewUIOptionWindow::UpdateMouseEvent()
{
	using namespace OptLayout;
	const float bx = m_Pos.x;
	const float by = m_Pos.y;

	// Modal open: only pick from list / click black veil to close
	if (m_bDropRes)
	{
		UpdateDropdownModal(0);
		return !CheckMouseIn(bx, by, WIN_W, WIN_H);
	}
	if (m_bDropFont)
	{
		UpdateDropdownModal(1);
		return !CheckMouseIn(bx, by, WIN_W, WIN_H);
	}
	if (m_bDropFps)
	{
		UpdateDropdownModal(2);
		return !CheckMouseIn(bx, by, WIN_W, WIN_H);
	}

	// Right column: label row OR checkbox (same Y as Automatic Attack pattern)
	auto hitR = [&](float rowY) -> bool {
		return SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(bx + R_X, by + rowY, R_HIT_W, ROW_H);
	};
	auto hitRBox = [&](float rowY) -> bool {
		return SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(bx + R_CHECK, by + rowY, CHECK_SZ, CHECK_SZ);
	};

	// RIGHT: GAME
	if (hitR(GAME_ROW0) || hitRBox(GAME_ROW0)) m_bAutoAttack = !m_bAutoAttack;
	if (hitR(GAME_ROW1) || hitRBox(GAME_ROW1)) m_bWhisperSound = !m_bWhisperSound;
	if (hitR(GAME_ROW2) || hitRBox(GAME_ROW2)) m_bSlideHelp = !m_bSlideHelp;
	if (hitR(GAME_ROW3) || hitRBox(GAME_ROW3)) { m_RenderEffect = !m_RenderEffect; SyncGraphicQualityFromFlags(); }
	if (hitR(GAME_ROW4) || hitRBox(GAME_ROW4)) { m_RenderEquipment = !m_RenderEquipment; SyncGraphicQualityFromFlags(); }
	if (hitR(GAME_ROW5) || hitRBox(GAME_ROW5)) { m_RenderTerrain = !m_RenderTerrain; SyncGraphicQualityFromFlags(); }
	if (hitR(GAME_ROW6) || hitRBox(GAME_ROW6)) { m_RenderObjects = !m_RenderObjects; SyncGraphicQualityFromFlags(); }
	if (hitR(GAME_ROW7) || hitRBox(GAME_ROW7))
	{
		SetPhotoMode(!m_bPhotoMode);
		PlayBuffer(SOUND_CLICK01);
	}
	if (hitR(GAME_ROW8) || hitRBox(GAME_ROW8))
	{
		SetShowPlayerName(!m_bShowPlayerName);
		PlayBuffer(SOUND_CLICK01);
	}

	// RIGHT: PERFORMANCE / COMBAT (same X pattern as GAME)
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(bx + FPS_DROP_X, by + PERF_FPS_Y, FPS_DROP_W, DROP_H))
	{
		m_bDropFps = !m_bDropFps;
		m_bDropRes = false;
		m_bDropFont = false;
		PlayBuffer(SOUND_CLICK01);
	}
	if (hitR(PERF_VSYNC_Y) || hitRBox(PERF_VSYNC_Y))
	{ ToggleVSyncOption(); PlayBuffer(SOUND_CLICK01); }
	if (hitR(COMBAT_HP_Y) || hitRBox(COMBAT_HP_Y))
	{ gAutoCombat.ToggleAutoHP(); PlayBuffer(SOUND_CLICK01); }
	if (hitR(COMBAT_PVP_Y) || hitRBox(COMBAT_PVP_Y))
	{ gAutoCombat.ToggleAutoPvP(); PlayBuffer(SOUND_CLICK01); }

	// LEFT: DISPLAY dropdowns
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(bx + DROP_BOX_X, by + DISP_RES_Y, DROP_W, DROP_H))
	{
		m_bDropRes = !m_bDropRes;
		m_bDropFont = false;
		m_bDropFps = false;
		PlayBuffer(SOUND_CLICK01);
	}
	else if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(bx + DROP_BOX_X, by + DISP_FONT_Y, DROP_W, DROP_H))
	{
		m_bDropFont = !m_bDropFont;
		m_bDropRes = false;
		m_bDropFps = false;
		PlayBuffer(SOUND_CLICK01);
	}

	// LEFT: GRAPHICS
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(bx + GFX_LOW_X, by + GFX_ROW_Y, 55, ROW_H))
	{ ApplyGraphicQuality(0); SaveGraphicQualityIni(); PlayBuffer(SOUND_CLICK01); }
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(bx + GFX_MED_X, by + GFX_ROW_Y, 55, ROW_H))
	{ ApplyGraphicQuality(1); SaveGraphicQualityIni(); PlayBuffer(SOUND_CLICK01); }
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(bx + GFX_HIGH_X, by + GFX_ROW_Y, 55, ROW_H))
	{ ApplyGraphicQuality(2); SaveGraphicQualityIni(); PlayBuffer(SOUND_CLICK01); }

	// LEFT: +Effect limitation (under GRAPHICS)
	if (CheckMouseIn(bx + FX_BAR_X, by + FX_BAR_Y, FX_BAR_W, FX_BAR_H))
	{
		if (SEASON3B::IsRepeat(VK_LBUTTON))
		{
			int x = MouseX - (int)(bx + FX_BAR_X);
			float fValue = (5.f * x) / FX_BAR_W;
			m_iRenderLevel = (int)fValue;
			if (m_iRenderLevel < 0) m_iRenderLevel = 0;
			if (m_iRenderLevel > 4) m_iRenderLevel = 4;
			SyncGraphicQualityFromFlags();
		}
	}

	// Musica volume bar (wzAudio 0-100)
	if (CheckMouseIn(bx + VOL_BAR_X - 8, by + MUSIC_BAR_Y, VOL_BAR_W + 8, 16))
	{
		int iOldValue = m_iMusicVolume;
		if (MouseWheel > 0) { MouseWheel = 0; if (m_iMusicVolume < 10) m_iMusicVolume++; }
		else if (MouseWheel < 0) { MouseWheel = 0; if (m_iMusicVolume > 0) m_iMusicVolume--; }
		if (SEASON3B::IsRepeat(VK_LBUTTON))
		{
			int x = MouseX - (int)(bx + VOL_BAR_X);
			if (x < 0) m_iMusicVolume = 0;
			else {
				float fValue = (10.f * x) / VOL_BAR_W;
				m_iMusicVolume = (int)fValue;
				if (m_iMusicVolume > 10) m_iMusicVolume = 10;
				if (m_iMusicVolume < 0) m_iMusicVolume = 0;
			}
		}
		if (iOldValue != m_iMusicVolume)
		{
			m_MusicOnOff = (m_iMusicVolume > 0) ? 1 : 0;
			if (m_iMusicVolume == 0)
			{
				wzAudioStop();
			}
			else
			{
				wzAudioSetVolume(m_iMusicVolume * 10);
			}
		}
	}

	// Som volume bar (DirectSound effects 0-9)
	if (CheckMouseIn(bx + VOL_BAR_X - 8, by + SOUND_BAR_Y, VOL_BAR_W + 8, 16))
	{
		int iOldValue = m_iSoundVolume;
		if (MouseWheel > 0) { MouseWheel = 0; if (m_iSoundVolume < 10) m_iSoundVolume++; }
		else if (MouseWheel < 0) { MouseWheel = 0; if (m_iSoundVolume > 0) m_iSoundVolume--; }
		if (SEASON3B::IsRepeat(VK_LBUTTON))
		{
			int x = MouseX - (int)(bx + VOL_BAR_X);
			if (x < 0) m_iSoundVolume = 0;
			else {
				float fValue = (10.f * x) / VOL_BAR_W;
				m_iSoundVolume = (int)fValue;
				if (m_iSoundVolume > 10) m_iSoundVolume = 10;
				if (m_iSoundVolume < 0) m_iSoundVolume = 0;
			}
		}
		if (iOldValue != m_iSoundVolume)
			SetEffectVolumeLevel(m_iSoundVolume);
	}

	return !CheckMouseIn(bx, by, WIN_W, WIN_H);
}

bool SEASON3B::CNewUIOptionWindow::UpdateKeyEvent()
{
	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_OPTION) == true)
	{
		if (SEASON3B::IsPress(VK_ESCAPE) == true)
		{
			// Only close Options here. Photo mode is exited by ESC when Options is already closed (HotKey).
			g_pNewUISystem->Hide(SEASON3B::INTERFACE_OPTION);
			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}

	return true;
}

bool SEASON3B::CNewUIOptionWindow::Update()
{
	return true;
}

bool SEASON3B::CNewUIOptionWindow::Render()
{
	EnableAlphaTest();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	RenderFrame();

	RenderContents();

	RenderButtons();

	DisableAlphaBlend();

	return true;
}

float SEASON3B::CNewUIOptionWindow::GetLayerDepth()	//. 10.5f
{
	return 10.5f;
}

float SEASON3B::CNewUIOptionWindow::GetKeyEventOrder()	// 10.f;
{
	return 10.0f;
}

void SEASON3B::CNewUIOptionWindow::OpenningProcess()
{
	CloseAllDropdowns();
	LoadPerformanceSettings();
}

void SEASON3B::CNewUIOptionWindow::ClosingProcess()
{
	SaveGraphicQualityIni();
	SavePerformanceSettings();
	SaveOptions();

	char buf[16];
	WritePrivateProfileStringA("Audio", "SoundVolume", itoa(m_iSoundVolume, buf, 10), ".\\config.ini");
	WritePrivateProfileStringA("Audio", "MusicVolume", itoa(m_iMusicVolume, buf, 10), ".\\config.ini");
}

void SEASON3B::CNewUIOptionWindow::SetGameOptions(BYTE GameOption)
{
	if ((GameOption & AUTOATTACK_ON) == AUTOATTACK_ON)
	{
		this->SetAutoAttack(true);
	}
	else
	{
		this->SetAutoAttack(false);
	}

	if ((GameOption & WHISPER_SOUND_ON) == WHISPER_SOUND_ON)
	{
		this->SetWhisperSound(true);
	}
	else
	{
		this->SetWhisperSound(false);
	}

	if ((GameOption & SLIDE_HELP_ON) == SLIDE_HELP_ON)
	{
		this->SetSlideHelp(true);
	}
	else
	{
		this->SetSlideHelp(false);
	}

	if ((GameOption & RENDER_EFFECT_ON) == RENDER_EFFECT_ON)
	{
		this->SetRenderEffect(true);
	}
	else
	{
		this->SetRenderEffect(false);
	}

	if ((GameOption & RENDER_EQUIPMENT_ON) == RENDER_EQUIPMENT_ON)
	{
		this->SetRenderEquipment(true);
	}
	else
	{
		this->SetRenderEquipment(false);
	}

	if ((GameOption & RENDER_TERRAIN_ON) == RENDER_TERRAIN_ON)
	{
		this->SetRenderTerrain(true);
	}
	else
	{
		this->SetRenderTerrain(false);
	}

	if ((GameOption & RENDER_OBJECTS_ON) == RENDER_OBJECTS_ON)
	{
		this->SetRenderObjects(true);
	}
	else
	{
		this->SetRenderObjects(false);
	}

	// Alinha radio Low/Med/High com flags vindas do server/save
	SyncGraphicQualityFromFlags();
}

void SEASON3B::CNewUIOptionWindow::LoadImages()
{
	LoadBitmap("Interface\\newui_button_close.tga", IMAGE_OPTION_BTN_CLOSE, GL_LINEAR);
	LoadBitmap("Interface\\newui_msgbox_back.jpg", IMAGE_OPTION_FRAME_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back03.tga", IMAGE_OPTION_FRAME_DOWN, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_top.tga", IMAGE_OPTION_FRAME_UP, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_back06(L).tga", IMAGE_OPTION_FRAME_LEFT, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_back06(R).tga", IMAGE_OPTION_FRAME_RIGHT, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_line.jpg", IMAGE_OPTION_LINE, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_point.tga", IMAGE_OPTION_POINT, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_check.tga", IMAGE_OPTION_BTN_CHECK, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_effect03.tga", IMAGE_OPTION_EFFECT_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_effect04.tga", IMAGE_OPTION_EFFECT_COLOR, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_volume01.tga", IMAGE_OPTION_VOLUME_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_volume02.tga", IMAGE_OPTION_VOLUME_COLOR, GL_LINEAR);

	LoadBitmap("Interface\\HUD\\checkbox.tga", IMAGE_CHECK_LIVE);
	LoadBitmap("Interface\\HUD\\uncheckbox.tga", IMAGE_UNCHECK_LIVE);
	// Login account list arrows
	LoadBitmap("Interface\\newui_ctskillup.jpg", IMAGE_OPTION_ARROW_UP, GL_LINEAR);
	LoadBitmap("Interface\\newui_ctskilldown.jpg", IMAGE_OPTION_ARROW_DOWN, GL_LINEAR);
}

void SEASON3B::CNewUIOptionWindow::UnloadImages()
{
	DeleteBitmap(IMAGE_OPTION_BTN_CLOSE);
	DeleteBitmap(IMAGE_OPTION_FRAME_BACK);
	DeleteBitmap(IMAGE_OPTION_FRAME_DOWN);
	DeleteBitmap(IMAGE_OPTION_FRAME_UP);
	DeleteBitmap(IMAGE_OPTION_FRAME_LEFT);
	DeleteBitmap(IMAGE_OPTION_FRAME_RIGHT);
	DeleteBitmap(IMAGE_OPTION_LINE);
	DeleteBitmap(IMAGE_OPTION_POINT);
	DeleteBitmap(IMAGE_OPTION_BTN_CHECK);
	DeleteBitmap(IMAGE_OPTION_EFFECT_BACK);
	DeleteBitmap(IMAGE_OPTION_EFFECT_COLOR);
	DeleteBitmap(IMAGE_OPTION_VOLUME_BACK);
	DeleteBitmap(IMAGE_OPTION_VOLUME_COLOR);
	DeleteBitmap(IMAGE_OPTION_ARROW_UP);
	DeleteBitmap(IMAGE_OPTION_ARROW_DOWN);
}

void SEASON3B::CNewUIOptionWindow::RenderFrame()
{
	using namespace OptLayout;
	window_backmsg(m_Pos.x, m_Pos.y, WIN_W, WIN_H);
}

void SEASON3B::CNewUIOptionWindow::RenderContents()
{
	using namespace OptLayout;
	const float bx = m_Pos.x;
	const float by = m_Pos.y;

	g_pRenderText->SetBgColor(0);

	// Title — centered across full panel width; mouse XY at end (layout helper)
	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetTextColor(CLRDW_GOLD);
	{
		char titleBuf[128];
		const char* baseTitle = GlobalText[3450] ? GlobalText[3450] : "Option";
		sprintf_s(titleBuf, "%s  X:%d Y:%d", baseTitle, (int)MouseX, (int)MouseY);
		g_pRenderText->RenderText((int)bx, (int)(by + TITLE_Y), titleBuf, (int)WIN_W, 0, RT3_SORT_CENTER);
	}

	// ========== LEFT ==========
	// Headers centered in left column (same column as Window/Font/Low content area)
	RenderSectionHeader(bx + L_X, by + DISP_HDR_Y, L_COL_W, "TELA");
	RenderSectionLine(bx + L_X, by + DISP_HDR_Y + 12.f, L_COL_W);
	RenderRowLabel(bx + L_X, by + DISP_RES_Y, "Janela");
	RenderRowLabel(bx + L_X, by + DISP_FONT_Y, "Fonte");

	const char* resText = (!m_resOptions.empty() && m_iResIndex >= 0 && m_iResIndex < (int)m_resOptions.size())
		? m_resOptions[m_iResIndex].c_str() : "-";
	const char* fontText = (!m_fontOptions.empty() && m_iFontIndex >= 0 && m_iFontIndex < (int)m_fontOptions.size())
		? m_fontOptions[m_iFontIndex].c_str() : "-";
	RenderDropdownBox(bx + DROP_BOX_X, by + DISP_RES_Y, DROP_W, DROP_H, resText, m_bDropRes);
	RenderDropdownBox(bx + DROP_BOX_X, by + DISP_FONT_Y, DROP_W, DROP_H, fontText, m_bDropFont);

	RenderSectionHeader(bx + L_X, by + GFX_HDR_Y, L_COL_W, "GRAFICOS");
	RenderSectionLine(bx + L_X, by + GFX_HDR_Y + 12.f, L_COL_W);
	RenderRowLabel(bx + GFX_LOW_TX, by + GFX_ROW_Y, "Baixo");
	RenderRowLabel(bx + GFX_MED_TX, by + GFX_ROW_Y, "Medio");
	RenderRowLabel(bx + GFX_HIGH_TX, by + GFX_ROW_Y, "Alto");
	// Effect limitation under Low/Med/High
	RenderRowLabel(bx + L_X, by + FX_LABEL_Y, GlobalText[1840]);

	// AUDIO at end of col1
	RenderSectionHeader(bx + L_X, by + AUDIO_HDR_Y, L_COL_W, "AUDIO");
	RenderSectionLine(bx + L_X, by + AUDIO_HDR_Y + 12.f, L_COL_W);
	RenderRowLabel(bx + L_X, by + MUSIC_LABEL_Y, "Musica");
	RenderRowLabel(bx + L_X, by + SOUND_LABEL_Y, "Som");

	// ========== RIGHT ==========
	RenderSectionHeader(bx + R_X, by + GAME_HDR_Y, R_COL_W, "JOGO");
	RenderSectionLine(bx + R_X, by + GAME_HDR_Y + 12.f, R_COL_W);
	RenderRowLabel(bx + R_X, by + GAME_ROW0, GlobalText[386]);
	RenderRowLabel(bx + R_X, by + GAME_ROW1, GlobalText[387]);
	RenderRowLabel(bx + R_X, by + GAME_ROW2, GlobalText[919]);
	RenderRowLabel(bx + R_X, by + GAME_ROW3, gTextClien.TextClien_Khac[6]);
	RenderRowLabel(bx + R_X, by + GAME_ROW4, gTextClien.TextClien_Khac[7]);
	RenderRowLabel(bx + R_X, by + GAME_ROW5, gTextClien.TextClien_Khac[8]);
	RenderRowLabel(bx + R_X, by + GAME_ROW6, gTextClien.TextClien_Khac[9]);
	RenderRowLabel(bx + R_X, by + GAME_ROW7, "Modo Foto");
	RenderRowLabel(bx + R_X, by + GAME_ROW8, "Nome");

	RenderSectionHeader(bx + R_X, by + PERF_HDR_Y, R_COL_W, "DESEMPENHO");
	RenderSectionLine(bx + R_X, by + PERF_HDR_Y + 12.f, R_COL_W);
	RenderRowLabel(bx + R_X, by + PERF_FPS_Y, "FPS");
	RenderDropdownBox(bx + FPS_DROP_X, by + PERF_FPS_Y, FPS_DROP_W, DROP_H,
		FpsPresetLabel(m_iFpsPreset), m_bDropFps);
	RenderRowLabel(bx + R_X, by + PERF_VSYNC_Y, "VSync");

	RenderSectionHeader(bx + R_X, by + COMBAT_HDR_Y, R_COL_W, "COMBATE");
	RenderSectionLine(bx + R_X, by + COMBAT_HDR_Y + 12.f, R_COL_W);
	RenderRowLabel(bx + R_X, by + COMBAT_HP_Y, "Auto HP");
	RenderRowLabel(bx + R_X, by + COMBAT_PVP_Y, "Auto PvP");
}

void SEASON3B::CNewUIOptionWindow::RenderChecked(float x, float y, bool bEnable)
{
	// 15x15, top-left Y identical to RenderRowLabel Y
	if (bEnable)
		RenderImage(IMAGE_CHECK_LIVE, x, y, OptLayout::CHECK_SZ, OptLayout::CHECK_SZ, 0.0, 0.0, 0.75, 0.75);
	else
		RenderImage(IMAGE_UNCHECK_LIVE, x, y, OptLayout::CHECK_SZ, OptLayout::CHECK_SZ, 0.0, 0.0, 0.75, 0.75);
}

void SEASON3B::CNewUIOptionWindow::RenderButtons()
{
	using namespace OptLayout;
	const float bx = m_Pos.x;
	const float by = m_Pos.y;

	// RIGHT: every toggle uses R_CHECK + same row Y as its label
	RenderChecked(bx + R_CHECK, by + GAME_ROW0, m_bAutoAttack);
	RenderChecked(bx + R_CHECK, by + GAME_ROW1, m_bWhisperSound);
	RenderChecked(bx + R_CHECK, by + GAME_ROW2, m_bSlideHelp);
	RenderChecked(bx + R_CHECK, by + GAME_ROW3, m_RenderEffect);
	RenderChecked(bx + R_CHECK, by + GAME_ROW4, m_RenderEquipment);
	RenderChecked(bx + R_CHECK, by + GAME_ROW5, m_RenderTerrain);
	RenderChecked(bx + R_CHECK, by + GAME_ROW6, m_RenderObjects);
	RenderChecked(bx + R_CHECK, by + GAME_ROW7, m_bPhotoMode);
	RenderChecked(bx + R_CHECK, by + GAME_ROW8, m_bShowPlayerName);

	// VSync / Auto HP / Auto PvP — same X as Automatic Attack checkbox
	RenderChecked(bx + R_CHECK, by + PERF_VSYNC_Y, m_bVSync);
	RenderChecked(bx + R_CHECK, by + COMBAT_HP_Y, gAutoCombat.IsAutoHP());
	RenderChecked(bx + R_CHECK, by + COMBAT_PVP_Y, gAutoCombat.IsAutoPvP());

	// LEFT: GRAPHICS + Effect limitation bar
	RenderChecked(bx + GFX_LOW_X, by + GFX_ROW_Y, m_iGraphicQuality == 0);
	RenderChecked(bx + GFX_MED_X, by + GFX_ROW_Y, m_iGraphicQuality == 1);
	RenderChecked(bx + GFX_HIGH_X, by + GFX_ROW_Y, m_iGraphicQuality == 2);

	RenderImage(IMAGE_OPTION_EFFECT_BACK, bx + FX_BAR_X, by + FX_BAR_Y, FX_BAR_W, FX_BAR_H);
	if (m_iRenderLevel >= 0)
		RenderImage(IMAGE_OPTION_EFFECT_COLOR, bx + FX_BAR_X, by + FX_BAR_Y, FX_BAR_W * 0.2f * (m_iRenderLevel + 1), FX_BAR_H);

	// Musica volume bar
	RenderImage(IMAGE_OPTION_VOLUME_BACK, bx + VOL_BAR_X, by + MUSIC_BAR_Y, VOL_BAR_W, 16.f);
	if (m_iMusicVolume > 0)
		RenderImage(IMAGE_OPTION_VOLUME_COLOR, bx + VOL_BAR_X, by + MUSIC_BAR_Y, VOL_BAR_W * 0.1f * m_iMusicVolume, 16.f);

	// Som volume bar
	RenderImage(IMAGE_OPTION_VOLUME_BACK, bx + VOL_BAR_X, by + SOUND_BAR_Y, VOL_BAR_W, 16.f);
	if (m_iSoundVolume > 0)
		RenderImage(IMAGE_OPTION_VOLUME_COLOR, bx + VOL_BAR_X, by + SOUND_BAR_Y, VOL_BAR_W * 0.1f * m_iSoundVolume, 16.f);

	// Modal picker last (on top of everything): black veil + centered list
	if (m_bDropRes)
		RenderDropdownModal(0);
	else if (m_bDropFont)
		RenderDropdownModal(1);
	else if (m_bDropFps)
		RenderDropdownModal(2);
}

void SEASON3B::CNewUIOptionWindow::RenderTable(float x, float y, float width, float height)
{
	EnableAlphaTest();

	glColor4f(0.0, 0.0, 0.0, 0.40000001);

	RenderColor((x + 3), (y + 2), (width - 7), (height - 7), 0.0, 0);

	EndRenderColor();

#if MAIN_UPDATE <= 603
	RenderImage(IMAGE_MAIN_TABLE_TOP_LEFT, x, y, 14.0, 14.0);

	RenderImage(IMAGE_MAIN_TABLE_TOP_RIGHT, (x + width - 14), y, 14.0, 14.0);

	RenderImage(IMAGE_MAIN_TABLE_BOTTOM_LEFT, x, (y + height - 14), 14.0, 14.0);

	RenderImage(IMAGE_MAIN_TABLE_BOTTOM_RIGHT, (x + width - 14), (y + height - 14), 14.0, 14.0);

	RenderImage(IMAGE_MAIN_TABLE_TOP_PIXEL, (x + 6), y, (width - 12), 14.0);

	RenderImage(IMAGE_MAIN_TABLE_RIGHT_PIXEL, (x + width - 14), (y + 6), 14.0, (height - 14));

	RenderImage(IMAGE_MAIN_TABLE_BOTTOM_PIXEL, (x + 6), (y + height - 14), (width - 12), 14.0);

	RenderImage(IMAGE_MAIN_TABLE_LEFT_PIXEL, x, (y + 6), 14.0, (height - 14));
#endif
}

void SEASON3B::CNewUIOptionWindow::SetAutoAttack(bool bAuto)
{
	m_bAutoAttack = bAuto;
}

bool SEASON3B::CNewUIOptionWindow::IsAutoAttack()
{
	return m_bAutoAttack;
}

void SEASON3B::CNewUIOptionWindow::SetWhisperSound(bool bSound)
{
	m_bWhisperSound = bSound;
}

bool SEASON3B::CNewUIOptionWindow::IsWhisperSound()
{
	return m_bWhisperSound;
}

void SEASON3B::CNewUIOptionWindow::SetSlideHelp(bool bHelp)
{
	m_bSlideHelp = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::IsSlideHelp()
{
	return m_bSlideHelp;
}

void SEASON3B::CNewUIOptionWindow::SetVolumeLevel(int iVolume)
{
	m_iVolumeLevel = iVolume;
}

int SEASON3B::CNewUIOptionWindow::GetVolumeLevel()
{
	return m_iVolumeLevel;
}

void SEASON3B::CNewUIOptionWindow::SetRenderLevel(int iRender)
{
	m_iRenderLevel = iRender;
}

int SEASON3B::CNewUIOptionWindow::GetRenderLevel()
{
	return m_iRenderLevel;
}

void SEASON3B::CNewUIOptionWindow::SetRenderEffect(bool bHelp)
{
	m_RenderEffect = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRenderEffect()
{
	return m_RenderEffect;
}

void SEASON3B::CNewUIOptionWindow::SetRenderEquipment(bool bHelp)
{
	m_RenderEquipment = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRenderEquipment()
{
	return m_RenderEquipment;
}

void SEASON3B::CNewUIOptionWindow::SetRenderTerrain(bool bHelp)
{
	m_RenderTerrain = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRenderTerrain()
{
	return m_RenderTerrain;
}

void SEASON3B::CNewUIOptionWindow::SetRenderObjects(bool bHelp)
{
	m_RenderObjects = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRenderObjects()
{
	return m_RenderObjects;
}

void SEASON3B::CNewUIOptionWindow::SetGraphicQuality(int quality)
{
	ApplyGraphicQuality(quality);
}

void SEASON3B::CNewUIOptionWindow::ApplyGraphicQuality(int quality)
{
	// Aggressive presets — real render flags already used by the client:
	//   RenderEffect   → particles / joints / sprites / effects (Create+Move+Render via EffectBudget)
	//   RenderObjects  → map props
	//   RenderTerrain  → terrain
	//   RenderEquipment→ character gear/SET glow
	//   RenderLevel    → item +level chrome (ZzzObject) AND soft FX density caps when quality=custom
	// Low  = weak PC / war FPS   | Med = balanced   | High = max beauty
	if (quality < 0) quality = 0;
	if (quality > 2) quality = 2;
	m_iGraphicQuality = quality;

	switch (quality)
	{
	case 0: // Low — most aggressive cut
		m_RenderEffect = false;   // no particles/skills FX
		m_RenderObjects = false;  // no map props
		m_RenderTerrain = true;   // keep walkable ground
		m_RenderEquipment = true; // keep gear visible (avoid “naked” models)
		m_iRenderLevel = 0;       // no +7…+15 glow/chrome
		break;
	case 1: // Medium
		m_RenderEffect = true;
		m_RenderObjects = true;
		m_RenderTerrain = true;
		m_RenderEquipment = true;
		m_iRenderLevel = 2;       // up to mid +levels
		break;
	case 2: // High
	default:
		m_RenderEffect = true;
		m_RenderObjects = true;
		m_RenderTerrain = true;
		m_RenderEquipment = true;
		m_iRenderLevel = 4;       // full +15 effect budget
		break;
	}

	SaveGraphicQualityIni();
}

void SEASON3B::CNewUIOptionWindow::SyncGraphicQualityFromFlags()
{
	// Manual check/slider changes → radio Low/Med/High or custom (-1)
	if (!m_RenderEffect && m_RenderEquipment && !m_RenderObjects && m_iRenderLevel <= 0)
		m_iGraphicQuality = 0;
	else if (m_RenderEffect && m_RenderEquipment && m_RenderTerrain && m_RenderObjects && m_iRenderLevel >= 4)
		m_iGraphicQuality = 2;
	else if (m_RenderEffect && m_RenderEquipment && m_RenderTerrain && m_RenderObjects && m_iRenderLevel >= 1 && m_iRenderLevel <= 3)
		m_iGraphicQuality = 1;
	else
		m_iGraphicQuality = -1;

	SaveGraphicQualityIni();
}

void SEASON3B::CNewUIOptionWindow::SaveGraphicQualityIni()
{
	char buf[16];
	WritePrivateProfileStringA("Graphics", "Quality", itoa(m_iGraphicQuality, buf, 10), ".\\config.ini");
	WritePrivateProfileStringA("Graphics", "RenderEffect", m_RenderEffect ? "1" : "0", ".\\config.ini");
	WritePrivateProfileStringA("Graphics", "RenderEquipment", m_RenderEquipment ? "1" : "0", ".\\config.ini");
	WritePrivateProfileStringA("Graphics", "RenderTerrain", m_RenderTerrain ? "1" : "0", ".\\config.ini");
	WritePrivateProfileStringA("Graphics", "RenderObjects", m_RenderObjects ? "1" : "0", ".\\config.ini");
	WritePrivateProfileStringA("Graphics", "RenderLevel", itoa(m_iRenderLevel, buf, 10), ".\\config.ini");
}

const char* SEASON3B::CNewUIOptionWindow::FpsPresetLabel(int preset)
{
	switch (preset)
	{
	case 0: return "30";
	case 1: return "60";
	case 2: return "120";
	case 3: return "MAX";
	default: return "120";
	}
}

int SEASON3B::CNewUIOptionWindow::FpsPresetToLimit(int preset)
{
	switch (preset)
	{
	case 0: return 30;
	case 1: return 60;
	case 2: return 120;
	case 3: return 0; // unlimited
	default: return 120;
	}
}

void SEASON3B::CNewUIOptionWindow::CycleFpsPreset()
{
	m_iFpsPreset = (m_iFpsPreset + 1) % 4; // 30 → 60 → 120 → MAX → 30
	ApplyPerformanceSettings();
	SavePerformanceSettings();
}

void SEASON3B::CNewUIOptionWindow::ToggleVSyncOption()
{
	m_bVSync = !m_bVSync;
	ApplyPerformanceSettings();
	SavePerformanceSettings();
}

void SEASON3B::CNewUIOptionWindow::ApplyPerformanceSettings()
{
	const int limit = FpsPresetToLimit(m_iFpsPreset);

	if (limit > 0)
	{
		gmProtect->ajust_fps_render = static_cast<BYTE>(limit);
		gFPSController->SetTargetFPS(static_cast<double>(limit));
		gFPSController->SetLimitingEnabled(true);
	}
	else
	{
		// MAX: no software cap; keep 120 as logic baseline for protect helpers
		gmProtect->ajust_fps_render = 120;
		gFPSController->SetTargetFPS(0.0);
		gFPSController->SetLimitingEnabled(false);
	}

	// Keep boot dialog radio in sync (0=120, 1=30, 2=40 unused, 3=60, 4=120)
	switch (m_iFpsPreset)
	{
	case 0: targetIndexFPS = 1; break; // 30
	case 1: targetIndexFPS = 3; break; // 60
	case 2: targetIndexFPS = 4; break; // 120
	case 3: targetIndexFPS = 0; break; // MAX ~ treat as 120 radio
	default: targetIndexFPS = 4; break;
	}

#ifdef V_SYNCRONIZE
	if (IsVSyncAvailable())
	{
		if (m_bVSync)
			EnableVSync();
		else
			DisableVSync();
	}
#endif
}

void SEASON3B::CNewUIOptionWindow::SavePerformanceSettings()
{
	char buf[16];
	WritePrivateProfileStringA("Performance", "FpsPreset", itoa(m_iFpsPreset, buf, 10), ".\\config.ini");
	WritePrivateProfileStringA("Performance", "VSync", m_bVSync ? "1" : "0", ".\\config.ini");

	leaf::CRegKey regkey;
	regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu2\\Config");
	regkey.WriteDword("targetIndexFPS", static_cast<DWORD>(targetIndexFPS));
	regkey.WriteDword("VSync", m_bVSync ? 1 : 0);
	regkey.WriteDword("FpsPreset", static_cast<DWORD>(m_iFpsPreset));
}

void SEASON3B::CNewUIOptionWindow::LoadPerformanceSettings()
{
	int preset = GetPrivateProfileIntA("Performance", "FpsPreset", -1, ".\\config.ini");
	int vsync = GetPrivateProfileIntA("Performance", "VSync", -1, ".\\config.ini");

	// Fallback: Registry (boot / older installs)
	if (preset < 0 || preset > 3)
	{
		leaf::CRegKey regkey;
		regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu2\\Config");
		DWORD dw = 2;
		if (regkey.ReadDword("FpsPreset", dw))
			preset = static_cast<int>(dw);
		else
		{
			// Map legacy targetIndexFPS → preset
			// 0/4=120, 1=30, 2=40→60, 3=60
			switch (targetIndexFPS)
			{
			case 1: preset = 0; break;
			case 2: preset = 1; break;
			case 3: preset = 1; break;
			case 0:
			case 4:
			default: preset = 2; break;
			}
		}
	}

	if (vsync < 0)
	{
		leaf::CRegKey regkey;
		regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu2\\Config");
		DWORD dw = 0;
		if (regkey.ReadDword("VSync", dw))
			vsync = static_cast<int>(dw);
		else
			vsync = 0;
	}

	if (preset < 0 || preset > 3)
		preset = 2;
	m_iFpsPreset = preset;
	m_bVSync = (vsync != 0);

	ApplyPerformanceSettings();
}

void SEASON3B::CNewUIOptionWindow::LoadGraphicQualityIni()
{
	// Preferência: flags individuais no config.ini (se já salvas)
	bool hasIni = GetPrivateProfileIntA("Graphics", "RenderEffect", -1, ".\\config.ini") >= 0
		|| GetPrivateProfileIntA("Graphics", "Quality", -99, ".\\config.ini") != -99;

	if (!hasIni)
	{
		ApplyGraphicQuality(2); // High default
		return;
	}

	int q = GetPrivateProfileIntA("Graphics", "Quality", 2, ".\\config.ini");
	if (q >= 0 && q <= 2)
		ApplyGraphicQuality(q);

	// Overrides individuais (custom ou pós-preset)
	if (GetPrivateProfileIntA("Graphics", "RenderEffect", -1, ".\\config.ini") >= 0)
		m_RenderEffect = GetPrivateProfileIntA("Graphics", "RenderEffect", 1, ".\\config.ini") != 0;
	if (GetPrivateProfileIntA("Graphics", "RenderEquipment", -1, ".\\config.ini") >= 0)
		m_RenderEquipment = GetPrivateProfileIntA("Graphics", "RenderEquipment", 1, ".\\config.ini") != 0;
	if (GetPrivateProfileIntA("Graphics", "RenderTerrain", -1, ".\\config.ini") >= 0)
		m_RenderTerrain = GetPrivateProfileIntA("Graphics", "RenderTerrain", 1, ".\\config.ini") != 0;
	if (GetPrivateProfileIntA("Graphics", "RenderObjects", -1, ".\\config.ini") >= 0)
		m_RenderObjects = GetPrivateProfileIntA("Graphics", "RenderObjects", 1, ".\\config.ini") != 0;
	if (GetPrivateProfileIntA("Graphics", "RenderLevel", -1, ".\\config.ini") >= 0)
	{
		m_iRenderLevel = GetPrivateProfileIntA("Graphics", "RenderLevel", 4, ".\\config.ini");
		if (m_iRenderLevel < 0) m_iRenderLevel = 0;
		if (m_iRenderLevel > 4) m_iRenderLevel = 4;
	}

	// Never leave Equipment off (naked models / broken SET)
	if (!m_RenderEquipment)
		m_RenderEquipment = true;

	// Radio from flags (Low may use RenderLevel 0)
	if (!m_RenderEffect && m_RenderEquipment && !m_RenderObjects && m_iRenderLevel <= 0)
		m_iGraphicQuality = 0;
	else if (m_RenderEffect && m_RenderEquipment && m_RenderTerrain && m_RenderObjects && m_iRenderLevel >= 4)
		m_iGraphicQuality = 2;
	else if (m_RenderEffect && m_RenderEquipment && m_RenderTerrain && m_RenderObjects && m_iRenderLevel >= 1 && m_iRenderLevel <= 3)
		m_iGraphicQuality = 1;
	else
		m_iGraphicQuality = -1;
}

void SEASON3B::CNewUIOptionWindow::change_resolution()
{
	if (m_iResIndex < 0 || m_iResIndex >= (int)m_resOptions.size())
		return;

	int index = gwinhandle->GetDisplayIndex(m_resOptions[m_iResIndex].c_str());

	if (m_Resolution != index)
	{
		leaf::CRegKey regkey;
		regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu2\\Config");

		if (regkey.WriteDword("Resolution", index))
		{
			m_Resolution = index;
			double backupWidth = gwinhandle->GetScreenX();
			double backupHight = gwinhandle->GetScreenY();

			gwinhandle->SetDisplayIndex(index, false);
			CameraFactorPtr->Init();
			OpenFont();
			ClearInput(TRUE);

			CInput& rInput = CInput::Instance();
			rInput.Create(gwinhandle->GethWnd(), WindowWidth, WindowHeight);

			g_pMoveCommandWindow->SetPos(1, 1);
			this->SetPos(0, 70);
			g_pNewUISystem->RenderFrameUpdate(backupWidth, backupHight);
			g_pNewUI3DRenderMng->Reload3DEffectObject(WindowWidth, WindowHeight);
		}
	}
}

void SEASON3B::CNewUIOptionWindow::change_fontsize()
{
	if (m_iFontIndex < 0 || m_iFontIndex >= (int)m_fontOptions.size())
		return;

	int fontsize = atoi(m_fontOptions[m_iFontIndex].c_str());
	if (fontsize <= 0)
		return;

	if (fontsize != FontHeight)
	{
		gwinhandle->SetFontSize(fontsize);

		pugi::xml_document file;
		pugi::xml_parse_result res = file.load_file("Data\\Resolutions.xml");
		if (res.status == pugi::status_ok)
		{
			pugi::xml_node root = file.child("ResolutionsInfo");
			for (pugi::xml_node child = root.child("Resolution"); child; child = child.next_sibling())
			{
				if (gwinhandle->GetDisplayIndex() == child.attribute("index").as_int())
				{
					child.attribute("font_size").set_value(fontsize);
					file.save_file("Data\\Resolutions.xml");
					return;
				}
			}
		}
	}
}

void SEASON3B::CNewUIOptionWindow::LoadResolution(const char* filename)
{
	m_resOptions.clear();
	m_fontOptions.clear();
	m_iResIndex = 0;
	m_iFontIndex = 0;

	pugi::xml_document file;
	pugi::xml_parse_result res = file.load_file(filename);
	if (res.status != pugi::status_ok)
		return;

	int fontSize = 13;
	int fontindex = 0;
	int resIndex = 0;

	pugi::xml_node root = file.child("ResolutionsInfo");
	for (pugi::xml_node child_io = root.child("Resolution"); child_io; child_io = child_io.next_sibling())
	{
		std::string text_name = child_io.attribute("name").as_string();
		m_resOptions.push_back(text_name);

		if (gwinhandle->GetDisplayIndex() == child_io.attribute("index").as_int())
		{
			fontSize = child_io.attribute("font_size").as_int();
			m_iResIndex = resIndex;
		}
		resIndex++;
	}

	root = file.child("FontSize");
	int i = 0;
	for (pugi::xml_node child_io = root.child("size"); child_io; child_io = child_io.next_sibling(), i++)
	{
		std::string text_name = child_io.attribute("fontsize").as_string();
		m_fontOptions.push_back(text_name);
		if (fontSize == child_io.attribute("fontsize").as_int())
			fontindex = i;
	}
	m_iFontIndex = fontindex;
	m_Resolution = gwinhandle->GetDisplayIndex();
}
