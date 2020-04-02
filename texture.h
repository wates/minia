#ifndef _TEXTURE_H_INC
#define _TEXTURE_H_INC
#include <windows.h>
#include <d3d.h>

static HRESULT CALLBACK TextureSearchCallback( DDPIXELFORMAT* pddpf,VOID* param );
static LPDIRECTDRAWSURFACE7 CreateTextureFromBitmap( LPDIRECT3DDEVICE7 pd3dDevice,HBITMAP hbm );
LPDIRECTDRAWSURFACE7 CreateTexture( LPDIRECT3DDEVICE7 pd3dDevice, CHAR* strName );


#endif