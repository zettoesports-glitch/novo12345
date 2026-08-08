#include "stdafx.h"
#include "NewUISystem.h"
#include "NewUIRankingTop.h"
#include "ZzzTexture.h"
#include "wsclientinline.h"
#include "CharacterManager.h"
#include "DSPlaySound.h"
#include "UIBaseDef.h"
#include "_TextureIndex.h"
#include "CBInterface.h"
#include "GameShop/NewUIInGameShop.h"
#include "Protocol.h"

using namespace SEASON3B;

namespace
{
	// Viewport do PhotoViewer: precisa caber asas/armas (antes 160x220 cortava)
	const float kCharW = 200.f;
	const float kCharH = 320.f;
	const float kCharZoom = 0.52f;   // menor = boneco inteiro no quadro
	const float kCharHeight = 15.f; // sobe um pouco o enquadramento (pernas/asas)
	const float kRowH = 18.f;
	const float kHeaderH = 20.f;
}

SEASON3B::CNewUIRankingTop::CNewUIRankingTop()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;

	is_request = false;
	m_RankListView = LIST_VIEW;
	m_RankMaxTop = 0;
	m_RankIndexCur = 0;
	m_RankSelectIndex = -1;
	m_LastPreviewTick = 0;
	memset(this->m_RankColum, 0, sizeof(this->m_RankColum));
	memset(this->m_RankName, 0, sizeof(this->m_RankName));
	memset(this->m_PreviewName, 0, sizeof(this->m_PreviewName));
}

SEASON3B::CNewUIRankingTop::~CNewUIRankingTop()
{
	Release();
}

bool SEASON3B::CNewUIRankingTop::Create(CNewUIManager* pNewUIMng, float x, float y)
{
	if (!pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(INTERFACE_RANKING_TOP, this);

	this->LoadImages();
	this->SetPos(x, y);
	this->SetInfo();
	this->Show(false);
	return true;
}

void SEASON3B::CNewUIRankingTop::Release()
{
	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		this->UnloadImages();
		m_RankList.clear();
		m_pNewUIMng = NULL;
	}
}

float SEASON3B::CNewUIRankingTop::ListLeft() const
{
	return (float)m_Pos.x + 20.f + kCharW + 16.f;
}

float SEASON3B::CNewUIRankingTop::ListTop() const
{
	return (float)m_Pos.y + 58.f;
}

float SEASON3B::CNewUIRankingTop::ListWidth() const
{
	// room for scrollbar on the right
	return (float)WINDOW_W - (20.f + kCharW + 16.f) - 36.f;
}

float SEASON3B::CNewUIRankingTop::ListHeight() const
{
	return kHeaderH + (float)m_RankListView * kRowH + 8.f;
}

void SEASON3B::CNewUIRankingTop::SetInfo()
{
	m_RenderCharacter.Init(0);
	m_RenderCharacter.SetArrangeType(1, 119, 30);
	// Viewport maior = menos clip de asa/arma (RenderPhotoCharacter usa m_iWidth/m_iHeight)
	m_RenderCharacter.SetSize((int)kCharW, (int)kCharH);
	m_RenderCharacter.CopyPlayer();
	m_RenderCharacter.SetAutoupdatePlayer(TRUE);
	m_RenderCharacter.SetAnimation(AT_STAND1);
	m_RenderCharacter.SetAngle(90.f);
	m_RenderCharacter.SetZoom(kCharZoom);
	m_RenderCharacter.SetHeight(kCharHeight);

	const float scrollX = ListLeft() + ListWidth() + 4.f;
	const float scrollY = ListTop() + kHeaderH;
	const int scrollH = (int)((float)m_RankListView * kRowH);
	m_pScrollBar.Create((int)scrollX, (int)scrollY, scrollH);
	m_pScrollBar.SetPercent(0.0);
}

void SEASON3B::CNewUIRankingTop::SetPos(float x, float y)
{
	m_Pos.x = (LONG)x;
	m_Pos.y = (LONG)y;
	// Um pouco mais baixo no painel para asas superiores caberem no viewport
	m_RenderCharacter.SetPosition(x + 16.f, y + 48.f);
	m_RenderCharacter.SetSize((int)kCharW, (int)kCharH);

	const float scrollX = ListLeft() + ListWidth() + 4.f;
	const float scrollY = ListTop() + kHeaderH;
	m_pScrollBar.SetPos(scrollX, scrollY);
}

void SEASON3B::CNewUIRankingTop::LoadImages()
{
	// Menu Custom chrome
	LoadBitmap("Interface\\HUD\\bg250.tga", IMAGE_BG_SERVER_INTERFACE_250, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\timerserver-bar.tga", IMAGE_TITTLE_SERVER_INTERFACE, GL_LINEAR);
	LoadBitmap("Interface\\HUD\\button_close_navi.tga", IMAGE_BUTTON_CLOSE_NAVI, GL_LINEAR);

	// Same page L/R buttons as Event board (InGameShop assets)
	LoadBitmap("Interface\\InGameShop\\ingame_Bt_page_L.tga", CNewUIInGameShop::IMAGE_IGS_PAGE_LEFT, GL_LINEAR);
	LoadBitmap("Interface\\InGameShop\\ingame_Bt_page_R.tga", CNewUIInGameShop::IMAGE_IGS_PAGE_RIGHT, GL_LINEAR);

	// VIP badges
	LoadBitmap("Interface\\HUD\\VipLevel1.tga", IMAGE_TOP_LEVEL1, GL_LINEAR, GL_CLAMP_TO_EDGE, true, false);
	LoadBitmap("Interface\\HUD\\VipLevel2.tga", IMAGE_TOP_LEVEL2, GL_LINEAR, GL_CLAMP_TO_EDGE, true, false);
	LoadBitmap("Interface\\HUD\\VipLevel3.tga", IMAGE_TOP_LEVEL3, GL_LINEAR, GL_CLAMP_TO_EDGE, true, false);
}

void SEASON3B::CNewUIRankingTop::UnloadImages()
{
	DeleteBitmap(IMAGE_TOP_LEVEL1);
	DeleteBitmap(IMAGE_TOP_LEVEL2);
	DeleteBitmap(IMAGE_TOP_LEVEL3);
}

bool SEASON3B::CNewUIRankingTop::UpdateKeyEvent()
{
	if (IsVisible() == true)
	{
		if (SEASON3B::IsPress(VK_ESCAPE))
		{
			g_pNewUISystem->Hide(INTERFACE_RANKING_TOP);
			return false;
		}

		if (SEASON3B::IsRelease(VK_LEFT))
		{
			this->RequestServerRankingInfo((BYTE)(m_RankIndexCur - 1));
			return false;
		}
		if (SEASON3B::IsRelease(VK_RIGHT))
		{
			this->RequestServerRankingInfo((BYTE)(m_RankIndexCur + 1));
			return false;
		}
	}

	return true;
}

bool SEASON3B::CNewUIRankingTop::UpdateMouseEvent()
{
	const float w = (float)WINDOW_W;
	const float h = (float)WINDOW_H;

	if (!SEASON3B::CheckMouseIn((float)m_Pos.x, (float)m_Pos.y, w, h))
		return true;

	// Close (Menu Custom position)
	const float closeX = (float)m_Pos.x + w - 22.f;
	const float closeY = (float)m_Pos.y + 8.f;
	if (SEASON3B::CheckMouseIn(closeX, closeY, 16.f, 16.f) && SEASON3B::IsRelease(VK_LBUTTON))
	{
		g_pNewUISystem->Hide(INTERFACE_RANKING_TOP);
		PlayBuffer(SOUND_CLICK01);
		return false;
	}

	const float listL = ListLeft();
	const float listT = ListTop() + kHeaderH;
	const float listW = ListWidth();
	const float listH = (float)m_RankListView * kRowH;

	size_t secure = 0;
	size_t current = 0;
	size_t good_count = m_RankList.size();

	if (good_count > m_RankListView)
	{
		double prev = m_pScrollBar.GetPercent();

		if (SEASON3B::CheckMouseIn(listL, listT, listW + 20.f, listH))
		{
			if (MouseWheel <= 0)
			{
				if (MouseWheel < 0)
				{
					MouseWheel = 0;
					prev += 0.1;
					if (prev > 1.0)
						prev = 1.0;
					m_pScrollBar.SetPercent(prev);
				}
			}
			else
			{
				MouseWheel = 0;
				prev -= 0.1;
				if (prev < 0.0)
					prev = 0.0;
				m_pScrollBar.SetPercent(prev);
			}
		}
		m_pScrollBar.UpdateMouseEvent();
		current = (size_t)((double)(good_count - m_RankListView) * prev);
	}

	for (size_t i = current; i < good_count && secure < m_RankListView; i++, secure++)
	{
		const float rowY = listT + (float)secure * kRowH;
		if (SEASON3B::CheckMouseIn(listL, rowY, listW, kRowH) && SEASON3B::IsRelease(VK_LBUTTON))
		{
			m_RankSelectIndex = i;
			// Fase 2: carregar set visual do personagem clicado
			if (i < m_RankList.size())
			{
				RequestRankingPreview(m_RankList[i].GetName());
				PlayBuffer(SOUND_CLICK01);
			}
		}
	}

	m_RenderCharacter.DoMouseAction();

	return false; // block click-through while open
}

bool SEASON3B::CNewUIRankingTop::Render()
{
	EnableAlphaTest(true);
	glColor4f(1.f, 1.f, 1.f, 1.f);

	this->RenderFrame();
	this->RenderTexte();

	DisableAlphaBlend();

	m_RenderCharacter.Render();

	return true;
}

bool SEASON3B::CNewUIRankingTop::Update()
{
	if (IsVisible())
	{
		m_pScrollBar.Update();
	}
	return true;
}

float SEASON3B::CNewUIRankingTop::GetLayerDepth()
{
	return 10.0f;
}

void SEASON3B::CNewUIRankingTop::OpenningProcess()
{
	// Center larger window
	const float x = PositionX_The_Mid((float)WINDOW_W);
	const float y = PositionY_The_Mid((float)WINDOW_H);
	SetPos(x, y);

	is_request = false;
	m_RankIndexCur = 0;

	SendRequestMaxRanking();
	RequestServerRankingInfo(m_RankIndexCur);

	m_pScrollBar.SetPercent(0.0);
	m_PreviewName[0] = '\0';
	m_RenderCharacter.SetAutoupdatePlayer(TRUE);
	m_RenderCharacter.CopyPlayer();
}

void SEASON3B::CNewUIRankingTop::ClosingProcess()
{
}

void SEASON3B::CNewUIRankingTop::RenderFrame()
{
	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;
	const float w = (float)WINDOW_W;
	const float h = (float)WINDOW_H;

	// === Menu Custom UI chrome ===
	RenderImageF(IMAGE_BG_SERVER_INTERFACE_250, x, y, w, h, 0.0, 0.0, 250.0, 250.0);

	// Titulo vermelho: nome do rank atual (ex: Reset, Level, BC...) — nao o texto fixo "Ranking"
	const float titleW = 180.f;
	const float titleH = 24.f;
	const char* titleText = (m_RankName[0] != '\0') ? m_RankName : "Ranking";
	RenderImageF(IMAGE_TITTLE_SERVER_INTERFACE,
		x + ((w - titleW) * 0.5f), y + 12.f, titleW, titleH, 0.0, 0.0, 223.0, 32.0);

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetTextColor(CLRDW_YELLOW);
	g_pRenderText->RenderText(
		x + ((w - titleW) * 0.5f), y + 12.f, titleText, (int)titleW, (int)titleH, RT3_SORT_CENTER);

	// Nome do personagem em preview (se clicou)
	if (m_PreviewName[0] != 0 && m_RenderCharacter.GetID() && m_RenderCharacter.GetID()[0])
	{
		g_pRenderText->SetFont(g_hFont);
		g_pRenderText->SetTextColor(CLRDW_GOLD);
		g_pRenderText->RenderText(x + 16.f, y + 42.f, m_PreviewName, (int)kCharW, 14, RT3_SORT_CENTER);
	}

	// Close
	const float closeX = x + w - 22.f;
	const float closeY = y + 8.f;
	RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, closeX, closeY, 16.0f, 16.0f, 0.0, 0.0, 15.0, 15.0);
	if (SEASON3B::CheckMouseIn(closeX, closeY, 16.0f, 16.0f))
	{
		glColor4f(0.55f, 0.55f, 0.55f, 1.0f);
		RenderImageF(IMAGE_BUTTON_CLOSE_NAVI, closeX, closeY, 16.0f, 16.0f, 0.0, 0.0, 15.0, 15.0);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void SEASON3B::CNewUIRankingTop::RenderTexte()
{
	const float x = (float)m_Pos.x;
	const float y = (float)m_Pos.y;

	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetFont(g_hFontBold);

	// (sem subtitulo "Ranking - Reset" — o nome do rank fica so na barra vermelha do titulo)
	char pszText[64];

	// Page L/R — same buttons as Event board (InGameShop style)
	const float pageY = y + (float)WINDOW_H - 38.f;
	const float pageLeftX = x + 50.f;
	const float pageRightX = x + (float)WINDOW_W - 70.f;
	const int maxPage = (m_RankMaxTop > 0) ? (int)m_RankMaxTop : 1;
	const int curPage = (int)m_RankIndexCur + 1;

	g_pRenderText->SetFont(g_hFont);
	g_pRenderText->SetTextColor(CLRDW_GOLD);
	sprintf_s(pszText, "%d / %d", curPage, maxPage);
	g_pRenderText->RenderText(
		(int)(x + (float)WINDOW_W * 0.5f), (int)(pageY + 4.f),
		pszText, 0, 0, RT3_WRITE_CENTER);

	if (m_RankIndexCur > 0 && gInterface != NULL)
	{
		if (gInterface->DrawButtonGUI(CNewUIInGameShop::IMAGE_IGS_PAGE_LEFT, pageLeftX, pageY, 20, 23, 3))
		{
			RequestServerRankingInfo((BYTE)(m_RankIndexCur - 1));
			PlayBuffer(SOUND_CLICK01);
		}
	}
	if (m_RankMaxTop > 0 && (m_RankIndexCur + 1) < m_RankMaxTop && gInterface != NULL)
	{
		if (gInterface->DrawButtonGUI(CNewUIInGameShop::IMAGE_IGS_PAGE_RIGHT, pageRightX, pageY, 20, 23, 3))
		{
			RequestServerRankingInfo((BYTE)(m_RankIndexCur + 1));
			PlayBuffer(SOUND_CLICK01);
		}
	}

	// Column headers
	const float listL = ListLeft();
	const float listT = ListTop();
	const float listW = ListWidth();

	const float colNum = 30.f;
	const float colName = listW * 0.32f;
	const float colClass = listW * 0.30f;
	const float colScore = listW - colNum - colName - colClass;

	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetBgColor(52, 57, 67, 255);
	g_pRenderText->SetTextColor(CLRDW_WHITE);

	float hx = listL;
	g_pRenderText->RenderFont((int)hx, (int)listT, "#", (int)colNum, (int)kHeaderH, RT3_SORT_CENTER);
	hx += colNum;
	g_pRenderText->RenderFont((int)hx, (int)listT, GlobalText[1389], (int)colName, (int)kHeaderH, RT3_SORT_CENTER); // name
	hx += colName;
	g_pRenderText->RenderFont((int)hx, (int)listT, GlobalText[1973], (int)colClass, (int)kHeaderH, RT3_SORT_CENTER); // class
	hx += colClass;
	g_pRenderText->RenderFont((int)hx, (int)listT, this->m_RankColum, (int)colScore, (int)kHeaderH, RT3_SORT_CENTER); // score
	g_pRenderText->SetBgColor(0);

	// Rows
	size_t secure = 0;
	size_t current = 0;
	size_t good_count = m_RankList.size();

	if (good_count > m_RankListView)
	{
		double prev = m_pScrollBar.GetPercent();
		current = (size_t)((double)(good_count - m_RankListView) * prev);
	}

	float rowY = listT + kHeaderH;

	for (size_t i = current; i < good_count && secure < m_RankListView; i++, secure++)
	{
		float rx = listL;

		if (SEASON3B::CheckMouseIn(listL, rowY, listW, kRowH) || m_RankSelectIndex == i)
		{
			EnableAlphaTest(true);
			glColor4ub(79, 86, 100, 255);
			RenderColor(listL, rowY, listW, kRowH - 1.f);
			EndRenderColor();
		}

		g_pRenderText->SetTextColor(CLRDW_WHITE);

		sprintf_s(pszText, "%02d", (int)i + 1);
		g_pRenderText->RenderFont((int)rx, (int)rowY, pszText, (int)colNum, (int)kRowH, RT3_SORT_CENTER);
		rx += colNum;

		BYTE LevelVip = m_RankList[i].GetVip();
		if (LevelVip > 0 && LevelVip <= 3)
		{
			GLuint vipImg = IMAGE_TOP_LEVEL1 + (LevelVip - 1);
			SEASON3B::RenderImage(vipImg, rx, rowY + 3.f, 12.f, 12.f, 0.0, 0.0, 1.f, 1.f, -1);
			g_pRenderText->RenderFont((int)(rx + 14.f), (int)rowY, m_RankList[i].GetName(), (int)(colName - 14.f), (int)kRowH, RT3_SORT_LEFT);
		}
		else
		{
			g_pRenderText->RenderFont((int)(rx + 4.f), (int)rowY, m_RankList[i].GetName(), (int)(colName - 4.f), (int)kRowH, RT3_SORT_LEFT);
		}
		rx += colName;

		g_pRenderText->RenderFont((int)rx, (int)rowY, m_RankList[i].GetClass(), (int)colClass, (int)kRowH, RT3_SORT_CENTER);
		rx += colClass;

		sprintf_s(pszText, "%u", m_RankList[i].GetScore());
		g_pRenderText->RenderFont((int)rx, (int)rowY, pszText, (int)colScore, (int)kRowH, RT3_SORT_CENTER);

		rowY += kRowH;
	}

	if (good_count > m_RankListView)
	{
		m_pScrollBar.Render();
	}
}

void SEASON3B::CNewUIRankingTop::ReceiveRankingInfo(BYTE* ReceiveBuffer)
{
	LPPWHEADER_DEFAULT_WORD Data = (LPPWHEADER_DEFAULT_WORD)ReceiveBuffer;

	m_RankSelectIndex = -1;
	if (m_RankMaxTop == 0 && Data->Value != 0)
	{
		m_RankMaxTop = Data->Value;
		RequestServerRankingInfo(m_RankIndexCur);
	}
	else
	{
		m_RankMaxTop = Data->Value;
	}
}

void SEASON3B::CNewUIRankingTop::ReceiveRankingListInfo(BYTE* ReceiveBuffer)
{
	LPPHEADER_RANKING_LIST Data = (LPPHEADER_RANKING_LIST)ReceiveBuffer;

	m_RankList.clear();
	m_pScrollBar.SetPercent(0.0);

	this->m_RankIndexCur = Data->index;

	m_RankSelectIndex = -1;
	m_PreviewName[0] = '\0';
	// Volta preview para o proprio char ao trocar pagina do rank
	m_RenderCharacter.SetAutoupdatePlayer(TRUE);
	m_RenderCharacter.CopyPlayer();

	memset(this->m_RankName, 0, sizeof(this->m_RankName));
	strcpy_s(this->m_RankName, Data->rankname);
	strcpy_s(this->m_RankColum, Data->col2);

	int offset = sizeof(PHEADER_RANKING_LIST);

	for (int n = 0; n < Data->count; n++)
	{
		LPPCREATE_RANKING_INFO Data2 = (LPPCREATE_RANKING_INFO)(ReceiveBuffer + offset);

		BYTE Class = gCharacterManager.ChangeServerClassTypeToClientClassType(Data2->PlayerClass);

		m_RankList.push_back(TEMPLATE_RANKING(
			Data2->Name,
			gCharacterManager.GetCharacterClassText(Class),
			Data2->LevelVip,
			Data2->TotalScore));

		offset += (sizeof(PCREATE_RANKING_INFO));
	}

	std::sort(m_RankList.begin(), m_RankList.end(), [](const TEMPLATE_RANKING& a, const TEMPLATE_RANKING& b) {
		return a.Score > b.Score;
		});

	is_request = false;
}

void SEASON3B::CNewUIRankingTop::RequestServerRankingInfo(BYTE Index)
{
	if (is_request == false && (Index >= 0 && Index < m_RankMaxTop))
	{
		is_request = true;
		SendRequestRankingInfo(Index);
	}
}

void SEASON3B::CNewUIRankingTop::RequestRankingPreview(const char* name)
{
	if (!name || !name[0])
		return;

	char clean[16] = { 0 };
	strncpy_s(clean, name, _TRUNCATE);
	// trim
	int n = (int)strlen(clean);
	while (n > 0 && (clean[n - 1] == ' ' || clean[n - 1] == '\t'))
		clean[--n] = '\0';

	if (clean[0] == '\0')
		return;

	// Ja mostrando esse personagem: nao puxa de novo
	if (m_PreviewName[0] != '\0' && _stricmp(m_PreviewName, clean) == 0)
		return;

	// Cooldown 5s entre previews diferentes (protege GS/DS e evita lag)
	const DWORD now = GetTickCount();
	if (m_LastPreviewTick != 0 && (now - m_LastPreviewTick) < (DWORD)PREVIEW_COOLDOWN_MS)
	{
		const int secLeft = (int)((PREVIEW_COOLDOWN_MS - (now - m_LastPreviewTick) + 999) / 1000);
		if (g_pChatListBox)
		{
			char msg[64];
			sprintf_s(msg, "Aguarde %d s para carregar outro personagem.", secLeft > 0 ? secLeft : 1);
			g_pChatListBox->AddText("", msg, SEASON3B::TYPE_ERROR_MESSAGE);
		}
		return;
	}

	m_LastPreviewTick = now;
	strncpy_s(m_PreviewName, clean, _TRUNCATE);
	SendRequestRankingPreview(clean);
}

void SEASON3B::CNewUIRankingTop::ReceiveRankingPreview(BYTE* ReceiveBuffer)
{
	if (!ReceiveBuffer || !IsVisible())
		return;

	PMSG_RANKING_PREVIEW_RECV* p = (PMSG_RANKING_PREVIEW_RECV*)ReceiveBuffer;

	char recvName[12] = { 0 };
	memcpy(recvName, p->name, 11);
	recvName[11] = '\0';
	// trim
	{
		int n = (int)strlen(recvName);
		while (n > 0 && (recvName[n - 1] == ' ' || recvName[n - 1] == '\t'))
			recvName[--n] = '\0';
	}

	if (p->result == 0)
	{
		m_PreviewName[0] = '\0';
		if (g_pChatListBox)
			g_pChatListBox->AddText("", "Nao foi possivel carregar o personagem.", SEASON3B::TYPE_ERROR_MESSAGE);
		return;
	}

	// Para de copiar o Hero e aplica o set do rank
	m_RenderCharacter.SetAutoupdatePlayer(FALSE);
	m_RenderCharacter.SetSize((int)kCharW, (int)kCharH);

	// Classe: preferir campo Class (server class do GS); fallback helper bits
	BYTE ServerClass = p->Class;
	if (ServerClass == 0)
		ServerClass = (BYTE)(p->Equipment[EQUIPMENT_HELPER] & 0x7F);

	const BYTE clientClass =
		gCharacterManager.ChangeServerClassTypeToClientClassType(ServerClass);

	CHARACTER* pc = m_RenderCharacter.GetPhotoChar();
	if (pc != NULL)
	{
		if (pc->Object.Type != MODEL_PLAYER)
			pc->Object.Type = MODEL_PLAYER;
		pc->Object.Live = true;
		pc->Object.Visible = true;

		// SetClass() da PhotoViewer so age se !initialized — setar direto
		if (pc->Class != clientClass)
		{
			pc->Class = clientClass;
			SetChangeClass(pc);
		}
		pc->SafeZone = false;
		pc->HideShadow = true;
	}

	// Aplica set visual (interno: ChangeCharacterExt + helper)
	m_RenderCharacter.SetEquipmentPacket(p->Equipment);
	m_RenderCharacter.SetID(recvName[0] ? recvName : p->name);
	m_RenderCharacter.SetAnimation(AT_STAND1);
	m_RenderCharacter.SetAngle(90.f);
	m_RenderCharacter.SetZoom(kCharZoom);
	m_RenderCharacter.SetHeight(kCharHeight);

	strncpy_s(m_PreviewName, recvName[0] ? recvName : p->name, _TRUNCATE);
}
