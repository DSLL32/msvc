/******************************************************************************
				Copyright Creative Technology Ltd, 1993
						All rights reserved

MODULE NAME	: Confidnc.c
PROGRAMMER	: Nam Hoang
******************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <memory.h>

#include <vasapi.h>

#define COMMAND 100
#define SCORE 	101
#define VADLGOPEN 4

long WINAPI _export WndProc (HWND hwnd, UINT message, UINT wParam,
										 LONG lParam);
static void myRegisterAPIWndMess(HWND hwnd);

WORD msgSpeechStart, msgSpeechStop;
WORD	msgLinkTerminated, msgMeter, msgAppSwitch;

LPSRCONTEXT	lpSRContext;
LPSRUSERLIST	lpSRUserList;
SRWORD	sSRWord;


static void myRegisterAPIWndMess(HWND hwnd)
{
	
   msgSpeechStop = RegisterWindowMessage("SPEECH_STOP");
	msgAppSwitch	= RegisterWindowMessage( "APP_SWITCH");
	msgLinkTerminated = RegisterWindowMessage( "LINK_TERMINATED");
	lpSRUserList= (LPSRUSERLIST) GlobalAllocPtr( GHND, sizeof( SRUSERLIST));
	lpSRContext = ( LPSRCONTEXT) GlobalAllocPtr( GHND, sizeof( SRCONTEXT));
}

static void myMsgSpeechStop(HWND hwnd,UINT wParam,LONG lParam)
{

	int i;
	char szCommand[80],szScore[80],szTemp[32];
	typedef struct
	{
		WORD wRecognizedWord;
      WORD wRecognizedScore;
	} SCONFIDENCE;

	SCONFIDENCE FAR* pConfidMem; 

	if( HIWORD(lParam))  // error in recognition
	{
		SetDlgItemText(hwnd,COMMAND,(LPSTR)"Command:\n( Not recognized )");
		SetDlgItemText(hwnd,SCORE,(LPSTR)"Score:\n -1 ");
		return;
   }
	pConfidMem = ( SCONFIDENCE FAR*)GlobalLock( wParam);  // global memory block for recognized word

	wsprintf((LPSTR)szCommand,"Command:\n");
	wsprintf((LPSTR)szScore,"Score:\n");
	for(i=0;i<5;i++)
	{

		// score of recognition
		if( pConfidMem[i].wRecognizedScore == -1)
			break;
		else
      {
			wsprintf((LPSTR)szTemp," %d\n",pConfidMem[i].wRecognizedScore);
			lstrcat(szScore,szTemp);
		}

		// command name of recognition
		wsprintf((LPSTR)szTemp,"%s\n",
			(LPSTR)lpSRContext->sCommInfo[pConfidMem[i].wRecognizedWord].szCommName);
		lstrcat(szCommand,szTemp);

   }
 	SetDlgItemText(hwnd,COMMAND,(LPSTR)szCommand);
	SetDlgItemText(hwnd,SCORE,(LPSTR)szScore);

	GlobalUnlock( wParam);

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
		wndclass.hIcon         = LoadIcon( hInstance, "CONFIDENCE");
		wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW);
		wndclass.hbrBackground = GetStockObject( WHITE_BRUSH);
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = "CONFIDENCE";

		RegisterClass( &wndclass);
	 }
	hwnd = CreateDialog( hInstance, "CONFIDENCE", 0, NULL);
	ShowWindow( hwnd, nCmdShow);
	UpdateWindow( hwnd);

	while (GetMessage( &msg, NULL, 0, 0))
	{
			TranslateMessage( &msg);
			DispatchMessage( &msg);
	}

	return msg.wParam;
}


long WINAPI _export WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	static BOOL	bLink=FALSE,bFirstTime = TRUE;

	switch( message)
	{

		case WM_CREATE:
			bLink = FALSE;
         SetFocus(hwnd);
			myRegisterAPIWndMess(hwnd);
			return 0;

		case WM_ACTIVATEAPP:
			if(!bLink && wParam)
				bLink =  !SRInitializeLink(hwnd);
			return 0;


		case WM_CLOSE:
			SRTerminateLink(hwnd);
			break;

		case WM_DESTROY:
			GlobalFreePtr( lpSRUserList);
			GlobalFreePtr( lpSRContext);
			PostQuitMessage (0);
			return 0;

		default:
			if( message == msgAppSwitch)
			{

				if( wParam) 			// becoming active
				{
					SRNotify(hwnd,1);
					SRActionOnOff( hwnd,FALSE);  // turn off the action all of the time

					if(bFirstTime)
					{
						if(SRRequest(hwnd,lpSRContext)== VADLGOPEN)
							return 0;
						bFirstTime = FALSE;
					}
					SROnOff(hwnd,0);
					SREnableCommand( hwnd, lpSRContext);
					SROnOff(hwnd,1);
					return 0;

				}
				else    // becoming inactive
				{
					SRRequest(hwnd,lpSRContext);
					return 0;
				}
			}
			if(message == msgSpeechStop)
			{
				myMsgSpeechStop(hwnd,wParam,lParam);
            return 0;
         }
			if(message == msgLinkTerminated)
			{
				bLink = FALSE;
				return 0;
			}
			break;

	} // end of switch-message

	return DefWindowProc (hwnd, message, wParam, lParam);
}
