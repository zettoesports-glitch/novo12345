#include "stdafx.h"
#include "FrameProfiler.h"
#include "ZzzOpenglUtil.h"
#include "UIControls.h"
#include "Winmain.h"
#include "Time/CFPSController.h"
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>

static bool s_enabled = false;
static double s_freq = 0.0;

static double s_accumMs[PERF_COUNT];
static double s_lastMs[PERF_COUNT];
static double s_peakMs[PERF_COUNT];
static int s_framesInWindow = 0;
static double s_windowStart = 0.0;

static LONGLONG s_sectionStart[PERF_COUNT];
static bool s_sectionOpen[PERF_COUNT];

static float s_prevMouseX = 0.f;
static float s_prevMouseY = 0.f;
static float s_mouseDelta = 0.f;
static bool s_mouseMoving = false;

static char s_logPath[MAX_PATH] = { 0 };
static int s_slowFrameCount = 0;       // hitches this 1s window
static int s_hitchTotal = 0;           // session total
static double s_lastHitchMs = 0.0;

// EMA + peak for display (session, not reset every second)
static double s_emaFrameMs = 0.0;
static double s_peakFrameMs = 0.0;
static double s_lastFrameMs = 0.0;
static int s_displayFps = 0;
static double s_displayAvgMs = 0.0;

// GPU timing — query objects for GL_TIME_ELAPSED
static GLuint s_gpuQuery = 0;
static bool s_gpuQueryActive = false;
static double s_lastGpuMs = 0.0;
static double s_accumGpuMs = 0.0;
static double s_peakGpuMs = 0.0;

// GPU timing per-section
static GLuint s_gpuSectionQueries[4] = { 0 };
static bool s_gpuSectionActive[4] = { false };
static double s_lastGpuSectionMs[4] = { 0 };
static double s_accumGpuSectionMs[4] = { 0 };
static double s_peakGpuSectionMs[4] = { 0 };

// Hitch = frame longer than budget of target FPS * 1.5 (min 20ms)
static double HitchThresholdMs()
{
	double target = gFPSController->GetTargetFPS();
	if (target <= 0.0)
		target = 60.0; // MAX mode — use 60 as soft budget for hitch detect
	double budget = 1000.0 / target;
	double hitch = budget * 1.5;
	if (hitch < 20.0)
		hitch = 20.0;
	if (hitch > 50.0)
		hitch = 50.0;
	return hitch;
}

static double NowMs()
{
	LARGE_INTEGER c;
	if (!QueryPerformanceCounter(&c) || s_freq <= 0.0)
		return (double)GetTickCount();
	return (double)c.QuadPart * 1000.0 / s_freq;
}

const char* PerfSectionName(int id)
{
	switch (id)
	{
	case PERF_FRAME: return "FRAME";
	case PERF_MOVE: return "Move";
	case PERF_MOVE_UI: return "Move.UI";
	case PERF_MOVE_OBJECTS: return "Move.Obj";
	case PERF_MOVE_CHARS: return "Move.Char";
	case PERF_MOVE_CAMERA: return "Move.Cam";
	case PERF_RENDER: return "Render";
	case PERF_TERRAIN: return "Terrain";
	case PERF_OBJECTS: return "Objects";
	case PERF_CHARS: return "Chars";
	case PERF_EFFECTS: return "Effects";
	case PERF_SELECT: return "Select";
	case PERF_UI: return "UI";
	case PERF_SWAP: return "Swap";
	case PERF_SLEEP: return "Sleep";
	case PERF_GPU: return "GPU";
	case PERF_GPU_TERRAIN: return "GPU.Terrain";
	case PERF_GPU_CHARS: return "GPU.Chars";
	case PERF_GPU_EFFECTS: return "GPU.Effects";
	case PERF_GPU_UI: return "GPU.UI";
	default: return "?";
	}
}

void PerfInit()
{
	LARGE_INTEGER f;
	if (QueryPerformanceFrequency(&f) && f.QuadPart > 0)
		s_freq = (double)f.QuadPart;
	else
		s_freq = 1000.0;

	memset(s_accumMs, 0, sizeof(s_accumMs));
	memset(s_lastMs, 0, sizeof(s_lastMs));
	memset(s_peakMs, 0, sizeof(s_peakMs));
	memset(s_sectionOpen, 0, sizeof(s_sectionOpen));
	s_framesInWindow = 0;
	s_windowStart = NowMs();
	s_slowFrameCount = 0;
	s_hitchTotal = 0;
	s_lastHitchMs = 0.0;
	s_emaFrameMs = 0.0;
	s_peakFrameMs = 0.0;
	s_lastFrameMs = 0.0;
	s_displayFps = 0;
	s_displayAvgMs = 0.0;

	s_lastGpuMs = 0.0;
	s_accumGpuMs = 0.0;
 	s_peakGpuMs = 0.0;
 	s_gpuQueryActive = false;
 	if (s_gpuQuery == 0 && glewIsSupported("GL_VERSION_3_3"))
	{
 		glGenQueries(1, &s_gpuQuery);
 		glGenQueries(4, s_gpuSectionQueries);
	}

	GetModuleFileNameA(NULL, s_logPath, MAX_PATH);
	char* slash = strrchr(s_logPath, '\\');
	if (slash)
		strcpy_s(slash + 1, MAX_PATH - (slash + 1 - s_logPath), "perf_debug.log");
	else
		strcpy_s(s_logPath, "perf_debug.log");
}

void PerfGpuBegin()
{
	if (!s_enabled || s_gpuQuery == 0 || s_gpuQueryActive)
		return;
	glBeginQuery(GL_TIME_ELAPSED, s_gpuQuery);
	s_gpuQueryActive = true;
}

void PerfGpuEnd()
{
	if (!s_enabled || !s_gpuQueryActive)
		return;
	glEndQuery(GL_TIME_ELAPSED);
	s_gpuQueryActive = false;

	GLint available = 0;
	glGetQueryObjectiv(s_gpuQuery, GL_QUERY_RESULT_AVAILABLE, &available);
	if (available)
	{
		GLuint64 elapsed = 0;
		glGetQueryObjectui64v(s_gpuQuery, GL_QUERY_RESULT, &elapsed);
		s_lastGpuMs = (double)elapsed / 1000000.0;
		s_accumGpuMs += s_lastGpuMs;
		if (s_lastGpuMs > s_peakGpuMs)
			s_peakGpuMs = s_lastGpuMs;
		s_lastMs[PERF_GPU] = s_lastGpuMs;
		s_accumMs[PERF_GPU] += s_lastGpuMs;
		if (s_lastGpuMs > s_peakMs[PERF_GPU])
			s_peakMs[PERF_GPU] = s_lastGpuMs;
	}
}

void PerfGpuBeginSection(int section)
{
	int idx = section - PERF_GPU_TERRAIN;
	if (idx < 0 || idx >= 4 || !s_enabled || s_gpuSectionQueries[idx] == 0 || s_gpuSectionActive[idx])
		return;
	glBeginQuery(GL_TIME_ELAPSED, s_gpuSectionQueries[idx]);
	s_gpuSectionActive[idx] = true;
}

void PerfGpuEndSection(int section)
{
	int idx = section - PERF_GPU_TERRAIN;
	if (idx < 0 || idx >= 4 || !s_enabled || !s_gpuSectionActive[idx])
		return;
	glEndQuery(GL_TIME_ELAPSED);
	s_gpuSectionActive[idx] = false;

	GLint available = 0;
	glGetQueryObjectiv(s_gpuSectionQueries[idx], GL_QUERY_RESULT_AVAILABLE, &available);
	if (available)
	{
		GLuint64 elapsed = 0;
		glGetQueryObjectui64v(s_gpuSectionQueries[idx], GL_QUERY_RESULT, &elapsed);
		double ms = (double)elapsed / 1000000.0;
		s_lastGpuSectionMs[idx] = ms;
		s_accumGpuSectionMs[idx] += ms;
		if (ms > s_peakGpuSectionMs[idx])
			s_peakGpuSectionMs[idx] = ms;
		s_lastMs[section] = ms;
		s_accumMs[section] += ms;
		if (ms > s_peakMs[section])
			s_peakMs[section] = ms;
	}
}

void PerfClear()
{
	memset(s_accumMs, 0, sizeof(s_accumMs));
	memset(s_lastMs, 0, sizeof(s_lastMs));
	memset(s_peakMs, 0, sizeof(s_peakMs));
	s_framesInWindow = 0;
	s_windowStart = NowMs();
	s_slowFrameCount = 0;
	s_hitchTotal = 0;
	s_lastHitchMs = 0.0;
	s_emaFrameMs = 0.0;
	s_peakFrameMs = 0.0;
	s_displayFps = 0;
	s_displayAvgMs = 0.0;
	s_lastGpuMs = 0.0;
	s_accumGpuMs = 0.0;
	s_peakGpuMs = 0.0;
	memset(s_lastGpuSectionMs, 0, sizeof(s_lastGpuSectionMs));
	memset(s_accumGpuSectionMs, 0, sizeof(s_accumGpuSectionMs));
	memset(s_peakGpuSectionMs, 0, sizeof(s_peakGpuSectionMs));
	memset(s_gpuSectionActive, 0, sizeof(s_gpuSectionActive));

	// Truncate log
	FILE* f = NULL;
	if (fopen_s(&f, s_logPath, "w") == 0 && f)
	{
		fputs("[PERF] cleared\n", f);
		fclose(f);
	}
	OutputDebugStringA("[PERF] stats + log cleared (Ctrl+F11)\n");
}

void PerfToggle()
{
	s_enabled = !s_enabled;
	if (s_enabled)
	{
		PerfInit();
		char msg[256];
		sprintf_s(msg, "[PERF] ON — F11 off | Ctrl+F11 clear | log: %s\n", s_logPath);
		OutputDebugStringA(msg);
	}
	else
	{
		OutputDebugStringA("[PERF] OFF\n");
	}
}

bool PerfIsEnabled()
{
	return s_enabled;
}

double PerfGetLastFrameMs() { return s_lastFrameMs; }
double PerfGetAvgFrameMs() { return s_displayAvgMs > 0.0 ? s_displayAvgMs : s_emaFrameMs; }
double PerfGetLastGpuMs() { return s_lastGpuMs; }
double PerfGetPeakFrameMs() { return s_peakFrameMs; }
int PerfGetHitchCount() { return s_hitchTotal; }

void PerfNoteMouse(float mx, float my)
{
	float dx = mx - s_prevMouseX;
	float dy = my - s_prevMouseY;
	s_mouseDelta = sqrtf(dx * dx + dy * dy);
	s_mouseMoving = (s_mouseDelta > 0.5f);
	s_prevMouseX = mx;
	s_prevMouseY = my;
}

void PerfBeginFrame()
{
	if (!s_enabled)
		return;

	for (int i = 0; i < PERF_COUNT; ++i)
	{
		s_lastMs[i] = 0.0;
		s_sectionOpen[i] = false;
	}
	PerfBegin(PERF_FRAME);
}

void PerfBegin(int section)
{
	if (!s_enabled || section < 0 || section >= PERF_COUNT)
		return;

	LARGE_INTEGER c;
	QueryPerformanceCounter(&c);
	s_sectionStart[section] = c.QuadPart;
	s_sectionOpen[section] = true;
}

void PerfEnd(int section)
{
	if (!s_enabled || section < 0 || section >= PERF_COUNT || !s_sectionOpen[section])
		return;

	LARGE_INTEGER c;
	QueryPerformanceCounter(&c);
	const double ms = (double)(c.QuadPart - s_sectionStart[section]) * 1000.0 / s_freq;
	s_lastMs[section] = ms;
	s_accumMs[section] += ms;
	if (ms > s_peakMs[section])
		s_peakMs[section] = ms;
	s_sectionOpen[section] = false;
}

static void WriteLogLine(const char* line)
{
	FILE* f = NULL;
	if (fopen_s(&f, s_logPath, "a") == 0 && f)
	{
		fputs(line, f);
		fputc('\n', f);
		fclose(f);
	}
	OutputDebugStringA(line);
	OutputDebugStringA("\n");
}

void PerfEndFrame()
{
	if (!s_enabled)
		return;

	if (s_sectionOpen[PERF_FRAME])
		PerfEnd(PERF_FRAME);

	s_framesInWindow++;

	const double frameMs = s_lastMs[PERF_FRAME];
	s_lastFrameMs = frameMs;

	// EMA ~0.5s feel
	if (s_emaFrameMs <= 0.0)
		s_emaFrameMs = frameMs;
	else
		s_emaFrameMs = s_emaFrameMs * 0.92 + frameMs * 0.08;

	if (frameMs > s_peakFrameMs)
		s_peakFrameMs = frameMs;

	const double hitchMs = HitchThresholdMs();
	const bool hitch = frameMs >= hitchMs;

	if (hitch)
	{
		s_slowFrameCount++;
		s_hitchTotal++;
		s_lastHitchMs = frameMs;

		int order[PERF_COUNT];
		for (int i = 0; i < PERF_COUNT; ++i)
			order[i] = i;
		for (int a = 1; a < PERF_COUNT; ++a)
		{
			for (int b = a + 1; b < PERF_COUNT; ++b)
			{
				if (s_lastMs[order[b]] > s_lastMs[order[a]])
				{
					int t = order[a];
					order[a] = order[b];
					order[b] = t;
				}
			}
		}

		const double target = gFPSController->GetTargetFPS();
		char line[1024];
		sprintf_s(line,
			"[PERF HITCH] %.1fms (budget %.1f) fps~%.0f target=%.0f mouse=%.1f %s | totalHitches=%d",
			frameMs, hitchMs,
			frameMs > 0.01 ? 1000.0 / frameMs : 0.0,
			target,
			s_mouseDelta,
			s_mouseMoving ? "MOVING" : "still",
			s_hitchTotal);
		WriteLogLine(line);

		for (int i = 0; i < 6; ++i)
		{
			int id = order[i];
			if (id == PERF_FRAME)
				continue;
			if (s_lastMs[id] < 0.05)
				continue;
			sprintf_s(line, "  %s = %.2f ms", PerfSectionName(id), s_lastMs[id]);
			WriteLogLine(line);
		}
	}

	// Summary every ~1s
	const double now = NowMs();
	if (now - s_windowStart >= 1000.0 && s_framesInWindow > 0)
	{
		char line[1024];
		const double avgFrame = s_accumMs[PERF_FRAME] / s_framesInWindow;
		s_displayAvgMs = avgFrame;
		s_displayFps = (avgFrame > 0.01) ? (int)(1000.0 / avgFrame + 0.5) : 0;

		const double target = gFPSController->GetTargetFPS();
		sprintf_s(line,
			"[PERF 1s] n=%d avg=%.2fms (~%d FPS) target=%.0f peak=%.1f hitch=%d (sess %d) mouse=%.1f",
			s_framesInWindow, avgFrame, s_displayFps, target,
			s_peakFrameMs, s_slowFrameCount, s_hitchTotal, s_mouseDelta);
		WriteLogLine(line);

		int order[PERF_COUNT];
		for (int i = 0; i < PERF_COUNT; ++i)
			order[i] = i;
		for (int a = 1; a < PERF_COUNT; ++a)
		{
			for (int b = a + 1; b < PERF_COUNT; ++b)
			{
				double aa = s_accumMs[order[a]] / s_framesInWindow;
				double bb = s_accumMs[order[b]] / s_framesInWindow;
				if (bb > aa)
				{
					int t = order[a];
					order[a] = order[b];
					order[b] = t;
				}
			}
		}
		for (int i = 0; i < PERF_COUNT; ++i)
		{
			int id = order[i];
			if (id == PERF_FRAME)
				continue;
			double avg = s_accumMs[id] / s_framesInWindow;
			if (avg < 0.05)
				continue;
			sprintf_s(line, "  avg %s = %.2f ms (peak %.2f)",
				PerfSectionName(id), avg, s_peakMs[id]);
			WriteLogLine(line);
		}

		memset(s_accumMs, 0, sizeof(s_accumMs));
		s_accumGpuMs = 0.0;
		memset(s_accumGpuSectionMs, 0, sizeof(s_accumGpuSectionMs));
		// Keep session peak; reset window peaks only
		memset(s_peakMs, 0, sizeof(s_peakMs));
		s_peakGpuMs = 0.0;
		memset(s_peakGpuSectionMs, 0, sizeof(s_peakGpuSectionMs));
		s_framesInWindow = 0;
		s_slowFrameCount = 0;
		s_windowStart = now;
	}
}

void PerfRenderOverlay()
{
	if (!s_enabled || !g_pRenderText)
		return;

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0, 0, 0, 180);

	const double fm = s_lastFrameMs;
	const double avgMs = (s_displayAvgMs > 0.0) ? s_displayAvgMs : s_emaFrameMs;
	const double fpsNow = (fm > 0.01) ? (1000.0 / fm) : 0.0;
	const double fpsAvg = (avgMs > 0.01) ? (1000.0 / avgMs) : 0.0;
	const double target = gFPSController->GetTargetFPS();
	const double budget = (target > 0.0) ? (1000.0 / target) : 0.0;
	const double hitchBudget = HitchThresholdMs();

	char line[256];

	// Line 1: FPS + target
	if (fpsNow < target * 0.85 && target > 0.0)
		g_pRenderText->SetTextColor(255, 80, 80, 255);
	else if (fpsNow < target * 0.95 && target > 0.0)
		g_pRenderText->SetTextColor(255, 200, 40, 255);
	else
		g_pRenderText->SetTextColor(0, 255, 120, 255);

	if (target > 0.0)
		sprintf_s(line, "PERF  FPS %.0f avg %.0f / %.0f   |  F11 off  Ctrl+F11 clear",
			fpsNow, fpsAvg, target);
	else
		sprintf_s(line, "PERF  FPS %.0f avg %.0f / MAX   |  F11 off  Ctrl+F11 clear",
			fpsNow, fpsAvg);
	g_pRenderText->RenderText(8, 8, line);

	// Line 2: ms + budget + hitch (cor baseada no frame time vs budget)
	if (budget > 0.0 && fm > budget * 1.5)
		g_pRenderText->SetTextColor(255, 80, 80, 255);
	else if (budget > 0.0 && fm > budget * 1.1)
		g_pRenderText->SetTextColor(255, 200, 40, 255);
	else
		g_pRenderText->SetTextColor(0, 255, 120, 255);

	if (budget > 0.0)
		sprintf_s(line, "ms  now %.2f  avg %.2f  peak %.1f  |  budget %.2f  hitch>%.0f  hits %d",
			fm, avgMs, s_peakFrameMs, budget, hitchBudget, s_hitchTotal);
	else
		sprintf_s(line, "ms  now %.2f  avg %.2f  peak %.1f  |  hitch>%.0f  hits %d",
			fm, avgMs, s_peakFrameMs, hitchBudget, s_hitchTotal);
	g_pRenderText->RenderText(8, 22, line);

	// Line 3: mouse + last hitch + GPU
	if (s_lastGpuMs >= 12.0)
		g_pRenderText->SetTextColor(255, 80, 80, 255);
	else if (s_lastGpuMs >= 8.0)
		g_pRenderText->SetTextColor(255, 200, 40, 255);
	else
		g_pRenderText->SetTextColor(0, 255, 120, 255);

	if (s_lastHitchMs > 0.0)
		sprintf_s(line, "mouse %.1f %s  |  last hitch %.1f ms  |  GPU %.2f ms",
			s_mouseDelta, s_mouseMoving ? "MOVE" : "still", s_lastHitchMs, s_lastGpuMs);
	else
		sprintf_s(line, "mouse %.1f %s  |  no hitch yet  |  GPU %.2f ms",
			s_mouseDelta, s_mouseMoving ? "MOVE" : "still", s_lastGpuMs);
	g_pRenderText->RenderText(8, 36, line);

	// Top sections of last frame
	int order[PERF_COUNT];
	for (int i = 0; i < PERF_COUNT; ++i)
		order[i] = i;
	for (int a = 1; a < PERF_COUNT; ++a)
	{
		for (int b = a + 1; b < PERF_COUNT; ++b)
		{
			if (s_lastMs[order[b]] > s_lastMs[order[a]])
			{
				int t = order[a];
				order[a] = order[b];
				order[b] = t;
			}
		}
	}

	int y = 54;
	int shown = 0;
	for (int i = 0; i < PERF_COUNT && shown < 10; ++i)
	{
		int id = order[i];
		if (id == PERF_FRAME)
			continue;
		if (s_lastMs[id] < 0.05)
			continue;

		if (s_lastMs[id] >= 8.0)
			g_pRenderText->SetTextColor(255, 80, 80, 255);
		else if (s_lastMs[id] >= 3.0)
			g_pRenderText->SetTextColor(255, 200, 40, 255);
		else
			g_pRenderText->SetTextColor(200, 200, 200, 255);

		sprintf_s(line, "%-10s  %.2f ms", PerfSectionName(id), s_lastMs[id]);
		g_pRenderText->RenderText(8, y, line);
		y += 14;
		shown++;
	}

	g_pRenderText->SetTextColor(140, 140, 140, 255);
	g_pRenderText->RenderText(8, y + 4, "log: perf_debug.log (hitch frames + 1s avg)");
	g_pRenderText->SetFont(g_hFont);
}
