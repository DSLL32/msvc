//***************************************************************************
// tutor.hpp - Application Class Definitions
// 9/04/92 - 2:16:41pm
// John Doe
//***************************************************************************
//PROTECT_BEGIN Revisions 
//PROTECT_END 
#include "zapp.hpp"
#include <mmsystem.h>					// Multimedia...needed for sndPlaySound()
#include <commdlg.h>					// Common Dialogs
#include "Tutor.h"				// Resource Constants
#include "jtools.hpp"				// Module Class Definitions
//PROTECT_BEGIN AppIncludes 
//PROTECT_END 

class Main : public zAppFrame 
{
	zPane *wp;	 // Window Pane
public:
	Main(zWindow *w, zString &title);
	~Main(){};

	virtual int command(zCommandEvt *);
	//PROTECT_BEGIN wMainClass 
	//PROTECT_END 
};
class MainPane : public zPane 	 
{
public:
	MainPane(zWindow *w);
	~MainPane(){};
	//PROTECT_BEGIN pMainClass 
	//PROTECT_END 
};
class NewWin : public zDialogFrame 
{
	zPane *wp;	 // Window Pane
public:
	NewWin(zWindow *w, zString &title);
	~NewWin(){};

	virtual int command(zCommandEvt *);
	virtual void shutdown() {zDialogFrame::shutdown(); delete this;}
	//PROTECT_BEGIN wNewWinClass 
	//PROTECT_END 
};
class NewWinPane : public zPane 	 
{
public:
	NewWinPane(zWindow *w);
	~NewWinPane(){};
	//PROTECT_BEGIN pNewWinClass 
	//PROTECT_END 
};
class About : public zDialogFrame
{
	char *p;		// Char Pointer for Edit Validation
	BOOL ok;
public:
	About(zWindow *, zResId&);

	virtual int command(zCommandEvt *);
	BOOL isOk() { return ok; }

	virtual BOOL CanClose();
	//PROTECT_BEGIN dABOUTClass 
	//PROTECT_END 
};

class Address : public zDialogFrame
{
	char *p;		// Char Pointer for Edit Validation
	zEditLine *Lname, *Name, *Title, *Coname, *Street1, *Street2, *City, *State, 
*Zip, *Bphone, *Hphone, *Fax, *Comments;
	zRadioButton *Male, *Female;
	zComboBox *Addtype;
	BOOL ok;
public:
	Address(zWindow *, zResId&);

	virtual int command(zCommandEvt *);
	BOOL isOk() { return ok; }

	virtual BOOL CanClose();
	virtual void shutdown() {zDialogFrame::shutdown(); delete this;}
	//PROTECT_BEGIN dADDRESSClass 
	//PROTECT_END 
};

class Search : public zDialogFrame
{
	char *p;		// Char Pointer for Edit Validation
	zListBox *List;
	BOOL ok;
public:
	Search(zWindow *, zResId&);

	virtual int command(zCommandEvt *);
	BOOL isOk() { return ok; }

	virtual BOOL CanClose();
	//PROTECT_BEGIN dSEARCHClass 
	//PROTECT_END 
};


