//-----------------------------------------------------------------------------
// ファイル: Texture.cpp
//
// 説明: テクスチャの作成方法を示すコード。このテクスチャ コードは簡略化されて
//       いるため、ロードできるのは16ビットのテクスチャのみである。また、テクス
//       チャの管理についても、完全ではない。
//
// Copyright (c) 1998-1999 Microsoft Corporation. All rights reserved
//-----------------------------------------------------------------------------

#include "texture.h"



//-----------------------------------------------------------------------------
// 名前: TextureSearchCallback()
// 説明: 16ビットのテクスチャ フォーマットを探すための列挙コールバック関数。
//       この関数はID3DDevice::EnumTextureFormats()から起動され、指定されたデバ
//       イスで使用可能なテクスチャ フォーマットをすべて列挙する。
//       列挙されたテクスチャ フォーマットのそれぞれのピクセル フォーマットは
//       "pddpf"パラメータに渡される。2番目のパラメータはアプリケーションが
//       チェックを行うために使われる。この場合、通常の16ビット テクスチャ
//       フォーマットを返す出力パラメータとしてこれを使用する。
//-----------------------------------------------------------------------------
static HRESULT CALLBACK TextureSearchCallback( DDPIXELFORMAT* pddpf,VOID* param )
{
	// 注: DDENUMRET_OKで戻り、フォーマットの列挙を続ける。

	// 特殊なモードは全てスキップ
    if( pddpf->dwFlags & (DDPF_LUMINANCE|DDPF_BUMPLUMINANCE|DDPF_BUMPDUDV) )
        return DDENUMRET_OK;
    
	// FourCCフォーマットは全てスキップ
    if( pddpf->dwFourCC != 0 )
        return DDENUMRET_OK;

	// アルファ モードはスキップ
    if( pddpf->dwFlags&DDPF_ALPHAPIXELS )
        return DDENUMRET_OK;

	// 16ビット フォーマットだけを探しているので、他は全てスキップ
    if( pddpf->dwRGBBitCount != 16 )
        return DDENUMRET_OK;

	// 適合するものを見つけたので、現在のピクセル フォーマットを出力
    // パラメータににコピー
    memcpy( (DDPIXELFORMAT*)param, pddpf, sizeof(DDPIXELFORMAT) );

    // DDENUMRET_CANCEL で戻り、列挙を終了
    return DDENUMRET_CANCEL;
}




//-----------------------------------------------------------------------------
// 名前: CreateTextureFromBitmap()
// 説明: ビットマップを使用して指定したデバイス用のテクスチャを作成する。この
//       コードはビットマップからテクスチャのアトリビュートを取得し、テクスチャ
//       を作成し、ビットマップをテクスチャにコピーする。
//-----------------------------------------------------------------------------
static LPDIRECTDRAWSURFACE7 CreateTextureFromBitmap( LPDIRECT3DDEVICE7 pd3dDevice,HBITMAP hbm )
{
    LPDIRECTDRAWSURFACE7 pddsTexture;
    HRESULT hr;

	// デバイスの能力を取得し、テクスチャを使用する際の制約がないか調べる。
    D3DDEVICEDESC7 ddDesc;
    if( FAILED( pd3dDevice->GetCaps( &ddDesc ) ) )
        return NULL;

	// ビットマップの構造体を取得(幅、高さ、bppを抽出するため)
    BITMAP bm;
    GetObject( hbm, sizeof(BITMAP), &bm );
    DWORD dwWidth  = (DWORD)bm.bmWidth;
    DWORD dwHeight = (DWORD)bm.bmHeight;

	// テクスチャのための新しいサーフェイス記述を設定する。テクスチャ管理
	// アトリビュートの使い方に注意。Direct3Dに面倒な作業の多くを任せている。
    DDSURFACEDESC2 ddsd;
    ZeroMemory( &ddsd, sizeof(DDSURFACEDESC2) );
    ddsd.dwSize          = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags         = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|
                           DDSD_PIXELFORMAT|DDSD_TEXTURESTAGE;
    ddsd.ddsCaps.dwCaps  = DDSCAPS_TEXTURE;
    ddsd.dwWidth         = dwWidth;
    ddsd.dwHeight        = dwHeight;

	// ハードウェア デバイスでのテクスチャ管理を有効にする。
    if( ddDesc.deviceGUID == IID_IDirect3DHALDevice )
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    else if( ddDesc.deviceGUID == IID_IDirect3DTnLHalDevice )
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    else
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

	// ドライバで必要な場合に幅と高さを調整
    if( ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2 )
    {
        for( ddsd.dwWidth=1;  dwWidth>ddsd.dwWidth;   ddsd.dwWidth<<=1 );
        for( ddsd.dwHeight=1; dwHeight>ddsd.dwHeight; ddsd.dwHeight<<=1 );
    }
    if( ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY )
    {
        if( ddsd.dwWidth > ddsd.dwHeight ) ddsd.dwHeight = ddsd.dwWidth;
        else                               ddsd.dwWidth  = ddsd.dwHeight;
    }

	// テクスチャ フォーマットを列挙し、最も近いデバイスに対応したテクスチャ
	// のピクセル フォーマットを探す。このチュートリアルのTextureSearchCallback
	// 関数は単に16ビットテクスチャを探すだけだが、実際のアプリケーションでは
	// アルファ テクスチャやバンプマップなど、他のフォーマットを使用することも
	// 考えられる。
    pd3dDevice->EnumTextureFormats( TextureSearchCallback, &ddsd.ddpfPixelFormat );
    if( 0L == ddsd.ddpfPixelFormat.dwRGBBitCount )
        return NULL;

	// デバイスのレンダリング対象を取得し、DDrawオブジェクトへのポインタを
	// 取得するために使用する。サーフェイスを作成するためには、DirectDraw
	// インターフェイスが必要である。
    LPDIRECTDRAWSURFACE7 pddsRender;
    LPDIRECTDRAW7        pDD;
    pd3dDevice->GetRenderTarget( &pddsRender );
    pddsRender->GetDDInterface( (VOID**)&pDD );
    pddsRender->Release();

	// テクスチャのための新しいサーフェイスを作成
    if( FAILED( hr = pDD->CreateSurface( &ddsd, &pddsTexture, NULL ) ) )
    {
        pDD->Release();
        return NULL;
    }

	// DDraw を使った処理が完了
    pDD->Release();

	// ビットマップをテクスチャ サーフェイスにコピーできるようになった。これを
	// 行うために、ビットマップのDCとサーフェイスのDCを作成する。次にBitBlt()
	// を使って実際のビットをコピーする。

	// ビットマップのDCを取得
    HDC hdcBitmap = CreateCompatibleDC( NULL );
    if( NULL == hdcBitmap )
    {
        pddsTexture->Release();
        return NULL;
    }
    SelectObject( hdcBitmap, hbm );

	// サーフェイスのDCを取得
    HDC hdcTexture;
    if( SUCCEEDED( pddsTexture->GetDC( &hdcTexture ) ) )
    {
		// ビットマップ イメージをサーフェイスにコピー
        BitBlt( hdcTexture, 0, 0, bm.bmWidth, bm.bmHeight, hdcBitmap,
                0, 0, SRCCOPY );
        pddsTexture->ReleaseDC( hdcTexture );
    }
    DeleteDC( hdcBitmap );

	// 新しく作成されたテクスチャを返す。
    return pddsTexture;
}




//-----------------------------------------------------------------------------
// 名前: CreateTexture()
// 説明: ファイル名を受け取り、そのファイルからローカルなビットマップを作成す
//       る。ここにファイル処理のコードを挿入することで、さまざまな画像フォー
//       マットをサポートすることもできる。
//-----------------------------------------------------------------------------
LPDIRECTDRAWSURFACE7 CreateTexture( LPDIRECT3DDEVICE7 pd3dDevice, CHAR* strName )
{
	// ビットマップを作成し、そこにテクスチャファイルをロードする。
	// 実行可能なリソースを先にチェックする。
    HBITMAP hbm = (HBITMAP)LoadImage( GetModuleHandle(NULL), strName, 
                              IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
    if( NULL == hbm )
    {
		// リソース内に無ければビットマップをファイルとしてロードする。
		// 実際のコードでは、さまざまなファイル パスでビットマップを
		// 探す必要があるかも知れない。
        hbm = (HBITMAP)LoadImage( NULL, strName, IMAGE_BITMAP, 0, 0, 
                                  LR_LOADFROMFILE|LR_CREATEDIBSECTION );
        if( NULL == hbm )
            return NULL;
    }

	// テクスチャを作成する実際の作業は次の関数で行われる。
    return CreateTextureFromBitmap( pd3dDevice, hbm );
}




