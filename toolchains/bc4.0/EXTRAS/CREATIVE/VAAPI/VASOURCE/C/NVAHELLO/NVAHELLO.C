/******************************************************************************
				Copyright Creative Technology Ltd, 1993
						All rights reserved

MODULE NAME	: vahello.C
PROGRAMMER	: Nam Hoang
******************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#define ID_HELLO 102

HANDLE hInst;

long WINAPI _export WndProc (HWND hwnd, UINT message, UINT wParam,
										 LONG lParam);
int PASCAL WinMain (HANDLE hInstance, HANDLE hPrevInstance,
					  LPSTR lpszCmdLine, int nCmdShow)
{
char szAppName[] = "HELLO";
HWND		hwnd,hAccel;
MSG		msg;
int nError;
WNDCLASS	wndclass;

	if (!hPrevInstance)
	{
		wndclass.style         = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc   = WndProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = DLGWINDOWEXTRA; // Because of CreateDialog
		wndclass.hInstance     = hInstance;
		wndclass.hIcon         = LoadIcon( NULL, IDI_APPLICATION);
		wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW);
		wndclass.hbrBackground = COLOR_WINDOW+1;
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = szAppName;

		nError = RegisterClass( &wndclass);
	 }
   hInst = hInstance;
	hwnd = CreateDialog( hInst, szAppName, 0, NULL);
	ShowWindow( hwnd, nCmdShow);
	UpdateWindow( hwnd);
  	hAccel = LoadAccelerators(hInst,szAppName);
	while (GetMessage( &msg, NULL, 0, 0))
	{
	  	if(!TranslateAccelerator(hwnd,hAccel,&msg))
      {
			TranslateMessage( &msg);
			DispatchMessage( &msg);
		}
	}

return msg.wParam;
}

void myPlayVoice(BOOL nFlag,HWND hwnd)
{

	char szVoiceBuf[256],szRetString[80];
	DWORD dwRes;

	if(nFlag) // PLay the voice
	{
	  	mciSendString ((LPSTR)"stop goober", NULL, 0, NULL);
	  	mciSendString ((LPSTR) "close goober",NULL,0,NULL);

		wsprintf((LPSTR)szVoiceBuf,"open nvahello.wav alias goober");
	  	dwRes = mciSendString ((LPSTR)szVoiceBuf, szRetString, 80, NULL);
	  	if(dwRes)
	  	{
		  return ;
	  	}
		dwRes = 	mciSendString ((LPSTR)"play goober ", szRetString, 80, NULL);
	  	if(dwRes)
		  return ;
   }
   else //close mci
	{
		  mciSendString ((LPSTR)"stop goober", NULL, 0, NULL);
		  mciSendString ((LPSTR) "close goober",NULL,0,NULL);
	}

}

long WINAPI _export WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	switch( message)
	{
		case WM_COMMAND:
			switch(wParam)
			{
				case ID_HELLO:
					myPlayVoice(TRUE,hwnd);
					return 0;
			}
			break;
		case WM_DESTROY:
			myPlayVoice(FALSE,hwnd); // close mci device
			PostQuitMessage(0);
			return 0;
      default:
			return DefWindowProc (hwnd, message, wParam, lParam);
	}
	return 0L;
}
