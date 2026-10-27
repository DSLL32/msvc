/*********************************************************
						Copyright Creative Technology Ltd, 1993
								All Right Reserved
MODULE:	TVAUser.cpp
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

#include <owl\dialog.h>
#include <tvassist.h>
#include "testapp.rh"            // Definition of all resources.

#include "tvauser.h"

class TUser: public TVAssist
{
	public:
		TUser(TVAssist* pParent):TVAssist(pParent,ACTIONOFF,FALSE){};
		void BecomingActivePostRequest();
		void GetUser();

} ;
void TUser::BecomingActivePostRequest()
{

	TVAssist::BecomingActivePostRequest();
	TUser::GetUser();
	
}
void TUser::GetUser()
{
	int i;
	if(! VAGetUserList() )  // member function of VAs class
	{
		SendDlgItemMessage(hMain,IDC_USERLB,CB_RESETCONTENT,0,0L);

		for( i=0; i<lpSRUserList->wNumUsers; i++)
		{
				SendDlgItemMessage(hMain,IDC_USERLB,CB_ADDSTRING,0,
						(DWORD)lpSRUserList->szTableName[i].szUserName);
		}
		WORD nCurSel = (WORD) SendDlgItemMessage(hMain,IDC_USERLB,CB_FINDSTRING,-1,(DWORD)lpSRContext->szUserName);

		if( nCurSel != (WORD) CB_ERR)
			 SendDlgItemMessage(hMain,IDC_USERLB,CB_SETCURSEL,(WORD)nCurSel,0L);
	}
	else
	{
		MessageBox(hMain,"function VAGetUserList returns an error","Error",
						MB_OK);
   }
   
}
DEFINE_RESPONSE_TABLE1(TVAUser, TDialog)
//{{TVAUserRSP_TBL_BEGIN}}
	 EV_COMMAND(IDOK,CmOk),
	 EV_REGISTERED("APP_SWITCH",EvAppSwitch),
	 EV_REGISTERED("LINK_TERMINATED",EvLinkTerminated),
	 EV_WM_ACTIVATE,

//{{TVAUserRSP_TBL_END}}
END_RESPONSE_TABLE;


//{{TVAUser Implementation}}


TVAUser::TVAUser (TVAssist* pParent,TWindow* parent,TResId ResId, TModule* module):
	 TDialog(parent,ResId,module)
{
	 VAUser = new TUser(pParent);
	 // <<Your Code is here>>
}


TVAUser::~TVAUser ()
{
	 delete VAUser;
    Destroy();

	 // <<Your Code is here>>
}


void TVAUser::EvActivate(UINT active,BOOL minized,HWND hWndOther)
{
	TWindow::EvActivate(active,minized,hWndOther);
	if(active)
		VAUser->InitializeLink(HWindow,active);
}

LRESULT TVAUser::EvAppSwitch(WPARAM wP,LPARAM lP)
{
	return VAUser->AppSwitch(wP,lP);
}
LRESULT TVAUser::EvLinkTerminated(WPARAM wP,LPARAM lP)
{
	return VAUser->LinkTerminated(wP,lP);
}
void TVAUser::CmOk()
{
	WORD wIndex;
	char szBuffer[40];
	
	if((wIndex =(WORD)SendDlgItemMessage(IDC_USERLB,CB_GETCURSEL,0,0L)) !=(WORD)CB_ERR)
	{
		SendDlgItemMessage(IDC_USERLB,CB_GETLBTEXT,wIndex,(DWORD)(LPSTR)szBuffer);
		if(!lstrcmp((LPSTR)szBuffer,(LPSTR)(VAUser->lpSRContext)->szUserName) )
		{
			TDialog::CmOk();
			return;
		}
		int nError = VAUser->ChangeUser((LPSTR)szBuffer);
		if(nError)
		{
			char szBuffer[30];
			wsprintf((LPSTR)szBuffer,"Error in Change user %d",nError);
		}
		//Request();		
	} 
	
	TDialog::CmOk();
}

