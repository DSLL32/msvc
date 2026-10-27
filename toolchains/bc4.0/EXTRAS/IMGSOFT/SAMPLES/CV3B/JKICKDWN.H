//***************************************************************************
// jkickdwn.h - Definitions & prototypes to kickdown for MS Windows 
// They are needed to implement commmon dialogs, sound support, and
// message dispatch.  This file would need to be modified for other platforms				
//***************************************************************************
#ifndef __JKICKDWN_H
#define __JKICKDWN_H
// 
// Defines needed to #include commdlg.h for common dialogs
//
#define __WINDOWS_H	 // Make sure windows.h isn't pulled in

#define LONG								long
#define PASCAL      				_pascal
#define FAR         				_far
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef char FAR*           LPSTR;
typedef const char FAR*     LPCSTR;
typedef DWORD 							COLORREF;
#define WINAPI      				FAR PASCAL
#define CALLBACK    				FAR PASCAL
#define LPCSTR      				LPSTR
#define UINT        				WORD
#define LPARAM      				LONG
#define WPARAM      				WORD
#define LRESULT     				LONG
#define HMODULE     				HANDLE
#define HINSTANCE   				HANDLE
#define HLOCAL      				HANDLE
#define HGLOBAL     				HANDLE
#define HWND		     				HANDLE
#define HDC			     				HANDLE

#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)(g)<<8))|(((DWORD)(BYTE)(b))<<16)))
#define LF_FACESIZE 	      32

typedef struct tagLOGFONT
{
    int     lfHeight;
    int     lfWidth;
    int     lfEscapement;
    int     lfOrientation;
    int     lfWeight;
    BYTE    lfItalic;
    BYTE    lfUnderline;
    BYTE    lfStrikeOut;
    BYTE    lfCharSet;
    BYTE    lfOutPrecision;
    BYTE    lfClipPrecision;
    BYTE    lfQuality;
    BYTE    lfPitchAndFamily;
    char    lfFaceName[LF_FACESIZE];
} LOGFONT;

#include "commdlg.h"		// Use Microsoft Common Dialogs

#define WM_COMMAND    0x0111	 // For message dispatch in jtools

extern "C"
{
	BOOL  WINAPI sndPlaySound(LPCSTR, UINT);	 // Play .wav files
	BOOL  WINAPI PostMessage(HWND, UINT, WPARAM, LPARAM);	 // Dispatch a message
	HINSTANCE WINAPI LoadLibrary(LPCSTR);	 // Load a DLL
}
	//PROTECT_BEGIN JKickDown 
	//PROTECT_END 
#endif

