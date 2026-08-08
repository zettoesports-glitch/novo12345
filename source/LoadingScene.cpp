//*****************************************************************************
// File: LoadingScene.cpp
// Loading ao entrar no mundo: so imagem (sem faixas pretas 03/04)
//*****************************************************************************

#include "stdafx.h"
#include "LoadingScene.h"

#include "Input.h"
#include "ZzzOpenglUtil.h"

CLoadingScene::CLoadingScene()
{
}

CLoadingScene::~CLoadingScene()
{
}

void CLoadingScene::Create()
{
	CInput rInput = CInput::Instance();
	// Arte principal TitleBg01+02 = 800 x 512 — estica na tela toda (sem barras)
	const float fScaleX = (float)rInput.GetScreenWidth() / 800.0f;
	const float fScaleY = (float)rInput.GetScreenHeight() / 512.0f;

	// 0 e 1: imagem principal | 2 e 3: faixas de baixo (escondidas)
	int anHeight[LDS_BACK_MAX] = { 512, 512, 88, 88 };
	for (int i = 0; i < LDS_BACK_MAX; ++i)
	{
		m_asprBack[i].Create(400, anHeight[i], BITMAP_TITLE + i, 0, NULL, 0, 0, false,
			SPR_SIZING_DATUMS_LT, fScaleX, fScaleY);
		m_asprBack[i].Show(i < 2); // so 0 e 1
	}

	m_asprBack[0].SetPosition(0, 0);
	m_asprBack[1].SetPosition(400, 0, X);
	// 2/3 nao usados (faixas pretas)
	m_asprBack[2].SetPosition(0, 512, Y);
	m_asprBack[3].SetPosition(400, 512);
}

void CLoadingScene::Release()
{
	for (int i = 0; i < LDS_BACK_MAX; ++i)
		m_asprBack[i].Release();
}

void CLoadingScene::Render()
{
	// So imagem principal (TitleBg01 + TitleBg02), full screen
	for (int i = 0; i < 2; ++i)
		m_asprBack[i].Render();

	// Linha de loading simples no rodape (sem faixa preta decorativa)
	const float sw = (float)WindowWidth;
	const float sh = (float)WindowHeight;
	const float barH = 6.0f * ((float)WindowHeight / 600.0f);
	const float barY = sh - barH * 3.0f;
	const float margin = sw * 0.08f;
	const float barW = sw - margin * 2.0f;

	// Trilha escura (Scale=false = pixels de tela)
	EnableAlphaTest();
	glColor4f(0.05f, 0.05f, 0.05f, 0.85f);
	RenderColor(margin, barY, barW, barH, 0.f, 0, false);
	// Barra (pulse leve — loading e rapido nesta cena)
	float t = (float)(GetTickCount() % 1200) / 1200.0f;
	float fill = 0.15f + 0.7f * t;
	if (fill > 1.0f) fill = 1.0f;
	glColor4f(0.85f, 0.70f, 0.25f, 0.95f);
	RenderColor(margin, barY, barW * fill, barH, 0.f, 0, false);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	EndRenderColor();
}
