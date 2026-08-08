#pragma once
#include "NewUIBase.h"
#include "NewUIManager.h"
#include "UIControlRender.h"

namespace SEASON3B
{
	struct TEMPLATE_RANKING
	{
		std::string Name;
		std::string Class;
		BYTE Vip;
		DWORD Score;
		TEMPLATE_RANKING(const std::string& n, const std::string& c, BYTE v, DWORD s)
			: Name(n), Class(c), Vip(v), Score(s)
		{
		}
		BYTE GetVip() {
			return Vip;
		}
		DWORD GetScore() {
			return Score;
		}
		const char* GetName() {
			return Name.c_str();
		}
		const char* GetClass() {
			return Class.c_str();
		}
	};

	class CNewUIRankingTop : public CNewUIObj
	{
	public:
		// Larger panel + Menu Custom chrome
		// Preview 3D precisa de area larga/alta p/ armas e asas nao cortarem
		enum {
			WINDOW_W = 540,
			WINDOW_H = 430,
			LIST_VIEW = 14,
		};

		enum IMAGE_LIST
		{
			// VIP badges (keep)
			IMAGE_TOP_LEVEL1 = BITMAP_IMAGE_FRAME_EMU + 4,
			IMAGE_TOP_LEVEL2 = BITMAP_IMAGE_FRAME_EMU + 5,
			IMAGE_TOP_LEVEL3 = BITMAP_IMAGE_FRAME_EMU + 6,
		};
	private:
		CUIComboBox m_ComboRankingList;
		std::vector<std::pair<std::string, int>> m_ComboBXHOptions;

		CNewUIManager* m_pNewUIMng;
		POINT m_Pos;
		CUIPhotoViewer m_RenderCharacter;

		bool is_request;
		size_t m_RankListView;
		size_t m_RankMaxTop;
		size_t m_RankIndexCur;
		size_t m_RankSelectIndex;
		char m_RankName[50];
		char m_RankColum[50];
		CNewUIScrollBarHTML m_pScrollBar;
		std::vector<TEMPLATE_RANKING> m_RankList;
	public:
		CNewUIRankingTop();
		virtual ~CNewUIRankingTop();

		bool Create(CNewUIManager* pNewUIMng, float x, float y);
		void Release();
		void SetInfo();
		void SetPos(float x, float y);
		void LoadImages();
		void UnloadImages();
		bool UpdateKeyEvent();
		bool UpdateMouseEvent();
		bool Render();
		bool Update();
		float GetLayerDepth();

		void OpenningProcess();
		void ClosingProcess();

		void RenderFrame();
		void RenderTexte();

		void ReceiveRankingInfo(BYTE* ReceiveBuffer);
		void ReceiveRankingListInfo(BYTE* ReceiveBuffer);
		// Fase 2: carrega set visual no preview 3D
		void ReceiveRankingPreview(BYTE* ReceiveBuffer);
	private:
		void RequestServerRankingInfo(BYTE Index);
		void RequestRankingPreview(const char* name);
		float ListLeft() const;
		float ListTop() const;
		float ListWidth() const;
		float ListHeight() const;
		// Cooldown entre previews 3D (evita spam GS/DS)
		enum { PREVIEW_COOLDOWN_MS = 5000 };
		DWORD m_LastPreviewTick;
		char m_PreviewName[16];
	};
}
