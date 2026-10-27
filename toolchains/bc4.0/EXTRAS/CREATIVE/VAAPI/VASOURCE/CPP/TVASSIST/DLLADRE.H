#ifndef __dlladre_H				// Sentry, use file only if it's not already included.
#define __dlladdre_H

/*  Project test
    
    Copyright © 1993.  All Rights Reserved.

    SUBSYSTEM:    test.exe Application
    FILE:         tvaaddrn.h
    AUTHOR:       


    OVERVIEW
    ========
    Class definition for TVAAddRename (TDialog).      
*/



//{{TDialog = TVADllAddRename}}
#define ADD 1
#define RENAME 2
class TVADllAddRename : public TDialog {
public:
	 int nAddOrRename;
	 WORD wRenameIndex;
	 TVAssist* AddRename;
	 void ErrorMessage(int);
	 BOOL CanClose();
	 TVADllAddRename (int ADDorRENAME,WORD wIndex,TVAssist* pParent,TWindow* parent, TResId resId, TModule* module = 0);
	 virtual ~TVADllAddRename ();
	 virtual void CmOk();
	 void EvActivate(UINT,BOOL,HWND);
	 LRESULT EvAppSwitch(WPARAM,LPARAM);
	 LRESULT EvLinkTerminated(WPARAM,LPARAM);
	 DECLARE_RESPONSE_TABLE(TVADllAddRename);
};    //{{TVADllAddRename}}


#endif 		// __dlladre_H sentry.

