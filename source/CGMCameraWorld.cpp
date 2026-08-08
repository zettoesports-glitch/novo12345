#include "stdafx.h"
#include "NewUISystem.h"
#include "ZzzInterface.h"
#include "CGMCameraWorld.h"
#include "WINHANDLE.h"
#include "TextClien.h"
#include "steady_clock.h"
#include "ZzzOpenglUtil.h"

CGMCameraWorld::CGMCameraWorld()
{
	m_Enable = 0;
	m_IsMove = 0;
	m_IsZoom = 0;
	cursor.X = 0;
	cursor.Y = 0;
	zoomPercent[0] = 40.0;
	zoomPercent[1] = 180.0;
	zoomPrecision = 2.0;
	m_fZoomProgress = 0.f;
	zoomfade = 0;
	zoomFactor = 0.f;
	desiredZoomFactor = 0.f;
	smoothed_zoom_factor = 0.f;

	m_Default = new CAMERA_INFO;
	m_Current = new CAMERA_INFO;
	m_nBackup = new CAMERA_INFO;

	m_Default->IsLoad = 0;

	m_Current->zoom = 35.0;
	m_Current->Rot.Y = -48.5;
	m_Current->Rot.Z = 150.0;
	m_Current->camAngle = 45.0;

	m_Default->zoom = 35.0;
	m_Default->Rot.Y = -48.5;
	m_Default->Rot.Z = 150.0;
	m_Default->camAngle = 45.0;
}

CGMCameraWorld::~CGMCameraWorld()
{
	delete m_Default;
	delete m_Current;
	delete m_nBackup;
}

void CGMCameraWorld::Init()
{
	ResolutionConfig* conf = gwinhandle->LoadCurrentConfig();

	// Nao escalar far plane com full screen-rate (1920/640=3 → far 8400+ e FPS <10 ao girar).
	// UI continua com g_fScreenRate_*; distancias de mundo usam escala limitada.
	float viewScale = g_fScreenRate_x;
	if (viewScale > 1.45f)
		viewScale = 1.45f;
	float nearScale = g_fScreenRate_y;
	if (nearScale > 1.45f)
		nearScale = 1.45f;

	// Widescreen: abre chao (corrige quadrado preto lateral)
	float wideBoost = 1.0f;
	if (WindowHeight > 0)
	{
		const float aspect = (float)WindowWidth / (float)WindowHeight;
		wideBoost = aspect / (4.0f / 3.0f);
		if (wideBoost < 1.0f) wideBoost = 1.0f;
		if (wideBoost > 1.70f) wideBoost = 1.70f;
	}

	m_Current->camWidthFar = 2800.0 * viewScale;
	m_Default->camWidthFar = 2800.0 * viewScale;
	//--
	m_Current->WidthFar[0] = 3600.0 * viewScale * wideBoost;
	m_Current->WidthFar[1] = 2800.0 * viewScale * wideBoost;
	m_Current->WidthFar[2] = 2000.0 * viewScale * wideBoost;
	m_Current->WidthFar[3] = 1900.0 * viewScale * wideBoost;
	m_Current->WidthFar[5] = 2600.0 * viewScale * wideBoost;
	//--
	m_Current->WidthNear[0] = 720.0 * nearScale * wideBoost;
	m_Current->WidthNear[1] = 720.0 * nearScale * wideBoost;
	m_Current->WidthNear[2] = 760.0 * nearScale * wideBoost;
	m_Current->WidthNear[3] = 720.0 * nearScale * wideBoost;
	m_Current->WidthNear[5] = 800.0 * nearScale * wideBoost;
	//--
	m_Current->camviewFar[0] = 4500.0 * viewScale;
	m_Current->camviewFar[1] = 3600.0 * viewScale;
	m_Current->camviewFar[2] = 3200.0 * viewScale;
	m_Current->camviewFar[3] = 3000.0 * viewScale;
	m_Current->camviewFar[5] = 3200.0 * viewScale;

	if (conf)
	{
		m_Current->WidthFar[4] = conf->width_far * wideBoost * 1.15f;
		m_Current->WidthNear[4] = conf->width_near * wideBoost * 1.10f;
		m_Current->camviewFar[4] = conf->view_far;
		if (m_Current->camviewFar[4] > 4500.0)
			m_Current->camviewFar[4] = 4500.0;
		if (m_Current->WidthFar[4] > 3600.0)
			m_Current->WidthFar[4] = 3600.0;
		if (m_Current->WidthNear[4] > 1500.0)
			m_Current->WidthNear[4] = 1500.0;
	}
	else
	{
		m_Current->WidthFar[4] = 2000.0 * viewScale * wideBoost;
		m_Current->WidthNear[4] = 750.0 * nearScale * wideBoost;
		m_Current->camviewFar[4] = 3400.0 * viewScale;
	}

	memcpy(m_nBackup, m_Current, sizeof(CAMERA_INFO));
	memcpy(m_Default->WidthFar, m_Current->WidthFar, sizeof(m_Default->WidthFar));
	memcpy(m_Default->WidthNear, m_Current->WidthNear, sizeof(m_Default->WidthNear));
	memcpy(m_Default->camviewFar, m_Current->camviewFar, sizeof(m_Default->camviewFar));
}

void CGMCameraWorld::Toggle()
{
	m_Enable = !m_Enable;

	if (m_Enable != 0)
		CreateNotice(gTextClien.TextClien_Khac[0], 0); //"Cam ONLINE"
	else
		CreateNotice(gTextClien.TextClien_Khac[1], 0); //"Cam OFFLINE"

	if (((m_Default->IsLoad == 0) ? (m_Default->IsLoad++) : m_Default->IsLoad) == 0)
	{
		m_Default->Rot.X = CameraAngle[2];
	}
}

void CGMCameraWorld::Backup()
{
	if (this->m_Enable != 0 && SceneFlag == MAIN_SCENE)
	{
		this->DefaultValues();
	}
}

bool CGMCameraWorld::IsEnable()
{
	return this->m_Enable!=0;
}

void CGMCameraWorld::DefaultValues()
{
	CameraAngle[2] = m_Default->Rot.X;
	m_Current->zoom = m_Default->zoom;
	m_Current->Rot.X = m_Default->Rot.X;
	m_Current->Rot.Y = m_Default->Rot.Y;
	m_Current->Rot.Z = m_Default->Rot.Z;
	m_Current->camAngle = m_Default->camAngle;
	m_Current->camWidthFar = m_Default->camWidthFar;
	memcpy(m_Current->WidthFar, m_Default->WidthFar, sizeof(m_Current->WidthFar));
	memcpy(m_Current->WidthNear, m_Default->WidthNear, sizeof(m_Current->WidthFar));
	memcpy(m_Current->camviewFar, m_Default->camviewFar, sizeof(m_Current->camviewFar));
}

void CGMCameraWorld::recover_backup_cam()
{
	memcpy(m_Current, m_nBackup, sizeof(CAMERA_INFO));
}

void CGMCameraWorld::CalcNearFar()
{
	float percent = (float)(m_Current->zoom / (m_Default->zoom / 100.f));

	m_Current->camWidthFar = (float)CalcFactor(m_Default->camWidthFar, percent) + (abs((m_Current->Rot.Z - m_Default->Rot.Z)) * 3.0f);

	for (int i = 0; i < 6; i++)
	{
		m_Current->camviewFar[i] = (float)CalcFactor(m_Default->camviewFar[i], percent) + (abs((m_Current->Rot.Z - m_Default->Rot.Z)) * 2.0f);

		m_Current->WidthFar[i] = (double)CalcFactor(m_Default->WidthFar[i], percent) + (abs((m_Current->Rot.Z - m_Default->Rot.Z)) * 2.0f);

		m_Current->WidthNear[i] = (double)CalcFactor(m_Default->WidthNear[i], percent) + (abs((m_Current->Rot.Z - m_Default->Rot.Z)) * 2.0f);
	}
	memcpy(m_nBackup, m_Current, sizeof(CAMERA_INFO));
}

void CGMCameraWorld::PreparedZoom()
{
	zoomfade = 0;

	m_IsZoom = 0;

	zoomFactor = 0.0;

	desiredZoomFactor = 0;

	m_fZoomProgress = 0.f;

	smoothed_zoom_factor = 0;

	zoomLimit[0] = (m_Default->zoom / 100.f) * zoomPercent[0];

	zoomLimit[1] = (m_Default->zoom / 100.f) * zoomPercent[1];
}

void CGMCameraWorld::ZoomInNearFar()
{
	if (!g_pNewUISystem->CheckMouseUse() && MouseOnWindow == 0
		&& SEASON3B::CheckMouseIn(0, 0, GetScreenWidth(), GetWindowsY)
		&& g_dwMouseUseUIID == 0)
	{
		if (m_Enable == 0 || SceneFlag != MAIN_SCENE)
		{
			m_IsZoom = 0;
			return;
		}

		if (MouseWheel < 0 || MouseWheel > 0)
		{
			// Start a new zoom only — do NOT touch pitch/yaw (that caused infinite pull-to-center)
			const int dir = (MouseWheel > 0) ? 1 : 2;
			const float startZoom = m_Current->zoom;

			// Refresh limits (safe if PreparedZoom never ran yet)
			zoomLimit[0] = (m_Default->zoom / 100.f) * zoomPercent[0];
			zoomLimit[1] = (m_Default->zoom / 100.f) * zoomPercent[1];

			m_IsZoom = TRUE;
			zoomFactor = startZoom;
			m_fZoomProgress = 0.f;
			smoothed_zoom_factor = startZoom;

			if (dir == 1)
				desiredZoomFactor = startZoom - zoomPrecision;
			else
				desiredZoomFactor = startZoom + zoomPrecision;

			// Clamp target to limits so animation can finish cleanly
			if (desiredZoomFactor < zoomLimit[0])
				desiredZoomFactor = zoomLimit[0];
			if (desiredZoomFactor > zoomLimit[1])
				desiredZoomFactor = zoomLimit[1];

			// Already at limit → nothing to animate
			if (fabsf(desiredZoomFactor - startZoom) < 0.001f)
			{
				m_IsZoom = FALSE;
				MouseWheel = 0;
				return;
			}

			MouseWheel = 0;
		}

		if (m_IsZoom)
		{
			// Done: snap and stop (no residual pitch loop)
			if (m_fZoomProgress >= 1.f)
			{
				m_Current->zoom = desiredZoomFactor;
				CalcNearFar();
				PreparedZoom();
				return;
			}

			// Time-based zoom only (~0.35s) — same feel at 30/60/120
			const float kZoomDurationSec = 0.35f;
			double dt = DeltaT;
			if (dt <= 0.0 || dt > 0.1)
				dt = 1.0 / 60.0;
			m_fZoomProgress += static_cast<float>(dt / kZoomDurationSec);
			if (m_fZoomProgress > 1.f)
				m_fZoomProgress = 1.f;

			smoothed_zoom_factor = hermite_interpolation(
				zoomFactor, desiredZoomFactor, desiredZoomFactor, desiredZoomFactor, m_fZoomProgress);

			// Zoom distance only — never auto-rotate pitch here
			m_Current->zoom = smoothed_zoom_factor;
			if (m_Current->zoom < zoomLimit[0])
				m_Current->zoom = zoomLimit[0];
			if (m_Current->zoom > zoomLimit[1])
				m_Current->zoom = zoomLimit[1];
			CalcNearFar();
		}
	}
}

void CGMCameraWorld::RotateInNearFar()
{
	if (m_Enable == 0 || m_IsZoom || SceneFlag != MAIN_SCENE || g_pNewUISystem->CheckMouseUse())
	{
		m_IsMove = 0;
		return;
	}

	m_IsMove = 0;

	if (SEASON3B::IsPress(VK_MBUTTON))
	{
		cursor.X = MouseX;
		cursor.Y = MouseY;
	}

	if (SEASON3B::IsRepeat(VK_MBUTTON))
		m_IsMove = TRUE;

	if (!m_IsMove) return;

	// Mouse-delta camera (degrees per pixel) — independent of FPS.
	// At high FPS deltas are smaller per frame; total degrees/second stays stable.
	float dx = (float)(MouseX - cursor.X);
	float dy = (float)(MouseY - cursor.Y);

	// Clamp extreme flicks (alt-tab / window edge) without making slow turns laggy.
	const float kMaxDelta = 64.f;
	if (dx > kMaxDelta) dx = kMaxDelta;
	else if (dx < -kMaxDelta) dx = -kMaxDelta;
	if (dy > kMaxDelta) dy = kMaxDelta;
	else if (dy < -kMaxDelta) dy = -kMaxDelta;

	// Tuned for free-look comfort at 60 and 120 equally.
	const float kYawSens = 0.26f;
	const float kPitchY = 0.095f;
	const float kPitchZ = 1.70f;

	// Light low-pass: blend toward current delta so hitch frames don't snap the camera.
	static float s_smoothDx = 0.f;
	static float s_smoothDy = 0.f;
	const float blend = 0.55f; // higher = more responsive
	s_smoothDx = s_smoothDx * (1.f - blend) + dx * blend;
	s_smoothDy = s_smoothDy * (1.f - blend) + dy * blend;
	dx = s_smoothDx;
	dy = s_smoothDy;

	if (dx != 0.f)
		SetAngleX(dx * kYawSens);

	if (dy != 0.f)
	{
		if (dy > 0.f)
		{
			if (m_Current->Rot.Y < -45.0f)
				SetAngleY(dy * kPitchY, dy * kPitchZ);
		}
		else
		{
			if (m_Current->Rot.Y > -90.0f)
				SetAngleY(dy * kPitchY, dy * kPitchZ);
		}
	}

	cursor.X = MouseX;
	cursor.Y = MouseY;
}

void CGMCameraWorld::SetAngleX(float fValue)
{
	if (m_Current->Rot.X > 309.0f || m_Current->Rot.X < -417.0f)
		m_Current->Rot.X = -45.0;
	else
		m_Current->Rot.X += fValue;

	// Yaw so muda CameraAngle[2] — near/far dependem de zoom/pitch (Rot.Z), nao recalcular.
	CameraAngle[2] = m_Current->Rot.X;
	m_Current->camAngle = -m_Current->Rot.X;
}

void CGMCameraWorld::SetAngleY(float fValue1, float fValue2)
{
	if (m_Current->Rot.Y < -45.0f || m_Current->Rot.Y > -90.0f)
	{
		m_Current->Rot.Y += fValue1;
		m_Current->Rot.Z -= fValue2;
		CalcNearFar();
	}
}

float CGMCameraWorld::hermite_interpolation(float y0, float y1, float y2, float y3, float mu)
{
	float m0, m1, mu2, mu3;
	float a0, a1, a2, a3;

	mu = sin(mu * Q_PI / 2.0f); // Aplicar EaseOutSine

	mu2 = mu * mu;
	mu3 = mu2 * mu;
	m0 = (y1 - y0) * 0.5f + (y2 - y1) * 0.5f;
	m1 = (y2 - y1) * 0.5f + (y3 - y2) * 0.5f;
	a0 = 2 * mu3 - 3 * mu2 + 1;
	a1 = mu3 - 2 * mu2 + mu;
	a2 = mu3 - mu2;
	a3 = -2 * mu3 + 3 * mu2;

	return (a0 * y1 + a1 * m0 + a2 * m1 + a3 * y2);
}

CAMERA_INFO* CGMCameraWorld::CurrentCam()
{
	return m_Current;
}

float CGMCameraWorld::GetSmoothed_zoom_factor()
{
	return m_Current->zoom / m_Default->zoom;
}
