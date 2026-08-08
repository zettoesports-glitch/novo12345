// ============================================================
// GPU Integration Module - Implementação
// ============================================================
#include "stdafx.h"
#include "GpuIntegration.h"
#include <gl/GL.h>
#include "ZzzCharacter.h"
#include "CGMProtect.h"
#include "ZzzOpenglUtil.h"
#include "Time/CFPSController.h"
#ifdef GPU_SKINNING
#include "GPUSkinning.h"
#endif

FpsCounter g_FpsCounter;

GPUVendor DetectGPU()
{
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* vendor   = (const char*)glGetString(GL_VENDOR);
    
    std::string r(renderer ? renderer : "");
    std::string v(vendor ? vendor : "");
    
    if (r.find("NVIDIA") != std::string::npos || v.find("NVIDIA") != std::string::npos)
        return GPUVendor::NVIDIA;
    if (r.find("AMD") != std::string::npos || r.find("ATI") != std::string::npos || v.find("AMD") != std::string::npos || v.find("ATI") != std::string::npos)
        return GPUVendor::AMD;
    if (r.find("Intel") != std::string::npos || v.find("Intel") != std::string::npos)
        return GPUVendor::INTEL;
    
    return GPUVendor::Unknown;
}

void PrintOpenGLInfo()
{
    const char* version  = (const char*)glGetString(GL_VERSION);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* vendor   = (const char*)glGetString(GL_VENDOR);
    const char* glsl     = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    
    char buf[512];
    sprintf_s(buf, "[GPU] ============================================");
    OutputDebugStringA(buf); OutputDebugStringA("\n");
    sprintf_s(buf, "[GPU] Vendor   : %s", vendor   ? vendor   : "N/A");
    OutputDebugStringA(buf); OutputDebugStringA("\n");
    sprintf_s(buf, "[GPU] Renderer : %s", renderer ? renderer : "N/A");
    OutputDebugStringA(buf); OutputDebugStringA("\n");
    sprintf_s(buf, "[GPU] OpenGL   : %s", version  ? version  : "N/A");
    OutputDebugStringA(buf); OutputDebugStringA("\n");
    sprintf_s(buf, "[GPU] GLSL     : %s", glsl     ? glsl     : "N/A");
    OutputDebugStringA(buf); OutputDebugStringA("\n");
    sprintf_s(buf, "[GPU] ============================================");
    OutputDebugStringA(buf); OutputDebugStringA("\n");
    
    g_FpsCounter.m_vendor = DetectGPU();
    
    const char* vendorName = "Unknown";
    if (renderer) strcpy_s(g_FpsCounter.gpuName, renderer); else g_FpsCounter.gpuName[0] = 0;
    switch (g_FpsCounter.m_vendor)
    {
        case GPUVendor::NVIDIA: vendorName = "NVIDIA"; break;
        case GPUVendor::AMD:    vendorName = "AMD";    break;
        case GPUVendor::INTEL:  vendorName = "INTEL";  break;
    }
    sprintf_s(buf, "[GPU] Detected : %s", vendorName);
    OutputDebugStringA(buf); OutputDebugStringA("\n");
}

void FpsCounter::Init()
{
    m_frameCount = 0;
    m_fps = 0.0f;
    currentFPS = 0.0f;
    m_vendor = GPUVendor::Unknown;
    gpuName[0] = 0;

    LARGE_INTEGER freq = {};
    LARGE_INTEGER now = {};
    if (QueryPerformanceFrequency(&freq) && freq.QuadPart > 0)
        m_freq = (double)freq.QuadPart;
    else
        m_freq = 1000.0; // fallback

    if (QueryPerformanceCounter(&now))
        m_lastTime = (double)now.QuadPart / m_freq;
    else
        m_lastTime = (double)GetTickCount() / 1000.0;
}

void FpsCounter::Frame()
{
    m_frameCount++;

    double currentTime;
    LARGE_INTEGER now = {};
    if (m_freq > 0.0 && QueryPerformanceCounter(&now))
        currentTime = (double)now.QuadPart / m_freq;
    else
        currentTime = (double)GetTickCount() / 1000.0;

    const double elapsed = currentTime - m_lastTime;
    // Atualiza ~1x por segundo (mesmo lugar na UI)
    if (elapsed >= 1.0)
    {
        m_fps = (elapsed > 0.000001) ? (float)((double)m_frameCount / elapsed) : 0.0f;
        currentFPS = m_fps;
        m_frameCount = 0;
        m_lastTime = currentTime;
    }
}

float FpsCounter::GetFPS()
{
    return m_fps;
}

void InitGPUSystems()
{
    g_FpsCounter.Init();
    
    PrintOpenGLInfo();
#ifdef GPU_SKINNING
    CGPUSkinning::Get().Init();
#endif
	// Profiler (F11) — ver FrameProfiler
	extern void PerfInit();
	PerfInit();
}

void RenderFPSOverlay(HWND hWnd)
{
	if (!hWnd)
		return;

	// Nome base do client (MainInfo / protect)
	const char* baseName = (GMProtect && GMProtect->GetWindowName() && GMProtect->GetWindowName()[0])
		? GMProtect->GetWindowName()
		: "MU Online";

	// VIP so depois de entrar no jogo com personagem (AccountLevel vem do GS no join)
	// Login / selecao de char: NAO inventar FREE/VIP
	extern int SceneFlag;
	const bool loggedInGame =
		(SceneFlag == MAIN_SCENE)
		&& Hero
		&& Hero->ID[0] != '\0'
		&& CharacterMachine != NULL;

	char buf[512];

	if (!loggedInGame)
	{
		SetWindowTextA(hWnd, baseName);
		return;
	}

	const char* accType = "FREE";
	switch (CharacterMachine->AccountLevel)
	{
	case 1: accType = "VIP 1"; break;
	case 2: accType = "VIP 2"; break;
	case 3: accType = "VIP 3"; break;
	default: accType = "FREE"; break;
	}

	// Titulo: Client | Personagem | FREE/VIP
	sprintf_s(buf, "%s | %s | %s", baseName, Hero->ID, accType);
	SetWindowTextA(hWnd, buf);
}
