#pragma once

#include "ZzzBMD.h"
#include "ZzzCharacter.h"

// GM spectator: camera follows a target character while the GM body is
// ghost-synced by the GameServer (/watch + hide).
class CSpectatorMode
{
public:
	CSpectatorMode();

	void Start(const char* targetName, WORD targetIndex, BYTE map, BYTE x, BYTE y);
	void Stop();
	bool IsActive() const { return m_Active; }

	// Only blocks movement (MoveHero). Chat / commands / hotkeys still work.
	bool IsInputBlocked() const { return m_Active; }

	// Call every frame in MAIN_SCENE: camera focus + hotkeys (ESC / END = unwatch).
	void Update();

	// Replace camera focus origin (world position) with lerped target.
	void ApplyCameraFocus(vec3_t& focusPosition);

	const char* GetTargetName() const { return m_TargetName; }

private:
	CHARACTER* FindTarget() const;
	static void TrimName(char* dst, size_t dstSize, const char* src);
	static bool NamesEqual(const char* a, const char* b);
	void RequestUnwatch();

	bool m_Active;
	bool m_FocusInit;
	bool m_UnwatchSent;
	WORD m_TargetIndex;
	char m_TargetName[11];
	vec3_t m_Focus;
	float m_Lerp;
};

extern CSpectatorMode gSpectatorMode;
