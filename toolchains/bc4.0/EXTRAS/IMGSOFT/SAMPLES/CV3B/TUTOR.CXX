//***************************************************************************
// tutor.cxx - My First PowerCode App
// 9/04/92 - 2:36:28pm
// John Doe
//***************************************************************************
//PROTECT_BEGIN AppRevisions 
//PROTECT_END 
#include "Tutor.hxx"				// Main App Class Definitions

//PROTECT_BEGIN GlobalVars 
//PROTECT_END 

App *ThisApp;			// Pointer to reference App::Run() etc...

//
// Frame Window Member Functions - Main 
//
// Window Constructor
MainWin::MainWin() : TopAppWindow() 
 ,MainMenu(ResID(IDM_MAIN))	
 ,MainIcon(ResID(IDI_MAIN))	
 ,MainAccel(ResID(IDA_MAIN))
{
	SetCaption("My First Object/Designer App");
	ChangeBackground(new Brush(Color(255, 255, 128)));
	ChangeMenu(&MainMenu);
	ChangeAccel(&MainAccel);

	SetIcon(&MainIcon);
	// Create ToolBar
 	pTB = new ToolBar(this, 44);	
	pTB->AddTool("Button", IDTI_MAINAPP, Point(-1, 45), 
		Dimension(45, 45), ResID(IDNB_MAINAPP), 
		ResID(IDSB_MAINAPP), "Y", "Y");
	pTB->AddTool("Button", IDTI_MAINDIALOG, Point(43, 45), 
		Dimension(45, 45), ResID(IDNB_MAINDIALOG), 
		ResID(IDSB_MAINDIALOG), "Y", "Y");
	pTB->AddTool("Button", IDTI_MAINWINDOW, Point(87, 45), 
		Dimension(45, 45), ResID(IDNB_MAINWINDOW), 
		ResID(IDSB_MAINWINDOW), "Y", "Y");
	pTB->AddTool("Checkbox", IDTI_MAINCHECKBOX, Point(178, 42), 
		Dimension(40, 40), ResID(IDNB_MAINCHECKBOX), 
		ResID(IDSB_MAINCHECKBOX), "N", "N");
	pTB->AddTool("Button", IDTI_MAINSOUND, Point(131, 45), 
		Dimension(45, 45), ResID(IDNB_MAINSOUND), 
		ResID(IDSB_MAINSOUND), "Y", "Y");
	pTB->Show();

	// PROTECT_BEGIN wMainConstructor 
	// PROTECT_END 
	Show();		// Display Window
}
// 
// Window Destructor
//
MainWin::~MainWin()
{
	if (pTB)			 	// Delete allocated ToolBar
		delete pTB;
}
// 
// Process Menu and Toolbar commands
//
void MainWin::MenuCommand( MenuCommandEvt e ) 
{
	switch (e.GetItemID())
	{
		case IDMI_NEW:
		{
			NewWinWin* p=new NewWinWin();
			p->Show();
			// PROTECT_BEGIN wMainMenuNew 
			// PROTECT_END 
		}
		break;  
		case IDMI_OPEN:
		{
			OPENFILENAME of;
			// Filter Type & Extension...separate strings with NULL byte
			char szFilter[256] = "All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0"; 
			char szFile[256] = ""; 
			memset(&of, 0, sizeof( OPENFILENAME ));
			of.lStructSize = sizeof( OPENFILENAME );
			of.lpstrFilter = szFilter;
			of.lpstrFile = szFile;
			of.nMaxFile = sizeof(szFile);
			of.hwndOwner=Handle();
			if (!GetOpenFileName(&of))
				return;
			// PROTECT_BEGIN wMainMenuOpen 
			// PROTECT_END 
		}
		break;  
		case IDMI_SAVE:
		{
			OPENFILENAME of;
			// Filter Type & Extension...separate strings with NULL byte
			char szFilter[256] = "All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0"; 
			char szFile[256] = ""; 
			memset(&of, 0, sizeof( OPENFILENAME ));
			of.lStructSize = sizeof( OPENFILENAME );
			of.lpstrFilter = szFilter;
			of.lpstrFile = szFile;
			of.nMaxFile = sizeof(szFile);
			of.hwndOwner=Handle();
			if (!GetSaveFileName(&of))
				return;
			// PROTECT_BEGIN wMainMenuSave 
			// PROTECT_END 
		}
		break;  
		case IDMI_PRINT:
		{
			PRINTDLG pd;
			memset(&pd, 0, sizeof( PRINTDLG ));
			pd.lStructSize = sizeof( PRINTDLG );
			pd.hwndOwner=Handle();
			pd.Flags=PD_RETURNDC;
			if (!PrintDlg(&pd))
				return;
			// PROTECT_BEGIN wMainMenuPrint 
			// PROTECT_END 
		}
		break;  
		case IDMI_EXIT:
		{
			// PROTECT_BEGIN wMainMenuExit 
			// PROTECT_END 
		}
		break;  
		case IDMI_ABOUT:
		{
			About* p=new About(this);
			p->Show();
			delete p;		 // Modal Dialogs don't delete themselves
			// PROTECT_BEGIN wMainMenuAbout 
			// PROTECT_END 
		}
		break;  
		case IDTI_MAINAPP:
		{
			GetApp()->Run("notepad.exe tutor.txt");
			// PROTECT_BEGIN wMainToolApp 
			// PROTECT_END 
		}
		break;  
		case IDTI_MAINDIALOG:
		{
			Address* p=new Address(this);
			p->Show();
			// PROTECT_BEGIN wMainToolDialog 
			// PROTECT_END 
		}
		break;  
		case IDTI_MAINWINDOW:
		{
			NewWinWin* p=new NewWinWin();
			p->Show();
			// PROTECT_BEGIN wMainToolWindow 
			// PROTECT_END 
		}
		break;  
		case IDTI_MAINSOUND:
		{
			sndPlaySound("tada.wav", 0);
			// PROTECT_BEGIN wMainToolSound 
			// PROTECT_END 
		}
		break;  
		// PROTECT_BEGIN wMainMenuCommand 
		// PROTECT_END 
		default:
			Window::MenuCommand(e);	 // Pass msg on
	}
}

//
// Frame Window Member Functions - NewWin 
//
// Window Constructor
NewWinWin::NewWinWin() : TopAppWindow() 
{

	// PROTECT_BEGIN wNewWinConstructor 
	// PROTECT_END 
	Show();		// Display Window
}
// 
// Window Destructor
//
NewWinWin::~NewWinWin()
{
}
// 
// Process Menu and Toolbar commands
//
void NewWinWin::MenuCommand( MenuCommandEvt e ) 
{
	switch (e.GetItemID())
	{
		// PROTECT_BEGIN wNewWinMenuCommand 
		// PROTECT_END 
		default:
			Window::MenuCommand(e);	 // Pass msg on
	}
}

//
// Dialog Member Functions - About 
//
About::About(pWindow Parent) 
: DialogWindow (Parent, ResID(ABOUT))
{

	//PROTECT_BEGIN dABOUTConstructor 
	//PROTECT_END 
}
// 
// Dialog Destructor
//
About::~About() 
{
}
//
// Distribute and handle Button click events 
//
void About::ButtonClick( ControlEvt e )
{
	switch (uint(e.GetControlID()))
	{
		// PROTECT_BEGIN dABOUTButtonClick 
		// PROTECT_END 
		case IDOK:
			if (CanClose())
			{
				//PROTECT_BEGIN dABOUTButtonOK 
				//PROTECT_END 
				EndDlg(TRUE);
			}
			break;
		case IDCANCEL:
			//PROTECT_BEGIN dABOUTButtonCancel 
			//PROTECT_END 
			EndDlg(FALSE);
			break;
		default:
			return;
	}
}
//
// Tool Item selections are treated like a menu command 
//
void About::MenuCommand( MenuCommandEvt e ) 
{
	switch (e.GetItemID())
	{
		// PROTECT_BEGIN dABOUTMenuCommand 
		// PROTECT_END 
		default:
			Window::MenuCommand(e);	 // Pass msg on
	}
}
//
// CanClose - Determine if dialog should be allowed to update
//
Bool About::CanClose()
{
	//PROTECT_BEGIN dABOUTCanClose 
	//PROTECT_END 
	return TRUE;
}
//
// Dialog Member Functions - Address 
//
Address::Address(pWindow Parent) 
: ModeLessDialog (Parent, ResID(ADDRESS))
{
	Lname = new SingleLineEdit(this, ID_LNAME);
	Lname->SetTextLimit(30);
	Name = new SingleLineEdit(this, ID_NAME);
	Title = new SingleLineEdit(this, ID_TITLE);
	Coname = new SingleLineEdit(this, ID_CONAME);
	Street1 = new SingleLineEdit(this, ID_STREET1);
	Street2 = new SingleLineEdit(this, ID_STREET2);
	City = new SingleLineEdit(this, ID_CITY);
	State = new SingleLineEdit(this, ID_STATE);
	Zip = new SingleLineEdit(this, ID_ZIP);
	Bphone = new SingleLineEdit(this, ID_BPHONE);
	Hphone = new SingleLineEdit(this, ID_HPHONE);
	Fax = new SingleLineEdit(this, ID_FAX);
	Comments = new SingleLineEdit(this, ID_COMMENTS);
	Addtype = new ComboBox(this, ID_ADDTYPE);
	Male = new RadioButton(this, ID_MALE);
	Female = new RadioButton(this, ID_FEMALE);

	Female->SetState(TRUE);	 // Set Initial Check
	// Fill Combo with initial values
	Addtype->AddString("Employee");
	Addtype->AddString("Vendor");
	Addtype->AddString("Friend");
	Addtype->AddString("Other");
	//PROTECT_BEGIN dADDRESSConstructor 
	//PROTECT_END 
}
// 
// Dialog Destructor
//
Address::~Address() 
{
}
//
// Distribute and handle Button click events 
//
void Address::ButtonClick( ControlEvt e )
{
	switch (uint(e.GetControlID()))
	{
		case ID_SEARCH:
		{
			Search* p=new Search(this);
			p->Show();
			delete p;		 // Modal Dialogs don't delete themselves
			//PROTECT_BEGIN dADDRESSButtonID_SEARCH 
			//PROTECT_END 
		}
		break;  
		// PROTECT_BEGIN dADDRESSButtonClick 
		// PROTECT_END 
		case IDOK:
			if (CanClose())
			{
				//PROTECT_BEGIN dADDRESSButtonOK 
				//PROTECT_END 
				EndDlg(TRUE);
			}
			break;
		case IDCANCEL:
			//PROTECT_BEGIN dADDRESSButtonCancel 
			//PROTECT_END 
			EndDlg(FALSE);
			break;
		default:
			return;
	}
}
//
// Tool Item selections are treated like a menu command 
//
void Address::MenuCommand( MenuCommandEvt e ) 
{
	switch (e.GetItemID())
	{
		// PROTECT_BEGIN dADDRESSMenuCommand 
		// PROTECT_END 
		default:
			Window::MenuCommand(e);	 // Pass msg on
	}
}
//
// CanClose - Determine if dialog should be allowed to update
//
Bool Address::CanClose()
{
	if ( Lname->GetTextLen() == 0 )
	{
		MessBox *Msg = new MessBox("Info", "Mandatory field requires input", this); 
		Msg->Show();
		delete Msg;
		Lname->SetFocus();	 // Position cursor to error
		return FALSE;
	}
	//PROTECT_BEGIN dADDRESSCanClose 
	//PROTECT_END 
	return TRUE;
}
//
// Dialog Member Functions - Search 
//
Search::Search(pWindow Parent) 
: DialogWindow (Parent, ResID(SEARCH))
{
	List = new ListBox(this, ID_LIST);

	//PROTECT_BEGIN dSEARCHConstructor 
	//PROTECT_END 
}
// 
// Dialog Destructor
//
Search::~Search() 
{
}
//
// Distribute and handle Button click events 
//
void Search::ButtonClick( ControlEvt e )
{
	switch (uint(e.GetControlID()))
	{
		// PROTECT_BEGIN dSEARCHButtonClick 
		// PROTECT_END 
		case IDOK:
			if (CanClose())
			{
				//PROTECT_BEGIN dSEARCHButtonOK 
				//PROTECT_END 
				EndDlg(TRUE);
			}
			break;
		case IDCANCEL:
			//PROTECT_BEGIN dSEARCHButtonCancel 
			//PROTECT_END 
			EndDlg(FALSE);
			break;
		default:
			return;
	}
}
//
// Tool Item selections are treated like a menu command 
//
void Search::MenuCommand( MenuCommandEvt e ) 
{
	switch (e.GetItemID())
	{
		// PROTECT_BEGIN dSEARCHMenuCommand 
		// PROTECT_END 
		default:
			Window::MenuCommand(e);	 // Pass msg on
	}
}
//
// CanClose - Determine if dialog should be allowed to update
//
Bool Search::CanClose()
{
	//PROTECT_BEGIN dSEARCHCanClose 
	//PROTECT_END 
	return TRUE;
}

//PROTECT_BEGIN AppUserCode 
//PROTECT_END 

//
//	Application Member Functions
//
App *GetApp()
{
	return ThisApp;
}

void App::Start(const int, const char*[])
{
	//PROTECT_BEGIN AppStart 
	//PROTECT_END 
	ThisApp = this;	 // Save global pointer to app
	MainWin MainWin;
	Exec();
}
