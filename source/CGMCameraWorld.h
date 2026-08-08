#pragma once
#define CalcFactor(value, percent)				(((value / 100.f) * percent))

typedef struct
{
	float X;
	float Y;
	double Z;
} __Angle;

typedef struct
{
	int IsLoad;
	float zoom;
	__Angle Rot;
	float camAngle;
	float camWidthFar;
	//--
	float camviewFar[6];
	double WidthFar[6];
	double WidthNear[6];
}CAMERA_INFO;


class CGMCameraWorld
{
public:
	CGMCameraWorld();
	virtual~CGMCameraWorld();

	void Init();
	void Toggle();
	void Backup();
	bool IsEnable();
	void DefaultValues();
	void recover_backup_cam();

	void CalcNearFar();
	void PreparedZoom();
	void ZoomInNearFar();
	void RotateInNearFar();
	void SetAngleX(float fValue);
	void SetAngleY(float fValue1, float fValue2);
	float hermite_interpolation(float y0, float y1, float y2, float y3, float mu);

	CAMERA_INFO* CurrentCam();

	float GetSmoothed_zoom_factor();

	// True while middle-mouse rotate or zoom is active (used to soft-cull during turn).
	bool IsCameraBusy() const { return m_IsMove != 0 || m_IsZoom != 0; }

private:
	BOOL m_Enable;
	BOOL m_IsMove;
	BOOL m_IsZoom;
	__Angle cursor;
	float zoomPrecision;
	float zoomLimit[2];
	float zoomPercent[2];
	//--
	int zoomfade;
	float zoomFactor;
	float desiredZoomFactor;
	// Progress 0..1 over ~0.35s (time-based, not frame-count → same feel at 30/60/120)
	float m_fZoomProgress;
	float smoothed_zoom_factor;
	//--
	float camviewNear[2];
	CAMERA_INFO* m_Default;
	CAMERA_INFO* m_Current;
	CAMERA_INFO* m_nBackup;
};

