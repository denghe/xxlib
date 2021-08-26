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

// todo: �Ը����߼�

// todo: ���ֱ�ӹرմ��ڣ��ƺ��ᱨ���ô�������ر��¼����ô�������������ɱ

// main һ��ʼ�ʹ����������
struct Loader {

	// �����ļ�����·��( ͨ���������������� ����ָ�� CDN ��̬�ļ�, ��Ȼ, ������������ת��ʵ������·��Ҳ�� )
	std::string filesIdxJsonUrl = "http://192.168.1.200/www/files_idx.json";

	// ����ע��������ͬʱҲ�� ProgramData Ŀ¼�µ���Ŀ¼��
	std::wstring wndName = L"xxx_game_loader";

	// ���ڱ���
	std::wstring wndTitle = L"xxx game loader";

	// ����������Ƴߴ磨�� DPI ����Ӱ�죬ʵ�ʿ��ܲ�ֹ��
	float wndWidth = 640, wndHeight = 300;
	//float wndWidth = 1280, wndHeight = 720;

	// Ĭ��������
	std::wstring fontName = L"consola";

	// Ĭ���ֺ�
	float fontSize = 16.f;
	//float fontSize = 32.f;

	// ����ɫ
	ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// flag ���ڵ�������״̬��¼
	bool popuped = false;

	// �Ƿ����
	bool hasError = false;

	// �����ı�
	std::string errText;

	// ���࿪����
	HANDLE hm = nullptr;

	// ������
	xx::Downloader dl;

	// json ������
	FilesIndexJson::Files fs;

	// ָ�� ProgramData\xxxxxxxxxxxx Ŀ¼
	FS::path rootPath;

	// ָ����һ��Ҫ���ص�ִ���ļ�·��
	std::wstring exeFilePath;


	Loader() = default;
	~Loader() {
		if (hm) {
			CloseHandle(hm);
		}
	}

	// ���� 0: ���سɹ��������˳�
	int Load() {
		// �ο� �������.txt

		// �ȼ����̻���. �࿪ֱ���˳�
		auto hm = CreateMutex(nullptr, true, wndName.c_str());
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			assert(!hm);
			return 0;
		}

		// ƴ�ӳ��洢�� root ·��
		auto ad = xx::GetPath_ProgramData();
		rootPath = ad / wndName;

		// ���Ŀ¼�����ھʹ���֮
		if (!FS::exists(rootPath)) {

			// �������ʧ�ܾ͵�������ʾ��OK�˳�
			if (!FS::create_directory(rootPath)) {
				hasError = true;
				errText = std::string("create directory failed at:\n") + rootPath.string();
				return 1;
			}
		}

		// �������� files_idx.json
		dl.SetBaseUrl(filesIdxJsonUrl);
		dl.Download("");
		while (dl.Busy()) Sleep(1);

		// �������ʧ�ܣ��ж��£������˳�( ���ߵ�������������? )
		if (!dl.Finished()) {
			hasError = true;
			errText = "network error! please try again!\n";
			return 1;
		}

		// ���ص� json ���뵽 fs	// todo: ���Ҫ try ��ֹ����?
		ajson::load_from_buff(fs, (char*)dl.data.buf, dl.data.len);
		auto& ff = fs.files;

		// ֹͣ�����߳�
		dl.Close();

		// ���û���ļ�??? 
		if (ff.empty()) {
			hasError = true;
			errText = "bad index file! please try again!";
		}

		// �ҳ�һ���Ҫ���ص� xxxx.exe
		for (auto const& f : ff) {
			if (f.path.ends_with(".exe")) {
				exeFilePath = ntcvt::from_chars(f.path);
				break;
			}
		}

		// ���û�� xxxxxxxxx.exe �ҵ��������˳�
		if (exeFilePath.empty()) {
			hasError = true;
			errText = "can't found exe file! please try again!";
		}

		// ƴ��Ϊ����·��
		exeFilePath = (rootPath / exeFilePath).wstring();


		// todo: ����һ��У���ģ���ļ��������ֽ�����������ڶ��پ�ֱ����ʾ UI ?
		// ��������� loader Ҫȷ�����ļ� �Ƚ��٣�У��Ӧ��˲�����

		// ��ʱ���ļ�����
		xx::Data d;

		// ������� fs, У�鳤�Ⱥ� md5. ͨ��һ��ɾһ��. ʣ�µľ���ûͨ����
		for (int i = ff.size() - 1; i >= 0; --i) {
			auto& f = ff[i];
			auto p = rootPath / f.path;

			// �ļ������ڣ�����
			if (!FS::exists(p)) continue;

			// �����ļ���ɾ��������					// todo: ��ȷ���Ƿ���Ч
			if (!FS::is_regular_file(p)) {
				FS::remove_all(p);
				continue;
			}

			// �ֽ����Բ���? ɾ��������
			if (FS::file_size(p) != f.len) {
				FS::remove(p);
				continue;
			}

			// ���ݶ�ȡ����? ���� OK �˳�
			if (xx::ReadAllBytes(p, d)) {
				hasError = true;
				errText = std::string("bad file at:\n") + p.string();
				return 1;
			}

			// ������� md5 �Բ��ϣ�ɾ��������
			auto md5 = GetDataMD5Hash(d);
			if (f.md5 != md5) {
				FS::remove(p);
				continue;
			}

			// У��ͨ�����Ƴ���ǰ��Ŀ( �����һ������ )
			if (auto last = ff.size() - 1; i < last) {
				ff[i] = ff[last];
			}
			ff.pop_back();
		}

		// ��� fs ������ʣ��, �ͽ��뵽 UI ���ػ���, ����ͼ��� exe ���˳�
		if (!ff.empty()) {

			// ͳ��
			for (auto const& f : ff) {
				totalCap += f.len;
			}

			if (totalCap == 0) {
				hasError = true;
				errText = "bad files len! please try again!";
				return 1;
			}

			// ��ʼ��������
			dl.SetBaseUrl(fs.baseUrl);

			// ��ʼ��������ʼʱ��� for calc download speed
			beginSeconds = xx::NowEpochSeconds();

			return 1;
		}

		// for test
		//hasError = true;
		//errText = "alksdjfljasdlfkjslkf lasj fdlkadsj flkj sdflk jaskdf\n jkl jflkas jdflkja slkfj sakdlf jlksd fjlkds jfa\n";
		//return 1;

		// ����ǰ����Ŀ¼�е� exe ֮����
		FS::current_path(FS::path(exeFilePath).parent_path());

		// ִ�� exe
		xx::ExecuteFile(exeFilePath.c_str());
		return 0;
	}

	// ���ط� 0: �����˳�
	int Draw() {
		if (hasError) return DrawError();
		else return DrawDownloader();
	}

	int DrawError() {
		int rtv = 0;

		// ִֻ��һ�εĴ��룺��������
		if (!popuped) {
			popuped = true;
			ImGui::OpenPopup("Error");
		}

		// ���þ�����ʾ
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing /* ImGuiCond_Always */, { 0.5f, 0.5f });

		// ��ʼ�����������ڣ��ߴ�����Ӧ
		if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

			// ��ʾ�����ı�
			ImGui::Text(errText.c_str());

			// ���� ���� �ĺ��
			ImGui::Dummy({ 0.0f, 5.0f });
			ImGui::Separator();
			ImGui::Dummy({ 0.0f, 5.0f });

			// �Ҷ��� OK ��ť. �������ȷָ��
			ImGui::Dummy({ 0.0f, 0.0f });
			ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::GetStyle().ItemSpacing.x + 80));
			if (ImGui::Button("OK", { 80, 35 })) {
				ImGui::CloseCurrentPopup();
				popuped = false;
				//rtv = 1;
				hasError = false;
			}

			// ����������������
			ImGui::EndPopup();
		}
		return rtv;
	}

	// �����ļ��������ֽ���( ��������ǰ�������ص�. ������ɲ��ۼ� )
	size_t totalLen = 0;

	// �����ļ����ֽ���
	size_t totalCap = 0;

	// totalLenEx = totalLen + currLen
	size_t totalLenEx = 0;

	// ��ǰ�ļ����ؽ��� = len / total
	float progress = 0.f;

	// �����ؽ��� = (totalLen + len) / totalCap
	float totalProgress = 0.f;

	// ��ǰ�������ص� File �±꣬��Ӧ fs.files[]
	int currFileIndex = -1;

	// �����ٶȼ������
	double beginSeconds = 0;

	// ÿ�������ֽ��ٶ�
	size_t bytesPerSeconds = 0;

	int DrawDownloader() {
		int rtv = 0;

		// for easy code
		auto&& ff = fs.files;

		// ��ǰ�ļ������ֽ���
		size_t currLen = 0;

		// �Ի�ȡ���������ؽ��ȣ�����������ؾ͸�����ʾ������Ϳ�ʼ����
		std::pair<size_t, size_t> LT;
		if (dl.TryGetProgress(LT)) {
			auto const& f = ff[currFileIndex];

			// ����ļ�����У��ʧ�ܣ�ֹͣ���ز�����
			if (LT.second && LT.second != f.len) {
				dl.Close();

				hasError = true;
				errText = "bad files len! please try again!";
				return 0;
			}

			// ���½���( total ����Ϊ 0, ���Ϊ 0 ��ʹ�� json �м�¼�ĳ��� )
			currLen = LT.first;
			progress = LT.first / (float)(LT.second == 0 ? ff[currFileIndex].len : LT.second);
			totalProgress = (totalLen + LT.first) / (float)totalCap;
		}
		else {
			// ֮ǰ���ļ���������
			if (currFileIndex >= 0) {
				auto const& f = ff[currFileIndex];

				// У�� md5 ���У�鲻����ֹͣ���ز����� todo: �������ض�У�鲻�������һ��ʱ������?
				auto md5 = GetDataMD5Hash(dl.data);
				if (ff[currFileIndex].md5 != md5) {
					dl.Close();

					hasError = true;
					errText = "bad files md5! please try again!";
					return 0;
				}

				// ��ʼ���̡����Ŀ¼�����û�оͽ�������Ѵ���( ������ɶ )����ɾ
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

				// ����ļ�д��ʧ�ܣ�����
				if (int r = xx::WriteAllBytes(rootPath / f.path, dl.data)) {
					dl.Close();

					hasError = true;
					errText = "write files to desk error! please try again!";
					return 0;
				}

				// todo: д���ٶ�һ��У�飿

				// ��������ɵ��ļ��ܳ��� �ۼӵ� totalLen
				progress = 0.f;
				totalLen += ff[currFileIndex].len;
				totalProgress = totalLen / (float)totalCap;
			}

			// ָ����һ��Ҫ���ص��ļ�
			++currFileIndex;

			// �����ǰ�ļ��±곬����Χ��˵���Ѿ�������ϣ�׼������ exe ���˳�
			if (currFileIndex >= ff.size()) {
				// ����ǰ����Ŀ¼�е� exe ֮����
				FS::current_path(FS::path(exeFilePath).parent_path());

				// ִ�� exe
				xx::ExecuteFile(exeFilePath.c_str());
				return 1;
			}

			// todo: ��ǰ�ж��ļ��Ƿ���ڣ�������ھ�У�鳤�Ⱥ� md5�����У��ͨ�������������أ�

			// ��ʼ����
			dl.Download(ff[currFileIndex].path);
		}

		// ����ʵʱ�������ܳ�
		totalLenEx = totalLen + currLen;
		auto elapsedSeconds = xx::NowEpochSeconds() - beginSeconds;
		if (elapsedSeconds > 0) {
			bytesPerSeconds = totalLenEx / (xx::NowEpochSeconds() - beginSeconds);
		}

		{
			// for easy code
			auto const& f = ff[currFileIndex];

			// ˫���������ٷֱ� + ��ǰ�ļ��� + �����ٶ� �ֽ�/��

			ImGui::SetNextWindowPos({ 10, 10 });
			ImGui::SetNextWindowSize({ wndWidth - 20, wndHeight - 20 });
			ImGui::Begin("Downloader", nullptr, ImGuiWindowFlags_NoDecoration);

			// ����Ӧ���ݿ�Ȳ�����. �²��Ȳ�����һ֡������ʾ
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


// ���ھ���
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

// ��ע����ֲ��� ���� �ļ�·��
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


// DX9 ���������
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// ǰ������
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ���
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	Loader loader;
	if (!loader.Load()) return 0;

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	auto frameInterval = (LONGLONG)(1000. * (double)freq.QuadPart / 64.);

	LARGE_INTEGER nLast;
	QueryPerformanceCounter(&nLast);

	// ��������ʾ�� DPI �Ŵ�����
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
