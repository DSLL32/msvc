#ifndef __tdl1_H				// Sentry, use file only if it's not already included.
#define __tdl1_H

/*  Project test
    
    Copyright © 1993.  All Rights Reserved.

    SUBSYSTEM:    test.exe Application
    FILE:         tdl1.h
    AUTHOR:       


    OVERVIEW
	 ========
    Class definition for tdlg (TDialog).      
*/
#include <owl\dialog.h>
#include "testapp.rh"            // Definition of all resources.

 class TVAControl: public TVAssist
{
	public:
		TVAControl(TVAssist* pParent):TVAssist(pParent,ACTIONON,FALSE){};
		void ShowComdInListBox(HWND );
		virtual void BecomingActivePostRequest();

};//{{TDialog = tdlg}}
class TVACommand : public TDialog {
public:
	 TVAControl* VAControl;
    TVACommand (TVAssist* pParent,TWindow* parent, TResId resId, TModule* module = 0);
	 virtual ~TVACommand ();
	LRESULT EvAppSwitch(WPARAM,LPARAM);
	LRESULT EvLinkTerminated(WPARAM,LPARAM);
	LRESULT EvActionStart(WPARAM,LPARAM);
	LRESULT EvActionStop(WPARAM,LPARAM);
	void EvActivate(UINT Activate,BOOL bMin,HWND hOther);
	void EvAddVACommand();
	void EvRenameVACommand();
	void EvDeleteVACommand();
	void EvTrainingAction();
	void EvSingle();
	void EvAll();
	DECLARE_RESPONSE_TABLE(TVACommand);
};    //{{TVACommand}}


#endif 		// __tdl1_H sentry.

