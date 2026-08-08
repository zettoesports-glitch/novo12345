#include "stdafx.h"
#include "jpexs.h"
#include "UIBaseDef.h"
#include "UIControls.h"
#include "ZzzInterface.h"
#include "NewUISystem.h"
#include "NewUICommon.h"
#include "CBDrawInterface.h"
#include "TextClien.h"
#include "wsclientinline.h"
#include "CustomEventTime.h"
#if(CB_BXHDMG)
#include "CB_BXHTopDmg.h"
#endif
using namespace SEASON3B;
CBDrawInterface* CBDrawInterface::Instance()
{
	static CBDrawInterface s_Instance;
	return &s_Instance;
}

CBDrawInterface::CBDrawInterface()
{
#if(CB_BXHDMG)
	if (gCB_BXHTopDmg == 0)
	{
		gCB_BXHTopDmg = new CB_BXHTopDmg;
	}
#endif
}

CBDrawInterface::~CBDrawInterface()
{
#if(CB_BXHDMG)
	if (gCB_BXHTopDmg)
	{
		delete gCB_BXHTopDmg;
		gCB_BXHTopDmg = 0;
	}
#endif
}
DWORD  SleepTimeHP = 0;
void CBDrawInterface::RenderFrame()
{
	g_CustomEventTime->DrawEventTimePanelWindow();
#if(CB_BXHDMG)
	if (gCB_BXHTopDmg) gCB_BXHTopDmg->DrawWindowMini();
#endif

	gInterface->DrawMessageBox();
}


