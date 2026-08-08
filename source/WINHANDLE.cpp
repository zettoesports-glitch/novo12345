#include "stdafx.h"
#include "resource.h"
#include "ThemidaSDK.h"

#include "UIMng.h"
#include "Nprotect.h"
#include "UIWindows.h"
#include "ScaleForm.h"
#include "wsclientinline.h"
#include "NewUISystem.h"
#include "ConnectVersionHex.h"
#include "WINHANDLE.h"
#include "Time/CFPSController.h"

CWINHANDLE::CWINHANDLE()
{
	hWnd = NULL;
	WndMode = true;
	WndActive = false;
	WndIconic = false;
	hInstance = NULL;
	wndIndex = 0;
	Shell_NotifyIcon(NIM_DELETE, &nid);


	iWinWidth = 1024;
	iWinHight = 768;
}

CWINHANDLE::~CWINHANDLE()
{
	this->Release();
}

void CWINHANDLE::Release()
{
	if (nid.cbSize != 0)
		Shell_NotifyIcon(NIM_DELETE, &nid);

	//if (this->hWnd)
	//{
	//	DestroyWindow(this->hWnd);
	//	this->hWnd = NULL;
	//}
}

// Garante area cliente = resolucao escolhida (evita 1px errado / DPI).
static void EnsureClientPixelSize(HWND hWnd, int clientW, int clientH, bool windowed)
{
	if (!hWnd || clientW <= 0 || clientH <= 0)
		return;

	RECT cr = {};
	GetClientRect(hWnd, &cr);
	const int cw = cr.right - cr.left;
	const int ch = cr.bottom - cr.top;
	if (cw == clientW && ch == clientH)
		return;

	RECT wr = { 0, 0, clientW, clientH };
	const DWORD style = (DWORD)GetWindowLong(hWnd, GWL_STYLE);
	const DWORD exStyle = (DWORD)GetWindowLong(hWnd, GWL_EXSTYLE);
	AdjustWindowRectEx(&wr, style, FALSE, exStyle);

	const int winW = wr.right - wr.left;
	const int winH = wr.bottom - wr.top;

	int x = 0;
	int y = 0;
	if (windowed)
	{
		const int screenW = GetSystemMetrics(SM_CXSCREEN);
		const int screenH = GetSystemMetrics(SM_CYSCREEN);
		x = (screenW - winW) / 2;
		y = (screenH - winH) / 2;
		if (x < 0) x = 0;
		if (y < 0) y = 0;
	}

	SetWindowPos(hWnd, HWND_TOP, x, y, winW, winH, SWP_NOZORDER | SWP_NOACTIVATE);
}

HWND CWINHANDLE::Create(HINSTANCE hCurrentInst, mu_uint32 RenderSizeX, mu_uint32 RenderSizeY)
{
	WNDCLASS wc = {};

	const char* windowName = GMProtect->GetWindowName();

	this->SetInstance(hCurrentInst);

	if (!GetClassInfo(hCurrentInst, windowName, &wc))
	{
		wc.style = CS_OWNDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = CWINHANDLE::WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hCurrentInst;
		wc.hIcon = LoadIcon(hCurrentInst, (LPCTSTR)IDI_ICON1);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = windowName;
		RegisterClass(&wc);
	}

	// Metricas fisicas com DPI awareness (nao "escaladas" pelo Windows)
	const int screenW = GetSystemMetrics(SM_CXSCREEN);
	const int screenH = GetSystemMetrics(SM_CYSCREEN);

	RECT rc = { 0, 0, (LONG)RenderSizeX, (LONG)RenderSizeY };

	if (this->CheckWndMode())
	{
		const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN;
		AdjustWindowRectEx(&rc, style, FALSE, 0);

		const int winW = rc.right - rc.left;
		const int winH = rc.bottom - rc.top;
		int posX = (screenW - winW) / 2;
		int posY = (screenH - winH) / 2;
		if (posX < 0) posX = 0;
		if (posY < 0) posY = 0;

		this->hWnd = CreateWindowEx(
			0,
			windowName,
			windowName,
			style,
			posX,
			posY,
			winW,
			winH,
			NULL,
			NULL,
			hCurrentInst,
			NULL);

		EnsureClientPixelSize(this->hWnd, (int)RenderSizeX, (int)RenderSizeY, true);
	}
	else
	{
		// Fullscreen borderless: preenche o monitor primario em pixels reais.
		// Se a resolucao escolhida for menor, centraliza (letterbox visual via fundo preto).
		int winW = (int)RenderSizeX;
		int winH = (int)RenderSizeY;
		int posX = 0;
		int posY = 0;

		// Preferir cobrir o monitor nativo para 1080p/1440p nativos sem blur
		if (winW >= screenW - 2 && winH >= screenH - 2)
		{
			winW = screenW;
			winH = screenH;
			posX = 0;
			posY = 0;
			// sincroniza buffers OpenGL com o monitor real
			RenderSizeX = (mu_uint32)screenW;
			RenderSizeY = (mu_uint32)screenH;
			this->InitSize(RenderSizeX, RenderSizeY);
		}
		else
		{
			posX = (screenW - winW) / 2;
			posY = (screenH - winH) / 2;
			if (posX < 0) posX = 0;
			if (posY < 0) posY = 0;
		}

		this->hWnd = CreateWindowEx(
			WS_EX_APPWINDOW,
			windowName,
			windowName,
			WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,
			posX,
			posY,
			winW,
			winH,
			NULL,
			NULL,
			hCurrentInst,
			NULL);

		EnsureClientPixelSize(this->hWnd, winW, winH, false);
	}

	nid.cbSize = sizeof(nid);
	nid.hWnd = this->hWnd;
	nid.uID = TRAY_ID_ICON;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = TRAY_ID_MESSAGE;
	strncpy_s(nid.szTip, sizeof(nid.szTip), windowName, _TRUNCATE);
	nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

	if (nid.hIcon == NULL)
	{
		nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	}

	return this->hWnd;
}

void CWINHANDLE::Destroyer()
{
	DestroyImGuiWindow();

#ifdef SHUTDOWN_SCALEFORM_INFO
	gfxinit->runtime_disconnect();
#endif // SHUTDOWN_SCALEFORM_INFO

	DestroyWindow();

#ifdef MAX_INSTANCE_GAME
	GMProtect->runtime_delete_mutex();
#endif // MAX_INSTANCE_GAME
}

void CWINHANDLE::InitSize(mu_uint32 RenderSizeX, mu_uint32 RenderSizeY)
{
	WindowWidth  = RenderSizeX;
	WindowHeight = RenderSizeY;

	// ScreenType (MainInfo.ini):
	// 0 = stretch classico 640x480 (pode distorcer em 16:9)
	// 1 = escala uniforme (letterbox logico, sem distorcer UI)
	// 2 = widescreen completo — referencia 1920x1080 (rate 1.6) e escala limpa em 1440p/4K
	if (gmProtect->ScreenType == 0)
	{
		g_fScreenRate_x = (float)WindowWidth / 640.0f;
		g_fScreenRate_y = (float)WindowHeight / 480.0f;
	}
	else if (gmProtect->ScreenType == 1)
	{
		const float sx = (float)WindowWidth / 640.0f;
		const float sy = (float)WindowHeight / 480.0f;
		const float s = (sx < sy) ? sx : sy;
		g_fScreenRate_x = s;
		g_fScreenRate_y = s;
	}
	else
	{
		// Referencia HD: 1920x1080 com rate 1.6 => espaco logico 1200 x 675
		// 1080p: rate 1.6 | 1440p (2560x1440): rate 2.133... | 4K: rate 3.2
		// Mantem proporcao em X e Y (UI nítida, sem "zoom DPI" errado).
		const float kLogicalW = 1920.0f / 1.6f; // 1200
		const float kLogicalH = 1080.0f / 1.6f; // 675

		if (WindowWidth > 0 && WindowHeight > 0)
		{
			g_fScreenRate_x = (float)WindowWidth / kLogicalW;
			g_fScreenRate_y = (float)WindowHeight / kLogicalH;
		}
		else
		{
			g_fScreenRate_x = 1.6f;
			g_fScreenRate_y = 1.6f;
		}

		// Evita rate zero/negativo
		if (g_fScreenRate_x < 0.5f) g_fScreenRate_x = 0.5f;
		if (g_fScreenRate_y < 0.5f) g_fScreenRate_y = 0.5f;
	}

	iWinWidth = ((float)WindowWidth / g_fScreenRate_x);
	iWinHight = ((float)WindowHeight / g_fScreenRate_y);
}

void CWINHANDLE::SetFontSize(mu_uint32 FontSize)
{
	if (FontHeight != FontSize || FontHeight == -1)
	{
		int iFontSize = FontSize - 1;

		SAFE_DELETE_OBJET(g_hFont);
		SAFE_DELETE_OBJET(g_hFontBold);
		SAFE_DELETE_OBJET(g_hFontBig);
		SAFE_DELETE_OBJET(g_hFixFont);

		const char* fontname = res_setting.Getfontfamily();

		g_hFont = CreateFont(iFontSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontname);

		g_hFontBold = CreateFont(iFontSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontname);

		g_hFontBig = CreateFont(iFontSize * 2, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontname);

		g_hFixFont = CreateFont(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontname);

		ResolutionConfig* conf = this->LoadCurrentConfig();

		if (conf->fontsize != FontSize)
		{
			conf->fontsize = FontSize;
		}

		FontHeight = conf->fontsize;
	}
}

void CWINHANDLE::Resize(mu_uint32 RenderSizeX, mu_uint32 RenderSizeY)
{
	if (!this->GethWnd())
		return;

	this->InitSize(RenderSizeX, RenderSizeY);

	if (this->CheckWndMode())
	{
		RECT rc = { 0, 0, (LONG)RenderSizeX, (LONG)RenderSizeY };
		const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN;
		AdjustWindowRectEx(&rc, style, FALSE, 0);
		const int winW = rc.right - rc.left;
		const int winH = rc.bottom - rc.top;
		const int screenW = GetSystemMetrics(SM_CXSCREEN);
		const int screenH = GetSystemMetrics(SM_CYSCREEN);
		int posX = (screenW - winW) / 2;
		int posY = (screenH - winH) / 2;
		if (posX < 0) posX = 0;
		if (posY < 0) posY = 0;
		SetWindowPos(this->GethWnd(), NULL, posX, posY, winW, winH, SWP_NOZORDER | SWP_NOACTIVATE);
		EnsureClientPixelSize(this->GethWnd(), (int)RenderSizeX, (int)RenderSizeY, true);
	}
	else
	{
		const int screenW = GetSystemMetrics(SM_CXSCREEN);
		const int screenH = GetSystemMetrics(SM_CYSCREEN);
		int winW = (int)RenderSizeX;
		int winH = (int)RenderSizeY;
		int posX = 0;
		int posY = 0;

		if (winW >= screenW - 2 && winH >= screenH - 2)
		{
			winW = screenW;
			winH = screenH;
			this->InitSize((mu_uint32)winW, (mu_uint32)winH);
		}
		else
		{
			posX = (screenW - winW) / 2;
			posY = (screenH - winH) / 2;
			if (posX < 0) posX = 0;
			if (posY < 0) posY = 0;
		}

		SetWindowPos(this->GethWnd(), HWND_TOP, posX, posY, winW, winH, SWP_NOZORDER | SWP_NOACTIVATE);
		EnsureClientPixelSize(this->GethWnd(), winW, winH, false);
	}
}

MSG CWINHANDLE::winLoop()
{
	MSG msg = {};

	// Professional frame pacer — sole software FPS limiter for the client.
	gFPSController->Initialize();

	while (msg.message != WM_QUIT)
	{
		this->UpdateWndActive();

		// Only pace when we actually render a frame (not when pumping messages only).
		bool renderedFrame = false;

		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					break;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (msg.message == WM_QUIT)
				break;
		}
		else
		{
			if (this->CheckWndMode() || this->CheckWndActive())
			{
				gFPSController->BeginFrame();
				Scene(g_hDC);
				gFPSController->EndFrame(); // absolute QPC schedule: sleep + spin
				renderedFrame = true;
			}
			else if (!this->CheckWndMode())
			{
				if (GetForegroundWindow() != this->GethWnd())
				{
					SetForegroundWindow(this->GethWnd());
					SetFocus(this->GethWnd());
				}

				if (g_iInactiveWarning < 1)
				{
					g_iInactiveWarning++;
					g_bMinimizedEnabled = TRUE;
					
					if (IsIconic(this->GethWnd()))
						ShowWindow(this->GethWnd(), SW_RESTORE);
					else
						ShowWindow(this->GethWnd(), SW_MINIMIZE);

					g_bMinimizedEnabled = FALSE;
				}
				else
				{
					// Evitar fechar o jogo ao minimizar/perder o foco no modo janela
					ShowWindow(this->GethWnd(), SW_MINIMIZE);
				}
			}
		}

		(void)renderedFrame;

		ProtocolCompiler();
		g_pChatRoomSocketList->ProtocolCompile();
	}

	gFPSController->Shutdown();

	return msg;
}

HWND CWINHANDLE::GethWnd()
{
	return hWnd;
}

void CWINHANDLE::SetInstance(HINSTANCE hInst)
{
	hInstance = hInst;
}

HINSTANCE CWINHANDLE::GetInstance()
{
	return hInstance;
}

void CWINHANDLE::SetWndActive(mu_boolean bActive)
{
	WndActive = bActive;
}

mu_boolean CWINHANDLE::CheckWndActive()
{
	return WndActive;
}

void CWINHANDLE::UpdateWndActive()
{
	WndActive = (GetForegroundWindow() == this->GethWnd());
}

void CWINHANDLE::SetWndMode(mu_boolean bActive)
{
	WndMode = bActive;
}

mu_boolean CWINHANDLE::CheckWndMode()
{
	return WndMode;
}

void CWINHANDLE::SetDisplayIndex(mu_uint8 index, mu_boolean Fontload)
{
	wndIndex = index;

	ResolutionConfig* conf = this->LoadCurrentConfig();

	this->InitSize(conf->width, conf->height);

	this->SetFontSize(conf->fontsize);


	if (Fontload == false)
	{
		this->Resize(conf->width, conf->height);
	}
}

mu_uint8 CWINHANDLE::GetDisplayIndex()
{
	return wndIndex;
}

mu_uint8 CWINHANDLE::GetDisplayIndex(const std::string text_name)
{
	return res_setting.FindInfoByIndex(text_name);
}

mu_float CWINHANDLE::GetScreenX()
{
	return iWinWidth;
}

mu_float CWINHANDLE::GetScreenY()
{
	return iWinHight;
}

ResolutionConfig* CWINHANDLE::LoadCurrentConfig()
{
	return res_setting.FindInfoByIndex(wndIndex);
}

void CWINHANDLE::Check_State()
{
	if (this->CheckWndMode())
	{
		this->Change_State(IsWindowVisible(hWnd));
	}
}

void CWINHANDLE::Change_State(mu_boolean bActive)
{
	if (this->CheckWndMode() && nid.cbSize != 0)
	{
		WndIconic = bActive;
		Shell_NotifyIcon((bActive ? NIM_ADD : NIM_DELETE), &nid);
		ShowWindow(hWnd, (bActive ? SW_HIDE : SW_SHOW));
	}
}

mu_boolean CWINHANDLE::CheckWndIconic()
{
	return WndIconic && !IsWindowVisible(hWnd);
}

mu_boolean CWINHANDLE::CheckPerformance()
{
	return !this->CheckWndIconic();
}

void CWINHANDLE::SendWindowMessage(const char* format, bool destroyAfter, ...)
{
	va_list args;
	char szMessage[512];
	va_start(args, destroyAfter);
	vsprintf(szMessage, format, args);
	va_end(args);

	g_ErrorReport.Write(szMessage);

	MessageBox(this->GethWnd(), szMessage, NULL, MB_OK);

	if (destroyAfter)
	{
		this->SendPostMessage(WM_DESTROY, 0, 0);
	}
}

void CWINHANDLE::SendNowMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	SendMessage(this->GethWnd(), Msg, wParam, lParam);
}

void CWINHANDLE::SendPostMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PostMessage(this->GethWnd(), Msg, wParam, lParam);
}

CWINHANDLE* CWINHANDLE::Instance()
{
	static CWINHANDLE s_Instance;
	return &s_Instance;
}

LONG CWINHANDLE::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef IMPLEMENT_IMGUI_WIN32
	ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
#endif // IMPLEMENT_IMGUI_WIN32

#ifdef SHUTDOWN_SCALEFORM_INFO
	if (SceneFlag == LOG_IN_SCENE || SceneFlag == CHARACTER_SCENE || SceneFlag == MAIN_SCENE)
	{
		gfxinit->CallBack(hwnd, msg, wParam, lParam);
	}
#endif // SHUTDOWN_SCALEFORM_INFO

	switch (msg)
	{
	case WM_SYSKEYDOWN:
	{
		return 0;
	}
	break;
	case WM_SYSCOMMAND:
	{
		if (wParam == SC_KEYMENU || wParam == SC_SCREENSAVE)
		{
			return 0;
		}
	}
	break;
	case WM_KEYDOWN:
		if (wParam == VK_F12)
		{
			gwinhandle->Check_State();
		}
		break;
	case TRAY_ID_MESSAGE:
		if (wParam == TRAY_ID_ICON)
		{
			switch (LOWORD(lParam))
			{
			case WM_LBUTTONDOWN:
				gwinhandle->Change_State(false);
				break;
			case WM_RBUTTONDOWN:
				// Mostrar men� contextual si lo necesitas
				break;
			}
		}
		break;
	case WM_ACTIVATE:
		gwinhandle->SetWndActive(LOWORD(wParam) != WA_INACTIVE);

		if (LOWORD(wParam) == WA_INACTIVE)
		{
			if (gwinhandle->CheckWndMode())
			{
				MouseLButton = false;
				MouseLButtonPop = false;
				MouseRButton = false;
				MouseRButtonPop = false;
				MouseRButtonPush = false;
				MouseLButtonDBClick = false;
				MouseMButton = false;
				MouseMButtonPop = false;
				MouseMButtonPush = false;
				MouseWheel = false;
			}
		}
		break;
	case WM_TIMER:
		switch (wParam)
		{
		case HACK_TIMER:
			VM_START;
			CheckHack();
			VM_END;
			break;
		case WINDOWMINIMIZED_TIMER:
			// gwinhandle->SendPostMessage(WM_DESTROY, 0, 0);
			break;
		case CHATCONNECT_TIMER:
			g_pFriendMenu->SendChatRoomConnectCheck();
			break;
		case SLIDEHELP_TIMER:
			if (gwinhandle->CheckWndMode())
			{
				if (g_pSlideHelpMgr)
					g_pSlideHelpMgr->CreateSlideText();
			}
			break;
		}
		break;
	case WM_USER_MEMORYHACK:
		KillGLWindow();
		break;
	case WM_NPROTECT_EXIT_TWO:
		SendHackingChecked(0x04, 0);
		SetTimer(gwinhandle->GethWnd(), WINDOWMINIMIZED_TIMER, 1 * 1000, NULL);
		MessageBox(NULL, GlobalText[16], "Error", MB_OK);
		break;
	case WM_ASYNCSELECTMSG:
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_CONNECT:
			break;
		case FD_READ:
			SocketClient.nRecv();
			break;
		case FD_WRITE:
			SocketClient.FDWriteSend();
			break;
		case FD_CLOSE:
			g_pChatListBox->AddText("", GlobalText[3], SEASON3B::TYPE_SYSTEM_MESSAGE);
			SocketClient.Close();
			CUIMng::Instance().PopUpMsgWin(MESSAGE_SERVER_LOST);
			break;
		}
		break;
	case WM_CTLCOLOREDIT:
		SetBkColor((HDC)wParam, RGB(0, 0, 0));
		SetTextColor((HDC)wParam, RGB(255, 255, 255));
		return (LRESULT)GetStockObject(BLACK_BRUSH);
		break;
	case WM_ERASEBKGND:
		return TRUE;
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
	}
	return 0;
	break;
	case WM_CLOSE:
	case WM_DESTROY:
	{
		Destroy = true;
		SocketClient.Close();
		DestroySound();
		KillGLWindow();
		CloseMainExe();
		GMConnectHex->OpenUpdater();
		PostQuitMessage(0);
	}
	break;
	case WM_SETCURSOR:
		ShowCursor(false);
		break;
	case WM_SIZE:
		if (SIZE_MINIMIZED == wParam)
		{
			if (!gwinhandle->CheckWndMode())
			{
				if (!g_bMinimizedEnabled)
				{
					DWORD dwMess[SIZE_ENCRYPTION_KEY];
					for (int i = 0; i < SIZE_ENCRYPTION_KEY; ++i)
					{
						dwMess[i] = GetTickCount();
					}
					g_SimpleModulusCS.LoadKeyFromBuffer((BYTE*)dwMess, FALSE, FALSE, FALSE, TRUE);
				}
			}
			else
			{
				gwinhandle->Change_State(false);
			}
		}
		break;
	default:
		if (msg >= WM_CHATROOMMSG_BEGIN && msg < WM_CHATROOMMSG_END)
			g_pChatRoomSocketList->ProcessSocketMessage(msg - WM_CHATROOMMSG_BEGIN, WSAGETSELECTEVENT(lParam));
		break;
	}

	MouseLButtonDBClick = false;
	if (MouseLButtonPop == true && (g_iMousePopPosition_x != MouseX || g_iMousePopPosition_y != MouseY))
		MouseLButtonPop = false;
	switch (msg)
	{
	case WM_MOUSEMOVE:
	{
		MouseRenderX = (float)LOWORD(lParam);
		MouseRenderY = (float)HIWORD(lParam);

		if (MouseRenderX < 0)
			MouseRenderX = 0;

		if (MouseRenderX > WindowWidth)
			MouseRenderX = WindowWidth;

		if (MouseRenderY < 0)
			MouseRenderY = 0;

		if (MouseRenderY > WindowHeight)
			MouseRenderY = WindowHeight;

		MouseX = (MouseRenderX / g_fScreenRate_x);
		MouseY = (MouseRenderY / g_fScreenRate_y);
	}
	break;
	case WM_LBUTTONDOWN:
		MouseLButtonPop = false;
		if (!MouseLButton)
			MouseLButtonPush = true;
		MouseLButton = true;
		SetCapture(gwinhandle->GethWnd());
		break;
	case WM_LBUTTONUP:
		MouseLButtonPush = false;
		MouseLButtonPop = true;
		MouseLButton = false;
		g_iMousePopPosition_x = MouseX;
		g_iMousePopPosition_y = MouseY;
		ReleaseCapture();
		break;
	case WM_RBUTTONDOWN:
		MouseRButtonPop = false;
		if (!MouseRButton)
			MouseRButtonPush = true;
		MouseRButton = true;
		SetCapture(gwinhandle->GethWnd());
		break;
	case WM_RBUTTONUP:
		MouseRButtonPush = false;
		if (MouseRButton)
			MouseRButtonPop = true;
		MouseRButton = false;
		ReleaseCapture();
		break;
	case WM_LBUTTONDBLCLK:
		MouseLButtonDBClick = true;
		break;
	case WM_MBUTTONDOWN:
		MouseMButtonPop = false;
		if (!MouseMButton)
			MouseMButtonPush = true;
		MouseMButton = true;
		SetCapture(gwinhandle->GethWnd());
		break;
	case WM_MBUTTONUP:
		MouseMButtonPush = false;
		if (MouseMButton)
			MouseMButtonPop = true;
		MouseRButton = false;
		ReleaseCapture();
		break;
	case WM_MOUSEWHEEL:
	{
		MouseWheel = (short)HIWORD(wParam) / WHEEL_DELTA;
	}
	break;
	case WM_IME_NOTIFY:
		break;
	case WM_CHAR:
	{
		switch (wParam)
		{
		case VK_RETURN:
			SetEnterPressed(true);
			break;
		}
	}
	break;
	}

	if (g_BuffSystem)
	{
		LRESULT result;
		TheBuffStateSystem().HandleWindowMessage(msg, wParam, lParam, result);
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}