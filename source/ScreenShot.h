#pragma once

class ScreenShot
{
public:
	ScreenShot();
	void PrintScreen();
	bool running;
};

extern ScreenShot gSS;

void ScreenShotThread2(void* lpParam);
