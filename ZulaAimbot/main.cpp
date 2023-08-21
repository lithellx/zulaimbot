#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "funcs.h"
#include "detours.h"

#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "d3d9.lib")

using namespace std;

HRESULT WINAPI hkPresent(LPDIRECT3DDEVICE9 pDevice, const RECT *a, const RECT *b, HWND c, const RGNDATA *d)
{
	__asm PUSHAD;

	ScreenCenterX = viewport.Width / 2.0f;
	ScreenCenterY = viewport.Height / 2.0f;

	__asm POPAD;
	return oPresent(pDevice, a, b, c, d);
}

HRESULT WINAPI hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	if (cPlayerA.size() != NULL)
	{
		UINT BestTarget = -1;
		DOUBLE fClosestPos = 99999;
		for (size_t i = 0; i < cPlayerA.size(); i += 1)
		{
			float radiusx = aimfov * (ScreenCenterX / 100);
			float radiusy = aimfov * (ScreenCenterY / 100);

			cPlayerA[i]->CrossDist = GetDistance(cPlayerA[i]->Player.x, cPlayerA[i]->Player.y, ScreenCenterX, ScreenCenterY);
			if (cPlayerA[i]->Player.x >= ScreenCenterX - radiusx &&
				cPlayerA[i]->Player.x <= ScreenCenterX + radiusx &&
				cPlayerA[i]->Player.y >= ScreenCenterY - radiusy &&
				cPlayerA[i]->Player.y <= ScreenCenterY + radiusy)
				if (cPlayerA[i]->CrossDist < fClosestPos)
				{
					fClosestPos = cPlayerA[i]->CrossDist;
					BestTarget = i;
				}
		}
		if (BestTarget != -1)
		{
			double DistX = (double)cPlayerA[BestTarget]->Player.x - viewport.Width / 2.0f;
			double DistY = (double)cPlayerA[BestTarget]->Player.y - viewport.Height / 2.0f;

			aimsmooth /= DistX;
			aimsmooth /= DistY;

			if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
				mouse_event(MOUSEEVENTF_MOVE, DistX, DistY, NULL, NULL);
			}
		}
		cPlayerA.clear();
	}
	return oEndScene(pDevice);
}

HRESULT WINAPI hkDrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT PrimitiveCount)
{
	__asm nop
	HRESULT hRet = oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, PrimitiveCount);

	return hRet;
}

_declspec (naked) HRESULT WINAPI playersAll()
{
	static LPDIRECT3DDEVICE9 pDevice;
	__asm
	{
		xor ebx, ebx
		imul edx, edx, 03
		mov dword ptr ds : [pDevice], eax
		pushad
	}

	W2AIM(pDevice, AimHhead);

/*
	W2AIM(pDevice, AimHbody);

	W2AIM(pDevice, AimHlegs);
*/
	__asm
	{
		popad
		jmp RetDX
	}
}

HRESULT WINAPI hkSetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
	__asm nop
	if (StreamNumber == 0) {
		uiStride = Stride;
	}
	return oSetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes, Stride);
}

LRESULT CALLBACK MsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return DefWindowProc(hwnd, uMsg, wParam, lParam); }
void DX_Init(DWORD* table)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassEx(&wc);
	HWND hWnd = CreateWindow("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, GetDesktopWindow(), NULL, wc.hInstance, NULL);
	LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	LPDIRECT3DDEVICE9 pd3dDevice;
	pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pd3dDevice);
	DWORD* pVTable = (DWORD*)pd3dDevice;
	pVTable = (DWORD*)pVTable[0];
	table[ES] = pVTable[42];
	table[DIP] = pVTable[82];
	table[SSS] = pVTable[100];
	DestroyWindow(hWnd);
}

DWORD WINAPI Thread(LPVOID)
{
	DWORD VTable[3] = { 0 };
	while (GetModuleHandle("d3d9.dll") == NULL) {
		Sleep(250);
	}
	DWORD dwVTable[2] = { 0 };
	CreateDevice(dwVTable);
	oPresent = (tPresent)DetourCreate((PBYTE)dwVTable[1], (PBYTE)&hkPresent, 5);
	DX_Init(VTable);
	HOOK(EndScene, VTable[ES]);
	HOOK(DrawIndexedPrimitive, VTable[DIP]);
	HOOK(SetStreamSource, VTable[SSS]);
	DetourCreate((PBYTE)dxAddr, (PBYTE)playersAll, 5);
	return 0;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		CreateThread(0, 0, Thread, 0, 0, 0);
	}
	return TRUE;
}

// github.com/lithellx - Credit: Uc/Mo1ra
