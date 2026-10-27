/******************************************************************************
				Copyright Creative Technology Ltd, 1993
						All rights reserved

MODULE NAME	: vahello.C
PROGRAMMER	: Nam Hoang
******************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <memory.h>
#include <mmsystem.h>
#include <vasapi.h>

#define WM_PLAYVOICE WM_USER+192
#define HELLOINDEX   32
#define VADLGOPEN	4  


long WINAPI _export WndProc (HWND hwnd, UINT message, UINT wParam,
										 LONG lParam);
static void myRegisterAPIWndMess(HWND hwnd);


WORD	msgLinkTerminated,msgActionStart,  msgAppSwitch;

LPSRCONTEXT	lpSRContext;
LPSRUSERLIST	lpSRUserList;
SRWORD	sSRWord;
ATOM gaUserAtomID = 1;  // initialize atom value

void myPlayVoice(BOOL nFlag,HWND hwnd, char *szFileName)
{
	
	char szVoiceBuf[256],szRetString[80];
	DWORD dwRes;

	if(nFlag) // play the voice
	{
	  	mciSendString ((LPSTR)"stop goober", NULL, 0, NULL);
	  	mciSendString ((LPSTR) "close goober",NULL,0,NULL);

		wsprintf((LPSTR)szVoiceBuf,"open %s type waveaudio alias goober",
			(LPSTR)szFileName);

		dwRes = mciSendString ((LPSTR)szVoiceBuf, szRetString, 80, NULL);

		if(dwRes)
		{
			SROnOff(hwnd,1);
			return ;
		}

		dwRes = 	mciSendString ((LPSTR)"play goober from 0 notify ",
						 szRetString, 80, hwnd);
		if(dwRes)
			return ;

	}
	else //close mci
	{
   	
		mciSendString ((LPSTR)"stop goober", NULL, 0, NULL);
		mciSendString ((LPSTR) "close goober",NULL,0,NULL);
	}

}
static void myRegisterAPIWndMess(HWND hwnd)
{
	
	 msgActionStart	= RegisterWindowMessage( "ACTION_START");

	 msgAppSwitch	= RegisterWindowMessage( "APP_SWITCH");

	 msgLinkTerminated = RegisterWindowMessage( "LINK_TERMINATED");

	 lpSRUserList= (LPSRUSERLIST) GlobalAllocPtr( GHND, sizeof( SRUSERLIST));
	 lpSRContext = ( LPSRCONTEXT) GlobalAllocPtr( GHND, sizeof( SRCONTEXT));
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
		wndclass.hIcon         = LoadIcon( hInstance, "VAHELLO");
		wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW);
		wndclass.hbrBackground = GetStockObject( WHITE_BRUSH);
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = "VAHELLO";

		RegisterClass( &wndclass);
	 }
	hwnd = CreateDialog( hInstance, "VAHELLO", 0, NULL);
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
			SetFocus(hwnd);
			myRegisterAPIWndMess(hwnd);
			return 0;

		case WM_ACTIVATEAPP:
			if(!bLink && wParam)
				bLink =  !SRInitializeLink(hwnd);   // call SRInitialLink()
			return 0;

		case WM_PLAYVOICE:
			 // play hello.wav when va recognizes "hello" command
			myPlayVoice(TRUE,hwnd,"hello.wav");
			return 0;

		case WM_CLOSE:
      	if(!gaUserAtomID) 		// user file doesn't include vahello.srt
				MessageBox(hwnd,
					"This user file does not include vahello. Terminating VAHello.",
					 "Error", MB_ICONSTOP|MB_OK);

			SRTerminateLink(hwnd);
			break;

		case WM_DESTROY:
			myPlayVoice(FALSE,hwnd,NULL);
			GlobalFreePtr( lpSRUserList);
			GlobalFreePtr( lpSRContext);
			PostQuitMessage (0);
			return 0;

		default:  				// all VA Messages will be processed here
			if( message == msgAppSwitch)
			{
				if( wParam) 				// becoming active, getting the link 
				{
					SREnableButtons(hwnd,FALSE);
					SRNotify(hwnd,1);   // enable message notification

					if(gaUserAtomID != LOWORD(lParam)) 
					{
						gaUserAtomID = LOWORD(lParam);
						if(!gaUserAtomID) // user file doesn't not include the vahello.srt
						{
							PostMessage(hwnd,WM_CLOSE,0,0L);
                     return 0;
                  }
						bFirstTime = TRUE;
               }
				  	if(bFirstTime)
               {
						if(SRRequest(hwnd,lpSRContext)== VADLGOPEN) // VA user or Training dlg is opened
							return 0; 	
						bFirstTime = FALSE;           // first time's over
               }
					SRActionOnOff(hwnd,lpSRContext->wAction); 	// restore action state
					SROnOff(hwnd,0);  							 		// turn off driver 
					SREnableCommand( hwnd, lpSRContext);         // restore the prev status
					SROnOff(hwnd,1);                       		// turn on dirver
					return 0;

				}
			  	else						 				// becoming inactive
			 	{
			 		SRRequest(hwnd,lpSRContext);	// save current status
			 		return 0;
			 	}
			}
			if( message == msgActionStart)    	// recognition/action process
			{
				if(LOWORD(lParam)==HELLOINDEX)
					SendMessage(hwnd,WM_PLAYVOICE,0,0L);
				return 0;
			}

			if(message == msgLinkTerminated)   	 // losing the link
			{
				bLink = FALSE;
			  //	myDebugString("link terminated");
				return 0;
			}
			break;

	} // end of switch-message

	return DefWindowProc (hwnd, message, wParam, lParam);
}
