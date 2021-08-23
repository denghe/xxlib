#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <string>

// main 一开始就创建这个对象
struct Loader {
	// 是否出错: 显示错误面板
	bool hasError = true;
	std::string errText = "alksdjfljasdlfkjslkf lasj fdlkadsj flkj sdflk jaskdf\n jkl jflkas jdflkja slkfj sakdlf jlksd fjlkds jfa\n";
	bool popuped = false;

	// 显示相关参数
	std::wstring wndName = L"game loader";
	std::wstring wndTitle = L"game loader";
	float wndWidth = 1280, wndHeight = 720;
	std::wstring fontName = L"consola";
	float fontSize = 32.f;
	ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// todo: 文件上下文
	float progress = 0.f;
	float totalProgress = 0.f;

	//
	Loader() {
	}

	// 返回 0: 加载成功，程序退出
	int Load() {
		// 参看 需求分析.txt

		// todo: run exe
		//finished = true;
		//return 0;

		return 1;
	}

	// 返回非 0: 程序退出
	int Draw() {
		if (hasError) return DrawError();
		else return DrawDownloader();
	}

	int DrawError() {
		int rtv = 0;

		// 只执行一次的代码：弹出窗口
		if (!popuped) {
			popuped = true;
			ImGui::OpenPopup("Error");
		}

		// 设置居中显示
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing /* ImGuiCond_Always */, { 0.5f, 0.5f });

		// 开始声明弹出窗口，尺寸自适应
		if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

			// 显示错误文本
			ImGui::Text(errText.c_str());

			// 上下 留空 的横杠
			ImGui::Dummy({ 1.0f, 5.0f });
			ImGui::Separator();
			ImGui::Dummy({ 0.0f, 5.0f });

			// 右对齐 OK 按钮. 宽高需明确指定
			ImGui::Dummy({ 0.0f, 0.0f });
			ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::GetStyle().ItemSpacing.x + 80));
			if (ImGui::Button("OK", { 80, 35 })) {
				ImGui::CloseCurrentPopup();
				popuped = false;
				//rtv = 1;
				hasError = false;
			}

			// 结束声明弹出窗口
			ImGui::EndPopup();
		}
		return rtv;
	}

	int DrawDownloader() {
		int rtv = 0;

		// todo: 下载 + 绘制
		// todo: 双进度条带百分比 + 当前文件名 + 下载速度 字节/秒

		ImGui::SetNextWindowPos({ 10, 10 });
		ImGui::SetNextWindowSize({ wndWidth - 20, wndHeight - 20 });
		ImGui::Begin("Downloader", nullptr, ImGuiWindowFlags_NoDecoration);

		// 自适应内容宽度并居中. 猜测宽度并在下一帧修正显示
		ImGui::Dummy({});
		static float urlWidth = 100.0f;
		float pos = urlWidth + ImGui::GetStyle().ItemSpacing.x;
		ImGui::SameLine((ImGui::GetWindowWidth() - pos) / 2);
		ImGui::Text("http://xxx.xxx.xxx");
		urlWidth = ImGui::GetItemRectSize().x;

		ImGui::Text("Downloading...");

		ImGui::ProgressBar(totalProgress, { -1.0f, 0.0f });
		totalProgress += 0.001f;
		if (totalProgress > 1) totalProgress -= 1;

		ImGui::Text("xxxx MB / xxxxx MB");

		ImGui::Dummy({ 1.0f, 5.0f });
		ImGui::Separator();
		ImGui::Dummy({ 0.0f, 5.0f });

		// todo: set color ?
		ImGui::Text("abcde.png");

		// Typically we would use {-1.0f,0.0f) or {-FLT_MIN,0.0f) to use all available width,
		// or {width,0.0f) for a specified width. {0.0f,0.0f) uses ItemWidth.
		ImGui::ProgressBar(progress, { -1.0f, 0.0f });
		progress += 0.01f;
		if (progress > 1) progress -= 1;

		ImGui::Text("xxx MB / xxxx MB");
		ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::GetStyle().ItemSpacing.x + 120));
		if (ImGui::Button("Cancel", { 120, 35 })) {
			rtv = 1;
		}

		ImGui::Dummy({ 1.0f, 5.0f });
		ImGui::Separator();
		ImGui::Dummy({ 0.0f, 5.0f });


		ImGui::End();

		return rtv;
	}
};

#pragma region engine

#include <d3d9.h>
#include "ntcvt.hpp"

// 窗口居中
void PlaceInCenterOfScreen(HWND window, DWORD style, BOOL menu) {
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	RECT clientRect;
	GetClientRect(window, &clientRect);
	AdjustWindowRect(&clientRect, style, menu);
	int clientWidth = clientRect.right - clientRect.left;
	int clientHeight = clientRect.bottom - clientRect.top;
	SetWindowPos(window, NULL,
		screenWidth / 2 - clientWidth / 2,
		screenHeight / 2 - clientHeight / 2,
		clientWidth, clientHeight, 0
	);
}

// 在注册表种查找 字体 文件路径
std::string GetSystemFontFile(const std::wstring& faceName) {

	HKEY hKey;
	LONG result;

	// Open Windows font registry key
	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS) return {};

	DWORD maxValueNameSize, maxValueDataSize;
	result = RegQueryInfoKey(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
	if (result != ERROR_SUCCESS) return {};

	DWORD valueIndex = 0;
	LPWSTR valueName = new WCHAR[maxValueNameSize];
	LPBYTE valueData = new BYTE[maxValueDataSize];
	DWORD valueNameSize, valueDataSize, valueType;
	std::wstring wsFontFile;

	// Look for a matching font name
	do {
		wsFontFile.clear();
		valueDataSize = maxValueDataSize;
		valueNameSize = maxValueNameSize;

		result = RegEnumValue(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);

		valueIndex++;

		if (result != ERROR_SUCCESS || valueType != REG_SZ) {
			continue;
		}

		std::wstring wsValueName(valueName, valueNameSize);

		// Found a match
		if (_wcsnicmp(faceName.c_str(), wsValueName.c_str(), faceName.length()) == 0) {

			wsFontFile.assign((LPWSTR)valueData, valueDataSize);
			break;
		}
	} while (result != ERROR_NO_MORE_ITEMS);

	delete[] valueName;
	delete[] valueData;

	RegCloseKey(hKey);

	if (wsFontFile.empty()) return {};

	// Build full font file path
	WCHAR winDir[MAX_PATH];
	GetWindowsDirectory(winDir, MAX_PATH);

	wsFontFile = std::wstring(winDir) + L"\\Fonts\\" + wsFontFile;

	return ntcvt::wcbs2a<std::string>(wsFontFile.data(), wsFontFile.size());
}


// DX9 相关上下文
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// 前置声明
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 入口
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	Loader loader;
	if (!loader.Load()) return 0;

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	auto frameInterval = (LONGLONG)(1000. * (double)freq.QuadPart / 64.);

	LARGE_INTEGER nLast;
	QueryPerformanceCounter(&nLast);

	// 可无视显示器 DPI 放大设置
	//ImGui_ImplWin32_EnableDpiAwareness();

	// register window class
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, loader.wndName.c_str(), NULL };
	::RegisterClassEx(&wc);

	// calc wnd size by content size
	RECT rect{ .left = 0, .top = 0, .right = (long)loader.wndWidth, .bottom = (long)loader.wndHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);

	// Create application window
	HWND hwnd = ::CreateWindow(wc.lpszClassName, loader.wndTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT
		, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, wc.hInstance, NULL);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd)) {
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// center window
	::PlaceInCenterOfScreen(hwnd, WS_OVERLAPPEDWINDOW, 0);

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// disable write ini fle
	io.IniFilename = nullptr;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);

	// Load Fonts
	io.Fonts->AddFontFromFileTTF(GetSystemFontFile(loader.fontName).c_str(), loader.fontSize);

	// Main loop
	bool done = false;
	while (!done) {
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT) done = true;
		}
		if (done) break;

		// Start the Dear ImGui frame
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// call loader code
		if (loader.Draw()) break;

		// Rendering
		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(loader.clearColor.x * loader.clearColor.w * 255.0f),
			(int)(loader.clearColor.y * loader.clearColor.w * 255.0f),
			(int)(loader.clearColor.z * loader.clearColor.w * 255.0f),
			(int)(loader.clearColor.w * 255.0f));
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0) {
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();
		}
		HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

		// Handle loss of D3D9 device
		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
			ResetDevice();
		}

		// fps limit to 60 without wait vsync
		LARGE_INTEGER nNow;
		QueryPerformanceCounter(&nNow);
		auto interval = nNow.QuadPart - nLast.QuadPart;
		nLast = nNow;
		if (auto waitMS = (frameInterval - interval) / freq.QuadPart - 1; waitMS > 0 && waitMS < 100) {
			Sleep((int)waitMS);
		}
	}

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd) {
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) return false;

	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) return false;

	return true;
}

void CleanupDeviceD3D() {
	if (g_pd3dDevice) {
		g_pd3dDevice->Release();
		g_pd3dDevice = NULL;
	}
	if (g_pD3D) {
		g_pD3D->Release();
		g_pD3D = NULL;
	}
}

void ResetDevice() {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL) {
		IM_ASSERT(0);
	}
	ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

	switch (msg) {
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetDevice();
		}
		return 0;
	case WM_SYSCOMMAND:
		// Disable ALT application menu
		if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

#pragma endregion
