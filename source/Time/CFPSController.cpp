#include "stdafx.h"
#include "Time/CFPSController.h"

// Global FPS control variables (legacy hooks)
int g_FPSOverrideActive = 0;
double g_FPSOverrideMs = 0.0;
bool g_UseOriginal150FPSPipeline = false;

CFPSController* CFPSController::m_pInstance = nullptr;

// Spin window: sleep until this many ms before deadline, then busy-wait.
// Too large => wastes CPU; too small => Windows sleep overshoot.
static constexpr double kSpinReserveMs = 1.25;
static constexpr double kMinSleepMs = 0.75;
static constexpr double kMaxCatchUpFrames = 2.0; // prevent spiral of death

CFPSController::CFPSController()
	: m_bInitialized(false)
	, m_LimitingEnabled(true)
	, m_PaceMode(PaceMode::Capped)
	, m_TargetFPS(120.0)
	, m_CurrentFPS(0.0)
	, m_AverageFPS(0.0)
	, m_FrameTimeMs(0.0)
	, m_FPSOverrideActive(false)
	, m_FPSOverrideMs(0.0)
	, m_UseOriginal150FPSPipeline(false)
	, m_HighPrecisionTimer(false)
	, m_NextFrameDeadlineSec(0.0)
	, m_LastFrameEndSec(0.0)
	, m_HasDeadline(false)
	, m_FpsEma(0.0)
	, m_UseQpc(false)
{
	m_QpcFreq.QuadPart = 0;
}

CFPSController::~CFPSController()
{
	Shutdown();
}

CFPSController* CFPSController::Instance()
{
	if (!m_pInstance)
		m_pInstance = new CFPSController();
	return m_pInstance;
}

bool CFPSController::Initialize()
{
	if (m_bInitialized)
		return true;

	m_UseQpc = (QueryPerformanceFrequency(&m_QpcFreq) != 0 && m_QpcFreq.QuadPart > 0);
	m_ClockOrigin = std::chrono::steady_clock::now();
	m_FrameStartTime = m_ClockOrigin;
	m_LastFrameEndSec = NowSeconds();
	m_NextFrameDeadlineSec = m_LastFrameEndSec;
	m_HasDeadline = false;
	m_FpsEma = 0.0;

	// 1ms timer resolution — critical for 60/120 pacing on Windows
	SetHighPrecisionTimer(true);

	m_bInitialized = true;
	return true;
}

void CFPSController::Shutdown()
{
	if (m_HighPrecisionTimer)
	{
		timeEndPeriod(1);
		m_HighPrecisionTimer = false;
	}
	m_bInitialized = false;
	m_HasDeadline = false;
}

void CFPSController::SetHighPrecisionTimer(bool enable)
{
	if (enable && !m_HighPrecisionTimer)
	{
		if (timeBeginPeriod(1) == TIMERR_NOERROR)
			m_HighPrecisionTimer = true;
	}
	else if (!enable && m_HighPrecisionTimer)
	{
		timeEndPeriod(1);
		m_HighPrecisionTimer = false;
	}
}

double CFPSController::NowSeconds() const
{
	if (m_UseQpc)
	{
		LARGE_INTEGER c;
		QueryPerformanceCounter(&c);
		return static_cast<double>(c.QuadPart) / static_cast<double>(m_QpcFreq.QuadPart);
	}

	using clock = std::chrono::steady_clock;
	return std::chrono::duration<double>(clock::now() - m_ClockOrigin).count();
}

double CFPSController::GetTargetFrameMs() const
{
	double fps = m_TargetFPS;
	if (m_FPSOverrideActive && m_FPSOverrideMs > 0.0)
		return m_FPSOverrideMs;
	if (fps <= 0.0)
		return 0.0;
	return 1000.0 / fps;
}

int CFPSController::MapFpsIndexToLimit(int index)
{
	switch (index)
	{
	case 1: return 30;
	case 2: return 40;
	case 3: return 60;
	case 4: return 120;
	case 0:
	default: return 120;
	}
}

void CFPSController::ApplyFpsIndex(int index)
{
	const int limit = MapFpsIndexToLimit(index);
	SetTargetFPS(static_cast<double>(limit));
	SetPaceMode(PaceMode::Capped);
	SetLimitingEnabled(true);
}

void CFPSController::SetPaceMode(PaceMode mode)
{
	m_PaceMode = mode;
	if (mode == PaceMode::Unlimited)
	{
		m_TargetFPS = 0.0;
		m_HasDeadline = false;
	}
}

void CFPSController::SetLimitingEnabled(bool enabled)
{
	m_LimitingEnabled = enabled;
	if (!enabled)
		m_HasDeadline = false;
}

void CFPSController::SetTargetFPS(double fps)
{
	if (fps <= 0.0)
	{
		m_TargetFPS = 0.0;
		m_PaceMode = PaceMode::Unlimited;
		m_HasDeadline = false;
		return;
	}

	// Clamp to sane range for MU clients
	if (fps < 20.0) fps = 20.0;
	if (fps > 300.0) fps = 300.0;

	m_TargetFPS = fps;
	m_PaceMode = PaceMode::Capped;
	// Resync deadline so we don't burst after a large change
	m_HasDeadline = false;
}

void CFPSController::SetFPSOverride(bool active, double overrideFps)
{
	m_FPSOverrideActive = active;
	g_FPSOverrideActive = active ? 1 : 0;

	if (active && overrideFps > 0.0)
	{
		m_FPSOverrideMs = 1000.0 / overrideFps;
		g_FPSOverrideMs = m_FPSOverrideMs;
	}
	else
	{
		m_FPSOverrideMs = 0.0;
		g_FPSOverrideMs = 0.0;
	}
	m_HasDeadline = false;
}

void CFPSController::BeginFrame()
{
	if (!m_bInitialized)
		return;
	m_FrameStartTime = std::chrono::steady_clock::now();
}

void CFPSController::UpdateFpsStats(double frameSeconds)
{
	if (frameSeconds <= 0.0)
		frameSeconds = 0.000001;

	m_FrameTimeMs = frameSeconds * 1000.0;
	m_CurrentFPS = 1.0 / frameSeconds;

	// Exponential moving average (~0.5s feel at 60fps)
	const double alpha = 0.08;
	if (m_FpsEma <= 0.0)
		m_FpsEma = m_CurrentFPS;
	else
		m_FpsEma = m_FpsEma * (1.0 - alpha) + m_CurrentFPS * alpha;
	m_AverageFPS = m_FpsEma;
}

void CFPSController::EndFrame()
{
	if (!m_bInitialized)
		return;

	// Pace first (absolute deadline), then sample wall time for FPS stats.
	ApplyFrameLimiting();

	const double now = NowSeconds();
	double frameSec = now - m_LastFrameEndSec;
	if (frameSec <= 0.0)
	{
		const double targetMs = GetTargetFrameMs();
		frameSec = (targetMs > 0.0) ? (targetMs * 0.001) : 0.001;
	}
	m_LastFrameEndSec = now;

	UpdateFpsStats(frameSec);
}

void CFPSController::ApplyFrameLimiting()
{
	if (!m_bInitialized || !m_LimitingEnabled)
		return;
	if (m_PaceMode == PaceMode::Unlimited)
		return;

	double periodSec = 0.0;
	if (m_FPSOverrideActive && m_FPSOverrideMs > 0.0)
		periodSec = m_FPSOverrideMs * 0.001;
	else if (m_TargetFPS > 0.0)
		periodSec = 1.0 / m_TargetFPS;
	else
		return;

	if (periodSec <= 0.0)
		return;

	const double now = NowSeconds();

	if (!m_HasDeadline)
	{
		m_NextFrameDeadlineSec = now + periodSec;
		m_HasDeadline = true;
		return; // first frame: establish schedule, don't over-sleep
	}

	// If we fell behind by more than N frames, resync (avoid death spiral)
	if (now > m_NextFrameDeadlineSec + periodSec * kMaxCatchUpFrames)
	{
		m_NextFrameDeadlineSec = now + periodSec;
		return;
	}

	double remainingSec = m_NextFrameDeadlineSec - now;
	if (remainingSec > 0.0)
	{
		if (m_UseOriginal150FPSPipeline)
		{
			// Legacy coarse sleep only
			const DWORD ms = static_cast<DWORD>((remainingSec * 1000.0));
			if (ms > 0)
				Sleep(ms);
		}
		else
		{
			// Professional: coarse sleep, then fine spin to deadline
			const double remainingMs = remainingSec * 1000.0;
			const double sleepMs = remainingMs - kSpinReserveMs;

			if (sleepMs >= kMinSleepMs)
			{
				// sleep_for is fine with timeBeginPeriod(1); leave spin reserve
				std::this_thread::sleep_for(
					std::chrono::duration<double, std::milli>(sleepMs));
			}

			// Fine wait (spin + light yield)
			for (;;)
			{
				const double t = NowSeconds();
				if (t >= m_NextFrameDeadlineSec)
					break;

				const double leftMs = (m_NextFrameDeadlineSec - t) * 1000.0;
				if (leftMs > 0.35)
					std::this_thread::yield();
				// else pure spin for last ~0.35ms
			}
		}
	}

	// Advance absolute schedule (keep phase lock)
	m_NextFrameDeadlineSec += periodSec;

	// If still behind after work+wait, jump schedule forward in steps
	const double after = NowSeconds();
	while (m_NextFrameDeadlineSec < after)
		m_NextFrameDeadlineSec += periodSec;
}
