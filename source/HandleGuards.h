#pragma once
#include <windows.h>

namespace guards
{
	class CDCGuard
	{
		HDC m_hdc;
	public:
		CDCGuard(HDC hdc) : m_hdc(hdc) {}
		~CDCGuard() { if (m_hdc) DeleteDC(m_hdc); }
		HDC get() const { return m_hdc; }
		void reset(HDC hdc) { if (m_hdc) DeleteDC(m_hdc); m_hdc = hdc; }
		void release() { m_hdc = NULL; }
	};

	class CBitMapGuard
	{
		HBITMAP m_bmp;
	public:
		CBitMapGuard(HBITMAP bmp) : m_bmp(bmp) {}
		~CBitMapGuard() { if (m_bmp) DeleteObject(m_bmp); }
		HBITMAP get() const { return m_bmp; }
		void reset(HBITMAP bmp) { if (m_bmp) DeleteObject(m_bmp); m_bmp = bmp; }
		void release() { m_bmp = NULL; }
	};

	class CHandleGuard
	{
		HANDLE m_h;
	public:
		CHandleGuard(HANDLE h) : m_h(h) {}
		~CHandleGuard() { if (m_h && m_h != INVALID_HANDLE_VALUE) CloseHandle(m_h); }
		HANDLE get() const { return m_h; }
		void release() { m_h = NULL; }
	};
}
