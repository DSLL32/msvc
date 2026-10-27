/**************************************************
			Copyright Creative Technology Ltd, 1993
				All rights reserved
Module:	memory.c
Programmer:	Nam Hoang

*****************************************************/
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <mmsystem.h>
#include "vagame.h"
#include <vasapi.h>

void MemIsAnswerRight(HWND hwnd,LONG lParam,
 						UINT wParam);

void RunMemoryTest(HWND hwnd);
void ShowMemNumbers(HWND hwnd,int);
extern myPlayVoice(BOOL,char *,HWND);

extern LPSRCONTEXT	lpSRContext;
extern LPSRUSERLIST	lpSRUserList;
extern int nWhichGame;

int gnMemIndex;
char gszMemAnswer[8];   // answer item

/****************************************************************
	ShowMemNumbers: displays the prices  and picks randomly one
   price for testing
****************************************************************/
void ShowMemNumbers(HWND hwnd,int nAnother)
{
	
	int i,j,number,nError;
	static BOOL bFocus = TRUE;
   int szPrice[8];
	char szNum[8];
	char szShowPrice[100]= {"  Price:\n"};

	if(nAnother==1) 			// to play another game
		bFocus = FALSE;

	nWhichGame = MEMORY;

	ShowWindow(GetDlgItem(hwnd,ID_MEMQUES),SW_HIDE);  

	if(bFocus)				// this is new game 
	{
		SetWindowText(hwnd,"Memory Game");
		for(i=4;i<10;i++)// hide all controls
			ShowWindow(GetDlgItem(hwnd,ID_MEMWIN+i),SW_HIDE);

		for(i=0;i<3;i++) // show controls which relate to MEMORY game
				ShowWindow(GetDlgItem(hwnd,ID_MEMWIN+i),SW_SHOW);
		SetFocus(GetDlgItem(hwnd,ID_MEM));
   }
	bFocus = TRUE;

	/* display the prices */
	srand((unsigned)time(NULL));
   
	gnMemIndex = rand()%8;
	for(i=0;i<8;i++)
	{
		number = rand()%98+1;
		szPrice[i] = number;
		for(j=0;j<i;j++) 				// to see there is a duplicate pair of prices
		{
			if(szPrice[j] == szPrice[i]) 	
			{
				number = rand()%98+1;
           	szPrice[i] = number;
            j=0; 						// test again
         }
      }
		wsprintf((LPSTR)szNum,"$%d.00\n",number);
      lstrcat(szShowPrice,szNum);
		if(gnMemIndex == i)
			wsprintf(gszMemAnswer,"$%d.00",number);
	}
	SetDlgItemText(hwnd, ID_MEMNUM1,szShowPrice);   	// display all prices

   /* enable and disnable commands */

	for(i=TEST+1; i< lpSRContext->wNumCommands;i++)  // disable other game commands
		lpSRContext->sCommInfo[i].wData= 1;

	lpSRContext->sCommInfo[ANSWER].wData = 1; 		 // disable the "answer" command

	lpSRContext->sCommInfo[TEST].wData = 0;   		// enable the "test" comm.

	nError = SROnOff(hwnd,0);

	SREnableCommand(hwnd,lpSRContext);

	SetDlgItemText(hwnd,ID_CORRECT,NULL);
	SetDlgItemText(hwnd,ID_STATIC,
		" Memorize the prices. Say \"Test\" to start, \"another\" or choose another game.");

  	if(!nError)
  		SROnOff(hwnd,1);

}

/************************************************************************************
RunMemoryTest: after saying "Test", this function is called. It hides all
	the prices, enables memory game and "answer" commands, disables other game comms.
*************************************************************************************/
void RunMemoryTest(HWND hwnd)
{
	int i;
	char szShowPrice[100];

	SetDlgItemText(hwnd,ID_MEMNUM1,NULL);  		// hide the prices

	wsprintf((LPSTR)szShowPrice,"Which item costs %s?", (LPSTR)gszMemAnswer);

	ShowWindow(GetDlgItem(hwnd,ID_MEMQUES),SW_SHOW);  //hide the question

	SetDlgItemText(hwnd,ID_MEMQUES,szShowPrice);

	wsprintf((LPSTR)szShowPrice,
		"Say an item name, \"answer\", \"another\", or choose another game. .");

	SetDlgItemText(hwnd,ID_STATIC,szShowPrice);

	for(i=TEST;i<lpSRContext->wNumCommands;i++)
	{
		if(i>= MEMORYSTART && i<=MEMORYEND )
			lpSRContext->sCommInfo[i].wData= 0;  	// enable memory comm.
      else
			lpSRContext->sCommInfo[i].wData= 1; 	// disable other game comm.
   }
	lpSRContext->sCommInfo[ANSWER].wData= 0;    // enable "answer" command

	SROnOff(hwnd,0);

	SREnableCommand(hwnd,lpSRContext);

	SROnOff(hwnd,1);
}

/***********************************************************************************
 MemIsAnswerRight:Tests the answer or displays the answer.
	It will call function myPlayVoice to playback a wave file : win.wav or lose.wav
	for right or wrong answer respectively.

************************************************************************************/
void MemIsAnswerRight(HWND hwnd,LONG lParam,UINT wParam)
{
	int i;
	char szLoseWin[60];


	if(LOWORD(lParam) && !wParam) 	// user says "answer"
	{
		wsprintf((LPSTR)szLoseWin,"%s is correct.",
				lpSRContext->sCommInfo[gnMemIndex+MEMORYSTART].szCommName);
		SetDlgItemText(hwnd,ID_CORRECT,(LPSTR)szLoseWin);

		//SROnOff(hwnd,1); // turn engine on
	}

	else if(wParam==(gnMemIndex+MEMORYSTART))
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