#include <windows.h>
#include <string.h>
#include "jtools.h"

extern HINSTANCE hInst;
static char *szToolbarProp = "Toolbar";		// Property Name

//******************************************************************************************
//	Create Tool Bar 
//******************************************************************************************
HWND CreateToolbar(HWND hParentWnd, int nHeight)
{
	HWND hToolWnd = CreateWindow(					
  		"ToolBarClass",
		  "",
  		WS_CHILD|WS_VISIBLE,
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN),	 // Default to largest possible
		nHeight,
		hParentWnd,	NULL,	hInst, NULL);

	return hToolWnd;
}
//****************************************************************************
//
// FUNCTION: ToolbarProc(HWND, UINT, WPARAM, LPARAM)
//
// PURPOSE:  Processes messages for Toolbar
//
//****************************************************************************
LRESULT CALLBACK _export ToolbarProc(HWND hWnd, UINT message,
	WPARAM wParam, LPARAM lParam)
{
	GLOBALHANDLE hMem;
	LPTBSTRUCT lpToolbar;
	HDC			 hDC;
	PAINTSTRUCT ps;
	RECT 	rcWin;
	  HPEN 	hOldPen;
	static bButtonDown;
	int 	i;

	switch (message) 
	{
		case WM_CREATE:
			hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(TBSTRUCT));

	  		if (hMem == NULL)	 // Not enough memory
			{  
				MessageBox(hWnd, "Insufficient memory.", NULL,
    					MB_OK | MB_ICONEXCLAMATION);
				return NULL;
  			}

	  		// Save the Toolbar handle as a window property.
  			SetProp(hWnd, szToolbarProp, hMem);

	  		lpToolbar = (LPTBSTRUCT) GlobalLock(hMem);
			lpToolbar->nNumTools = 0;
			lpToolbar->bButtonDown = FALSE;
			lpToolbar->hShadowPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
			lpToolbar->hButtonBrush = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
			lpToolbar->nSelToolItem = -1;
	  		GlobalUnlock(hMem);

			bButtonDown = FALSE;
			break;

		case WM_PAINT:
			memset(&ps, 0, sizeof(PAINTSTRUCT));
			hDC = BeginPaint(hWnd, &ps);

			hMem = GetProp(hWnd, szToolbarProp);
  			lpToolbar = (LPTBSTRUCT) GlobalLock(hMem);

			// Paint Tool Bar borders and Show ToolItems

	  		GetClientRect( hWnd, (LPRECT)&rcWin );
			hOldPen = (HPEN)SelectObject(hDC, GetStockObject(BLACK_PEN));
			MoveTo(hDC, 0, rcWin.bottom-1); LineTo(hDC, rcWin.right, rcWin.bottom-1);

			SelectObject(hDC, lpToolbar->hShadowPen);
			MoveTo(hDC, 0, rcWin.bottom-2); LineTo(hDC, rcWin.right, rcWin.bottom-2);

			SelectObject(hDC, hOldPen);

			// Show each ToolItem
			for ( i=0; i<lpToolbar->nNumTools; i++ )
				ShowToolItem(&lpToolbar->TItems[i], hDC, lpToolbar->hButtonBrush,
					lpToolbar->hShadowPen);
  		
			GlobalUnlock(hMem);
			EndPaint(hWnd, &ps);
			break;

		case WM_LBUTTONDOWN:
			hMem = GetProp(hWnd, szToolbarProp);
  			lpToolbar = (LPTBSTRUCT) GlobalLock(hMem);

			for ( i=0; i<lpToolbar->nNumTools; i++ )
			{
				if ( HitTest(&lpToolbar->TItems[i], LOWORD(lParam),	HIWORD(lParam)) )
				{
					lpToolbar->nSelToolItem = i;	 // Save selected index

					SetToolItemState(&lpToolbar->TItems[i], !lpToolbar->TItems[i].bState );
					hDC = GetDC(hWnd);
					ShowToolItem(&lpToolbar->TItems[i], hDC, lpToolbar->hButtonBrush,
						lpToolbar->hShadowPen);
					ReleaseDC(hWnd, hDC);
				
					// Tell Parent the CheckBox has been set
					if ( !lpToolbar->TItems[i].bButton )	
						PostMessage(GetParent(hWnd), WM_COMMAND, lpToolbar->TItems[i].nID, 0);
				
					break;	 // Don't activate more than 1 if overlap
				}
			}
			GlobalUnlock(hMem);
			bButtonDown = TRUE;
			SetCapture(hWnd);
			break;
		case WM_MOUSEMOVE:
			hMem = GetProp(hWnd, szToolbarProp);
  			lpToolbar = (LPTBSTRUCT) GlobalLock(hMem);

			i = lpToolbar->nSelToolItem;	// Index to selected Tool
			if ( bButtonDown && i>=0 && lpToolbar->TItems[i].bButton )
			{
				if ( HitTest(&lpToolbar->TItems[i], LOWORD(lParam),	HIWORD(lParam))
					!= lpToolbar->TItems[i].bState )
				{
					SetToolItemState(&lpToolbar->TItems[i], !lpToolbar->TItems[i].bState );
					
					hDC = GetDC(hWnd);
					ShowToolItem(&lpToolbar->TItems[i], hDC, lpToolbar->hButtonBrush,
						lpToolbar->hShadowPen);
					ReleaseDC(hWnd, hDC);
				}
			}

			GlobalUnlock(hMem);
			break;
		case WM_LBUTTONUP:
			hMem = GetProp(hWnd, szToolbarProp);
  			lpToolbar = (LPTBSTRUCT) GlobalLock(hMem);

			for ( i=0; i<lpToolbar->nNumTools; i++ )
			{
				if ( HitTest(&lpToolbar->TItems[i], LOWORD(lParam),	HIWORD(lParam))
					&& lpToolbar->TItems[i].bState )
				{
					if ( lpToolbar->TItems[i].bButton )
					{
						SetToolItemState(&lpToolbar->TItems[i], !lpToolbar->TItems[i].bState );
						hDC = GetDC(hWnd);
						ShowToolItem(&lpToolbar->TItems[i], hDC, lpToolbar->hButtonBrush,
							lpToolbar->hShadowPen);
						ReleaseDC(hWnd, hDC);

						// Tell Parent the button has been set
						PostMessage(GetParent(hWnd), WM_COMMAND, lpToolbar->TItems[i].nID, 0);	
					}
				}
				else
				{
					if ( lpToolbar->TItems[i].bButton && lpToolbar->TItems[i].bState )
					{
						SetToolItemState(&lpToolbar->TItems[i], !lpToolbar->TItems[i].bState );
						hDC = GetDC(hWnd);
						ShowToolItem(&lpToolbar->TItems[i], hDC, lpToolbar->hButtonBrush,
							lpToolbar->hShadowPen);
						ReleaseDC(hWnd, hDC);
					}
				}
			}
			GlobalUnlock(hMem);
			bButtonDown = FALSE;
			ReleaseCapture();
			break;

		case WM_DESTROY:
  			// Delete anything allocated in Toolbar Struct
			hMem = GetProp(hWnd, szToolbarProp);
  			lpToolbar = (LPTBSTRUCT) GlobalLock(hMem);
			DeleteObject(lpToolbar->hShadowPen);
			// Delete all ToolItem Bitmaps
			for ( i=0; i<lpToolbar->nNumTools; i++ )
			{
				if ( lpToolbar->TItems[i].hBitmap1 ) 
	   			DeleteObject(lpToolbar->TItems[i].hBitmap1);
				if ( lpToolbar->TItems[i].hBitmap2 ) 
	   			DeleteObject(lpToolbar->TItems[i].hBitmap2);
			}

	  		GlobalUnlock(hMem);

			GlobalFree(GetProp(hWnd, szToolbarProp));	// Free Toolbar Struct
			RemoveProp(hWnd, szToolbarProp);	// Clean up property list
			break;
		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (NULL);
}

void AddToolItem(HWND hToolbar, LPSTR Type, int id, int X,int Y,int W,int H, LPSTR szBitmap1,
LPSTR szBitmap2, LPSTR Shadow, LPSTR Border )
{
	GLOBALHANDLE hMem;
	LPTBSTRUCT lpTB;
	LPTITEMSTRUCT lpTItem;

	hMem = GetProp(hToolbar, szToolbarProp);
	lpTB = (LPTBSTRUCT) GlobalLock(hMem);
	lpTItem = &lpTB->TItems[lpTB->nNumTools];


	lpTItem->nID = id;
	lpTItem->hBitmap1 = LoadBitmap(hInst, szBitmap1);
	lpTItem->hBitmap2 = LoadBitmap(hInst, szBitmap2);
	
	lpTItem->rect.left = X;
	lpTItem->rect.top = Y;
	lpTItem->rect.right = X + W;
	lpTItem->rect.bottom = Y + H;
	lpTItem->bState = FALSE;
	lpTItem->bEnabled = TRUE;

	if ( *Shadow == 'Y' )
		lpTItem->bShadow = TRUE;
	else
		lpTItem->bShadow = FALSE;

	if ( *Border == 'Y' )
		lpTItem->bBorder = TRUE;
	else
		lpTItem->bBorder = FALSE;

	if ( !_fstricmp(Type, "Button") )
		lpTItem->bButton = TRUE;
	else
		lpTItem->bButton = FALSE;
	
	if ( lpTB->nNumTools < MAXTOOLS-1 )
		lpTB->nNumTools++;		 // Increment Number of Tools

	GlobalUnlock(hMem);
}

void ShowToolItem(LPTITEMSTRUCT lpTItem, HDC PaintDC, HBRUSH hButtonBrush, HPEN hShadowPen)
{
	HDC MemoryDC;
	HANDLE OldBitmapHandle;
	DWORD dwMode; 

	HPEN hOldPen = (HPEN)SelectObject(PaintDC, GetStockObject(BLACK_PEN));
	HBRUSH hOldBrush = (HBRUSH)SelectObject(PaintDC, hButtonBrush);

	int	nOffset = 0, nShift = 0;

	if ( lpTItem->bBorder )
		nOffset++;
	if ( lpTItem->bShadow )
		nOffset++;
	if ( lpTItem->bState && lpTItem->bShadow)
		nShift=1;

	if (lpTItem->bEnabled)
		dwMode = SRCCOPY;
	else
		dwMode = MERGECOPY;

	if (lpTItem->bBorder)
		Rectangle(PaintDC, lpTItem->rect.left, lpTItem->rect.top,
			lpTItem->rect.right, lpTItem->rect.bottom);
	else
	  FillRect(PaintDC, &lpTItem->rect, hButtonBrush);

	if ( !lpTItem->hBitmap1 )
		return;

	MemoryDC = CreateCompatibleDC(PaintDC);
	if (lpTItem->bState && lpTItem->hBitmap2)
		OldBitmapHandle = SelectObject(MemoryDC, lpTItem->hBitmap2);
	else
		OldBitmapHandle = SelectObject(MemoryDC, lpTItem->hBitmap1);
	BitBlt(PaintDC, lpTItem->rect.left+nOffset+nShift, lpTItem->rect.top+nOffset+nShift,
		lpTItem->rect.right-lpTItem->rect.left, lpTItem->rect.bottom-lpTItem->rect.top,
		MemoryDC, 0, 0, dwMode);

	SelectObject(MemoryDC, OldBitmapHandle);
	DeleteDC(MemoryDC);

	if (lpTItem->bShadow)
	{
		if (lpTItem->bState)
	 		SelectObject(PaintDC, hShadowPen);
		else
	 		SelectObject(PaintDC, GetStockObject(WHITE_PEN));
	 	MoveTo(PaintDC, lpTItem->rect.left+nOffset-1, lpTItem->rect.bottom-nOffset);
	 	LineTo(PaintDC, lpTItem->rect.left+nOffset-1, lpTItem->rect.top+nOffset-1);
	 	LineTo(PaintDC, lpTItem->rect.right-nOffset+1, lpTItem->rect.top+nOffset-1);
	
		if (!lpTItem->bState)
		{
	 		SelectObject(PaintDC, hShadowPen);
	 		MoveTo(PaintDC, lpTItem->rect.right-nOffset, lpTItem->rect.top+nOffset-1);
	 		LineTo(PaintDC, lpTItem->rect.right-nOffset, lpTItem->rect.bottom-nOffset);
	 		LineTo(PaintDC, lpTItem->rect.left+nOffset-2, lpTItem->rect.bottom-nOffset);
	 		MoveTo(PaintDC, lpTItem->rect.right-nOffset-1,lpTItem->rect.top+nOffset);
	 		LineTo(PaintDC, lpTItem->rect.right-nOffset-1, lpTItem->rect.bottom-nOffset-1);
	 		LineTo(PaintDC, lpTItem->rect.left+nOffset-1, lpTItem->rect.bottom-nOffset-1);
		}
	 
	}

	SelectObject(PaintDC, hOldPen);
	SelectObject(PaintDC, hOldBrush);
}
BOOL HitTest(LPTITEMSTRUCT lpTItem, int nX,int nY)
{
	POINT pt;
	pt.x = nX, pt.y = nY;

	if (!lpTItem->bEnabled)			// Allow hits for Enabled Tools
		return FALSE;

	return PtInRect(&lpTItem->rect, pt);
}
void SetToolItemState(LPTITEMSTRUCT lpTItem, BOOL State)
{
	lpTItem->bState = State;
}
void EnableToolItem(LPTITEMSTRUCT lpTItem, BOOL bFlag)
{
	lpTItem->bEnabled = bFlag;
}
