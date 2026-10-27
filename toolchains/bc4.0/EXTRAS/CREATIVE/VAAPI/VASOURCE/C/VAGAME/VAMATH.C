/* *********************************************************************************
			Copyright Creative Technology Ltd, 1993
				All rights reserved
Module: vamath.c
Programmer:	Nam Hoang

*************************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <mmsystem.h>
#include "vagame.h"
#include <vasapi.h>


void myPlayVoice(BOOL nFlag,char *szFileName,HWND hwnd);
void MathIsAnswerRight(HWND hwnd, LONG lParam,
		 UINT wParam, char szAnswer);
char  RunMathTest(HWND hwnd,int);

extern LPSRCONTEXT	lpSRContext;
extern LPSRUSERLIST	lpSRUserList;
extern int nWhichGame;

/* ****************************************************************
myPlayVoice: plays back the digital wave file or close mci device
******************************************************************/
void myPlayVoice(BOOL nFlag,char *szFileName,HWND hwnd)
{
	
	char szVoiceBuf[256],szRetString[80];

	if(nFlag) // PLay the voice
	{
	  	mciSendString ((LPSTR) "close goober",NULL,0,NULL);
		wsprintf((LPSTR)szVoiceBuf,"open %s type waveaudio alias goober",(LPSTR)szFileName);
		mciSendString ((LPSTR)szVoiceBuf, szRetString, 80, NULL);
		mciSendString ((LPSTR)"play goober from 0  ", szRetString, 80, NULL);
  }
  else //close mci
  {
	  	mciSendString ((LPSTR) "close goober",NULL,0,NULL);

  }

}

/*******************************************************************************
 AnswerPostion: picks randomly a digit postion in a number for testing. This digit is
 replaced by an "?".
**************************************************************************************/
int AnswerPostion(int lNum)
{
	char szNum[5];
	int length;

   wsprintf(szNum,"%d",lNum);
	length = lstrlen(szNum);
	return(rand()%length);  // pick a position of a number for testing
}

/**********************************************************************
 AnswerValue: displays randomly three numbers and sets up an new
   answer value. One of them will contain a "?" which replaces the the answer value
***************************************************************************/
char  AnswerValue(int lNum1,int lNum2, int lNum3,HWND hwnd)
{
	int nWhichNum,nPostion;
   char szNum[8],szAnswer;
   
	nWhichNum = rand()%3;

	
	switch(nWhichNum)
	{
		case 0:
			nPostion = AnswerPostion(lNum1);
			wsprintf((LPSTR)szNum,"%d",lNum1);

			szAnswer = szNum[nPostion];
			szNum[nPostion] = '?';
			SetDlgItemText(hwnd,ID_OPERAND1,(LPSTR)szNum);
			wsprintf((LPSTR)szNum,"+%d",lNum2);
			SetDlgItemText(hwnd,ID_OPERAND2,(LPSTR)szNum);
			wsprintf(szNum,"%d",lNum3);
			SetDlgItemText(hwnd,ID_RESULT,szNum);

			break;
		case 1:
			nPostion = AnswerPostion(lNum2);
			wsprintf(szNum,"+%d",lNum2);
			szAnswer = szNum[nPostion+1];
			szNum[nPostion+1] = '?';
			SetDlgItemText(hwnd,ID_OPERAND2,(LPSTR)szNum);
			wsprintf(szNum,"%d",lNum1);
			SetDlgItemText(hwnd,ID_OPERAND1,(LPSTR)szNum);
			wsprintf(szNum,"%d",lNum3);
			SetDlgItemText(hwnd,ID_RESULT,szNum);
			break;

		case 2:
			nPostion = AnswerPostion(lNum3);
			wsprintf(szNum,"%d",lNum3);
			szAnswer = szNum[nPostion];
			szNum[nPostion] = '?';
			SetDlgItemText(hwnd,ID_RESULT,(LPSTR)szNum);
			wsprintf(szNum,"%d",lNum1);
			SetDlgItemText(hwnd,ID_OPERAND1,(LPSTR)szNum);
			wsprintf(szNum,"+%d",lNum2);
			SetDlgItemText(hwnd,ID_OPERAND2,(LPSTR)szNum);

			break;
	}
	return szAnswer;
}

/******************************************************************************
 IsAnswerRight: Tests the answer or displaya the correct answer.              *
	Calls function myPlayVoice to playback a wave file : win.wav or lose.wav   *
	if the answer is correct or incorrect, respectively.                       *
*******************************************************************************/
void MathIsAnswerRight(HWND hwnd, LONG lParam, UINT wParam, char szAnswer)
{
	int i;
	char szLoseWin[60];


	if(LOWORD(lParam) && !wParam) 	//  "answer" command
	{
		
		wsprintf((LPSTR)szLoseWin,"%c is correct.",
				szAnswer);

		SetDlgItemText(hwnd,ID_CORRECT,(LPSTR)szLoseWin);
	  //	SROnOff(hwnd,1); // turn engine on
	}
   else if(szAnswer == lpSRContext->sCommInfo[wParam].szCommName[0])
	{
		SetDlgItemText(hwnd,ID_CORRECT,(LPSTR)"Correct!");
		myPlayVoice(TRUE,"win.wav",hwnd);

   }
	else
	{
		SetDlgItemText(hwnd,ID_CORRECT,(LPSTR)"Incorrect!");
		myPlayVoice(TRUE,"lose.wav",hwnd);
	}

}

/***********************************************************************************
	RunMathTest:  enables all math commands, disables other game commands, shows math
   question.
*************************************************************************************/
char  RunMathTest(HWND hwnd,int nAnother)
{

	unsigned num1,num2,num3,i,nError;

	static BOOL bFocus = TRUE;
	char szAnswer;

	if(nAnother==1)  				// play another game
		bFocus = FALSE;

	nWhichGame = MATH;

	if(bFocus)  					// this is the new game
	{
	  	/*  show math controls and hide  other controls */
	  	SetWindowText(hwnd,"Math Game");
	  	for(i=0;i<10;i++)				// hide all  controls
		  	ShowWindow(GetDlgItem(hwnd,ID_MEMWIN+i),SW_HIDE);

	 	for(i=0;i<5;i++) 				// show math controls
		  	ShowWindow(GetDlgItem(hwnd,ID_MATHWIN+i),SW_SHOW);
		SetFocus(GetDlgItem(hwnd,ID_MATH));
	}
	bFocus = TRUE;

	/* pick three new numbers and display them */
	srand((unsigned)time(NULL));
	num1 = rand()%4000;
	num2 = rand()%4000;
	num3 = num1 + num2;

	szAnswer = AnswerValue(num1,num2,num3,hwnd);
	SetDlgItemText(hwnd,ID_CORRECT,NULL);
	SetDlgItemText(hwnd,ID_STATIC,
			" Say a number from 0 to 9, \"answer\", \"another\", or choose another game. ");

	/* disable and enalbe command */
	for(i=TEST; i< lpSRContext->wNumCommands;i++)
	{
	  	if(i>= MATHSTART && i<= MATHEND)
			lpSRContext->sCommInfo[i].wData= 0;  // enable math game comm.
	  	else
		  	lpSRContext->sCommInfo[i].wData= 1;
	}

	lpSRContext->sCommInfo[ANSWER].wData= 0; // enable "answer" comm.

	nError = SROnOff(hwnd,0);
	SREnableCommand(hwnd,lpSRContext);
	if(!nError)
	  SROnOff(hwnd,1);

	return szAnswer;
}
