#include "stdafx.h"
#include "Time/CFPSController.h"
#include "Time/Timer.h"

// Global FPS control variables
int g_FPSOverrideActive = 0;
double g_FPSOverrideMs = 0.0;
bool g_UseOriginal150FPSPipeline = false;

extern CTimer* g_pTimer;

CFPSController* CFPSController::m_pInstance = nullptr;

CFPSController::CFPSController()
    : m_bInitialized(false),
      m_TargetFPS(100.0),
      m_CurrentFPS(0.0),
      m_FrameTimeMs(0.0),
      m_FPSOverrideActive(false),
      m_FPSOverrideMs(0.0),
      m_UseOriginal150FPSPipeline(false)
{
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

    m_bInitialized = true;
    m_FrameStartTime = std::chrono::high_resolution_clock::now();
    m_LastFrameTime = m_FrameStartTime;
    return true;
}

void CFPSController::Shutdown()
{
    m_bInitialized = false;
}

void CFPSController::BeginFrame()
{
    m_FrameStartTime = std::chrono::high_resolution_clock::now();
}

void CFPSController::EndFrame()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto frameElapsed = std::chrono::duration<double>(now - m_FrameStartTime).count();
    m_FrameTimeMs = frameElapsed * 1000.0;

    // Calculate current FPS
    if (m_FrameTimeMs > 0.0)
    {
        m_CurrentFPS = 1000.0 / m_FrameTimeMs;
    }

    // Apply frame limiting
    ApplyFrameLimiting();

    m_LastFrameTime = now;
}

void CFPSController::SetTargetFPS(double fps)
{
    if (fps > 0.0)
    {
        m_TargetFPS = fps;
    }
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
}

void CFPSController::ApplyFrameLimiting()
{
    if (!m_bInitialized || !g_pTimer)
        return;

    double ms_per_frame = 1000.0 / m_TargetFPS;
    static double last_render_tick_count = 0.0;

    // Check for FPS override
    if (ms_per_frame > 0.0)
    {
        if (g_FPSOverrideActive == 1 && g_FPSOverrideMs > 0.0)
        {
            ms_per_frame = g_FPSOverrideMs;
        }

        double now_ms = g_pTimer->GetAbsTime();
        
        if (last_render_tick_count == 0.0)
            last_render_tick_count = now_ms;

        double elapsed_ms = now_ms - last_render_tick_count;
        
        if (elapsed_ms < ms_per_frame)
        {
            double rest_ms = ms_per_frame - elapsed_ms;
            
            if (g_UseOriginal150FPSPipeline)
            {
                // ORIGINAL PIPELINE FIX
                if (rest_ms > 1.0)
                {
                    Sleep((DWORD)rest_ms);
                }
            }
            else
            {
                // SLEEP SPIN FOR RECHECK
                // CPU AT TIMING
                constexpr float min_spin_ms = 1.0f;
                constexpr float spin_yield_threshold_ms = 0.5f;
                constexpr float sleep_threshold_ms = 5.0f;
                
                double sleep_ms = rest_ms - min_spin_ms;
                
                double start_sleep = g_pTimer->GetTimeElapsed();
                if (sleep_ms > sleep_threshold_ms)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(sleep_ms)));
                }
                
                double actual_sleep_ms = g_pTimer->GetTimeElapsed() - start_sleep;
                double start_spin = g_pTimer->GetTimeElapsed();
                double spin_ms = rest_ms - actual_sleep_ms;
                
                // ROT + FRAME COUNT TIMIBNGS
                while (true)
                {
                    double current = g_pTimer->GetTimeElapsed();
                    double spinned_ms = current - start_spin;
                    if (spinned_ms >= spin_ms) break;
                    if (spin_ms - spinned_ms > spin_yield_threshold_ms)
                    {
                        std::this_thread::yield();
                    }
                }
            }
        }

        last_render_tick_count = now_ms;
    }
}
