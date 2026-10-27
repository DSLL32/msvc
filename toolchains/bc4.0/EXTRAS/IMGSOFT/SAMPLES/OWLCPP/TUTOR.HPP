//***************************************************************************
// tutor.hpp - Application Class Definitions
// 9/04/92 - 2:13:18pm
// John Doe
//***************************************************************************
//PROTECT_BEGIN Revisions 
//PROTECT_END 
#include <owl.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dialog.h>
#include <listbox.h>
#include <edit.h>
#include <radiobut.h>
#include <checkbox.h>
#include <combobox.h>
#include <listbox.h>
#include <mmsystem.h>		// Multimedia...needed for sndPlaySound()
#include <commdlg.h>		// Common Dialogs
#include "Tutor.h"				// Resource Constants
#include "jtools.hpp"				// Module Class Definitions
//PROTECT_BEGIN AppIncludes 
//PROTECT_END 


_CLASSDEF(Main)
class Main : public TWindow 
{
public:
	Main(PTWindowsObject AParent);

	virtual void CloseWindow();
	virtual void SetupWindow();
	virtual LPSTR GetClassName() {return "Main";}
	virtual void GetWindowClass(WNDCLASS&);
	virtual void MsgMainApp(RTMessage Msg) = [CM_FIRST+CM_MAINAPP];
	virtual void MsgMainDialog(RTMessage Msg) = [CM_FIRST+CM_MAINDIALOG];
	virtual void MsgMainWindow(RTMessage Msg) = [CM_FIRST+CM_MAINWINDOW];
	virtual void MsgMainSound(RTMessage Msg) = [CM_FIRST+CM_MAINSOUND];
	virtual void CMNew(RTMessage Msg) = [CM_FIRST+CM_NEW];
	virtual void CMOpen(RTMessage Msg) = [CM_FIRST+CM_OPEN];
	virtual void CMSave(RTMessage Msg) = [CM_FIRST+CM_SAVE];
	virtual void CMPrint(RTMessage Msg) = [CM_FIRST+CM_PRINT];
	virtual void CMAbout(RTMessage Msg) = [CM_FIRST+CM_ABOUT];
	//PROTECT_BEGIN wMainClass 
	//PROTECT_END 
};
_CLASSDEF(NewWin)
class NewWin : public TWindow 
{
public:
	NewWin(PTWindowsObject AParent);

	virtual void CloseWindow();
	virtual void SetupWindow();
	virtual LPSTR GetClassName() {return "NewWin";}
	virtual void GetWindowClass(WNDCLASS&);
	//PROTECT_BEGIN wNewWinClass 
	//PROTECT_END 
};
_CLASSDEF(About)
class About : public TDialog 
{
	char szWorkStr[81], *p;		// Work Buffer
public:
	About(PTWindowsObject AParent, Pchar DlgName);

	virtual void CloseWindow(int ARetValue);
	virtual void SetupWindow();
	virtual BOOL CanClose();
	//PROTECT_BEGIN dABOUTClass 
	//PROTECT_END 
};

_CLASSDEF(Address)
class Address : public TDialog 
{
	char szWorkStr[81], *p;		// Work Buffer
	PTEdit Lname, Name, Title, Coname, Street1, Street2, City, State, Zip, Bphone, 
Hphone, Fax, Comments;
	PTRadioButton Male, Female;
	PTComboBox Addtype;
public:
	Address(PTWindowsObject AParent, Pchar DlgName);

	virtual void CloseWindow(int ARetValue);
	virtual void SetupWindow();
	virtual BOOL CanClose();
	virtual void CMSearch(RTMessage Msg) = [ID_FIRST+ID_SEARCH];
	//PROTECT_BEGIN dADDRESSClass 
	//PROTECT_END 
};

_CLASSDEF(Search)
class Search : public TDialog 
{
	char szWorkStr[81], *p;		// Work Buffer
	PTListBox List;
public:
	Search(PTWindowsObject AParent, Pchar DlgName);

	virtual void CloseWindow(int ARetValue);
	virtual void SetupWindow();
	virtual BOOL CanClose();
	//PROTECT_BEGIN dSEARCHClass 
	//PROTECT_END 
};


class TutorApp : public TApplication {
public:
	TutorApp(LPSTR AName, HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
		: TApplication(AName, hInstance, hPrevInstance, lpCmdLine, nCmdShow) {};
  
	virtual void InitInstance()
	{
  	TApplication::InitInstance();
  	HAccTable = LoadAccelerators(hInstance, "Tutor");
	}
	virtual void InitMainWindow();
	//PROTECT_BEGIN AppClass 
	//PROTECT_END 
};
