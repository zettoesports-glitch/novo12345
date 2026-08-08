#if !defined(AFX_NEWUIINVENTORYJEWEL_H__1151C4F9_04A5_47B1_A717_E7905BEEAD08__INCLUDED_)
#define AFX_NEWUIINVENTORYJEWEL_H__1151C4F9_04A5_47B1_A717_E7905BEEAD08__INCLUDED_
#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUIButton.h"
#include "NewUIMyInventory.h"

namespace SEASON3B
{
	class WareHoly
	{
	public:
		WareHoly() :bKeyIndex(0), bItemindex(-1), bItemLevel(-1), bItemCount(0) {};
		WareHoly(BYTE KeyIndex, int itemindex, int itemLevel, __int64 ItemCount) :
			bKeyIndex(KeyIndex), bItemindex(itemindex), bItemLevel(itemLevel), bItemCount(ItemCount) {
		};
		~WareHoly() {};

		void setIndex(int index) { bItemindex = index; };
		int GetIndex() { return bItemindex; };
		int GetLevel() { return (bItemLevel != -1) ? bItemLevel : 0; };
		void setValue(__int64 value) { bItemCount = value; };
		__int64 GetValue() { return bItemCount; };

	private:
		BYTE bKeyIndex;
		int bItemindex;
		int bItemLevel;
		__int64 bItemCount;
	};

	// Standalone Jewel Bank — Menu Custom chrome, centered
	class CNewUIInventoryJewel : public CNewUIObj
	{
	public:
		enum {
			WINDOW_W = 360,
			WINDOW_H = 470,
			LIST_ROWS = 6,
			ROW_H = 32,
		};

	private:
		CNewUIManager* m_pNewUIMng;
		POINT m_Pos;
		int		m_dwCurIndex;
		int		m_dwSelIndex;
		int		m_nSelPage;
		int		m_nMaxPage;
		int		m_nOptionCurrent;
		RECT	m_nRectItem;

		// Action buttons use circular jewel medallion art (not old text plates)
		CNewUIButton m_ButtonNext;
		CNewUIButton m_ButtonBack;

		std::vector<WareHoly> m_bItems;

	public:
		CNewUIInventoryJewel();
		virtual ~CNewUIInventoryJewel();

		bool Create(CNewUIManager* pNewUIMng, int x, int y);
		void Initialize();
		void Release();
		void SetPos(int x, int y);
		void InitButtons();
		bool Render();
		bool Update();
		bool UpdateMouseEvent();
		bool UpdateKeyEvent();
		float GetLayerDepth();
		bool CheckExpansionInventory();
		void OpenningProcess();
		void ClosingProcess();
		void RemoveData();
		void InsertData(BYTE Index, short ItemIndex, short ItemLevel, __int64 count);
		bool check_budget(DWORD _iCount);

	private:
		void CenterWindow();
		void RenderFrame();
		void RenderTexts();
		void RenderInter();
		void RenderButtons();
		void RenderHoly();
		void FrameTarget(float iPos_x, float iPos_y, float width, float height, DWORD color);
		void render_option_group();
		float ListLeft() const { return m_Pos.x + 20.f; }
		// Compact layout: smaller preview + tighter gaps name → Qty → list
		float ListTop() const { return m_Pos.y + 168.f; }
		float ListWidth() const { return (float)WINDOW_W - 40.f; }
		// Preview was 110 → half = 55
		enum { PREVIEW_SIZE = 55 };

		void GetDepositRect(float& x, float& y, float& w, float& h) const;
		void GetWithdrawRect(float& x, float& y, float& w, float& h) const;
		bool HitCircle(float x, float y, float w, float h) const;
		void RenderActionButtons();
	};
}

#endif
