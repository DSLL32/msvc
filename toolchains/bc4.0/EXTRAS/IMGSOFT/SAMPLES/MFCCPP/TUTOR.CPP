//***************************************************************************
// tutor.cpp - My First PowerCode App
// 9/04/92 - 2:15:09pm
// John Doe
//***************************************************************************
//PROTECT_BEGIN AppRevisions 
//PROTECT_END 
#include "Tutor.hpp"				// Main App Class Definitions

TutorApp theApp;		 // Create and run application

//PROTECT_BEGIN GlobalVars 
//PROTECT_END 

//
// Main message map:
// Associate messages with appropriate member functions.
//
BEGIN_MESSAGE_MAP( Main, CFrameWnd )
	ON_WM_DESTROY()	 //Make sure destructor is called
	ON_COMMAND( IDM_NEW, OnNew )
	ON_COMMAND( IDM_OPEN, OnOpen )
	ON_COMMAND( IDM_SAVE, OnSave )
	ON_COMMAND( IDM_PRINT, OnPrint )
	ON_COMMAND( IDM_EXIT, OnExit )
	ON_COMMAND( IDM_ABOUT, OnAbout )
	ON_COMMAND( IDT_MAINAPP, OnMainApp )
	ON_COMMAND( IDT_MAINDIALOG, OnMainDialog )
	ON_COMMAND( IDT_MAINWINDOW, OnMainWindow )
	ON_COMMAND( IDT_MAINSOUND, OnMainSound )
	// PROTECT_BEGIN wMainMsgMap 
	// PROTECT_END 
END_MESSAGE_MAP()
//
// Member Functions - Main 
//
// Window Constructor
Main::Main(CWnd* Parent) 
{
	// Create Window with previously defined window class
	if (!Create("Main", "My First Object/Designer App",
		WS_OVERLAPPEDWINDOW, 
		rectDefault,
		Parent, "Main"))
		 AfxAbort();
	// Create ToolBar
	pToolbar = new TToolBar(this, 44);	
	pToolbar->Add("Button", IDT_MAINAPP, -1, -1, 45, 45, 
		 "NMainApp", "SMainApp", "Y", "Y");
	pToolbar->Add("Button", IDT_MAINDIALOG, 43, -1, 45, 45, 
		 "NMainDialog", "SMainDialog", "Y", "Y");
	pToolbar->Add("Button", IDT_MAINWINDOW, 87, -1, 45, 45, 
		 "NMainWindow", "SMainWindow", "Y", "Y");
	pToolbar->Add("Checkbox", IDT_MAINCHECKBOX, 178, 2, 40, 40, 
		 "NMainCheckBox", "SMainCheckBox", "N", "N");
	pToolbar->Add("Button", IDT_MAINSOUND, 131, -1, 45, 45, 
		 "NMainSound", "SMainSound", "Y", "Y");
	LoadAccelTable("Main");	 
	// PROTECT_BEGIN wMainConstructor 
	// PROTECT_END 
}
// Window Destructor
Main::~Main() 
{
	delete pToolbar;	// Delete tool bar object
	// PROTECT_BEGIN wMainDestructor 
	// PROTECT_END 
}
//
// Window Member Response functions
//
void Main::OnMainApp()
{
	//PROTECT_BEGIN wMainMainApp 
	//PROTECT_END 
	WinExec("notepad.exe tutor.txt", SW_SHOW);
}
void Main::OnMainDialog()
{
	// Create modeless dialog from heap - it will delete itself
	Address *pDlg = new Address(this);
	pDlg->ShowWindow(SW_SHOW);
	//PROTECT_BEGIN wMainMainDialog 
	//PROTECT_END 
}
void Main::OnMainWindow()
{
	CWnd *pWin = new NewWin(this);
	pWin->ShowWindow(SW_SHOW);
	pWin->UpdateWindow();
	//PROTECT_BEGIN wMainMainWindow 
	//PROTECT_END 
}
void Main::OnMainSound()
{
	sndPlaySound("tada.wav", SND_SYNC);
	//PROTECT_BEGIN wMainMainSound 
	//PROTECT_END 
}
void Main::OnNew()
{
	CWnd *pWin = new NewWin(this);
	pWin->ShowWindow(SW_SHOW);
	pWin->UpdateWindow();
	//PROTECT_BEGIN wMainNew 
	//PROTECT_END 
}
void Main::OnOpen()
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
	of.hwndOwner=m_hWnd;
	if (GetOpenFileName(&of))
	{
		//PROTECT_BEGIN wMainOpen 
		//PROTECT_END 
	}
}
void Main::OnSave()
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
	of.hwndOwner=m_hWnd;
	if (GetSaveFileName(&of))
	{
		//PROTECT_BEGIN wMainSave 
		//PROTECT_END 
	}
}
void Main::OnPrint()
{
	PRINTDLG pd;
	memset(&pd, 0, sizeof( PRINTDLG ));
	pd.lStructSize = sizeof( PRINTDLG );
	pd.hwndOwner=m_hWnd;
	pd.Flags=PD_RETURNDC;
	if (PrintDlg(&pd))
	{
		//PROTECT_BEGIN wMainPrint 
		//PROTECT_END 
	}
}
void Main::OnExit()
{
	//PROTECT_BEGIN wMainExit 
	//PROTECT_END 
}
void Main::OnAbout()
{
	// Create and display modal dialog
	About About(this);
	About.DoModal();
	//PROTECT_BEGIN wMainAbout 
	//PROTECT_END 
}
//
// NewWin message map:
// Associate messages with appropriate member functions.
//
BEGIN_MESSAGE_MAP( NewWin, CFrameWnd )
	ON_WM_DESTROY()	 //Make sure destructor is called
	// PROTECT_BEGIN wNewWinMsgMap 
	// PROTECT_END 
END_MESSAGE_MAP()
//
// Member Functions - NewWin 
//
// Window Constructor
NewWin::NewWin(CWnd* Parent) 
{
	// Create Window with previously defined window class
	if (!Create("NewWin", "",
		WS_OVERLAPPEDWINDOW, 
		rectDefault,
		Parent, NULL))
		 AfxAbort();
	// PROTECT_BEGIN wNewWinConstructor 
	// PROTECT_END 
}
// Window Destructor
NewWin::~NewWin() 
{
	// PROTECT_BEGIN wNewWinDestructor 
	// PROTECT_END 
}
//
// Window Member Response functions
//
//
// About message map:
// Associate messages with appropriate member functions.
//
BEGIN_MESSAGE_MAP( About, CModalDialog )
	ON_COMMAND(IDOK, OnOK )
	//PROTECT_BEGIN dAboutMsgMap 
	//PROTECT_END 
END_MESSAGE_MAP()
//
// Dialog Member Functions - About 
//
About::About(CWnd* pParentWnd) 
	: CModalDialog( "ABOUT", pParentWnd )
{
	//PROTECT_BEGIN dAboutConstructor 
	//PROTECT_END 
}
// Dialog Destructor
About::~About() 
{
	//PROTECT_BEGIN dAboutDestructor 
	//PROTECT_END 
}
// 
//OnInitDialog - Initialize dialog controls and data
//
BOOL About::OnInitDialog()
{
	// Call base class for proper initialization.
	if (!CDialog::OnInitDialog())
		return FALSE;


	//PROTECT_BEGIN dAboutOnInitDialog 
	//PROTECT_END 
	return TRUE;
}
//
// Dialog Response functions - About
//
//
// CanClose - Determine if dialog should be allowed to update
//
BOOL About::CanClose()
{
	//PROTECT_BEGIN dAboutCanClose 
	//PROTECT_END 
	return TRUE;
}
//
// OnOK:
// If CanClose() then update data from dialog
//
void About::OnOK()
{
	if ( !CanClose() ) // Only Update Dialog if CanClose is TRUE
		return;
	
	CModalDialog::OnOK();
	//PROTECT_BEGIN dAboutOnOK 
	//PROTECT_END 
}
//
// Address message map:
// Associate messages with appropriate member functions.
//
BEGIN_MESSAGE_MAP( Address, CDialog )
	ON_WM_DESTROY()	 //Make sure destructor is called
	ON_COMMAND(IDCANCEL, OnCancel )
	ON_COMMAND(IDOK, OnOK )
	ON_BN_CLICKED(ID_SEARCH, OnBtnSearch )
	//PROTECT_BEGIN dAddressMsgMap 
	//PROTECT_END 
END_MESSAGE_MAP()
//
// Dialog Member Functions - Address 
//
Address::Address(CWnd* pParentWnd) 
	: CDialog()
{
	if (!Create("ADDRESS", pParentWnd))
		 AfxAbort();
	//PROTECT_BEGIN dAddressConstructor 
	//PROTECT_END 
}
// Dialog Destructor
Address::~Address() 
{
	//PROTECT_BEGIN dAddressDestructor 
	//PROTECT_END 
}
// 
//OnInitDialog - Initialize dialog controls and data
//
BOOL Address::OnInitDialog()
{
	// Call base class for proper initialization.
	if (!CDialog::OnInitDialog())
		return FALSE;

	SendDlgItemMessage(ID_LNAME, EM_LIMITTEXT, 30, 0);
	CheckDlgButton(ID_FEMALE, TRUE);	 // Set Initial Check
	// Fill Combo with initial values
	CComboBox *Addtype = (CComboBox *) GetDlgItem(ID_ADDTYPE);
	Addtype->AddString("Employee");
	Addtype->AddString("Vendor");
	Addtype->AddString("Friend");
	Addtype->AddString("Other");

	//PROTECT_BEGIN dAddressOnInitDialog 
	//PROTECT_END 
	return TRUE;
}
//
// Dialog Response functions - Address
//
void Address::OnBtnSearch()
{
	// Create and display modal dialog
	Search Search(this);
	Search.DoModal();
	//PROTECT_BEGIN dAddressSearch 
	//PROTECT_END 
}
//
// CanClose - Determine if dialog should be allowed to update
//
BOOL Address::CanClose()
{
	if ( GetDlgItemText(ID_LNAME, szWorkStr, sizeof(szWorkStr)) == 0 )
	{
		MessageBox("Mandatory field requires input.", "Error"); 
		::SetFocus(::GetDlgItem(m_hWnd, ID_LNAME));	 // Position cursor to error
		return FALSE;
	}
	//PROTECT_BEGIN dAddressCanClose 
	//PROTECT_END 
	return TRUE;
}
//
// OnOK:
// If CanClose() then update data from dialog
//
void Address::OnOK()
{
	if ( !CanClose() ) // Only Update Dialog if CanClose is TRUE
		return;
	
	DestroyWindow();	// End Modeless Dialog
	//PROTECT_BEGIN dAddressOnOK 
	//PROTECT_END 
}
//
// Search message map:
// Associate messages with appropriate member functions.
//
BEGIN_MESSAGE_MAP( Search, CModalDialog )
	ON_COMMAND(IDOK, OnOK )
	//PROTECT_BEGIN dSearchMsgMap 
	//PROTECT_END 
END_MESSAGE_MAP()
//
// Dialog Member Functions - Search 
//
Search::Search(CWnd* pParentWnd) 
	: CModalDialog( "SEARCH", pParentWnd )
{
	//PROTECT_BEGIN dSearchConstructor 
	//PROTECT_END 
}
// Dialog Destructor
Search::~Search() 
{
	//PROTECT_BEGIN dSearchDestructor 
	//PROTECT_END 
}
// 
//OnInitDialog - Initialize dialog controls and data
//
BOOL Search::OnInitDialog()
{
	// Call base class for proper initialization.
	if (!CDialog::OnInitDialog())
		return FALSE;


	//PROTECT_BEGIN dSearchOnInitDialog 
	//PROTECT_END 
	return TRUE;
}
//
// Dialog Response functions - Search
//
//
// CanClose - Determine if dialog should be allowed to update
//
BOOL Search::CanClose()
{
	//PROTECT_BEGIN dSearchCanClose 
	//PROTECT_END 
	return TRUE;
}
//
// OnOK:
// If CanClose() then update data from dialog
//
void Search::OnOK()
{
	if ( !CanClose() ) // Only Update Dialog if CanClose is TRUE
		return;
	
	CModalDialog::OnOK();
	//PROTECT_BEGIN dSearchOnOK 
	//PROTECT_END 
}

//PROTECT_BEGIN AppUserCode 
//PROTECT_END 

//
//	Application Member Functions
//
BOOL TutorApp::InitInstance()
{
	m_pMainWnd = new Main(NULL);
	m_pMainWnd->ShowWindow(m_nCmdShow);
	m_pMainWnd->UpdateWindow();
	//PROTECT_BEGIN AppInitInstance 
	//PROTECT_END 
	return TRUE;
}
//
// InitApplication - Create a window class for each window defined
// This makes it easier to modify icons, brushes, menus, etc.
//
BOOL TutorApp::InitApplication()
{
	if (!CWinApp::InitApplication())	 // Perform standard initialization
		return FALSE;


	WNDCLASS  wc;
	// Register custom window classes for each window
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpfnWndProc  = AfxWndProc;
	wc.hInstance = AfxGetInstanceHandle();
	wc.hIcon = LoadIcon("Main");
	wc.hCursor = LoadStandardCursor(IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(255,255,128)); 
	wc.lpszMenuName =  NULL; // Specify Menu in Create()
	wc.lpszClassName = "Main";

	::RegisterClass(&wc);

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpfnWndProc  = AfxWndProc;
	wc.hInstance = AfxGetInstanceHandle();
	wc.hIcon = LoadStandardIcon(IDI_APPLICATION);
	wc.hCursor = LoadStandardCursor(IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1); 
	wc.lpszMenuName =  NULL; // Specify Menu in Create()
	wc.lpszClassName = "NewWin";

	::RegisterClass(&wc);

	//PROTECT_BEGIN AppInitApplication 
	//PROTECT_END 
	return TRUE;
}
