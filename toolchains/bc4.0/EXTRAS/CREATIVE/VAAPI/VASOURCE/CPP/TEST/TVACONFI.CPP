/*********************************************************
						Copyright Creative Technology Ltd, 1993
								All Right Reserved
MODULE:	TVAConfi.cpp
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

#include "TVAConfid.h"
class TConfid: public TVAssist
{
	public:
		TConfid(TVAssist* pParent):TVAssist(pParent,ACTIONOFF,FALSE){};
		virtual void Confidence(LPSCONFIDENCE pConfidMem);
};
void TConfid:: Confidence(LPSCONFIDENCE pConfidMem)
{
	char szCommand[80],szScore[80],szTemp[32];
	int i;

	if(!pConfidMem)  // speech stop returns an error, pConfidMem = NULL;
	{
		SetDlgItemText(hMain,IDC_COMMAND,(LPSTR)"Command:\n( Not recognized )");
		SetDlgItemText(hMain,IDC_SCORE,(LPSTR)"Score:\n -1 ");
		return ;
	}
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
	SetDlgItemText(hMain,IDC_COMMAND,(LPSTR)szCommand);
	SetDlgItemText(hMain,IDC_SCORE,(LPSTR)szScore);


}

DEFINE_RESPONSE_TABLE1(TVAConfid, TDialog)
//{{TVAConfidRSP_TBL_BEGIN}}
	 EV_REGISTERED("APP_SWITCH",EvAppSwitch),
	 EV_REGISTERED("LINK_TERMINATED",EvLinkTerminated),
	 EV_REGISTERED("SPEECH_STOP",EvSpeechStop),
	 EV_WM_ACTIVATE,

//{{TVAConfidRSP_TBL_END}}
END_RESPONSE_TABLE;


//{{TVAConfid Implementation}}


TVAConfid::TVAConfid (TVAssist* pParent,TWindow* parent,TResId ResId, TModule* module):
	 TDialog(parent,ResId,module)
{
	 VAConfid = new TConfid(pParent);
	 // <<Your Code is here>>
}


TVAConfid::~TVAConfid ()
{
	 VAConfid->VATerminateLink();
	 delete VAConfid;
    Destroy();

	 // <<Your Code is here>>
}


void TVAConfid::EvActivate(UINT active,BOOL minized,HWND hWndOther)
{
	TWindow::EvActivate(active,minized,hWndOther);
	if(active)
		VAConfid->InitializeLink(HWindow,active);
}

LRESULT TVAConfid::EvAppSwitch(WPARAM wP,LPARAM lP)
{
	return VAConfid->AppSwitch(wP,lP);
}
LRESULT TVAConfid::EvLinkTerminated(WPARAM wP,LPARAM lP)
{
	return VAConfid->LinkTerminated(wP,lP);
}
LRESULT TVAConfid::EvSpeechStop(WPARAM wP,LPARAM lP)
{
	return VAConfid->SpeechStop(wP,lP);
}

