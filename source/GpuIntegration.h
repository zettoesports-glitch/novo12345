// ============================================================
// GPU Integration Module - Ativa todas as otimizações do projeto
// ============================================================
#pragma once

#include <string>
#include <windows.h>

// Detecção de GPU
enum class GPUVendor { Unknown = 0, NVIDIA = 1, AMD = 2, INTEL = 3 };
GPUVendor DetectGPU();

// Info do OpenGL
void PrintOpenGLInfo();

// FPS Counter (QueryPerformanceCounter — alta precisao)
class FpsCounter {
public:
    void Init();
    void Frame();
    float GetFPS();
    bool IsVendorNVIDIA() { return m_vendor == GPUVendor::NVIDIA; }
    bool IsVendorAMD()    { return m_vendor == GPUVendor::AMD; }
    bool IsVendorIntel()  { return m_vendor == GPUVendor::INTEL; }
    
    float currentFPS;   // valor usado no canto do client (NewUIMainFrameWindow)
    GPUVendor m_vendor;
    char gpuName[64];
private:
    int m_frameCount;
    double m_lastTime;  // segundos (QPC)
    double m_freq;      // ticks/segundo
    float m_fps;
};

extern FpsCounter g_FpsCounter;

// Inicializa tudo
void InitGPUSystems();

// Atualiza titulo da janela (VIP so com personagem logado no jogo)
void RenderFPSOverlay(HWND hWnd);
