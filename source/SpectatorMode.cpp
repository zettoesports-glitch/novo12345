#include "stdafx.h"
#include "SpectatorMode.h"
#include "NewUISystem.h"
#include "ZzzOpenglUtil.h"
#include "ZzzLodTerrain.h"
#include "CGMCharacter.h"
#include "wsclientinline.h"

CSpectatorMode gSpectatorMode;

CSpectatorMode::CSpectatorMode()
{
	m_Active = false;
	m_FocusInit = false;
	m_UnwatchSent = false;
	m_TargetIndex = 0;
	// Snappier follow so camera does not lag behind moving players
	m_Lerp = 0.35f;
	memset(m_TargetName, 0, sizeof(m_TargetName));
	Vector(0.f, 0.f, 0.f, m_Focus);
}

void CSpectatorMode::TrimName(char* dst, size_t dstSize, const char* src)
{
	if (dst == NULL || dstSize == 0)
		return;

	dst[0] = 0;
	if (src == NULL)
		return;

	while (*src == ' ' || *src == '\t')
		++src;

	size_t n = 0;
	while (src[n] != 0 && n + 1 < dstSize && n < 10)
	{
		dst[n] = src[n];
		++n;
	}
	dst[n] = 0;

	while (n > 0 && (dst[n - 1] == ' ' || dst[n - 1] == '\t'))
	{
		dst[--n] = 0;
	}
}

bool CSpectatorMode::NamesEqual(const char* a, const char* b)
{
	char ta[16];
	char tb[16];
	TrimName(ta, sizeof(ta), a);
	TrimName(tb, sizeof(tb), b);
	return _stricmp(ta, tb) == 0;
}

void CSpectatorMode::Start(const char* targetName, WORD targetIndex, BYTE map, BYTE x, BYTE y)
{
	m_Active = true;
	m_FocusInit = false;
	m_UnwatchSent = false;
	m_TargetIndex = targetIndex;
	memset(m_TargetName, 0, sizeof(m_TargetName));

	if (targetName != NULL)
	{
		TrimName(m_TargetName, sizeof(m_TargetName), targetName);
	}

	// Bootstrap focus near target tile until character object is available
	m_Focus[0] = (float)x * TERRAIN_SCALE + TERRAIN_SCALE * 0.5f;
	m_Focus[1] = (float)y * TERRAIN_SCALE + TERRAIN_SCALE * 0.5f;
	m_Focus[2] = RequestTerrainHeight(m_Focus[0], m_Focus[1]);
	m_FocusInit = true;

	if (g_pChatListBox)
	{
		char msg[160];
		sprintf_s(msg, "[Watch] ON: %s | /unwatch or ESC/END to stop", m_TargetName[0] ? m_TargetName : "?");
		g_pChatListBox->AddText("", msg, SEASON3B::TYPE_SYSTEM_MESSAGE);
	}
}

void CSpectatorMode::Stop()
{
	if (!m_Active)
	{
		return;
	}

	m_Active = false;
	m_FocusInit = false;
	m_UnwatchSent = false;
	m_TargetIndex = 0;
	memset(m_TargetName, 0, sizeof(m_TargetName));

	if (g_pChatListBox)
	{
		g_pChatListBox->AddText("", "[Watch] Spectator OFF", SEASON3B::TYPE_SYSTEM_MESSAGE);
	}
}

void CSpectatorMode::RequestUnwatch()
{
	if (!m_Active || m_UnwatchSent)
		return;

	m_UnwatchSent = true;
	// Free camera/input immediately; also tell server to stop body-follow
	SendChat("/unwatch");
	Stop();
}

CHARACTER* CSpectatorMode::FindTarget() const
{
	if (!m_Active || gmCharacters == NULL)
	{
		return NULL;
	}

	// 1) By name (trim spaces — client IDs are often space-padded)
	if (m_TargetName[0] != 0)
	{
		for (int i = 0; i < MAX_CHARACTERS_CLIENT; i++)
		{
			CHARACTER* c = gmCharacters->GetCharacter(i);
			if (c == NULL || !c->Object.Live)
				continue;
			if (NamesEqual(m_TargetName, c->ID))
				return c;
		}
	}

	// 2) By viewport Key (server object index)
	if (m_TargetIndex != 0)
	{
		for (int i = 0; i < MAX_CHARACTERS_CLIENT; i++)
		{
			CHARACTER* c = gmCharacters->GetCharacter(i);
			if (c != NULL && c->Object.Live && c->Key == m_TargetIndex)
				return c;
		}
	}

	return NULL;
}

void CSpectatorMode::Update()
{
	if (!m_Active)
		return;

	// Hotkeys to leave (movement is blocked; chat may be hard to open in combat UI)
	// END = primary, ESC = secondary when no modal UI focus
	if (HIBYTE(GetAsyncKeyState(VK_END)) || HIBYTE(GetAsyncKeyState(VK_ESCAPE)))
	{
		// Debounce: only fire once until server confirms Stop
		static DWORD s_lastHotkey = 0;
		DWORD now = GetTickCount();
		if (now - s_lastHotkey > 800)
		{
			s_lastHotkey = now;
			RequestUnwatch();
		}
	}
}

void CSpectatorMode::ApplyCameraFocus(vec3_t& focusPosition)
{
	if (!m_Active)
	{
		return;
	}

	Update();

	CHARACTER* target = FindTarget();
	const float* tp = NULL;

	if (target != NULL)
	{
		tp = target->Object.Position;
	}
	else if (Hero != NULL && Hero->Object.Live)
	{
		// Server soft-follows the GM body to the target (CommandWatchProc).
		// If the target is not yet in our character list, still move the camera with Hero.
		tp = Hero->Object.Position;
	}

	if (tp != NULL)
	{
		// Frame-rate independent lerp
		float t = m_Lerp;
		if (t < 0.05f) t = 0.05f;
		if (t > 1.0f) t = 1.0f;

		// Prefer faster catch-up when far (avoids "stuck at cast point")
		float dx = tp[0] - m_Focus[0];
		float dy = tp[1] - m_Focus[1];
		float dist2 = dx * dx + dy * dy;
		if (dist2 > (400.f * 400.f))
			t = 1.0f; // snap if very far
		else if (dist2 > (150.f * 150.f))
			t = 0.55f;

		if (!m_FocusInit)
		{
			VectorCopy(tp, m_Focus);
			m_FocusInit = true;
		}
		else
		{
			m_Focus[0] += (tp[0] - m_Focus[0]) * t;
			m_Focus[1] += (tp[1] - m_Focus[1]) * t;
			m_Focus[2] += (tp[2] - m_Focus[2]) * t;
		}
	}
	else if (!m_FocusInit)
	{
		m_FocusInit = true;
	}

	if (m_FocusInit)
	{
		VectorCopy(m_Focus, focusPosition);
	}
}
