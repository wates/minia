#include "MidiOut.h"

WORD ConvW(WORD wData)
{
	WORD wWork;
	((char*)&wWork)[0]=((char*)&wData)[1];
	((char*)&wWork)[1]=((char*)&wData)[0];
	return wWork;
}

DWORD ConvDW(DWORD dwData)
{
	DWORD dwWork;
	((char*)&dwWork)[0] = ((char*)&dwData)[3];
	((char*)&dwWork)[1] = ((char*)&dwData)[2];
	((char*)&dwWork)[2] = ((char*)&dwData)[1];
	((char*)&dwWork)[3] = ((char*)&dwData)[0];
	return dwWork;
}

DWORD GetCount(LPBYTE* ppData)
{
	BYTE bbData;
	DWORD dwData=0;
	do
	{
		bbData=**ppData;
		++*ppData;
		dwData=(dwData<<7)+(bbData&0x7f);
	}while(bbData&0x80);
	return dwData;
}

//MIDIOutクラス

void MidiOut::Open()
{
	midiOutOpen(&Out,MIDI_MAPPER,NULL,NULL,CALLBACK_NULL);
}

void MidiOut::Close()
{
	midiOutClose(Out);
}

DWORD MidiOut::SendMsg(LPBYTE Msg)
{
	switch(Msg[0]&0xF0)
	{
	case 0xF0:
	{
		LPBYTE temp;
		if(Msg[0]==0xF0)
		{
			DWORD Bf=1;
			temp=Msg+1;
			int Gc;
			while(Msg[Bf]&0x80)
				Bf++;
			Gc=GetCount(&temp);
			MIDIHDR Hdr;
			ZeroMemory(&Hdr,sizeof(Hdr));
			Hdr.lpData=new char[Gc+1];
			ZeroMemory(Hdr.lpData,Gc+1);
			memcpy(Hdr.lpData+1,Msg+Bf+1,Gc);
			Hdr.lpData[0]=(char)0xF7;
			Hdr.dwBufferLength=Gc+1;
			midiOutPrepareHeader(Out,&Hdr,sizeof(Hdr));
			midiOutLongMsg(Out,&Hdr,sizeof(Hdr));
			return Gc+Bf+1;
		}
		else if(Msg[0]==0xFF)
		{
			if(Msg[1]==0x2F)
				return 2;
			temp=Msg+2;
			return GetCount(&temp)+3;
		}
		break;
	}
	case 0x80:
	case 0x90:
	case 0xA0:
	case 0xB0:
	case 0xE0:
		midiOutShortMsg(Out,*((DWORD*)Msg));
		return 3;
		break;
	case 0xC0:
	case 0xD0:
		midiOutShortMsg(Out,*((DWORD*)Msg));
		return 2;
		break;
	default:
		midiOutShortMsg(Out,*((DWORD*)Msg));
		return 2;
		break;
	}
	return 0;
}


BOOL MidiFile::Open(char *FileName)
{
	HANDLE File;
	DWORD ReadSize;
	File=CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if(File==INVALID_HANDLE_VALUE)
		return FALSE;
	//ファイルヘッダの読み取り
	if(!ReadFile(File,&Header,14,&ReadSize,NULL))
		return FALSE;
	if(14!=ReadSize)
		return FALSE;
	if(*((DWORD*)(Header.Check))!=0x6468544D)
		return FALSE;
	Header.Size=ConvDW(Header.Size);
	Header.Format=ConvW(Header.Format);
	Header.TimeBase=ConvW(Header.TimeBase);
	Header.Tracks=ConvW(Header.Tracks);
	//トッラクデータの読み取り
	Track=new MidiTrack[Header.Tracks];
	for(int i=0;i<Header.Tracks;i++)
	{
		if(!ReadFile(File,Track[i].Check,4,&ReadSize,NULL))
			return FALSE;
		if(*((DWORD*)(Track[i].Check))!=0x6B72544D||ReadSize!=4)
			return FALSE;
		if(!ReadFile(File,&Track[i].Size,4,&ReadSize,NULL))
			return FALSE;
		Track[i].Size=ConvDW(Track[i].Size);
		Track[i].Data=new BYTE[Track[i].Size];
		if(!ReadFile(File,Track[i].Data,Track[i].Size,&ReadSize,NULL))
			return FALSE;
		if(ReadSize!=Track[i].Size)
			return FALSE;
	}
	CloseHandle(File);
	Frame=NULL;
	return TRUE;
}
void MidiFile::Release()
{
	for(int i=0;i<Header.Tracks;i++)
		delete[]Track[i].Data;
	delete[]Track;
	if(Frame)
	{
		for(DWORD i=0;i<FrameNum;i++)
			delete[]Frame[i].Data;
		delete[]Frame;
	}
}

void MidiFile::CreateFrame()
{
	DWORD DataNum=0;
	DWORD Dco;
	for(DWORD y=0;y<Header.Tracks;y++)
	{
		for(Dco=0;Dco<Track[y].Size;DataNum++)
		{
			while(Track[y].Data[Dco]&0x80)
			{
				Dco++;
			}
			Dco++;
			switch(Track[y].Data[Dco]&0xF0)
			{
			case 0x80:
			case 0x90:
			case 0xA0:
			case 0xB0:
			case 0xE0:
				Dco+=3;
				break;
			case 0xC0:
			case 0xD0:
				Dco+=2;
				break;
			case 0xF0:
				{
					LPBYTE temp;
					DWORD Ttemp,GC;
					if(Track[y].Data[Dco]==0xF0)
					{
						temp=Track[y].Data+Dco+1;
						GC=GetCount(&temp);
						Ttemp=0;
						while(Track[y].Data[Dco+1+Ttemp]&0x80)
							Ttemp++;
						Ttemp++;
						Dco+=GC+1+Ttemp;
					}
					if(Track[y].Data[Dco]==0xFF)
					{
						if(Track[y].Data[Dco+1]==0x2F)
						{
							DataNum--;
							Dco+=2;
							break;
						}
						temp=Track[y].Data+Dco+2;
						Ttemp=0;
						while(Track[y].Data[Dco+2+Ttemp]&0x80)
							Ttemp++;
						Ttemp++;
						GC=GetCount(&temp);
						Dco+=GC+2+Ttemp;
					}
				}
				break;
			default://謎
				Dco+=2;
				break;
			}
		}
	}
	Frame=new MidiFrame[DataNum];
	TrackState *ts=new TrackState[Header.Tracks];
	int t;
	LPBYTE Msg;
	LPBYTE temp;
	for(t=0;t<Header.Tracks;t++)
	{
		ts[t].Count=0;
		ts[t].Addres=0;
		ts[t].WaitCount=0;
		ts[t].Loop=TRUE;
		ts[t].ms=0;
	}
	float Tempo=120;
	for(y=0;y<DataNum-1;)
	{
		for(t=0;t<Header.Tracks;t++)
		{
			if(ts[t].Addres>=Track[t].Size)
				continue;
			ts[t].Count+=Tempo/60000*Header.TimeBase;
			if(Tempo)
				if(ts[t].WaitCount>=ts[t].Count)
				{
					ts[t].ms++;
					continue;
				}
			Frame[y].Track=t;
			temp=Track[t].Data+ts[t].Addres;
			if(y)
				Frame[y-1].Count=ts[t].ms;
			ts[t].WaitCount+=GetCount(&temp);
			while(Track[t].Data[ts[t].Addres]&0x80)
				ts[t].Addres++;
			ts[t].Addres++;
			Msg=Track[t].Data+ts[t].Addres;
			switch(Msg[0]&0xF0)
			{
			case 0xF0:
			{
				if(Msg[0]==0xF0)
				{
					DWORD Bf=1;
					temp=Msg+1;
					int Gc;
					while(Msg[Bf]&0x80)
						Bf++;
					Gc=GetCount(&temp);
					Frame[y].DataSize=Gc+Bf+1;
				}
				else if(Msg[0]==0xFF)
				{
					if(Msg[1]==0x2F)
					{
						Frame[y].DataSize=2;
					}
					if(Msg[1]==0x51)
					{
						DWORD tt;
						tt=(Msg[3]<<16)|(Msg[4]<<8)|(Msg[5]);
						Tempo=60000000/(float)tt;
					}
					temp=Msg+2;
					Frame[y].DataSize=GetCount(&temp)+3;
				}
				break;
			}
			case 0x80:
			case 0x90:
			case 0xA0:
			case 0xB0:
			case 0xE0:
				Frame[y].DataSize=3;
				ts[t].PrevMsg=Msg;
				break;
			case 0xC0:
			case 0xD0:
				Frame[y].DataSize=2;
				break;
			default:
				Frame[y].DataSize=2;
				break;
			}
			if(!(Msg[0]&0x80))
			{
				Frame[y].DataSize=3;
				Frame[y].Data=new BYTE[Frame[y].DataSize];
				BYTE scm[3];
				scm[0]=ts[t].PrevMsg[0];
				scm[1]=Msg[0];
				scm[2]=Msg[1];
				memcpy(Frame[y].Data,scm,Frame[y].DataSize);
				ts[t].Addres+=2;
			}
			else
			{
				Frame[y].Data=new BYTE[Frame[y].DataSize];
				memcpy(Frame[y].Data,Msg,Frame[y].DataSize);
				ts[t].Addres+=Frame[y].DataSize;
			}
			y++;
			if(!Tempo)
				t--;
		}
	}
	FrameNum=y;
}
