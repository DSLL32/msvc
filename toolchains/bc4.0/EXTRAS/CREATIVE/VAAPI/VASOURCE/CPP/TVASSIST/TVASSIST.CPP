/*********************************************************
						Copyright Creative Technology Ltd, 1993
								All Right Reserved
MODULE: tvassist.CPP
PROGRAMMER:	Nam Hoang								
*********************************************************/
#include <owl\owlpch.h>
#include <owl\applicat.h>
#include <owl\window.h>
#include <owl\dialog.h>
#include "tvassist.h"
#include <stdlib.h>
#include "vadllrc.h"
#include "dlladre.h"
#include "dlltrvoi.h"
#include "dlldelet.h"

TModule* VALib;
HWND hPrevVA;


TVAssist::TVAssist(TVAssist far * VAParent = NULL,int nAction = ACTIONON,
							BOOL bButton = TRUE )
{
	if(VAParent)  // when create another VA Object, Prev Object has to be terminated
	{
		VAParent->VATerminateLink();
		VAParent->bLink = TRUE;
	}
	msgTrainVoice = RegisterWindowMessage("TVASSIST_TRAINVOICE");
	gaUserAtomID = -1;
	bVAStatus = bFirstTime= TRUE;
	bTrainAction =bTrainVoice= bLink = FALSE;
	wCurTraining = 0; 								// for training voice
	bPrevAct = nAction;
	bEnableButtons = bButton;

	lpSRUserList =(LPSRUSERLIST) new SRUSERLIST;    // ALLOCATE STRUCTURES
	lpSRContext = (LPSRCONTEXT) new SRCONTEXT;
	lpSRContext->wAction = bPrevAct & ACTIONON;

}

TVAssist::TVAssist()
{
	msgTrainVoice = RegisterWindowMessage("TVASSIST_TRAINVOICE");
	gaUserAtomID = -1;
	bVAStatus = bFirstTime= TRUE;
	bTrainAction =bTrainVoice= bLink = FALSE;
	wCurTraining = 0; 								// for training voice
	bPrevAct = ACTIONON;
	bEnableButtons = TRUE;
	lpSRUserList =(LPSRUSERLIST) new SRUSERLIST;    // ALLOCATE STRUCTURES
	lpSRContext = (LPSRCONTEXT) new SRCONTEXT;
	lpSRContext->wAction = bPrevAct & ACTIONON;

}

TVAssist::TVAssist(int nAction,BOOL bEnableButton)
{
	TVAssist(NULL,nAction,bEnableButton);
}

TVAssist::~TVAssist(void)
{
	SRTerminateLink(hPrevVA);
	delete lpSRUserList;
	delete lpSRContext;

}


void TVAssist::ProcessNoApp()
{
}

void TVAssist::BecomingActive()
{
	int nError = SRNotify(hMain,TRUE); // enable message notification

	if(gaUserAtomID != LOWORD(lVAParam))  // no app file or different user
	{
		gaUserAtomID = LOWORD(lVAParam);
		if(!gaUserAtomID) // user file doesn't include  app file srt
		{
			ProcessNoApp();
		}
		bFirstTime = TRUE;
	}

	if(bFirstTime)
	{
		if(SRRequest(hMain,lpSRContext)==4) // tried to obtain commands
			return ;              // but training dialog was open
		BecomingActivePostRequest();  // placeholder function
		bFirstTime = FALSE;
	}
	VAEnableButtons(bEnableButtons);  // reenable button status inside VA

	if(bPrevAct & PREVACTION)     //restore previous action
	{
		 nError = VAActionOnOff(lpSRContext->wAction); 	// restore action state
	}
	else // set to bPreAct status
	{
		VAActionOnOff( bPrevAct & ACTIONON);
	}
	nError = SROnOff(hMain,0);  // turn off SR ... needed for enable command

	SREnableCommand(hMain,lpSRContext); // restore previous command status

	if(!nError)
	 SROnOff(hMain,TRUE);  // restore VA recognition status

	SetFocus(hMain);   // make sure this application has focus

}

void TVAssist::BecomingInActive( )
{
	SRRequest(hMain,lpSRContext); // save commands so they
															// can be restored later
}

LRESULT TVAssist::AppSwitch(WPARAM wP, LPARAM lP	)
{
	wVAParam = wP;
	lVAParam = lP;
	if(wP)
		BecomingActive();
	else
		BecomingInActive();
	return 0;
}

int TVAssist::VAInitializeLink(HWND hWnd)
{
	return SRInitializeLink(hWnd);
}
// The InitializeLink member functions are called from
// CmActivate(UINT active,BOOL minimize,HWND hOther)
void TVAssist::InitializeLink(HWND hWnd,UINT active)
{
	if(active != WA_INACTIVE  && !bLink)
	{
		hMain = hPrevVA = hWnd;
		 if((SRInitializeLink(hWnd)))
		 {
			bLink = FALSE;
		}
		else
			bLink = TRUE;
	}
}


void TVAssist::ReInitializeLink(HWND hWnd)
{
	bLink = FALSE;
	hMain = hPrevVA = hWnd;

	int nError = SRInitializeLink(hWnd);

	if(nError)
	{
		char szError[40];
		LoadString((HINSTANCE)GetWindowWord(hWnd,GWW_HINSTANCE),nError+500,
			(LPSTR)szError, 40);
		MessageBox(hWnd,szError, "Error",MB_OK);
		bLink = FALSE;
	}
	else
		bLink = TRUE;
}


LRESULT TVAssist::ActionStart(WPARAM wP, LPARAM lP)
{
	wVAParam = wP;
	lVAParam = lP;
	if(HIWORD(lP))
		ActionStartError();
	else
		ActionStartSuccess();
	return 0;
}


LRESULT  TVAssist::ActionStop( WPARAM wP, LPARAM lP)
{
	wVAParam = wP;
	lVAParam = lP;

	if(lP)
			ActionStopError();
	else
		if(bTrainAction)
			ActionStopTrainAction();  // done training action
		else
			ActionStopRecognition(); // done playing back macro
	  return 0;

}

void TVAssist::ActionStopTrainAction()
{
	SaveTrainAction();   // save action training (not automatically saved)
}

void TVAssist::SaveTrainAction(void)
{
	if(bTrainAction)
	{
		bTrainAction= FALSE;

		SRFreeze(hMain,FALSE);					// unfreeze the commands

		SROnOff(hMain,0); 						// turn off before calling SRSave

		SRSave(hMain, sSRWord.szUserName);

		SROpen();

		SROnOff(hMain,bVAStatus);  		// restore status of VA

		bVAStatus = FALSE;

		SetFocus(hMain);
	}

}

void TVAssist:: ActionStartError( )
{
	// called during recognition mode if there is a macro playback error
}

void TVAssist:: ActionStartSuccess()
{
	 // in recognition mode, the developer can call any routine at this time
	 // in training mode this will be called when the action training has begun
}


LRESULT TVAssist::SpeechStop(WPARAM wP, LPARAM lP)
{
	wVAParam = wP;
	lVAParam = lP;

	if(HIWORD(lP))
	{
		SpeechStopError();
		return 0;
	}
	else
		SpeechStopSuccess();

	return 0;
}

LRESULT TVAssist::SpeechStart(WPARAM wP,LPARAM lP )
{
	wVAParam = wP;
	lVAParam = lP;
	return 0;
}


void TVAssist::SpeechStopError()
{
	if(bTrainVoice)
		SpeechStopErrorTrainVoice();
	else
		SpeechStopErrorRecognition();
}

void TVAssist::SpeechStopErrorTrainVoice()
{
	//PostMessage(hMain,WM_VATRAINVOICE,0,0L);
	PostMessage(hMain,msgTrainVoice,0,0L);
}

void TVAssist::SpeechStopSuccessTrainVoice()
{
	wCurTraining--;
	PostMessage(hMain,msgTrainVoice,0,0L);
	//PostMessage(hMain,WM_VATRAINVOICE,0,0L);
}

void TVAssist::SpeechStopSuccess()
{
	if(bTrainVoice)
		SpeechStopSuccessTrainVoice();
	else
		SpeechStopSuccessRecognition();
}

void TVAssist::SpeechStopErrorRecognition()
{
	Confidence(NULL);
}

void TVAssist::SpeechStopSuccessRecognition()
{
	LPSCONFIDENCE pConfidMem;
	pConfidMem = ( LPSCONFIDENCE)GlobalLock((HGLOBAL)wVAParam );  // global memory block for recognized word
	Confidence(pConfidMem);
	GlobalUnlock((HGLOBAL)wVAParam);
}

void TVAssist::Confidence(LPSCONFIDENCE )
{
}

LRESULT TVAssist::LinkTerminated( WPARAM wP, LPARAM lP)
{
	wVAParam = wP;
	lVAParam = lP;
	bLink = FALSE;
	return 0;
}

int  TVAssist::VATerminateLink()
{
	return SRTerminateLink(hMain);
}

LRESULT TVAssist::Meter(WPARAM wP, LPARAM lP)
{
	wVAParam = wP;
	lVAParam = lP;

	if(bTrainVoice)
		MeterTrainMode();
	else
		MeterRecogMode();
	return 0;
}
void TVAssist::MeterTrainMode()
{
}

void TVAssist::MeterRecogMode()
{

}
int  TVAssist:: VAEnableCommand()
{
	return SREnableCommand(hMain,lpSRContext);
}
int  TVAssist::EnableCommand()
{
	int nError = SROnOff(hMain,0);
	int nReturn = SREnableCommand(hMain,lpSRContext);
	if(!nError)			// reinstate the VA's previous OnOff status
		SROnOff(hMain,TRUE);
	return nReturn;
}

// SetCommand Status - sets a range of commands to enabled or disabled
void TVAssist::SetCommandStatus(WORD wStart,WORD wEnd,BOOL bEnable)
{
	int i, nError;

	for(i=wStart;i<=wEnd;i++)
		lpSRContext->sCommInfo[i].wData = bEnable;

	nError = SROnOff(hMain,0);

	SREnableCommand(hMain,lpSRContext);

	if(!nError)
		SROnOff(hMain,1);
}

int TVAssist::VARequest()
{
	int nError= SRRequest(hMain,lpSRContext);
	return nError;
}
int TVAssist::VADeleteCommand()
{
	return  SRDeleteCommand(hMain, &sSRWord);
}
int TVAssist::DeleteCommand(WORD wIndex)
{
	int nError,nReturn;
	lstrcpy( sSRWord.szUserName, lpSRContext->szUserName );
	lstrcpy( sSRWord.szAppName, lpSRContext->szAppName );

	sSRWord.wIndex = wIndex;

	nError = SROnOff(hMain,0);

	if(!(nReturn = SRDeleteCommand(hMain, &sSRWord)))
		SRSave(hMain,sSRWord.szUserName);
	if(!nError)
		SROnOff(hMain,TRUE);
	return nReturn;
}

int TVAssist::VAAddCommand()
{
	return SRAddCommand(hMain,&sSRWord);
}

int TVAssist::AddCommand(LPSTR lpszCommand)
{
	int nError,nReturn;

	nError = SROnOff((hMain),0);

	lstrcpy( sSRWord.szUserName, lpSRContext->szUserName );
	lstrcpy( sSRWord.szAppName, lpSRContext->szAppName );
	lstrcpy(sSRWord.szCommName,lpszCommand);

	if(lpszCommand[0]== 0)
	{
			return UNKNOWNERROR;
	}
	nReturn = SRAddCommand(hMain, &sSRWord);

	if(!nReturn)
		SRSave(hMain,sSRWord.szUserName);

	if(!nError)
		SROnOff(hMain,TRUE);

	return nReturn;
}

int TVAssist::VARenameCommand()
{
	return SRRenameCommand(hMain, &sSRWord);
}

int TVAssist::RenameCommand(WORD wIndex, LPSTR lpszRename)
{
	int nError;

	nError = SROnOff(hMain,0);
	lstrcpy( sSRWord.szUserName, lpSRContext->szUserName );
	lstrcpy( sSRWord.szAppName, lpSRContext->szAppName );

	sSRWord.wIndex = wIndex;
	lstrcpy(sSRWord.szCommName,lpszRename);

	if(lpszRename[0] == 0)
	{
			return UNKNOWNERROR;
	}

	int nReturn  = SRRenameCommand(hMain, &sSRWord);

	if(nReturn)
		return nReturn;

	Save(sSRWord.szUserName);

	if(!nError)
		SROnOff(hMain,TRUE);

	return nReturn;

}

int TVAssist::VAEnableButtons(BOOL bEnableButtons)
{
	return SREnableButtons(hMain,bEnableButtons);
}

int TVAssist::VAActionOnOff(BOOL bAction)
{
	lpSRContext->wAction = bAction;
	return SRActionOnOff(hMain,bAction);
}


int TVAssist::VAOnOff(BOOL bOnOff)
{
	return SROnOff(hMain,bOnOff);
}

int TVAssist::VAAbortVoiceTraining()
{
	return SRAbortVoiceTraining(hMain);
}

int TVAssist::VASave(LPSTR lpszUserName)
{
	return SRSave(hMain,lpszUserName);
}

int TVAssist::Save(LPSTR lpszUserName)
{
	int nError = SROnOff(hMain,FALSE);
	int nReturn =   SRSave(hMain,lpszUserName);

	if(!nError)
		SROnOff(hMain,TRUE);
	return nReturn;
}

int TVAssist::VAGetUserList()
{
	return SRGetUserList( hMain, lpSRUserList);
}


int TVAssist::VAChangeUser(LPSTR lpszUserName)
{
	return  SRChangeUser( hMain,lpszUserName );
}

int TVAssist::ChangeUser(LPSTR lpszUserName)
{
	lstrcpy( sSRWord.szUserName,lpszUserName );

	int nError = SROnOff(hMain,0); 				// turn driver off before change user
	int nReturn = SRChangeUser( hMain,lpszUserName );

	if(nReturn)
	{
		if(!nError)
			SROnOff(hMain,1);
		return nReturn;
	}
	SRRequest(hMain,lpSRContext);

	if(!nError)
		SROnOff(hMain,1);
	return 0;
}

int TVAssist::VAFreeze(BOOL bFreeze)
{
	return SRFreeze(hMain,bFreeze);
}

int TVAssist::VANotify(BOOL bNotify)
{
	return  SRNotify(hMain, bNotify);
}

int TVAssist::VATrainAction( )
{
	return SRTrainAction(hMain, &sSRWord);
}

int TVAssist::TrainAction(int nIndex)
{
	int nError;

	lstrcpy( sSRWord.szUserName, lpSRContext->szUserName );
	lstrcpy( sSRWord.szAppName, lpSRContext->szAppName );

	sSRWord.wIndex = nIndex;

	(lpSRContext->wOnOff) ? (bVAStatus = TRUE) : (bVAStatus = FALSE);

	SRFreeze(hMain,TRUE); 				// freeze the command set
	sSRWord.bNew = 1;
	nError =  SRTrainAction(hMain, &sSRWord);

	if(nError)
		return nError;
			bTrainAction = TRUE;
		return 0;
}

// DisplayCommand is used in TrainVoice function
void TVAssist::DisplayCommand(WORD wIndexCommand,WORD wIDDisplayComd)
{
	SetDlgItemText(hMain,wIDDisplayComd,
		 (LPSTR) lpSRContext->sCommInfo[wIndexCommand].szCommName);
}

/*
	TrainVoice( )

	Purpose:
		Dialog or window to train voice

	Params:

		hWnd - Window Handle
		wCmdIndex - Index of command from which to start training
		nTrainings - Number of trainings per command
		bSingleOrAll - bSingleAll=1 just to train 1 command
						bSingle = 0 to train all commands from present index
		wDisplayComd - Edit or static control which holds name of command
*/

int TVAssist::VATrainVoice()
{
	return SRTrainVoice(hMain, &sSRWord);
}
int TVAssist::TrainVoice(WORD wStartIndex,WORD nTrainings=1,
				BOOL bSingle=TRUE,WORD wDisplayComd=0)
{
	int nError;
	static int nCurIndex;

	if(!bTrainVoice)
	{
		lstrcpy( sSRWord.szUserName, lpSRContext->szUserName );
		lstrcpy( sSRWord.szAppName, lpSRContext->szAppName );

		wCurTraining = nTrainings;
		bTrainVoice = TRUE;
		nCurIndex = wStartIndex;
	}


	if(wCurTraining)
	{
		if(nTrainings>1)   // more than 1 number of voice training
		{

			if(wCurTraining == nTrainings)
			{
				sSRWord.wIndex = wStartIndex;
				sSRWord.bNew = 1;
			}
			else
			{
				sSRWord.bNew = 0;
			}
		}
		else if (nTrainings == 1)
		{
			sSRWord.wIndex = wStartIndex;

			sSRWord.bNew = 1;
		}
	}
	else // finished training the voice for a command
	{
		if(!bSingle && (nCurIndex < lpSRContext->wNumCommands-1))
		{
			wCurTraining = nTrainings;
			nCurIndex++;
			wStartIndex = nCurIndex;
			DisplayCommand(nCurIndex,wDisplayComd);
			sSRWord.wIndex = nCurIndex;
			sSRWord.bNew = 1;
		}
		else  // finish voice training
		{
			wCurTraining = 0;
			bTrainVoice = FALSE;
			FinishTrainVoice();
			return 0;
		}
	}

	nError = SRTrainVoice(hMain, &sSRWord);

	if(nError)
	{
		wCurTraining = 0;
		bTrainVoice = FALSE;
		nCurIndex = 0;
	}
	return nError;
}

BOOL far _export AddVADlg(HWND hWnd,HWND hWndChild,TVAssist* pParent)
{
	TWindow* ParentWindow = new TWindow(hWnd,VALib);

	TVADllAddRename  *AddVAObject ;
	AddVAObject = new TVADllAddRename(ADD,0,pParent,ParentWindow,IDD_ADDRENAME,VALib);

	if(AddVAObject)
	{
		AddVAObject->Execute();
		delete AddVAObject;
		delete ParentWindow;

		if(pParent)
		{
			if(hWndChild)
				pParent->ReInitializeLink(hWndChild);
			else
				pParent->ReInitializeLink(hWnd);
		}
		return TRUE;
	}
	else
	{
		if(pParent)
		{
			delete ParentWindow;
			if(hWndChild)
				pParent->ReInitializeLink(hWndChild);
			else
				pParent->ReInitializeLink(hWnd);
		}
		return FALSE;
	}
}

BOOL far _export RenameVADlg(HWND hWnd,HWND hWndChild,TVAssist* pParent, WORD wIndex)
{
	TWindow* ParentWindow = new TWindow(hWnd,VALib);

	TVADllAddRename  *RenameVAObject ;
	RenameVAObject = new TVADllAddRename(RENAME,wIndex,pParent,ParentWindow,IDD_ADDRENAME,VALib);

	if(RenameVAObject)
	{
		RenameVAObject->Execute();
		delete RenameVAObject;
		delete ParentWindow;

		if(pParent)
		{
			if(hWndChild)
				pParent->ReInitializeLink(hWndChild);
			else
				pParent->ReInitializeLink(hWnd);
		}
		return TRUE;
	}
	else
	{
		delete ParentWindow;

		if(pParent)
		{
			if(hWndChild)
				pParent->ReInitializeLink(hWndChild);
			else
				pParent->ReInitializeLink(hWnd);
		}
		return FALSE;
	}
}
BOOL far _export DeleteVADlg(HWND hWnd,HWND hWndChild,TVAssist* pParent)
{
	TWindow* ParentWindow = new TWindow(hWnd,VALib);

	TVADlgDelete  *DlgDeleteVAObject ;
	DlgDeleteVAObject = new TVADlgDelete(pParent,ParentWindow,IDD_DELETE,VALib);

	if(DlgDeleteVAObject)
	{
		DlgDeleteVAObject->Execute();
		delete DlgDeleteVAObject;
		delete ParentWindow;

		if(pParent)
		{
			if(hWndChild)
				pParent->ReInitializeLink(hWndChild);
			else
				pParent->ReInitializeLink(hWnd);
		}
		return TRUE;
	}
	else
	{

		delete ParentWindow;

		if(pParent)
		{
			if(hWndChild)
				pParent->ReInitializeLink(hWndChild);
			else
				pParent->ReInitializeLink(hWnd);
		}
		return FALSE;
	}
}

BOOL far _export TrainVoiceVADlg(HWND hWnd,HWND hWndChild,TVAssist* pParent,BOOL bSingleTraining, WORD wIndex)
{
	TWindow* ParentWindow = new TWindow(hWnd,VALib);

	TVADllTrVoiceDlg  *TrVoiceVAObject ;
	TrVoiceVAObject = new TVADllTrVoiceDlg(pParent,ParentWindow,wIndex,bSingleTraining);

	if(TrVoiceVAObject)
	{
		TrVoiceVAObject->Execute();
		delete TrVoiceVAObject;
		delete ParentWindow;

		if(pParent)
		{
			if(hWndChild)
				pParent->ReInitializeLink(hWndChild);
			else
				pParent->ReInitializeLink(hWnd);
		}
		return TRUE;
	}
	else
	{
		delete ParentWindow;

		if(pParent)
		{
			if(hWndChild)
				pParent->ReInitializeLink(hWndChild);
			else
				pParent->ReInitializeLink(hWnd);
		}
		return FALSE;
	}
}
int FAR PASCAL LibMain(HINSTANCE hInst, WORD /*DS*/, WORD /*heapSz*/, LPSTR cmdLn)
{

	VALib = new TModule("vadll",hInst,cmdLn);

	if(VALib->Status)
	{
		delete VALib;
		VALib = NULL;
	}
	return TRUE;
}

