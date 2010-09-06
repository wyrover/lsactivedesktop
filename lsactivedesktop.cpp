#define STRICT
#define NOCRYPT

#include <windows.h>
#include "lsadsettings.h"
#include "webform.h"
#include "lsactivedesktop.h"
#include <tchar.h>
#include <map>
#include <string>
#include <sstream>
#include "webwindow.h"
#include "webformdispatchimpl.h"

HINSTANCE hInstance;
HWND hMain;         // Our main window

const TCHAR* className = _T("classLSActiveDesktop");
LPCSTR revID = "LSActiveDesktop 0.1 by Tobbe";

bool loaded;
LSADSettings settings;
std::map<std::string, WebWindow*> webWindows;
WebformDispatchImpl *webformDispatchImpl;

void __cdecl bangNavigate(HWND caller, const char* bangCommandName, const char* args);
void __cdecl bangRunJSFunction(HWND caller, const char* bangCommandName, const char* args);
void reportError(LPCSTR msg);
void readSettings();
LRESULT CALLBACK PlainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

LRESULT CALLBACK PlainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case LM_GETREVID:
			lstrcpy((LPSTR)lParam, revID);

			return strlen((LPTSTR)lParam);
		case LM_REFRESH:
			readSettings();

			return 0;
		case WM_CREATE:
			for (std::map<std::string, WebWindow*>::iterator it = webWindows.begin(); it != webWindows.end(); it++) {
				int x = settings.windowProperties[it->first].x;
				int y = settings.windowProperties[it->first].y;
				int width = settings.windowProperties[it->first].width;
				int height = settings.windowProperties[it->first].height;
				std::string url = settings.windowProperties[it->first].url;
				bool showScrollbars = settings.windowProperties[it->first].showScrollbars;
				it->second->Create(hInstance, x, y, width, height, showScrollbars);
				it->second->webForm->Go(url.c_str());
				y += 400;
			}
			break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

extern "C" int __cdecl initModuleEx(HWND parentWnd, HINSTANCE dllInst, LPCSTR szPath)
{
	hInstance = dllInst;
	OleInitialize(0);

	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)PlainWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = className;
	wcex.hIconSm = NULL;

	if (!RegisterClassEx(&wcex)) {
		reportError("Error registering LSActiveDesktop window class");
		return 1;
	}

	readSettings();

	webformDispatchImpl = new WebformDispatchImpl();

	for (std::map<std::string, LSADWebWndProp>::iterator it = settings.windowProperties.begin(); it != settings.windowProperties.end(); it++) {
		webWindows.insert(std::make_pair(it->first, new WebWindow(webformDispatchImpl)));
	}

	hMain = CreateWindowEx(0, className, _T("WindowLSActiveDesktop"), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		1000, 700, 200, 200, HWND_MESSAGE, NULL, hInstance, NULL);

	if (hMain == NULL) {
		reportError("Error creating LSActiveDesktop window");
		UnregisterClass(className, dllInst);
		return 1;
	}

	ShowWindow(hMain, SW_SHOW);

	/*for (std::map<std::string, LSADWebWndProp>::iterator it = settings.windowProperties.begin(); it != settings.windowProperties.end(); it++) {
		std::ostringstream bangName;
		bangName << "!" << it->first << "Navigate";
		AddBangCommandEx(bangName.str().c_str(), bangNavigate);
		bangName.str("");*/
		/*bangName << "!" << it->first << "Forward";
		AddBangCommandEx(bangName.str().c_str(), bangForward);
		bangName.str("");
		bangName << "!" << it->first << "Refresh";
		AddBangCommandEx(bangName.str().c_str(), bangRefresh);
		bangName.str("");
		bangName << "!" << it->first << "RefreshCache";
		AddBangCommandEx(bangName.str().c_str(), bangRefreshCache);
		bangName.str("");*/
	//}
	AddBangCommandEx("!LSActiveDesktopNavigate", bangNavigate);
	AddBangCommandEx("!LSActiveDesktopRunJSFunction", bangRunJSFunction);

	// Register message for version info
	UINT msgs[] = {LM_GETREVID, LM_REFRESH, 0};
	SendMessage(GetLitestepWnd(), LM_REGISTERMESSAGE, (WPARAM)hMain, (LPARAM)msgs);

	return 0;
}

void __cdecl bangNavigate(HWND caller, const char* bangCommandName, const char* args)
{
	/*std::string bangName(bangCommandName);
	size_t nameLength = bangName.length() - strlen("Navigate");
	std::string webWindowName = bangName.substr(1, nameLength - 1);

	if (args && args[0] != '\0') {
		SetWindowText(webWindows[webWindowName]->webForm->hWnd, args);
	}*/

	const char *tokenStart = args;
	char token[MAX_LINE_LENGTH + 1];
	if (!GetToken(tokenStart, token, &tokenStart, false)) {
		reportError("Wrong bang command syntax");
	}
	std::string name(token);
	if (!GetToken(tokenStart, token, &tokenStart, false)) {
		reportError("Wrong bang command syntax");
	}
	std::string url(token);

	webWindows[name]->webForm->Go(url.c_str());
}

void __cdecl bangRunJSFunction(HWND caller, const char* bangCommandName, const char* args)
{
	const char *tokenStart = args;
	char token[MAX_LINE_LENGTH + 1];
	if (!GetToken(tokenStart, token, &tokenStart, false)) {
		reportError("Wrong bang command syntax");
	}
	std::string name(token);
	if (!GetToken(tokenStart, token, &tokenStart, false)) {
		reportError("Wrong bang command syntax");
	}
	std::string cmd(token);

	webWindows[name]->webForm->RunJSFunction(cmd);
}

void readSettings()
{
	settings.showErrors = GetRCBoolDef("LSActiveDesktopShowErrors", TRUE) != FALSE;
	settings.showScrollbars = !(GetRCBool("LSActiveDesktopHideScrollbars", TRUE) != FALSE);

	char line[MAX_LINE_LENGTH + 1];
	const char *tokenStart = line;
	char token[MAX_LINE_LENGTH + 1];
	GetRCLine("LSActiveDesktopWebWindows", line, MAX_LINE_LENGTH + 1, NULL);
	while (GetToken(tokenStart, token, &tokenStart, false)) {
		std::string name(token);

		LSADWebWndProp props;
		props.x = GetRCCoordinate((name + "X").c_str(), 0, GetSystemMetrics(SM_CXVIRTUALSCREEN));
		props.y = GetRCCoordinate((name + "Y").c_str(), 0, GetSystemMetrics(SM_CYVIRTUALSCREEN));
		props.width = GetRCInt((name + "Width").c_str(), 100);
		props.height = GetRCInt((name + "Height").c_str(), 100);
		props.showScrollbars = !(GetRCBool((name + "HideScrollbars").c_str(), TRUE) != FALSE);

		char url[MAX_LINE_LENGTH + 1];
		GetRCString((name + "URL").c_str(), url, "http://tlundberg.com", MAX_LINE_LENGTH + 1);
		props.url = url;

		settings.windowProperties.insert(make_pair(name, props));
	}
	/*settings.unmuteOnVolUp = GetRCBoolDef("tVolEzyUnmuteOnVolUp", TRUE) != FALSE;
	settings.unmuteOnVolDown = GetRCBoolDef("tVolEzyUnmuteOnVolDown", FALSE) != FALSE;
	char buffer[1024];
	GetRCString("tVolEzyVolumeChangedCommand", buffer, "!none", 1024);
	settings.volumeChangedCommand = buffer;
	GetRCString("tVolEzyMuteChangedCommand", buffer, "!none", 1024);
	settings.muteChangedCommand = buffer;*/
}

void reportError(LPCSTR msg)
{
	if (settings.showErrors)
	{
		MessageBox(NULL, msg, _T("LSActiveDesktop error"), MB_OK | MB_ICONERROR);
	}
}

extern "C" void __cdecl quitModule(HINSTANCE dllInst)
{
	RemoveBangCommand("!LSActiveDesktopNavigate");
	/*RemoveBangCommand("!LSActiveDesktopBack");
	RemoveBangCommand("!LSActiveDesktopForward");
	RemoveBangCommand("!LSActiveDesktopRefresh");
	RemoveBangCommand("!LSActiveDesktopRefreshCache");*/

	UINT msgs[] = {LM_GETREVID, LM_REFRESH, 0};
	SendMessage(GetLitestepWnd(), LM_UNREGISTERMESSAGE, (WPARAM)hMain, (LPARAM)msgs);

	for (std::map<std::string, WebWindow*>::iterator it = webWindows.begin(); it != webWindows.end(); it++) {
		delete it->second;
	}

	delete webformDispatchImpl;

	if (hMain != NULL)
	{
		DestroyWindow(hMain);
		hMain = NULL;
	}

	UnregisterClass(className, dllInst);
	OleUninitialize();
}