/*********************************************************
						Copyright Creative Technology Ltd, 1993
								All Right Reserved
MODULE: HELLO.CPP
PROGRAMMER:	Nam Hoang								
*********************************************************/
#include <owl\owlpch.h>
#include <owl\owldefs.h>
#include <owl\version.h>

#include <owl\module.h>
#include <owl\applicat.h>

#include <owl\dc.h>
#include <owl\menu.h>

#include <owl\window.h>
#include <owl\framewin.h>
#include <mmsystem.h>
#include "tvassist.h"
#include "hello.h"

void myPlayVoice(BOOL nFlag,HWND hwnd, char *szFileName)
{
	char szVoiceBuf[256],szRetString[80];
	DWORD dwRes;

	if(nFlag) // play the voice
	{
		mciSendString ((LPSTR)"stop goober", NULL, 0, NULL);
		mciSendString ((LPSTR) "close goober",NULL,0,NULL);

		wsprintf((LPSTR)szVoiceBuf,"open %s type waveaudio alias goober",
			(LPSTR)szFileName);

		dwRes = mciSendString ((LPSTR)szVoiceBuf, szRetString, 80, NULL);

		if(dwRes)
		{
			SROnOff(hwnd,1);
			return ;
		}

		dwRes = 	mciSendString ((LPSTR)"play goober from 0 notify ",
						 szRetString, 80, hwnd);
		if(dwRes)
			return ;
	}
	else //close mci
	{
		mciSendString ((LPSTR)"stop goober", NULL, 0, NULL);
		mciSendString ((LPSTR) "close goober",NULL,0,NULL);
	}
}

class TVAHello: public TVAssist
{
	public:
		TVAHello(){};
		virtual void ActionStartSuccess();
		~TVAHello(){myPlayVoice(FALSE,NULL,NULL);};
};

void TVAHello::ActionStartSuccess()
{
	// if VA recognized a command then It plays hello.wav
	TVAssist::ActionStartSuccess();
	if(LOWORD(lVAParam) == 32) // Hello command
		myPlayVoice(TRUE,hMain,"hello.wav");
}

class THelloFrame: public TFrameWindow  // move to HEADER FILE
{
	public:
		TVAssist *VAObject;   // forward reference for VA object;
		THelloFrame(TWindow* pParent, const char far* Title):TFrameWindow(pParent,
						Title){VAObject = new TVAHello;};
		~THelloFrame(){delete VAObject;};
		void EvActivate(UINT active,BOOL minized,HWND hWndOther);
		LRESULT EvAppSwitch(WPARAM,LPARAM);
		LRESULT EvLinkTerminated(WPARAM,LPARAM);
		LRESULT EvActionStart(WPARAM,LPARAM);
		DECLARE_RESPONSE_TABLE(THelloFrame);
};
// DEFINE REGISTERED MESSAGES AS MACROS...
DEFINE_RESPONSE_TABLE1(THelloFrame,TFrameWindow)
	EV_REGISTERED("APP_SWITCH",EvAppSwitch),           //3 registered messages for VA API
	EV_REGISTERED("LINK_TERMINATED",EvLinkTerminated),
	EV_REGISTERED("ACTION_START",EvActionStart),
	EV_WM_ACTIVATE,
END_RESPONSE_TABLE;

LRESULT THelloFrame::EvActionStart(WPARAM wP,LPARAM lP) // for registered message
{
	return VAObject->ActionStart(wP,lP);
}

void THelloFrame::EvActivate(UINT active,BOOL minized,HWND hWndOther)
{
	TWindow::EvActivate(active,minized,hWndOther);
	if(active)
		VAObject->InitializeLink(HWindow,active);
}

LRESULT THelloFrame::EvAppSwitch(WPARAM wP,LPARAM lP)  // for registered message
{
	return VAObject->AppSwitch(wP,lP);
}

LRESULT THelloFrame::EvLinkTerminated(WPARAM wP,LPARAM lP)  // for registered message
{
	return VAObject->LinkTerminated(wP,lP);
}

void HelloApp::InitMainWindow()
{
	MainWindow = new THelloFrame(0,"Hello Voice Assist Class Lib");
}

int OwlMain(int /*argc*/, char* /*argv*/ [])
{
	HelloApp Hello;
	return Hello.Run();
}
