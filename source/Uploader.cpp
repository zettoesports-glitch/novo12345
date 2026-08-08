#include "stdafx.h"
#include "Uploader.h"
#include "ScreenShot.h"
#include "ZzzCharacter.h"
#include <winhttp.h>
#include <stdio.h>

#pragma comment(lib, "winhttp.lib")

Uploader gUP;

#define SCREENSHOT_URL_HOST L"muonline.com"
#define SCREENSHOT_URL_PATH L"/ping.php"
#define SCREENSHOT_FILE "Data\\Interface\\sxa"

Uploader::Uploader()
{
}

void Uploader::Send()
{
	FILE* pFile = NULL;
	if (fopen_s(&pFile, SCREENSHOT_FILE, "rb") != 0 || pFile == NULL)
	{
		gSS.running = false;
		return;
	}

	fseek(pFile, 0, SEEK_END);
	long fileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	if (fileSize <= 0)
	{
		fclose(pFile);
		gSS.running = false;
		return;
	}

	unsigned char* fileData = new unsigned char[fileSize];
	fread(fileData, 1, fileSize, pFile);
	fclose(pFile);

	char account[12] = { 0 };
	if (CharacterAttribute && Hero)
	{
		memcpy(account, Hero->ID, sizeof(account) - 2);
	}
	else
	{
		strcpy_s(account, "Unknown");
	}

	const char* boundary = "----MuOnlineBoundary7MA4YWxkTrZu0gW";
	char headerA[512];
	int headerLenA = sprintf_s(headerA, sizeof(headerA),
		"Content-Type: multipart/form-data; boundary=%s\r\n",
		boundary);

	wchar_t headerW[512];
	MultiByteToWideChar(CP_ACP, 0, headerA, headerLenA, headerW, 512);
	DWORD headerLen = (DWORD)headerLenA;

	int contentLen = 0;

	char part1[512];
	int part1Len = sprintf_s(part1, sizeof(part1),
		"--%s\r\n"
		"Content-Disposition: form-data; name=\"username\"\r\n\r\n"
		"%s\r\n"
		"--%s\r\n"
		"Content-Disposition: form-data; name=\"printfile\"; filename=\"screen.jpg\"\r\n"
		"Content-Type: image/jpeg\r\n\r\n",
		boundary, account, boundary);

	char part2[128];
	int part2Len = sprintf_s(part2, sizeof(part2),
		"\r\n--%s\r\n"
		"Content-Disposition: form-data; name=\"submit\"\r\n\r\n"
		"send\r\n"
		"--%s--\r\n",
		boundary, boundary);

	contentLen = part1Len + fileSize + part2Len;

	char* body = new char[contentLen];
	memcpy(body, part1, part1Len);
	memcpy(body + part1Len, fileData, fileSize);
	memcpy(body + part1Len + fileSize, part2, part2Len);

	delete[] fileData;

	HINTERNET hSession = WinHttpOpen(L"MuOnline/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

	if (hSession)
	{
		HINTERNET hConnect = WinHttpConnect(hSession,
			SCREENSHOT_URL_HOST,
			INTERNET_DEFAULT_HTTP_PORT, 0);

		if (hConnect)
		{
			HINTERNET hRequest = WinHttpOpenRequest(hConnect,
				L"POST", SCREENSHOT_URL_PATH,
				NULL, WINHTTP_NO_REFERER,
				WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

			if (hRequest)
			{
				WinHttpSendRequest(hRequest,
					headerW, headerLen,
					(WINHTTP_NO_OUTPUT_BUFFER),
					0, contentLen, 0);

				WinHttpWriteData(hRequest, body, contentLen, NULL);

				WinHttpReceiveResponse(hRequest, NULL);

				WinHttpCloseHandle(hRequest);
			}

			WinHttpCloseHandle(hConnect);
		}

		WinHttpCloseHandle(hSession);
	}

	delete[] body;

	DeleteFileA(SCREENSHOT_FILE);

	gSS.running = false;
}
