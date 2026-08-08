#include "stdafx.h"
#include "CastleSiegeHUD.h"
#include "WSclient.h"
#include "ZzzInterface.h"
#include "MapManager.h"

CastleSiegeHUD gCastleSiegeHUD;

CastleSiegeHUD::CastleSiegeHUD() {}
CastleSiegeHUD::~CastleSiegeHUD() {}

void CastleSiegeHUD::Render()
{
	if (Switch_Info == NULL || World != 30) // 30 = Castle Siege map (Lorencia Castle)
		return;

	if (!g_pRenderText)
		return;

	int iStartX = 10;
	int iStartY = 80;
	int iWidth = 120;
	int iHeight = 50;

	g_pRenderText->SetBgColor(0, 0, 0, 180);
	g_pRenderText->SetTextColor(255, 215, 0, 255);

	char szText[128];

	sprintf_s(szText, "SWITCH 1");
	g_pRenderText->RenderText(iStartX, iStartY, szText, iWidth, 0, RT3_SORT_CENTER);

	if (Switch_Info[0].m_bySwitchState != 0)
	{
		g_pRenderText->SetTextColor(255, 255, 255, 255);
		sprintf_s(szText, "[%s]", Switch_Info[0].m_szGuildName);
		g_pRenderText->RenderText(iStartX, iStartY + 15, szText, iWidth, 0, RT3_SORT_CENTER);
		
		g_pRenderText->SetTextColor(200, 200, 200, 255);
		sprintf_s(szText, "%s", Switch_Info[0].m_szUserName);
		g_pRenderText->RenderText(iStartX, iStartY + 30, szText, iWidth, 0, RT3_SORT_CENTER);
	}
	else
	{
		g_pRenderText->SetTextColor(150, 150, 150, 255);
		g_pRenderText->RenderText(iStartX, iStartY + 15, "-", iWidth, 0, RT3_SORT_CENTER);
	}

	iStartY += 50;

	g_pRenderText->SetTextColor(255, 215, 0, 255);
	sprintf_s(szText, "SWITCH 2");
	g_pRenderText->RenderText(iStartX, iStartY, szText, iWidth, 0, RT3_SORT_CENTER);

	if (Switch_Info[1].m_bySwitchState != 0)
	{
		g_pRenderText->SetTextColor(255, 255, 255, 255);
		sprintf_s(szText, "[%s]", Switch_Info[1].m_szGuildName);
		g_pRenderText->RenderText(iStartX, iStartY + 15, szText, iWidth, 0, RT3_SORT_CENTER);
		
		g_pRenderText->SetTextColor(200, 200, 200, 255);
		sprintf_s(szText, "%s", Switch_Info[1].m_szUserName);
		g_pRenderText->RenderText(iStartX, iStartY + 30, szText, iWidth, 0, RT3_SORT_CENTER);
	}
	else
	{
		g_pRenderText->SetTextColor(150, 150, 150, 255);
		g_pRenderText->RenderText(iStartX, iStartY + 15, "-", iWidth, 0, RT3_SORT_CENTER);
	}
}
