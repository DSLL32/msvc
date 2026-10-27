/**********************************************************************************

			Copyright Creative Technology Ltd, 1993
				All rights reserved

NAME:	Nam Hoang
PROGRAM:	VAGAME.C
************************************************************************************/

#include <windows.h>
#include <windowsx.h>

#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <mmsystem.h>
#include "vagame.h"
#include <vasapi.h>



LONG WINAPI _export WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
extern char  RunMathTest(HWND hwnd,int);
extern int GenerateNumber(void);
extern char  AnswerValue(int lNum1,int lNum2, int lNum3,HWND hwnd);
extern void MathIsAnswerRight(HWND hwnd, LONG lParam,
			 UINT wParam, char szAnswer);
extern void myPlayVoice(BOOL, char *, HWND);

extern void RunMemoryTest(HWND hwnd);
extern void ShowMemNumbers(HWND hwnd,int);
extern  void MemIsAnswerRight(HWND hwnd,LONG lParam,
						UINT wParam);
extern void RunCapitalTest(HWND hwnd,int);
extern void CapIsAnswerRight(HWND hwnd,LONG lParam,UINT wParam);

extern void CleanItems(HWND hwnd);
static void myRegisterAPIWndMess(HWND hwnd);
static void myBecomingActive(HWND hwnd,LPARAM);


WORD	msgActionStart, msgActionStop, msgSpeechStart, msgSpeechStop;
WORD	msgLinkTerminated, msgMeter, msgAppSwitch;

extern char szLoseWin[60];
LPSRCONTEXT	lpSRContext;
LPSRUSERLIST	lpSRUserList;
SRWORD	sSRWord;
int nWhichGame=-1 ;
BOOL gbAnother= FALSE;
HANDLE hInst;
ATOM gaUserAtomID = 1;


/*******************************************************************************
	myRegisterAPIWndMess():
	register VAAPI messages and allocate VA two structures: SRUSERLIST and SRCONTEXT
********************************************************************************/
static void myRegisterAPIWndMess(HWND hwnd)
{
	  msgActionStart	= RegisterWindowMessage( "ACTION_START");
	  msgActionStop	= RegisterWindowMessage( "ACTION_STOP");
	  msgSpeechStart	= RegisterWindowMessage( "SPEECH_START");
	  msgSpeechStop	= RegisterWindowMessage( "SPEECH_STOP");
	  msgMeter			= RegisterWindowMessage( "METER");
	  msgAppSwitch		= RegisterWindowMessage( "APP_SWITCH");
	  msgLinkTerminated = RegisterWindowMessage( "LINK_TERMINATED");

	  lpSRUserList= (LPSRUSERLIST) GlobalAllocPtr( GHND, sizeof( SRUSERLIST));
	  lpSRContext = ( LPSRCONTEXT) GlobalAllocPtr( GHND, sizeof( SRCONTEXT));

}

/* *************************************************************
myBecomingActive: is called any time when application is  becoming active.
		Fills up the struct lpSRContext and restores the previous status of commands, and
		enables  VAAPI messages notification and disables VA buttons. It also checks current
      user.
******************************************************************************************/
static void myBecomingActive(HWND hwnd,LPARAM lParam)
{
	
	int nError,i;
	static BOOL bFirstTime = TRUE;
   char szError[60];
	SRNotify( hwnd, 1);  			 // enable all messages
	SREnableButtons(hwnd,FALSE);	// disable "delete","add","rename","action" in VA

	nError = SROnOff(hwnd,0);
	if(gaUserAtomID != LOWORD(lParam))  // check current user
	{
		gaUserAtomID = LOWORD(lParam);
		if(!gaUserAtomID)   // user file does not include vagame file
      {
			PostMessage(hwnd,WM_CLOSE,0,0L);
         return ;
      }
      bFirstTime = TRUE;
   }

	if(bFirstTime)
	{

		if(SRRequest(hwnd,lpSRContext)==VADLGOPEN)  // user or training dlg is opened
			return;

		bFirstTime = FALSE;

		if(nWhichGame != -1) 		// change user, re-initialize.
		{
			nWhichGame = -1;
         gbAnother = FALSE; 
			SetDlgItemText(hwnd,ID_CORRECT,NULL);

			for (i = 0; i<3; i++)  // uncheck the current selected radiobutton
			{
				if( Button_GetCheck(GetDlgItem(hwnd,ID_MATH+i)) )  // which one is selected?
				{
					Button_SetCheck(GetDlgItem(hwnd,ID_MATH+i),0);
               break;
            }
			}
			for(i=0;i<10;i++)  // hide the previous game
				ShowWindow(GetDlgItem(hwnd,ID_MEMWIN+i),SW_HIDE);

      }
		SetDlgItemText(hwnd,ID_STATIC,
			  "Say \"Math\", or \"Memory\", \"State Capitals\" to play.");
		for(i= 1; i<lpSRContext->wNumCommands;i++)
		{
      		
		  	if(i== GOTOSLEEP || i == WAKEUP || (i>=GAMESTART && i<= GAMEEND) )
			  	lpSRContext->sCommInfo[i].wData = 0;
		  	else
			  	lpSRContext->sCommInfo[i].wData= 1;
		}
		SREnableCommand( hwnd, lpSRContext);
		if(!nError)
		  	SROnOff(hwnd,1);
		return;
	}

	SRActionOnOff(hwnd,lpSRContext->wAction);

	SREnableCommand( hwnd, lpSRContext);  		// restore prev status

	if(!nError) 					//  engine was not turned off when appl. gain focus
   	SROnOff(hwnd,1);
   	
}

/*****************************************************************************************
 myActionStart:
	Processes the recognized command. Sends message to 
	controls in WndProc  to play or to answer the question
*******************************************************************************************/
void myActionStart(HWND hwnd,LONG lParam)
{

	int nIndex,i;

	nIndex = LOWORD(lParam);

	if(nIndex == TEST)  			// for memory game only, to display memory question
		PostMessage(hwnd,WM_COMMAND,ID_TEST,0L);

	else if (nIndex == ANSWER) // close the game
		PostMessage(hwnd,WM_ANSWERTEST,0,MAKELONG(1,0)); // want to know the answer

	else if(nIndex == ANOTHER) 	// play another game
		PostMessage(hwnd,WM_COMMAND,nWhichGame-1+ID_MATH,MAKELONG(1,1));

	else if (nIndex >= GAMESTART && nIndex <= GAMEEND) // which game
	{
		nWhichGame = nIndex-GAMESTART+1;
		
		PostMessage(hwnd,WM_COMMAND,nWhichGame-1+ID_MATH,MAKELONG(0,1));
	}

	else if (nIndex >= MATHSTART && nIndex <= MATHEND) // math answer 
		SendMessage(hwnd,WM_ANSWERTEST,nIndex,0L);

	else if( nIndex >= MEMORYSTART && nIndex <= MEMORYEND) // memory answer
		SendMessage(hwnd,WM_ANSWERTEST,nIndex,0L);

	else if( nIndex >= CAPITALSTART && nIndex <= CAPITALEND) // capital answer
		SendMessage(hwnd,WM_ANSWERTEST,nIndex,0L);

}


int PASCAL WinMain (HANDLE hInstance, HANDLE hPrevInstance,
					  LPSTR lpszCmdLine, int nCmdShow)
{

	HWND		hwnd;
	MSG		msg;
	WNDCLASS	wndclass;

	if (!hPrevInstance)
	{
		wndclass.style         = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc   = WndProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = DLGWINDOWEXTRA; // Because of CreateDialog
		wndclass.hInstance     = hInstance;
		wndclass.hIcon         = LoadIcon( hInstance, "GAME");
		wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW);
		wndclass.hbrBackground = GetStockObject( WHITE_BRUSH);
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = "GAME";

		RegisterClass( &wndclass);
	}

   hInst = hInstance;
	hwnd = CreateDialog( hInst, "GAME", 0, NULL);

	ShowWindow( hwnd, nCmdShow);
	UpdateWindow( hwnd);

	while (GetMessage( &msg, NULL, 0, 0))
	{
		TranslateMessage( &msg);
		DispatchMessage( &msg);
	}

	return msg.wParam;
}

LONG WINAPI _export WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	

	static BOOL bLink = FALSE;
  	static char szAnswer;
	int i;
	char szNum[5];

	switch( message)
	{
		case WM_CREATE:
			bLink = FALSE;
			SetFocus(hwnd);
			myRegisterAPIWndMess(hwnd);
			return 0;

		case WM_ACTIVATEAPP:
			if(!bLink && wParam)
			{
				bLink =  !SRInitializeLink(hwnd);
			}
			return 0;

		case WM_ANSWERTEST:
			switch(nWhichGame)
			{
				case MATH:
					MathIsAnswerRight(hwnd,lParam,wParam,szAnswer);
					break;
				case MEMORY:
					MemIsAnswerRight(hwnd,lParam,wParam);
					break;
				case CAPITAL:
					CapIsAnswerRight(hwnd,lParam,wParam);
					break;
			}

			return 0;
		case MM_MCINOTIFY:
			return 0;
	  	case WM_KEYDOWN:
			if(wParam== VK_UP || wParam == VK_DOWN) // process up/dow arrow keys 
         {
			 	for(i=0;i<3;i++)
				{
					if( Button_GetCheck(GetDlgItem(hwnd,ID_MATH+i)) )  // which one is selected?
					{
						if(wParam==VK_DOWN)
						{
							((ID_MATH+i+1)>ID_CAPITAL)?
								SendMessage(hwnd,WM_COMMAND,ID_MATH,0L):
								SendMessage(hwnd,WM_COMMAND,ID_MATH+i+1,0L);
						}
						else  // up arroW
						{
							((ID_MATH+i-1)< ID_MATH)?
								SendMessage(hwnd,WM_COMMAND,ID_CAPITAL,0L):
								SendMessage(hwnd,WM_COMMAND,ID_MATH+i-1,0L);
						}
						return 0;
               } // end of if
				}// end of for
			}
			return 0;
	  
		case WM_COMMAND:
			switch(wParam)
			{
				case ID_CAPITAL:
				  RunCapitalTest(hwnd,LOWORD(lParam));
				  SetFocus(hwnd);
				  return 0;

				case ID_MATH:
					szAnswer = RunMathTest(hwnd,LOWORD(lParam));
               SetFocus(hwnd);
					return 0;

				case ID_MEM:
					ShowMemNumbers(hwnd,LOWORD(lParam));
				  	SetFocus(hwnd);
					return 0;

				case ID_TEST:
            	if(nWhichGame == MEMORY)
							RunMemoryTest(hwnd);
					SetFocus(hwnd);
					return 0;
			}
			break;

		case WM_CLOSE:
			if(!gaUserAtomID)
				MessageBox(hwnd,"This user file not include VAGame file. Terminating VAGame.",
								 "Error", MB_ICONSTOP|MB_OK);
			SRTerminateLink( hwnd);
			break;

		case WM_DESTROY:
      	myPlayVoice(FALSE,0,hwnd);
			GlobalFreePtr( lpSRUserList);
			GlobalFreePtr( lpSRContext);
			PostQuitMessage (0);
			return 0;

		default:
			if(message == msgAppSwitch)
			{
				if(wParam)  // becoming active
					myBecomingActive(hwnd,lParam);
				else    // becoming inactive
					SRRequest(hwnd,lpSRContext);
            return 0;
			}
		  	if(message == msgActionStart)
			{
				myActionStart(hwnd,lParam); // process recognized command
				return 0;
			}
			if(message == msgLinkTerminated)
			{
				bLink = FALSE;
				return 0;
			}

			break;


	} // END OF SWITCH MESSAGE

	return DefWindowProc (hwnd, message, wParam, lParam);
}

