#ifndef __DDCLASS_H
#define __DDCLASS_H

#include <ddraw.h>
#include <d3d.h>
#include <dinput.h>
#include "texture.h"
class DirectDraw
{
	LPDIRECTDRAW7 lpDD;
	DDBLTFX Bfx;
	LPDIRECT3D7 lpD3D;
	LPDIRECTINPUT7 lpDI;
	HWND hwnd;
	HINSTANCE hInstApp;
	DWORD Flags;
public:
	LPDIRECTDRAWSURFACE7 lpDDSPrimary;
	LPDIRECTDRAWSURFACE7 lpDDSBack;
	LPDIRECTDRAWSURFACE7 lpDDSWall;
	LPDIRECTDRAWSURFACE7 lpDDTBeat;
	LPDIRECTDRAWSURFACE7 lpDDTBar;
	LPDIRECTDRAWSURFACE7 lpDDTVelocity;
	LPDIRECT3DDEVICE7 lpD3DDevice;
	LPDIRECTINPUTDEVICE7 lpDIDKey;
	BOOL IsHAL;
	void SetHandle(HWND hw,HINSTANCE hInst);
	void SetFlags(DWORD Flag);
	HRESULT Create();//����
	void FillRect(RECT *rc,DWORD col);//�h��Ԃ�
	HRESULT CreateSurface(LPDIRECTDRAWSURFACE7 *lplpDDS,int width,int height);
	BOOL LoadBitmap(LPDIRECTDRAWSURFACE7 lpDDS,LPCSTR bitmap);
	BOOL TextOut(int x,int y,char *tex,COLORREF col=0x00ffffff,COLORREF bk=0x80000000,int size=0,char *type="FixedSys");//�e�L�X�g�A�E�g
	void Release();//�J��
	void Flip();//�t���b�v
	DirectDraw();//������
};

#endif//__DDCLASS_H