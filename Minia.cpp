#include "Minia.h"

//メイン
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgs, int nWinMode )
{
	//ウインドウクラス定義
	WNDCLASS wc;
	wc.hInstance=hInstance;
	wc.lpszClassName="Minia";
	wc.lpfnWndProc=(WNDPROC)MainWndProc;
	wc.style=NULL;
	wc.hIcon=LoadIcon(hInstance,"MINIA");
	wc.hCursor=LoadCursor((HINSTANCE)NULL,IDC_ARROW);
	wc.lpszMenuName=NULL;
	wc.cbClsExtra=NULL;
	wc.cbWndExtra=NULL;
	wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
	if(!RegisterClass(&wc))
		return FALSE;
	//ウインドウハンドル取得
	HWND hwnd=CreateWindow("Minia","Minia",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,512+GetSystemMetrics(SM_CXEDGE)+GetSystemMetrics(SM_CXFIXEDFRAME)*2,384+GetSystemMetrics(SM_CYEDGE)+GetSystemMetrics(SM_CYFIXEDFRAME)*2+GetSystemMetrics(SM_CYCAPTION),NULL,NULL,hInstance,NULL);
	if(!hwnd)
		return FALSE;
	//ウインドウ表示
	ShowWindow(hwnd,nWinMode);
	UpdateWindow(hwnd);
	//ゲームメインスレッド開始
	if(lpszArgs[0])
	{
		FileName=new char[strlen(lpszArgs)];
		strcpy(FileName,lpszArgs);
	}
	else
	{
		GetCurrentDirectory(1024,CurrentDirectory);
/*		OPENFILENAME Open;
		ZeroMemory(&Open,sizeof(Open));
		Open.lStructSize=sizeof(Open);
		Open.hInstance=hInstance;
		Open.hwndOwner=hwnd;
		Open.lpstrFilter="*.mid";
		Open.lpstrTitle="ファイルを開く";
		Open.lpstrInitialDir=cd;
//		Open.lpstrFile=new char[1024];
		Open.nMaxFile=1024;
		Open.nMaxFileTitle=256;
		Open.nMaxCustFilter=256;*/
    static TCHAR strFileName[512];
    TCHAR strCurrentName[512] = "*.mid";
    

    OPENFILENAME Open = { sizeof(OPENFILENAME), hwnd, NULL,
                         "SMF Files(*.mid)\0*.mid\0\0",
                         NULL, 0, 1, strCurrentName, 512, strFileName, 512,
                         CurrentDirectory,"ファイルを開く", OFN_FILEMUSTEXIST, 0, 1,
                         ".mid", 0, NULL, NULL };
		GetOpenFileName(&Open);
		FileName=new char[1024];
		strcpy(FileName,Open.lpstrFile);
	}
	DD.SetHandle(hwnd,hInstance);
	_beginthread(MainThread,0,&hwnd);
	//メッセージポンプ
	MSG msg;
	while(GetMessage(&msg,(HWND)NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return TRUE;
}

//メインウインドウコールバック
LRESULT APIENTRY MainWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	BOOL bActive;
	switch (message)
	{
	case WM_ACTIVATEAPP:
		bActive = wParam;
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		break;
	default:
		return DefWindowProc( hwnd, message, wParam, lParam );
	}
	return 0;
}

//ゲームメインスレッド
void MainThread(void *Param)
{
	HWND hw=*((HWND*)Param);
	MainGame Game(hw);
	if(!Game.Initialize())
	{
		_endthread();
		return;
	}
	Game.Loop();
	SendMessage(hw,WM_CLOSE,0,0);
	_endthread();
}

MainGame::MainGame(HWND hw)
{
	hwnd=hw;
}

BOOL MainGame::Initialize()
{
	char buf[1024];
	char iniFile[1024];
	wsprintf(iniFile,"%s\\Minia.ini",CurrentDirectory);
	DD.SetFlags(Flag=GetPrivateProfileInt("Game","Flags",0,iniFile));

	if(!Midi.Open(FileName))
	{
		MessageBox(hwnd,"ファイルの形式が違うかオープンに失敗しました",FileName,MB_OK);
		SendMessage(hwnd,WM_CLOSE,0,0);
		return FALSE;
	}
	if(Midi.Header.Format!=0)
	{
		if(IDCANCEL==MessageBox(hwnd,"MIDIフォーマット0でないのでちゃんと再生されるかわかりません",FileName,MB_OKCANCEL|MB_ICONINFORMATION))
		{
			SendMessage(hwnd,WM_CLOSE,0,0);
			return FALSE;
		}
	}
	Midi.CreateFrame();
	if(DD.Create())
	{
		MessageBox(hwnd,"初期化に失敗しました",NULL,MB_OK);
		SendMessage(hwnd,WM_CLOSE,0,0);
		return FALSE;
	}
	Out.Open();
	ZeroMemory(&Header,sizeof(MidiHeader));
	//設定の読みこみ
	for(int i=0;i<12;i++)
	{
		wsprintf(buf,"Key%d",i);
		int f=GetPrivateProfileInt("KeySetting",buf,0,iniFile);
		if(f)
			KeySetting[i]=f;
	}
	Diff=GetPrivateProfileInt("Game","Mode",5,iniFile);
	Part=GetPrivateProfileInt("Game","Part",0,iniFile)|0x90;
	GetPrivateProfileString("Bitmap","Wall",NULL,buf,1024,iniFile);
	DD.CreateSurface(&DD.lpDDSWall,512,384);
	if(buf)
	{
		DD.LoadBitmap(DD.lpDDSWall,buf);
	}
	else
	{
		RECT Rect;
		GetWindowRect(hwnd,&Rect);
		Rect.left+=GetSystemMetrics(SM_CXEDGE)+GetSystemMetrics(SM_CXFIXEDFRAME)-1;
		Rect.top+=GetSystemMetrics(SM_CYEDGE)+GetSystemMetrics(SM_CYFIXEDFRAME)+GetSystemMetrics(SM_CYCAPTION)-1;
		Rect.right-=GetSystemMetrics(SM_CXEDGE)+GetSystemMetrics(SM_CXFIXEDFRAME)+1;
		Rect.bottom-=GetSystemMetrics(SM_CYEDGE)+GetSystemMetrics(SM_CYFIXEDFRAME)+1;
		DD.lpDDSWall->Blt(NULL,DD.lpDDSPrimary,NULL,NULL,NULL);
	}
	GetPrivateProfileString("Bitmap","Beat","Beat.bmp",buf,1024,iniFile);
	DD.lpDDTBeat=CreateTexture(DD.lpD3DDevice,buf);
	GetPrivateProfileString("Bitmap","Bar","Bar.bmp",buf,1024,iniFile);
	DD.lpDDTBar=CreateTexture(DD.lpD3DDevice,buf);
	GetPrivateProfileString("Bitmap","Velocity","Velocity.bmp",buf,1024,iniFile);
	DD.lpDDTVelocity=CreateTexture(DD.lpD3DDevice,buf);
	return TRUE;
}

void MainGame::Release()
{
	Out.Close();
	DD.Release();
	Midi.Release();
}

void MainGame::Loop()
{
	DWORD Time=timeGetTime(),t;
	Header.BPM=120;
	int lo=TRUE;
	Count=0;
	MidiCount=0;
	Msg=0;
	while(lo)
	{
		if(Count%3)
		{
			Time+=8;
			MidiCount+=8;
		}
		else
		{
			Time+=9;
			MidiCount+=9;
		}
		lo=Process();
		if(Time>=timeGetTime())
		{
			Draw();
			t=timeGetTime();
			if(Time>=t)
				Sleep(Time-t);
		}
		Count++;
	}
}

int MainGame::Process()
{
	Items=INFINITE;
	DD.lpDIDKey->GetDeviceState(256,KeyState);
	DD.lpDIDKey->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),NULL,&Items,DIGDD_PEEK);
	if(Items)
	{
		if(KeyBuffer)
		{
			delete[]KeyBuffer;
			KeyBuffer=NULL;
		}
		KeyBuffer=new DIDEVICEOBJECTDATA[Items];
		DD.lpDIDKey->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),KeyBuffer,&Items,NULL);
	}
	while((int)Midi.Frame[Msg].Count<=MidiCount)
	{
		if((DWORD)Msg==Midi.FrameNum)
			break;
#if _DEBUG
		char buf[256];
		sprintf(buf,"%6d:Tr%2d:%5d/%5d:",Midi.Frame[Msg].Count,Midi.Frame[Msg].Track,Midi.FrameNum,Msg);
		for(int i=0;(DWORD)i<Midi.Frame[Msg].DataSize;i++)
		{
			if(Midi.Frame[Msg].Data[i]&0xF0)
			{
				sprintf(buf+strlen(buf),"%X",Midi.Frame[Msg].Data[i]);
			}
			else
			{
				sprintf(buf+strlen(buf),"0%X",Midi.Frame[Msg].Data[i]);
			}
		}
		sprintf(buf+strlen(buf),"\r\n");
		OutputDebugString(buf);
#endif
		LPBYTE Data=Midi.Frame[Msg].Data;
		if(Data[0]==0xFF)
		{
			DWORD tt;
			LPBYTE temp;
			DWORD Gc;
			switch(Data[1])
			{
			case 0x03:
				if(Midi.Frame[Msg].Track==0)
				{
					temp=Data+1;
					Gc=GetCount(&temp);
					Header.title=new char[Gc+1];
					ZeroMemory(Header.title,Gc+1);
					tt=0;
					while(Data[tt+2]&0x80)
						tt++;
					memcpy(Header.title,Data+3+tt,Gc);
//					Header.title[GetCount(&temp)]=0;
					SetWindowText(hwnd,Header.title);
				}
				break;
			case 0x51:
				tt=(Data[3]<<16)|(Data[4]<<8)|(Data[5]);
				Header.BPM=60000000/tt;
				break;
			}
		}
		else
		{
			for(int i=0;Beat[i].SetData(Data);i++);
		}
		Msg++;
	}
	if(KeyState[DIK_ESCAPE]&0x80)
		return FALSE;
	for(int i=0;i<1024;i++)
		Beat[i].Process();
	SoundLevel.Process();
	return TRUE;
}

void MainGame::Draw()
{
	D3DTLVERTEX vr[256];
	ZeroMemory(vr,sizeof(D3DTLVERTEX)*256);
	for(int i=0;i<20;i++)
	{
		vr[i].sz=0;
		vr[i].rhw=1;
		vr[i].color=RGB_MAKE(255,255,255);
		vr[i].specular=RGB_MAKE(0,0,0);
	}
	vr[0].sx=16;
	vr[0].sy=316;
	vr[1].sx=240;
	vr[1].sy=316;
	vr[2].sx=16;
	vr[2].sy=324;
	vr[3].sx=240;
	vr[3].sy=324;
	for(i=0;i<8;i++)
	{
		vr[i*2+4].sx=(float)(16+i*32);
		vr[i*2+4].sy=(float)(16);
		vr[i*2+5].sx=(float)(16+i*32);
		vr[i*2+5].sy=(float)(324);
	}
	DD.lpDDSBack->Blt(NULL,DD.lpDDSWall,NULL,NULL,NULL);
	DD.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,FALSE);
	DD.lpD3DDevice->BeginScene();
	DD.lpD3DDevice->DrawPrimitive(D3DPT_LINELIST,D3DFVF_TLVERTEX,vr,20,NULL);
	int s=0;
	for(i=0;i<12;i++)
		if(KeyState[KeySetting[i]]&0x80)
		{
			vr[s*6].color=RGBA_MAKE(255,255,255,255);
			vr[s*6].specular=RGB_MAKE(0,0,0);
			vr[s*6].rhw=1;
			vr[s*6].sz=0;
			vr[s*6].sx=16+Lv[i];
			vr[s*6].sy=16;
			vr[s*6].tu=0.0390625*(Lv[i]);
			vr[s*6].tv=0;
			vr[s*6+1].color=RGBA_MAKE(255,255,255,255);
			vr[s*6+1].specular=RGB_MAKE(0,0,0);
			vr[s*6+1].rhw=1;
			vr[s*6+1].sz=0;
			vr[s*6+1].sx=16+Lv[i]+32;
			vr[s*6+1].sy=16;
			vr[s*6+1].tu=0.0390625*(Lv[i]+32);
			vr[s*6+1].tv=0;
			vr[s*6+2].color=RGBA_MAKE(255,255,255,255);
			vr[s*6+2].specular=RGB_MAKE(0,0,0);
			vr[s*6+2].rhw=1;
			vr[s*6+2].sz=0;
			vr[s*6+2].sx=16+Lv[i];
			vr[s*6+2].sy=324;
			vr[s*6+2].tu=0.0390625*(Lv[i]);
			vr[s*6+2].tv=1;
			vr[s*6+3]=vr[s*6+2];
			vr[s*6+4]=vr[s*6+1];
			vr[s*6+5].color=RGBA_MAKE(255,255,255,255);
			vr[s*6+5].specular=RGB_MAKE(0,0,0);
			vr[s*6+5].rhw=1;
			vr[s*6+5].sz=0;
			vr[s*6+5].sx=16+Lv[i]+32;
			vr[s*6+5].sy=324;
			vr[s*6+5].tu=0.0390625*(Lv[i]+32);
			vr[s*6+5].tv=1;
			s++;
		}
	DD.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
	SoundLevel.Draw();
	DD.lpD3DDevice->SetTexture(0,DD.lpDDTBar);
	DD.lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vr,s*6,NULL);
	DD.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,FALSE);
	DD.lpD3DDevice->SetTexture(0,DD.lpDDTBeat);
	for(i=0;i<1024;i++)
		Beat[i].Draw();
	DD.lpD3DDevice->SetTexture(0,NULL);
	DD.lpD3DDevice->EndScene();
	char buf[1024];
	sprintf(buf,"%5d/%5d Tr:%3d BPM:%3d Time:%12f",Midi.FrameNum,Msg,Midi.Frame[Msg].Track,Header.BPM,MidiCount/1000);
	DD.TextOut(0,0,buf);
	DD.Flip();
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SoundLevel_c::SoundLevel_c()
{
	for(int i=0;i<16;i++)
		Level[i]=0;
}

void SoundLevel_c::set(LPBYTE Data)
{
	if((Data[0]&0xF0)==0x90&&Data[1]>Level[Data[0]&0x0F])
	{
		Level[Data[0]&0x0F]=Data[1];
	}
}

void SoundLevel_c::Process()
{
	for(int i=0;i<16;i++)
	{
		if(Level[i])
			Level[i]--;
	}
}

void SoundLevel_c::Draw()
{
	int x=256,y=368;
	D3DTLVERTEX vr[96];
	for(int i=0;i<16;i++)
	{
		vr[i*6].color=RGBA_MAKE(255,255,255,255);
		vr[i*6].specular=RGB_MAKE(0,0,0);
		vr[i*6].rhw=1;
		vr[i*6].sz=0;
		vr[i*6].sx=x+i*16;
		vr[i*6].sy=y-Level[i];
		vr[i*6].tu=i*0.0625f;
		vr[i*6].tv=1-0.0078125f*Level[i];
		vr[i*6+1].color=RGBA_MAKE(255,255,255,255);
		vr[i*6+1].specular=RGB_MAKE(0,0,0);
		vr[i*6+1].rhw=1;
		vr[i*6+1].sz=0;
		vr[i*6+1].sx=x+i*16+16;
		vr[i*6+1].sy=y-Level[i];
		vr[i*6+1].tu=i*0.0625f+0.0625f;
		vr[i*6+1].tv=1-0.0078125f*Level[i];
		vr[i*6+2].color=RGBA_MAKE(255,255,255,255);
		vr[i*6+2].specular=RGB_MAKE(0,0,0);
		vr[i*6+2].rhw=1;
		vr[i*6+2].sz=0;
		vr[i*6+2].sx=x+i*16;
		vr[i*6+2].sy=y;
		vr[i*6+2].tu=i*0.0625f;
		vr[i*6+2].tv=1;
		vr[i*6+3]=vr[i*6+2];
		vr[i*6+4]=vr[i*6+1];
		vr[i*6+5].color=RGBA_MAKE(255,255,255,255);
		vr[i*6+5].specular=RGB_MAKE(0,0,0);
		vr[i*6+5].rhw=1;
		vr[i*6+5].sz=0;
		vr[i*6+5].sx=x+i*16+16;
		vr[i*6+5].sy=y;
		vr[i*6+5].tu=i*0.0625f+0.0625f;
		vr[i*6+5].tv=1;
	}
	DD.lpD3DDevice->SetTexture(0,DD.lpDDTVelocity);
	DD.lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vr,96,NULL);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Beat_c::Beat_c()
{
	Pos=0;
	Data=NULL;
	Flags=0;
	State=FALSE;
}

BOOL Beat_c::SetData(LPBYTE dd)
{
	if(State)
		return TRUE;
	Data=dd;
	Pos=-8;
	State=TRUE;
	Flags=0;
	Level=Data[1]%Diff;
	Alpha=255;
	Size=0;
	return FALSE;
}

void Beat_c::Process()
{
	if(State)
	{
		BOOL KeyDown=FALSE;
		if(Data[0]==Part&&Data[1])
		{
			for(int i=0;i<(int)Items;i++)
			{
				if(KeyBuffer[i].dwOfs==KeySetting[Data[1]%Diff])
				{
					if(KeyBuffer[i].dwData&0x80)
					{
						KeyDown=TRUE;
						break;
					}
				}
			}
		}
		if(Pos<300&&Pos>284&&Flags==0&&KeyDown)
		{
			SoundLevel.set(Data);
			Out.SendMsg(Data);
			Flags=1;
		}
		if(Pos>300&&Flags==0&&!(Data[0]==Part&&Data[1]))
		{
			SoundLevel.set(Data);
			Out.SendMsg(Data);
			Flags=1;
		}
		if(Pos<300&&Pos>284&&Flags)
		{
			Alpha-=8;
			Size++;
			if(Alpha<0)
			{
				State=FALSE;
			}
		}
		else
		{
			Pos+=Header.BPM/120.0f;
			if(Pos>=300)
				Alpha=255-(Pos-300)*8;
			if(Pos>=332)
				State=FALSE;
		}
	}
}

void Beat_c::Draw()
{
	if(!State)
		return;
	if(Data[0]!=Part||!Data[1])
		return;
	float x,y;
	x=0.125f*(Level%8);
	y=0.5*(Level/8);
	D3DTLVERTEX vr[4];
	vr[0].sx=Lv[Level]+16-Size/3;
	vr[0].sy=Pos+16-Size/6;
	vr[0].sz=0;
	vr[0].tu=x;
	vr[0].tv=y;
	vr[0].rhw=1;
	if(Alpha!=255)
	{
		vr[0].color=RGBA_MAKE(Alpha,Alpha,Alpha,255-Alpha);
	}
	else
		vr[0].color=RGB_MAKE(255,255,255);
	vr[0].specular=RGB_MAKE(0,0,0);
	vr[1].sx=Lv[Level]+32+16+Size/3;
	vr[1].sy=Pos+16-Size/6;
	vr[1].sz=0;
	vr[1].tu=x+0.125;
	vr[1].tv=y;
	vr[1].rhw=1;
	if(Alpha!=255)
	{
		vr[1].color=RGBA_MAKE(Alpha,Alpha,Alpha,255-Alpha);
	}
	else
		vr[1].color=RGB_MAKE(255,255,255);
	vr[1].specular=RGB_MAKE(0,0,0);
	vr[2].sx=Lv[Level]+16-Size/3;
	vr[2].sy=Pos+16+8+Size/6;
	vr[2].sz=0;
	vr[2].tu=x;
	vr[2].tv=y+0.5;
	vr[2].rhw=1;
	if(Alpha!=255)
	{
		vr[2].color=RGBA_MAKE(Alpha,Alpha,Alpha,255-Alpha);
	}
	else
		vr[2].color=RGB_MAKE(255,255,255);
	vr[2].specular=RGB_MAKE(0,0,0);
	vr[3].sx=Lv[Level]+32+16+Size/3;
	vr[3].sy=Pos+16+8+Size/6;
	vr[3].sz=0;
	vr[3].tu=x+0.125;
	vr[3].tv=y+0.5;
	vr[3].rhw=1;
	if(Alpha!=255)
	{
		vr[3].color=RGBA_MAKE(Alpha,Alpha,Alpha,255-Alpha);
	}
	else
		vr[3].color=RGB_MAKE(255,255,255);
	vr[3].specular=RGB_MAKE(0,0,0);
	if(Alpha!=255)
	{
		DD.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
	}
	else
	{
		DD.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,FALSE);
	}
	DD.lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DFVF_TLVERTEX,vr,4,NULL);
}