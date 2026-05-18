#pragma once
#include "PSO2CameraTool.hpp"

#include <MinHook.h>
#include <d3d9.h>
#include <d3dx9core.h>
#include "Asm.h"
#include "../external/imgui/settings_form.h"



typedef void (*pso2hLogLine_t)(const char* format, ...);
pso2hLogLine_t pso2hLogLine = nullptr;

bool m_bCreated = false;
bool wndproc_found = false;
D3DVIEWPORT9 viewport;
LPD3DXFONT dxFont;
HMODULE hmRendDx9Base = NULL;

HWND game_hwnd = 0;
HMODULE psoBase = 0;

typedef HRESULT(__stdcall* tEndScene)(LPDIRECT3DDEVICE9 Device);
tEndScene oEndScene;

typedef HRESULT(__stdcall* tReset)(LPDIRECT3DDEVICE9 Device, D3DPRESENT_PARAMETERS* pPresentationParameters);
tReset oReset;

WNDPROC game_wndproc = NULL;
extern IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool MENU_DISPLAYING = false;


static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

uintptr_t cameraFarCullJna;
uintptr_t cameraFarCullObjectJe;
uintptr_t cameraNearCullAddy;


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	if (MENU_DISPLAYING)
	{
		if ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || (msg >= WM_KEYFIRST && msg <= WM_KEYLAST))
			return true;
	}

	return CallWindowProc(game_wndproc, hWnd, msg, wParam, lParam);
}

uintptr_t getTerrainFarCullAddy()
{
	return cameraFarCullJna;
}
uintptr_t getObjectFarCullAddy()
{
	return cameraFarCullObjectJe;
}
uintptr_t getCameraNearCullAddy()
{
	return cameraNearCullAddy;
}
HRESULT __stdcall hkEndScene(LPDIRECT3DDEVICE9 Device)
{
	using namespace Asm;

	if (Device == nullptr)
		return oEndScene(Device);
	if (!m_bCreated)
	{
		m_bCreated = true;
		D3DXCreateFontA(Device, 20, 0, FW_BOLD, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas", &dxFont);
		Device->GetViewport(&viewport);

		D3DDEVICE_CREATION_PARAMETERS d3dcp;
		Device->GetCreationParameters(&d3dcp);

		game_hwnd = d3dcp.hFocusWindow;
		

		DWORD farCullScan = AobScan(cameraFarCullAob);
		if (farCullScan)
		{
			cameraFarCullJna = farCullScan;
			if (pso2hLogLine) pso2hLogLine("[PigPSO2Cam] Found camera far cull: %p", (void*)cameraFarCullJna);
		}
		DWORD objectCullScan = AobScan(cameraFarCullObjectsAob);
		if (objectCullScan) {
			cameraFarCullObjectJe = objectCullScan;
			cameraFarCullObjectJe += 0x7;
			if (pso2hLogLine) pso2hLogLine("[PigPSO2Cam] Found object far cull: %p", (void*)cameraFarCullObjectJe);
		}
		DWORD nearCullScan = AobScan(cameraNearCullAob);
		if (nearCullScan)
		{
			cameraNearCullAddy = nearCullScan;
			if (pso2hLogLine) pso2hLogLine("[PigPSO2Cam] Found camera near cull: %p", (void*)cameraNearCullAddy);
		}
		
	}
	if (!wndproc_found) {
		HWND wnd = FindWindowA("Phantasy Star Online 2", NULL);
		if (wnd) {
			game_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(wnd, GWLP_WNDPROC, (LONG_PTR)WndProc));
			menu_init(game_hwnd, Device);

			wndproc_found = true;
			if (pso2hLogLine) pso2hLogLine("[PigPSO2Cam] Main window found and WndProc hooked.");
		}
	}

	static bool insert_down = false;
	bool current_insert = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
	if (current_insert && !insert_down) {
		MENU_DISPLAYING = !MENU_DISPLAYING;
		if (pso2hLogLine) pso2hLogLine(MENU_DISPLAYING ? "[PigPSO2Cam] Displaying menu" : "[PigPSO2Cam] Hiding menu");
	}
	insert_down = current_insert;

	static bool escape_down = false;
	bool current_escape = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
	if (current_escape && !escape_down && MENU_DISPLAYING) {
		MENU_DISPLAYING = false;
		if (pso2hLogLine) pso2hLogLine("[PigPSO2Cam] Hiding menu");
	}
	escape_down = current_escape;

	if (MENU_DISPLAYING)
	{
		draw_menu(&MENU_DISPLAYING);
	}


	return oEndScene(Device);
}
HRESULT APIENTRY hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	if (dxFont)
		dxFont->OnLostDevice();

	HRESULT result = oReset(pDevice, pPresentationParameters);
	if (SUCCEEDED(result))
	{
		if (dxFont)
			dxFont->OnResetDevice();
		m_bCreated = false;
		ImGui_ImplDX9_InvalidateDeviceObjects();
		pDevice->GetViewport(&viewport);

		ImGui_ImplDX9_CreateDeviceObjects();
	}
	return result;
}


DWORD_PTR* pVTable;
HWND tmpWnd;
bool CreateDeviceD3D(HWND hWnd)
{
	tmpWnd = CreateWindowA("BUTTON", "DX", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, GetModuleHandle(NULL), NULL);

	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.hDeviceWindow = tmpWnd;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
	{
		if (pso2hLogLine) pso2hLogLine("[PigPSO2Cam] CreateDevice failed.");
		return false;
	}

	pVTable = (DWORD_PTR*)g_pd3dDevice;
	pVTable = (DWORD_PTR*)pVTable[0];
	if (pso2hLogLine) pso2hLogLine("[PigPSO2Cam] D3D dummy device created. pVTable: %p", pVTable);
	return true;
}

DWORD WINAPI HookThread()
{
	if (pso2hLogLine) pso2hLogLine("[PigPSO2Cam] HookThread started.");

	CreateDeviceD3D(game_hwnd);
	if (!pVTable)
	{
		if (pso2hLogLine) pso2hLogLine("[PigPSO2Cam] Failed to create D3D dummy device.");
		return false;
	}
	
	if (MH_Initialize() == MH_OK)
	{
		MH_CreateHook((LPVOID)pVTable[42], (LPVOID)hkEndScene, reinterpret_cast<LPVOID*>(&oEndScene));
		MH_CreateHook((LPVOID)pVTable[16], (LPVOID)hkReset, reinterpret_cast<LPVOID*>(&oReset));
		MH_EnableHook(MH_ALL_HOOKS);
	}

	if (pso2hLogLine) pso2hLogLine("[PigPSO2Cam] MinHook attached for EndScene and Reset.");

	g_pD3D->Release();
	g_pd3dDevice->Release();
	DestroyWindow(tmpWnd);
	return 0;
}

int Initialize() {
	HMODULE hPso2Host = GetModuleHandleA("pso2h.dll");
	if (hPso2Host) {
		pso2hLogLine = (pso2hLogLine_t)GetProcAddress(hPso2Host, "pso2hLogLine");
		if (pso2hLogLine) {
			pso2hLogLine("[PigPSO2Cam] Loaded pso2hLogLine successfully.");
		}
	}

	/*while (hmRendDx9Base == NULL)
	{
		Sleep(200);
		hmRendDx9Base = GetModuleHandleA("d3d9.dll");
	}
	while (game_hwnd == NULL) {
		Sleep(200);
		game_hwnd = FindWindowA("Phantasy Star Online 2", NULL);
	}
	Sleep(1000); //idk it just be like this
	*/
	HookThread();
	return 1;
}