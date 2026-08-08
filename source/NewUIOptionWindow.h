// NewUIOptionWindow.h: interface for the CNewUIOptionWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NEWUIOPTIONWINDOW_H__1469FA1D_7C15_4AFE_AD6E_59C303E72BC0__INCLUDED_)
#define AFX_NEWUIOPTIONWINDOW_H__1469FA1D_7C15_4AFE_AD6E_59C303E72BC0__INCLUDED_

#pragma once

#include <vector>
#include <string>
#include "NewUIManager.h"
#include "NewUIMyInventory.h"
#include "NewUIQuestProgress.h"
#include "Winmain.h"

namespace SEASON3B
{
	
	class CNewUIOptionWindow  : public CNewUIObj 
	{
#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM
	public:
#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM
		enum IMAGE_LIST
		{
			IMAGE_MAIN_TABLE_TOP_LEFT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_LEFT,	//. newui_item_table01(L).tga (14,14)
			IMAGE_MAIN_TABLE_TOP_RIGHT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_RIGHT,	//. newui_item_table01(R).tga (14,14)
			IMAGE_MAIN_TABLE_BOTTOM_LEFT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_LEFT,	//. newui_item_table02(L).tga (14,14)
			IMAGE_MAIN_TABLE_BOTTOM_RIGHT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_RIGHT,	//. newui_item_table02(R).tga (14,14)
			IMAGE_MAIN_TABLE_TOP_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_PIXEL,			//. newui_item_table03(up).tga (1, 14)
			IMAGE_MAIN_TABLE_BOTTOM_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_PIXEL,	//. newui_item_table03(dw).tga (1,14)
			IMAGE_MAIN_TABLE_LEFT_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_LEFT_PIXEL,		//. newui_item_table03(L).tga (14,1)
			IMAGE_MAIN_TABLE_RIGHT_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_RIGHT_PIXEL,
			IMAGE_OPTION_FRAME_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
			IMAGE_OPTION_BTN_CLOSE = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_CLOSE,
			IMAGE_OPTION_FRAME_DOWN = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,

			IMAGE_OPTION_FRAME_UP = BITMAP_OPTION_BEGIN,
			IMAGE_OPTION_FRAME_LEFT,
			IMAGE_OPTION_FRAME_RIGHT,
			IMAGE_OPTION_LINE,
			IMAGE_OPTION_POINT,
			IMAGE_OPTION_BTN_CHECK,
			IMAGE_OPTION_EFFECT_BACK,
			IMAGE_OPTION_EFFECT_COLOR,
			IMAGE_OPTION_VOLUME_BACK,
			IMAGE_OPTION_VOLUME_COLOR,
			IMAGE_ND_BTN_L = CNewUIQuestProgress::IMAGE_QP_BTN_L,				// Quest_bt_L.tga	(17,36)
			IMAGE_ND_BTN_R = CNewUIQuestProgress::IMAGE_QP_BTN_R,
			IMAGE_CHECK_LIVE = BITMAP_IMAGE_FRAME_EMU + 10,
			IMAGE_UNCHECK_LIVE = BITMAP_IMAGE_FRAME_EMU + 11,
			// Same arrow art as login account list (15x13)
			IMAGE_OPTION_ARROW_UP = BITMAP_IMAGE_FRAME_EMU + 12,
			IMAGE_OPTION_ARROW_DOWN = BITMAP_IMAGE_FRAME_EMU + 13,
		};

	public:
		CNewUIOptionWindow();
		virtual ~CNewUIOptionWindow();

		bool Create(CNewUIManager* pNewUIMng, int x, int y);
		void Release();
		
		void SetPos(int x, int y);
		
		bool UpdateMouseEvent();
		bool UpdateKeyEvent();
		bool Update();
		bool Render();

		float GetLayerDepth();	//. 10.5f
		float GetKeyEventOrder();	// 10.f;
		
		void OpenningProcess();
		void ClosingProcess();

		void SetGameOptions(BYTE GameOption);

		void SetAutoAttack(bool bAuto);
		bool IsAutoAttack();
		void SetWhisperSound(bool bSound);
		bool IsWhisperSound();
		void SetSlideHelp(bool bHelp);
		bool IsSlideHelp();
		void SetVolumeLevel(int iVolume);
		int GetVolumeLevel();
		void SetRenderLevel(int iRender);
		int GetRenderLevel();

		void SetRenderEffect(bool bHelp);
		bool GetRenderEffect();
		void SetRenderEquipment(bool bHelp);
		bool GetRenderEquipment();
		void SetRenderTerrain(bool bHelp);
		bool GetRenderTerrain();
		void SetRenderObjects(bool bHelp);
		bool GetRenderObjects();

		// Graphic Quality: 0=Low, 1=Medium, 2=High (-1=custom)
		void SetGraphicQuality(int quality);
		int  GetGraphicQuality() const { return m_iGraphicQuality; }
		void ApplyGraphicQuality(int quality);
		void SyncGraphicQualityFromFlags();

		// Performance: FpsPreset 0=30, 1=60, 2=120, 3=MAX
		int  GetFpsPreset() const { return m_iFpsPreset; }
		bool IsVSyncEnabledOption() const { return m_bVSync; }

		// Modo Foto: hide full HUD for screenshots
		bool IsPhotoMode() const { return m_bPhotoMode; }
		void SetPhotoMode(bool bOn);

		// Show player names above heads (players only; default off)
		bool IsShowPlayerName() const { return m_bShowPlayerName; }
		void SetShowPlayerName(bool bOn);

		void RenderTable(float x, float y, float width, float height);

		void SetSoundVolume(int vol) { m_iSoundVolume = vol; }
		int  GetSoundVolume() { return m_iSoundVolume; }
		void SetMusicVolume(int vol) { m_iMusicVolume = vol; }
		int  GetMusicVolume() { return m_iMusicVolume; }

	private:
		void LoadImages();
		void UnloadImages();
		void SetButtonInfo();
		void RenderFrame();
		void RenderContents();
		void RenderChecked(float x, float y, bool bEnable);
		void RenderButtons();
		void RenderDropdownBox(float x, float y, float w, float h, const char* text, bool open);
		// Modal picker (black veil + centered list): 0=Window, 1=Font, 2=FPS
		void RenderDropdownModal(int kind);
		bool UpdateDropdownModal(int kind);
		void change_resolution();
		void change_fontsize();
		void LoadResolution(const char* filename);
		void SaveGraphicQualityIni();
		void LoadGraphicQualityIni();
		void LoadPerformanceSettings();
		void SavePerformanceSettings();
		void ApplyPerformanceSettings();
		void CycleFpsPreset();
		void ToggleVSyncOption();
		void CloseAllDropdowns();
		static const char* FpsPresetLabel(int preset);
		static int FpsPresetToLimit(int preset);
	private:
		CNewUIManager*				m_pNewUIMng;
		POINT						m_Pos;
		CNewUIButton m_BtnClose;
		bool m_bAutoAttack;
		bool m_bWhisperSound;
		bool m_bSlideHelp;
		int m_iVolumeLevel;
		int m_iRenderLevel;
		bool m_RenderEffect;
		bool m_RenderEquipment;
		bool m_RenderTerrain;
		bool m_RenderObjects;
		bool m_bPhotoMode;      // Modo Foto
		bool m_bShowPlayerName; // Show player names (not monsters)
		int  m_iGraphicQuality; // 0 Low, 1 Medium, 2 High, -1 custom
		int  m_Resolution;
		int  m_iFpsPreset;      // 0=30, 1=60, 2=120, 3=MAX
		bool m_bVSync;
		int  m_iSoundVolume;
		int  m_iMusicVolume;

		// Compact dropdowns (resolution / font / fps) — modal lists
		std::vector<std::string> m_resOptions;
		std::vector<std::string> m_fontOptions;
		std::vector<std::string> m_fpsOptions;
		int  m_iResIndex;
		int  m_iFontIndex;
		bool m_bDropRes;
		bool m_bDropFont;
		bool m_bDropFps;
		int  m_iDropScrollRes;
		int  m_iDropScrollFont;
		int  m_iDropScrollFps;
	};
}

#endif // !defined(AFX_NEWUIOPTIONWINDOW_H__1469FA1D_7C15_4AFE_AD6E_59C303E72BC0__INCLUDED_)
