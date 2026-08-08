#pragma once
#include "WSclient.h"
#define SET_NUMBERHB(x) ((BYTE)((DWORD)(x)>>(DWORD)8))
#define SET_NUMBERLB(x) ((BYTE)((DWORD)(x)&0xFF))
#define SET_NUMBERHW(x) ((WORD)((DWORD)(x)>>(DWORD)16))
#define SET_NUMBERLW(x) ((WORD)((DWORD)(x)&0xFFFF))
#define SET_NUMBERHDW(x) ((DWORD)((QWORD)(x)>>(QWORD)32))
#define SET_NUMBERLDW(x) ((DWORD)((QWORD)(x)&0xFFFFFFFF))

#define MAKE_NUMBERW(x,y) ((WORD)(((BYTE)((y)&0xFF))|((BYTE)((x)&0xFF)<<8)))
#define MAKE_NUMBERDW(x,y) ((DWORD)(((WORD)((y)&0xFFFF))|((WORD)((x)&0xFFFF)<<16)))
#define MAKE_NUMBERQW(x,y) ((QWORD)(((DWORD)((y)&0xFFFFFFFF))|((DWORD)((x)&0xFFFFFFFF)<<32)))
//===Move Item
struct PMSG_ITEM_MOVE_RECV
{
	PSBMSG_HEAD h;
	BYTE sFlag;
	BYTE tFlag;
	BYTE Source;
	BYTE Target;
};
struct XULY_CGPACKET
{
	PSBMSG_HEAD header; // C3:F3:03
	DWORD ThaoTac;
};
struct SEND_COUNTLIST
{
	PSWMSG_HEAD header;
	int Count;
	BYTE Type;
};


struct RANK_INFO_SEND
{
	char NameRank[128];
};

extern std::vector <std::string> m_DataSelectNameTop;

// C1:FA:D0 - GM spectator mode (watch player)
struct PMSG_SPECTATOR_MODE_RECV
{
	PSBMSG_HEAD header;
	BYTE Enable;
	BYTE TargetIndex[2];
	BYTE Map;
	BYTE X;
	BYTE Y;
	char TargetName[11];
};

// C1:F3:E9 - Account item lock request (client -> GS)
struct PMSG_ACCOUNT_LOCK_SEND
{
	PSBMSG_HEAD header;
	BYTE type; // 0=lock 1=unlock 2=change
	char password[8];
	char password2[8];
};

// C1:F3:EA - Account item lock result (GS -> client)
struct PMSG_ACCOUNT_LOCK_RECV
{
	PSBMSG_HEAD header;
	BYTE result;
	BYTE locked;
	BYTE failLeft;
};

enum eAccountLockTypeClient
{
	ACCOUNT_LOCK_TYPE_LOCK = 0,
	ACCOUNT_LOCK_TYPE_UNLOCK = 1,
	ACCOUNT_LOCK_TYPE_CHANGE = 2,
};

enum eAccountLockResultClient
{
	ALR_LOCK_OK = 0,
	ALR_UNLOCK_OK = 1,
	ALR_CHANGE_OK = 2,
	ALR_STATE = 3,
	ALR_ERR_ALREADY = 10,
	ALR_ERR_NOT_LOCKED = 11,
	ALR_ERR_WRONG = 12,
	ALR_ERR_INVALID = 13,
	ALR_ERR_BLOCKED = 14,
	ALR_ERR_SAME = 15,
};

extern bool g_bAccountItemsLocked;

void SendAccountLock(BYTE type, const char* password, const char* password2 = NULL);
void GCAccountLockRecv(PMSG_ACCOUNT_LOCK_RECV* lpMsg);

// Troca senha da CONTA (login) C1:F3:EC request / C1:F3:ED result
#pragma pack(push, 1)
struct PMSG_ACCOUNT_PASS_CHANGE_SEND
{
	PSBMSG_HEAD header; // F3:EC
	char oldPass[11];
	char newPass[11];
};
struct PMSG_ACCOUNT_PASS_CHANGE_RECV
{
	PSBMSG_HEAD header; // F3:ED
	BYTE result; // 1=ok 0=senha atual errada 2=invalida 3=falha
};
#pragma pack(pop)

void SendAccountPassChange(const char* oldPass, const char* newPass);
void GCAccountPassChangeRecv(PMSG_ACCOUNT_PASS_CHANGE_RECV* lpMsg);

// Ranking preview (client -> GS) C1:F3:E5
#pragma pack(push, 1)
struct PMSG_RANKING_PREVIEW_REQ
{
	PSBMSG_HEAD header;
	char name[11];
};

// Ranking preview equip (GS -> client) C1:F3:E5
// Equipment fixo 13 slots (igual GS EQUIPMENT_NEW_LENGTH) — evita mismatch de size
struct PMSG_RANKING_PREVIEW_RECV
{
	PSBMSG_HEAD header;
	BYTE result;
	char name[11];
	BYTE Class; // server class (ja convertida no GS)
	DWORD Equipment[13];
};
#pragma pack(pop)

void SendRequestRankingPreview(const char* name);
void GCRankingPreviewRecv(PMSG_RANKING_PREVIEW_RECV* lpMsg);

BOOL ProtocolCoreEx(BYTE head, BYTE* lpMsg, int size, int key);