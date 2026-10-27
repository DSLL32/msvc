/******************************************************************************
			Copyright Creative Technology Ltd, 1993
				All rights reserved

MODULE NAME	: vamemo.C
PROGRAMMER	: Nam Hoang
******************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <memory.h>
#include <mmsystem.h>
#include <vasapi.h>
#include "vamemo.h"


struct MEMO Memo[MAXMEMO];

/* from file.c */
extern void ConstructPath(void);
extern void GetVoiceFileName(int);
extern int FillMemoStruct(LPSTR);
extern int AddMemoFile(HWND hWnd,int nNumrecs);
extern void ConstructPath(void);
extern void DeleteMemoStruct(HWND hwnd,int nDeleteIndex);
extern void myPlayVoice(BOOL nFlag,HWND,char *);
extern void myDeleteUserMemWaveFile(LPSTR szUserName,HWND hwnd);
extern int DeleteMemFiles(HWND);

static void myRegisterAPIWndMess(HWND hwnd);
static int myRequest(HWND hwnd);
static void myBecomingActive(HWND,LPARAM);
static void myTrainVoice(HWND hwnd);
static void myMsgActionStart(HWND hwnd,UINT,LONG);
static void myMsgStopSpeech(HWND hwnd,UINT,LONG);
static void myTrainAction(HWND hwnd);
static int myFirstSetup(HWND hwnd);
BOOL myMemoFileExist(LPSTR szUser,HWND hwnd);
WORD OpenWaveDevice(void);

HWND	hCommListBox, hUserListBox;
WORD	msgActionStart, msgActionStop, msgSpeechStart, msgSpeechStop;
WORD	msgLinkTerminated, msgMeter, msgAppSwitch;

LPSRCONTEXT	lpSRContext;
LPSRUSERLIST	lpSRUserList;
SRWORD	sSRWord;
int gnIndex,gnNumMemos,gnAction = GETACTION;
int  gnAddStatus = 0;
HANDLE hInst;
ATOM aUserAtomID = 1;

#include "addfnct.c"
#include "recfnct.c"
#include "DelGet.c"

HWAVEOUT hWaveOut;

PCMWAVEFORMAT sWaveFormat,*pFormat;

WORD OpenWaveDevice()
{
	WORD wError;

	sWaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
	sWaveFormat.wf.nChannels=1;
	sWaveFormat.wf.nSamplesPerSec = 11025L;
	sWaveFormat.wf.nAvgBytesPerSec = 11025L;
	sWaveFormat.wf.nBlockAlign = 1;
	sWaveFormat.wBitsPerSample=8;

	wError=waveOutOpen((LPHWAVEOUT) &hWaveOut, WAVE_MAPPER,
		(LPWAVEFORMAT) &sWaveFormat, 0L,0L,NULL);

	return wError;
}

/***********************************************************************************
 	myRegisterAPIWndMess: registers VAAPI messages and allocates  two global structures
*************************************************************************************/
static void myRegisterAPIWndMess(HWND hwnd)
{
	
	msgActionStart	= RegisterWindowMessage( "ACTION_START");
	msgActionStop	= RegisterWindowMessage( "ACTION_STOP");
	msgSpeechStart	= RegisterWindowMessage( "SPEECH_START");
	msgSpeechStop	= RegisterWindowMessage( "SPEECH_STOP");
	msgMeter			= RegisterWindowMessage( "METER");
	msgAppSwitch	= RegisterWindowMessage( "APP_SWITCH");
	msgLinkTerminated = RegisterWindowMessage( "LINK_TERMINATED");

	lpSRUserList= (LPSRUSERLIST) GlobalAllocPtr( GHND, sizeof( SRUSERLIST));
	lpSRContext = ( LPSRCONTEXT) GlobalAllocPtr( GHND, sizeof( SRCONTEXT));

}

/***********************************************************************************
	myMsgStopSpeech: when VA recognized a command, this function will check which recognized
	command, and postmessage to the control that relates to this command
****************************************************************************************/
static void myMsgStopSpeech(HWND hwnd,UINT wParam,LONG lParam)
{
	int nIndex,i;
	char szBuffer[40];

	if( HIWORD(lParam))  // error in recognition
	{
		if(gnAddStatus == IDD_TR_VOICE)
		{
			ReleaseCapture();
			PostMessage(hwnd,WM_COMMAND,IDD_TR_VOICE,0L);
		}
		return;
   }
		

	nIndex = LOWORD(lParam);
	switch(nIndex)  // is the index "Add" or "Delete"?
	{
		case DELETEACTION:
			PostMessage(hwnd,WM_COMMAND,IDD_DELETE,0L);
			return ;
		case ADDACTION:
			PostMessage(hwnd,WM_COMMAND,IDD_ADD,0L);
			return ;
		default:
      	break;
	}

 	ListBox_SetCurSel(hCommListBox,nIndex-ADDACTION-1);  // highlight a memo
	if(gnAction == DELETEACTION)   // Delete Mode is on
	{
			PostMessage(hwnd,WM_COMMAND,IDD_DELETE,0L);
         return ;
   }
	if(gnAddStatus== IDD_TR_VOICE) 				// add mode is current, train voice is over
		myAddComm(hwnd);
}

/****************************************************************************************
	myMsgActionStart: Processes recognized command. Posts or sends message to play
	the memo 
*****************************************************************************************/
static void myMsgActionStart(HWND hwnd,UINT wParam,LONG lParam)
{
	
	char szBuffer[32];
	int nIndex,i;

	if( HIWORD(lParam))
   	return ;

	if(gnAction == ADDACTION) 			// add mode is present
		return ;

	nIndex = LOWORD(lParam); 			// index of recog. command
	if(gnAction != DELETEACTION)    	// if not Delete mode then see which index VA recognized
	{
		if(nIndex > ADDACTION)        // index is one of memos
		{
      	
			if(gnAddStatus)            
			{
				gnAddStatus = 0;       	// reinitialize the gnAddStatus
			}
		  	wsprintf((LPSTR)szBuffer,"  Play \"%s\"",
				(LPSTR) lpSRContext->sCommInfo[nIndex].szCommName );

			SetDlgItemText(hwnd,IDD_STATUS,szBuffer);

			if(!OpenWaveDevice())   	// open dummy device because mci is slow
			{									// Voice will be played after the action
				PostMessage(hwnd,WM_PLAYVOICE,nIndex-ADDACTION-1,0L);  	// play record memo
         }
		  	return;
		}
   }
}


/**********************************************************************************
	myMsgActionStop:  This function is neccessary
		after quitting the action training to restore  VA, and to save a new memo.
***********************************************************************************/
static void myMsgActionStop(HWND hwnd,UINT wParam,LONG lParam)
{

	if(gnAddStatus==IDD_TR_ACTION) 							// after quit the training action
	{
		gnAddStatus= 0;
                                                      // unfreeze the command set
		SRFreeze(hwnd,FALSE);
		SROpen();  													// restore V_ASSIST

		Button_Enable(GetDlgItem(hwnd,IDD_ADD),TRUE);

		Button_Enable(GetDlgItem(hwnd,IDD_DELETE),TRUE);

		SetDlgItemText(hwnd,IDD_EDIT,0);                // save
		PostMessage(hwnd,WM_SAVE,0,0L);
		PostMessage(hwnd,WM_COMMAND,IDD_GET,0L); 			// set GET-MODE

	}

}

/***************************************************************************************
	myTrainVoice:Called after typing a new memo name. Trains the voice memo
*****************************************************************************************/
static void myTrainVoice(HWND hwnd)
{

	char szBuffer[40];

	SetDlgItemText(hwnd,IDD_STATUS,(LPSTR)"Please say a memo name");

	sSRWord.wIndex = gnIndex;
	sSRWord.bNew = 1;
	lstrcpy( sSRWord.szUserName, lpSRContext->szUserName );
	lstrcpy( sSRWord.szAppName, lpSRContext->szAppName );

	if(SRTrainVoice( hwnd, &sSRWord)) 			// SRTrainVoice returns an error
	{
		MessageBox(hwnd,"Bad recording! Last memo is cancel. Say \"Add\" to create a new command.",
						"Error",MB_OK);
		PostMessage(hwnd,WM_COMMAND,IDD_GET,0L);
	}
	else 
	{
		SetCapture(hwnd);   						// capture the mouse in this application
	}
}


/****************************************************************************************
	myTrainAction: Called if gbTrainActionOption is True (go to sysmenu to setup this
	variable). Trains the action for a new memo after recording memo.
*************************************************************************************/
static void myTrainAction(HWND hwnd)
{
	char szBuffer[40];

	SetDlgItemText(hwnd,IDD_STATUS, " Press \"Pause\" when done.");

	sSRWord.wIndex = gnIndex;

	SRFreeze(hwnd,TRUE); 				// free the command set

	sSRWord.bNew = 1;

	SRTrainAction( hwnd, &sSRWord);
}

/**************************************************************************************
	myMemoFileExist: will check the szUser ( xxxxx.mem ) is in lpSRUseList->szTableName.
*************************************************************************************/
BOOL myMemoFileExist(LPSTR szUser,HWND hwnd)
{
	int i;
	char szFile[30];

	for( i=0; i<lpSRUserList->wNumUsers; i++)
	{
		wsprintf((LPSTR)&szFile,"%s.mem",lpSRUserList->szTableName[i].szUserName);
		if((lstrcmpi((LPSTR)szUser, (LPSTR)szFile))==0)
      {
			return	TRUE;
      }
	}
	return FALSE;

}

/******************************************************************************
 myFirstSetup: Fills up Memo struct, makes a subclass	for edit control,
 	disables all generic commands except "close",	"go to sleep", "wake up", 
 	enables all  application commands, and deletes mem files which are not present
 	in user table list anymore.
**********************************************************************************/
static int myFirstSetup(HWND hwnd)
{
	int i;
	char szTitle[60],szUserName[30];

	if(myRequest(hwnd)== VADLGOPEN)
		return VADLGOPEN;  			// Request and fill list box and struct

	// enable application comms, "gotosleep" and "wakeup"
	for (i=1;i<lpSRContext->wNumCommands;i++)
	{
		if(i == GOTOSLEEP || i==WAKEUP || i>= 32 )
			lpSRContext->sCommInfo[i].wData = 0;
		else
			lpSRContext->sCommInfo[i].wData = 1; // disable generic comm.
	}


	SRGetUserList( hwnd, lpSRUserList);    // get users list 
	DeleteMemFiles(hwnd);  	// delete all mem files that their name are not in the user table.


	wsprintf((LPSTR)szTitle,"%s VoiceAssist Memos",(LPSTR)sSRWord.szUserName);

	SetWindowText(hwnd,(LPSTR)szTitle);

	Button_Enable(GetDlgItem(hwnd,IDD_STOPRECORD),FALSE); 

	if(wpEditWndProc<=0)   //	 subclass the edit control
	{
		ghEditMemo=GetDlgItem(hwnd,IDD_EDIT);
		wpEditWndProc=SubclassWindow(ghEditMemo,EditMemoSubClassProc);
		EnableWindow(ghEditMemo,FALSE);  	// disable edit window until the add mode is on
	}
  
   // fill struct Memo from mem file and find out how many records in this struct. 
	gnNumMemos = FillMemoStruct((LPSTR)sSRWord.szUserName);


	if(gnNumMemos <=0)  // there is no memo
   {
		SetDlgItemText(hwnd,IDD_STATUS,(LPSTR)" Say \" Add\" to create a new  memo");

		Button_Enable(GetDlgItem(hwnd,IDD_DELETE),FALSE); // disable "delete" button

		lpSRContext->sCommInfo[DELETEACTION].wData = 1;   // disable the "delete" comm
	}
	else if(gnNumMemos != (lpSRContext->wNumCommands - ADDACTION -1) )  
	{  // number of memos in Mem file is not same as in lpSRContext

		ListBox_ResetContent( hCommListBox);

		wsprintf((LPSTR)szUserName,"%s.mem",(LPSTR)sSRWord.szUserName);

		myDeleteUserMemWaveFile(szUserName,hwnd); 		// delete this szUserName.mem

		gnNumMemos = FillMemoStruct((LPSTR)sSRWord.szUserName);

		SetDlgItemText(hwnd,IDD_STATUS,(LPSTR)" Say \" Add\" to create a new  memo");

		Button_Enable(GetDlgItem(hwnd,IDD_DELETE),FALSE); // disable "delete" button

		lpSRContext->sCommInfo[DELETEACTION].wData = 1;   // disable the "delete" comm
	}
   else
		SetDlgItemText(hwnd,IDD_STATUS," Say \" Add \", \" Delete \" or memo name. ");

	SREnableCommand( hwnd, lpSRContext);
   return 0;

}

/********************************************************************************************
	myBecomingActive: it is called anytime when application becomes active.
		It restores previous status command, enables all of this application commands and "wake up",
		" go to sleep" and "close" and disables other commands. This function also check to see the
		current user file which is same or not with prev user file, or the current user file
		includes this appl srt file or not. 
   
***********************************************************************************************/
static void myBecomingActive(HWND hwnd,LPARAM lParam)
{
	static BOOL bFirstTime = TRUE;
	int nError;

	SRNotify( hwnd, 1);
	nError = SROnOff(hwnd,0);
	SREnableButtons(hwnd,FALSE);

	if(aUserAtomID != LOWORD(lParam))
	{
		aUserAtomID = LOWORD(lParam);    // save the current user atom id
		if(!aUserAtomID)						// user file not include vamemo.srt
		{
			PostMessage(hwnd,WM_CLOSE,0,0L);
			return ;
      }
		bFirstTime = TRUE;
   }
	if(bFirstTime)
	{
	  	if(myFirstSetup(hwnd)== VADLGOPEN)  // uer or training dlg is opened
	  		return ;
	  bFirstTime = FALSE;
	}
	SREnableCommand( hwnd, lpSRContext);
   if(!nError)
		SROnOff(hwnd,1);
}


/*
	myRequest: fills up struct lpSRContext,  updates list box,
				and gets user and application name
*/                                                
static int myRequest(HWND hwnd)
{
	int i,nError;
	char szBuffer[32];

	if(SRRequest(hwnd,lpSRContext) == VADLGOPEN)
		return VADLGOPEN; 	// fail for request

	ListBox_ResetContent( hCommListBox);

	for( i=ADDACTION+1; i<lpSRContext->wNumCommands; i++)// Show all memo comm. in list box
		ListBox_AddString( hCommListBox,(LPSTR) lpSRContext->sCommInfo[i].szCommName);

	lstrcpy((LPSTR) sSRWord.szUserName, (LPSTR)lpSRContext->szUserName );

	lstrcpy((LPSTR) sSRWord.szAppName,(LPSTR) lpSRContext->szAppName );
   return 0;
}


int PASCAL WinMain (HANDLE hInstance, HANDLE hPrevInstance,
					  LPSTR lpszCmdLine, int nCmdShow)
{
	HWND		hwnd,hAccel;
	MSG		msg;
	WNDCLASS	wndclass;

	if (!hPrevInstance)
	{
		wndclass.style         = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc   = WndProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = DLGWINDOWEXTRA; // Because of CreateDialog
		wndclass.hInstance     = hInstance;
		wndclass.hIcon         = LoadIcon( hInstance, "VAMEMO");
		wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW);
		wndclass.hbrBackground = GetStockObject( WHITE_BRUSH);
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = "VAMEMO";

		RegisterClass( &wndclass);
	 }

   hInst = hInstance;
   ConstructPath();
	hwnd = CreateDialog( hInstance, "VAMEMO", 0, NULL);

	hCommListBox = GetDlgItem( hwnd, IDD_LISTBOX);

	ShowWindow( hwnd, nCmdShow);
	UpdateWindow( hwnd);
   hAccel = LoadAccelerators(hInstance,"VAMEMO");
   
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

long WINAPI _export WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{

  static BOOL	bLink=FALSE;
  int nIndex,nError;
  static HMENU hMenu;

  char szBuffer[150],szUserName[32];

	switch( message)
	{

		case WM_CREATE:
			SetFocus(hwnd);
			myRegisterAPIWndMess(hwnd);
			hMenu = GetSystemMenu(hwnd,FALSE);
         // append the train action option into sysmenu
			AppendMenu(hMenu,MF_ENABLED, IDD_TR_ACTION, "T&rain Action Off");
			return 0;

		case WM_ACTIVATEAPP:
			if(!bLink && wParam)
         {
				bLink =  !SRInitializeLink(hwnd);
			}
		  return 0;

		case WM_SAVE: 				// save memo after recording memo.
			SROnOff(hwnd,0);
			SRSave( hwnd, sSRWord.szUserName);
			SROnOff(hwnd,1);
			return 0;

		case WM_RECORDMEMO: 		// record memo after editting the command
			if(gnAddStatus)
				myRecordMemo(hwnd);
			return 0;

		case WM_PLAYVOICE: 		// play the record memo for each command is recognized
			SROnOff(hwnd,0);
			waveOutClose(hWaveOut);
			myPlayVoice(TRUE,hwnd,Memo[wParam].szVoiceFile);
			return 0;
	  
		case MM_MCINOTIFY:
			if(wParam == MCI_NOTIFY_SUCCESSFUL)
         {
				if(gnAddStatus == IDD_RECORDMEMO)
				{
				 	mySaveMemo(hwnd);
			  		SROnOff(hwnd,1);  	// turn on engine after recording or play back
					return 0;

				}
				SetDlgItemText(hwnd,IDD_STATUS,"Say a memo name or say \"Add\" or\"Delete\"");

				myPlayVoice(FALSE,hwnd,NULL);   // close mci device after play back the wave file

				SROnOff(hwnd,1);  	// turn on engine after recording or play back

			}
			return 0;

		case WM_COMMAND:
		switch(wParam)
		{
			case IDD_ADD:  										// ADD A NEW MEMO

				if(gnAction == DELETEACTION) 					// previous action was delete mode,
					SRActionOnOff(hwnd,TRUE);     			// turns action on 

				if(SROnOff(hwnd,1)==VADLGOPEN )      		// VA dialog is opened
				{
					MessageBox(hwnd,
						"VA dialog is opened! Close it before adding a new memo.",
						"Error",MB_OK);
					return 0;
				}
				gnAction = ADDACTION;
				myPreEditCommand(hwnd);
            SendMessage(hwnd,WM_COMMAND,IDD_TR_VOICE,0L);
				break;

			case IDD_LISTBOX:
				if(gnAction == GETACTION) 					// GET mode is present
				{
					if(HIWORD(lParam) == LBN_DBLCLK) 	//  selected memo to play with action
					{
						if( LB_ERR != (nIndex =  ListBox_GetCurSel(hCommListBox)) )
						{
							wsprintf((LPSTR)szBuffer,"  Play \"%s\"",
								(LPSTR) lpSRContext->sCommInfo[nIndex+ADDACTION+1].szCommName );
							SetDlgItemText(hwnd,IDD_STATUS,szBuffer);
							SROnOff(hwnd,0);  		
							if(!OpenWaveDevice())
								PostMessage(hwnd,WM_PLAYVOICE,nIndex,0L);  // play record memo
						}

					}
				}
				return 0;

			case IDD_TR_VOICE:   
				gnAddStatus = wParam; // set gnAddStatus == IDD_TR_VOICE
				myTrainVoice(hwnd);
				return 0;

			case IDD_TR_ACTION:   	// train action for a new memo.
				gnAddStatus = IDD_TR_ACTION;
				myTrainAction(hwnd);
				return 0;

			case IDD_GET: 				// set up getmode back after add-mode or delete-mode is used
				myGetRoutine(hwnd);
				return 0;

			case IDD_DELETE:  	  // set up Delete-mode or Delete a memo
				myDeleteRoutine(hwnd);
				return 0;

			case IDD_STOPRECORD:  // stop recording
				if(gnAddStatus == IDD_RECORDMEMO)
				{
					ReleaseCapture();
					mySaveMemo(hwnd);
            }
					
				return 0;

			default :
				break;
		}
		break;

		case WM_CLOSE:

			SRRequest(hwnd,lpSRContext); // to get the new value lpSRContext->wNumCommands

      	if(!aUserAtomID)	// user file without Vamemo file
				MessageBox(hwnd,
				"The user file not include Vamemo file. Terminating Vamemo!",
				 "No vamemo file. Error", MB_ICONSTOP | MB_OK);

			else if((gnNumMemos !=(lpSRContext->wNumCommands - ADDACTION-1)) ||( wParam == 1))
			{ 	// number Memos in vamemo list box is not same as in VA listBox

				ListBox_ResetContent( hCommListBox);

				wsprintf((LPSTR)szUserName,"%s.mem",(LPSTR)sSRWord.szUserName);

				myDeleteUserMemWaveFile(szUserName,hwnd); 	// delete an exist szUserName.mem

				MessageBox(hwnd," Number of memos in  this application and in user file are\
				 not same. Terminating Vamemo ! ", "Index Error",MB_OK);
			}

			SRTerminateLink(hwnd);
			break;


		case WM_LBUTTONDOWN:
			if(gnAddStatus == IDD_RECORDMEMO)
			{
				ReleaseCapture();
				mySaveMemo(hwnd);
         }
			return 0;

		case WM_DESTROY:
      	
			myPlayVoice(FALSE,hwnd,NULL); 		// close mci device
			GlobalFreePtr( lpSRUserList);
			GlobalFreePtr( lpSRContext);
			PostQuitMessage (0);
			return 0;

		case WM_SYSCOMMAND:
		{
			if(wParam==IDD_TR_ACTION)
			{
				if(gbTrainActionOption)
				{
					// don't want to train action 
            	gbTrainActionOption = FALSE;
					ModifyMenu(hMenu,IDD_TR_ACTION,MF_ENABLED,IDD_TR_ACTION,
						(LPSTR)"Train Action Off" );
				}
				else
				{
					// want to train action
					gbTrainActionOption = TRUE;
					ModifyMenu(hMenu,IDD_TR_ACTION,MF_ENABLED,IDD_TR_ACTION,
						(LPSTR)"Train Action On" );

            }
				return 0;
			}
			 // no break point here
      }

		default:

			if( message == msgSpeechStop)
			{
				myMsgStopSpeech(hwnd,wParam,lParam);
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
					myRequest(hwnd);
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

	} // end of switch-message

	return DefWindowProc (hwnd, message, wParam, lParam);
}
