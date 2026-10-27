//***************************************************************************
// tutor.cpp - My First PowerCode App
// 9/04/92 - 2:16:48pm
// John Doe
//***************************************************************************
//PROTECT_BEGIN AppRevisions 
//PROTECT_END 
#include "Tutor.hpp"				// Main App Class Definitions & Includes


//PROTECT_BEGIN GlobalVars 
//PROTECT_END 

// Pane Member Functions - Main 
MainPane::MainPane(zWindow *w) : zPane(w,new zSizeBelowToolBar(44)) 	 
{
	// PROTECT_BEGIN pMainConstructor 
	// PROTECT_END 
	WndBackColor = RGB(255,255,128);
	show();
}
//
// Frame Member Functions - Main 
//
// Window Constructor
Main::Main(zWindow *w, zString &title)
 : zAppFrame(w,new zSizer(),WS_OVERLAPPEDWINDOW, title) 
{
	wp = new MainPane(this);	 // Create Pane

	WndBackColor = RGB(255,255,128);
	menu(new zMenu(this, zResId("Main")));
	setIcon(new zIcon(zResId("Main")));

	// Create ToolBar
	ToolBar *pTB = new ToolBar(this, new zSizer(0,0,GetSystemMetrics(SM_CXSCREEN),44));	
	pTB->AddTool("Button", ID_MAINAPP, -1, -1, 45, 45, 
		 "NMainApp", "SMainApp", "Y", "Y");
	pTB->AddTool("Button", ID_MAINDIALOG, 43, -1, 45, 45, 
		 "NMainDialog", "SMainDialog", "Y", "Y");
	pTB->AddTool("Button", ID_MAINWINDOW, 87, -1, 45, 45, 
		 "NMainWindow", "SMainWindow", "Y", "Y");
	pTB->AddTool("Checkbox", ID_MAINCHECKBOX, 178, 2, 40, 40, 
		 "NMainCheckBox", "SMainCheckBox", "N", "N");
	pTB->AddTool("Button", ID_MAINSOUND, 131, -1, 45, 45, 
		 "NMainSound", "SMainSound", "Y", "Y");
	pTB->show();
	// PROTECT_BEGIN wMainConstructor 
	// PROTECT_END 
}
int Main::command(zCommandEvt *Ev) 
{
	switch (Ev->cmd())
	{
		case IDM_NEW:
		{
			NewWin* p=new NewWin(this, zString(""));
			p->show();
			//PROTECT_BEGIN wMainNew 
			//PROTECT_END 
		}
		break;  
		case IDM_OPEN:
		{
			char *types[]=
			{ 
				"All Files (*.*)","*.*",
				"Text Files (*.txt)","*.txt",
				0,0 
			};
			zFileOpenForm *p = new zFileOpenForm(this,"Open File", NULL, types);
			if (p->completed()) 
			{
				// Use p->name() to retrieve filename
				//PROTECT_BEGIN wMainOpen 
				//PROTECT_END 
			}
			delete p;
		}
		break;  
		case IDM_SAVE:
		{
			char *types[]=
			{ 
				"All Files (*.*)","*.*",
				"Text Files (*.txt)","*.txt",
				0,0 
			};
			zFileSaveAsForm *p = new zFileSaveAsForm(this,"Save File As", NULL, types);
			if (p->completed()) 
			{
				// Use p->name() to retrieve filename
				//PROTECT_BEGIN wMainSave 
				//PROTECT_END 
			}
			delete p;
		}
		break;  
		case IDM_PRINT:
		{
			PRINTDLG pd;
			memset(&pd, 0, sizeof( PRINTDLG ));
			pd.lStructSize = sizeof( PRINTDLG );
			pd.hwndOwner=(HWND)*this;	 
			pd.Flags=PD_RETURNDC;
			if (PrintDlg(&pd))
			{
				//PROTECT_BEGIN wMainPrint 
				//PROTECT_END 
			}
		}
		break;  
		case IDM_EXIT:
		{
			//PROTECT_BEGIN wMainExit 
			//PROTECT_END 
		}
		break;  
		case IDM_ABOUT:
		{
			About* p=new About(this, zResId("ABOUT"));
			p->show();
			p->modal();
			delete p;		 // Modal Dialogs don't delete themselves
			//PROTECT_BEGIN wMainAbout 
			//PROTECT_END 
		}
		break;  
		case ID_MAINAPP:
		{
			//PROTECT_BEGIN wMainApp 
			//PROTECT_END 
			WinExec("notepad.exe tutor.txt", SW_SHOW);
		}
		break;  
		case ID_MAINDIALOG:
		{
			Address* p=new Address(this, zResId("ADDRESS"));
			p->show();
			//PROTECT_BEGIN wMainDialog 
			//PROTECT_END 
		}
		break;  
		case ID_MAINWINDOW:
		{
			NewWin* p=new NewWin(this, zString(""));
			p->show();
			//PROTECT_BEGIN wMainWindow 
			//PROTECT_END 
		}
		break;  
		case ID_MAINSOUND:
		{
			sndPlaySound("tada.wav", SND_SYNC);
			//PROTECT_BEGIN wMainSound 
			//PROTECT_END 
		}
		break;  
		// PROTECT_BEGIN wMainCommand 
		// PROTECT_END 
		default:
			return 0;		 // Msg not processed
	}
	return 1;
}

// Pane Member Functions - NewWin 
NewWinPane::NewWinPane(zWindow *w) : zPane(w,new zSizeWithParent) 	 
{
	// PROTECT_BEGIN pNewWinConstructor 
	// PROTECT_END 
	show();
}
//
// Frame Member Functions - NewWin 
//
// Window Constructor
// The default sizer doesn't work here...so create win at prototyped size
NewWin::NewWin(zWindow *w, zString &title)
 : zDialogFrame(w,new zSizer(CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT),WS_OVERLAPPEDWINDOW, title) 
{
	wp = new NewWinPane(this);	 // Create Pane

	// PROTECT_BEGIN wNewWinConstructor 
	// PROTECT_END 
}
int NewWin::command(zCommandEvt *Ev) 
{
	switch (Ev->cmd())
	{
		// PROTECT_BEGIN wNewWinCommand 
		// PROTECT_END 
		default:
			return 0;		 // Msg not processed
	}
	return 1;
}

//
// Dialog Member Functions - About 
//
About::About(zWindow *w,zResId& rid) : zDialogFrame(w,rid)
{
	//PROTECT_BEGIN dABOUTConstructor 
	//PROTECT_END 
}
//
// command - Distribute and handle command msgs 
//
int About::command(zCommandEvt *Ev) {
	switch (Ev->cmd()) 
	{
		case IDOK:
			if (CanClose())
			{
				//PROTECT_BEGIN dABOUTIDOK 
				//PROTECT_END 
				ok=TRUE;
				shutdown();
			}
			break;
		case IDCANCEL:
			//PROTECT_BEGIN dABOUTIDCANCEL 
			//PROTECT_END 
			ok=FALSE;
			shutdown();
			break;
		//PROTECT_BEGIN dABOUTCommand 
		//PROTECT_END 
		default:
			return 0;		 // Msg not processed
	}
	return 1;
}
//
// CanClose - Determine if dialog should be allowed to update
//
BOOL About::CanClose()
{
	return TRUE;
}
//
// Dialog Member Functions - Address 
//
Address::Address(zWindow *w,zResId& rid) : zDialogFrame(w,rid)
{
	Lname = new zEditLine(this, ID_LNAME);
	Lname->limitText(30);
	Name = new zEditLine(this, ID_NAME);
	Title = new zEditLine(this, ID_TITLE);
	Coname = new zEditLine(this, ID_CONAME);
	Street1 = new zEditLine(this, ID_STREET1);
	Street2 = new zEditLine(this, ID_STREET2);
	City = new zEditLine(this, ID_CITY);
	State = new zEditLine(this, ID_STATE);
	Zip = new zEditLine(this, ID_ZIP);
	Bphone = new zEditLine(this, ID_BPHONE);
	Hphone = new zEditLine(this, ID_HPHONE);
	Fax = new zEditLine(this, ID_FAX);
	Comments = new zEditLine(this, ID_COMMENTS);
	Addtype = new zComboBox(this, ID_ADDTYPE);
	Male = new zRadioButton(this, ID_MALE);
	Female = new zRadioButton(this, ID_FEMALE);
	Female->check(TRUE);	 // Set Initial Check
	// Fill Combo with initial values
	Addtype->add("Employee");
	Addtype->add("Vendor");
	Addtype->add("Friend");
	Addtype->add("Other");
	//PROTECT_BEGIN dADDRESSConstructor 
	//PROTECT_END 
}
//
// command - Distribute and handle command msgs 
//
int Address::command(zCommandEvt *Ev) {
	switch (Ev->cmd()) 
	{
		case IDOK:
			if (CanClose())
			{
				//PROTECT_BEGIN dADDRESSIDOK 
				//PROTECT_END 
				ok=TRUE;
				shutdown();
			}
			break;
		case IDCANCEL:
			//PROTECT_BEGIN dADDRESSIDCANCEL 
			//PROTECT_END 
			ok=FALSE;
			shutdown();
			break;
		case ID_SEARCH:
		{
			Search* p=new Search(this, zResId("SEARCH"));
			p->show();
			p->modal();
			delete p;		 // Modal Dialogs don't delete themselves
			//PROTECT_BEGIN dADDRESSSearch 
			//PROTECT_END 
		}
		break;
		//PROTECT_BEGIN dADDRESSCommand 
		//PROTECT_END 
		default:
			return 0;		 // Msg not processed
	}
	return 1;
}
//
// CanClose - Determine if dialog should be allowed to update
//
BOOL Address::CanClose()
{
	if ( Lname->text().length() == 0 )
	{
		zMessage *msg = new zMessage(this, "Mandatory field requires input", 
			"Info",	MB_OK | MB_ICONINFORMATION);
		delete msg;
		Lname->setFocus();	 // Position cursor to error
		return FALSE;
	}
	return TRUE;
}
//
// Dialog Member Functions - Search 
//
Search::Search(zWindow *w,zResId& rid) : zDialogFrame(w,rid)
{
	List = new zListBox(this, ID_LIST);
	//PROTECT_BEGIN dSEARCHConstructor 
	//PROTECT_END 
}
//
// command - Distribute and handle command msgs 
//
int Search::command(zCommandEvt *Ev) {
	switch (Ev->cmd()) 
	{
		case IDOK:
			if (CanClose())
			{
				//PROTECT_BEGIN dSEARCHIDOK 
				//PROTECT_END 
				ok=TRUE;
				shutdown();
			}
			break;
		case IDCANCEL:
			//PROTECT_BEGIN dSEARCHIDCANCEL 
			//PROTECT_END 
			ok=FALSE;
			shutdown();
			break;
		//PROTECT_BEGIN dSEARCHCommand 
		//PROTECT_END 
		default:
			return 0;		 // Msg not processed
	}
	return 1;
}
//
// CanClose - Determine if dialog should be allowed to update
//
BOOL Search::CanClose()
{
	return TRUE;
}

//PROTECT_BEGIN AppUserCode 
//PROTECT_END 

//
//	Application Member Functions
//
void zApp::main() 
{
	//PROTECT_BEGIN AppMain 
	//PROTECT_END 
	loadAccelerator(zResId("Tutor"));
	Main* p=new Main(0,zString("My First Object/Designer App"));
	p->show();
	go();
	delete p;
}
