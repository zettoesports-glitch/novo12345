#include "stdafx.h"
#include "NewUISystem.h"
#include "NewUISecurityPanel.h"
#include "DSPlaySound.h"
#include "UIBaseDef.h"
#include "_TextureIndex.h"
#include "wsclientinline.h"
#include "UIControls.h"
#include "ZzzInterface.h"
#include "CBInterface.h"
#include "Protocol.h"

using namespace SEASON3B;

namespace
{
	const float kFieldH = 26.f;
	const float kBtnH = 30.f;
}

SEASON3B::CNewUISecurityPanel::CNewUISecurityPanel()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;
	m_Mode = MODE_LOCK;
	m_Focus = 0;
	m_Captcha.clear();
	m_CooldownUntil = 0;
	m_BackspaceHoldStart = 0;
	m_BackspaceLastDelete = 0;
	ClearFields();
}

SEASON3B::CNewUISecurityPanel::~CNewUISecurityPanel()
{
	Release();
}

bool SEASON3B::CNewUISecurityPanel::Create(CNewUIManager* pNewUIMng, float x, float y)
{
	if (!pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(INTERFACE_ACCOUNT_SECURITY, this);

	LoadBitmap("Interface\\HUD\\bg250.tga", IMAGE_BG_SERVER_INTERFACE_250, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\timerserver-bar.tga", IMAGE_TITTLE_SERVER_INTERFACE, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\button_close_navi.tga", IMAGE_BUTTON_CLOSE_NAVI, GL_LINEAR);

	SetPos(x, y);
	Show(false);
	return true;
}

void SEASON3B::CNewUISecurityPanel::Release()
{
	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUISecurityPanel::SetPos(float x, float y)
{
	m_Pos.x = (LONG)x;
	m_Pos.y = (LONG)y;
}

void SEASON3B::CNewUISecurityPanel::ClearFields()
{
	for (int i = 0; i < 4; i++)
		m_Field[i][0] = '\0';
	m_Focus = 0;
}

void SEASON3B::CNewUISecurityPanel::RefreshCaptcha()
{
	// Captcha so faz sentido no painel de troca de senha
	if (m_Mode != MODE_PASS)
	{
		m_Captcha.clear();
		return;
	}

	if (gInterface)
		m_Captcha = gInterface->generateCaptcha(4);
	else
	{
		m_Captcha.clear();
		for (int i = 0; i < 4; i++)
			m_Captcha.push_back((char)('0' + (rand() % 10)));
	}
	m_Field[CaptchaField()][0] = '\0';
}

void SEASON3B::CNewUISecurityPanel::OpenMode(EMode mode)
{
	m_Mode = mode;
	ClearFields();
	m_Captcha.clear();

	// Captcha apenas no painel Senha
	if (m_Mode == MODE_PASS)
		RefreshCaptcha();

	SetPos(PositionX_The_Mid((float)WINDOW_W), PositionY_The_Mid((float)WindowH()));
	g_pNewUISystem->Show(INTERFACE_ACCOUNT_SECURITY);
}

void SEASON3B::CNewUISecurityPanel::OpenningProcess()
{
	SetPos(PositionX_The_Mid((float)WINDOW_W), PositionY_The_Mid((float)WindowH()));
	if (m_Mode == MODE_PASS && m_Captcha.empty())
		RefreshCaptcha();
}

void SEASON3B::CNewUISecurityPanel::ClosingProcess()
{
	ClearFields();
	m_Captcha.clear();
	m_BackspaceHoldStart = 0;
	m_BackspaceLastDelete = 0;
}

float SEASON3B::CNewUISecurityPanel::GetLayerDepth()
{
	return 10.2f;
}

bool SEASON3B::CNewUISecurityPanel::Update()
{
	return true;
}

int SEASON3B::CNewUISecurityPanel::MaxChars(int field) const
{
	if (m_Mode == MODE_PASS && field == CaptchaField())
		return 4;
	if (m_Mode == MODE_PASS)
		return ACC_PASS_MAX; // senha da conta
	return LOCK_PASS_MAX; // senha do lock item
}

void SEASON3B::CNewUISecurityPanel::AppendChar(char c)
{
	if (m_Focus < 0 || m_Focus >= FieldCount())
		return;

	if (m_Mode == MODE_PASS && m_Focus == CaptchaField())
	{
		if (c < '0' || c > '9')
			return;
	}
	else if (m_Mode == MODE_PASS)
	{
		// Senha da conta: letras e numeros
		const bool ok =
			(c >= '0' && c <= '9') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z');
		if (!ok)
			return;
	}
	else
	{
		// Senha do lock: so alnum
		const bool ok =
			(c >= '0' && c <= '9') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z');
		if (!ok)
			return;
	}

	size_t n = strlen(m_Field[m_Focus]);
	if ((int)n >= MaxChars(m_Focus))
		return;
	m_Field[m_Focus][n] = c;
	m_Field[m_Focus][n + 1] = '\0';
}

void SEASON3B::CNewUISecurityPanel::Backspace()
{
	if (m_Focus < 0 || m_Focus >= FieldCount())
		return;
	size_t n = strlen(m_Field[m_Focus]);
	if (n == 0)
		return;
	m_Field[m_Focus][n - 1] = '\0';
}

bool SEASON3B::CNewUISecurityPanel::IsValidLockPass(const char* pass) const
{
	if (!pass || pass[0] == '\0')
		return false;
	size_t n = strlen(pass);
	if (n < 1 || n > (size_t)LOCK_PASS_MAX)
		return false;
	for (size_t i = 0; i < n; i++)
	{
		char c = pass[i];
		if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
			return false;
	}
	bool allZero = true;
	for (size_t i = 0; i < n; i++)
	{
		if (pass[i] != '0')
		{
			allZero = false;
			break;
		}
	}
	return !allZero;
}

bool SEASON3B::CNewUISecurityPanel::IsValidAccountPass(const char* pass) const
{
	if (!pass || pass[0] == '\0')
		return false;
	size_t n = strlen(pass);
	// Senha da conta: 4 a 10 caracteres (padrao MU)
	if (n < 4 || n > (size_t)ACC_PASS_MAX)
		return false;
	for (size_t i = 0; i < n; i++)
	{
		char c = pass[i];
		if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
			return false;
	}
	return true;
}

bool SEASON3B::CNewUISecurityPanel::UpdateKeyEvent()
{
	if (!IsVisible())
		return true;

	if (SEASON3B::IsPress(VK_ESCAPE))
	{
		g_pNewUISystem->Hide(INTERFACE_ACCOUNT_SECURITY);
		PlayBuffer(SOUND_CLICK01);
		return false;
	}

	for (int k = 0; k <= 9; k++)
	{
		if (SEASON3B::IsPress('0' + k) || SEASON3B::IsPress(VK_NUMPAD0 + k))
		{
			AppendChar((char)('0' + k));
			return false;
		}
	}
	for (int k = 0; k < 26; k++)
	{
		if (SEASON3B::IsPress('A' + k))
		{
			const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
			const bool caps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
			const bool upper = (shift != caps);
			char c = upper ? (char)('A' + k) : (char)('a' + k);
			AppendChar(c);
			return false;
		}
	}
	// Backspace: 1 toque apaga 1; segurar apaga em sequencia (com delay)
	{
		const bool backDown = (GetAsyncKeyState(VK_BACK) & 0x8000) != 0;
		const DWORD now = GetTickCount();

		if (SEASON3B::IsPress(VK_BACK))
		{
			Backspace();
			m_BackspaceHoldStart = now;
			m_BackspaceLastDelete = now;
			return false;
		}

		if (backDown && m_BackspaceHoldStart != 0)
		{
			// Apos delay inicial, apaga a cada BACKSPACE_HOLD_RATE_MS
			if ((now - m_BackspaceHoldStart) >= (DWORD)BACKSPACE_HOLD_DELAY_MS
				&& (now - m_BackspaceLastDelete) >= (DWORD)BACKSPACE_HOLD_RATE_MS)
			{
				Backspace();
				m_BackspaceLastDelete = now;
				return false;
			}
		}
		else if (!backDown)
		{
			m_BackspaceHoldStart = 0;
			m_BackspaceLastDelete = 0;
		}
	}
	if (SEASON3B::IsPress(VK_TAB))
	{
		m_Focus = (m_Focus + 1) % FieldCount();
		return false;
	}
	if (SEASON3B::IsPress(VK_RETURN))
	{
		if (m_Mode == MODE_LOCK)
		{
			// Enter no Travar: se ja travado -> destravar, senao travar
			if (g_bAccountItemsLocked)
				DoUnlock();
			else
				DoLock();
		}
		else
		{
			DoChangeAccountPass();
		}
		return false;
	}
	return true;
}

bool SEASON3B::CNewUISecurityPanel::FieldHit(float x, float y, float w, float h) const
{
	return SEASON3B::CheckMouseIn(x, y, w, h);
}

bool SEASON3B::CNewUISecurityPanel::UpdateMouseEvent()
{
	if (!IsVisible())
		return true;

	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;
	const float w = (float)WINDOW_W;
	const float h = (float)WindowH();

	if (!SEASON3B::CheckMouseIn(x, y, w, h))
		return true;

	if (SEASON3B::CheckMouseIn(x + w - 22.f, y + 8.f, 16.f, 16.f) && SEASON3B::IsRelease(VK_LBUTTON))
	{
		g_pNewUISystem->Hide(INTERFACE_ACCOUNT_SECURITY);
		PlayBuffer(SOUND_CLICK01);
		return false;
	}

	const float fieldW = w - 60.f;
	const float fieldX = x + 30.f;
	float fieldY = y + 88.f;

	if (m_Mode == MODE_LOCK)
	{
		// So 1 campo de senha
		if (SEASON3B::IsRelease(VK_LBUTTON) && FieldHit(fieldX, fieldY, fieldW, kFieldH))
		{
			m_Focus = 0;
			PlayBuffer(SOUND_CLICK01);
			return false;
		}

		const float btnY = y + h - 48.f;
		const float btnW = 120.f;
		if (SEASON3B::IsRelease(VK_LBUTTON) && SEASON3B::CheckMouseIn(x + 30.f, btnY, btnW, kBtnH))
		{
			DoLock();
			return false;
		}
		if (SEASON3B::IsRelease(VK_LBUTTON) && SEASON3B::CheckMouseIn(x + w - 30.f - btnW, btnY, btnW, kBtnH))
		{
			DoUnlock();
			return false;
		}
	}
	else // MODE_PASS
	{
		for (int i = 0; i < 3; i++)
		{
			if (SEASON3B::IsRelease(VK_LBUTTON) && FieldHit(fieldX, fieldY, fieldW, kFieldH))
			{
				m_Focus = i;
				PlayBuffer(SOUND_CLICK01);
				return false;
			}
			fieldY += kFieldH + 26.f;
		}

		// Captcha: caixa laranja regenera | input foca
		const float capY = fieldY;
		if (SEASON3B::IsRelease(VK_LBUTTON) && FieldHit(fieldX, capY, 70.f, kFieldH))
		{
			RefreshCaptcha();
			m_Focus = CaptchaField();
			PlayBuffer(SOUND_CLICK01);
			return false;
		}
		if (SEASON3B::IsRelease(VK_LBUTTON) && FieldHit(fieldX + 78.f, capY, fieldW - 78.f, kFieldH))
		{
			m_Focus = CaptchaField();
			PlayBuffer(SOUND_CLICK01);
			return false;
		}

		const float btnY = y + h - 48.f;
		const float bx = x + (w - 160.f) * 0.5f;
		if (SEASON3B::IsRelease(VK_LBUTTON) && SEASON3B::CheckMouseIn(bx, btnY, 160.f, kBtnH))
		{
			DoChangeAccountPass();
			return false;
		}
	}

	return false;
}

bool SEASON3B::CNewUISecurityPanel::IsOnCooldown() const
{
	return GetTickCount() < m_CooldownUntil;
}

int SEASON3B::CNewUISecurityPanel::CooldownSecLeft() const
{
	if (!IsOnCooldown())
		return 0;
	return (int)((m_CooldownUntil - GetTickCount() + 999) / 1000);
}

void SEASON3B::CNewUISecurityPanel::StartCooldown()
{
	m_CooldownUntil = GetTickCount() + (DWORD)COOLDOWN_MS;
}

bool SEASON3B::CNewUISecurityPanel::GuardCooldown()
{
	if (!IsOnCooldown())
		return false;
	char msg[64];
	sprintf_s(msg, "Aguarde %d s para usar de novo.", CooldownSecLeft());
	g_pChatListBox->AddText("", msg, SEASON3B::TYPE_ERROR_MESSAGE);
	PlayBuffer(SOUND_CLICK01);
	return true;
}

bool SEASON3B::CNewUISecurityPanel::GuardCaptcha(int fieldIdx)
{
	std::string input = m_Field[fieldIdx];
	if (!gInterface || !gInterface->check_Captcha(m_Captcha, input))
	{
		g_pChatListBox->AddText("", "Captcha incorreto. Tente novamente.", SEASON3B::TYPE_ERROR_MESSAGE);
		RefreshCaptcha();
		m_Focus = fieldIdx;
		PlayBuffer(SOUND_CLICK01);
		return true;
	}
	return false;
}

// =============================================================================
// MODE_LOCK — so Travar / Destravar itens
// =============================================================================

void SEASON3B::CNewUISecurityPanel::DoLock()
{
	if (m_Mode != MODE_LOCK)
		return;
	if (GuardCooldown())
		return;
	if (!IsValidLockPass(m_Field[0]))
	{
		g_pChatListBox->AddText("", "Digite a senha do Lock (1-7 letras ou numeros).", SEASON3B::TYPE_ERROR_MESSAGE);
		return;
	}
	SendAccountLock(ACCOUNT_LOCK_TYPE_LOCK, m_Field[0], NULL);
	StartCooldown();
	PlayBuffer(SOUND_CLICK01);
	g_pNewUISystem->Hide(INTERFACE_ACCOUNT_SECURITY);
}

void SEASON3B::CNewUISecurityPanel::DoUnlock()
{
	if (m_Mode != MODE_LOCK)
		return;
	if (GuardCooldown())
		return;
	if (!IsValidLockPass(m_Field[0]))
	{
		g_pChatListBox->AddText("", "Digite a senha do Lock para destravar.", SEASON3B::TYPE_ERROR_MESSAGE);
		return;
	}
	// Lock item: so unlock com senha do lock (sem captcha)
	SendAccountLock(ACCOUNT_LOCK_TYPE_UNLOCK, m_Field[0], NULL);
	StartCooldown();
	PlayBuffer(SOUND_CLICK01);
	g_pNewUISystem->Hide(INTERFACE_ACCOUNT_SECURITY);
}

// =============================================================================
// MODE_PASS — troca SENHA DA CONTA (login / memb__pwd)
// =============================================================================

void SEASON3B::CNewUISecurityPanel::DoChangeAccountPass()
{
	if (m_Mode != MODE_PASS)
		return;
	if (GuardCooldown())
		return;
	if (m_Field[0][0] == '\0' || m_Field[1][0] == '\0' || m_Field[2][0] == '\0' || m_Field[3][0] == '\0')
	{
		g_pChatListBox->AddText("", "Preencha todos os campos (incluindo captcha).", SEASON3B::TYPE_ERROR_MESSAGE);
		return;
	}
	if (!IsValidAccountPass(m_Field[0]) || !IsValidAccountPass(m_Field[1]) || !IsValidAccountPass(m_Field[2]))
	{
		g_pChatListBox->AddText("", "Senha da conta: 4 a 10 caracteres.", SEASON3B::TYPE_ERROR_MESSAGE);
		return;
	}
	if (strcmp(m_Field[1], m_Field[2]) != 0)
	{
		g_pChatListBox->AddText("", "Nova senha e confirmacao diferentes.", SEASON3B::TYPE_ERROR_MESSAGE);
		return;
	}
	if (strcmp(m_Field[0], m_Field[1]) == 0)
	{
		g_pChatListBox->AddText("", "A nova senha deve ser diferente da atual.", SEASON3B::TYPE_ERROR_MESSAGE);
		return;
	}
	if (GuardCaptcha(CaptchaField()))
		return;

	// Pacote proprio: troca senha da CONTA (nao e lock de itens)
	SendAccountPassChange(m_Field[0], m_Field[1]);
	StartCooldown();
	PlayBuffer(SOUND_CLICK01);
	g_pNewUISystem->Hide(INTERFACE_ACCOUNT_SECURITY);
}

// =============================================================================
// Draw
// =============================================================================

void SEASON3B::CNewUISecurityPanel::DrawFrame(const char* title)
{
	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;
	const float w = (float)WINDOW_W;
	const float h = (float)WindowH();

	RenderImageF(IMAGE_BG_SERVER_INTERFACE_250, x, y, w, h, 0.0, 0.0, 250.0, 250.0);

	const float titleW = 180.f;
	const float titleH = 24.f;
	RenderImageF(IMAGE_TITTLE_SERVER_INTERFACE,
		x + ((w - titleW) * 0.5f), y + 12.f, titleW, titleH, 0.0, 0.0, 223.0, 32.0);

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetTextColor(CLRDW_YELLOW);
	g_pRenderText->RenderText(x + ((w - titleW) * 0.5f), y + 12.f, title, (int)titleW, (int)titleH, RT3_SORT_CENTER);

	const float closeX = x + w - 22.f;
	const float closeY = y + 8.f;
	RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, closeX, closeY, 16.0f, 16.0f, 0.0, 0.0, 15.0, 15.0);
	if (SEASON3B::CheckMouseIn(closeX, closeY, 16.0f, 16.0f))
	{
		glColor4f(0.55f, 0.55f, 0.55f, 1.0f);
		RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, closeX, closeY, 16.0f, 16.0f, 0.0, 0.0, 15.0, 15.0);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void SEASON3B::CNewUISecurityPanel::DrawField(int idx, float x, float y, float w, float h, const char* label, bool mask)
{
	g_pRenderText->SetFont(g_hFont);
	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetTextColor(CLRDW_WHITE);
	g_pRenderText->RenderText(x, y - 15.f, label, (int)w, 14, RT3_SORT_LEFT);

	const bool focus = (m_Focus == idx);
	EnableAlphaTest(true);
	if (focus)
		glColor4ub(50, 55, 80, 220);
	else
		glColor4ub(20, 22, 35, 200);
	RenderColor(x, y, w, h, 0.0, 0);

	if (focus)
		glColor4ub(255, 220, 60, 255);
	else
		glColor4ub(120, 110, 80, 200);
	RenderColor(x, y, w, 1.f, 0.0, 0);
	RenderColor(x, y + h - 1.f, w, 1.f, 0.0, 0);
	RenderColor(x, y, 1.f, h, 0.0, 0);
	RenderColor(x + w - 1.f, y, 1.f, h, 0.0, 0);
	EndRenderColor();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	char show[16];
	size_t n = strlen(m_Field[idx]);
	if (mask)
	{
		for (size_t i = 0; i < n && i < 15; i++)
			show[i] = '*';
		show[n] = '\0';
	}
	else
	{
		strcpy_s(show, m_Field[idx]);
	}
	if (n == 0)
		strcpy_s(show, focus ? "_" : "");

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetTextColor(CLRDW_GOLD);
	g_pRenderText->RenderText(x + 8.f, y, show, (int)w - 16, (int)h, RT3_SORT_LEFT);
}

void SEASON3B::CNewUISecurityPanel::DrawCaptcha(float x, float y, float w, float h, int fieldIdx)
{
	g_pRenderText->SetFont(g_hFont);
	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetTextColor(CLRDW_WHITE);
	g_pRenderText->RenderText(x, y - 15.f, "Captcha: (clique no codigo para renovar)", (int)w, 14, RT3_SORT_LEFT);

	EnableAlphaTest(true);
	glColor4f(1.0f, 0.22f, 0.0f, 1.0f);
	RenderColor(x, y, 70.f, h, 0.0, 0);
	EndRenderColor();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetTextColor(CLRDW_WHITE);
	g_pRenderText->RenderText(x, y, m_Captcha.c_str(), 70, (int)h, RT3_SORT_CENTER);

	const float inX = x + 78.f;
	const float inW = w - 78.f;
	const bool focus = (m_Focus == fieldIdx);
	EnableAlphaTest(true);
	if (focus)
		glColor4ub(50, 55, 80, 220);
	else
		glColor4ub(20, 22, 35, 200);
	RenderColor(inX, y, inW, h, 0.0, 0);
	if (focus)
		glColor4ub(255, 220, 60, 255);
	else
		glColor4ub(120, 110, 80, 200);
	RenderColor(inX, y, inW, 1.f, 0.0, 0);
	RenderColor(inX, y + h - 1.f, inW, 1.f, 0.0, 0);
	RenderColor(inX, y, 1.f, h, 0.0, 0);
	RenderColor(inX + inW - 1.f, y, 1.f, h, 0.0, 0);
	EndRenderColor();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	char show[16];
	size_t n = strlen(m_Field[fieldIdx]);
	if (n == 0)
		strcpy_s(show, focus ? "_" : "");
	else
		strcpy_s(show, m_Field[fieldIdx]);

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetTextColor(CLRDW_GOLD);
	g_pRenderText->RenderText(inX + 8.f, y, show, (int)inW - 16, (int)h, RT3_SORT_LEFT);
}

void SEASON3B::CNewUISecurityPanel::DrawButton(float x, float y, float w, float h, const char* text, bool green, bool& outHover, bool enabled)
{
	outHover = enabled && SEASON3B::CheckMouseIn(x, y, w, h);
	EnableAlphaTest(true);
	if (!enabled)
		glColor4ub(70, 70, 70, 200);
	else if (green)
		glColor4ub(outHover ? 40 : 25, outHover ? 160 : 120, outHover ? 50 : 35, outHover ? 235 : 215);
	else
		glColor4ub(outHover ? 180 : 140, outHover ? 40 : 25, outHover ? 40 : 25, outHover ? 230 : 210);
	RenderColor(x, y, w, h, 0.0, 0);
	if (!enabled) glColor4ub(100, 100, 100, 255);
	else if (green) glColor4ub(90, 220, 100, 255);
	else glColor4ub(220, 80, 80, 255);
	RenderColor(x, y, w, 1.f, 0.0, 0);
	RenderColor(x, y + h - 1.f, w, 1.f, 0.0, 0);
	RenderColor(x, y, 1.f, h, 0.0, 0);
	RenderColor(x + w - 1.f, y, 1.f, h, 0.0, 0);
	EndRenderColor();
	glColor4f(1.f, 1.f, 1.f, 1.f);

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetTextColor(enabled ? (outHover ? CLRDW_YELLOW : CLRDW_WHITE) : CLRDW_GRAY);
	g_pRenderText->RenderText(x, y, text, (int)w, (int)h, RT3_SORT_CENTER);
}

bool SEASON3B::CNewUISecurityPanel::Render()
{
	if (!IsVisible())
		return true;

	EnableAlphaTest(true);
	glColor4f(1.f, 1.f, 1.f, 1.f);

	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;
	const float w = (float)WINDOW_W;
	const float h = (float)WindowH();

	// Titulos bem distintos
	const char* title = (m_Mode == MODE_LOCK) ? "Lock item" : "Senha da Conta";
	DrawFrame(title);

	g_pRenderText->SetFont(g_hFont);
	g_pRenderText->SetBgColor(0);

	if (m_Mode == MODE_LOCK)
	{
		// --- Painel TRAVAR ---
		if (g_bAccountItemsLocked)
		{
			g_pRenderText->SetTextColor(CLRDW_RED);
			g_pRenderText->RenderText(x + 20.f, y + 42.f,
				"STATUS: ITENS TRAVADOS", (int)w - 40, 14, RT3_SORT_CENTER);
		}
		else
		{
			g_pRenderText->SetTextColor(CLRDW_GREEN);
			g_pRenderText->RenderText(x + 20.f, y + 42.f,
				"STATUS: ITENS LIVRES", (int)w - 40, 14, RT3_SORT_CENTER);
		}
		g_pRenderText->SetTextColor(CLRDW_GRAY);
		g_pRenderText->RenderText(x + 20.f, y + 58.f,
			"Bloqueia mover/dropar itens. Senha 1-7 (A-Z / 0-9).", (int)w - 40, 14, RT3_SORT_CENTER);

		const float fieldW = w - 60.f;
		const float fieldX = x + 30.f;
		DrawField(0, fieldX, y + 95.f, fieldW, kFieldH, "Senha do Lock:", true);

		const float btnY = y + h - 48.f;
		bool hover = false;
		const bool canAct = !IsOnCooldown();
		char cdLabel[32];
		if (!canAct)
			sprintf_s(cdLabel, "Aguarde %ds", CooldownSecLeft());

		DrawButton(x + 30.f, btnY, 120.f, kBtnH, canAct ? "Lock item" : cdLabel, true, hover, canAct);
		DrawButton(x + w - 30.f - 120.f, btnY, 120.f, kBtnH, canAct ? "Destravar" : cdLabel, false, hover, canAct);
	}
	else
	{
		// --- Painel SENHA DA CONTA (login) ---
		g_pRenderText->SetTextColor(CLRDW_GRAY);
		g_pRenderText->RenderText(x + 20.f, y + 42.f,
			"Altera a senha de LOGIN da conta (nao e Lock item).", (int)w - 40, 14, RT3_SORT_CENTER);
		g_pRenderText->RenderText(x + 20.f, y + 56.f,
			"4 a 10 caracteres. Captcha obrigatorio.", (int)w - 40, 14, RT3_SORT_CENTER);

		const float fieldW = w - 60.f;
		const float fieldX = x + 30.f;
		float fieldY = y + 88.f;

		DrawField(0, fieldX, fieldY, fieldW, kFieldH, "Senha atual da conta:", true);
		fieldY += kFieldH + 26.f;
		DrawField(1, fieldX, fieldY, fieldW, kFieldH, "Nova senha da conta:", true);
		fieldY += kFieldH + 26.f;
		DrawField(2, fieldX, fieldY, fieldW, kFieldH, "Confirmar nova senha:", true);
		fieldY += kFieldH + 26.f;
		DrawCaptcha(fieldX, fieldY, fieldW, kFieldH, CaptchaField());

		const float btnY = y + h - 48.f;
		bool hover = false;
		const bool canAct = !IsOnCooldown();
		char cdLabel[32];
		if (!canAct)
			sprintf_s(cdLabel, "Aguarde %ds", CooldownSecLeft());

		DrawButton(x + (w - 160.f) * 0.5f, btnY, 160.f, kBtnH,
			canAct ? "Trocar Senha Conta" : cdLabel, true, hover, canAct);
	}

	if (IsOnCooldown())
	{
		g_pRenderText->SetFont(g_hFont);
		g_pRenderText->SetTextColor(CLRDW_ORANGE);
		char tip[64];
		sprintf_s(tip, "Anti-spam: espere %d segundo(s)...", CooldownSecLeft());
		g_pRenderText->RenderText(x + 20.f, y + h - 68.f, tip, (int)w - 40, 14, RT3_SORT_CENTER);
	}

	DisableAlphaBlend();
	return true;
}
