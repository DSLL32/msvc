/******************************************************************************
			Copyright Creative Technology Ltd, 1993
				All rights reserved
MODULE NAME	: VAControl.c
PROGRAMMER	: Nam Hoang
******************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <memory.h>
#include "apicli.h"
#include <vasapi.h>
#include "vacontro.h"




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
		wndclass.hIcon         = LoadIcon( hInstance, "CLIENT");
		wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW);
		wndclass.hbrBackground = GetStockObject( LTGRAY_BRUSH);
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = "CLIENT";

		RegisterClass( &wndclass);
	}

	hwnd = CreateDialog( hInstance, "CLIENT", 0, NULL);

	hUserListBox = GetDlgItem( hwnd, IDD_USERLIST);
	hCommListBox = GetDlgItem( hwnd, IDD_LISTBOX);

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
	

  	static BOOL	bLink=FALSE;
  	char szBuffer[150];

	switch( message)
	{

		case WM_CREATE:
			SetFocus(hwnd);
			myRegisterAPIWndMess(hwnd);
			return 0;

		case WM_ACTIVATEAPP:
			
			if(!bLink)
         {
				bLink =  !SRInitializeLink(hwnd);
				SROnOff(hwnd,gbVAStatus);
            gbVAStatus = FALSE;
         }
         return 0;


		case WM_COMMAND:
		switch(wParam)
		{
      	
			case IDD_INITIATE:
			  	bLink = !SRInitializeLink( hwnd);
			  	return 0;

			case IDD_TERMINATE:

			  	SRTerminateLink( hwnd);
			  	return 0;

			case IDD_ADVISE:
			case IDD_UNADVISE:

			  	SRNotify( hwnd, IDD_UNADVISE - wParam);
			  	return 0;

			case IDD_OPENDRV:
			case IDD_CLOSEDRV:
			  	SROnOff( hwnd, IDD_CLOSEDRV - wParam);
			  	return 0;


			case IDD_ENABLE_ACTION:
			case IDD_DISABLE_ACTION:

				SRActionOnOff( hwnd, IDD_DISABLE_ACTION - wParam);
				myRequest(hwnd);
				return 0;

			case IDD_GETUSERS:
				myGetUsers(hwnd);
				return 0;

			case IDD_CHANGE_USER:
				myChangeUser(hwnd);
				return 0;

			case IDD_VERSION:
				myGetVersion(hwnd);
				return 0;

			case IDD_CAPTURE:
				SRFreeze( hwnd, TRUE);
				return 0;

			case IDD_RELEASE:
				SRFreeze( hwnd, FALSE);
				return 0;

			case IDD_CLOSE: // close voice assist
				SRClose( );
				return 0;

			case IDD_OPEN:  // open voice assist
				SROpen();
				return 0;

			case IDD_REQUEST:
				myRequest(hwnd);
				return 0;

			case IDD_SAVE:
				SROnOff(hwnd,0);
				SRSave( hwnd, sSRWord.szUserName);
            SROnOff(hwnd,1);
				return 0;

			case IDD_TR_VOICE:
				myTrainVoice(hwnd);
				return 0;

			case IDD_TR_ACTION:
				myTrainAction(hwnd);
				return 0;

			case IDD_DELETE:
				myDeleteComm(hwnd);
				return 0;

			case IDD_ADD:
            myAddComm(hwnd);
				return 0;

			case IDD_RENAME:
				myRenameComm(hwnd);
				return 0;

			case IDD_ACTIVATE:
			case IDD_DEACTIVATE:
				myDisableEnableComm(hwnd,wParam);
				return 0;

		}
		break;

		case WM_CLOSE:
			SRTerminateLink( hwnd);
			break;


		case WM_DESTROY:
			GlobalFreePtr( lpSRUserList);
			GlobalFreePtr( lpSRContext);
			PostQuitMessage (0);
			return 0;

		default:
			if( message == msgMeter)
			{
				SetDlgItemText( hwnd, IDD_MESSAGE, "Meter");
				wsprintf((LPSTR) szBuffer, "%d", wParam);
				SetDlgItemText( hwnd, IDD_MESSAGE_DATA, szBuffer);
				return  0;
			}

			if( message == msgSpeechStart)
			{

				SetDlgItemText( hwnd, IDD_MESSAGE, "Speech start");
				return 0 ;
			}

			if( message == msgSpeechStop)
			{
				SetDlgItemText( hwnd, IDD_MESSAGE, "Speech start");

				myMsgSpeechStop(hwnd,wParam,lParam);
				return 0;
			}

			if( message == msgAppSwitch)
			{

				if( wParam) 		// becoming active
				{
					myBecomingActive(hwnd,lParam);
					return 0;
				}
				if(wParam == 0) 	// becoming inactive
				{
            	SRRequest(hwnd,lpSRContext);
					return 0;
				}

			}

			if( message == msgActionStart)
			{
				myMsgActionStart(hwnd,wParam,lParam);
				return 0;
			}

			if( message == msgActionStop)
			{
				myMsgActionStop(hwnd,wParam,lParam);
				return 0;
			}

			if(message == msgLinkTerminated)
			{
				bLink = FALSE;
				return 0;
			}
			break;
	}

	return DefWindowProc (hwnd, message, wParam, lParam);
}

static void myGetVersion(HWND hwnd)
{
	 LONG		lVersion;
	 char szBuffer[10];

	 if( !SRGetVersion( hwnd, &lVersion))
	 {
		 wsprintf( szBuffer, "%lX\n", (LONG)lVersion);
	 }
}

static void myMsgSpeechStop(HWND hwnd,UINT wParam,LONG lParam)
{
	int nError,i;
	char szBuffer[40];

	if((nError = HIWORD(lParam)))  // error in recognition
	{
		wsprintf( szBuffer, "%d", nError); // Error code
		SetDlgItemText( hwnd, IDD_MESSAGE_DATA, szBuffer);
		SetDlgItemText( hwnd, IDD_MESSAGE, "Err in Recognition");
		wsprintf(szBuffer,"\nErr in Rec. # %d\n",nError);
		return;
	}
	SetDlgItemText( hwnd, IDD_MESSAGE, "Stop speech");

	wsprintf( szBuffer, "%d", LOWORD(lParam)); // index of recognized command
	SetDlgItemText( hwnd, IDD_MESSAGE_DATA, szBuffer);

}


/**************************************************************
	myAddComm(): make a modal dialog box for adding a new command
***************************************************************/
static void myAddComm(HWND hwnd)
{
	 int nError;
	 FARPROC lpfnAddCommProc;
	 HANDLE hInstance;

	 hInstance = GetWindowWord( hwnd, GWW_HINSTANCE);
	 lpfnAddCommProc = MakeProcInstance(AddCommProc,hInstance);
	 DialogBox(hInstance,"AddCommand",hwnd,lpfnAddCommProc);
	 FreeProcInstance(lpfnAddCommProc);
	 if(gnIndex>=0)
		 ListBox_SetCurSel(hCommListBox, gnIndex);
}

static void myRenameComm(HWND hwnd)
{
	
	int nError;
	FARPROC lpfnRenameCommProc;
	HANDLE hInstance;

	gnIndex = ListBox_GetCurSel(hCommListBox);

	if(gnIndex <0)
	{
	  	MessageBox(hwnd," You didn't select a command to rename.","Error",MB_OK);
	  	return ;
	}
	else if(gnIndex < 32)
	{
	  	MessageBox(hwnd,"Cannot rename the generic commands","Error", MB_OK);
	  	return ;
	}

	SROnOff(hwnd,0);

	hInstance = GetWindowWord( hwnd, GWW_HINSTANCE);

	lpfnRenameCommProc = MakeProcInstance(RenameCommProc,hInstance);

	DialogBox(hInstance,"RenameCommand",hwnd,lpfnRenameCommProc);

	FreeProcInstance(lpfnRenameCommProc);

	myRequest(hwnd);
}

static void myChangeUser(HWND hwnd)
{
	
	WORD i;
	char szBuffer[40];

	if (  ( i = ListBox_GetCurSel(hUserListBox) ) == LB_ERR)
		return ;

	ListBox_GetText( hUserListBox, i,szBuffer );

	lstrcpy( sSRWord.szUserName,szBuffer );

	SROnOff(hwnd,0); 				// turn driver off before change user

	SRChangeUser( hwnd,szBuffer );

	myRequest(hwnd);

	SROnOff(hwnd,1);
}

static void myGetUsers(HWND hwnd)
{
	
	WORD i;

	if( SRGetUserList( hwnd, lpSRUserList))
		return; // there is an error

	ListBox_ResetContent( hUserListBox);
	for( i=0; i<lpSRUserList->wNumUsers; i++)
		  ListBox_AddString( hUserListBox, lpSRUserList->szTableName[i].szUserName);

}

static void myDisableEnableComm(HWND hwnd,UINT wParam)
{
	 WORD i;

	 if ( ( i = ListBox_GetCurSel(hCommListBox) )==LB_ERR)
		 return ;

	 lpSRContext->sCommInfo[i].wData = !(IDD_DEACTIVATE - wParam);

	 SROnOff(hwnd,0);

	 SREnableCommand( hwnd, lpSRContext);

	 myRequest(hwnd);

}

static void myMsgActionStop(HWND hwnd,UINT wParam,LONG lParam)
{

	 if(gbTrainAction)
	 {
		 gbTrainAction= FALSE;

		 SRFreeze(hwnd,FALSE);					// unfreeze the commands

		 SROnOff(hwnd,0); 						// turn off before calling SRSave

		 SRSave( hwnd, sSRWord.szUserName);

		 SROpen();

		 SROnOff(hwnd,gbVAStatus);  		// restore status of VA

		 gbVAStatus = FALSE;

		 SetFocus(hwnd);
	 }

}

static void myMsgActionStart(HWND hwnd,UINT wParam,LONG lParam)
{
	
	char szBuffer[32];
	int nError;

	if((nError = HIWORD(lParam))) // error in action
	{
	  SetDlgItemText( hwnd, IDD_MESSAGE, "ACTION ERROR");
	  wsprintf( szBuffer, "%d", nError);
	  SetDlgItemText( hwnd, IDD_MESSAGE_DATA, szBuffer);
	  return ;

	}

	SetDlgItemText( hwnd, IDD_MESSAGE, "ACTION");
	wsprintf( szBuffer, "%d", LOWORD(lParam)); 			// index recognition command
	SetDlgItemText( hwnd, IDD_MESSAGE_DATA, szBuffer);

	ListBox_SetCurSel(hCommListBox,LOWORD(lParam));

}

static void myTrainVoice(HWND hwnd)
{
	FARPROC lpfnVoiceTrainProc;
	HANDLE hInstance;
	int nError;

	if ( LB_ERR == ( gnIndex = ListBox_GetCurSel(hCommListBox) ))
	{
		MessageBox(hwnd," Select a command to train!", "Nothing to train!",MB_OK);
		return ;
	}

	(lpSRContext->wOnOff) ? (gbVAStatus = TRUE) : (gbVAStatus = FALSE);

	nError = SRTerminateLink(hwnd);  		// terminate the link because it makes another Dlg

	hInstance = GetWindowWord( hwnd, GWW_HINSTANCE);

	lpfnVoiceTrainProc = MakeProcInstance((FARPROC) VoiceTrainProcModel,hInstance);

	DialogBox(hInstance ,"TRAINVOICE",hwnd,lpfnVoiceTrainProc);

	FreeProcInstance(lpfnVoiceTrainProc);

	PostMessage(hwnd,WM_ACTIVATEAPP,0,0L); // reinitialize the link

	return ;

}

static void myTrainAction(HWND hwnd)
{
	 WORD i,nError;


	 if (( sSRWord.wIndex = ListBox_GetCurSel(hCommListBox) )==LB_ERR)
	 {
		 MessageBox(hwnd," Select a command to train!", "Nothing to train",MB_OK);
		 return ;
    }
	 gbTrainAction = TRUE;

	 (lpSRContext->wOnOff) ? (gbVAStatus = TRUE) : (gbVAStatus = FALSE);

	 SRFreeze(hwnd,TRUE); 				// freeze the command set
	 sSRWord.bNew = 1;
	 SRTrainAction( hwnd, &sSRWord);

}

static void myRegisterAPIWndMess(HWND hwnd)
{

	 msgActionStart	= RegisterWindowMessage( "ACTION_START");
	 msgActionStop	= RegisterWindowMessage( "ACTION_STOP");
	 msgSpeechStart	= RegisterWindowMessage( "SPEECH_START");
	 msgSpeechStop	= RegisterWindowMessage( "SPEECH_STOP");
	 msgMeter		= RegisterWindowMessage( "METER");
	 msgAppSwitch	= RegisterWindowMessage( "APP_SWITCH");
	 msgLinkTerminated = RegisterWindowMessage( "LINK_TERMINATED");
	 lpSRUserList= (LPSRUSERLIST) GlobalAllocPtr( GHND, sizeof( SRUSERLIST));
	 lpSRContext = ( LPSRCONTEXT) GlobalAllocPtr( GHND, sizeof( SRCONTEXT));

}

static void myBecomingActive(HWND hwnd,LPARAM lParam)
{
	
	static BOOL bFirstTime = TRUE;
	char szBuffer[32];
	int nError;

	SRNotify( hwnd, 1);
	if(bFirstTime)
	{
	  myRequest(hwnd);
	  bFirstTime = FALSE;
	}

	SRActionOnOff(hwnd,lpSRContext->wAction); 	//restore the prev status action
	nError = SROnOff(hwnd,0);
	SREnableCommand( hwnd, lpSRContext);   // restore prev. status comm.

   nError ? SROnOff(hwnd,FALSE):SROnOff(hwnd,TRUE);
}

static void myRequest(HWND hwnd)
{
	int i,nError;
	char szBuffer[32];

	if(SRRequest(hwnd,lpSRContext))
	{
		return ; 							// fail for request
	}

	wsprintf( szBuffer, "%d", lpSRContext->wNumCommands);
	SetDlgItemText( hwnd, IDD_NO_COMMANDS, szBuffer);
	SetDlgItemText( hwnd, IDD_APPLICATION, lpSRContext->szAppName);
	SetDlgItemText( hwnd, IDD_USER_NAME, lpSRContext->szUserName);

	// Is the driver on ?
	lpSRContext->wOnOff ?	lstrcpy( szBuffer, "On") : lstrcpy( szBuffer, "Off");

	lpSRContext->wAdvise ? SetDlgItemText( hwnd, IDD_ADVISEFLAG, "Advise On") :
		  SetDlgItemText( hwnd, IDD_ADVISEFLAG, "Advise Off");   // Is advise on ?

	lpSRContext->wAction ?	SetDlgItemText( hwnd, IDD_ACTIONFLAG, "Action On") :
		 SetDlgItemText( hwnd, IDD_ACTIONFLAG, "Action Off");	// Will VoiceAssist execute the action ?


	SetDlgItemText( hwnd, IDD_SNDNC_ON_OFF, szBuffer);


	ListBox_ResetContent( hCommListBox);

	for( i=0; i<lpSRContext->wNumCommands; i++)
	{
		wsprintf( szBuffer, "%d  %s",
			lpSRContext->sCommInfo[i].wData,
			(LPSTR) lpSRContext->sCommInfo[i].szCommName);
		ListBox_AddString( hCommListBox, szBuffer);
	}

	lstrcpy( sSRWord.szUserName, lpSRContext->szUserName );
	lstrcpy( sSRWord.szAppName, lpSRContext->szAppName );
}

static void myDeleteComm(HWND hwnd)
{

	sSRWord.wIndex = ListBox_GetCurSel(hCommListBox);

	if(sSRWord.wIndex <0)
	{
	  	MessageBox(hwnd,"You didn't select a command to delete yet.","Error", MB_OK);
	  	return ;
	}
	else if(sSRWord.wIndex < 32)
	{
	  	MessageBox(hwnd,"Cannot delete the generic commands","Error", MB_OK);
	 	 return ;
	}

	SROnOff(hwnd,0);

	if( SRDeleteCommand( hwnd, &sSRWord))
	  return;

	myRequest(hwnd);

	SROnOff(hwnd,1);

}

BOOL WINAPI _export AddCommProc(HWND hwnd, UINT message,
							 UINT wParam, LONG lParam)
{
	char szBuffer[32];

	switch(message)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch(wParam)
			{
				case IDOK:

					Edit_GetText(GetDlgItem(hwnd,ADD_EDIT), szBuffer,
										 sizeof(szBuffer));
					if(szBuffer[0] != 0)
					{
						SROnOff(GetParent(hwnd),0); 	// turn vassist off
						lstrcpy(sSRWord.szCommName,szBuffer);
						if(SRAddCommand(GetParent(hwnd), &sSRWord))
                  {
							MessageBox(hwnd,"Cannot add this command","Error",MB_OK);
							PostMessage(hwnd,WM_COMMAND,ADD_CANCEL,0L);
                  }
						gnIndex = lpSRContext->wNumCommands;
						PostMessage(GetParent(hwnd), WM_COMMAND,IDD_REQUEST,0L);

					}
					PostMessage(hwnd,WM_DESTROY,0,0L);
					return TRUE;

				case ADD_CANCEL:
					gnIndex = -1;
					PostMessage(hwnd,WM_DESTROY,0,0L);
					return TRUE;

			}
			return TRUE;

		case WM_DESTROY:
			EndDialog(hwnd,0);
			return TRUE;
	}
	return FALSE;
}

BOOL WINAPI _export RenameCommProc(HWND hwnd, UINT message,
							 UINT wParam, LONG lParam)
{
	char szBuffer[32];

	switch(message)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch(wParam)
			{
				case IDOK:

					Edit_GetText(GetDlgItem(hwnd,ADD_EDIT), szBuffer,
										 sizeof(szBuffer));
					if(szBuffer[0] != 0)
					{
               	
						sSRWord.wIndex = gnIndex;
						lstrcpy(sSRWord.szCommName,szBuffer);
						if(SRRenameCommand(GetParent(hwnd), &sSRWord))
                  {
							MessageBox(hwnd,"Cannot rename this command","Error",MB_OK);
							PostMessage(hwnd,WM_COMMAND,ADD_CANCEL,0L);
                  }

					}
					PostMessage(hwnd,WM_DESTROY,0,0L);
					return TRUE;

				case ADD_CANCEL:
					PostMessage(hwnd,WM_DESTROY,0,0L);
					return TRUE;

			}
			return TRUE;

		case WM_DESTROY:
      	gnIndex = -1;
			EndDialog(hwnd,0);
			return TRUE;
	}
	return FALSE;
}



