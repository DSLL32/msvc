/*  VCS_ID
 *  $Filename:   threads.cxx
 *  $Author  :   John Pompeii
 *  $Revision:   1.2  $
 *  $Date:   06 Jan 1992 01:09:50  $
 */
 
extern "C" 
{
	#include <stdio.h>
}

#define InclGraphics
#include <ObjectPM.hpp>
#include "threads.hpp"

short MainThread :: Start()       
{       
	TopWindow w;	// Create the top  class      
    Exec();     
 
	return 0;
}

short ChildThread :: Start()		// initial entry point for new threads!
{
	TopWindow w;	// Create the top  class      
    Exec();
    Quit(TRUE);		// Self destruct ("this")  

	return 0;
}

TopWindow :: TopWindow()
{
	CreateWindow(OpmStdFrame);
	if (CurrentThread->IsPrimaryThread())
	{
 		menubar = new wMenu (this, 100, "~Window\\");
		menubar->SetSubMenuItems(100, 101, "Create;");
		SetCaption("Thread Test Program");
		SetSwitchTitle("Thread Test Program");
	}
	else
	{
		SetCaption("Threaded App Window");
		SetSwitchTitle("Threaded App Window");
	}
	SetIcon(icon = new wIcon(ResIcon, I_OBJPM)); 
	Show();
	ToTop();
}

TopWindow :: ~TopWindow()
{
	delete icon;
	delete menubar;
}

void TopWindow :: Paint() 
{
	WindowPS()->Erase();
	wTextPen p(WindowPS());
	p.SetBufferSize(60);

	if (CurrentThread->IsPrimaryThread())
		p.Display(WindowRectangle(), DfCenter+DfVCenter, "I am the primary window!");
	else
		p.Printf(WindowRectangle(), DfCenter+DfVCenter, "I am a window running in thread: %d", CurrentThread->ThreadId());
}

long TopWindow :: MenuCommand(wCommandMsg m)
{
	switch(m.usCmd())
	{
		case 101:	// Create
			new ChildThread(ThisThread, "");
			break;
	}
    return FALSE;
}
