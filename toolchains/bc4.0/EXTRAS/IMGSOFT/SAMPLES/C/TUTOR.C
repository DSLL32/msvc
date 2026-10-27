//***************************************************************************
// tutor.c - My First PowerCode App
// 9/04/92 - 2:08:40pm
// John Doe
//***************************************************************************
//PROTECT_BEGIN AppRevisions 
//PROTECT_END 
#define _EXTERN 		 // Ignore External definition in main module
#include "Tutor.h"		// Function Prototypes & other includes

//PROTECT_BEGIN GlobalVars 
//PROTECT_END 


//****************************************************************************
//
// FUNCTION: MainWndProc(HWND, UINT, WPARAM, LPARAM)
//
// PURPOSE:  Processes messages
//
//****************************************************************************

LRESULT CALLBACK _export MainWndProc(HWND hWnd, UINT message, 
	WPARAM wParam, LPARAM lParam)
{
	HWND hToolWnd;			 // Handle to Tool Bar Window
	//PROTECT_BEGIN wMainVars 
	//PROTECT_END 

	switch (message) 
	{
		case WM_CREATE:
			// Create Tool bar - This is a child so don't worry about showing
			hToolWnd = CreateToolbar(hWnd, 44);
			AddToolItem(hToolWnd, "Button", CM_MAINAPP, -1, -1, 45, 45, 
		 		"NMainApp", "SMainApp", "Y", "Y");
			AddToolItem(hToolWnd, "Button", CM_MAINDIALOG, 43, -1, 45, 45, 
		 		"NMainDialog", "SMainDialog", "Y", "Y");
			AddToolItem(hToolWnd, "Button", CM_MAINWINDOW, 87, -1, 45, 45, 
		 		"NMainWindow", "SMainWindow", "Y", "Y");
			AddToolItem(hToolWnd, "Checkbox", CM_MAINCHECKBOX, 178, 2, 40, 40, 
		 		"NMainCheckBox", "SMainCheckBox", "N", "N");
			AddToolItem(hToolWnd, "Button", CM_MAINSOUND, 131, -1, 45, 45, 
		 		"NMainSound", "SMainSound", "Y", "Y");
			//PROTECT_BEGIN wMainWM_CREATE 
			//PROTECT_END 
			break;
		case WM_COMMAND:
			switch (wParam) 
			{
				case CM_NEW:
				{
					HWND hNewWin = CreateWindow( 
						"NewWinWClass",
						"",
						WS_OVERLAPPEDWINDOW,
  						CW_USEDEFAULT,
						CW_USEDEFAULT,
  						CW_USEDEFAULT,
						CW_USEDEFAULT,
  						hWnd,	NULL,	hInst, NULL);
					ShowWindow(hNewWin, SW_SHOW);
					UpdateWindow(hNewWin);
					//PROTECT_BEGIN wMainNew 
					//PROTECT_END 
				}
				break;  
				case CM_OPEN:
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
					of.hwndOwner=hWnd;
					if (GetOpenFileName(&of))
					{
						//PROTECT_BEGIN wMainOpen 
						//PROTECT_END 
					}
				}
				break;  
				case CM_SAVE:
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
					of.hwndOwner=hWnd;
					if (GetSaveFileName(&of))
					{
						//PROTECT_BEGIN wMainSave 
						//PROTECT_END 
					}
				}
				break;  
				case CM_PRINT:
				{
					PRINTDLG pd;
					memset(&pd, 0, sizeof( PRINTDLG ));
					pd.lStructSize = sizeof( PRINTDLG );
					pd.hwndOwner=hWnd;
					pd.Flags=PD_RETURNDC;
					if (PrintDlg(&pd))
					{
						//PROTECT_BEGIN wMainPrint 
						//PROTECT_END 
					}
				}
				break;  
				case CM_EXIT:
				{
					//PROTECT_BEGIN wMainExit 
					//PROTECT_END 
				}
				break;  
				case CM_ABOUT:
				{
					DLGPROC lpDlgProc = (DLGPROC)MakeProcInstance((FARPROC)ABOUTDlgProc, hInst);
					DialogBox(hInst, "ABOUT", hWnd, lpDlgProc);
					FreeProcInstance((FARPROC)lpDlgProc);
					//PROTECT_BEGIN wMainAbout 
					//PROTECT_END 
				}
				break;  
				case CM_MAINAPP:
				{
					//PROTECT_BEGIN wMainMainApp 
					//PROTECT_END 
					WinExec("notepad.exe tutor.txt", SW_SHOW);
				}
				break;  
				case CM_MAINDIALOG:
				{
					HWND hAddress = CreateDialog(hInst, "ADDRESS", hWnd, ADDRESSDlgProc);
					ShowWindow(hAddress, SW_SHOW);
					//PROTECT_BEGIN wMainMainDialog 
					//PROTECT_END 
				}
				break;  
				case CM_MAINWINDOW:
				{
					HWND hNewWin = CreateWindow( 
						"NewWinWClass",
						"",
						WS_OVERLAPPEDWINDOW,
  						CW_USEDEFAULT,
						CW_USEDEFAULT,
  						CW_USEDEFAULT,
						CW_USEDEFAULT,
  						hWnd,	NULL,	hInst, NULL);
					ShowWindow(hNewWin, SW_SHOW);
					UpdateWindow(hNewWin);
					//PROTECT_BEGIN wMainMainWindow 
					//PROTECT_END 
				}
				break;  
				case CM_MAINCHECKBOX:
				{
					//PROTECT_BEGIN wMainMainCheckBox 
					//PROTECT_END 
				}
				break;  
				case CM_MAINSOUND:
				{
					sndPlaySound("tada.wav", SND_SYNC);
					//PROTECT_BEGIN wMainMainSound 
					//PROTECT_END 
				}
				break;  
			} 
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			//PROTECT_BEGIN wMainWM_DESTROY 
			//PROTECT_END 
			break;
		default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return (NULL);
}

//****************************************************************************
//
// FUNCTION: NewWinWndProc(HWND, UINT, WPARAM, LPARAM)
//
// PURPOSE:  Processes messages
//
//****************************************************************************

LRESULT CALLBACK _export NewWinWndProc(HWND hWnd, UINT message, 
	WPARAM wParam, LPARAM lParam)
{
	//PROTECT_BEGIN wNewWinVars 
	//PROTECT_END 

	switch (message) 
	{
		case WM_CREATE:
			//PROTECT_BEGIN wNewWinWM_CREATE 
			//PROTECT_END 
			break;
		case WM_COMMAND:
			switch (wParam) 
			{
			} 
			break;
		case WM_DESTROY:
			//PROTECT_BEGIN wNewWinWM_DESTROY 
			//PROTECT_END 
			break;
		default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return (NULL);
}
//****************************************************************************
//
// FUNCTION: ABOUTDlgProc(HWND, unsigned, WORD, LONG)
//
// PURPOSE:  Processes Dialog Messages
//
//****************************************************************************
BOOL CALLBACK _export ABOUTDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//PROTECT_BEGIN dABOUTVars 
	//PROTECT_END 
	switch (message) 
	{
		case WM_INITDIALOG:
			//PROTECT_BEGIN dABOUTWM_INITDIALOG 
			//PROTECT_END 
			break;
		case WM_CLOSE:
			//PROTECT_BEGIN dABOUTWM_CLOSE 
			//PROTECT_END 
			PostMessage(hWnd, WM_COMMAND, IDCANCEL, 0L);
			break; 
		case WM_COMMAND:
			switch (wParam) 
			{
				case IDOK:				// End with update
					//PROTECT_BEGIN dABOUTIDOK 
					//PROTECT_END 
	     				EndDialog(hWnd, TRUE);
					break;
				case IDCANCEL:		// End with no update
					//PROTECT_BEGIN dABOUTIDCANCEL 
					//PROTECT_END 
					EndDialog(hWnd, FALSE);
					break;
			} 
			break;
		default:
			return FALSE;
	}
	return TRUE;
}
//****************************************************************************
//
// FUNCTION: ADDRESSDlgProc(HWND, unsigned, WORD, LONG)
//
// PURPOSE:  Processes Dialog Messages
//
//****************************************************************************
BOOL CALLBACK _export ADDRESSDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//PROTECT_BEGIN dADDRESSVars 
	//PROTECT_END 
	switch (message) 
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hWnd, ID_LNAME, EM_LIMITTEXT, 30, 0L);
			SendMessage(GetDlgItem(hWnd, ID_FEMALE), BM_SETCHECK, TRUE, 0);
			// Fill Combo with initial values
			SendMessage(GetDlgItem(hWnd, ID_ADDTYPE), 
				CB_ADDSTRING, 0, (LONG)(LPSTR)"Employee");
			SendMessage(GetDlgItem(hWnd, ID_ADDTYPE), 
				CB_ADDSTRING, 0, (LONG)(LPSTR)"Vendor");
			SendMessage(GetDlgItem(hWnd, ID_ADDTYPE), 
				CB_ADDSTRING, 0, (LONG)(LPSTR)"Friend");
			SendMessage(GetDlgItem(hWnd, ID_ADDTYPE), 
				CB_ADDSTRING, 0, (LONG)(LPSTR)"Other");
			//PROTECT_BEGIN dADDRESSWM_INITDIALOG 
			//PROTECT_END 
			break;
		case WM_CLOSE:
			//PROTECT_BEGIN dADDRESSWM_CLOSE 
			//PROTECT_END 
			PostMessage(hWnd, WM_COMMAND, IDCANCEL, 0L);
			break; 
		case WM_COMMAND:
			switch (wParam) 
			{
				case ID_LNAME:			// Edit  
					//PROTECT_BEGIN dADDRESSLname 
					//PROTECT_END 
				case ID_NAME:			// Edit  
					//PROTECT_BEGIN dADDRESSName 
					//PROTECT_END 
				case ID_TITLE:			// Edit  
					//PROTECT_BEGIN dADDRESSTitle 
					//PROTECT_END 
				case ID_CONAME:			// Edit  
					//PROTECT_BEGIN dADDRESSConame 
					//PROTECT_END 
				case ID_STREET1:			// Edit  
					//PROTECT_BEGIN dADDRESSStreet1 
					//PROTECT_END 
				case ID_STREET2:			// Edit  
					//PROTECT_BEGIN dADDRESSStreet2 
					//PROTECT_END 
				case ID_CITY:			// Edit  
					//PROTECT_BEGIN dADDRESSCity 
					//PROTECT_END 
				case ID_STATE:			// Edit  
					//PROTECT_BEGIN dADDRESSState 
					//PROTECT_END 
				case ID_ZIP:			// Edit  
					//PROTECT_BEGIN dADDRESSZip 
					//PROTECT_END 
				case ID_BPHONE:			// Edit  
					//PROTECT_BEGIN dADDRESSBphone 
					//PROTECT_END 
				case ID_HPHONE:			// Edit  
					//PROTECT_BEGIN dADDRESSHphone 
					//PROTECT_END 
				case ID_FAX:			// Edit  
					//PROTECT_BEGIN dADDRESSFax 
					//PROTECT_END 
				case ID_COMMENTS:			// Edit  
					//PROTECT_BEGIN dADDRESSComments 
					//PROTECT_END 
					switch(HIWORD(lParam))			// Edit Notifications
					{ 
						case EN_SETFOCUS:  
							break;
						case EN_KILLFOCUS: 
							break;
						case EN_CHANGE: 
							break;
					}
					break;
				case ID_ADDTYPE:			// ComboBox  
					//PROTECT_BEGIN dADDRESSAddtype 
					//PROTECT_END 
					switch(HIWORD(lParam))			// ComboBox Notifications
					{ 
						case CBN_SELCHANGE:  
							break;
						case CBN_KILLFOCUS: 
							break;
						case CBN_DBLCLK: 
							break;
					}
					break;
				case ID_SEARCH:
				{
					DLGPROC lpDlgProc = (DLGPROC)MakeProcInstance((FARPROC)SEARCHDlgProc, hInst);
					DialogBox(hInst, "SEARCH", hWnd, lpDlgProc);
					FreeProcInstance((FARPROC)lpDlgProc);
					//PROTECT_BEGIN dADDRESSSearch 
					//PROTECT_END 
				}
				break;  
				case IDOK:				// End with update
					if ( CanCloseADDRESS(hWnd) )
					{
						//PROTECT_BEGIN dADDRESSIDOK 
						//PROTECT_END 
		   				DestroyWindow(hWnd);
					}
					break;
				case IDCANCEL:		// End with no update
					//PROTECT_BEGIN dADDRESSIDCANCEL 
					//PROTECT_END 
			  		DestroyWindow(hWnd);
					break;
			} 
			break;
		default:
			return FALSE;
	}
	return TRUE;
}
//
// CanCloseADDRESS - Determine if dialog can be exited with update
//
BOOL CanCloseADDRESS(HWND hWnd)
{
	char szWorkStr[81], *p;
	if ( !GetDlgItemText(hWnd, ID_LNAME, szWorkStr, 80) )
	{
		MessageBox(hWnd, "Mandatory field requires input.", "Error", 
			MB_OK | MB_ICONEXCLAMATION);
		SetFocus(GetDlgItem(hWnd, ID_LNAME));	 // Position cursor to error
		return FALSE;
	}
	//PROTECT_BEGIN dADDRESSCanClose 
	//PROTECT_END 
	return TRUE;
}
//****************************************************************************
//
// FUNCTION: SEARCHDlgProc(HWND, unsigned, WORD, LONG)
//
// PURPOSE:  Processes Dialog Messages
//
//****************************************************************************
BOOL CALLBACK _export SEARCHDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//PROTECT_BEGIN dSEARCHVars 
	//PROTECT_END 
	switch (message) 
	{
		case WM_INITDIALOG:
			//PROTECT_BEGIN dSEARCHWM_INITDIALOG 
			//PROTECT_END 
			break;
		case WM_CLOSE:
			//PROTECT_BEGIN dSEARCHWM_CLOSE 
			//PROTECT_END 
			PostMessage(hWnd, WM_COMMAND, IDCANCEL, 0L);
			break; 
		case WM_COMMAND:
			switch (wParam) 
			{
				case ID_LIST:			// ListBox  
					//PROTECT_BEGIN dSEARCHList 
					//PROTECT_END 
					switch(HIWORD(lParam))			// ListBox Notifications
					{ 
						case LBN_SELCHANGE:  
							break;
						case LBN_KILLFOCUS: 
							break;
						case LBN_DBLCLK: 
							break;
					}
					break;
				case IDOK:				// End with update
					//PROTECT_BEGIN dSEARCHIDOK 
					//PROTECT_END 
	     				EndDialog(hWnd, TRUE);
					break;
				case IDCANCEL:		// End with no update
					//PROTECT_BEGIN dSEARCHIDCANCEL 
					//PROTECT_END 
					EndDialog(hWnd, FALSE);
					break;
			} 
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

//PROTECT_BEGIN AppUserCode 
//PROTECT_END 

//****************************************************************************
//
// FUNCTION: InitApplication(HINSTANCE)
//
// PURPOSE: Initializes window data and registers window & dialog classes
//
//****************************************************************************

BOOL InitApplication(HINSTANCE hInstance)
{
	WNDCLASS  wc;
	//
	// Register class for each window. Also register every dialog that has a 
	// class specified within the resource definition
	//
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, "Main");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(255,255,128)); 
	wc.lpszMenuName =  "Main";
	wc.lpszClassName = "MainWClass";

	RegisterClass(&wc);

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = NewWinWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1); 
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = "NewWinWClass";

	RegisterClass(&wc);


	// Tool Bar Class
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = ToolbarProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH); 
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = "ToolbarClass";

	RegisterClass(&wc);

	//PROTECT_BEGIN InitApplication 
	//PROTECT_END 

	return TRUE;
}


//****************************************************************************
//
// FUNCTION:  InitInstance(HINSTANCE, INT)
//
// PURPOSE:  Saves instance handle and creates main window
//
//****************************************************************************

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{

	hInst = hInstance;

	hWndMain = CreateWindow(
		"MainWClass",
		"My First Object/Designer App",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL );

	if (!hWndMain)
		return (FALSE);

	ShowWindow(hWndMain, nCmdShow);
	UpdateWindow(hWndMain);
	//PROTECT_BEGIN InitInstance 
	//PROTECT_END 
	return (TRUE);
}

//****************************************************************************
//
// FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
//
// PURPOSE: Calls initialization function, processes message loop
//
//****************************************************************************

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
	LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
 	HACCEL hAccel;


	if (!hPrevInstance)
		if (!InitApplication(hInstance))
	   		return (FALSE);

	if (!InitInstance(hInstance, nCmdShow))
  		return (FALSE);

	//PROTECT_BEGIN WinMain 
	//PROTECT_END 

 	hAccel = LoadAccelerators(hInstance, "Tutor");

	while (GetMessage(&msg, NULL, NULL, NULL)) 
	{
		if(TranslateAccelerator(hWndMain, hAccel, &msg))
			continue;
		TranslateMessage(&msg);
	 	DispatchMessage(&msg); 
	}
  
	return (msg.wParam);
}
