//***************************************************************************
// tutor.hxx - Application Class Definitions
// 9/04/92 - 2:36:20pm
// John Doe
//***************************************************************************
//PROTECT_BEGIN Revisions 
//PROTECT_END 
#include <CommonVu.hxx>
#include <string.h>
#include "jkickdwn.h"			// Needed for Postmessage() & Win31 stuff
#include "Tutor.h"	 		// Resource Constants
#include "jtools.hxx"		// Module Class Definitions
//PROTECT_BEGIN AppIncludes 
//PROTECT_END 

App *GetApp();	 // Return pointer to app
class MainWin : public TopAppWindow 
{
	Menu MainMenu;	
	Icon MainIcon;	
	Accel MainAccel;	
	ToolBar *pTB;
public:
	MainWin();
	~MainWin();

protected:
  virtual void MenuCommand ( MenuCommandEvt );
	//PROTECT_BEGIN wMainClass 
	//PROTECT_END 
};

class NewWinWin : public TopAppWindow 
{
public:
	NewWinWin();
	~NewWinWin();

protected:
  virtual void MenuCommand ( MenuCommandEvt );
	virtual void Close(Event) {delete this;} // Delete dynamic win
	//PROTECT_BEGIN wNewWinClass 
	//PROTECT_END 
};

class About : public DialogWindow

{
	char *p, szWorkStr[80];		// Char Pointer & buffer for Edit Validation
public:
	About(pWindow Parent);
	~About();
	virtual void ButtonClick(ControlEvt);
  virtual void MenuCommand (MenuCommandEvt);	// For ToolItems 
	virtual Bool CanClose();
	//PROTECT_BEGIN dABOUTClass 
	//PROTECT_END 
};

class Address : public ModeLessDialog

{
	char *p, szWorkStr[80];		// Char Pointer & buffer for Edit Validation
	SingleLineEdit *Lname, *Name, *Title, *Coname, *Street1, *Street2, *City, 
*State, *Zip, *Bphone, *Hphone, *Fax, *Comments;
	RadioButton *Male, *Female;
	ComboBox *Addtype;
public:
	Address(pWindow Parent);
	~Address();
	virtual void ButtonClick(ControlEvt);
  virtual void MenuCommand (MenuCommandEvt);	// For ToolItems 
	virtual Bool CanClose();
	virtual void Close(Event) {delete this;} // Delete modeless dlg
	//PROTECT_BEGIN dADDRESSClass 
	//PROTECT_END 
};

class Search : public DialogWindow

{
	char *p, szWorkStr[80];		// Char Pointer & buffer for Edit Validation
	ListBox *List;
public:
	Search(pWindow Parent);
	~Search();
	virtual void ButtonClick(ControlEvt);
  virtual void MenuCommand (MenuCommandEvt);	// For ToolItems 
	virtual Bool CanClose();
	//PROTECT_BEGIN dSEARCHClass 
	//PROTECT_END 
};


