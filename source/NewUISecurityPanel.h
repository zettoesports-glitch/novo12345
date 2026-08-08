#pragma once
#include "NewUIBase.h"
#include "NewUIManager.h"
#include <string>

namespace SEASON3B
{
	// Dois paineis SEPARADOS (mesmo Obj, modos isolados):
	//
	// MODE_LOCK  (Menu > Lock item)
	//   - So trava/destrava itens (senha do Lock 1-7)
	//
	// MODE_PASS  (Menu > Senha)
	//   - So troca a SENHA DA CONTA (login / memb__pwd)
	//   - Campos: atual, nova, confirmar + captcha
	//
	class CNewUISecurityPanel : public CNewUIObj
	{
	public:
		enum {
			WINDOW_W = 340,
			WINDOW_H = 380,
			WINDOW_H_LOCK = 280,
			WINDOW_H_PASS = 400,
		};
		enum EMode {
			MODE_LOCK = 0, // Lock / Unlock itens
			MODE_PASS = 1, // Trocar senha da CONTA
		};

	private:
		CNewUIManager* m_pNewUIMng;
		POINT m_Pos;
		EMode m_Mode;
		int m_Focus;
		char m_Field[4][22]; // conta ate 10 + null; captcha 4
		std::string m_Captcha; // so MODE_PASS
		enum {
			COOLDOWN_MS = 3000,
			LOCK_PASS_MAX = 7,
			ACC_PASS_MAX = 10,
			// Segurar Backspace: delay inicial + intervalo de apagar
			BACKSPACE_HOLD_DELAY_MS = 350,
			BACKSPACE_HOLD_RATE_MS = 45,
		};
		DWORD m_CooldownUntil;
		DWORD m_BackspaceHoldStart; // tick do 1o press
		DWORD m_BackspaceLastDelete; // tick do ultimo apagar

	public:
		CNewUISecurityPanel();
		virtual ~CNewUISecurityPanel();

		bool Create(CNewUIManager* pNewUIMng, float x, float y);
		void Release();
		void SetPos(float x, float y);
		void OpenMode(EMode mode);

		bool UpdateKeyEvent();
		bool UpdateMouseEvent();
		bool Render();
		bool Update();
		float GetLayerDepth();
		void OpenningProcess();
		void ClosingProcess();

		int WindowH() const { return (m_Mode == MODE_PASS) ? WINDOW_H_PASS : WINDOW_H_LOCK; }

	private:
		void ClearFields();
		void RefreshCaptcha();
		void AppendChar(char c);
		void Backspace();
		void DrawFrame(const char* title);
		void DrawField(int idx, float x, float y, float w, float h, const char* label, bool mask);
		void DrawCaptcha(float x, float y, float w, float h, int fieldIdx);
		void DrawButton(float x, float y, float w, float h, const char* text, bool green, bool& outHover, bool enabled = true);
		bool FieldHit(float x, float y, float w, float h) const;
		bool IsOnCooldown() const;
		int CooldownSecLeft() const;
		void StartCooldown();
		bool GuardCooldown();
		bool IsValidLockPass(const char* pass) const; // 1-7 alnum (item lock)
		bool IsValidAccountPass(const char* pass) const; // 4-10 alnum (conta)
		bool GuardCaptcha(int fieldIdx);

		// --- MODE_LOCK only ---
		void DoLock();
		void DoUnlock();

		// --- MODE_PASS only (senha da CONTA) ---
		void DoChangeAccountPass();

		// LOCK: 1 campo | PASS: 4 (atual, nova, conf, captcha)
		int FieldCount() const { return (m_Mode == MODE_LOCK) ? 1 : 4; }
		int CaptchaField() const { return 3; }
		int MaxChars(int field) const;
	};
}
