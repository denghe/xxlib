#pragma once
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imgui_stdlib.h"
#include <d3d9.h>
#include "ntcvt.hpp"

// 用法说明在最下面

// 窗口居中
inline void PlaceInCenterOfScreen(HWND window, DWORD style, BOOL menu) {
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
inline std::string GetSystemFontFile(const std::wstring& faceName) {

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
template<typename ImGuiContext>
inline int RunImGuiContext(HINSTANCE hInstance, ImGuiContext& ctx) {

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	auto frameInterval = (LONGLONG)(1000. * (double)freq.QuadPart / 300.);

	LARGE_INTEGER nLast;
	QueryPerformanceCounter(&nLast);

	// 可无视显示器 DPI 放大设置
	if (ctx.enableDpiAwarenes) {
		ImGui_ImplWin32_EnableDpiAwareness();
	}

	// register window class
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, ctx.wndName.c_str(), NULL };
	::RegisterClassEx(&wc);

	// calc wnd size by content size
	RECT rect{ .left = 0, .top = 0, .right = (long)ctx.wndWidth, .bottom = (long)ctx.wndHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);

	// Create application window
	HWND hwnd = ::CreateWindow(wc.lpszClassName, ctx.wndTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT
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
	auto fontPath = GetSystemFontFile(ctx.fontName);
	io.Fonts->AddFontFromFileTTF(fontPath.c_str(), ctx.fontSize, nullptr, io.Fonts->GetGlyphRangesChineseFull());

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

		// call ctx code
		if (ctx.Draw()) break;

		// Rendering
		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(ctx.clearColor.x * ctx.clearColor.w * 255.0f),
			(int)(ctx.clearColor.y * ctx.clearColor.w * 255.0f),
			(int)(ctx.clearColor.z * ctx.clearColor.w * 255.0f),
			(int)(ctx.clearColor.w * 255.0f));
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

	io.Fonts->ClearFonts();

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions

inline bool CreateDeviceD3D(HWND hWnd) {
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

inline void CleanupDeviceD3D() {
	if (g_pd3dDevice) {
		g_pd3dDevice->Release();
		g_pd3dDevice = NULL;
	}
	if (g_pD3D) {
		g_pD3D->Release();
		g_pD3D = NULL;
	}
}

inline void ResetDevice() {
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
inline LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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

// 业务类需要继承这个基类，并提供 int Draw() 函数，最后传给 RunContext
struct ImGuiContextBase {

	// 窗口注册类名
	std::wstring wndName = L"imgui_window1";

	// 窗口标题
	std::wstring wndTitle = L"imgui window1";

	// 窗口内容设计尺寸（受 DPI 缩放影响，实际可能不止）
	float wndWidth = 1280, wndHeight = 720;

	// 默认字体名
	std::wstring fontName = L"simhei";

	// 默认字号
	float fontSize = 16.f;

	// 背景色
	ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// 常用交互色
	ImVec4 normalColor { 0, 0, 0, 1.0f };
	ImVec4 pressColor { 0.5f, 0, 0, 1.0f };
	ImVec4 releaseColor { 0, 0.5f, 0, 1.0f };

	// 是否忽略显示器 dpi 设置( 忽略 + 大字体会比较清晰 )
	bool enableDpiAwarenes = false;
};

// 项目需要链接 d3d9.lib
// 项目需要设置 search path 到 下列 文件 所在目录. 同时这些 cpp 需要拉到项目中 进行编译
// 
// imgui.cpp
// imgui_demo.cpp
// imgui_draw.cpp
// imgui_tables.cpp
// imgui_widgets.cpp
// 
// imgui_impl_dx9.cpp
// imgui_impl_win32.cpp
// 
// imgui_stdlib.cpp
// 
// ntcvt.hpp




// 示例

//#pragma execution_character_set("utf-8")
//#include "xx_imgui_helper.h"
//
//struct Context : ImGuiContextBase {
//	Context() {
//		this->wndName = L"imgui_test1";
//		this->wndTitle = L"imgui test1";
//	}
//	bool hasError = false;
//	std::string errorText;
//	bool popuped = false;
//	int Draw() {
//		if (hasError) return DrawError();
//		else return DrawNormal();
//	}
//	int DrawError() {
//		int rtv = 0;
//		if (!popuped) {
//			popuped = true;
//			ImGui::OpenPopup("Error");
//		}
//		// 设置居中显示
//		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
//		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing /* ImGuiCond_Always */, { 0.5f, 0.5f });
//		// 开始声明弹出窗口，尺寸自适应
//		if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
//			// 显示错误文本
//			ImGui::Text(errorText.c_str());
//			// 上下 留空 的横杠
//			ImGui::Dummy({ 0.0f, 5.0f });
//			ImGui::Separator();
//			ImGui::Dummy({ 0.0f, 5.0f });
//			// 右对齐 OK 按钮. 宽高需明确指定
//			ImGui::Dummy({ 0.0f, 0.0f });
//			ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::GetStyle().ItemSpacing.x + 80));
//			if (ImGui::Button("OK", { 80, 35 })) {
//				ImGui::CloseCurrentPopup();
//				popuped = false;
//				hasError = false;
//			}
//			// 结束声明弹出窗口
//			ImGui::EndPopup();
//		}
//		return rtv;
//	}
//	int DrawNormal() {
//		// todo
//		return 0;
//	}
//};
//
//int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
//	Context ctx;
//	ctx.hasError = true;
//	ctx.errorText = "asdf qwer xxxxxxxxxxxxxxxxxx";
//	return RunImGuiContext(hInstance, ctx);
//}
