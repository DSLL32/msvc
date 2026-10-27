/******************************************************************************
			Copyright Creative Technology Ltd, 1993
				All rights reserved
Module:	file.c
Programmer:	Nam Hoang
*****************************************************************************/

/* when you use Microsoft Compiler, change unlink() to be _unlink() */
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <string.h>
#include <stdio.h>
#include <dos.h>

#include "vamemo.h"
#include <vasapi.h>


extern struct MEMO Memo[MAXMEMO];

void GetVoiceFileName(int);
BOOL CALLBACK _export RecordDlgProc(HWND hDlg,WORD message,WORD wParam,LONG lParam);
BOOL CreateUniqueFileName(LPSTR lpPath,LPSTR lpPrefix,LPSTR lpFileName);
void ConstructPath(void);
void DeleteAMemo(HWND hwnd,int nDeleteIndex);
void myPlayVoice(BOOL,HWND,char *);
void myDeleteUserMemWaveFile(LPSTR szUserName,HWND hwnd);

extern BOOL myMemoFileExist(LPSTR ,HWND hwnd);
int DeleteMemFiles(HWND);

extern SRWORD	sSRWord;
extern HANDLE hInst;
extern int gnNumMemos;

char gszCurPath[80];  // path of program
int FillMemoStruct(LPSTR);
int AddMemoFile(HWND hWnd,int nNumrecs);
int gnNumberMemo;

/********************************************************************************
	DeleteMemoFiles: will delete Mem  and wave Files which don't have their names
	in lpSRContext table
*******************************************************************************/
int DeleteMemFiles(HWND hwnd)
{
	struct find_t ffblk;
	int done;
   
   done = _dos_findfirst("*.mem",_A_NORMAL,&ffblk);
   while (!done) 
	{

		if(!myMemoFileExist((LPSTR)ffblk.name,hwnd)) // last user is delete
		{
			myDeleteUserMemWaveFile((LPSTR)ffblk.name,hwnd);
   	}

		done = _dos_findnext(&ffblk);
   }

   return 0;
}

/******************************************************************
	myDeleteUserMemWaveFile: delete szUserName file and its wave files
*********************************************************************/
void myDeleteUserMemWaveFile(LPSTR szUserName,HWND hwnd)
{
	HFILE hInfile;
	int i=0;
	struct MEMO MemoTemp;

	hInfile=_lopen((LPSTR)szUserName,READ);

	if(hInfile==HFILE_ERROR)
   {
		gnNumMemos = 0;
		return ;
   }
	else
   {
		while(1)
		{
			if(_lread(hInfile,&MemoTemp,sizeof(Memo[0])) != sizeof(Memo[0]))
				break;
			memcpy(&Memo[i],&MemoTemp,sizeof(Memo[0]));
      	i++;
		}
   	_lclose(hInfile);
		gnNumMemos = i;
   }
	unlink(szUserName);  				// remove mem file

  	for(i=0; i<gnNumMemos;i++)
		unlink(Memo[i].szVoiceFile);  // remove all wave files

  	gnNumMemos = 0;
}

/**************************************************************
AddMemoFile: It update ___.mem file after recording a memo
*******************************************************************/
int AddMemoFile(HWND hWnd,int nNumrecs)
{
	char cBuf[128],szFileName[40];
	int i;
   HFILE hOutFile;


	wsprintf((LPSTR)szFileName,"%s.mem",(LPSTR)sSRWord.szUserName);
	hOutFile=_lopen((LPSTR)szFileName,WRITE);
	if(hOutFile==-1)
	{
		wsprintf((LPSTR)cBuf,"Can't open %s",(LPSTR)szFileName);
		return 0;
	}
	_llseek(hOutFile,0,0);  // go to end of file
	_lwrite(hOutFile,Memo,(sizeof(Memo[0]))*nNumrecs);
	_lclose(hOutFile);
	return 1;
}


/***********************************************************************************
 FillMemoStruct: fill up struct Memo for first time this szUserName application runs
************************************************************************************/
int FillMemoStruct(LPSTR szUserName)
{
	char szFileName[40];
	HFILE hInfile;
	int i=0;
	struct MEMO MemoTemp;

	wsprintf((LPSTR)szFileName,"%s.mem",szUserName);
	hInfile=_lopen((LPSTR)szFileName,READ);

	if(hInfile==HFILE_ERROR)
	{
		hInfile=_lcreat(szFileName,0);
		_lclose(hInfile);
		return 0;
	}

	while(1)
	{
		if(_lread(hInfile,&MemoTemp,sizeof(Memo[0])) != sizeof(Memo[0]))
			break;
		memcpy(&Memo[i],&MemoTemp,sizeof(Memo[0]));
      i++;
	}
   _lclose(hInfile);
	return i;

}

void ConstructPath(void)
{
	char *ptr;

	GetModuleFileName(hInst,gszCurPath,80);
	ptr=gszCurPath+lstrlen(gszCurPath);
	while(*ptr !='\\') ptr--;
	*ptr=0;
}

/*  ----------------------------------------------------------------------------
CreateUniqueFileName() - creates unique names for voice annotations and OLE documents

	Creates a unique filename in lpPath based on lpPrefix.  The Name is stored in
	lpFileName. A maximum of 100 unique names can be created.

	Example:

		if lpPath = c:\sched5 && lpPrefix = "wave"

		and "wave0.xxx" exists,

		lpFilename will == "c:\sched5\wave1.xxx"
    ----------------------------------------------------------------------------
*/      

BOOL CreateUniqueFileName(LPSTR lpPath,LPSTR lpPrefix,LPSTR lpFileName)
{
	int i;
   HFILE hInfile;

	for(i=0;i<100;i++)
	{
		lpFileName[0]=0;
		wsprintf(lpFileName,"%s\\%s%d.xxx",lpPath,lpPrefix,i);

	   hInfile=_lopen(lpFileName,READ);
		if(hInfile==HFILE_ERROR)
			return TRUE;
		_lclose(hInfile);
	}
   return FALSE;
}

/* ----------------------------------------------------------------------
	GetVoiceFilename()
		Creates a Unique voice file name and places it Memo[nIndex].szVoiceFile.
		Uses global gszCurPath
*/
void GetVoiceFileName(int nIndex)
{
	
   Memo[nIndex].szVoiceFile[0] = 0;
	CreateUniqueFileName((LPSTR)gszCurPath,(LPSTR)"wave",(LPSTR)&Memo[nIndex].szVoiceFile);
}

/***********************************************************************
DeleteAMemo: Deletle a memo
***********************************************************************/

void DeleteAMemo(HWND hwnd,int nDeleteIndex)
{
	int i;
	char szFileName[32];
	HFILE hInfile;

	unlink(Memo[nDeleteIndex].szVoiceFile);

	for (i= nDeleteIndex;i<gnNumMemos-1;i++)
	{
		lstrcpy((LPSTR)Memo[i].szVoiceFile,(LPSTR)Memo[i+1].szVoiceFile);
	}

	gnNumMemos--;

	if(gnNumMemos<0) // there is nothing to delete
		EnableWindow(GetDlgItem(hwnd,IDD_DELETE),FALSE);

	wsprintf((LPSTR)szFileName,"%s.mem",(LPSTR)sSRWord.szUserName);

	unlink(szFileName);

	hInfile=_lcreat((LPSTR)szFileName,0);

   _lclose(hInfile);


}

void myPlayVoice(BOOL nFlag,HWND hwnd, char *szFileName)
{
	
	char szVoiceBuf[256],szRetString[80];
	DWORD dwRes;

	if(nFlag) // PLay the voice
	{
		mciSendString ((LPSTR) "close goober",NULL,0,NULL);

		wsprintf((LPSTR)szVoiceBuf,"open %s type waveaudio alias goober",(LPSTR)szFileName);
		dwRes = mciSendString ((LPSTR)szVoiceBuf, szRetString, 80, NULL);
		if(dwRes)
		{
			SROnOff(hwnd,1);
			return ;
		}

		dwRes = 	mciSendString ((LPSTR)"play goober from 0 notify ", szRetString, 80, hwnd);
		if(dwRes)
			return ;

	}
	else //close mci
	{
	  	mciSendString ((LPSTR)"stop goober", NULL, 0, NULL);
	 	mciSendString ((LPSTR) "close goober",NULL,0,NULL);
	}

}