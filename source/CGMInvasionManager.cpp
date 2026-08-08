#include "stdafx.h"
#include "jpexs.h"
#include "UIBaseDef.h"
#include "UIControls.h"
#include "CGMInvasionManager.h"
#include "NewUISystem.h"
#include "CustomEventTime.h"
#include "CBInterface.h"
#include "DSPlaySound.h"

extern int FontHeight;
extern char* getMonsterName(int type);

namespace
{
	// Urgency colors for remaining time
	// > 5 min  yellow | 2-5 min orange | < 2 min red
	DWORD GetInvasionTimeColor(DWORD remainSec)
	{
		if (remainSec <= 120)
			return CLRDW_RED;
		if (remainSec <= 300)
			return CLRDW_ORANGE;
		return CLRDW_YELLOW;
	}

	void FormatRemainTime(DWORD remainSec, char* out, size_t outSize)
	{
		int totalseconds = (int)remainSec;
		if (totalseconds < 0)
			totalseconds = 0;

		int hours = totalseconds / 3600;
		int minutes = (totalseconds / 60) % 60;
		int seconds = totalseconds % 60;

		if (hours > 0)
			sprintf_s(out, outSize, "%02d:%02d:%02d", hours, minutes, seconds);
		else
			sprintf_s(out, outSize, "%02d:%02d", minutes, seconds);
	}

	// Soft blink when under 30s (hide text on alternate half-second)
	bool ShouldBlinkTime(DWORD remainSec)
	{
		if (remainSec > 30 || remainSec == 0)
			return false;

		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
		return ((ms / 400) % 2) == 0;
	}
}

SEASON3B::CGMInvasionManager::CGMInvasionManager()
{
	m_CountActive = 0;
	is_Opentable = false;
	currentInvasion = -1;
	last_time = std::chrono::steady_clock::now();

	m_Pos.x = 0;
	m_Pos.y = 0;
	m_RenderFrameX = 0;
	m_RenderFrameY = 0;

	InvasionInfo.clear();
}

SEASON3B::CGMInvasionManager::~CGMInvasionManager()
{
	for (type_map_invasion::iterator it = InvasionInfo.begin(); it != InvasionInfo.end(); ++it)
	{
		it->second.total_monster.clear();
	}

	InvasionInfo.clear();
}

void SEASON3B::CGMInvasionManager::SetInvasion(int Index, DWORD TimeRemaing, char* Name)
{
	if (Index != -1 && Index >= 0 && Index < MAX_INVASION)
	{
		currentInvasion = Index;
		INVASION_GLOBAL_INFO& invasion = InvasionInfo[Index];

		invasion.index = Index;
		if (Name != NULL)
			invasion.Name = Name;
		else
			invasion.Name.clear();
		invasion.TimeRemaing = TimeRemaing;
		invasion.total_monster.clear();
	}
}

void SEASON3B::CGMInvasionManager::SetMonsterKill(int Index, int MonsterIndex, int Monster_Kill, int MonsterCount)
{
	if (Index != -1 && Index >= 0 && Index < MAX_INVASION)
	{
		INVASION_GLOBAL_INFO* invasion = FindInvasion(Index);

		if (invasion != NULL)
		{
			size_t length = invasion->total_monster.size();

			for (size_t i = 0; i < length; i++)
			{
				INVASION_MONSTER_INFO* info = &invasion->total_monster[i];

				if (info->MonsterIndex == MonsterIndex)
				{
					info->Monster_Kill = Monster_Kill;
					info->MonsterCount = MonsterCount;
				}
			}
		}
	}
}

void SEASON3B::CGMInvasionManager::SetMonsterCount(int Index, int MonsterIndex, int Monster_Kill, int MonsterCount)
{
	if (Index != -1 && Index >= 0 && Index < MAX_INVASION)
	{
		INVASION_GLOBAL_INFO* invasion = FindInvasion(Index);

		if (invasion != NULL)
		{
			invasion->total_monster.push_back({ MonsterIndex, Monster_Kill, MonsterCount });
		}
	}
}

void SEASON3B::CGMInvasionManager::Update()
{
	auto current_time = std::chrono::steady_clock::now();
	double difTime = std::chrono::duration<double>(current_time - last_time).count();

	if (difTime >= 1.0)
	{
		m_CountActive = 0;
		last_time = current_time;

		for (type_map_invasion::iterator it = InvasionInfo.begin(); it != InvasionInfo.end(); ++it)
		{
			INVASION_GLOBAL_INFO* invasion = &it->second;

			if (invasion->TimeRemaing > 0)
			{
				m_CountActive++;
				invasion->TimeRemaing -= 1;

				if (currentInvasion == -1)
				{
					currentInvasion = it->first;
				}
			}
			else
			{
				if (currentInvasion == it->first)
				{
					currentInvasion = -1;
				}
			}
		}

		// Pick first active if current expired
		if (currentInvasion == -1 && m_CountActive > 0)
		{
			for (type_map_invasion::iterator it = InvasionInfo.begin(); it != InvasionInfo.end(); ++it)
			{
				if (it->second.TimeRemaing > 0)
				{
					currentInvasion = it->first;
					break;
				}
			}
		}
	}
}

float SEASON3B::CGMInvasionManager::GetHeaderHeight() const
{
	// Name line + time line
	return (float)((FontHeight + 2) * 2);
}

float SEASON3B::CGMInvasionManager::GetPanelHeight(const INVASION_GLOBAL_INFO* Invasion) const
{
	float h = GetHeaderHeight();

	if (is_Opentable && Invasion != NULL && Invasion->TimeRemaing > 0)
	{
		// monster lines (no extra "Time:" row — time is already in header)
		size_t length = Invasion->total_monster.size();
		if (length == 0)
			length = 1; // room for empty hint
		h += 6.f * g_fScreenRate_y; // gap
		h += (float)((FontHeight + 2) * (int)length);
	}

	return h;
}

void SEASON3B::CGMInvasionManager::SetPos(float RenderFrameX, float RenderFrameY)
{
	const float RenderSizeX = 180.0f;

	m_RenderFrameX = RenderFrameX - (RenderSizeX / 2.f);
	m_RenderFrameY = RenderFrameY;
	m_Pos.x = (LONG)m_RenderFrameX;
	m_Pos.y = (LONG)m_RenderFrameY;
}

bool SEASON3B::CGMInvasionManager::UpdateMouseEvent()
{
	const float RenderSizeX = 180.0f;

	if ((m_RenderFrameX + RenderSizeX) > GetScreenWidth()
		|| g_pNewUISystem->IsVisible(INTERFACE_MINI_MAP)
		|| g_pNewUISystem->IsVisible(INTERFACE_INGAMESHOP)
		|| g_pNewUISystem->IsVisible(INTERFACE_MASTER_LEVEL)
		)
	{
		return false;
	}

	if (m_CountActive <= 0)
		return false;

	INVASION_GLOBAL_INFO* Invasion = this->FindInvasion(currentInvasion);
	float headerH = GetHeaderHeight() / g_fScreenRate_y;
	float panelH = GetPanelHeight(Invasion) / g_fScreenRate_y;

	// Hit area covers header always; expanded panel when open
	float hitH = is_Opentable ? panelH : headerH;

	if (!SEASON3B::CheckMouseIn(m_RenderFrameX, m_RenderFrameY, RenderSizeX, hitH))
		return false;

	if (SEASON3B::IsRelease(VK_LBUTTON))
	{
		// Prev / Next only on header strip
		if (m_CountActive > 1 && SEASON3B::CheckMouseIn(m_RenderFrameX + 3.0f, m_RenderFrameY + (headerH - 18.f) / 2.f, 17.f, 18.f))
		{
			this->PreviousInvasion();
		}
		else if (m_CountActive > 1 && SEASON3B::CheckMouseIn(m_RenderFrameX + RenderSizeX - 20.0f, m_RenderFrameY + (headerH - 18.f) / 2.f, 17.f, 18.f))
		{
			this->NextInvasion();
		}
		else
		{
			// Clique no banner (nome + tempo) → abre Eventos (ideia #7)
			// Se Eventos ja estiver aberto, alterna lista de kills da invasao
			if (g_CustomEventTime && !gInterface->Data[eWindowEventTime].OnShow)
			{
				g_CustomEventTime->ClearCustomEventTime();
				g_CustomEventTime->OpenTestWindow(0);
				PlayBuffer(SOUND_CLICK01);
			}
			else
			{
				is_Opentable = !is_Opentable;
				PlayBuffer(SOUND_CLICK01);
			}
		}
	}

	return true;
}

void SEASON3B::CGMInvasionManager::RenderFrame(float RenderFrameX, float RenderFrameY)
{
	const float RenderSizeX = 180.0f;

	// Keep centered from caller each frame
	SetPos(RenderFrameX, RenderFrameY);

	if ((m_RenderFrameX + RenderSizeX) > GetScreenWidth()
		|| g_pNewUISystem->IsVisible(INTERFACE_MINI_MAP)
		|| g_pNewUISystem->IsVisible(INTERFACE_INGAMESHOP)
		|| g_pNewUISystem->IsVisible(INTERFACE_MASTER_LEVEL)
		)
	{
		return;
	}

	if (m_CountActive <= 0)
		return;

	INVASION_GLOBAL_INFO* Invasion = this->FindInvasion(currentInvasion);
	if (Invasion == NULL || Invasion->TimeRemaing == 0)
		return;

	float _ScreenX = (m_RenderFrameX * g_fScreenRate_x);
	float _ScreenY = (m_RenderFrameY * g_fScreenRate_y);

	float headerH = GetHeaderHeight();
	float lineH = (float)(FontHeight + 2);

	// ---- Header background (always: name + time) ----
	jpexs::RenderBoxjpexs_dds(IMAGE_HUD_PLAYER, _ScreenX, _ScreenY, RenderSizeX * g_fScreenRate_x, headerH);

	DWORD timeColor = GetInvasionTimeColor(Invasion->TimeRemaing);
	bool blink = ShouldBlinkTime(Invasion->TimeRemaing);

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0);

	// Event name (from MuServer InvasionManager.dat) — always visible
	const char* eventName = (Invasion->Name.empty() == false) ? Invasion->Name.c_str() : "Invasion";
	g_pRenderText->SetTextColor(timeColor);
	g_pRenderText->RenderTextClipped(_ScreenX, _ScreenY, eventName, (RenderSizeX * g_fScreenRate_x), lineH, RT3_SORT_CENTER);

	// Remaining time under name (blink only under 30s)
	char pszTime[32];
	FormatRemainTime(Invasion->TimeRemaing, pszTime, sizeof(pszTime));
	if (!blink)
	{
		g_pRenderText->SetTextColor(timeColor);
		g_pRenderText->RenderTextClipped(_ScreenX, _ScreenY + lineH, pszTime, (RenderSizeX * g_fScreenRate_x), lineH, RT3_SORT_CENTER);
	}

	// Prev / Next when multiple active invasions
	if (m_CountActive > 1)
	{
		float headerHy = headerH / g_fScreenRate_y;
		RenderlookFetch(IMAGE_ND_BTN_L, m_RenderFrameX + 3.0f, m_RenderFrameY + (headerHy - 18.f) / 2.f, true);
		RenderlookFetch(IMAGE_ND_BTN_R, m_RenderFrameX + RenderSizeX - 20.0f, m_RenderFrameY + (headerHy - 18.f) / 2.f, true);
	}

	// ---- Expanded: kill list only (on click) ----
	if (is_Opentable)
	{
		size_t length = Invasion->total_monster.size();
		size_t drawLines = (length > 0) ? length : 1;
		float gap = 6.f * g_fScreenRate_y;
		float bodyH = (float)((FontHeight + 2) * (int)drawLines);

		float bodyY = _ScreenY + headerH + gap;
		jpexs::RenderBoxjpexs_dds(IMAGE_HUD_PLAYER, _ScreenX, bodyY, (RenderSizeX * g_fScreenRate_x), bodyH);

		g_pRenderText->SetFont(g_hFont);
		g_pRenderText->SetTextColor(CLRDW_WHITE3);

		float textX = _ScreenX + 6.f * g_fScreenRate_x;
		float textY = bodyY;
		float rowH = (float)(FontHeight + 2);

		if (length == 0)
		{
			g_pRenderText->RenderTextClipped(textX, textY, "- (no data)", 0, rowH, RT3_SORT_LEFT);
		}
		else
		{
			char pszText[128];
			for (size_t i = 0; i < length; i++)
			{
				INVASION_MONSTER_INFO* Data = &Invasion->total_monster[i];
				const char* monName = getMonsterName(Data->MonsterIndex);
				if (monName == NULL || monName[0] == '\0')
					monName = "Monster";

				sprintf_s(pszText, "- %s (%d / %d)", monName, Data->Monster_Kill, Data->MonsterCount);
				g_pRenderText->RenderTextClipped(textX, textY, pszText, 0, rowH, RT3_SORT_LEFT);
				textY += rowH;
			}
		}
	}
}

void SEASON3B::CGMInvasionManager::NextInvasion()
{
	auto it = InvasionInfo.find(currentInvasion);

LABEL_REPEAT:
	if (it == InvasionInfo.end() || std::next(it) == InvasionInfo.end())
	{
		return;
	}

	++it;

	if (it->second.TimeRemaing == 0)
	{
		goto LABEL_REPEAT;
	}

	currentInvasion = it->first;
}

void SEASON3B::CGMInvasionManager::PreviousInvasion()
{
	auto it = InvasionInfo.find(currentInvasion);

LABEL_REPEAT:
	if (it == InvasionInfo.end() || it == InvasionInfo.begin())
	{
		return;
	}

	--it;

	if (it->second.TimeRemaing == 0)
	{
		goto LABEL_REPEAT;
	}

	currentInvasion = it->first;
}

INVASION_GLOBAL_INFO* SEASON3B::CGMInvasionManager::FindInvasion(int Index)
{
	auto it = InvasionInfo.find(Index);

	if (it != InvasionInfo.end())
	{
		return &(it->second);
	}

	return NULL;
}
