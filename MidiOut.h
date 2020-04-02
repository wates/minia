#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

WORD ConvW(WORD wData);
DWORD ConvDW(DWORD dwData);
DWORD GetCount(LPBYTE* ppData);

class MidiOut
{
	HMIDIOUT Out;
public:
	void Open();
	void Close();
	DWORD SendMsg(LPBYTE Msg);
};

struct MidiHeader
{
	BYTE Check[4];//識別子
	DWORD Size;//メインヘッダサイズ
	WORD Format;//フォーマット
	WORD Tracks;//トラック数
	WORD TimeBase;//分解能
};

struct MidiTrack
{
	BYTE Check[4];
	DWORD Size;
	LPBYTE Data;
};

//おまけ
struct MidiFrame
{
	DWORD Count;
	BYTE Track;
	LPBYTE Data;
	DWORD DataSize;
};

struct TrackState
{
	DWORD Addres;
	double Tempo;
	double Count;
	double WaitCount;
	BOOL Loop;
	DWORD ms;
	LPBYTE PrevMsg;
};


class MidiFile
{
	MidiTrack *Track;
public:
	MidiHeader Header;
	MidiFrame *Frame;
	DWORD FrameNum;
	BOOL Open(char *FileName);
	void CreateFrame();
	void Release();
};
