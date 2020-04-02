//-----------------------------------------------------------------------------
// �t�@�C��: Texture.cpp
//
// ����: �e�N�X�`���̍쐬���@�������R�[�h�B���̃e�N�X�`�� �R�[�h�͊ȗ��������
//       ���邽�߁A���[�h�ł���̂�16�r�b�g�̃e�N�X�`���݂̂ł���B�܂��A�e�N�X
//       �`���̊Ǘ��ɂ��Ă��A���S�ł͂Ȃ��B
//
// Copyright (c) 1998-1999 Microsoft Corporation. All rights reserved
//-----------------------------------------------------------------------------

#include "texture.h"



//-----------------------------------------------------------------------------
// ���O: TextureSearchCallback()
// ����: 16�r�b�g�̃e�N�X�`�� �t�H�[�}�b�g��T�����߂̗񋓃R�[���o�b�N�֐��B
//       ���̊֐���ID3DDevice::EnumTextureFormats()����N������A�w�肳�ꂽ�f�o
//       �C�X�Ŏg�p�\�ȃe�N�X�`�� �t�H�[�}�b�g�����ׂė񋓂���B
//       �񋓂��ꂽ�e�N�X�`�� �t�H�[�}�b�g�̂��ꂼ��̃s�N�Z�� �t�H�[�}�b�g��
//       "pddpf"�p�����[�^�ɓn�����B2�Ԗڂ̃p�����[�^�̓A�v���P�[�V������
//       �`�F�b�N���s�����߂Ɏg����B���̏ꍇ�A�ʏ��16�r�b�g �e�N�X�`��
//       �t�H�[�}�b�g��Ԃ��o�̓p�����[�^�Ƃ��Ă�����g�p����B
//-----------------------------------------------------------------------------
static HRESULT CALLBACK TextureSearchCallback( DDPIXELFORMAT* pddpf,VOID* param )
{
	// ��: DDENUMRET_OK�Ŗ߂�A�t�H�[�}�b�g�̗񋓂𑱂���B

	// ����ȃ��[�h�͑S�ăX�L�b�v
    if( pddpf->dwFlags & (DDPF_LUMINANCE|DDPF_BUMPLUMINANCE|DDPF_BUMPDUDV) )
        return DDENUMRET_OK;
    
	// FourCC�t�H�[�}�b�g�͑S�ăX�L�b�v
    if( pddpf->dwFourCC != 0 )
        return DDENUMRET_OK;

	// �A���t�@ ���[�h�̓X�L�b�v
    if( pddpf->dwFlags&DDPF_ALPHAPIXELS )
        return DDENUMRET_OK;

	// 16�r�b�g �t�H�[�}�b�g������T���Ă���̂ŁA���͑S�ăX�L�b�v
    if( pddpf->dwRGBBitCount != 16 )
        return DDENUMRET_OK;

	// �K��������̂��������̂ŁA���݂̃s�N�Z�� �t�H�[�}�b�g���o��
    // �p�����[�^�ɂɃR�s�[
    memcpy( (DDPIXELFORMAT*)param, pddpf, sizeof(DDPIXELFORMAT) );

    // DDENUMRET_CANCEL �Ŗ߂�A�񋓂��I��
    return DDENUMRET_CANCEL;
}




//-----------------------------------------------------------------------------
// ���O: CreateTextureFromBitmap()
// ����: �r�b�g�}�b�v���g�p���Ďw�肵���f�o�C�X�p�̃e�N�X�`�����쐬����B����
//       �R�[�h�̓r�b�g�}�b�v����e�N�X�`���̃A�g���r���[�g���擾���A�e�N�X�`��
//       ���쐬���A�r�b�g�}�b�v���e�N�X�`���ɃR�s�[����B
//-----------------------------------------------------------------------------
static LPDIRECTDRAWSURFACE7 CreateTextureFromBitmap( LPDIRECT3DDEVICE7 pd3dDevice,HBITMAP hbm )
{
    LPDIRECTDRAWSURFACE7 pddsTexture;
    HRESULT hr;

	// �f�o�C�X�̔\�͂��擾���A�e�N�X�`�����g�p����ۂ̐��񂪂Ȃ������ׂ�B
    D3DDEVICEDESC7 ddDesc;
    if( FAILED( pd3dDevice->GetCaps( &ddDesc ) ) )
        return NULL;

	// �r�b�g�}�b�v�̍\���̂��擾(���A�����Abpp�𒊏o���邽��)
    BITMAP bm;
    GetObject( hbm, sizeof(BITMAP), &bm );
    DWORD dwWidth  = (DWORD)bm.bmWidth;
    DWORD dwHeight = (DWORD)bm.bmHeight;

	// �e�N�X�`���̂��߂̐V�����T�[�t�F�C�X�L�q��ݒ肷��B�e�N�X�`���Ǘ�
	// �A�g���r���[�g�̎g�����ɒ��ӁBDirect3D�ɖʓ|�ȍ�Ƃ̑�����C���Ă���B
    DDSURFACEDESC2 ddsd;
    ZeroMemory( &ddsd, sizeof(DDSURFACEDESC2) );
    ddsd.dwSize          = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags         = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|
                           DDSD_PIXELFORMAT|DDSD_TEXTURESTAGE;
    ddsd.ddsCaps.dwCaps  = DDSCAPS_TEXTURE;
    ddsd.dwWidth         = dwWidth;
    ddsd.dwHeight        = dwHeight;

	// �n�[�h�E�F�A �f�o�C�X�ł̃e�N�X�`���Ǘ���L���ɂ���B
    if( ddDesc.deviceGUID == IID_IDirect3DHALDevice )
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    else if( ddDesc.deviceGUID == IID_IDirect3DTnLHalDevice )
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    else
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

	// �h���C�o�ŕK�v�ȏꍇ�ɕ��ƍ����𒲐�
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

	// �e�N�X�`�� �t�H�[�}�b�g��񋓂��A�ł��߂��f�o�C�X�ɑΉ������e�N�X�`��
	// �̃s�N�Z�� �t�H�[�}�b�g��T���B���̃`���[�g���A����TextureSearchCallback
	// �֐��͒P��16�r�b�g�e�N�X�`����T�����������A���ۂ̃A�v���P�[�V�����ł�
	// �A���t�@ �e�N�X�`����o���v�}�b�v�ȂǁA���̃t�H�[�}�b�g���g�p���邱�Ƃ�
	// �l������B
    pd3dDevice->EnumTextureFormats( TextureSearchCallback, &ddsd.ddpfPixelFormat );
    if( 0L == ddsd.ddpfPixelFormat.dwRGBBitCount )
        return NULL;

	// �f�o�C�X�̃����_�����O�Ώۂ��擾���ADDraw�I�u�W�F�N�g�ւ̃|�C���^��
	// �擾���邽�߂Ɏg�p����B�T�[�t�F�C�X���쐬���邽�߂ɂ́ADirectDraw
	// �C���^�[�t�F�C�X���K�v�ł���B
    LPDIRECTDRAWSURFACE7 pddsRender;
    LPDIRECTDRAW7        pDD;
    pd3dDevice->GetRenderTarget( &pddsRender );
    pddsRender->GetDDInterface( (VOID**)&pDD );
    pddsRender->Release();

	// �e�N�X�`���̂��߂̐V�����T�[�t�F�C�X���쐬
    if( FAILED( hr = pDD->CreateSurface( &ddsd, &pddsTexture, NULL ) ) )
    {
        pDD->Release();
        return NULL;
    }

	// DDraw ���g��������������
    pDD->Release();

	// �r�b�g�}�b�v���e�N�X�`�� �T�[�t�F�C�X�ɃR�s�[�ł���悤�ɂȂ����B�����
	// �s�����߂ɁA�r�b�g�}�b�v��DC�ƃT�[�t�F�C�X��DC���쐬����B����BitBlt()
	// ���g���Ď��ۂ̃r�b�g���R�s�[����B

	// �r�b�g�}�b�v��DC���擾
    HDC hdcBitmap = CreateCompatibleDC( NULL );
    if( NULL == hdcBitmap )
    {
        pddsTexture->Release();
        return NULL;
    }
    SelectObject( hdcBitmap, hbm );

	// �T�[�t�F�C�X��DC���擾
    HDC hdcTexture;
    if( SUCCEEDED( pddsTexture->GetDC( &hdcTexture ) ) )
    {
		// �r�b�g�}�b�v �C���[�W���T�[�t�F�C�X�ɃR�s�[
        BitBlt( hdcTexture, 0, 0, bm.bmWidth, bm.bmHeight, hdcBitmap,
                0, 0, SRCCOPY );
        pddsTexture->ReleaseDC( hdcTexture );
    }
    DeleteDC( hdcBitmap );

	// �V�����쐬���ꂽ�e�N�X�`����Ԃ��B
    return pddsTexture;
}




//-----------------------------------------------------------------------------
// ���O: CreateTexture()
// ����: �t�@�C�������󂯎��A���̃t�@�C�����烍�[�J���ȃr�b�g�}�b�v���쐬��
//       ��B�����Ƀt�@�C�������̃R�[�h��}�����邱�ƂŁA���܂��܂ȉ摜�t�H�[
//       �}�b�g���T�|�[�g���邱�Ƃ��ł���B
//-----------------------------------------------------------------------------
LPDIRECTDRAWSURFACE7 CreateTexture( LPDIRECT3DDEVICE7 pd3dDevice, CHAR* strName )
{
	// �r�b�g�}�b�v���쐬���A�����Ƀe�N�X�`���t�@�C�������[�h����B
	// ���s�\�ȃ��\�[�X���Ƀ`�F�b�N����B
    HBITMAP hbm = (HBITMAP)LoadImage( GetModuleHandle(NULL), strName, 
                              IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
    if( NULL == hbm )
    {
		// ���\�[�X���ɖ�����΃r�b�g�}�b�v���t�@�C���Ƃ��ă��[�h����B
		// ���ۂ̃R�[�h�ł́A���܂��܂ȃt�@�C�� �p�X�Ńr�b�g�}�b�v��
		// �T���K�v�����邩���m��Ȃ��B
        hbm = (HBITMAP)LoadImage( NULL, strName, IMAGE_BITMAP, 0, 0, 
                                  LR_LOADFROMFILE|LR_CREATEDIBSECTION );
        if( NULL == hbm )
            return NULL;
    }

	// �e�N�X�`�����쐬������ۂ̍�Ƃ͎��̊֐��ōs����B
    return CreateTextureFromBitmap( pd3dDevice, hbm );
}




