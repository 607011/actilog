/// actilog - sums up mouse movements (in pixels), counts clicks and
///           key presses, writes statistics to standard output.
///
///    Copyright (C) 2013 Oliver Lau <ola@ct.de>
///
///    This program is free software: you can redistribute it and/or modify
///    it under the terms of the GNU General Public License as published by
///    the Free Software Foundation, either version 3 of the License, or
///    (at your option) any later version.
///
///    This program is distributed in the hope that it will be useful,
///    but WITHOUT ANY WARRANTY; without even the implied warranty of
///    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///    GNU General Public License for more details.
///
///    You should have received a copy of the GNU General Public License
///    along with this program.  If not, see <http://www.gnu.org/licenses/>.
///
///
/// Usage: actilog.exe > logfile.txt
/// 

#include <windows.h>
#include <windowsx.h>
#include <cstdio>
#include <cmath>


template <typename T>
T squared(T x) { return x*x; }


POINT lastMousePos = { LONG_MAX, LONG_MAX };
DOUBLE mouseDist = 0;
CONST UINT gTimerInterval = 10*1000;


VOID logTimestamp()
{
	SYSTEMTIME t;
	GetLocalTime(&t);
	printf("%4d-%02d-%02d %02d:%02d:%02d ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
}


LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
	switch (wParam)
	{
	case WM_MOUSEMOVE:
		{
			if (lastMousePos.x < LONG_MAX && lastMousePos.y < LONG_MAX)
				mouseDist += sqrt((DOUBLE)squared(lastMousePos.x - pMouse->pt.x) + (DOUBLE)squared(lastMousePos.y - pMouse->pt.y));
			lastMousePos = pMouse->pt;
			break;
		}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		{
			logTimestamp();
			printf("CLICK\n");
			break;
		}
	}
	return 0;
}


LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
	switch (wParam)
	{
	case WM_KEYUP:
		{
			logTimestamp();
			printf("KEYPRESS %u\n", pKeyBoard->vkCode);
			break;
		}
	default:
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}
	return 0;
}


VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (mouseDist > 0) {
		logTimestamp();
		printf("MOVE %ld\n", (LONG)(mouseDist + 0.5));
		mouseDist = 0;
	}
}


int main()
{
	HINSTANCE appInstance = GetModuleHandle(NULL);
	SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, appInstance, 0);
	SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, appInstance, 0);
	SetTimer(NULL, 0, gTimerInterval, TimerProc);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return EXIT_SUCCESS;
}
