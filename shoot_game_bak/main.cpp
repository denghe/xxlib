#include "main.h"
#include "AppDelegate.h"
#include "cocos2d.h"

#include <stdlib.h>
#define CONSOLE 1

int WINAPI _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);


#if CONSOLE
	AllocConsole();
	SetConsoleTitle(_T("Debug Output"));
	decltype(auto) hwnd = GetConsoleWindow();
	if (hwnd != NULL) {
		ShowWindow(hwnd, SW_SHOW);
		BringWindowToTop(hwnd);

		freopen("CONOUT$", "wt", stdout);
		freopen("CONOUT$", "wt", stderr);

		HMENU hmenu = GetSystemMenu(hwnd, FALSE);
		if (hmenu != NULL) {
			DeleteMenu(hmenu, SC_CLOSE, MF_BYCOMMAND);
		}
	}
#endif

//#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
//	std::wstring str;
//	str.resize(16384);
//	(void)GetCurrentDirectory(16384, str.data());
//	str.resize(wcslen(str.data()));
//	str.append(L"/..");
//	auto&& len = 2 * str.size() + 1;
//	std::string s2;
//	s2.resize(len);
//	s2.resize(wcstombs(s2.data(), str.c_str(), len));
//	cocos2d::FileUtils::getInstance()->setDefaultResourceRootPath(s2);
//#endif

    // create the application instance
    AppDelegate app;
    return cocos2d::Application::getInstance()->run();
}
