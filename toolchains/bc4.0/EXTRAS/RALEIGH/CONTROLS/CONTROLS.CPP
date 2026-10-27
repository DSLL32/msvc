
/*  VCS_ID
 *  $Filename:   controls.cpp
 *  $Author  :   John Pompeii
 *  $Revision:   1.1  $
 *  $Date:   28 Dec 1991 14:20:32  $
 */

#define INCL_WIN
#include <os2.h>
 
#define InclGraphics
#define InclControls
#include <ObjectPM.hpp>
#include "controls.hpp"	

// Note: This program demonstrates the use and event handling of 
// some of the control windows.  The intent of this program was to 
// demonstrate all controls.  However, its not finished.  Although this
// is the case, there is still a lot of information in the code that 
// is finished.  Thus, we decided to include it with the samples.  
// The finished version of this program will be available in the 
// next release.   - jp
//

extern "C"
{
	#include <string.h>
}

short bMainThread :: Start()		// Program execution starts here
{
	AppWindow w;	    // Contstruct the top level frame window
	Exec();				// Go to the message loop.

	return 0;
}

AppWindow :: AppWindow()
{												   
	CreateWindow(FaTaskList | FaTitleBar | FaSysMenu | FaSizeBorder | FaMinMax);
	SetCaption("Controls Demo");				   // Set captions
	SetSwitchTitle("Controls Demo Program");
	SetIcon(icon = new wIcon(ResIcon, I_OBJPM));   // Set the app's icon

	// This section create a menu bar and then adds items to the 
	// sub menu itself.  It is also possible to construct the menu
	// from the resource file if desired

	new wMenu(this, MB_DEMO, "~Demo\\~Help\\");
	MenuBar()->SetSubMenuItems(MB_DEMO, MI_LISTBOX, 
	"~ListBox;~CellBox;~Buttons;~Slider;~Entry Field;~Value Set;~Spin Button");

	// Create a temporary system menu object for the purposes of adding
	// a seperator and the "About..." menu item.

	wSysMenu sm(this);
	sm.AppendSeparator();
	sm.AppendItem(SC_ABOUT, "~About...");

	// build the pages

	topPage = new wPageWindow(this);
	topPage->AddChildPage(new ListBoxPage("Listbox"));
	topPage->AddChildPage(new ButtonPage("Button"));
	topPage->AddChildPage(new SliderPage("Slider"));
	topPage->AddChildPage(new EntryPage("Entry"));
	topPage->AddChildPage(new SpinPage("SpinButton"));
	topPage->AddChildPage(new CellBoxPage("CellBox"));
	topPage->AddChildPage(new ValueSetPage("ValueSet"));

	ChangeFrameSize(CalcFrameSize(wDimension(320, 135, DlgUnits)));
	ChangePosition(wPointl(80, 80));
 	SetMaximumSize(GetSize());

	Show();				   // Make the frame visible
	ToTop();
}

AppWindow :: ~AppWindow()
{				   
	delete icon;
}

// The MenuCommand member is sent a Command message each time
// an item on the menu bar is selected.

long AppWindow :: MenuCommand(wCommandMsg m)
{
	switch(m.usCmd())
	{
		case MI_LISTBOX:
			ChangePage(topPage->ChildPage("Listbox"));
			break;

		case MI_CELLBOX:
			ChangePage(topPage->ChildPage("CellBox"));
			break;

		case MI_BUTTON:
			ChangePage(topPage->ChildPage("Button"));
			break;

		case MI_SLIDER:
			ChangePage(topPage->ChildPage("Slider"));
			break;

		case MI_ENTRY:
			ChangePage(topPage->ChildPage("Entry"));
			break;

		case MI_VALUE:
			ChangePage(topPage->ChildPage("ValueSet"));
			break;

		case MI_SPIN:
			ChangePage(topPage->ChildPage("SpinButton"));
			break;

		case SC_ABOUT:
			{
				AboutDialog aboutWin(this);		 // Create the "about" box
				return FALSE;
			}
	}
	return FALSE;
}

// The Paint member is responsible for painting the invalid
// area of the client window.  This program, like many programs that
// do simple drawing, let the object library create a wPresSpace
// object on which to draw on.  The wPresSpace object is accessed
// through the member function WindowPS() of the wWindow class.

void AppWindow :: Paint()
{
	WindowPS()->Erase();
}

// The Close member is called when the "Close" menu option is chosen
// from the system menu.  Returning TRUE from this function allows the
// exit process to continue.  Otherwise, a return of FALSE will prevent
// the application from being closed

long AppWindow :: Close(wMessage)
{
	// create a message box to confirm exit from the application

	wMessageBox m(this, "              Are You Sure?", "Exit", MbYesNo+MbIconQuestion+MbApplModal);
	return m.MessBoxVal() == MBID_YES;
}


/*****************************************************************************/
/* 
/* Common Page Methods for all demos
/* 
/*****************************************************************************/


ControlDemoPage :: ControlDemoPage(char *name, char *dn) 
                 : wPageWindow(name)
{
	demoName = dn;
}

short ControlDemoPage :: PageIn()
{
	CreateWindow(ObjectWindow, 100, wPointl(0, 0), 
 	             wDimension(ViewPortWindow()->GetSize()));

	ChangeColor(BackgndClr, SysClrDialogBackground);

	MenuBar() = new wMenu(this, MB_ACTIONS, "~Actions\\~Exit\\");
    MenuBar()->SetSubMenuItems(MB_EXIT, MI_CONT, "Continue Demo; Exit;");

	AddMessageHandler(this, EVENTMETHOD(ControlDemoPage, ProcClearMLE), 
	                  WM_COMMAND, UID_CLRBUTTON);
	AddMessageHandler(this, EVENTMETHOD(ControlDemoPage, ExitDemo), 
	                  WM_COMMAND, MI_EXITNOW);	

 	pmle = new wMLEntry(this, UID_NOTIFY, wPointl(196, 6, DlgUnits),
 	                   wDimension(114, 110, DlgUnits), 
					   WsVisible | MlHscroll | MlVscroll | MlBorder | MlReadOnly);

	pclr = new wPushButton(this, UID_CLRBUTTON, wPointl(265, 120, DlgUnits), 
 	                   wDimension(46, 11, DlgUnits), 
					   WsVisible | BsNoPointerFocus);

 	pclr->SetText("Clear");
	return TRUE;
}

short ControlDemoPage :: PageOut()
{
	delete pclr;
	delete pmle;
	delete MenuBar();
	DestroyWindow();
	return TRUE;
}

void  ControlDemoPage :: Paint()
{
	WindowPS()->Erase(WindowRectangle(), SysClrDialogBackground); // Erase the client window
	wTextPen t(WindowPS());		  // Create a new wTextPen object 
	t.SetColor(ClrDarkBlue);
	t.Display(wPointl(200, 122, DlgUnits), "Notifications");
	t.Display(wPointl(5, 4, DlgUnits), demoName);
	PaintStats(TRUE);
}

void ControlDemoPage :: PaintStats(bool)
{
}

long ControlDemoPage :: ProcClearMLE(wMessage)
{
	pmle->DeleteString(wRange(0, pmle->GetNumChars()));
	return FALSE;
}

long ControlDemoPage :: ExitDemo(wMessage)
{
	((wFrameWindow *)ViewPortWindow())->ChangePage(OwnerPage());
	return FALSE;
}

void ControlDemoPage :: AddNotifierMessage(char *msg)
{
	pmle->SetCursorPosition(pmle->GetNumChars());
	pmle->AddString(msg);
}


/*****************************************************************************/
/* 
/* wListBox Demo
/* 
/*****************************************************************************/


ListBoxPage :: ListBoxPage(char *name) 
             : ControlDemoPage(name, "List-box Controls")
{
}

short ListBoxPage :: PageIn()
{
	ControlDemoPage::PageIn();
    MenuBar()->SetSubMenuItems(MB_ACTIONS, MI_LBSTYLE, 
	      "Change Style...;Append Item;Insert Item;Delete Item;Search...;");

	AddMessageHandler(this, EVENTMETHOD(ListBoxPage, AddItem), 
	                  WM_COMMAND, MI_LBAPPEND);
	AddMessageHandler(this, EVENTMETHOD(ListBoxPage, AddItem), 
	                  WM_COMMAND, MI_LBINSERT);
	AddMessageHandler(this, EVENTMETHOD(ListBoxPage, DeleteItem), 
	                  WM_COMMAND, MI_LBDELETE);

	lb = new DemoListBox(this, UID_LISTBOX);
	for (nextitem = 0; nextitem < 20; nextitem++)
	{
		char sbuf[60];
		sprintf(sbuf, "Listbox item string #%d", nextitem);
		lb->AddString(sbuf);
	}

	Show();
	return TRUE;
}

short ListBoxPage :: PageOut()
{
	delete lb;
	ControlDemoPage::PageOut();
	return TRUE;
}

long ListBoxPage :: MenuCommand(wCommandMsg)
{
	return FALSE;
}

// This method takes care of enabling/disabling the menu items

long ListBoxPage :: MenuAction(wMenuActionMsg m)
{
	if (m.actionType() == MENUINIT)
	{
		ushort iMenu = (m.usId() / 100) - 1;

		if (iMenu == 0)		// Action menu?
		{
			if (lb->GetSelectedIndex() >= 0)
			{
				MenuBar()->EnableItem(MI_LBINSERT);
				MenuBar()->EnableItem(MI_LBDELETE);
			}
			else
			{
				MenuBar()->DisableItem(MI_LBINSERT);
				MenuBar()->DisableItem(MI_LBDELETE);
			}
		}
	}
	return m.actionType() == MENUSELECT;
}

long ListBoxPage :: AddItem(wCommandMsg m)
{
	char sbuf[60];
	sprintf(sbuf, "Listbox item string #%d", nextitem++);
	if (m.usCmd() == MI_LBAPPEND)
		lb->AddString(sbuf);
	else
		lb->AddString(sbuf, lb->GetSelectedIndex());

	PaintStats();
	return FALSE;
}
	
long ListBoxPage :: DeleteItem(wCommandMsg)
{
	lb->DeleteItem(lb->GetSelectedIndex());
	PaintStats();
	return FALSE;
}

void ListBoxPage :: PaintStats(bool fPSGiven)
{
	wPresSpace *ps = fPSGiven? WindowPS(): new wPresSpace(this);
	
	wTextPen t(ps); 
	t.SetColor(ClrDarkBlue);
	t.SetBackColor(SysClrDialogBackground);
	t.SetBackMix(BmOverPaint);

	t.Printf(wPointl(13, 38, DlgUnits), "Item Count: %d  ", lb->GetItemCount());

	short item = lb->GetTopIndex();
    t.Printf(wPointl(93, 38,	DlgUnits), "Top Index: ");
 	if (item < 0)
		t.Display("N/A");
	else
		t.Printf("%d    ", item);

	item = lb->GetSelectedIndex();
    t.Printf(wPointl(13, 28,	DlgUnits), "Selected Index: ");
 	if (item < 0)
		t.Display("N/A");
	else
		t.Printf("%d    ", item);

	if (!fPSGiven)
		delete ps;
}


DemoListBox :: DemoListBox(ControlDemoPage *parent, ushort id)
			 : wListBox(parent, id, wPointl(13, 48, DlgUnits), 
	                    wDimension(174, 65, DlgUnits), WsVisible)
{
	demopg = parent;
}

void DemoListBox :: Select(wControlMsg)
{
	char sbuf[80];

	sprintf(sbuf, "Select - Item #%d selected\n", GetSelectedIndex());
	demopg->AddNotifierMessage(sbuf);
	demopg->PaintStats();
}

void DemoListBox :: Enter(wControlMsg)
{
	char sbuf[80];

	sprintf(sbuf, "Enter - Item #%d\n", GetSelectedIndex());
	demopg->AddNotifierMessage(sbuf);
	demopg->PaintStats();
}

void DemoListBox :: FocusGained(wControlMsg)
{
	demopg->AddNotifierMessage("FocusGained\n");
	demopg->PaintStats();
}

void DemoListBox :: FocusLost(wControlMsg)
{
	demopg->AddNotifierMessage("FocusLost\n");
	demopg->PaintStats();
}

void DemoListBox :: Scroll(wControlMsg)
{
	demopg->AddNotifierMessage("Scroll - Listbox scrolling...\n");
	demopg->PaintStats();
}



/*****************************************************************************/
/* 
/* Buttons Demo
/* 
/*****************************************************************************/

ButtonPage :: ButtonPage(char *name) 
             : ControlDemoPage(name, "Button Controls")
{
}

short ButtonPage :: PageIn()
{
	ControlDemoPage::PageIn();

    // create the push button controls										 
	buttons[0] = new wPushButton(this, UID_PUSHBTN, wPointl(12, 100, DlgUnits), 
	                 wDimension(74, 14, DlgUnits), WsVisible | BsDefault);
	buttons[0]->SetText("Push Button");

	buttons[1] = new wPushButton(this, UID_ICONBTN1, wPointl(12, 65, DlgUnits), 
	                 wDimension(25, 23, DlgUnits), WsVisible);
	buttons[1]->SetImage(icons[0] = new wIcon(ResIcon, ICO_PAINT));

	buttons[2] = new wPushButton(this, UID_ICONBTN2, wPointl(36, 65, DlgUnits), 
	                 wDimension(25, 23, DlgUnits), WsVisible);
	buttons[2]->SetImage(icons[1] = new wIcon(ResIcon, ICO_MARK));

	buttons[3] = new wPushButton(this, UID_ICONBTN3, wPointl(60, 65, DlgUnits), 
	                 wDimension(25, 23, DlgUnits), WsVisible);
	buttons[3]->SetImage(icons[2] = new wIcon(ResIcon, ICO_TRASH));


	// create the radio button set


	groupBox = new wGroupBox(this, 0, wPointl(99, 59, DlgUnits), 
	                 wDimension(81, 59, DlgUnits), "Radio Buttons", WsVisible);

	groupBox->ChangeColor(BorderClr, ClrGreen);

	radios[0] = new wRadioButton(this, UID_RADIO1, wPointl(107, 93, DlgUnits), 
	                 wDimension(67, 10, DlgUnits), BsAuto | WsVisible);
	radios[0]->SetText("Choice 1");

	radios[1] = new wRadioButton(this, UID_RADIO2, wPointl(107, 82, DlgUnits), 
	                 wDimension(67, 10, DlgUnits), BsAuto | WsVisible);
	radios[1]->SetText("Choice 2");

	radios[2] = new wRadioButton(this, UID_RADIO3, wPointl(107, 71, DlgUnits), 
	                 wDimension(67, 10, DlgUnits), BsAuto | WsVisible);
	radios[2]->SetText("Choice 3");

	// create the check box
	checkBox = new wCheckBox(this, UID_CHKBOX, wPointl(99, 35, DlgUnits), 
	                 wDimension(84, 11, DlgUnits), BsAuto | WsVisible);
	checkBox->SetText("Check Box");

	Show();
	return TRUE;
}

short ButtonPage :: PageOut()
{
	int i;

	for (i = 0; i < 4; i++)
		delete buttons[i];

	for (i = 0; i < 3; i++)
		delete radios[i];

	for (i = 0; i < 3; i++)
		delete icons[i];

	delete checkBox;
	delete groupBox;
	ControlDemoPage::PageOut();
	return TRUE;
}

long ButtonPage :: Control(wControlMsg m)
{
	char sbuf[80];
	char *name;

	if (m.id() == UID_NOTIFY)
		return 0;

	switch (m.usNotifyCode())
	{
		case BN_CLICKED:
			sprintf(sbuf, "Clicked - id = %d\n", m.id());
			break;

		case BN_DBLCLICKED:
			sprintf(sbuf, "DoubleClicked - id = %d\n", m.id());
			break;
	};
	AddNotifierMessage(sbuf);
	return FALSE;
}

long ButtonPage :: PushButtonCommand(wCommandMsg m)
{
	char sbuf[80];

	sprintf(sbuf, "PushButtonClick - id = %d\n", m.usCmd());
	AddNotifierMessage(sbuf);
	return FALSE;
}


/*****************************************************************************/
/* 
/* Slider Demo
/* 
/*****************************************************************************/

SliderPage :: SliderPage(char *name)
	: ControlDemoPage(name, "Slider Control")
{
}

short SliderPage :: PageIn()
{
	ControlDemoPage::PageIn();

//	AddMessageHandler(this, EVENTMETHOD(SliderPage, ProcNotifiers), 
//	                  WM_CONTROL, UID_SLIDER);

	slider = new DemoSlider(this, UID_SLIDER, wPointl(11, 11, DlgUnits), 
		    wDimension(173, 118, DlgUnits), SlBtnBottomLeft | 
			SlRibbonStrip | SlVertical | SlSnaptoincrement , 60, 0, 34, 0);

	SetupSlider();

	slider->Show();
	Show();
	return TRUE;
}

short SliderPage :: PageOut()
{
	delete slider;
	ControlDemoPage::PageOut();
	return TRUE;
}

#ifdef XXXXX
long SliderPage :: ProcNotifiers(wControlMsg m)
{
	char sbuf[80];

	if (m.id() == UID_NOTIFY)
		return 0;

	switch (m.usNotifyCode())
	{
		case SLN_CHANGE:
			sprintf(sbuf, "SLN_CHANGE - Position=%ld\n", m.usData());
			break;
		case SLN_SLIDERTRACK:
			sprintf(sbuf, "SLN_SLIDERTRACK - Position=%ld\n", m.usData());
			break;
		case SLN_SETFOCUS:
			sprintf(sbuf, "SN_SETFOCUS - Focus Gained\n");
			break;
		case SLN_KILLFOCUS:
			sprintf(sbuf, "SLN_KILLFOCUS - Focus Lost\n");
			break;
		default:
			return FALSE;
	};
	AddNotifierMessage(sbuf);
	return FALSE;
}

#endif

void SliderPage :: SetupSlider()
{
	// the text for the tick marks
	char *txt1[]={"32","37","42","47","52","57","62","67","72","77","82","87"},
		 *txt2[]={"0","5","10","15","20","25","30","35"};
	ushort	lp;

	// set the font to times roman 8
	slider->ChangeFont(TmsRmn8);

	// set the primary scale text
	for (lp=0 ; lp<59 ; lp+=5)
		slider->SetScaleText(lp, txt1[lp/5]);

	// set the primary tick marks
	for (lp=0 ; lp<59 ; lp++)
		slider->SetTickSize(lp, 5 + ((lp % 5) == 0) * 3);

	// switch to working with the secondary scale
	slider->SetPrimaryScale(ScaleTwo);

	// set the secondary scale text
	for (lp=0 ; lp<33 ; lp+=5)
		slider->SetScaleText(lp, txt2[lp/5]);

	// set the secondary tick marks
	for (lp=0 ; lp<33; lp++)
 		slider->SetTickSize(lp, 5+ ((lp % 5) == 0) * 3);

	// add some detent positions
	slider->AddDetent((ushort)80);
	slider->AddDetent((ushort)22);
}


DemoSlider :: DemoSlider(ControlDemoPage *pg, ushort id, wPointl p, 
                         wDimension d, ulong style,	ushort inc1, 
						 ushort sp1, ushort inc2, ushort sp2)

			:  wSlider(pg, id, p, d, style, inc1, sp1, inc2, sp2)
{
	demopg = pg;
}


void DemoSlider :: Changed(wSliderPositionMsg m)
{
	char sbuf[80];

	sprintf(sbuf, "Position Changed to: %ld\n", m.ulPixelsFromHome());
	demopg->AddNotifierMessage(sbuf);
}

void DemoSlider :: FocusGained(wControlMsg)
{
	demopg->AddNotifierMessage("Focus Gained\n");
}

void DemoSlider :: FocusLost(wControlMsg)
{
	demopg->AddNotifierMessage("Focus Lost\n");
}

void DemoSlider :: SliderDrag(wSliderPositionMsg m)
{
	char sbuf[80];

	sprintf(sbuf, "SliderDrag - pos: %ld\n", m.ulPixelsFromHome());
	demopg->AddNotifierMessage(sbuf);
}


/*****************************************************************************/
/* 
/* wEntry Demo
/* 
/*****************************************************************************/

EntryPage :: EntryPage(char *name)
	: ControlDemoPage(name, "Single and Multi Line wEntry Controls")
{
}

short EntryPage :: PageIn()
{
	ControlDemoPage::PageIn();

	AddMessageHandler(this, EVENTMETHOD(EntryPage, ProcNotifiers), 
	                  WM_CONTROL, UID_ENTRY);

	AddMessageHandler(this, EVENTMETHOD(EntryPage, ProcNotifiers), 
	                  WM_CONTROL, UID_MLE);

	entry = new wEntry(this, UID_ENTRY, wPointl(11, 108, DlgUnits), 
	                     wDimension(173, 10, DlgUnits), 	
						 EsLeftJust | EsMargin | EsAutoScroll);

	mle = new wMLEntry(this, UID_MLE, wPointl(11, 14, DlgUnits),
						wDimension(173, 85, DlgUnits), MlHscroll | MlVscroll);

	MenuBar()->SetSubMenuItems(MB_ACTIONS, MI_CUT, 
		"Cu~t;~Copy;~Paste;Cl~ear;-;~Undo;~Word Wrap;");

	entry->Show();
	mle->Show();
	Show();
	return TRUE;
}

short EntryPage :: PageOut()
{
	delete entry;
	ControlDemoPage::PageOut();
	return TRUE;
}

long  EntryPage :: ProcNotifiers(wControlMsg m)
{
	char sbuf[80];

	if (m.id() == UID_NOTIFY)
		return 0;

	if (m.id() == UID_ENTRY)
		switch (m.usNotifyCode())
		{
			case EN_SETFOCUS:
				sprintf(sbuf, "EN_SETFOCUS - Focus Gained\n");
				break;
			case EN_KILLFOCUS:
				sprintf(sbuf, "EN_KILLFOCUS - Focus Lost\n");
				break;
			case EN_CHANGE:
				sprintf(sbuf, "EN_CHANGE - Contents have changed\n");
				break;
			case EN_SCROLL:
				sprintf(sbuf, "EN_SCROLL - About to scroll\n");
				break;
      		case EN_MEMERROR:
				sprintf(sbuf, "EN_MEMERROR\n");
				break;
      		case EN_OVERFLOW:
				sprintf(sbuf, "EN_OVERFLOW - wField is full\n");
				break;
      		case EN_INSERTMODETOGGLE:
				sprintf(sbuf, "EN_INSERTMODETOGGLE\n");
				break;
			default:		 	
				return Default(m);
		}

	if (m.id() == UID_MLE)
		switch(m.usNotifyCode())
		{
			case MLN_OVERFLOW:
				sprintf(sbuf, "MLN_OVERFLOW\n");
				break;
			case MLN_PIXHORZOVERFLOW:
				sprintf(sbuf, "MLN_PIXHORZOVERFLOW\n");
				break;
			case MLN_PIXVERTOVERFLOW:
				sprintf(sbuf, "MLN_PIXVERTOVERFLOW\n");
				break;
			case MLN_TEXTOVERFLOW:
				sprintf(sbuf, "MLN_TEXTOVERFLOW - MLE is full\n");
				break;
			case MLN_VSCROLL:
				sprintf(sbuf, "MLN_VSCROLL\n");
				break;
			case MLN_HSCROLL:
				sprintf(sbuf, "MLN_HSCROLL\n");
				break;
			case MLN_CHANGE:
				sprintf(sbuf, "MLN_CHNGE - Contents have changed\n");
				break;
			case MLN_SETFOCUS:
				sprintf(sbuf, "MLN_SETFOCUS - Focus Gained\n");
				break;
			case MLN_KILLFOCUS:
				sprintf(sbuf, "MLN_KILLFOCUS - Focus Lost\n");
				break;
			case MLN_MARGIN:
				sprintf(sbuf, "MLN_MARGIN - Mouse move into margin\n");
				break;
			case MLN_SEARCHPAUSE:
				sprintf(sbuf, "MLN_SEARCHPAUSE\n");
				break;
			case MLN_MEMERROR:
				sprintf(sbuf, "MLN_MEMERROR\n");
				break;
			case MLN_UNDOOVERFLOW:
				sprintf(sbuf, "MLN_UNDOOVERFLOW - Too much to undo\n");
				break;
			case MLN_CLPBDFAIL:
				sprintf(sbuf, "MLN_CLPBDFAIL - Cut/Copy/Paste failed\n");
				break;
			default:		 	
				return Default(m);
		}
	AddNotifierMessage(sbuf);
	return FALSE;
}

long  EntryPage :: MenuAction(wMenuActionMsg m)
{
	if (m.actionType() == MENUINIT)
	{
		ushort iMenu = (m.usId() / 100) - 1;

		if (iMenu == 0)		// Action menu?
		{
			wRange r=entry->GetSelection();
			if ((r.High() - r.Low()) > 0)
			{
				MenuBar()->EnableItem(MI_CUT);
				MenuBar()->EnableItem(MI_COPY);
				MenuBar()->EnableItem(MI_PASTE);
				MenuBar()->EnableItem(MI_CLEAR);
			}
			else
			{
				MenuBar()->DisableItem(MI_CUT);
				MenuBar()->DisableItem(MI_COPY);
				MenuBar()->DisableItem(MI_PASTE);
				MenuBar()->DisableItem(MI_CLEAR);
			}
			long	stat = mle->GetUndoStatus();
			if (stat)
				MenuBar()->EnableItem(MI_UNDO);
			else
				MenuBar()->DisableItem(MI_UNDO);
		}
	}
	return m.actionType() == MENUSELECT;
}

long  EntryPage :: MenuCommand(wCommandMsg m)
{
	switch(m.usCmd())
	{
		case MI_CUT:
			entry->CutSelection();
			break;
		case MI_COPY:
			entry->CopySelection();
			break;
		case MI_PASTE:
			entry->PasteSelection();
			break;
		case MI_CLEAR:
			entry->ClearSelection();
			break;
		case MI_UNDO:
			mle->Undo();
			break;
		case MI_WORDWRAP:
			mle->SetWrap(!mle->GetWrap());
			MenuBar()->ToggleCheckItem(MI_WORDWRAP);
			break;
		default:
			return Default(m);
	}

	return FALSE;
}

/*****************************************************************************/
/* 
/* Value Set Demo
/* 
/*****************************************************************************/

ValueSetPage :: ValueSetPage(char *name)
	: ControlDemoPage(name, "Value Set Control")
{
}

short ValueSetPage :: PageIn()
{
	short	lp1, lp2, lp3;

	ControlDemoPage::PageIn();

	AddMessageHandler(this, EVENTMETHOD(ValueSetPage, ProcNotifiers), 
	                  WM_CONTROL, UID_VCOLOR);

	// create the color valueset control
	color = new wValueSet(this, UID_VCOLOR, wPointl(11,14,DlgUnits), 
		wDimension(173,115,DlgUnits), wDimension(8,2), 
		VsColorIndex | VsItemBorder | VsBorder);

	// setup the colors
	for (lp1 = 1, lp3 = 0 ; lp1 < 9 ; lp1++)
	{
		for (lp2 = 1 ; lp2 < 3 ; lp2++, lp3++)
			color->SetItem(bPosition(lp2, lp1),	
			               new wValueSetItem(wColor((long)lp3)));
	}
	color->Show();
	Show();
	return TRUE;
}

short ValueSetPage :: PageOut()
{
	delete color;
	ControlDemoPage::PageOut();
	return TRUE;
}

long  ValueSetPage :: ProcNotifiers(wControlMsg m)
{
	char 		sbuf[80];
	bPosition	rc;

	if (m.id() == UID_NOTIFY)
		return 0;
	
	switch (m.usNotifyCode())
	{
		case VN_ENTER:
			rc = color->GetSelectedPosition();
			sprintf(sbuf, "VN_ENTER - Item row %ld, column %ld)\n",
				rc.y(), rc.x());
			break;
		case VN_KILLFOCUS:
			strcpy(sbuf, "VN_KILLFOCUS - Focus lost\n");
			break;
		case VN_SETFOCUS:
			strcpy(sbuf, "VN_SETFOCUS - Focus gained\n");
			break;
		case VN_SELECT:
			rc = color->GetSelectedPosition();
			sprintf(sbuf, "VN_SELECT - Item selected row %ld, column %ld\n",
				rc.y(), rc.x());
			break;

		default:
			return Default(m);
	};

	AddNotifierMessage(sbuf);
	return FALSE;
}


/*****************************************************************************/
/* 
/* Spin Button Demo
/* 
/*****************************************************************************/

SpinPage :: SpinPage(char *name)
		  : ControlDemoPage(name, "Spin Button Controls")
{
}

short SpinPage :: PageIn()
{
	short	lp1, lp2, lp3;

	ControlDemoPage::PageIn();

	AddMessageHandler(this, EVENTMETHOD(SpinPage, ProcNotifiers), 
	                  WM_CONTROL, UID_ASPIN);
	AddMessageHandler(this, EVENTMETHOD(SpinPage, ProcNotifiers), 
	                  WM_CONTROL, UID_NSPIN);
	AddMessageHandler(this, EVENTMETHOD(SpinPage, ProcNotifiers), 
	                  WM_CONTROL, UID_MASTER);
	AddMessageHandler(this, EVENTMETHOD(SpinPage, ProcNotifiers), 
	                  WM_CONTROL, UID_SLAVE);

	// create the string spin button
	bStringList	sl(TRUE);
	sl.Insert(new bString("Apple"));
	sl.Insert(new bString("Banana"));
	sl.Insert(new bString("Cherry"));

	aspin = new wSpinButton(this, UID_ASPIN, wPointl(11,24,DlgUnits), 
				   wDimension(175,20,DlgUnits),
				   SbMaster | SbAllCharacters | WsVisible);

	aspin->SetStrings(&sl);

	// create the numeric spin button
	nspin = new wSpinButton(this, UID_NSPIN, wPointl(11,54,DlgUnits), 
				   wDimension(175,20,DlgUnits), 
				   SbMaster | SbNumericOnly | SbFastSpin | WsVisible);

	nspin->SetRange(wRange(-9999,9999));

	// create master-slave spin buttons
	slave = new wSpinButton(this, UID_SLAVE, wPointl(11,84,DlgUnits), 
		wDimension(87,20,DlgUnits),
		SbServant | WsVisible );

	slave->SetRange(wRange(-9999,9999));

	master = new wSpinButton(this, UID_MASTER, wPointl(98,84,DlgUnits), 
				 wDimension(88,20,DlgUnits), SbMaster );
	master->SetRange(wRange(-9999,9999));
	slave->SetMaster(master);

	atitle = new wTextBox(this, 999, wPointl(11,36,DlgUnits), 
				 wDimension(175,10,DlgUnits), "String Spin Button", WsVisible);

	ntitle = new wTextBox(this, 999, wPointl(11,66,DlgUnits), 
				 wDimension(175,10,DlgUnits), "Numeric Spin Button", WsVisible);

	mstitle = new wTextBox(this, 999, wPointl(11,96,DlgUnits), 
			      wDimension(175,10,DlgUnits), "Master-Slave Spin Buttons",
				  WsVisible);

	Show();
	return TRUE;
}

short SpinPage :: PageOut()
{
	delete aspin;
	delete nspin;
	delete master;
	delete slave;
	delete atitle;
	delete ntitle;
	delete mstitle;
	ControlDemoPage::PageOut();
	return TRUE;
}

long  SpinPage :: ProcNotifiers(wControlMsg m)
{
	char 		sbuf[80], *cp;

	if (m.id() == UID_NOTIFY)
		return 0;

	switch(m.id())
	{
		case UID_ASPIN:
			cp = "STRING";
			break;
		case UID_NSPIN:
			cp = "NUMERIC";
			break;
		case UID_MASTER:
			cp = "MASTER";
			break;
		case UID_SLAVE:
			cp = "SLAVE";
			break;
	}

	switch (m.usNotifyCode())
	{
		case SPBN_KILLFOCUS:
			sprintf(sbuf, "SPBN_KILLFOCUS - Focus lost - id: %s\n", cp);
			break;
		case SPBN_SETFOCUS:
			sprintf(sbuf, "SPBN_SETFOCUS - Focus gained - id: %s\n", cp);
			break;
		case SPBN_UPARROW:
			sprintf(sbuf, "SPBN_UPARROW - id: %s\n", cp);
			break;
		case SPBN_DOWNARROW:
			sprintf(sbuf, "SPBN_DOWNARROW - id: %s\n", cp);
			break;
		case SPBN_ENDSPIN:
			sprintf(sbuf, "SPBN_ENDSPIN - Mouse button released - id: %s\n", cp);
			break;
		case SPBN_CHANGE:
			sprintf(sbuf, "SPBN_CHANGE - Value changed - id: %s\n", cp);
			break;
		default:
			return Default(m);
	};

	AddNotifierMessage(sbuf);
	return FALSE;
}

/*****************************************************************************/
/* 
/* wCellBox Demo
/* 
/*****************************************************************************/


CellBoxPage :: CellBoxPage(char *name) 
             : ControlDemoPage(name, "CellBox Control")
{
}

short CellBoxPage :: PageIn()
{
	ControlDemoPage::PageIn();
    MenuBar()->SetSubMenuItems(MB_ACTIONS, MI_LBSTYLE, 
	      "Change Style...;Append Item;Insert Item;Delete Item;Search...;");

	AddMessageHandler(this, EVENTMETHOD(CellBoxPage, ProcNotifiers), 
	                  WM_CONTROL, UID_CELLBOX);
	AddMessageHandler(this, EVENTMETHOD(CellBoxPage, AddItem), 
	                  WM_COMMAND, MI_LBAPPEND);
	AddMessageHandler(this, EVENTMETHOD(CellBoxPage, AddItem), 
	                  WM_COMMAND, MI_LBINSERT);
	AddMessageHandler(this, EVENTMETHOD(CellBoxPage, DeleteItem), 
	                  WM_COMMAND, MI_LBDELETE);

	cb = new wCellBox(this, UID_CELLBOX, wPointl(13, 48, DlgUnits), 
	                 wDimension(174, 65, DlgUnits), WsVisible | LsVertScroll);
			 	
	cb->AddSubCol(68, 0L, ColDialog);
	cb->AddSubCol(53, 0L, ColDialog);
	cb->AddSubCol(53, 0L, ColDialog);

	cb->AddAttr(wColor(ClrRed));
	cb->AddAttr(wColor(ClrGreen));
	cb->AddAttr(Helv10, 0);
	cb->AddAttr(TmsRmn10, ItalicFont);

	for (nextitem = 0; nextitem < 20; nextitem++)
	{
		char sbuf[60];
		sprintf(sbuf,"!RItem #%d!T%sColumn 2!R!T%sColumn 3",
			nextitem, (nextitem % 2) ? "!A0" : "!A1",
			(nextitem % 2) ? "!A2" : "!A3");
		cb->AddString(sbuf);
	}

	Show();
	return TRUE;
}

short CellBoxPage :: PageOut()
{
	delete cb;
	ControlDemoPage::PageOut();
	return TRUE;
}

long  CellBoxPage :: ProcNotifiers(wControlMsg m)
{
	char sbuf[80];

	switch (m.usNotifyCode())
	{
		case LN_ENTER:
			sprintf(sbuf, "LN_ENTER - Item #%d\n", cb->GetSelectedIndex());
			break;

		case LN_KILLFOCUS:
			strcpy(sbuf, "LN_KILLFOCUS - Focus lost\n");
			break;

		case LN_SCROLL:
			strcpy(sbuf, "LN_SCROLL - Listbox scrolling...\n");
			break;

		case LN_SETFOCUS:
			strcpy(sbuf, "LN_SETFOCUS - Focus gained\n");
			break;

		case LN_SELECT:
			sprintf(sbuf, "LN_SELECT - Item #%d selected\n", cb->GetSelectedIndex());
			break;
	};
	AddNotifierMessage(sbuf);
	PaintStats();
	return FALSE;
}

long CellBoxPage :: MenuCommand(wCommandMsg)
{
	return FALSE;
}

// This method takes care of enabling/disabling the menu items

long CellBoxPage :: MenuAction(wMenuActionMsg m)
{
	if (m.actionType() == MENUINIT)
	{
		ushort iMenu = (m.usId() / 100) - 1;

		if (iMenu == 0)		// Action menu?
		{
			if (cb->GetSelectedIndex() >= 0)
			{
				MenuBar()->EnableItem(MI_LBINSERT);
				MenuBar()->EnableItem(MI_LBDELETE);
			}
			else
			{
				MenuBar()->DisableItem(MI_LBINSERT);
				MenuBar()->DisableItem(MI_LBDELETE);
			}
		}
	}
	return m.actionType() == MENUSELECT;
}

long CellBoxPage :: AddItem(wCommandMsg m)
{
	char sbuf[60];
	sprintf(sbuf,"!RItem #%d!T!A2%sColumn 2!R!T!A3Column 3",
		nextitem, (nextitem % 2) ? "!A2" : "!A3");
	if (m.usCmd() == MI_LBAPPEND)
		cb->AddString(sbuf);
	else
		cb->AddString(sbuf, cb->GetSelectedIndex());

	PaintStats();
	return FALSE;
}
	
long CellBoxPage :: DeleteItem(wCommandMsg)
{
	cb->DeleteItem(cb->GetSelectedIndex());
	PaintStats();
	return FALSE;
}

void CellBoxPage :: PaintStats(bool fPSGiven)
{
	wPresSpace *ps = fPSGiven? WindowPS(): new wPresSpace(this);
	
	wTextPen t(ps); 
	t.SetColor(ClrDarkBlue);
	t.SetBackColor(SysClrDialogBackground);
	t.SetBackMix(BmOverPaint);

	t.Printf(wPointl(13, 38, DlgUnits), "Item Count: %d  ", cb->GetItemCount());

	short item = cb->GetTopIndex();
    t.Printf(wPointl(93, 38,	DlgUnits), "Top Index: ");
 	if (item < 0)
		t.Display("N/A");
	else
		t.Printf("%d    ", item);

	item = cb->GetSelectedIndex();
    t.Printf(wPointl(13, 28,	DlgUnits), "Selected Index: ");
 	if (item < 0)
		t.Display("N/A");
	else
		t.Printf("%d    ", item);

	if (!fPSGiven)
		delete ps;
}
