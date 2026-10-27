#include <owl.h>
#include <array.h>
#include <string.h>
#include "jtools.hpp"

TToolItem::TToolItem(PTWindowsObject AParent, Pchar Type, int id, int X,int Y,int W,int H, Pchar pBitmap1,
Pchar pBitmap2, Pchar Shadow, Pchar Border )
{
	nID = id;
	hBitmap1 = LoadBitmap(AParent->GetApplication()->hInstance, pBitmap1);
	hBitmap2 = LoadBitmap(AParent->GetApplication()->hInstance, pBitmap2);
	
	rect.left = X;
	rect.top = Y;
	rect.right = X + W;
	rect.bottom = Y + H;
	bState = FALSE;
	bEnabled = TRUE;

	if ( *Shadow == 'Y' )
		bShadow = TRUE;
	else
		bShadow = FALSE;

	if ( *Border == 'Y' )
		bBorder = TRUE;
	else
		bBorder = FALSE;

	if ( !stricmp(Type, "Button") )
		bButton = TRUE;
	else
		bButton = FALSE;
}
TToolItem::~TToolItem()
{
	if ( hBitmap1 ) 
	    DeleteObject(hBitmap1);

	if ( hBitmap2 ) 
	    DeleteObject(hBitmap2);
}
void TToolItem::Show(HDC PaintDC, HBRUSH hButtonBrush, HPEN hShadowPen)
{
	HDC MemoryDC;
	HBITMAP OldBitmapHandle;
	DWORD dwMode; 

	HPEN hOldPen = (HPEN) SelectObject(PaintDC, GetStockObject(BLACK_PEN));
	HBRUSH hOldBrush = (HBRUSH) SelectObject(PaintDC, hButtonBrush);

	int	nOffset = 0, nShift = 0;

	if ( bBorder )
		nOffset++;
	if ( bShadow )
		nOffset++;
	if ( bState && bShadow)
		nShift=1;

	if (bEnabled)
		dwMode = SRCCOPY;
	else
		dwMode = MERGECOPY;

	if (bBorder)
		Rectangle(PaintDC, rect.left, rect.top, rect.right, rect.bottom);
	else
	  FillRect(PaintDC, &rect, hButtonBrush);

	if ( !hBitmap1 )
		return;

	MemoryDC = CreateCompatibleDC(PaintDC);
	if (bState && hBitmap2)
		OldBitmapHandle = (HBITMAP) SelectObject(MemoryDC, hBitmap2);
	else
		OldBitmapHandle = (HBITMAP) SelectObject(MemoryDC, hBitmap1);
	BitBlt(PaintDC, rect.left+nOffset+nShift, rect.top+nOffset+nShift, rect.right-rect.left, rect.bottom-rect.top,
        	MemoryDC, 0, 0, dwMode);

	SelectObject(MemoryDC, OldBitmapHandle);
	DeleteDC(MemoryDC);

	if (bShadow)
	{
		if (bState)
	 		SelectObject(PaintDC, hShadowPen);
		else
	 		SelectObject(PaintDC, GetStockObject(WHITE_PEN));
	 	MoveTo(PaintDC, rect.left+nOffset-1, rect.bottom-nOffset);
	 	LineTo(PaintDC, rect.left+nOffset-1, rect.top+nOffset-1);
	 	LineTo(PaintDC, rect.right-nOffset+1, rect.top+nOffset-1);
	
		if (!bState)
		{
	 		SelectObject(PaintDC, hShadowPen);
	 		MoveTo(PaintDC, rect.right-nOffset, rect.top+nOffset-1);
	 		LineTo(PaintDC, rect.right-nOffset, rect.bottom-nOffset);
	 		LineTo(PaintDC, rect.left+nOffset-2, rect.bottom-nOffset);
	 		MoveTo(PaintDC, rect.right-nOffset-1, rect.top+nOffset);
	 		LineTo(PaintDC, rect.right-nOffset-1, rect.bottom-nOffset-1);
	 		LineTo(PaintDC, rect.left+nOffset-1, rect.bottom-nOffset-1);
		}
	 
	}

	SelectObject(PaintDC, hOldPen);
	SelectObject(PaintDC, hOldBrush);
}

int TToolItem::HitTest(int nX,int nY)
{
	POINT pt;
	pt.x = nX, pt.y = nY;

	if (!bEnabled)			// Allow hits for Enabled Tools
		return FALSE;

#pragma warn -stv
	return PtInRect(&rect, pt);
#pragma warn .stv
}

TToolBar::TToolBar(PTWindowsObject AParent, int nHeight) :
   TWindow(AParent, "")
{

	bButtonDown = FALSE;
	hShadowPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
	hButtonBrush = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
	SelToolItem = NULL;

	Attr.X = 0;
	Attr.Y = 0;
	Attr.W = GetSystemMetrics(SM_CXSCREEN);	 // Default to largest possible
	Attr.H = nHeight;
	Attr.Style = WS_CHILD|WS_VISIBLE;


	ToolItems = new Array(5, 0, 5);
}
TToolBar::~TToolBar()
{
	delete ToolItems;
	DeleteObject(hShadowPen);
}
void TToolBar::GetWindowClass(WNDCLASS& AWndClass)
{
  	TWindow::GetWindowClass(AWndClass);
	AWndClass.hbrBackground = hButtonBrush;
}
void TToolBar::Paint(HDC DC, PAINTSTRUCT& ) 
{
	RECT     rcWin;
  
  	GetClientRect( HWindow, (LPRECT)&rcWin );

	HPEN hOldPen = (HPEN) SelectObject(DC, GetStockObject(BLACK_PEN));
	MoveTo(DC, 0, rcWin.bottom-1); LineTo(DC, rcWin.right, rcWin.bottom-1);

	SelectObject(DC, hShadowPen);
	MoveTo(DC, 0, rcWin.bottom-2); LineTo(DC, rcWin.right, rcWin.bottom-2);

	SelectObject(DC, hOldPen);

	RArrayIterator ToolIterator = (RArrayIterator)(ToolItems->initIterator());
	while ( int(ToolIterator) != 0 )
	{
  		RObject AnObject = ToolIterator++;
		if ( AnObject != NOOBJECT )
			((PTToolItem)(&AnObject))->Show(DC, hButtonBrush, hShadowPen);
  	}
	delete &ToolIterator;
}

void TToolBar::WMLButtonDown(RTMessage Msg)
{
	SelToolItem = NULL;

	RArrayIterator ToolIterator = (RArrayIterator)(ToolItems->initIterator());
	while ( int(ToolIterator) != 0 )
  	{
  		RObject AnObject = ToolIterator++;
	  	if ( AnObject != NOOBJECT )
		{
			PTToolItem pToolItem = (PTToolItem)(&AnObject);
			if (pToolItem->HitTest(Msg.LP.Lo, Msg.LP.Hi))
			{
				SelToolItem = pToolItem;
				SelToolItem->SetState( !SelToolItem->GetState() );
				HDC DC = GetDC(HWindow);
				SelToolItem->Show(DC, hButtonBrush, hShadowPen);
				ReleaseDC(HWindow, DC);
				
				if ( !SelToolItem->bButton )	// Tell Parent the CheckBox has been set
					PostMessage(Parent->HWindow, WM_COMMAND, SelToolItem->GetID(), 0);	
				break;
			}
		}
  	}
	delete &ToolIterator;
	bButtonDown = TRUE;
	SetCapture(HWindow);
}
void TToolBar::WMMouseMove(RTMessage Msg)
{
	if ( bButtonDown && SelToolItem && SelToolItem->bButton )
	{
		if ( SelToolItem->HitTest(Msg.LP.Lo, Msg.LP.Hi) != SelToolItem->GetState())
		{
			SelToolItem->SetState( !SelToolItem->GetState() );
			HDC DC = GetDC(HWindow);
			SelToolItem->Show(DC, hButtonBrush, hShadowPen);
			ReleaseDC(HWindow, DC);
		}
	}
}
void TToolBar::WMLButtonUp(RTMessage Msg)
{
	RArrayIterator ToolIterator = (RArrayIterator)(ToolItems->initIterator());
	while ( int(ToolIterator) != 0 )
	{
  		RObject AnObject = ToolIterator++;
  		if ( AnObject != NOOBJECT )
		{
			PTToolItem pToolItem = (PTToolItem)(&AnObject);
			if (pToolItem->HitTest(Msg.LP.Lo, Msg.LP.Hi) && pToolItem->GetState())
			{
				if ( pToolItem->bButton )
				{
					pToolItem->SetState( !pToolItem->GetState() );
					HDC DC = GetDC(HWindow);
					pToolItem->Show(DC, hButtonBrush, hShadowPen);
					ReleaseDC(HWindow, DC);

					PostMessage(Parent->HWindow, WM_COMMAND,pToolItem->GetID(), 0);	// Tell Parent the button has been set
				}
			}
			else
			{
				if ( pToolItem->bButton && pToolItem->GetState())
				{
					pToolItem->SetState( !pToolItem->GetState() );
					HDC DC = GetDC(HWindow);
					pToolItem->Show(DC, hButtonBrush, hShadowPen);
					ReleaseDC(HWindow, DC);
				}
			}
		}
  }
  delete &ToolIterator;

  bButtonDown = FALSE;
	ReleaseCapture();
}
void TToolBar::SetItemState(int ID, int bState)
{
	RArrayIterator ToolIterator = (RArrayIterator)(ToolItems->initIterator());
	while ( int(ToolIterator) != 0 )
	{
		RObject AnObject = ToolIterator++;
		if ( AnObject != NOOBJECT )
			if(((PTToolItem)(&AnObject))->GetID() == ID)
			{
				((PTToolItem)(&AnObject))->SetState(bState);
				break;
			}
	}
	delete &ToolIterator;
}

