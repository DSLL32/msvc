{*-----------------------------------------------------------------
 * tutor.pas - My First PowerCode App													 
 * 1/18/93 - 9:26:55am																	 
 * John Doe																				 
 *-----------------------------------------------------------------}
{PROTECT_BEGIN AppRevisions }
{PROTECT_END }

program Tutor;

{$R Tutor.res}

uses WinTypes, WinProcs, Objects, OMemory, OWindows, ODialogs, Strings, CommDlg, 
MMSystem, Jtools;

{PROTECT_BEGIN GlobalVars }
{PROTECT_END }

const
	cm_MainApp =    100;
	cm_MainDialog = 101;
	cm_MainWindow = 102;
	cm_MainCheckBox = 103;
	cm_MainSound =  104;
	cm_FilePrint =  105;
	cm_HelpAbout =  106;
	ID_ADDTYPE =    107;
	ID_MALE =       108;
	ID_FEMALE =     109;
	ID_LNAME =      110;
	ID_NAME =       111;
	ID_TITLE =      112;
	ID_CONAME =     113;
	ID_STREET1 =    114;
	ID_STREET2 =    115;
	ID_CITY =       116;
	ID_STATE =      117;
	ID_ZIP =        118;
	ID_BPHONE =     119;
	ID_HPHONE =     120;
	ID_FAX =        121;
	ID_COMMENTS =   122;
	ID_SEARCH =     123;
	ID_LIST =       124;


type
	PMain = ^TMain;
	TMain = object(TWindow)
	constructor Init(AParent: PWindowsObject);
	procedure SetupWindow; virtual;
	procedure GetWindowClass(var AWndClass: TWndClass);virtual;
	function GetClassName: PChar;virtual;
	procedure CMMainApp(var Msg: TMessage);
		virtual cm_First + cm_MainApp;
	procedure CMMainDialog(var Msg: TMessage);
		virtual cm_First + cm_MainDialog;
	procedure CMMainWindow(var Msg: TMessage);
		virtual cm_First + cm_MainWindow;
	procedure CMMainSound(var Msg: TMessage);
		virtual cm_First + cm_MainSound;
	procedure CMFileNew(var Msg: TMessage);
		virtual cm_First + cm_FileNew;
	procedure CMFileOpen(var Msg: TMessage);
		virtual cm_First + cm_FileOpen;
	procedure CMFileSave(var Msg: TMessage);
		virtual cm_First + cm_FileSave;
	procedure CMFilePrint(var Msg: TMessage);
		virtual cm_First + cm_FilePrint;
	procedure CMHelpAbout(var Msg: TMessage);
		virtual cm_First + cm_HelpAbout;
	{PROTECT_BEGIN wMainDefinition }
	{PROTECT_END }
end;
type
	PNewWin = ^TNewWin;
	TNewWin = object(TWindow)
	constructor Init(AParent: PWindowsObject);
	procedure SetupWindow; virtual;
	procedure GetWindowClass(var AWndClass: TWndClass);virtual;
	function GetClassName: PChar;virtual;
	{PROTECT_BEGIN wNewWinDefinition }
	{PROTECT_END }
end;
type
  PAbout = ^TAbout;
  TAbout = object(TDialog)
 	WorkStr : array[0..80] of Char;

 	constructor Init(AParent: PWindowsObject; AName : PChar);
	procedure SetupWindow; virtual;
	function CanClose: Boolean;virtual;
	{PROTECT_BEGIN dABOUTDefinition }
	{PROTECT_END }
  end;
type
  PAddress = ^TAddress;
  TAddress = object(TDialog)
 	WorkStr : array[0..80] of Char;
	EdLname, EdName, EdTitle, EdConame, EdStreet1, EdStreet2, EdCity, EdState, 
EdZip, EdBphone, EdHphone, EdFax, EdComments : PEdit;
	RbMale, RbFemale : PRadioButton;
	CoAddtype : PComboBox;

 	constructor Init(AParent: PWindowsObject; AName : PChar);
	procedure SetupWindow; virtual;
	function CanClose: Boolean;virtual;
	procedure MsgSearch(var Msg: TMessage); 
		virtual id_First + ID_SEARCH;
	{PROTECT_BEGIN dADDRESSDefinition }
	{PROTECT_END }
  end;
type
  PSearch = ^TSearch;
  TSearch = object(TDialog)
 	WorkStr : array[0..80] of Char;
	LbList : PListBox; 

 	constructor Init(AParent: PWindowsObject; AName : PChar);
	procedure SetupWindow; virtual;
	function CanClose: Boolean;virtual;
	{PROTECT_BEGIN dSEARCHDefinition }
	{PROTECT_END }
  end;

type
	TTutorApp = object(TApplication)
  	procedure InitMainWindow; virtual;
	procedure InitInstance; virtual;
	{PROTECT_BEGIN AppDefinition }
	{PROTECT_END }
	end;

{*-----------------------------------------------------------------
 * Window Method implementations - Main    
 *-----------------------------------------------------------------}
constructor TMain.Init(AParent: PWindowsObject);
var 	pTB : PToolBar;
begin
	TWindow.Init(AParent, 'My First Object/Designer App');
	Attr.Menu := LoadMenu(HInstance, 'Main');
	{ Create ToolBar - This is a child window so OWL will delete }
	pTB := New(PToolBar, Init(@Self, 44));	
	pTB^.AddToolItem(@Self, 'Button', CM_MAINAPP, -1, -1, 45, 45, 
		'NMainApp', 'SMainApp', 'Y', 'Y');
	pTB^.AddToolItem(@Self, 'Button', CM_MAINDIALOG, 43, -1, 45, 45, 
		'NMainDialog', 'SMainDialog', 'Y', 'Y');
	pTB^.AddToolItem(@Self, 'Button', CM_MAINWINDOW, 87, -1, 45, 45, 
		'NMainWindow', 'SMainWindow', 'Y', 'Y');
	pTB^.AddToolItem(@Self, 'Checkbox', CM_MAINCHECKBOX, 178, 2, 40, 40, 
		'NMainCheckBox', 'SMainCheckBox', 'N', 'N');
	pTB^.AddToolItem(@Self, 'Button', CM_MAINSOUND, 131, -1, 45, 45, 
		'NMainSound', 'SMainSound', 'Y', 'Y');
	{PROTECT_BEGIN wMainConstructor }
	{PROTECT_END }
end;

{*-----------------------------------------------------------------
 * procedure TMain.GetWindowClass
 * Allows window class info to be modified (icon, color, etc.)
 *-----------------------------------------------------------------}
procedure TMain.GetWindowClass(var AWndClass: TWndClass);
begin
	TWindow.GetWindowClass(AWndClass); { Get the default class }
	AWndClass.hbrBackground := CreateSolidBrush(RGB(255,255,128));
	AWndClass.hIcon := LoadIcon(HInstance, 'Main');
	{PROTECT_BEGIN wMainGetWindowClass }
	{PROTECT_END }
end;

{*-----------------------------------------------------------------
 * function TMain.GetClassName
 * Returns a unique class name for the window class.
 *-----------------------------------------------------------------}
function TMain.GetClassName: PChar;
begin
	GetClassName := 'Main';
end;

{*-----------------------------------------------------------------
 * procedure TMain.SetupWindow
 * Setup all child windows and controls.
 * Perform any window setup operations requiring a window handle.
 *-----------------------------------------------------------------}
procedure TMain.SetupWindow;
begin
	TWindow.SetupWindow;
	{PROTECT_BEGIN wMainSetupWindow }
	{PROTECT_END }
end;

{*-----------------------------------------------------------------
 * Window Message Response Methods  - Main    		
 *-----------------------------------------------------------------}
procedure TMain.CMMainApp(var Msg: TMessage);
begin
	{ PROTECT_BEGIN wMainMainApp  }
	{ PROTECT_END  }
	WinExec('notepad.exe tutor.txt', SW_SHOW);
end;
procedure TMain.CMMainDialog(var Msg: TMessage);
var pDlg : PDialog;
begin
	pDlg := New(PADDRESS, Init(@Self, 'ADDRESS'));
	Application^.MakeWindow(pDlg);
	pDlg^.Show(SW_SHOW);
	{ PROTECT_BEGIN wMainMainDialog  }
	{ PROTECT_END  }
end;
procedure TMain.CMMainWindow(var Msg: TMessage);
var pWin : PWindow;
begin
	pWin := New(PNewWin, Init(@Self));
	Application^.MakeWindow(pWin);
	pWin^.Show(SW_SHOW);
	{ PROTECT_BEGIN wMainMainWindow  }
	{ PROTECT_END  }
end;
procedure TMain.CMMainSound(var Msg: TMessage);
begin
	sndPlaySound('tada.wav', SND_SYNC);
	{ PROTECT_BEGIN wMainMainSound  }
	{ PROTECT_END  }
end;
procedure TMain.CMFileNew(var Msg: TMessage);
var pWin : PWindow;
begin
	pWin := New(PNewWin, Init(@Self));
	Application^.MakeWindow(pWin);
	pWin^.Show(SW_SHOW);
	{ PROTECT_BEGIN wMainFileNew  }
	{ PROTECT_END  }
end;
procedure TMain.CMFileOpen(var Msg: TMessage);
var
	OpenFN      	: TOpenFileName;
	szFilter    	: array [0..256] of Char;
	szFile		: array [0..256] of Char;
begin
	FillChar(szFilter, SizeOf(szFilter), #0);  
	StrCopy(szFilter, 'All Files (*.*)');
	StrCopy(@szFilter[StrLen(szFilter)+1], '*.*');
	FillChar(OpenFN, SizeOf(TOpenFileName), #0);
	StrCopy(szFile, '');
	with OpenFN do
	begin
		hInstance	:= HInstance;
		hwndOwner     	:= HWindow;
		lpstrFile     	:= szFile;
		nMaxFile      	:= SizeOf(szFile);
		lpstrFilter   	:= szFilter;
		flags         	:= ofn_FileMustExist;
		lStructSize   	:= sizeof(TOpenFileName);
		nFilterIndex  	:= 1;
	end;
	if GetOpenFileName(OpenFN) <> True then	exit;
	{ PROTECT_BEGIN wMainFileOpen  }
	{ PROTECT_END  }

end;
procedure TMain.CMFileSave(var Msg: TMessage);
var
	OpenFN      	: TOpenFileName;
	szFilter    	: array [0..256] of Char;
	szFile		: array [0..256] of Char;
begin
	FillChar(szFilter, SizeOf(szFilter), #0);  
	StrCopy(szFilter, 'All Files (*.*)');
	StrCopy(@szFilter[StrLen(szFilter)+1], '*.*');
	FillChar(OpenFN, SizeOf(TOpenFileName), #0);
	StrCopy(szFile, '');
	with OpenFN do
	begin
		hInstance	:= HInstance;
		hwndOwner     	:= HWindow;
		lpstrFile     	:= szFile;
		nMaxFile      	:= SizeOf(szFile);
		lpstrFilter   	:= szFilter;
		flags         	:= ofn_FileMustExist;
		lStructSize   	:= sizeof(TOpenFileName);
		nFilterIndex  	:= 1;
	end;
	if GetSaveFileName(OpenFN) <> True then	exit;
	{ PROTECT_BEGIN wMainFileSave  }
	{ PROTECT_END  }

end;
procedure TMain.CMFilePrint(var Msg: TMessage);
var	PrintD	: TPrintDlg;
begin
	FillChar(PrintD, SizeOf(TPrintDlg), #0);
	with PrintD do
	begin
		hInstance     	:= HInstance;
		hwndOwner     	:= HWindow;
		lStructSize	:= sizeof(TPrintDlg);
		Flags		:= pd_ReturnDC;
  end;
	if PrintDlg(PrintD) <> True then exit;
	{ PROTECT_BEGIN wMainFilePrint  }
	{ PROTECT_END  }

end;
procedure TMain.CMHelpAbout(var Msg: TMessage);
var pDlg : PDialog;
begin
	Application^.ExecDialog(New(PAbout, Init(@Self, 'ABOUT'))); 
	{ PROTECT_BEGIN wMainHelpAbout  }
	{ PROTECT_END  }
end;
{*-----------------------------------------------------------------
 * Window Method implementations - NewWin    
 *-----------------------------------------------------------------}
constructor TNewWin.Init(AParent: PWindowsObject);
begin
	TWindow.Init(AParent, '');
	Attr.Menu := LoadMenu(HInstance, '');
	Attr.Style := WS_OVERLAPPEDWINDOW or ws_Visible;
	{PROTECT_BEGIN wNewWinConstructor }
	{PROTECT_END }
end;

{*-----------------------------------------------------------------
 * procedure TNewWin.GetWindowClass
 * Allows window class info to be modified (icon, color, etc.)
 *-----------------------------------------------------------------}
procedure TNewWin.GetWindowClass(var AWndClass: TWndClass);
begin
	TWindow.GetWindowClass(AWndClass); { Get the default class }
	{PROTECT_BEGIN wNewWinGetWindowClass }
	{PROTECT_END }
end;

{*-----------------------------------------------------------------
 * function TNewWin.GetClassName
 * Returns a unique class name for the window class.
 *-----------------------------------------------------------------}
function TNewWin.GetClassName: PChar;
begin
	GetClassName := 'NewWin';
end;

{*-----------------------------------------------------------------
 * procedure TNewWin.SetupWindow
 * Setup all child windows and controls.
 * Perform any window setup operations requiring a window handle.
 *-----------------------------------------------------------------}
procedure TNewWin.SetupWindow;
begin
	TWindow.SetupWindow;
	{PROTECT_BEGIN wNewWinSetupWindow }
	{PROTECT_END }
end;

{*-----------------------------------------------------------------
 * Window Message Response Methods  - NewWin    		
 *-----------------------------------------------------------------}
{*-----------------------------------------------------------------
 * Dialog Method implementations - TAbout    
 *-----------------------------------------------------------------}
constructor TAbout.Init(AParent: PWindowsObject; AName :PChar);
begin
	TDialog.Init(AParent, AName);
	{PROTECT_BEGIN dABOUTConstructor }
	{PROTECT_END }
end;


{*-----------------------------------------------------------------
 * procedure TAbout.SetupWindow
 * Setup all child windows and controls.
 * Perform any window setup operations requiring a window handle.
 *-----------------------------------------------------------------}
procedure TAbout.SetupWindow;
begin
	TDialog.SetupWindow;
	{PROTECT_BEGIN dAboutSetupWindow }
	{PROTECT_END }
end;

{*-----------------------------------------------------------------
 * function TAbout.CanClose
 * Determine if dialog should be allowed to update
 *-----------------------------------------------------------------}
function TAbout.CanClose : Boolean;
var I : Word;
begin
	CanClose := true;
	{PROTECT_BEGIN dAboutCanClose }
	{PROTECT_END }
end;
{*-----------------------------------------------------------------
 * Dialog Message Response Methods  - TAbout
 *-----------------------------------------------------------------}
{*-----------------------------------------------------------------
 * Dialog Method implementations - TAddress    
 *-----------------------------------------------------------------}
constructor TAddress.Init(AParent: PWindowsObject; AName :PChar);
begin
	TDialog.Init(AParent, AName);
	New(EdLname, InitResource(@Self, ID_LNAME, 30));
	New(EdName, InitResource(@Self, ID_NAME, 0));
	New(EdTitle, InitResource(@Self, ID_TITLE, 0));
	New(EdConame, InitResource(@Self, ID_CONAME, 0));
	New(EdStreet1, InitResource(@Self, ID_STREET1, 0));
	New(EdStreet2, InitResource(@Self, ID_STREET2, 0));
	New(EdCity, InitResource(@Self, ID_CITY, 0));
	New(EdState, InitResource(@Self, ID_STATE, 0));
	New(EdZip, InitResource(@Self, ID_ZIP, 0));
	New(EdBphone, InitResource(@Self, ID_BPHONE, 0));
	New(EdHphone, InitResource(@Self, ID_HPHONE, 0));
	New(EdFax, InitResource(@Self, ID_FAX, 0));
	New(EdComments, InitResource(@Self, ID_COMMENTS, 0));
	New(CoAddtype, InitResource(@Self, ID_ADDTYPE, 0));
	New(RbMale, InitResource(@Self, ID_MALE));
	New(RbFemale, InitResource(@Self, ID_FEMALE));
	{PROTECT_BEGIN dADDRESSConstructor }
	{PROTECT_END }
end;


{*-----------------------------------------------------------------
 * procedure TAddress.SetupWindow
 * Setup all child windows and controls.
 * Perform any window setup operations requiring a window handle.
 *-----------------------------------------------------------------}
procedure TAddress.SetupWindow;
begin
	TDialog.SetupWindow;
	RbFemale^.Check;	 { Set Initial Check }
	{ Fill Combo with initial values }
	CoAddtype^.AddString('Employee');
	CoAddtype^.AddString('Vendor');
	CoAddtype^.AddString('Friend');
	CoAddtype^.AddString('Other');
	{PROTECT_BEGIN dAddressSetupWindow }
	{PROTECT_END }
end;

{*-----------------------------------------------------------------
 * function TAddress.CanClose
 * Determine if dialog should be allowed to update
 *-----------------------------------------------------------------}
function TAddress.CanClose : Boolean;
var I : Word;
begin
	if EdLname^.GetText(WorkStr, 80) = 0 then
	begin
		MessageBox(HWindow, 'Mandatory field requires input.', 'Error', 
	  	 MB_OK or MB_ICONEXCLAMATION);
		SetFocus(EdLname^.HWindow);	 { Position cursor to error }
		CanClose := false;
		exit;
	end;
	CanClose := true;
	{PROTECT_BEGIN dAddressCanClose }
	{PROTECT_END }
end;
{*-----------------------------------------------------------------
 * Dialog Message Response Methods  - TAddress
 *-----------------------------------------------------------------}
procedure TAddress.MsgSearch(var Msg: TMessage);
var pDlg : PDialog;
begin
	Application^.ExecDialog(New(PSearch, Init(@Self, 'SEARCH'))); 
	{ PROTECT_BEGIN dAddressSearch  }
	{ PROTECT_END  }
end;
{*-----------------------------------------------------------------
 * Dialog Method implementations - TSearch    
 *-----------------------------------------------------------------}
constructor TSearch.Init(AParent: PWindowsObject; AName :PChar);
begin
	TDialog.Init(AParent, AName);
	New(LbList, InitResource(@Self, ID_LIST));
	{PROTECT_BEGIN dSEARCHConstructor }
	{PROTECT_END }
end;


{*-----------------------------------------------------------------
 * procedure TSearch.SetupWindow
 * Setup all child windows and controls.
 * Perform any window setup operations requiring a window handle.
 *-----------------------------------------------------------------}
procedure TSearch.SetupWindow;
begin
	TDialog.SetupWindow;
	{PROTECT_BEGIN dSearchSetupWindow }
	{PROTECT_END }
end;

{*-----------------------------------------------------------------
 * function TSearch.CanClose
 * Determine if dialog should be allowed to update
 *-----------------------------------------------------------------}
function TSearch.CanClose : Boolean;
var I : Word;
begin
	CanClose := true;
	{PROTECT_BEGIN dSearchCanClose }
	{PROTECT_END }
end;
{*-----------------------------------------------------------------
 * Dialog Message Response Methods  - TSearch
 *-----------------------------------------------------------------}

{PROTECT_BEGIN AppUserCode  }
{PROTECT_END  }

{*-----------------------------------------------------------------
 * TTutorApp method implementations:        
 *-----------------------------------------------------------------}

procedure TTutorApp.InitMainWindow;
begin
	MainWindow := New(PMain, Init(nil));
	{PROTECT_BEGIN AppInitMain }
	{PROTECT_END }
end;
{*-----------------------------------------------------------------
 *  InitInstance - Needed to load accelerator table      
 *-----------------------------------------------------------------}
procedure TTutorApp.InitInstance;
begin
	TApplication.InitInstance;
	HAccTable := LoadAccelerators(HInstance, 'Tutor');
end;
{*-----------------------------------------------------------------
 * Main Program:
 *-----------------------------------------------------------------}
var
	TutorApp: TTutorApp;

begin
	TutorApp.Init('Tutor');
	TutorApp.Run;
	TutorApp.Done;
end.
