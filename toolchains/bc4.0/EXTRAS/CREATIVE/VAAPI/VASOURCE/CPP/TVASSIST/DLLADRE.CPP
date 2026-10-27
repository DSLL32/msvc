/*********************************************************
						Copyright Creative Technology Ltd, 1993
								All Right Reserved
MODULE: dlladre.CPP
PROGRAMMER:	Nam Hoang								
*********************************************************/
#include <owl\owldefs.h>
#include <owl\version.h>

#include <owl\module.h>
#include <owl\applicat.h>

#include <owl\dc.h>

#include <owl\window.h>
#include <owl\framewin.h>

#include <owl\dialog.h>
#include "tvassist.h"
#include "vadllrc.h"            // Definition of all resources.
#include "dlladre.h"

class TVADllCmd: public TVAssist
{
	public:
		BOOL bRename;
		int  nRenameIndex;
		TVADllCmd(TVAssist* pParent,int AddOrRename,int nIndex);
		void BecomingActivePostRequest();
};

TVADllCmd::TVADllCmd(TVAssist* pParent,int AddOrRename,int nIndex)
	:TVAssist(pParent,ACTIONOFF,FALSE)
{
	if(AddOrRename == RENAME)
	{
		bRename = TRUE;
		nRenameIndex = nIndex;
	}
	else
		bRename = FALSE;
}

void TVADllCmd::BecomingActivePostRequest()
{
	if(bRename) // display a command in static text control
		SetDlgItemText(hMain,IDC_EDITCOMMAND,
			(LPSTR)lpSRContext->sCommInfo[nRenameIndex].szCommName);
}
//{{TVADllAddRename Implementation}}

DEFINE_RESPONSE_TABLE1(TVADllAddRename,TDialog)
	EV_COMMAND(IDOK,CmOk),
	EV_COMMAND(IDCANCEL,CmCancel),
	EV_REGISTERED("APP_SWITCH",EvAppSwitch),
	EV_REGISTERED("LINK_TERMINATED",EvLinkTerminated),
	EV_WM_ACTIVATE,
END_RESPONSE_TABLE;


TVADllAddRename::TVADllAddRename (int ADDorRENAME,WORD wIndex,
			TVAssist* pParent,TWindow* parent, TResId resId, TModule* module):
	 TDialog(parent, resId, module)
{
	 // create VA Object
	 AddRename = new TVADllCmd(pParent,ADDorRENAME,wIndex);
	 nAddOrRename = ADDorRENAME;

	 wRenameIndex = wIndex;

}


TVADllAddRename::~TVADllAddRename ()
{
	delete AddRename; // delete TVAssist object
	Destroy();

}
void TVADllAddRename::ErrorMessage(int nError)
{
	char szError[150];
	LoadString((HINSTANCE)GetWindowWord(GWW_HINSTANCE),nError,(LPSTR)szError,sizeof(szError));
	MessageBox(szError,"Error",MB_OK);
	switch(nError%100)
	{
		case 1:  // invalid window
		case 2:  // invalid pointer to SRWord struct
		case 3:  // a link does not exist
		case 4:  // user or application name doesn't match with current name
		case 6:  // recognition is on
		case 7:  //user or traindlg is on
		case 16: // out of memory
			TDialog::CmOk();
			break;
	}
}
void TVADllAddRename::CmOk()
{
	char szAddRename[32];
	int nError;
	GetDlgItemText(IDC_EDITCOMMAND,(LPSTR)szAddRename,sizeof(szAddRename));
	if(nAddOrRename == ADD)
	{
		nError = AddRename->AddCommand(szAddRename);
		if(nError)
		{
			ErrorMessage(nError+ADD*100);
			return;
		}

	}
	else
	{
		nError = AddRename->RenameCommand(wRenameIndex,szAddRename);
		if(nError)
		{
			ErrorMessage(nError+RENAME*100);
			return;
		}
	}
	TDialog::CmOk();
}

BOOL TVADllAddRename::CanClose()
{
	//AddRename->VATerminateLink();
	return TWindow::CanClose();
}

void TVADllAddRename::EvActivate(UINT active,BOOL minized,HWND hWndOther)
{
	TWindow::EvActivate(active,minized,hWndOther);
	if(active)
	{
		if(nAddOrRename ==RENAME)
			SetWindowText("Rename Command");
		AddRename->InitializeLink(HWindow,active);
	}
}

LRESULT TVADllAddRename::EvAppSwitch(WPARAM wP,LPARAM lP)
{
	return AddRename->AppSwitch(wP,lP);

}
LRESULT TVADllAddRename::EvLinkTerminated(WPARAM wP,LPARAM lP)
{
	return AddRename->LinkTerminated(wP,lP);
}


