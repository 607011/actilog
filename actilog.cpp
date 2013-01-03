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

#include "getopt/getopt.h"


CONST UINT DefaultTimerInterval = 10*1000;

enum _long_options {
	SELECT_HELP = 0x1,
	SELECT_SECURE,
	SELECT_MOVE_INTERVAL
};

static struct option long_options[] = {
	{ "secure",        no_argument, 0, SELECT_SECURE },
	{ "move-interval", required_argument, 0, SELECT_MOVE_INTERVAL },
	{ "help",          no_argument, 0, SELECT_HELP },
	{ NULL,            0, 0, 0 }
};


template <typename T>
T squared(T x) { return x*x; }


POINT lastMousePos = { LONG_MAX, LONG_MAX };
DOUBLE mouseDist = 0;
BOOL bSecure = FALSE;
BOOL bVerbose = FALSE;
LONG aHisto[256];
LONG aLastHisto[256];


BOOL hasHistoChanged()
{
	for (int i = 0; i < 256; ++i) {
		if (aHisto[i] != aLastHisto[i])
			return TRUE;
	}
	return FALSE;
}


VOID clearHistogram()
{
	for (int i = 0; i < 256; ++i) {
		aLastHisto[i] = aHisto[i];
		aHisto[i] = 0;
	}
}


VOID logTimestamp()
{
	SYSTEMTIME t;
	GetLocalTime(&t);
	printf("%4d-%02d-%02d %02d:%02d:%02d ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
}


inline VOID logFlush()
{
	printf("\n");
}


LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
	switch (wParam)
	{
	case WM_MOUSEMOVE:
		if (lastMousePos.x < LONG_MAX && lastMousePos.y < LONG_MAX)
			mouseDist += sqrt((DOUBLE)squared(lastMousePos.x - pMouse->pt.x) + (DOUBLE)squared(lastMousePos.y - pMouse->pt.y));
		lastMousePos = pMouse->pt;
		break;
	case WM_MOUSEWHEEL:
		logTimestamp();
		printf("WHEEL");
		logFlush();
		break;
	case WM_LBUTTONUP:
		// fall-through
	case WM_RBUTTONUP:
		logTimestamp();
		printf("CLICK");
		logFlush();
		break;
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
			if (bSecure) {
				if (pKeyBoard->vkCode >= 0 && pKeyBoard->vkCode < 256)
					++aHisto[pKeyBoard->vkCode];
			}
			else {
				logTimestamp();
				printf("KEY %u", pKeyBoard->vkCode);
				logFlush();
			}
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
		printf("MOVE %ld", (LONG)(mouseDist + 0.5));
		logFlush();
		mouseDist = 0;
	}
	if (bSecure && hasHistoChanged()) {
		logTimestamp();
		printf("KEYSTAT ");
		for (int i = 0; i < 256; ++i) {
			printf("%d", aHisto[i]);
			if (i < 255)
				printf(",");
		}
		logFlush();
		clearHistogram();
	}
}


VOID disclaimer()
{
	printf("\n\n\n"
		" Copyright (c) 2013 Oliver Lau <ola@ct.de>\n"
		"\n"
		" This program is free software: you can redistribute it and/or modify\n"
		" it under the terms of the GNU General Public License as published by\n"
		" the Free Software Foundation, either version 3 of the License, or\n"
		" (at your option) any later version.\n"
		"\n"
		" This program is distributed in the hope that it will be useful,\n"
		" but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		" GNU General Public License for more details.\n"
		"\n"
		" You should have received a copy of the GNU General Public License\n"
		" along with this program.  If not, see http://www.gnu.org/licenses/ \n\n");
}


VOID usage()
{
	printf("actilog - sums up mouse movements (in pixels), counts clicks and\n"
		"key presses, writes statistics to standard output.\n"
		"\n"
		"Usage: actilog [options]\n"
		"\n"
		"  -s\n"
		"     secure mode (do not log key codes)\n"
		"  -i interval\n"
		"     sum up mouse movements every interval seconds\n"
		"  -h\n"
		"  -?\n"
		"  --help\n"
		"     show this help (and license information)\n"
		"\n");
}


int main(int argc, char* argv[])
{
	UINT uTimerInterval = DefaultTimerInterval;
	for (;;) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "h?i:sv", long_options, &option_index);
		if (c == -1)
			break;
		switch (c)
		{
		case 'v':
			bVerbose = TRUE;
			break;
		case '?':
			// fall-through
		case 'h':
			usage();
			disclaimer();
			return EXIT_SUCCESS;
			break;
		case 's':
			// fall-through
		case SELECT_SECURE:
			bSecure = TRUE;
			break;
		case 'i':
			// fall-through
		case SELECT_MOVE_INTERVAL:
			if (optarg == NULL) {
				usage();
				return EXIT_FAILURE;
			}
			uTimerInterval = 1000 * atoi(optarg);
			break;
		default:
			usage();
			return EXIT_FAILURE;
			break;
		}
	}
	setvbuf(stdout, NULL, _IONBF, 0);
	if (bVerbose) {
		logTimestamp();
		printf("START interval = %d, secure = %d", uTimerInterval, bSecure);
		logFlush();
	}
	SecureZeroMemory(aHisto, 256*sizeof(aHisto[0]));
	SecureZeroMemory(aLastHisto, 256*sizeof(aLastHisto[0]));
	HINSTANCE hApp = GetModuleHandle(NULL);
	HHOOK hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hApp, 0);
	HHOOK hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hApp, 0);
	UINT_PTR uIDTimer = SetTimer(NULL, 0, uTimerInterval, TimerProc);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	KillTimer(NULL, uIDTimer);
	UnhookWindowsHookEx(hMouseHook);
	UnhookWindowsHookEx(hKeyboardHook);
	if (bVerbose) {
		logTimestamp();
		printf("STOP");
		logFlush();
	}
	return EXIT_SUCCESS;
}
