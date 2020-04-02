#include "DDClass.h"

DirectDraw::DirectDraw()
{
	lpDD=NULL;
	lpDDSPrimary=NULL;
	lpDDSBack=NULL;
	lpDDSWall=NULL;
	lpDDTBeat=NULL;
	lpDDTBar=NULL;
	lpDDTVelocity=NULL;
	ZeroMemory(&Bfx,sizeof(Bfx));
	Bfx.dwSize=sizeof(Bfx);
	lpD3D=NULL;
	lpD3DDevice=NULL;
	lpDI=NULL;
	lpDIDKey=NULL;
}

void DirectDraw::SetHandle(HWND hw,HINSTANCE hInst)
{
	hwnd=hw;
	hInstApp=hInst;
}

void DirectDraw::SetFlags(DWORD Flag)
{
	Flags=Flag;
}

HRESULT DirectDraw::Create()
{
	int width,height;
	if(Flags&1)
	{
		width=512;
		height=384;
	}
	else
	{
		RECT cRc;
		GetClientRect(hwnd,&cRc);
		width=cRc.right-cRc.left;
		height=cRc.bottom-cRc.top;
	}
	HRESULT hr;
	LPDIRECTDRAW pDD;
	if(DD_OK!=(hr=DirectDrawCreateEx(NULL,(void**)(&pDD),IID_IDirectDraw7,NULL)))
		return hr;
	if(Flags&1)
	{
		if(DD_OK!=(hr=pDD->SetCooperativeLevel(hwnd,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN|DDSCL_ALLOWREBOOT)))
			return hr;
	}
	else
	{
		if(DD_OK!=(hr=pDD->SetCooperativeLevel(hwnd,DDSCL_NORMAL)))
			return hr;
	}
	pDD->QueryInterface(IID_IDirectDraw7,(void**)&lpDD);
	pDD->Release();
	if(Flags&1)
	{
		if(DD_OK!=(hr=lpDD->SetDisplayMode(512,384,16,0,0)))
			return hr;
		//マウスを非表示する
		ShowCursor(FALSE);
		//プライマリーサーフェイス
		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd,sizeof(ddsd));
		ddsd.dwSize=sizeof(ddsd);
		ddsd.dwFlags=DDSD_CAPS|DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE|DDSCAPS_FLIP|DDSCAPS_COMPLEX|DDSCAPS_3DDEVICE;
		ddsd.dwBackBufferCount=1;
		if(DD_OK!=(hr=lpDD->CreateSurface(&ddsd,&lpDDSPrimary,NULL)))
			return hr;
		//バックサーフェイス
		DDSCAPS2 ddscaps;
		ZeroMemory(&ddscaps,sizeof(ddscaps));
		ddscaps.dwCaps=DDSCAPS_BACKBUFFER;
		if(DD_OK!=(hr=lpDDSPrimary->GetAttachedSurface(&ddscaps,&lpDDSBack)))
			return hr;
	}
	else
	{
		//プライマリーサーフェイス
		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd,sizeof(ddsd));
		ddsd.dwSize=sizeof(ddsd);
		ddsd.dwFlags=DDSD_CAPS;
		ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;
		if(DD_OK!=(hr=lpDD->CreateSurface(&ddsd,(LPDIRECTDRAWSURFACE7*)&lpDDSPrimary,NULL)))
			return hr;
		//バックバッファ
		ddsd.dwFlags=DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE;
		ddsd.dwWidth=width;
		ddsd.dwHeight=height;
		if(DD_OK!=(hr=lpDD->CreateSurface(&ddsd,(LPDIRECTDRAWSURFACE7*)&lpDDSBack,NULL)))
			return hr;
	}
	LPDIRECTDRAWCLIPPER lpDDClipper=NULL;
	//クリッパー
	if(DD_OK!=(hr=lpDD->CreateClipper(0,&lpDDClipper,NULL)))
		return hr;
	//画面全体
	struct
	{
		RGNDATAHEADER rdh;
		RECT clip;
	}cl;
	cl.rdh.dwSize=sizeof(cl);
	cl.rdh.iType=RDH_RECTANGLES;
	cl.rdh.nCount=1;
	cl.rdh.nRgnSize=sizeof(RECT);
	cl.rdh.rcBound.left=0;
	cl.rdh.rcBound.top=0;
	cl.rdh.rcBound.right=width;
	cl.rdh.rcBound.bottom=height;
	cl.clip.left=0;
	cl.clip.top=0;
	cl.clip.right=width;
	cl.clip.bottom=height;
	if(DD_OK!=(hr=lpDDClipper->SetClipList((RGNDATA*)&cl,0)))
		return hr;
	if(DD_OK!=(hr=lpDDSBack->SetClipper(lpDDClipper)))
		return hr;
	lpDDClipper->Release();
	//クライアント領域
	if(!(Flags&1))
	{
		LPDIRECTDRAWCLIPPER lpDDWindowClipper=NULL;
		if(DD_OK!=(hr=lpDD->CreateClipper(0,&lpDDWindowClipper,NULL)))
			return hr;
		if(DD_OK!=(hr=lpDDWindowClipper->SetHWnd(NULL,hwnd)))
			return hr;
		if(DD_OK!=(hr=lpDDSPrimary->SetClipper(lpDDWindowClipper)))
			return hr;
		lpDDWindowClipper->Release();
	}
	//DDからD3D生成
	if(DD_OK!=(hr=lpDD->QueryInterface(IID_IDirect3D7,(void**)&lpD3D)))
		return hr;
	//HALのデバイスを生成する
	if(D3D_OK!=lpD3D->CreateDevice(IID_IDirect3DHALDevice,lpDDSBack,&lpD3DDevice))
	{
		//HALが無理だったらMMXでやってみる
		if(D3D_OK!=lpD3D->CreateDevice(IID_IDirect3DMMXDevice,lpDDSBack,&lpD3DDevice))
			//MMXでも無理ならHELでがんばる
			if(D3D_OK!=(hr=lpD3D->CreateDevice(IID_IDirect3DRGBDevice,lpDDSBack,&lpD3DDevice)))
				//それでも無理ならあきらめる
				return hr;
		IsHAL=FALSE;
	}
	else
		IsHAL=TRUE;
	//ビューポートの設定
	D3DVIEWPORT7 D3DVP;
	D3DVP.dwX=0;
	D3DVP.dwY=0;
	D3DVP.dwWidth=width;
	D3DVP.dwHeight=height;
	D3DVP.dvMinZ=0.0f;
	D3DVP.dvMaxZ=1.0f;
	if(D3D_OK!=(hr=lpD3DDevice->SetViewport(&D3DVP)))
		return hr;
	lpD3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE,TRUE);//カラーキー有り
	lpD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND,D3DBLEND_SRCALPHA);//半透明設定
	//DirectInput
	LPDIRECTINPUT pDI=NULL;
	if(DI_OK!=(hr=DirectInputCreate(hInstApp,DIRECTINPUT_VERSION,&pDI,NULL)))
		return hr;
	pDI->QueryInterface(IID_IDirectInput2,(void **)&lpDI);
	pDI->Release();

	LPDIRECTINPUTDEVICE pDIDKey=NULL;
	if(DI_OK!=(hr=lpDI->CreateDevice(GUID_SysKeyboard,&pDIDKey,NULL)))
		return hr;
	pDIDKey->QueryInterface(IID_IDirectInputDevice,(void **)&lpDIDKey);
	pDIDKey->Release();
	if(DI_OK!=(hr=lpDIDKey->SetDataFormat(&c_dfDIKeyboard)))
		return hr;
	if(DI_OK!=(hr=lpDIDKey->SetCooperativeLevel(hwnd,DISCL_NONEXCLUSIVE|DISCL_BACKGROUND)))
		return hr;
	DIPROPDWORD dp;
	dp.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dp.diph.dwSize=sizeof(DIPROPDWORD);
	dp.diph.dwObj=0;
	dp.diph.dwHow=DIPH_DEVICE;
	dp.dwData=sizeof(DIDEVICEOBJECTDATA)*256;
	lpDIDKey->SetProperty(DIPROP_BUFFERSIZE,(LPCDIPROPHEADER)&dp);
	lpDIDKey->Acquire();
	return 0;
}

void DirectDraw::FillRect(RECT *rc,DWORD col)
{
	Bfx.dwFillColor=col;
	lpDDSBack->Blt(rc,NULL,NULL,DDBLT_COLORFILL,&Bfx);
}

BOOL DirectDraw::TextOut(int x,int y,char *tex,COLORREF col,COLORREF bk,int size,char *type)
{
	HDC hDC;
	HFONT hFont;
	hFont=CreateFont(size,0,0,0,0,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_DONTCARE,type);
	if(DD_OK!=lpDDSBack->GetDC(&hDC))return FALSE;
	SelectObject(hDC,hFont);
	SetTextColor(hDC,col);
	if(bk&0x80000000)
		SetBkMode(hDC,TRANSPARENT);
	else
		SetBkColor(hDC,bk);
	::TextOut(hDC,x,y,tex,strlen(tex));
	lpDDSBack->ReleaseDC(hDC);
	DeleteObject(hFont);
	return TRUE;
}

HRESULT DirectDraw::CreateSurface(LPDIRECTDRAWSURFACE7 *lplpDDS,int width,int height)
{
	DDSURFACEDESC2 DDSD;
	ZeroMemory(&DDSD,sizeof(DDSD));
 	DDSD.dwSize=sizeof(DDSD);
	DDSD.dwFlags=DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH;
	DDSD.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
	DDSD.dwWidth=width;
	DDSD.dwHeight=height;
	return lpDD->CreateSurface(&DDSD,lplpDDS,NULL);
}


BOOL DirectDraw::LoadBitmap(LPDIRECTDRAWSURFACE7 lpDDS,LPCSTR bitmap)
{
	HBITMAP hbm;
	HDC hdcImage;
	HDC hdc;
	if(!(hbm=(HBITMAP)LoadImage(NULL,bitmap,IMAGE_BITMAP,NULL,NULL,LR_CREATEDIBSECTION)))
		if(!(hbm=(HBITMAP)LoadImage(NULL,bitmap,IMAGE_BITMAP,NULL,NULL,LR_LOADFROMFILE|LR_CREATEDIBSECTION)))return FALSE;
	if(!(hdcImage=CreateCompatibleDC(NULL)))return FALSE;
	SelectObject(hdcImage,hbm);
	if(DD_OK!=lpDDS->GetDC(&hdc))return FALSE;
	if(!BitBlt(hdc,0,0,GetDeviceCaps(hdcImage,HORZRES),GetDeviceCaps(hdcImage,VERTRES),hdcImage,0,0,SRCCOPY))return FALSE;
	lpDDS->ReleaseDC(hdc);
	DeleteDC(hdcImage);
	return TRUE;
}

void DirectDraw::Flip()
{
	if(Flags&1)
	{
		lpDDSPrimary->Flip(NULL,NULL);
	}
	else
	{
		RECT Rect;
		GetWindowRect(hwnd,&Rect);
		Rect.left+=GetSystemMetrics(SM_CXEDGE)+GetSystemMetrics(SM_CXFIXEDFRAME)-1;
		Rect.top+=GetSystemMetrics(SM_CYEDGE)+GetSystemMetrics(SM_CYFIXEDFRAME)+GetSystemMetrics(SM_CYCAPTION)-1;
		Rect.right-=GetSystemMetrics(SM_CXEDGE)+GetSystemMetrics(SM_CXFIXEDFRAME)+1;
		Rect.bottom-=GetSystemMetrics(SM_CYEDGE)+GetSystemMetrics(SM_CYFIXEDFRAME)+1;
		lpDDSPrimary->Blt(&Rect,lpDDSBack,NULL,NULL,NULL);
	}
}

void DirectDraw::Release()
{
	lpDIDKey->Unacquire();
	lpDIDKey->Release();
	lpDI->Release();
	lpD3DDevice->Release();
	lpD3D->Release();
	lpDDTVelocity->Release();
	lpDDTBar->Release();
	lpDDTBeat->Release();
	lpDDSWall->Release();
	lpDDSBack->Release();
	lpDDSPrimary->Release();
	lpDD->Release();
}