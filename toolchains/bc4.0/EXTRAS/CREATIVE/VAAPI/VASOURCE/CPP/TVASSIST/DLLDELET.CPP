/*********************************************************
						Copyright Creative Technology Ltd, 1993
								All Right Reserved
MODULE: dlldelet.CPP
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
#include "tvassist.h"
#include "vadllrc.h"
#include "dlldelet.h"

void TVADelete::ShowComdInListBox(HWND hWnd)
{
	int i;
	char szBuffer[32];
	SendDlgItemMessage(hWnd,IDC_DELETE,LB_RESETCONTENT,0,0L);

	for( i=32; i<(lpSRContext)->wNumCommands; i++)
	{
		wsprintf( szBuffer, " %s",(LPSTR) (lpSRContext)->sCommInfo[i].szCommName);
		SendDlgItemMessage(hWnd,IDC_DELETE,LB_ADDSTRING,0,(DWORD)(LPSTR)szBuffer);
	}

}

void TVADelete::BecomingActivePostRequest()
{
	TVAssist::BecomingActivePostRequest();
	ShowComdInListBox(hMain);
}

//{{TVACommand Implementation}}

DEFINE_RESPONSE_TABLE1(TVADlgDelete,TDialog)
	//EV_COMMAND(IDC_CLOSE,CmOk),
	EV_COMMAND(IDOK,CmOk),
	EV_COMMAND(IDCANCEL,CmCancel),
	EV_REGISTERED("APP_SWITCH",EvAppSwitch),
	EV_REGISTERED("LINK_TERMINATED",EvLinkTerminated),
	EV_WM_ACTIVATE,
END_RESPONSE_TABLE;

TVADlgDelete::TVADlgDelete (TVAssist* pParent,TWindow* parent, TResId resId, TModule* module):
	 TDialog(parent, resId, module)
{
	 // Change the window's background color
	 //SetBkgndColor(0x08ffffffL);
	 VADeleteObject = new TVADelete(pParent);
	 // <<Your Code is here>>
}

TVADlgDelete::~TVADlgDelete ( )
{
	VADeleteObject->VATerminateLink();
	delete VADeleteObject;
	Destroy();
}

void TVADlgDelete::EvActivate(UINT active,BOOL minized,HWND hWndOther)
{
	TWindow::EvActivate(active,minized,hWndOther);
	if(active)
		VADeleteObject->InitializeLink(HWindow,active);
}

LRESULT TVADlgDelete::EvAppSwitch(WPARAM wP,LPARAM lP)
{
	return VADeleteObject->AppSwitch(wP,lP);
}

LRESULT TVADlgDelete::EvLinkTerminated(WPARAM wP,LPARAM lP)
{
	return VADeleteObject->LinkTerminated(wP,lP);
}

void TVADlgDelete::CmOk()
{
	int i, nVAOnOrOff;
	int nNumSelItems[989];
	DWORD dwReturn = SendDlgItemMessage(IDC_DELETE,LB_GETSELITEMS,989,
							(DWORD)nNumSelItems);

	if(dwReturn !=(DWORD) LB_ERR)
	{
		lstrcpy( (VADeleteObject->sSRWord).szUserName, VADeleteObject->lpSRContext->szUserName );
		lstrcpy( (VADeleteObject->sSRWord).szAppName, VADeleteObject->lpSRContext->szAppName );
		nVAOnOrOff = VADeleteObject->VAOnOff(FALSE);

		for(i=0;i<dwReturn;i++)
		{
			(VADeleteObject->sSRWord).wIndex = nNumSelItems[i]+32-i;
				
			int nError = SRDeleteCommand(VADeleteObject->hMain,
								 &(VADeleteObject->sSRWord));
			if(nError)
			{
				ErrorMessage(nError+300);
				if(!nVAOnOrOff)
					VADeleteObject->VAOnOff(TRUE);
				VADeleteObject->VATerminateLink();
				TDialog::CmOk();
				return;
			}
		}
		VADeleteObject->Save((VADeleteObject->sSRWord).szUserName);

		if(!nVAOnOrOff)
			VADeleteObject->VAOnOff(TRUE);

	  VADeleteObject->VATerminateLink();
	  TDialog::CmOk();
	}
}

void TVADlgDelete::ErrorMessage(int nError)
{
	char szError[150];
	LoadString((HINSTANCE)GetWindowWord(GWW_HINSTANCE),nError,(LPSTR)szError,sizeof(szError));
	MessageBox(szError,"Error",MB_OK);
}

void TVADlgDelete::CmCancel()
{
	  VADeleteObject->VATerminateLink();
	  TDialog::CmCancel();
}




