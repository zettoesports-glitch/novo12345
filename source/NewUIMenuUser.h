#pragma once
#include "NewUIBase.h"
#include "NewUIManager.h"

namespace SEASON3B
{
	// Custom menu — 6 icones 80x80 (3x2): joias, vip, eventos, ranking, travar, senha
	class CNewUIMenuUser : public CNewUIObj
	{
	public:
		enum {
			WINDOW_W = 320,
			WINDOW_H = 320,
			ICON_BTN = 80,
			ICON_TEX = 80,
			BTN_COLS = 3,
			BTN_ROWS = 2,
			BTN_COUNT = 6,
		};

		enum EMenuBtn {
			BTN_JEWEL = 0,
			BTN_VIP,
			BTN_EVENT,
			BTN_RANK,
			BTN_LOCK,
			BTN_SENHA,
		};

	private:
		CNewUIManager* m_pNewUIMng;
		POINT m_Pos;

	public:
		CNewUIMenuUser();
		virtual ~CNewUIMenuUser();
		bool Create(CNewUIManager* pNewUIMng, float x, float y);
		void Release();

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

	private:
		void RenderFrame();
		void RenderMenuButtons();
		bool GetBtnRect(int index, float& outX, float& outY, float& outW, float& outH) const;
		int HitTestBtn() const;
		bool IsClickClose() const;
		GLuint GetBtnTexture(int index) const;
		const char* GetBtnLabel(int index) const;
	};
}
