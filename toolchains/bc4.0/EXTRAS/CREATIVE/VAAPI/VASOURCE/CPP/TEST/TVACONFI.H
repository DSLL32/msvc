#ifndef __tvaconfi_H				// Sentry, use file only if it's not already included.
#define __tvaconfi_H

/*  Project test
    
    Copyright © 1993.  All Rights Reserved.

	 SUBSYSTEM:    test.exe Application
    FILE:         tvauser.h
    AUTHOR:       


    OVERVIEW
    ========
    Class definition for TVAUser (TDialog).      
*/



//{{TDialog = TVAConfid}}
class TVAConfid : public TDialog {
public:
	TVAssist* VAConfid;
	 TVAConfid (TVAssist* pParent,TWindow* parent,TResId resId, TModule* module = 0);
    virtual ~TVAConfid ();

//{{TVAConfidRSP_TBL_BEGIN}}
protected:
    void EvActivate (UINT active, BOOL minimized, HWND hWndOther );
	 LRESULT EvAppSwitch(WPARAM,LPARAM);
	 LRESULT EvLinkTerminated(WPARAM,LPARAM);
	 LRESULT EvSpeechStop(WPARAM,LPARAM);
//{{TVAConfidRSP_TBL_END}}
DECLARE_RESPONSE_TABLE(TVAConfid);
};    //{{TVAConfid}}


#endif 		// __TVAConfid_H sentry.

