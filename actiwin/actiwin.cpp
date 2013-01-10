// actiwin.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "actiwin.h"

#define MAX_LOADSTRING 100


static const UINT DefaultTimerInterval = 600;
static const double DefaultDPI = 92.0;


// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name


enum _long_options {
	SELECT_HELP = 0x1,
	SELECT_INTERVAL,
	SELECT_OUTPUT_FILE,
	SELECT_OVERWRITE,
	SELECT_DPI
};


static struct option long_options[] = {
	{ TEXT("output"),    required_argument, 0, SELECT_OUTPUT_FILE },
	{ TEXT("interval"),  required_argument, 0, SELECT_INTERVAL },
	{ TEXT("help"),      no_argument, 0, SELECT_HELP },
	{ TEXT("overwrite"), no_argument, 0, SELECT_OVERWRITE },
	{ TEXT("dpi"),       required_argument, 0, SELECT_DPI },
	{ NULL,            0, 0, 0 }
};


Logger logger;
POINT ptLastMousePos = { LONG_MAX, LONG_MAX };
double fMouseDist = 0;
double fDPI = DefaultDPI;
int nClicks = 0;
int nDoubleClicks = 0;
int nWheel = 0;
bool bVerbose = false;
bool bOverwrite = false;
int aHisto[256];
int aLastHisto[256];


ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


inline bool hasHistoChanged()
{
	for (int i = 0; i < 256; ++i)
		if (aHisto[i] != aLastHisto[i] && aHisto[i] != 0)
			return true;
	return false;
}


LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
	switch (wParam)
	{
	case WM_MOUSEMOVE:
		if (ptLastMousePos.x < LONG_MAX && ptLastMousePos.y < LONG_MAX)
			fMouseDist += sqrt((double)squared(ptLastMousePos.x - pMouse->pt.x) + (double)squared(ptLastMousePos.y - pMouse->pt.y));
		ptLastMousePos = pMouse->pt;
		break;
#if (_WIN32_WINNT >= 0x0600)
	case WM_MOUSEHWHEEL:
		// fall-through
#endif
	case WM_MOUSEWHEEL:
		++nWheel;
		break;
#if (_WIN32_WINNT >= 0x0500)
	case WM_XBUTTONDBLCLK:
		// fall-through
#endif
	case WM_LBUTTONDBLCLK:
		// fall-through
	case WM_MBUTTONDBLCLK:
		// fall-through
	case WM_RBUTTONDBLCLK:
		++nDoubleClicks;
		break;
#if (_WIN32_WINNT >= 0x0500)
	case WM_XBUTTONUP:
		// fall-through
#endif
	case WM_LBUTTONUP:
		// fall-through
	case WM_MBUTTONUP:
		// fall-through
	case WM_RBUTTONUP:
		++nClicks;
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
			const DWORD dwKeyCode = pKeyBoard->vkCode;
			if (dwKeyCode >= 0 && dwKeyCode < 256)
				++aHisto[dwKeyCode];
			break;
		}
	default:
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}
	return 0;
}


void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (fMouseDist > 0) {
		logger.logWithTimestamp(TEXT("MOVE %lf px (%lf m)"), fMouseDist, fMouseDist / fDPI * 2.54 / 100);
		fMouseDist = 0;
	}
	if (nWheel > 0) {
		logger.logWithTimestamp(TEXT("WHEEL %d"), nWheel);
		nWheel = 0;
	}
	if (nClicks > 0) {
		logger.logWithTimestamp(TEXT("CLICK %d"), nClicks);
		nClicks = 0;
	}
	if (nDoubleClicks > 0) {
		logger.logWithTimestamp(TEXT("DBLCLICK %d"), nDoubleClicks);
		nDoubleClicks = 0;
	}
	if (hasHistoChanged()) {
		logger.logWithTimestampNoLF(TEXT("KEYSTAT "));
		for (int i = 0; i < 256; ++i) {
			logger.log(TEXT("%d"), aHisto[i]);
			if (i < 255)
				logger.log(TEXT(","));
		}
		logger.flush();
		for (int i = 0; i < 256; ++i) {
			aLastHisto[i] = aHisto[i];
			aHisto[i] = 0;
		}
	}
}


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UINT uTimerInterval = DefaultTimerInterval;
	int argCount;
	LPTSTR *szArgList = CommandLineToArgvA(GetCommandLine(), &argCount);
	for (;;) {
		int option_index = 0;
		int c = getopt_long(argCount, szArgList, "i:vo:", long_options, &option_index);
		if (c == -1)
			break;
		switch (c)
		{
		case 'v':
			bVerbose = true;
			break;
		case SELECT_OVERWRITE:
			bOverwrite = true;
			break;
		case 'o':
			// fall-through
		case SELECT_OUTPUT_FILE:
			if (optarg == NULL) {
				return EXIT_FAILURE;
			}
			logger.setFilename(optarg);
			break;
		case SELECT_DPI:
			fDPI = atof(optarg);
			break;
		case 'i':
			// fall-through
		case SELECT_INTERVAL:
			if (optarg == NULL) {
				return EXIT_FAILURE;
			}
			uTimerInterval = atoi(optarg);
			break;
		default:
			return EXIT_FAILURE;
		}
	}
	SecureZeroMemory(aHisto, 256*sizeof(aHisto[0]));
	SecureZeroMemory(aLastHisto, 256*sizeof(aLastHisto[0]));
	bool success = logger.open(bOverwrite);
	if (!success) {
		fprintf(stderr, "Fatal error: cannot create file '%s'\n", logger.filename());
		return EXIT_FAILURE;
	}
	if (bVerbose)
		logger.logWithTimestamp(TEXT("START interval = %d secs, dpi = %lf, verbose = %s"), uTimerInterval, fDPI, bVerbose? TEXT("true") : TEXT("false"));
	HINSTANCE hApp = GetModuleHandle(NULL);
	HHOOK hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hApp, 0);
	HHOOK hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hApp, 0);
	UINT_PTR uIDTimer = SetTimer(NULL, 0, 1000 * uTimerInterval, TimerProc);

	MSG msg;
	HACCEL hAccelTable;
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_ACTIWIN, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	if (!InitInstance (hInstance, nCmdShow))
		return FALSE;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ACTIWIN));
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	KillTimer(NULL, uIDTimer);
	UnhookWindowsHookEx(hMouseHook);
	UnhookWindowsHookEx(hKeyboardHook);
	if (bVerbose)
		logger.logWithTimestamp(TEXT("STOP"));
	logger.close();
	return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ACTIWIN));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_ACTIWIN);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	return RegisterClassEx(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	hInst = hInstance; // Store instance handle in our global variable
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 320, 320, NULL, NULL, hInstance, NULL);
	if (!hWnd)
		return FALSE;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_QUERYENDSESSION:
		logger.logWithTimestamp(TEXT("WM_QUERYENDSESSION"));
		break;
	case WM_ENDSESSION:
		logger.logWithTimestamp(TEXT("WM_ENDSESSION"));
		break;
	case WM_WTSSESSION_CHANGE:
		logger.logWithTimestamp(TEXT("WM_WTSSESSION_CHANGE"));
		break;
	case WM_POWER:
		logger.logWithTimestamp(TEXT("WM_POWER"));
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);
			TextOut(hdc, 0, 0, TEXT("Blah!"), 5);
			EndPaint(hWnd, &ps);
			break;
		}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}



INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
