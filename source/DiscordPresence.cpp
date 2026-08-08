#include "stdafx.h"
#include "DiscordPresence.h"
#include "discord_rpc.h"
#include "ZzzScene.h"
#include "ZzzCharacter.h"
#include "MapManager.h"
#include "CharacterManager.h"

#ifdef _DEBUG
#pragma comment(lib, "discord_rpc_d.lib")
#else
#pragma comment(lib, "discord_rpc_r.lib")
#endif

#define DISCORD_APP_ID "970477832646062100"
#define DISCORD_UPDATE_INTERVAL 15

static time_t g_DiscordStartTime = 0;
static time_t g_DiscordLastUpdate = 0;
static bool g_DiscordInitialized = false;
static int g_LastSceneFlag = -1;
static WORD g_LastLevel = 0;
static int g_LastMap = -1;

void DiscordPresence_Init()
{
	if (g_DiscordInitialized)
		return;

	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	Discord_Initialize(DISCORD_APP_ID, &handlers, 1, NULL);

	g_DiscordStartTime = time(NULL);
	g_DiscordInitialized = true;
	g_LastSceneFlag = -1;
	g_LastLevel = 0;
	g_LastMap = -1;
}

void DiscordPresence_Update()
{
	if (!g_DiscordInitialized)
		return;

	time_t now = time(NULL);
	if (now - g_DiscordLastUpdate < DISCORD_UPDATE_INTERVAL)
		return;
	g_DiscordLastUpdate = now;

	DiscordRichPresence presence;
	memset(&presence, 0, sizeof(presence));

	presence.startTimestamp = g_DiscordStartTime;
	presence.largeImageKey = "loget";
	presence.largeImageText = "Mu Lendario";
	presence.buttons[0].label = "Entrar no Discord";
	presence.buttons[0].url = "https://discord.gg/d5p3TPujSt";
	presence.buttons[1].label = "Visitar o Site";
	presence.buttons[1].url = "https://muonline.com";

	if (SceneFlag == LOG_IN_SCENE)
	{
		if (g_LastSceneFlag != LOG_IN_SCENE)
			g_LastSceneFlag = LOG_IN_SCENE;
		presence.details = "Tela de Login";
		presence.state = "Conectando ao servidor";
	}
	else if (SceneFlag == CHARACTER_SCENE)
	{
		if (g_LastSceneFlag != CHARACTER_SCENE)
			g_LastSceneFlag = CHARACTER_SCENE;
		presence.details = "Selecao de Personagem";
		presence.state = "Escolhendo heroi";
	}
	else if (SceneFlag == MAIN_SCENE && Hero && CharacterAttribute)
	{
		WORD level = CharacterAttribute->Level;
		int currentMap = gMapManager->currentMap;
		const char* mapName = gMapManager->GetMapName(currentMap);

		char details[128];
		char state[128];

		if (gCharacterManager.IsMasterLevel(CharacterAttribute->Class) && level >= CharacterAttribute->MaxCharacterLevel)
		{
			sprintf_s(details, sizeof(details), "%s | ML %d", Hero->ID, CharacterAttribute->Level - CharacterAttribute->MaxCharacterLevel + 1);
		}
		else
		{
			sprintf_s(details, sizeof(details), "%s | Level %d", Hero->ID, level);
		}

		if (mapName && mapName[0])
		{
			sprintf_s(state, sizeof(state), "Em %s", mapName);
		}
		else
		{
			strcpy_s(state, sizeof(state), "Explorando");
		}

		presence.details = details;
		presence.state = state;

		g_LastLevel = level;
		g_LastMap = currentMap;
	}
	else
	{
		presence.details = "Mu Lendario";
		presence.state = "Iniciando";
	}

	Discord_UpdatePresence(&presence);
}

void DiscordPresence_Shutdown()
{
	if (!g_DiscordInitialized)
		return;

	Discord_ClearPresence();
	Discord_Shutdown();
	g_DiscordInitialized = false;
}
