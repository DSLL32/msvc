#include "jtools.hxx"
#include "jkickdwn.h"			 // Needed for Postmessage() 
#include <string.h>

ToolItem::ToolItem(const char *Type, int id, Point Pt, Dimension Size, 
	ResID BitmapID1, ResID BitmapID2, const char *Shadow, const char *Border)
{
	nID = id;
	pBitmap1 = new Bitmap(BitmapID1);
	pBitmap2 = new Bitmap(BitmapID2);
	
	rect = new Rectangle(Pt, Size);
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
ToolItem::~ToolItem()
{
	if ( pBitmap1 ) 
	    delete pBitmap1;

	if ( pBitmap2 ) 
	    delete pBitmap2;

	delete rect;
}
void ToolItem::Show(pWindow c)
{
	BitmapObject *bmo;
	long Mode; 
	Pen BlackPen(SolidLine, 1, Color(BLACK));
	Pen WhitePen(SolidLine, 1, Color(WHITE));
	Pen ShadowPen(SolidLine, 1, Color(125, 125, 125)); // DARK_GRAY
	Brush ButtonBrush(LightBrush);	
	Brush FrameBrush(BlackBrush); // Needed for PaintRectangle	Frame

	Pen *OldPen = c->ChangePen(&BlackPen);
	Brush *OldBrush = c->ChangeBrush(&ButtonBrush);

	int	nOffset = 0, nShift = 0;

	if ( bBorder )
		nOffset++;
	if ( bShadow )
		nOffset++;
	if ( bState && bShadow)
		nShift=1;

	c->PaintRectangle(*rect);	 // Fill With Gray

	if (bBorder)
	{
		c->ChangeBrush(&FrameBrush);
		c->PaintRectangle(*rect, Frame);
	}

	if ( !pBitmap1->Handle() )
		return;
	
	if (bState && pBitmap2->Handle())
		bmo = new BitmapObject(Point(rect->Left()+nOffset+nShift,rect->Bottom()+nOffset-nShift),
			pBitmap2->GetSize(), pBitmap2);
	else
		bmo = new BitmapObject(Point(rect->Left()+nOffset+nShift,rect->Bottom()+nOffset-nShift),
			pBitmap1->GetSize(), pBitmap1);
	
 	c->Draw(bmo);	 // Draw Bitmap
	delete bmo;

	if (bShadow)
	{
		if (bState)
			c->ChangePen(&ShadowPen);
		else
			c->ChangePen(&WhitePen);
	 	
		c->MoveTo(Point(rect->Left()+nOffset-1, rect->Bottom()+nOffset-1));
	 	c->LineTo(Point(rect->Left()+nOffset-1, rect->Top()-nOffset));
	 	c->LineTo(Point(rect->Right()-nOffset+1, rect->Top()-nOffset));
	
		if (!bState)
		{
			c->ChangePen(&ShadowPen);
	 		c->MoveTo(Point(rect->Right()-nOffset, rect->Top()-nOffset));
	 		c->LineTo(Point(rect->Right()-nOffset, rect->Bottom()+nOffset-1));
	 		c->LineTo(Point(rect->Left()+nOffset-2, rect->Bottom()+nOffset-1));
	 		c->MoveTo(Point(rect->Right()-nOffset-1, rect->Top()-nOffset-1));
	 		c->LineTo(Point(rect->Right()-nOffset-1, rect->Bottom()+nOffset));
	 		c->LineTo(Point(rect->Left()+nOffset-1, rect->Bottom()+nOffset));
		}
		else
		{
			if (!bBorder) 	
			{
				c->ChangePen(&WhitePen);
				c->MoveTo(Point(rect->Right()-nOffset, rect->Top()-nOffset));
		 		c->LineTo(Point(rect->Right()-nOffset, rect->Bottom()+nOffset));
	 			c->LineTo(Point(rect->Left()+nOffset-1, rect->Bottom()+nOffset));
			}
		}
	}
	
	c->ChangePen(OldPen);
	c->ChangeBrush(OldBrush);
}

ToolBar::ToolBar(pWindow Parent, int nHeight)
 : ChildAppWindow(Parent, 0, 0)
{
	bButtonDown = FALSE;
	SelToolItem = NULL;
	ChangeBackground(new Brush(LightBrush));
	ChangePosition(Point(0, Parent->CanvasRect().Top()-nHeight));
	//ChangeSize(Dimension(GetSystemMetrics(SM_CXSCREEN), nHeight)); 	
	ChangeSize(Dimension(1500, nHeight));	// May want to use GetSystemMetrics()
}
ToolBar::~ToolBar()
{
}
void ToolBar::Expose(ExposeEvt ) 
{
	
	Pen ShadowPen(SolidLine, 1, Color(125, 125, 125)); // DARK_GRAY
	Pen BlackPen(SolidLine, 1, Color(BLACK));

	Rectangle r1 = CanvasRect();

	Pen *OldPen = ChangePen(&BlackPen);
	MoveTo(Point(0, 0)); LineTo(Point(r1.Right(), 0));
	
	ChangePen(&ShadowPen);
	MoveTo(Point(0, 1)); LineTo(Point(r1.Right(), 1));
	
	for (int i=0; i<nNumTools; i++)
		ToolItems[i]->Show(this);
	
	ChangePen(OldPen);
}

void ToolBar::MouseButtonDn(MouseEvt e)
{
	if (e.isLeftButton()) 
	{
		SelToolItem = NULL;
		for (int i=0; i<nNumTools; i++)
		{
			if (ToolItems[i]->rect->PtInRect(e.Where())) 
			{
				SelToolItem = ToolItems[i];
				SelToolItem->SetState( !SelToolItem->GetState() );
				SelToolItem->Show(this);
				
				if ( !SelToolItem->bButton )	// Tell Parent the CheckBox has been set
					PostMessage(GetParent()->Handle(), WM_COMMAND, SelToolItem->GetID(), 0);	
				break;
			}
		}
	}
	bButtonDown = TRUE;
	MouseTrap(TRUE);
}
void ToolBar::MouseDrag(MouseEvt e)
{
	if ( bButtonDown && SelToolItem && SelToolItem->bButton )
	{
		if ( SelToolItem->rect->PtInRect(e.Where()) != SelToolItem->GetState() )
		{
			SelToolItem->SetState( !SelToolItem->GetState() );
			SelToolItem->Show(this);
		}
	}
}
void ToolBar::MouseButtonUp(MouseEvt e)
{
	if (e.isLeftButton()) 
	{
		for (int i=0; i<nNumTools; i++)
		{
			if ( ToolItems[i]->rect->PtInRect(e.Where()) && ToolItems[i]->GetState())
			{
				if ( ToolItems[i]->bButton )
				{
			 		ToolItems[i]->SetState( !ToolItems[i]->GetState() );
					ToolItems[i]->Show(this);
					PostMessage(GetParent()->Handle(), WM_COMMAND, SelToolItem->GetID(), 0);	
				}
			}
			else
			{
				if ( ToolItems[i]->bButton && ToolItems[i]->GetState())
				{
					ToolItems[i]->SetState( !ToolItems[i]->GetState() );
					ToolItems[i]->Show(this);
				}
			}
		}
	}
  bButtonDown = FALSE;
	MouseTrap(FALSE);
}
void ToolBar::SetItemState(int ID, int bState)
{
 	for (int i=0; i<nNumTools; i++)
	{
	 	if(ToolItems[i]->GetID() == ID)
	 	{
	 		ToolItems[i]->SetState(bState);
	 		break;
	 	}
	}
}
