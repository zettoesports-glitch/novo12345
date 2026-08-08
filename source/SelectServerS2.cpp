#include "stdafx.h"
#include "ZzzBMD.h"
#include "ZzzCharacter.h"
#include "ZzzObject.h"
#include "ZzzInfomation.h"
#include "ZzzAI.h"
#include "ZzzScene.h"
#include "GOBoid.h"
#include "GIPetManager.h"
#include "SelectServerS2.h"

CHARACTER* g_pDarkLordS2 = NULL;
int g_nDarkLordKey = 2;

static float g_DarkLordS2Position[3] = { 10603.0f, 14112.0f, 580.0f };
static float g_DarkLordS2Angle[3] = { 0.0f, 0.0f, 188.0f };

void CreateLoginDarkLordS2()
{
    CHARACTER* pCharacter = NULL;
    g_nDarkLordKey = 2;
    pCharacter = CreateHeroLogIn(g_nDarkLordKey, Dark_Lord, 0, 1.6f,
        g_DarkLordS2Angle, g_DarkLordS2Position);

    if (pCharacter != NULL)
    {
        g_pDarkLordS2 = pCharacter;
        gGoboidManager->CreateBug(MODEL_DARK_HORSE, g_DarkLordS2Position, &pCharacter->Object);
        giPetManager::CreatePetDarkSpirit(pCharacter);
    }
}

void MoveLoginDarkLordS2()
{
    CHARACTER* pCharacter = g_pDarkLordS2;
    if (pCharacter == NULL) return;

    static int lastAnimationTime = 0;
    static bool useHorseSkill = true;
    int currentTime = (int)WorldTime;

    if ((currentTime - lastAnimationTime) >= 4500)
    {
        lastAnimationTime = currentTime;

        if (useHorseSkill)
        {
            SetAction(&pCharacter->Object, PLAYER_HIT_DARKHORSE, true);
        }
        else
        {
            SetAction(&pCharacter->Object, PLAYER_IDLE2_DARKHORSE, true);
        }

        useHorseSkill = !useHorseSkill;
    }
}
