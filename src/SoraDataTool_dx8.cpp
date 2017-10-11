#define CINTERFACE

#include <Windows.h>
#include <d3d8/d3d8.h>

#include <stdio.h>

#define ERROR_EXIT(condition) if(condition) { printf(#condition "\n"); return false; }

static HWND GetHwnd(void)
{
	char pszWindowTitle[1024];
	GetConsoleTitle(pszWindowTitle, 1024);
	return FindWindow(NULL, pszWindowTitle);
}

bool GetDX8() {
	HMODULE md_d3d8 = LoadLibraryA("C:\\Windows\\System32\\d3d8.dll");
	auto pDirect3DCreate8 = (decltype(Direct3DCreate8)*)GetProcAddress(md_d3d8, "Direct3DCreate8");
	ERROR_EXIT(!pDirect3DCreate8);

	IDirect3D8* pD3D = pDirect3DCreate8(D3D_SDK_VERSION);
	ERROR_EXIT(!pD3D);

	D3DCAPS8 caps;
	pD3D->lpVtbl->GetDeviceCaps(pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
	int vp = 0;
	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	}
	else {
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

	D3DPRESENT_PARAMETERS d3dpp{};
	d3dpp.BackBufferWidth = 640;
	d3dpp.BackBufferHeight = 480;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = GetHwnd();
	d3dpp.Windowed = true;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.Flags = 0;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	IDirect3DDevice8* pD3DD = NULL;
	pD3D->lpVtbl->CreateDevice(pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetHwnd(), vp, &d3dpp, &pD3DD);
	ERROR_EXIT(!pD3DD);

	printf("Direct3DCreate8 : d3d8.dll+%08X\n", (unsigned)pDirect3DCreate8 - (unsigned)md_d3d8);
	printf("IDirect3D8::CreateDevice : d3d8.dll+%08X\n", (unsigned)pD3D->lpVtbl->CreateDevice - (unsigned)md_d3d8);
	printf("IDirect3DDevice8::Present : d3d8.dll+%08X\n", (unsigned)pD3DD->lpVtbl->Present - (unsigned)md_d3d8);
	printf("\n");

	pD3DD->lpVtbl->Release(pD3DD);
	pD3D->lpVtbl->Release(pD3D);
	FreeLibrary(md_d3d8);

	return true;
}

