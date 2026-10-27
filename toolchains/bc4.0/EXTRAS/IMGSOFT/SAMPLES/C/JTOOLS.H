#ifndef __JTOOLS_H
#define __JTOOLS_H

#define MAXTOOLS	50		 // Max Number of ToolItems - Can be safely increased

typedef struct 
{
  int nID;
  HBITMAP	hBitmap1, hBitmap2;
  BOOL bState, bButton, bBorder, bShadow, bEnabled;
  RECT rect;
} TITEMSTRUCT, FAR *LPTITEMSTRUCT;

typedef struct 
{
  HPEN hShadowPen;
  HBRUSH hButtonBrush;
  BOOL bButtonDown;
  int nSelToolItem;
  int nNumTools;	 
  TITEMSTRUCT	TItems[MAXTOOLS];
} TBSTRUCT, FAR *LPTBSTRUCT;

HWND CreateToolbar(HWND hParentWnd, int nHieght);
LRESULT CALLBACK _export ToolbarProc(HWND, UINT, WPARAM, LPARAM);
void AddToolItem(HWND,LPSTR,int,int,int,int,int,LPSTR,LPSTR,LPSTR,LPSTR);
void ShowToolItem(LPTITEMSTRUCT lpTItem, HDC PaintDC, HBRUSH hButtonBrush, HPEN hShadowPen);
void SetToolItemState(LPTITEMSTRUCT lpTItem, BOOL State);
void EnableToolItem(LPTITEMSTRUCT lpTItem, BOOL bFlag);
BOOL HitTest(LPTITEMSTRUCT lpTItem, int nX,int nY);
#endif
