#ifndef _MAIN_H_INC
#define _MAIN_H_INC

#ifndef _MT
#define _MT
#endif//_MT
/*
#ifndef INITGUID
#define INITGUID
#endif//INITGUID
*/
#include <process.h>
#include <windows.h>
#include <stdio.h>
#include "DDClass.h"
#include "MidiOut.h"

#pragma comment(lib,"libcmt.lib")
#pragma comment(lib,"ddraw.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"winmm.lib")

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgs, int nWinMode );
LRESULT APIENTRY MainWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
void MainThread(void *Param);

char *FileName;

struct Header_t
{
	int BPM;
	char *title;
}Header;

DirectDraw DD;
MidiOut Out;
LPDIDEVICEOBJECTDATA KeyBuffer=NULL;
BYTE KeyState[256];
DWORD Items;
DWORD Flag;
int Diff=1;
int Part;
float Lv[12]={0,16,32,48,64,96,112,128,144,160,176,192};
BYTE KeySetting[12]={44,31,45,32,46,47,34,48,35,49,36,50};
char CurrentDirectory[1024];

class MainGame
{
	MidiFile Midi;
	HWND hwnd;
	int Count;
	double MidiCount;
	int Msg;
public:
	BOOL Initialize();
	MainGame(HWND hw);
	void Loop();
	int Process();
	void Draw();
	void Release();
};

class SoundLevel_c
{
	BYTE Level[16];
public:
	SoundLevel_c();
	void set(LPBYTE Data);
	void Process();
	void Draw();
}SoundLevel;

class Beat_c
{
	BOOL State;
	LPBYTE Data;
	float Pos;
	int Level;
	DWORD Flags;
	int Alpha;
	int Size;
public:
	Beat_c();
	BOOL SetData(LPBYTE dd);
	void Process();
	void Draw();
}Beat[2048];

#endif//__MAIN_H_INC