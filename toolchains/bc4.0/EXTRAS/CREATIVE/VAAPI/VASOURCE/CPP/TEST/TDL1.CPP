/*********************************************************
						Copyright Creative Technology Ltd, 1993
								All Right Reserved
MODULE: 	TDl1.cpp
PROGRAMMER:	Nam Hoang								
*********************************************************/
#include <owl\owldefs.h>
#include <owl\version.h>

#include <owl\module.h>
#include <owl\applicat.h>

#include <owl\dc.h>
#include <owl\menu.h>

#include <owl\window.h>
#include <owl\framewin.h>
#include <tvassist.h>
#include "tdl1.h"

void TVAControl::ShowComdInListBox(HWND hWnd)
{
	int i;
	char szBuffer[32];
	SendDlgItemMessage(hWnd,IDC_COMMAND,LB_RESETCONTENT,0,0L);

	for( i=0; i<(lpSRContext)->wNumCommands; i++)
	{
		wsprintf( szBuffer, " %s",(LPSTR) (lpSRContext)->sCommInfo[i].szCommName);
		SendDlgItemMessage(hWnd,IDC_COMMAND,LB_ADDSTRING,0,(DWORD)(LPSTR)szBuffer);
	}

}

void TVAControl::BecomingActivePostRequest()
{
	TVAssist::BecomingActivePostRequest();
	ShowComdInListBox(hMain);
}

//{{TVACommand Implementation}}

DEFINE_RESPONSE_TABLE1(TVACommand,TDialog)
	EV_COMMAND(IDC_ADD,EvAddVACommand),
	EV_COMMAND(IDC_RENAME,EvRenameVACommand),
	EV_COMMAND(IDC_DELETE,EvDeleteVACommand),
	EV_COMMAND(IDC_TRACTION,EvTrainingAction),
	EV_COMMAND(IDC_TRSINGLE,EvSingle),
	EV_COMMAND(IDC_TRALL,EvAll),
	EV_COMMAND(IDC_CLOSE,CmOk),
	EV_REGISTERED("APP_SWITCH",EvAppSwitch),
	EV_REGISTERED("LINK_TERMINATED",EvLinkTerminated),
	EV_REGISTERED("ACTION_START",EvActionStart),
	EV_REGISTERED("ACTION_STOP",EvActionStop),
	EV_WM_ACTIVATE,
END_RESPONSE_TABLE;

TVACommand::TVACommand (TVAssist* pParent,TWindow* parent, TResId resId, TModule* module):
	 TDialog(parent, resId, module)
{
	 VAControl = new TVAControl(pParent);

}

LRESULT TVACommand::EvActionStart(WPARAM wP,LPARAM lP)
{
	return VAControl->ActionStart(wP,lP);
}

LRESULT TVACommand::EvActionStop(WPARAM wP,LPARAM lP)
{
	return VAControl->ActionStop(wP,lP);
}

TVACommand::~TVACommand ()
{
	delete VAControl;
	Destroy();
}

void TVACommand::EvActivate(UINT active,BOOL minized,HWND hWndOther)
{

	TWindow::EvActivate(active,minized,hWndOther);
	if(active)
	{
		VAControl->InitializeLink(HWindow,active);
	}
}

LRESULT TVACommand::EvAppSwitch(WPARAM wP,LPARAM lP)
{
	return VAControl->AppSwitch(wP,lP);
}

LRESULT TVACommand::EvLinkTerminated(WPARAM wP,LPARAM lP)
{
	return VAControl->LinkTerminated(wP,lP);
}

void TVACommand::EvRenameVACommand()
{
	int nIndexSel;
	if((nIndexSel =(int)SendDlgItemMessage(IDC_COMMAND,LB_GETCURSEL,0,0L)) != LB_ERR)
	{
		RenameVADlg((HWND)GetParent(),HWindow,VAControl,nIndexSel);
		VAControl->VARequest();
		VAControl->ShowComdInListBox(HWindow);
	}
}

void TVACommand::EvDeleteVACommand()
{
	DeleteVADlg((HWND)GetParent(),HWindow,VAControl);
	VAControl->VARequest();
	VAControl->ShowComdInListBox(HWindow);
}

void TVACommand::EvAddVACommand()
{
	AddVADlg((HWND)GetParent(),HWindow,VAControl);
	VAControl->VARequest();
	VAControl->ShowComdInListBox(HWindow);
}


void TVACommand::EvSingle()
{
	int nIndexSel;
	if((nIndexSel =(int) SendDlgItemMessage(IDC_COMMAND,LB_GETCURSEL,0,0L)) != LB_ERR)
	{
		TrainVoiceVADlg((HWND)GetParent(),HWindow,VAControl,TRUE,nIndexSel); // training a 34th command  by using vadll dlg
	}
	VAControl->VARequest();
	VAControl->ShowComdInListBox(HWindow);

}

void TVACommand::EvAll()
{
	int nIndexSel;
	if((nIndexSel =(int) SendDlgItemMessage(IDC_COMMAND,LB_GETCURSEL,0,0L)) != LB_ERR)
	{
		TrainVoiceVADlg((HWND)GetParent(),HWindow,VAControl,FALSE,nIndexSel); // training a 34th command  by using vadll dlg
	}
	else
		TrainVoiceVADlg((HWND)GetParent(),HWindow,VAControl,FALSE,0); // training a 34th command  by using vadll dlg
	VAControl->VARequest();
	VAControl->ShowComdInListBox(HWindow);

}

void TVACommand::EvTrainingAction()
{
	WORD wIndexSel = (WORD)SendDlgItemMessage(IDC_COMMAND,LB_GETCURSEL,0,0L);

	if(wIndexSel ==(WORD) LB_ERR || wIndexSel <= (WORD)31 )  // list box returns an error
		return ;
	if( VAControl->TrainAction(wIndexSel))
			MessageBox(" Action training error!  ","Error",MB_OK);

}

