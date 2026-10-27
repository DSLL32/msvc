/*********************************************************
						Copyright Creative Technology Ltd, 1993
								All Right Reserved
MODULE: dlltrvoi.CPP
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
#include "tvassist.h"
#include "vadllrc.h"            // Definition of all resources.

#include "dlltrvoi.h"

extern TModule* VALib;

void HideOkButton(HWND hOk,HWND hCancel)
{
	int i;
	if(IsWindowEnabled(hOk))
	{
		EnableWindow(hOk,FALSE);
		ShowWindow(hOk,SW_HIDE);
		ShowWindow(hCancel,SW_SHOWNORMAL);
		for (i=0;i<3;i++)
			EnableWindow(GetDlgItem(GetParent(hOk),IDC_TR1+i),FALSE);
	}
}

class TVADllVoice: public TVAssist
{
	public:
		WORD wStartIndex;
		TVADllVoice(TVAssist* pParent,WORD wIndex);
		virtual void SpeechStopSuccessTrainVoice();
		virtual void SpeechStopErrorTrainVoice();
		virtual void BecomingActivePostRequest();
		virtual void DisplayCommand(WORD,WORD);
		virtual void FinishTrainVoice(void);
};

TVADllVoice::TVADllVoice(TVAssist* pParent,WORD wIndex):TVAssist(pParent,ACTIONOFF,FALSE)
{
	 wStartIndex =wIndex;
}

void TVADllVoice::SpeechStopSuccessTrainVoice()
{
	TVAssist::SpeechStopSuccessTrainVoice();
	SendDlgItemMessage(hMain,IDC_TR1+wCurTraining,BM_SETCHECK,0,0L); // button setcheck
	SendDlgItemMessage(hMain,IDC_TR1+wCurTraining-1,BM_SETCHECK,1,0L); // button setcheck
}

void TVADllVoice::SpeechStopErrorTrainVoice()
{
	char szBuf[150];
	LoadString((HINSTANCE)GetWindowWord(hMain,GWW_HINSTANCE)
					,HIWORD(lVAParam)+400,(LPSTR)szBuf,150);
	MessageBox(hMain,szBuf,"Error", MB_OK);
	TVAssist::SpeechStopErrorTrainVoice();
}

void TVADllVoice::FinishTrainVoice()
{
	Save(sSRWord.szUserName);
	PostMessage(hMain,WM_COMMAND,IDCANCEL,0L);
}

void TVADllVoice::DisplayCommand(WORD nIndexCommand,WORD nIDDisplayComd)
{
	TVAssist::DisplayCommand(nIndexCommand,nIDDisplayComd); // display a command
	SendDlgItemMessage(hMain,IDC_TR1+wCurTraining-1,BM_SETCHECK,1,0L); // button setcheck
}

void TVADllVoice::BecomingActivePostRequest()
{
	SetDlgItemText(hMain,IDC_COMMAND,
		 (LPSTR) lpSRContext->sCommInfo[wStartIndex].szCommName); // display a command
	SendDlgItemMessage(hMain,IDC_TR1,BM_SETCHECK,1,0L); 				// setcheck at IDC_TR1 button
}

DEFINE_RESPONSE_TABLE1(TVADllTrVoiceDlg, TDialog)
//{{TVADllTrVoiceDlgRSP_TBL_BEGIN}}
		EV_WM_ACTIVATE,
		EV_COMMAND(IDOK,CmOk),
		EV_COMMAND(IDCANCEL,CmCancel),
		EV_COMMAND(IDC_TR1,CmTr1),
		EV_COMMAND(IDC_TR2,CmTr2),
		EV_COMMAND(IDC_TR3,CmTr3),
		EV_REGISTERED("TVASSIST_TRAINVOICE",EvTrainVoice),
		EV_REGISTERED("APP_SWITCH",EvAppSwitch),     // register 3 messages for VA API
		EV_REGISTERED("LINK_TERMINATED",EvLinkTerminated),
		EV_REGISTERED("SPEECH_STOP",EvSpeechStop),
//{{TVADllTrVoiceDlgRSP_TBL_END}}
END_RESPONSE_TABLE;
//{{TVADllTrVoiceDlg Implementation}}

LRESULT TVADllTrVoiceDlg::EvTrainVoice(WPARAM wP,LPARAM lP)
{
	int nError = VAVoiceDlg->TrainVoice(wStartIndex,wNumTrains,
			bSingleOrAll,(WORD)IDC_COMMAND);
	if(nError)
	{
		char szError[150];
		LoadString((HINSTANCE)GetWindowWord(GWW_HINSTANCE),nError,(LPSTR)szError,sizeof(szError));
		MessageBox(szError,"Error",MB_OK);
		PostMessage(WM_COMMAND,IDCANCEL,0L);
		return 0;
	}
	return 0;
}

TVADllTrVoiceDlg::TVADllTrVoiceDlg (TVAssist* pParent,TWindow* parent,WORD wIndex,BOOL bSingle):
	 TDialog(parent, IDD_TRVOICE,VALib)
{
	 // Change the window's background color
	 // <<Your Code is here>>
	 VAVoiceDlg = new TVADllVoice(pParent,wIndex);
	 wStartIndex = wIndex;
	 bSingleOrAll = bSingle;
	 wNumTrains =1;
	 bFirstOkClick = TRUE;
}



TVADllTrVoiceDlg::~TVADllTrVoiceDlg ()
{
	 delete VAVoiceDlg;
	 Destroy();

}

void TVADllTrVoiceDlg::CmCancel()
{
	VAVoiceDlg->VAAbortVoiceTraining();
	TDialog::CmCancel();
}void TVADllTrVoiceDlg::CmOk(){
	HideOkButton((HWND)GetDlgItem(IDOK),(HWND)GetDlgItem(IDCANCEL));
	EvTrainVoice(0,0);
}
void TVADllTrVoiceDlg::EvActivate (UINT active, BOOL minimized, HWND hWndOther )
{
	 // <<Your Code is here>>
	TWindow::EvActivate(active,minimized,hWndOther);
	if(active)
		VAVoiceDlg->InitializeLink(HWindow,active);

}

LRESULT TVADllTrVoiceDlg::EvAppSwitch(WPARAM wP,LPARAM lP)
{
	return VAVoiceDlg->AppSwitch(wP,lP);
}

LRESULT TVADllTrVoiceDlg::EvLinkTerminated(WPARAM wP,LPARAM lP)
{
	return VAVoiceDlg->LinkTerminated(wP,lP);
}

LRESULT TVADllTrVoiceDlg::EvSpeechStop(WPARAM wP,LPARAM lP)
{
	return VAVoiceDlg->SpeechStop(wP,lP);
}
