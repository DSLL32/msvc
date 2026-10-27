//***************************************************************************
// tutor.cpp - My First PowerCode App
// 9/04/92 - 2:13:25pm
// John Doe
//***************************************************************************
//PROTECT_BEGIN AppRevisions 
//PROTECT_END 
#include "Tutor.hpp"				// Main App Class Definitions
//PROTECT_BEGIN GlobalVars 
//PROTECT_END 
//
// Member Functions - Main 
//
// Window Constructor
Main::Main(PTWindowsObject AParent) 
: TWindow(AParent, "My First Object/Designer App")
{

	// Create ToolBar - This is a child window so OWL will delete
	PTToolBar pTB = new TToolBar(this, 44);	
	pTB->ToolItems->add(* new TToolItem(this, "Button", CM_MAINAPP, -1, -1, 45, 45, 
		 "NMainApp", "SMainApp", "Y", "Y"));
	pTB->ToolItems->add(* new TToolItem(this, "Button", CM_MAINDIALOG, 43, -1, 45, 45, 
		 "NMainDialog", "SMainDialog", "Y", "Y"));
	pTB->ToolItems->add(* new TToolItem(this, "Button", CM_MAINWINDOW, 87, -1, 45, 45, 
		 "NMainWindow", "SMainWindow", "Y", "Y"));
	pTB->ToolItems->add(* new TToolItem(this, "Checkbox", CM_MAINCHECKBOX, 178, 2, 40, 40, 
		 "NMainCheckBox", "SMainCheckBox", "N", "N"));
	pTB->ToolItems->add(* new TToolItem(this, "Button", CM_MAINSOUND, 131, -1, 45, 45, 
		 "NMainSound", "SMainSound", "Y", "Y"));
	// PROTECT_BEGIN wMainConstructor 
	// PROTECT_END 
}
//
// GetWindowClass - Allows a new window class to be defined 
//
void Main::GetWindowClass(WNDCLASS& AWndClass)
{
	TWindow::GetWindowClass(AWndClass);
	AWndClass.hbrBackground = CreateSolidBrush(RGB(255,255,128));
	AWndClass.hIcon = LoadIcon(GetApplication()->hInstance, "Main");
	//PROTECT_BEGIN wMainGetWindowClass 
	//PROTECT_END 
}
//
//	SetupWindow - Initialization stuff requiring a window handle
//
void Main::SetupWindow()
{
	AssignMenu("Main");

	TWindowsObject::SetupWindow();	// will setup child windows
	//PROTECT_BEGIN wMainSetupWindow 
	//PROTECT_END 
}
//
// Window Response functions
//
void Main::MsgMainApp(RTMessage )
{
	//PROTECT_BEGIN wMainMainApp 
	//PROTECT_END 
	WinExec("notepad.exe tutor.txt", SW_SHOW);
}
void Main::MsgMainDialog(RTMessage )
{
	PAddress pDlg = new Address(this, "ADDRESS");
	GetApplication()->MakeWindow(pDlg);
	pDlg->Show(SW_SHOW);
	//PROTECT_BEGIN wMainMainDialog 
	//PROTECT_END 
}
void Main::MsgMainWindow(RTMessage )
{
	PTWindow pWin = new NewWin(this);
	GetApplication()->MakeWindow(pWin);
	pWin->Show(SW_SHOW);
	//PROTECT_BEGIN wMainMainWindow 
	//PROTECT_END 
}
void Main::MsgMainSound(RTMessage )
{
	sndPlaySound("tada.wav", SND_SYNC);
	//PROTECT_BEGIN wMainMainSound 
	//PROTECT_END 
}
void Main::CMNew(RTMessage )
{
	PTWindow pWin = new NewWin(this);
	GetApplication()->MakeWindow(pWin);
	pWin->Show(SW_SHOW);
	//PROTECT_BEGIN wMainNew 
	//PROTECT_END 
}
void Main::CMOpen(RTMessage )
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
	of.hwndOwner=HWindow;
	if (GetOpenFileName(&of))
	{
		//PROTECT_BEGIN wMainOpen 
		//PROTECT_END 
	}
}
void Main::CMSave(RTMessage )
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
	of.hwndOwner=HWindow;
	if (GetSaveFileName(&of))
	{
		//PROTECT_BEGIN wMainSave 
		//PROTECT_END 
	}
}
void Main::CMPrint(RTMessage )
{
	PRINTDLG pd;
	memset(&pd, 0, sizeof( PRINTDLG ));
	pd.lStructSize = sizeof( PRINTDLG );
	pd.hwndOwner=HWindow;
	pd.Flags=PD_RETURNDC;
	if (PrintDlg(&pd))
	{
		//PROTECT_BEGIN wMainPrint 
		//PROTECT_END 
	}
}
void Main::CMAbout(RTMessage )
{
	GetApplication()->ExecDialog(new About(this, "ABOUT") );
	//PROTECT_BEGIN wMainAbout 
	//PROTECT_END 
}
//
// CloseWindow - Close Window and Update Data
//
void Main::CloseWindow()
{
	if ( CanClose() )	// Update Window if OK
	{
		//PROTECT_BEGIN wMainCloseWindow 
		//PROTECT_END 
		TWindow::CloseWindow();	// Finish the CloseWindow
	}
}


//
// Member Functions - NewWin 
//
// Window Constructor
NewWin::NewWin(PTWindowsObject AParent) 
: TWindow(AParent, "")
{
	Attr.Style = WS_OVERLAPPEDWINDOW|WS_VISIBLE;

	// PROTECT_BEGIN wNewWinConstructor 
	// PROTECT_END 
}
//
// GetWindowClass - Allows a new window class to be defined 
//
void NewWin::GetWindowClass(WNDCLASS& AWndClass)
{
	TWindow::GetWindowClass(AWndClass);
	//PROTECT_BEGIN wNewWinGetWindowClass 
	//PROTECT_END 
}
//
//	SetupWindow - Initialization stuff requiring a window handle
//
void NewWin::SetupWindow()
{

	TWindowsObject::SetupWindow();	// will setup child windows
	//PROTECT_BEGIN wNewWinSetupWindow 
	//PROTECT_END 
}
//
// Window Response functions
//
//
// CloseWindow - Close Window and Update Data
//
void NewWin::CloseWindow()
{
	if ( CanClose() )	// Update Window if OK
	{
		//PROTECT_BEGIN wNewWinCloseWindow 
		//PROTECT_END 
		TWindow::CloseWindow();	// Finish the CloseWindow
	}
}


//
// Dialog Member Functions - About 
//
About::About(PTWindowsObject AParent, Pchar DlgName) : TDialog(AParent, DlgName)
{
	//PROTECT_BEGIN dABOUTConstructor 
	//PROTECT_END 
}
//
//	SetupWindow - Initialize controls, anything requiring win handle
//
void About::SetupWindow()
{
	TWindowsObject::SetupWindow();	// will setup child windows
	//PROTECT_BEGIN dAboutSetupWindow 
	//PROTECT_END 
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
// CloseWindow - Close Window and Update Data
//
void About::CloseWindow(int ARetValue)
{
	if ( ARetValue == IDOK && CanClose() ) // Update Dialog
	{
		
		//PROTECT_BEGIN dAboutCloseWindow 
		//PROTECT_END 
		TDialog::CloseWindow(ARetValue);	// Finish the CloseWindow
	}
}
//
// Dialog Member Functions - Address 
//
Address::Address(PTWindowsObject AParent, Pchar DlgName) : TDialog(AParent, DlgName)
{
	Lname = new TEdit(this, ID_LNAME, 30);
	Name = new TEdit(this, ID_NAME, 0);
	Title = new TEdit(this, ID_TITLE, 0);
	Coname = new TEdit(this, ID_CONAME, 0);
	Street1 = new TEdit(this, ID_STREET1, 0);
	Street2 = new TEdit(this, ID_STREET2, 0);
	City = new TEdit(this, ID_CITY, 0);
	State = new TEdit(this, ID_STATE, 0);
	Zip = new TEdit(this, ID_ZIP, 0);
	Bphone = new TEdit(this, ID_BPHONE, 0);
	Hphone = new TEdit(this, ID_HPHONE, 0);
	Fax = new TEdit(this, ID_FAX, 0);
	Comments = new TEdit(this, ID_COMMENTS, 0);
	Addtype = new TComboBox(this, ID_ADDTYPE, 0);
	Male = new TRadioButton(this, ID_MALE, NULL);
	Female = new TRadioButton(this, ID_FEMALE, NULL);
	//PROTECT_BEGIN dADDRESSConstructor 
	//PROTECT_END 
}
//
//	SetupWindow - Initialize controls, anything requiring win handle
//
void Address::SetupWindow()
{
	TWindowsObject::SetupWindow();	// will setup child windows
	Female->SetCheck(TRUE);	 // Set Initial Check
	// Fill Combo with initial values
	Addtype->AddString("Employee");
	Addtype->AddString("Vendor");
	Addtype->AddString("Friend");
	Addtype->AddString("Other");
	//PROTECT_BEGIN dAddressSetupWindow 
	//PROTECT_END 
}
//
// Dialog Response functions - Address
//
void Address::CMSearch(RTMessage )
{
	GetApplication()->ExecDialog(new Search(this, "SEARCH") );
	//PROTECT_BEGIN dAddressSearch 
	//PROTECT_END 
}
//
// CanClose - Determine if dialog should be allowed to update
//
BOOL Address::CanClose()
{
	if ( Lname->GetTextLen() == 0 )
	{
		MessageBox(HWindow, "Mandatory field requires input.", "Error", 
	  	 MB_OK | MB_ICONEXCLAMATION);
		SetFocus(Lname->HWindow);	 // Position cursor to error
		return FALSE;
	}
	//PROTECT_BEGIN dAddressCanClose 
	//PROTECT_END 
	return TRUE;
}
//
// CloseWindow - Close Window and Update Data
//
void Address::CloseWindow(int ARetValue)
{
	if ( ARetValue == IDOK && CanClose() ) // Update Dialog
	{
		
		//PROTECT_BEGIN dAddressCloseWindow 
		//PROTECT_END 
		TDialog::CloseWindow(ARetValue);	// Finish the CloseWindow
	}
}
//
// Dialog Member Functions - Search 
//
Search::Search(PTWindowsObject AParent, Pchar DlgName) : TDialog(AParent, DlgName)
{
	List = new TListBox(this, ID_LIST, NULL);
	//PROTECT_BEGIN dSEARCHConstructor 
	//PROTECT_END 
}
//
//	SetupWindow - Initialize controls, anything requiring win handle
//
void Search::SetupWindow()
{
	TWindowsObject::SetupWindow();	// will setup child windows
	//PROTECT_BEGIN dSearchSetupWindow 
	//PROTECT_END 
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
// CloseWindow - Close Window and Update Data
//
void Search::CloseWindow(int ARetValue)
{
	if ( ARetValue == IDOK && CanClose() ) // Update Dialog
	{
		
		//PROTECT_BEGIN dSearchCloseWindow 
		//PROTECT_END 
		TDialog::CloseWindow(ARetValue);	// Finish the CloseWindow
	}
}

//PROTECT_BEGIN AppUserCode 
//PROTECT_END 

//
//	Application Member Functions
//
void TutorApp::InitMainWindow()
{
		MainWindow = new Main(NULL);
	//PROTECT_BEGIN AppInitMain 
	//PROTECT_END 
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  LPSTR lpCmdLine, int nCmdShow)
{
	TutorApp TutorApp("My First PowerCode App", hInstance, hPrevInstance,
		lpCmdLine, nCmdShow);
	TutorApp.Run();
	return TutorApp.Status;
}
