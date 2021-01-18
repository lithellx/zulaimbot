#pragma warning(disable: 4244)
#pragma warning(disable: 4996)
bool Create;

LPD3DXLINE	 Line1;
LPD3DXLINE   Line2;
LPD3DXLINE   Line3;
D3DVIEWPORT9 viewport;
float ScreenCenterX;
float ScreenCenterY;

float AimHhead = 28.0f;
float AimHbody = 0.0f;
float AimHlegs = -28.0f;
struct ModelPlayer2
{
	D3DXVECTOR3 Player;
	float CrossDist;
};
std::vector<ModelPlayer2*>cPlayerA;

float GetDistance(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}
int aimsmooth = 2;
int aimfov = 60;

#define GWL_WNDPROC         (-4)
#define HOOK(func,addy)	o##func = (t##func)DetourFunction((PBYTE)addy,(PBYTE)hk##func)
#define ES	0
#define DIP	1
#define SSS	2
unsigned int	uiStride = NULL;

typedef HRESULT(WINAPI *tPresent)(LPDIRECT3DDEVICE9 pDevice, const RECT *a, const RECT *b, HWND c, const RGNDATA *d);
tPresent oPresent;

typedef HRESULT(WINAPI* tSetStreamSource)(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride);
tSetStreamSource oSetStreamSource;

typedef HRESULT(WINAPI* tEndScene)(LPDIRECT3DDEVICE9 pDevice);
tEndScene oEndScene;

typedef HRESULT(WINAPI* tDrawIndexedPrimitive)(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
tDrawIndexedPrimitive oDrawIndexedPrimitive;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

VOID CreateDevice(DWORD *dwVTable)
{
	LPDIRECT3D9 pD3d9;
	LPDIRECT3DDEVICE9 pD3DDevice;
	pD3d9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3d9 == NULL)
		return;
	D3DPRESENT_PARAMETERS pPresentParms;
	ZeroMemory(&pPresentParms, sizeof(pPresentParms));
	pPresentParms.Windowed = TRUE;
	pPresentParms.BackBufferFormat = D3DFMT_UNKNOWN;
	pPresentParms.SwapEffect = D3DSWAPEFFECT_DISCARD;
	if (FAILED(pD3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pPresentParms, &pD3DDevice)))
		return;
	DWORD *dwTable = (DWORD *)pD3DDevice;
	dwTable = (DWORD *)dwTable[0];
	dwVTable[0] = dwTable[16];
	dwVTable[1] = dwTable[17];
}

VOID *DetourCreate(BYTE *src, CONST BYTE *dst, CONST INT len)
{
	BYTE *jmp = (BYTE *)malloc(len + 5);
	DWORD dwBack;
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwBack);
	memcpy(jmp, src, len);
	jmp += len;
	jmp[0] = 0xE9;
	*(DWORD *)(jmp + 1) = (DWORD)(src + len - jmp) - 5;
	src[0] = 0xE9;
	*(DWORD *)(src + 1) = (DWORD)(dst - src) - 5;
	for (INT i = 5; i < len; i++)
		src[i] = 0x90;
	VirtualProtect(src, len, dwBack, &dwBack);
	return(jmp - len);
}

DWORD ESADDR = (DWORD)GetModuleHandleA("acknex.dll") + 0x3A74A; // endscene
DWORD RETES = (ESADDR + 0x8);

DWORD dxAddr = (DWORD)GetModuleHandleA("d3dx9_42.dll") + 0x4DEB0; // all players
DWORD RetDX = (dxAddr + 0x5);

// Credit: Uc/Mo1ra
