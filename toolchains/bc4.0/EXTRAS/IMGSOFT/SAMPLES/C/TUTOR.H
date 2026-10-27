//***************************************************************************
// tutor.h - Application Prototypes & Definitions
// 9/04/92 - 2:08:34pm
// John Doe
//***************************************************************************
//PROTECT_BEGIN Revisions 
//PROTECT_END 
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <mmsystem.h>		// Multimedia...needed for sndPlaySound()
#include <commdlg.h>		// Common Dialogs
#include "Tutor.hh"			// Resource Constants
#include "jtools.h"			// Module Class Definitions
//PROTECT_BEGIN AppIncludes 
//PROTECT_END 

int PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);


_EXTERN HINSTANCE 	hInst;			// Application Instance Handle
_EXTERN HWND   	hWndMain;       // Main Window Handle                             
//PROTECT_BEGIN AppGlobalVars 
//PROTECT_END 
LRESULT CALLBACK _export MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK _export NewWinWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK _export ABOUTDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CanCloseABOUT(HWND);
BOOL CALLBACK _export ADDRESSDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CanCloseADDRESS(HWND);
BOOL CALLBACK _export SEARCHDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CanCloseSEARCH(HWND);

