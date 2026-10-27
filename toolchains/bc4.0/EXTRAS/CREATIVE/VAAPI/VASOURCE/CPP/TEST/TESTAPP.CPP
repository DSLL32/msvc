/*********************************************************
						Copyright Creative Technology Ltd, 1993
								All Right Reserved
MODULE:	TestApp.cpp
Programmer:	Nam Hoang
*********************************************************/

#include <owl\owldefs.h>
#include <owl\version.h>

#include <owl\module.h>
#include <owl\applicat.h>

#include <owl\dc.h>
#include <owl\menu.h>

#include <owl\window.h>
#include <owl\framewin.h>
#include <mmsystem.h>
#include <tvassist.h>
#include "testapp.h"
#include "tdl1.h"
#include "tvauser.h"
#include "tvaconfi.h"

class TTestFrame: public TFrameWindow  // move to HEADER FILE
{
	public:
		TVAssist *VAObject;   // forward reference for VA object;
		TTestFrame(TWindow* pParent, const char far* Title):TFrameWindow(pParent,
						Title){VAObject = new TVAssist;};
		~TTestFrame(){delete VAObject;};
		LRESULT EvAppSwitch(WPARAM,LPARAM);
		LRESULT EvLinkTerminated(WPARAM,LPARAM);
		void EvRButtonDown(UINT modkeys,TPoint& point);
		void EvActivate(UINT Activate,BOOL bMin,HWND hOther);
		void CmdCommand();
		void CmdUser();
		void CmdConfidence();
		void CmdExit();
		DECLARE_RESPONSE_TABLE(TTestFrame);
};

// DEFINE REGISTERED MESSAGES AS MACROS...
DEFINE_RESPONSE_TABLE1(TTestFrame,TFrameWindow)
	EV_COMMAND(CM_VACONTROLCOMMAND1,CmdCommand),
	EV_COMMAND(CM_VACONTROLUSER1,CmdUser),
	EV_COMMAND(CM_VACONTROLCONFIDENCE1,CmdConfidence),
	EV_COMMAND(CM_VACONTROLEXIT1,CmdExit),
	EV_REGISTERED("APP_SWITCH",EvAppSwitch),           //3 registered messages for VA API
	EV_REGISTERED("LINK_TERMINATED",EvLinkTerminated),
	EV_WM_ACTIVATE,
END_RESPONSE_TABLE;


void TTestFrame::EvActivate(UINT active,BOOL minized,HWND hWndOther)
{
	TWindow::EvActivate(active,minized,hWndOther);
	if(active)
		VAObject->InitializeLink(HWindow,active);
}

LRESULT TTestFrame::EvAppSwitch(WPARAM wP,LPARAM lP)  // for registered message
{
	return VAObject->AppSwitch(wP,lP);
}

LRESULT TTestFrame::EvLinkTerminated(WPARAM wP,LPARAM lP)  // for registered message
{
	return VAObject->LinkTerminated(wP,lP);
}
void TTestFrame::CmdExit()
{
	TWindow::CloseWindow();
}
void TTestFrame::CmdCommand()
{
	TVACommand *VACmdDlg;
	VACmdDlg = new TVACommand((TVAssist*)VAObject,this,IDD_TDLG);
	VACmdDlg->Execute();
	delete VACmdDlg;
	VAObject->ReInitializeLink(HWindow);
}

void TTestFrame::CmdUser()
{
	TVAUser *UserDlg;
	UserDlg = new TVAUser((TVAssist*)VAObject,this,IDD_USER);
	UserDlg->Execute();
	delete UserDlg;
	VAObject->ReInitializeLink(HWindow);
}

void TTestFrame::CmdConfidence()
{
	TVAConfid *ConfidDlg;
	ConfidDlg = new TVAConfid((TVAssist*)VAObject,this,IDD_CONFIDENCE);
	ConfidDlg->Execute();
	delete ConfidDlg;
	VAObject->ReInitializeLink(HWindow);
}

void testApp::InitMainWindow ()
{
	MainWindow = new TTestFrame(0,"Hello Application");
	MainWindow->AssignMenu(SDI_MENU);
}


int OwlMain (int /*argc*/, char* /*argv*/ [])
{
	 testApp   App;

	 int result = App.Run();

	 return result;
}
