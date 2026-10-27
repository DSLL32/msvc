#ifndef __tvauser_H				// Sentry, use file only if it's not already included.
#define __tvauser_H

/*  Project test
    
    Copyright © 1993.  All Rights Reserved.

    SUBSYSTEM:    test.exe Application
    FILE:         tvauser.h
    AUTHOR:       


    OVERVIEW
    ========
    Class definition for TVAUser (TDialog).      
*/



//{{TDialog = TVAUser}}
class TVAUser : public TDialog {
public:
	TVAssist* VAUser;
	 TVAUser (TVAssist* pParent,TWindow* parent,TResId resId, TModule* module = 0);
    virtual ~TVAUser ();

//{{TVAUserRSP_TBL_BEGIN}}
protected:
    void EvActivate (UINT active, BOOL minimized, HWND hWndOther );
	 void CmOk();
	 LRESULT EvAppSwitch(WPARAM,LPARAM);
	 LRESULT EvLinkTerminated(WPARAM,LPARAM);
//{{TVAUserRSP_TBL_END}}
DECLARE_RESPONSE_TABLE(TVAUser);
};    //{{TVAUser}}


#endif 		// __tvauser_H sentry.

