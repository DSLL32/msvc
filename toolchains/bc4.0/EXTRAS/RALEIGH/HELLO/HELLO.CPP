/*  VCS_ID
 *  $Filename:   hello.cpp
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:20:32  $
 */
 
#define InclGraphics
#include <ObjectPM.hpp>
#include "hello.hpp"	// include the class definitions

short bMainThread :: Start()		// Program execution starts here
{
	HelloWindow w;	    // Contstruct the top level frame window
	Exec();				// Go to the message loop.

	return 0;
}

HelloWindow :: HelloWindow()
{												   
	CreateWindow(OpmStdFrame);					   // Create the frame
	SetCaption("Hello World");					   // Set captions
	SetSwitchTitle("Hello World Demo Program");
	SetIcon(icon = new wIcon(ResIcon, ITriangle));   // Set the app's icon
	msg = 0;

	// This section create a menu bar and then adds items to the 
	// sub menu itself.  It is also possible to construct the menu
	// from the resource file if desired

	menubar = new wMenu(this, MBarMessage, "~Message\\");
	menubar->SetSubMenuItems(MBarMessage, MiNext, "Next\t\x1a;Previous\t\x1b");

	// Create a temporary system menu object for the purposes of adding
	// a seperator and the "About..." menu item.

	wSysMenu sm(this);
	sm.AppendSeparator();
	sm.AppendItem(ScAbout, "~About...");

	// Finally, create an accelerator table to bind the left and right
	// arrow keys to the "Next" and "Prev" menu options.  Like the menu's,
	// it is also possible to load an accelerator table from the resource
	// file

	acceltab = new wAccelTable(this);
	acceltab->Set(VkRight, MiNext, AsVirtualKey);
	acceltab->Set(VkLeft,  MiPrev, AsVirtualKey);

	// Add event handlers for the menu items.  This mechanism allows a
	// method to be tied to a specific menu item.  If these calls are not
	// used, the menu events will be delivered the MenuCommand
	// method of the wWindow class

	AddMessageHandler(this, EVENTMETHOD(HelloWindow, ProcNextPrev), 
	                  WM_COMMAND, MiNext);	
	AddMessageHandler(this, EVENTMETHOD(HelloWindow, ProcNextPrev), 
	                  WM_COMMAND, MiPrev);	
	AddMessageHandler(this, EVENTMETHOD(HelloWindow, ProcAbout), 
	                  WM_COMMAND, ScAbout);	

	Show();				   // Make the frame visible
}

HelloWindow :: ~HelloWindow()
{				   
	delete acceltab;	   // destroy the allocated frame objects
	delete menubar;
	delete icon;
}

// process the "Next" and "Prev" message commands 

long HelloWindow :: ProcNextPrev(wMessage m)
{
	if ((ushort)m.mp1 == MiNext)
		msg = msg == LASTMSG? 0: msg + 1;
	else
		msg = msg == 0? LASTMSG: msg - 1;

	RePaint();		// invalidate the client window 
	return FALSE;
}

// handle displaying the "About" window

long HelloWindow :: ProcAbout(wMessage)
{
	AboutDialog aboutWin(this);		 // Create the "about" box
	return FALSE;
}

// The Paint member is responsible for painting the invalid
// area of the client window.  This program, like many programs that
// do simple drawing, let the object library create a wPresSpace
// object on which to draw on.  The wPresSpace object is accessed
// through the member function WindowPS() of the wWindow class.

void HelloWindow :: Paint()
{
	static char *msgtext[LASTMSG+1] = {
		"Hello World",
		"Welcome to OS/2 Object Oriented Programming",
		"Introducing ObjectPM",
		"A C++ Class Library and Application Framework",
		"Have a Good Day!!!"
	};

	WindowPS()->Erase();		  // Erase the client window
	wTextPen t(WindowPS());		  // Create a new wTextPen object 
	t.SetColor(ClrBlue);
	wFont f(TmsRmn14, UnderscoreFont, WindowPS());
	t.Display(WindowRectangle(), DfCenter | DfVCenter, msgtext[msg]);
}

// The Close member is called when the "Close" menu option is chosen
// from the system menu.  Returning TRUE from this function allows the
// exit process to continue.  Otherwise, a return of FALSE will prevent
// the application from being closed

long HelloWindow :: Close(wMessage)
{
	// create a message box to confirm exit from the application

	wMessageBox m(this, "              Are You Sure?", "Exit", MbYesNo+MbIconQuestion+MbApplModal);
	return m.MessBoxVal() == 6;
}
