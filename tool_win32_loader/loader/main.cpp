#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <xx_helpers.h>

#include <files_idx_json.h>
#include <xx_downloader.h>
#include <get_md5.h>
#include <ntcvt.hpp>
#include <xx_file.h>
namespace FS = std::filesystem;

// todo: 自更新逻辑

// todo: 如果直接关闭窗口，似乎会报错，得处理这个关闭事件，让代码正常流程自杀

// main 一开始就创建这个对象
struct Loader {

	// 索引文件下载路径( 通常从索引服务下载 而非指向 CDN 或静态文件, 当然, 从索引服务跳转到实际下载路径也行 )
	std::string filesIdxJsonUrl = "http://192.168.1.200/www/files_idx.json";

	// 窗口注册类名。同时也是 ProgramData 目录下的子目录名
	std::wstring wndName = L"xxx_game_loader";

	// 窗口标题
	std::wstring wndTitle = L"xxx game loader";

	// 窗口内容设计尺寸（受 DPI 缩放影响，实际可能不止）
	float wndWidth = 640, wndHeight = 300;
	//float wndWidth = 1280, wndHeight = 720;

	// 默认字体名
	std::wstring fontName = L"consola";

	// 默认字号
	float fontSize = 16.f;
	//float fontSize = 32.f;

	// 背景色
	ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// flag 用于弹出窗口状态记录
	bool popuped = false;

	// 是否出错
	bool hasError = false;

	// 错误文本
	std::string errText;

	// 防多开互斥
	HANDLE hm = nullptr;

	// 下载器
	xx::Downloader dl;

	// json 上下文
	FilesIndexJson::Files fs;

	// 指向 ProgramData\xxxxxxxxxxxx 目录
	FS::path rootPath;

	// 指向下一步要加载的执行文件路径
	std::wstring exeFilePath;


	Loader() = default;
	~Loader() {
		if (hm) {
			CloseHandle(hm);
		}
	}

	// 返回 0: 加载成功，程序退出
	int Load() {
		// 参看 需求分析.txt

		// 先检查进程互斥. 多开直接退出
		auto hm = CreateMutex(nullptr, true, wndName.c_str());
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			assert(!hm);
			return 0;
		}

		// 拼接出存储区 root 路径
		auto ad = xx::GetPath_ProgramData();
		rootPath = ad / wndName;

		// 如果目录不存在就创建之
		if (!FS::exists(rootPath)) {

			// 如果创建失败就弹错误提示，OK退出
			if (!FS::create_directory(rootPath)) {
				hasError = true;
				errText = std::string("create directory failed at:\n") + rootPath.string();
				return 1;
			}
		}

		// 阻塞下载 files_idx.json
		dl.SetBaseUrl(filesIdxJsonUrl);
		dl.Download("");
		while (dl.Busy()) Sleep(1);

		// 如果下载失败，判断下，报错退出( 或者弹窗进度条下载? )
		if (!dl.Finished()) {
			hasError = true;
			errText = "network error! please try again!\n";
			return 1;
		}

		// 下载的 json 读入到 fs	// todo: 这句要 try 防止出错?
		ajson::load_from_buff(fs, (char*)dl.data.buf, dl.data.len);
		auto& ff = fs.files;

		// 停止下载线程
		dl.Close();

		// 如果没有文件??? 
		if (ff.empty()) {
			hasError = true;
			errText = "bad index file! please try again!";
		}

		// 找出一会儿要加载的 xxxx.exe
		for (auto const& f : ff) {
			if (f.path.ends_with(".exe")) {
				exeFilePath = ntcvt::from_chars(f.path);
				break;
			}
		}

		// 如果没有 xxxxxxxxx.exe 找到，报错退出
		if (exeFilePath.empty()) {
			hasError = true;
			errText = "can't found exe file! please try again!";
		}

		// 拼凑为完整路径
		exeFilePath = (rootPath / exeFilePath).wstring();


		// todo: 估算一下校验规模？文件数？总字节数？如果大于多少就直接显示 UI ?
		// 正常情况下 loader 要确保的文件 比较少，校验应该瞬间完成

		// 临时读文件容器
		xx::Data d;

		// 倒序遍历 fs, 校验长度和 md5. 通过一条删一条. 剩下的就是没通过的
		for (int i = ff.size() - 1; i >= 0; --i) {
			auto& f = ff[i];
			auto p = rootPath / f.path;

			// 文件不存在：继续
			if (!FS::exists(p)) continue;

			// 不是文件？删掉并继续					// todo: 不确定是否奏效
			if (!FS::is_regular_file(p)) {
				FS::remove_all(p);
				continue;
			}

			// 字节数对不上? 删掉并继续
			if (FS::file_size(p) != f.len) {
				FS::remove(p);
				continue;
			}

			// 数据读取错误? 报错 OK 退出
			if (xx::ReadAllBytes(p, d)) {
				hasError = true;
				errText = std::string("bad file at:\n") + p.string();
				return 1;
			}

			// 算出来的 md5 对不上？删掉并继续
			auto md5 = GetDataMD5Hash(d);
			if (f.md5 != md5) {
				FS::remove(p);
				continue;
			}

			// 校验通过：移除当前条目( 用最后一条覆盖 )
			if (auto last = ff.size() - 1; i < last) {
				ff[i] = ff[last];
			}
			ff.pop_back();
		}

		// 如果 fs 有数据剩余, 就进入到 UI 下载环节, 否则就加载 exe 并退出
		if (!ff.empty()) {

			// 统计
			for (auto const& f : ff) {
				totalCap += f.len;
			}

			if (totalCap == 0) {
				hasError = true;
				errText = "bad files len! please try again!";
				return 1;
			}

			// 初始化下载器
			dl.SetBaseUrl(fs.baseUrl);

			// 初始化下载起始时间点 for calc download speed
			beginSeconds = xx::NowEpochSeconds();

			return 1;
		}

		// for test
		//hasError = true;
		//errText = "alksdjfljasdlfkjslkf lasj fdlkadsj flkj sdflk jaskdf\n jkl jflkas jdflkja slkfj sakdlf jlksd fjlkds jfa\n";
		//return 1;

		// 将当前工作目录切到 exe 之所在
		FS::current_path(FS::path(exeFilePath).parent_path());

		// 执行 exe
		xx::ExecuteFile(exeFilePath.c_str());
		return 0;
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
			ImGui::Dummy({ 0.0f, 5.0f });
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

	// 所有文件已下载字节数( 不包含当前正在下载的. 下载完成才累加 )
	size_t totalLen = 0;

	// 所有文件总字节数
	size_t totalCap = 0;

	// totalLenEx = totalLen + currLen
	size_t totalLenEx = 0;

	// 当前文件下载进度 = len / total
	float progress = 0.f;

	// 总下载进度 = (totalLen + len) / totalCap
	float totalProgress = 0.f;

	// 当前正在下载的 File 下标，对应 fs.files[]
	int currFileIndex = -1;

	// 下载速度计算相关
	double beginSeconds = 0;

	// 每秒下载字节速度
	size_t bytesPerSeconds = 0;

	int DrawDownloader() {
		int rtv = 0;

		// for easy code
		auto&& ff = fs.files;

		// 当前文件下载字节数
		size_t currLen = 0;

		// 试获取下载器下载进度，如果正在下载就更新显示，否则就开始下载
		std::pair<size_t, size_t> LT;
		if (dl.TryGetProgress(LT)) {
			auto const& f = ff[currFileIndex];

			// 如果文件长度校验失败，停止下载并报错
			if (LT.second && LT.second != f.len) {
				dl.Close();

				hasError = true;
				errText = "bad files len! please try again!";
				return 0;
			}

			// 更新进度( total 可能为 0, 如果为 0 就使用 json 中记录的长度 )
			currLen = LT.first;
			progress = LT.first / (float)(LT.second == 0 ? ff[currFileIndex].len : LT.second);
			totalProgress = (totalLen + LT.first) / (float)totalCap;
		}
		else {
			// 之前有文件正在下载
			if (currFileIndex >= 0) {
				auto const& f = ff[currFileIndex];

				// 校验 md5 如果校验不过，停止下载并报错。 todo: 反复下载都校验不过，间隔一段时间再下?
				auto md5 = GetDataMD5Hash(dl.data);
				if (ff[currFileIndex].md5 != md5) {
					dl.Close();

					hasError = true;
					errText = "bad files md5! please try again!";
					return 0;
				}

				// 开始存盘。检查目录，如果没有就建。如果已存在( 不管是啥 )，就删
				auto p = rootPath / f.path;
				auto pp = p.parent_path();
				if (FS::exists(pp) && !FS::is_directory(pp)) {
					FS::remove_all(pp);
				}
				if (!FS::exists(pp)) {
					if (!FS::create_directories(pp)) {
						hasError = true;
						errText = "create directory error! please try again!";
						return 0;
					}
				}

				// 如果文件写盘失败，报错
				if (int r = xx::WriteAllBytes(rootPath / f.path, dl.data)) {
					dl.Close();

					hasError = true;
					errText = "write files to desk error! please try again!";
					return 0;
				}

				// todo: 写完再读一下校验？

				// 已下载完成的文件总长度 累加到 totalLen
				progress = 0.f;
				totalLen += ff[currFileIndex].len;
				totalProgress = totalLen / (float)totalCap;
			}

			// 指向下一个要下载的文件
			++currFileIndex;

			// 如果当前文件下标超出范围，说明已经下载完毕，准备加载 exe 并退出
			if (currFileIndex >= ff.size()) {
				// 将当前工作目录切到 exe 之所在
				FS::current_path(FS::path(exeFilePath).parent_path());

				// 执行 exe
				xx::ExecuteFile(exeFilePath.c_str());
				return 1;
			}

			// todo: 下前判断文件是否存在？如果存在就校验长度和 md5？如果校验通过就跳过不下载？

			// 开始下载
			dl.Download(ff[currFileIndex].path);
		}

		// 计算实时已下载总长
		totalLenEx = totalLen + currLen;
		auto elapsedSeconds = xx::NowEpochSeconds() - beginSeconds;
		if (elapsedSeconds > 0) {
			bytesPerSeconds = totalLenEx / (xx::NowEpochSeconds() - beginSeconds);
		}

		{
			// for easy code
			auto const& f = ff[currFileIndex];

			// 双进度条带百分比 + 当前文件名 + 下载速度 字节/秒

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

			ImGui::Text((std::to_string(totalLen / 1024) + " KB / " + std::to_string(totalCap / 1024) + " KB").c_str());

			ImGui::Dummy({ 0.0f, 5.0f });
			ImGui::Separator();
			ImGui::Dummy({ 0.0f, 5.0f });

			// todo: set color ?
			ImGui::Text(f.path.c_str());

			// Typically we would use {-1.0f,0.0f) or {-FLT_MIN,0.0f) to use all available width,
			// or {width,0.0f) for a specified width. {0.0f,0.0f) uses ItemWidth.
			ImGui::ProgressBar(progress, { -1.0f, 0.0f });

			ImGui::Text((std::to_string(currLen / 1024) + " KB / " + std::to_string(f.len / 1024) + " KB").c_str());

			ImGui::Text((std::string("download speed: ") + std::to_string(bytesPerSeconds / 1024) + " KB per seconds").c_str());

			ImGui::Dummy({ 0.0f, 5.0f });
			ImGui::Separator();
			ImGui::Dummy({ 0.0f, 5.0f });

			ImGui::Dummy({ 0.0f, 0.0f }); 
			ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::GetStyle().ItemSpacing.x + 120));
			if (ImGui::Button("Cancel", { 120, 35 })) {
				rtv = 1;
			}

			ImGui::End();
		}

		return rtv;
	}
};

#pragma region engine

#include <d3d9.h>
#include "ntcvt/ntcvt.hpp"


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
