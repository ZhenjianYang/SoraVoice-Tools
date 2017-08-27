#define CINTERFACE

#include <Windows.h>

#include <d3d9.h>
#include <dsound.h>
#include <dinput.h>

#include <stdio.h>

#define ERROR_EXIT(condition) if(condition) { printf(#condition "\n"); return false; }

static HWND GetHwnd(void)
{
	char pszWindowTitle[1024];
	GetConsoleTitle(pszWindowTitle, 1024);
	return FindWindow(NULL, pszWindowTitle);
}

bool GetDSound();
bool GetDInput();
bool GetDX9();
extern bool GetDX8();

int main(int argc, char* argv[])
{
	GetDSound();
	GetDInput();
	GetDX9();
	GetDX8();

	return 0;
}

bool GetDSound() {
	HMODULE md_dsound = LoadLibraryA("C:\\Windows\\System32\\dsound.dll");
	auto pDirectSoundCreate = (decltype(DirectSoundCreate)*)GetProcAddress(md_dsound, "DirectSoundCreate");

	ERROR_EXIT(!pDirectSoundCreate);
	IDirectSound* pDS;
	ERROR_EXIT(DS_OK != pDirectSoundCreate(NULL, &pDS, NULL));
	ERROR_EXIT(DS_OK != pDS->lpVtbl->SetCooperativeLevel(pDS, GetHwnd(), DSSCL_PRIORITY));

	WAVEFORMATEX waveFormatEx{};
	DSBUFFERDESC dSBufferDesc{};
	waveFormatEx.wFormatTag = 1;
	waveFormatEx.nChannels = 1;
	waveFormatEx.nSamplesPerSec = 48000;
	waveFormatEx.wBitsPerSample = 16;
	waveFormatEx.nBlockAlign = 2;
	waveFormatEx.nAvgBytesPerSec = waveFormatEx.nSamplesPerSec * waveFormatEx.wBitsPerSample / 8;
	waveFormatEx.cbSize = 0;
	dSBufferDesc.dwSize = sizeof(dSBufferDesc);
	dSBufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
	dSBufferDesc.dwBufferBytes = waveFormatEx.nAvgBytesPerSec;
	dSBufferDesc.dwReserved = 0;
	dSBufferDesc.lpwfxFormat = &waveFormatEx;
	dSBufferDesc.guid3DAlgorithm = {};
	IDirectSoundBuffer *pDSBuff;
	ERROR_EXIT(DS_OK != pDS->lpVtbl->CreateSoundBuffer(pDS, &dSBufferDesc, &pDSBuff, NULL));

	printf("DirectSoundCreate : dsound.dll+%08X\n", (unsigned)pDirectSoundCreate - (unsigned)md_dsound);
	printf("IDirectSoundBuffer::Play : dsound.dll+%08X\n", (unsigned)pDSBuff->lpVtbl->Play - (unsigned)md_dsound);
	printf("\n");

	pDSBuff->lpVtbl->Release(pDSBuff);
	pDS->lpVtbl->Release(pDS);
	FreeLibrary(md_dsound);

	return true;
}


bool GetDInput() {
	HMODULE md_dinput = LoadLibraryA("C:\\Windows\\System32\\dinput8.dll");
	auto pDirectInput8Create = (decltype(DirectInput8Create)*)GetProcAddress(md_dinput, "DirectInput8Create");
	ERROR_EXIT(!pDirectInput8Create);

	const GUID guid_IID_IDirectInput8 = { 0xBF798030, 0x483A, 0x4DA2, 0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00 };
	const GUID guid_SysKeyboard = { 0x6F1D2B61, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };

	IDirectInput8* pDI;
	auto p = GetModuleHandle(NULL);
	ERROR_EXIT(DI_OK != pDirectInput8Create(p, DIRECTINPUT_VERSION, guid_IID_IDirectInput8, (void**)&pDI, NULL));

	IDirectInputDevice8* pDID;
	ERROR_EXIT(DI_OK != pDI->lpVtbl->CreateDevice(pDI, guid_SysKeyboard, &pDID, NULL));

	printf("IDirectInput8::CreateDevice : dinput8.dll+%08X\n", (unsigned)pDI->lpVtbl->CreateDevice - (unsigned)md_dinput);
	printf("IDirectInputDevice8::GetDeviceState : dinput8.dll+%08X\n", (unsigned)pDID->lpVtbl->GetDeviceState - (unsigned)md_dinput);
	printf("\n");

	pDID->lpVtbl->Release(pDID);
	pDI->lpVtbl->Release(pDI);
	FreeLibrary(md_dinput);

	return true;
}

bool GetDX9() {
	HMODULE md_d3d9 = LoadLibraryA("C:\\Windows\\System32\\d3d9.dll");
	auto pDirect3DCreate9 = (decltype(Direct3DCreate9)*)GetProcAddress(md_d3d9, "Direct3DCreate9");
	ERROR_EXIT(!pDirect3DCreate9);

	IDirect3D9* pD3D = pDirect3DCreate9(D3D_SDK_VERSION);
	ERROR_EXIT(!pD3D);

	D3DCAPS9 caps;
	pD3D->lpVtbl->GetDeviceCaps(pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
	int vp = 0;
	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	}
	else {
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.BackBufferWidth = 640;
	d3dpp.BackBufferHeight = 480;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality = 0;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = GetHwnd();
	d3dpp.Windowed = true;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.Flags = 0;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	IDirect3DDevice9* pD3DD = NULL;
	pD3D->lpVtbl->CreateDevice(pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetHwnd(), vp, &d3dpp, &pD3DD);
	ERROR_EXIT(!pD3DD);

	printf("Direct3DCreate9 : d3d9.dll+%08X\n", (unsigned)pDirect3DCreate9 - (unsigned)md_d3d9);
	printf("IDirect3D9::CreateDevice : d3d9.dll+%08X\n", (unsigned)pD3D->lpVtbl->CreateDevice - (unsigned)md_d3d9);
	printf("IDirect3DDevice9::Present : d3d9.dll+%08X\n", (unsigned)pD3DD->lpVtbl->Present - (unsigned)md_d3d9);
	printf("\n");

	pD3DD->lpVtbl->Release(pD3DD);
	pD3D->lpVtbl->Release(pD3D);
	FreeLibrary(md_d3d9);

	return true;
}
