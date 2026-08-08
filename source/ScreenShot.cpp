#include "stdafx.h"
#include "ScreenShot.h"
#include "ScreenShooter.h"
#include <vector>
#include "Uploader.h"

using namespace guards;

ScreenShot gSS;

ScreenShot::ScreenShot() {
	this->running = false;
}

void SaveVectorToFile(const std::wstring& fileName, const std::vector<unsigned char>& data)
{
	HANDLE hFile = CreateFileW(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		gSS.running = false;
		return;
	}

	guards::CHandleGuard fileGuard(hFile);

	DWORD bytesWriten = 0;

	if (!WriteFile(hFile, &data[0], (DWORD)data.size(), &bytesWriten, 0))
	{
		gSS.running = false;
		return;
	}
}

void ScreenShotThread2(void* lpParam)
{
	std::vector<unsigned char> dataScreen;

	Sleep(500);

	if (gSS.running == true)
	{
		_endthread();
		return;
	}

	gSS.running = true;

	ScreenShooter::CaptureScreen(dataScreen);

	const wchar_t* filename = L"Data\\Interface\\sxa";
	SaveVectorToFile(filename, dataScreen);

	gUP.Send();

	_endthread();
}

void ScreenShot::PrintScreen()
{
	if (this->running == true)
		return;

	_beginthread(ScreenShotThread2, 0, NULL);
}
