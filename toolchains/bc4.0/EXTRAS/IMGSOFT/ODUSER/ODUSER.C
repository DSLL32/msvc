#define  STRICT
#include <windows.h>
#include <string.h>

HINSTANCE	hInst;		// Instance handle of this DLL
HWND 		hAttrWnd;       // Handle used to access Object/Designer atributes

//
// Global structure used to pass data to Object/Designer
//
struct AStruct
{
	LPSTR Attr;
	LPSTR Value;
}AttrStruct;

#define WM_USERGETATTRVALUE		WM_USER + 1
#define WM_USERSETATTRVALUE		WM_USER + 2

//
// Function Prototypes
//
BOOL CALLBACK _export UserAttributeEditHook(HWND hWnd, LPSTR Object, LPSTR Attr);
void UserSetAttrValue (LPSTR Attr, LPSTR Value);
LPSTR UserGetAttrValue (LPSTR Attr, LPSTR Value);

// Sample Dialog to Update Application Attributes
BOOL CALLBACK _export AppAttrsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

//
// User-Defined Source Driver Function Prototypes. All functions must be prototyped
// as LPSTR CALLBACK _export Function(HWND, LPSTR)
//
LPSTR CALLBACK _export StrUp( HWND, LPSTR );

//
// A Simple Example of User-Defined Source Driver Function - Duplicates
// functionality of $UP()$ function.
//
#pragma argsused
LPSTR CALLBACK _export StrUp(HWND hWnd, LPSTR str)
{
	strupr(str);
        return str;
}

//
// User defined hook function that is called whenever the user edits an attribute
// within Object/Designer. This function can be used to modify any of the attributes
// for the current object.
//
// Note: UserGetAttrValue() & UserSetAttrValue() require the HWND passed into this
// function. You should not pass the HWND from a custom dialog or window.
// 
BOOL CALLBACK _export UserAttributeEditHook(HWND hWnd, LPSTR szObject, LPSTR szAttr)
{
  // Must store handle to access attributes
	hAttrWnd = hWnd;

	// Only modify attributes if user edits DLLs attribute
	if ( !strcmp(szObject, "Application") && !strcmp(szAttr, "Custom") )
	{
		DLGPROC lpDlgProc = (DLGPROC)MakeProcInstance((FARPROC)AppAttrsDlgProc, hInst);
		DialogBox(hInst, "APPATTRS", hWnd, lpDlgProc);
		FreeProcInstance((FARPROC)lpDlgProc);
		return TRUE;  // Object/Designer doesn't need to perform editing
	}

        return FALSE;   // No Action taken, Use default editing 
}


//
// Functions to Get and Set attribute values for the current object
//
// Note: These functions cannot be called from source driver functions.
//
LPSTR UserGetAttrValue (LPSTR Attr, LPSTR Value)
{
	AttrStruct.Attr = Attr; 
	AttrStruct.Value = Value;
	SendMessage(hAttrWnd, WM_USERGETATTRVALUE, 0, (LONG)&AttrStruct);
	return Value;
}
void UserSetAttrValue (LPSTR Attr, LPSTR Value)
{
	AttrStruct.Attr = Attr; 
	AttrStruct.Value = Value;
	SendMessage(hAttrWnd, WM_USERSETATTRVALUE, 0, (LONG)&AttrStruct);
}


//
// Standard DLL Interface Functions
//
#pragma argsused
int FAR PASCAL LibMain( HINSTANCE hInstance, WORD wDataSegment,
                                   WORD wHeapSize, LPSTR lpszCmdLine )
{
	hInst = hInstance;    // Save instance handle
	if ( wHeapSize != 0 )
		UnlockData( 0 );
	return 1;   // Indicate that the DLL was initialized successfully.
}

#pragma argsused
int FAR PASCAL WEP ( int bSystemExit )
{
	return 1;
}
//****************************************************************************
//
// FUNCTION: AppAttrsDlgProc(HWND, unsigned, WORD, LONG)
//
// PURPOSE:  Sample Dialog to update attributes for Application Object
//
//****************************************************************************
#pragma argsused
BOOL CALLBACK _export AppAttrsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
char szBuff[255];
	switch (message) 
	{
		case WM_INITDIALOG:
			SetDlgItemText(hWnd, 100, UserGetAttrValue("Programmer", szBuff));
			UserGetAttrValue("DLLs", szBuff);
			switch(*szBuff)
			{
				case 'B':  // Borland Custom Controls
					SendDlgItemMessage(hWnd, 102, BM_SETCHECK, TRUE, 0);
					break;
				case 't':  // Drover's Toolbox
					SendDlgItemMessage(hWnd, 103, BM_SETCHECK, TRUE, 0);
					break;
				default:            // No Custom Controls
					SendDlgItemMessage(hWnd, 101, BM_SETCHECK, TRUE, 0);
					break;
                        }
		
			UserGetAttrValue("Type", szBuff);
			switch(*szBuff)
			{
				case 'D':   // Database App
                                	if (!strcmp(szBuff, "Database"))
						SendDlgItemMessage(hWnd, 106, BM_SETCHECK, TRUE, 0);
					else
						SendDlgItemMessage(hWnd, 107, BM_SETCHECK, TRUE, 0);
					break;
				case 'M':  // MDI Application
					SendDlgItemMessage(hWnd, 105, BM_SETCHECK, TRUE, 0);
					break;
				default:      // Standard App
					SendDlgItemMessage(hWnd, 104, BM_SETCHECK, TRUE, 0);
					break;
                        }
			break;
		case WM_CLOSE:
			PostMessage(hWnd, WM_COMMAND, IDCANCEL, 0L);
			break; 
		case WM_COMMAND:
			switch (wParam) 
			{
				case IDOK:				// End with update
					GetDlgItemText(hWnd, 100, szBuff, 255);
					UserSetAttrValue("Programmer", szBuff);

					if (SendDlgItemMessage(hWnd, 102, BM_GETCHECK, 0, 0))
						UserSetAttrValue("DLLs", "BWCC");
					else if (SendDlgItemMessage(hWnd, 103, BM_GETCHECK, 0, 0))
						UserSetAttrValue("DLLs", "tbPro1;tbPro2;tbPro3");
					else
						UserSetAttrValue("DLLs", "");

					if (SendDlgItemMessage(hWnd, 105, BM_GETCHECK, 0, 0))
						UserSetAttrValue("Type", "MDI");
					else if (SendDlgItemMessage(hWnd, 106, BM_GETCHECK, 0, 0))
						UserSetAttrValue("Type", "Database");
					else if (SendDlgItemMessage(hWnd, 107, BM_GETCHECK, 0, 0))
						UserSetAttrValue("Type", "DatabaseMDI");
					else
						UserSetAttrValue("Type", "Standard");

					EndDialog(hWnd, TRUE);
					break;
				case IDCANCEL:		// End with no update
					EndDialog(hWnd, FALSE);
					break;
			} 
			break;
		default:
			return FALSE;
	}
	return TRUE;
}
