#include "stdafx.h"
#include "Protocol.h"
#include "WSclient.h"
#include "NewUISystem.h"
#include "wsclientinline.h"
#include "SkillManager.h"
#include "CharacterManager.h"
#include "Util.h"
#if (CB_GETMIXRATE)
#include "CB_GetMixRate.h"
#endif
#include "CustomEventTime.h"
#include "CB_DangKyInGame.h"
#if(CB_BXHDMG)
#include "CB_BXHTopDmg.h"
#endif
#include "SpectatorMode.h"
#include "ScreenShot.h"
#include "NewUIChatLogWindow.h"
#include "NewUIRankingTop.h"

bool g_bAccountItemsLocked = false;

void SendAccountLock(BYTE type, const char* password, const char* password2)
{
	PMSG_ACCOUNT_LOCK_SEND pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xE9, sizeof(pMsg));
	pMsg.type = type;

	// Copia senha em claro so no buffer local do pacote
	if (password)
	{
		strncpy_s(pMsg.password, password, _TRUNCATE);
		pMsg.password[7] = '\0';
	}
	if (password2)
	{
		strncpy_s(pMsg.password2, password2, _TRUNCATE);
		pMsg.password2[7] = '\0';
	}

	// Criptografa no fio (mesmo Bux do login: XOR 0xFC,0xCF,0xAB)
	// Campos inteiros de 8 bytes (inclui zeros) — nao vai legivel no sniffer
	BuxConvert((BYTE*)pMsg.password, (int)sizeof(pMsg.password));
	BuxConvert((BYTE*)pMsg.password2, (int)sizeof(pMsg.password2));

	DataSend((BYTE*)&pMsg, pMsg.header.size);

	// Limpa rastro na stack local (best-effort)
	SecureZeroMemory(pMsg.password, sizeof(pMsg.password));
	SecureZeroMemory(pMsg.password2, sizeof(pMsg.password2));
}

void GCAccountLockRecv(PMSG_ACCOUNT_LOCK_RECV* lpMsg)
{
	if (!lpMsg)
		return;

	g_bAccountItemsLocked = (lpMsg->locked != 0);

	// Mensagens extras do client (GS ja manda notice)
	if (lpMsg->result == ALR_CHANGE_OK)
	{
		if (g_pChatListBox)
			g_pChatListBox->AddText("", "Senha do Lock alterada com sucesso!", SEASON3B::TYPE_SYSTEM_MESSAGE);
	}
	else if (lpMsg->result == ALR_ERR_WRONG)
	{
		if (g_pChatListBox && lpMsg->failLeft > 0)
		{
			char msg[64];
			sprintf_s(msg, "Senha incorreta. Tentativas restantes: %d", (int)lpMsg->failLeft);
			g_pChatListBox->AddText("", msg, SEASON3B::TYPE_ERROR_MESSAGE);
		}
	}
	else if (lpMsg->result == ALR_ERR_BLOCKED)
	{
		if (g_pChatListBox)
			g_pChatListBox->AddText("", "Lock bloqueado por 5 minutos (muitas tentativas).", SEASON3B::TYPE_ERROR_MESSAGE);
	}
}

static void RankingPreviewTrimName(char* name, int maxLen)
{
	if (!name || maxLen <= 0)
		return;
	name[maxLen - 1] = '\0';
	// trim direita
	int n = (int)strlen(name);
	while (n > 0 && (name[n - 1] == ' ' || name[n - 1] == '\t'))
	{
		name[n - 1] = '\0';
		n--;
	}
	// trim esquerda
	int s = 0;
	while (name[s] == ' ' || name[s] == '\t')
		s++;
	if (s > 0)
		memmove(name, name + s, strlen(name + s) + 1);
}

void SendRequestRankingPreview(const char* name)
{
	if (!name || !name[0])
		return;

	PMSG_RANKING_PREVIEW_REQ pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xE5, sizeof(pMsg));
	strncpy_s(pMsg.name, name, _TRUNCATE);
	RankingPreviewTrimName(pMsg.name, (int)sizeof(pMsg.name));
	if (pMsg.name[0] == '\0')
		return;

	DataSend((BYTE*)&pMsg, pMsg.header.size);
}

void GCRankingPreviewRecv(PMSG_RANKING_PREVIEW_RECV* lpMsg)
{
	if (!lpMsg)
		return;

	// Encaminha para a janela de ranking (se aberta)
	if (gNewUIRankingTop)
		gNewUIRankingTop->ReceiveRankingPreview((BYTE*)lpMsg);
}

void SendAccountPassChange(const char* oldPass, const char* newPass)
{
	PMSG_ACCOUNT_PASS_CHANGE_SEND pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xEC, sizeof(pMsg));
	if (oldPass)
	{
		strncpy_s(pMsg.oldPass, oldPass, _TRUNCATE);
		pMsg.oldPass[10] = '\0';
	}
	if (newPass)
	{
		strncpy_s(pMsg.newPass, newPass, _TRUNCATE);
		pMsg.newPass[10] = '\0';
	}
	// Ofusca no fio (mesmo Bux do login)
	BuxConvert((BYTE*)pMsg.oldPass, (int)sizeof(pMsg.oldPass));
	BuxConvert((BYTE*)pMsg.newPass, (int)sizeof(pMsg.newPass));
	DataSend((BYTE*)&pMsg, pMsg.header.size);
	SecureZeroMemory(pMsg.oldPass, sizeof(pMsg.oldPass));
	SecureZeroMemory(pMsg.newPass, sizeof(pMsg.newPass));
}

void GCAccountPassChangeRecv(PMSG_ACCOUNT_PASS_CHANGE_RECV* lpMsg)
{
	if (!lpMsg || !g_pChatListBox)
		return;

	switch (lpMsg->result)
	{
	case 1:
		g_pChatListBox->AddText("", "Senha da conta alterada com sucesso!", SEASON3B::TYPE_SYSTEM_MESSAGE);
		break;
	case 0:
		g_pChatListBox->AddText("", "Senha atual da conta incorreta.", SEASON3B::TYPE_ERROR_MESSAGE);
		break;
	case 2:
		g_pChatListBox->AddText("", "Senha invalida (use 4 a 10 caracteres).", SEASON3B::TYPE_ERROR_MESSAGE);
		break;
	default:
		g_pChatListBox->AddText("", "Falha ao alterar senha da conta.", SEASON3B::TYPE_ERROR_MESSAGE);
		break;
	}
}

BOOL ProtocolCoreEx(BYTE head, BYTE* lpMsg, int size, int key) // OK
{
	switch (head)
	{
#if(CB_GETMIXRATE)
	case 0x88:
		if (gCB_GetMixRate) gCB_GetMixRate->GCRecvMixInfo(lpMsg, size);
		break;
#endif
	case 0xF3:
		switch (((lpMsg[0] == 0xC1) ? lpMsg[3] : lpMsg[4]))
		{
		case 0xE5:
			GCRankingPreviewRecv((PMSG_RANKING_PREVIEW_RECV*)lpMsg);
			return 1;
		case 0xE8:
			g_CustomEventTime->GCReqEventTime((PMSG_CUSTOM_EVENTTIME_RECV*)lpMsg);
			return 1;
		case 0xEA:
			GCAccountLockRecv((PMSG_ACCOUNT_LOCK_RECV*)lpMsg);
			return 1;
		case 0xED:
			GCAccountPassChangeRecv((PMSG_ACCOUNT_PASS_CHANGE_RECV*)lpMsg);
			return 1;
		}
		break;
	case 0xD3:
		switch (((lpMsg[0] == 0xC1) ? lpMsg[3] : lpMsg[4]))
		{
#if(CB_DANGKYINGAME)
		case 0x05:
			gCB_DangKyInGame->RecvKQRegInGame((XULY_CGPACKET*)lpMsg);
			break;
#endif
#if(CB_BXHDMG)
		case 0x2D:
			if (gCB_BXHTopDmg) gCB_BXHTopDmg->RecvProtocol(lpMsg);
			return 1;
#endif
		}
		break;
	case 0xFA:
		switch (((lpMsg[0] == 0xC1) ? lpMsg[3] : lpMsg[4]))
		{
		case 0xD0: // Spectator /watch
		{
			PMSG_SPECTATOR_MODE_RECV* pMsg = (PMSG_SPECTATOR_MODE_RECV*)lpMsg;
			if (pMsg->Enable)
			{
				WORD targetIndex = MAKE_NUMBERW(pMsg->TargetIndex[0], pMsg->TargetIndex[1]);
				gSpectatorMode.Start(pMsg->TargetName, targetIndex, pMsg->Map, pMsg->X, pMsg->Y);
			}
			else
			{
				gSpectatorMode.Stop();
			}
			return 1;
		}
		}
		break;
	case 0xFB:
		switch (((lpMsg[0] == 0xC1) ? lpMsg[3] : lpMsg[4]))
		{
		case 0x30:
			gSS.PrintScreen();
			return 1;
		}
		break;

	}
	return false;
}

