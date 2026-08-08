// EffectBudget.h
// Options-driven spawn/sim budget for client FX pools.
// Problem: GetRenderEffect() historically only skipped *Render* paths while
// Create* + Move* still filled MAX_PARTICLES / MAX_JOINTS / etc. (CPU waste).
// This header is the single gate for Create + Move early-out + soft caps.
//
#pragma once

#include "NewUISystem.h"
#include "_define.h"

namespace EffectBudget
{
	// True when skill/world FX are allowed to spawn and simulate.
	// Null option window (boot) → allow (login/menu FX must still work).
	inline bool Enabled()
	{
		if (!g_pOption)
			return true;
		return g_pOption->GetRenderEffect() != false;
	}

	// Soft caps while effects are ON (Low/Med/High or custom via RenderLevel).
	// High keeps engine maxima. Medium cuts skill spam. Low+manual FX = tiny budget.
	inline int CapParticles()
	{
		if (!Enabled())
			return 0;
		if (!g_pOption)
			return MAX_PARTICLES;

		switch (g_pOption->GetGraphicQuality())
		{
		case 0: return 500;              // Low + effects re-enabled manually
		case 1: return 1200;             // Medium
		case 2: return MAX_PARTICLES;    // High (3000)
		default:
		{
			// Custom: Effect slider (RenderLevel 0..4) now also limits density
			int lv = g_pOption->GetRenderLevel();
			if (lv < 0) lv = 0;
			if (lv > 4) lv = 4;
			static const int kCap[5] = { 600, 1000, 1500, 2200, MAX_PARTICLES };
			return kCap[lv];
		}
		}
	}

	inline int CapJoints()
	{
		if (!Enabled())
			return 0;
		if (!g_pOption)
			return MAX_JOINTS;

		switch (g_pOption->GetGraphicQuality())
		{
		case 0: return 80;
		case 1: return 200;
		case 2: return MAX_JOINTS; // 500
		default:
		{
			int lv = g_pOption->GetRenderLevel();
			if (lv < 0) lv = 0;
			if (lv > 4) lv = 4;
			static const int kCap[5] = { 100, 160, 240, 360, MAX_JOINTS };
			return kCap[lv];
		}
		}
	}

	inline int CapEffects()
	{
		if (!Enabled())
			return 0;
		if (!g_pOption)
			return MAX_EFFECTS;

		switch (g_pOption->GetGraphicQuality())
		{
		case 0: return 40;
		case 1: return 90;
		case 2: return MAX_EFFECTS; // 200
		default:
		{
			int lv = g_pOption->GetRenderLevel();
			if (lv < 0) lv = 0;
			if (lv > 4) lv = 4;
			static const int kCap[5] = { 50, 80, 120, 160, MAX_EFFECTS };
			return kCap[lv];
		}
		}
	}

	inline int CapSprites()
	{
		if (!Enabled())
			return 0;
		if (!g_pOption)
			return MAX_SPRITES;

		switch (g_pOption->GetGraphicQuality())
		{
		case 0: return 150;
		case 1: return 400;
		case 2: return MAX_SPRITES; // 1000
		default:
		{
			int lv = g_pOption->GetRenderLevel();
			if (lv < 0) lv = 0;
			if (lv > 4) lv = 4;
			static const int kCap[5] = { 200, 350, 550, 800, MAX_SPRITES };
			return kCap[lv];
		}
		}
	}

	// Count live entries; return true if at/over cap (caller should refuse spawn).
	template<typename TLiveArray>
	inline bool IsFull(const TLiveArray* pool, int poolSize, int cap)
	{
		if (cap <= 0)
			return true;
		if (cap >= poolSize)
			return false;

		int live = 0;
		for (int i = 0; i < poolSize; ++i)
		{
			if (pool[i].Live)
			{
				if (++live >= cap)
					return true;
			}
		}
		return false;
	}

	template<typename TLiveArray>
	inline void ClearAll(TLiveArray* pool, int poolSize)
	{
		for (int i = 0; i < poolSize; ++i)
			pool[i].Live = false;
	}
}
