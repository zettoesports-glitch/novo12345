#pragma once
// ---------------------------------------------------------------------------
// FrameProfiler — performance debug (F11 on/off, Ctrl+F11 clear stats/log)
// Overlay: FPS, ms/frame, target, hitch, top sections
// Log: slow frames + 1s averages → perf_debug.log next to Main.exe
// ---------------------------------------------------------------------------

enum PerfSection
{
	PERF_FRAME = 0,
	PERF_MOVE,
	PERF_MOVE_UI,
	PERF_MOVE_OBJECTS,
	PERF_MOVE_CHARS,
	PERF_MOVE_CAMERA,
	PERF_RENDER,
	PERF_TERRAIN,
	PERF_OBJECTS,
	PERF_CHARS,
	PERF_EFFECTS,
	PERF_SELECT,
	PERF_UI,
	PERF_SWAP,
	PERF_SLEEP,
	PERF_GPU,
	PERF_GPU_TERRAIN,
	PERF_GPU_CHARS,
	PERF_GPU_EFFECTS,
	PERF_GPU_UI,
	PERF_COUNT
};

const char* PerfSectionName(int id);

void PerfInit();
void PerfToggle();              // F11 — on/off
void PerfClear();               // Ctrl+F11 — reset peaks + truncate log
bool PerfIsEnabled();
void PerfBeginFrame();
void PerfEndFrame();
void PerfBegin(int section);
void PerfEnd(int section);
void PerfNoteMouse(float mx, float my);
void PerfRenderOverlay();       // on-screen HUD (BeginBitmap)

// GPU timing (GL_TIME_ELAPSED)
void PerfGpuBegin();
void PerfGpuEnd();
double PerfGetLastGpuMs();

// GPU timing per-section
void PerfGpuBeginSection(int section);
void PerfGpuEndSection(int section);

// Live stats (for other systems / title bar if needed)
double PerfGetLastFrameMs();
double PerfGetAvgFrameMs();
double PerfGetPeakFrameMs();
int    PerfGetHitchCount();

// RAII helper
struct PerfScope
{
	int id;
	explicit PerfScope(int section) : id(section) { PerfBegin(section); }
	~PerfScope() { PerfEnd(id); }
};

#define PERF_SCOPE(section) PerfScope _perf_##section(section)
