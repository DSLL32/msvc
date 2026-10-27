#ifndef __dlgtrvoi_H				// Sentry, use file only if it's not already included.
#define __dlgtrvoi_H
#include <owl\button.h>
/*  Project test
    
    Copyright © 1993.  All Rights Reserved.

    SUBSYSTEM:    test.exe Application
    FILE:         ttrvcdlg.h
    AUTHOR:       


    OVERVIEW
    ========
    Class definition for TVADllTrVoiceDlg (TDialog).      
*/



//{{TDialog = TVADllTrVoiceDlg}}
class TVADllTrVoiceDlg : public TDialog {
public:
	WORD wNumTrains, wStartIndex;
	BOOL bSingleOrAll,bFirstOkClick;
	TVAssist* VAVoiceDlg;

	 TVADllTrVoiceDlg (TVAssist* pParent, TWindow* parent,WORD wIndex,BOOL bSingle);
    virtual ~TVADllTrVoiceDlg ();

//{{TVADllTrVoiceDlgRSP_TBL_BEGIN}}
protected:
	 void EvActivate (UINT active, BOOL minimized, HWND hWndOther );
	 void CmOk();
	 void CmCancel();
	 LRESULT EvSpeechStop(WPARAM,LPARAM);
	 LRESULT EvLinkTerminated(WPARAM,LPARAM);
	 LRESULT EvAppSwitch(WPARAM,LPARAM);
	 LRESULT EvTrainVoice(WPARAM,LPARAM);
	 void CmTr1(){wNumTrains = 1;};
	 void CmTr2(){wNumTrains = 2;};
	 void CmTr3(){wNumTrains = 3;};
//{{TVADllTrVoiceDlgRSP_TBL_END}}
DECLARE_RESPONSE_TABLE(TVADllTrVoiceDlg);

};    //{{TVADllTrVoiceDlg}}


#endif 		// __ttrvcdlg_H sentry.

