/****************************************************************************
			Copyright Creative Technology Ltd, 1993
				All rights reserved
Module: Us.c
Programmer: Nam Hoang
****************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <mmsystem.h>
#include "vagame.h"
#include <vasapi.h>

#define MAXSTATE 51

void RunCapitalTest(HWND hwnd,int);
void CapIsAnswerRight(HWND hwnd,LONG lParam,UINT wParam);

extern myPlayVoice(BOOL,char *,HWND);
int gnCapitalAnswer;
extern LPSRCONTEXT	lpSRContext;
extern  LPSRUSERLIST	lpSRUserList;
extern int nWhichGame;

/***************************************************
RunCapitalTest: sets up a question and an answer
******************************************************/
void RunCapitalTest(HWND hwnd,int nAnother)
{
	int i,nError;
   static BOOL bFocus = TRUE;
	char szCapital[100];
	char szState[32];
   
	if(nAnother==1) 	// to play another game
   	bFocus = FALSE;
	nWhichGame = CAPITAL;

	
	if(bFocus)  // this is new game
	{
		SetWindowText(hwnd,"Capital State Game");

		for(i=0;i<10;i++)
			ShowWindow(GetDlgItem(hwnd,ID_MEMWIN+i),SW_HIDE);
		ShowWindow(GetDlgItem(hwnd,ID_CAPITALTEST),SW_SHOW);
		SetFocus(GetDlgItem(hwnd,ID_CAPITAL));
	}
	bFocus = TRUE;

	/* set up a new question and an new answer */
	srand( (unsigned)time(NULL));
	gnCapitalAnswer = rand()%MAXSTATE;
	LoadString(GetWindowWord(hwnd,GWW_HINSTANCE),
		IDD_STATES+gnCapitalAnswer,(LPSTR)szState,sizeof(szState));
	wsprintf((LPSTR)szCapital,"\n What is the capital of %s? ",
							  (LPSTR)szState);
	SetDlgItemText(hwnd,ID_CAPITALTEST,(LPSTR)szCapital);
	SetDlgItemText(hwnd,ID_CORRECT,NULL);
	SetDlgItemText(hwnd,ID_STATIC,
	(LPSTR)" Say the capital, \"answer\", \"another\", or choose another game.");

   /* disable and enable command */
	for(i=TEST; i< lpSRContext->wNumCommands;i++)
	{
		if( i>= CAPITALSTART && i<= CAPITALEND)
			lpSRContext->sCommInfo[i].wData= 0;   	// enable  S.C comm.
		else
			lpSRContext->sCommInfo[i].wData= 1;    // disable other game comm.
   }
	lpSRContext->sCommInfo[ANSWER].wData= 0;  	// enable "Answer" comm.

	nError = SROnOff(hwnd,0);

	SREnableCommand(hwnd,lpSRContext);

	if(!nError)
		SROnOff(hwnd,1);
}

/*******************************************************************************************
 CapIsAnswerRight: to test the answer. It will call function myPlayVoice to
	playback a voc file : win.wav or lose.wav for correct or incorrect answer respectively.

********************************************************************************************/
void CapIsAnswerRight(HWND hwnd,LONG lParam,UINT wParam)
{
	int i;
	char szLoseWin[60];

	if(LOWORD(lParam) && !wParam) // user says "answer"
	{
		wsprintf((LPSTR)szLoseWin,"%s is correct.",
						lpSRContext->sCommInfo[gnCapitalAnswer+CAPITALSTART].szCommName);
	  	SetDlgItemText(hwnd,ID_CORRECT,(LPSTR)szLoseWin);
	}
	else if(wParam==(gnCapitalAnswer+CAPITALSTART))
	{
		SetDlgItemText(hwnd,ID_CORRECT,"Correct!");
		myPlayVoice(TRUE,"win.wav",hwnd);

	}
	else
	{
		SetDlgItemText(hwnd,ID_CORRECT,"Incorrect!");
		myPlayVoice(TRUE,"lose.wav",hwnd);
	}

}
