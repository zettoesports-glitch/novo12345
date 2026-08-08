#pragma once

#include <chrono>
#include <thread>
#include <windows.h>

/**
 * Professional frame pacer (single source of truth for FPS limiting).
 *
 * Design:
 *  - QPC absolute schedule (next-frame deadline), not "sleep leftover from last now"
 *  - Coarse sleep + fine spin for stable pacing at 60/120+
 *  - Optional unlimited mode (target <= 0)
 *  - VSync is external; when VSync is on, software cap still works if target < refresh
 *  - Game time / animation normalization stays in steady_clock (REFERENCE_FPS=25)
 */
class CFPSController
{
public:
	enum class PaceMode
	{
		Unlimited = 0,  // no software cap
		Capped    = 1,  // software frame pacer (default, professional)
	};

	static CFPSController* Instance();

	bool Initialize();
	void Shutdown();

	// Call once per main-loop iteration (only limiter that sleeps)
	void BeginFrame();
	void EndFrame();

	// Target FPS. Values <= 0 => unlimited.
	void SetTargetFPS(double fps);
	double GetTargetFPS() const { return m_TargetFPS; }

	void SetPaceMode(PaceMode mode);
	PaceMode GetPaceMode() const { return m_PaceMode; }
	void SetLimitingEnabled(bool enabled);

	// Apply radio index 0..4 used by Config dialog / registry
	// 0=120, 1=30, 2=40, 3=60, 4=120
	void ApplyFpsIndex(int index);
	static int MapFpsIndexToLimit(int index);

	double GetCurrentFPS() const { return m_CurrentFPS; }
	double GetAverageFPS() const { return m_AverageFPS; }
	double GetFrameTime() const { return m_FrameTimeMs; }
	double GetTargetFrameMs() const;

	void SetFPSOverride(bool active, double overrideFps = 0.0);
	bool IsFPSOverrideActive() const { return m_FPSOverrideActive; }

	// Legacy flag kept for compatibility (routes to simple Sleep path if true)
	void SetUseOriginal150FPSPipeline(bool use) { m_UseOriginal150FPSPipeline = use; }
	bool GetUseOriginal150FPSPipeline() const { return m_UseOriginal150FPSPipeline; }

	// Raise Windows timer resolution for tighter sleep (1ms)
	void SetHighPrecisionTimer(bool enable);

	virtual ~CFPSController();

private:
	CFPSController();
	static CFPSController* m_pInstance;

	void ApplyFrameLimiting();
	double NowSeconds() const;
	void UpdateFpsStats(double frameSeconds);

	bool m_bInitialized;
	bool m_LimitingEnabled;
	PaceMode m_PaceMode;

	double m_TargetFPS;       // <= 0 = unlimited
	double m_CurrentFPS;
	double m_AverageFPS;
	double m_FrameTimeMs;

	bool m_FPSOverrideActive;
	double m_FPSOverrideMs;
	bool m_UseOriginal150FPSPipeline;
	bool m_HighPrecisionTimer;

	// Absolute schedule (seconds, QPC domain via chrono steady)
	double m_NextFrameDeadlineSec;
	double m_LastFrameEndSec;
	bool m_HasDeadline;

	// EMA for average FPS
	double m_FpsEma;

	std::chrono::steady_clock::time_point m_FrameStartTime;
	std::chrono::steady_clock::time_point m_ClockOrigin;
	LARGE_INTEGER m_QpcFreq;
	bool m_UseQpc;
};

extern int g_FPSOverrideActive;
extern double g_FPSOverrideMs;
extern bool g_UseOriginal150FPSPipeline;

#define gFPSController (CFPSController::Instance())
