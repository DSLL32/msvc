#include "zapp.hpp"
#include "jtools.hpp"
#include <string.h>

ToolItem::ToolItem(const char *Type, int id, int X,int Y,int W,int H, 
	const char *Bitmap1, const char *Bitmap2, const char *Shadow, const char *Border)
{
	nID = id;
	pBitmap1 = new zBitmap(zResId(Bitmap1));
	pBitmap2 = new zBitmap(zResId(Bitmap2));
	
	rect = new zRect(X, Y, X+W, Y+H);
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
void ToolItem::Show(zDisplay *c, zBrush *ButtonBrush, zPen *ShadowPen)
{
	HDC MemoryDC;
	HANDLE OldBitmapHandle;
	DWORD dwMode; 
	c->pushPen(new zPen(BlackPen));
	c->pushBrush(ButtonBrush);

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
		c->rectangle(*rect);
	else
	  c->fill(*rect);

	if ( !*pBitmap1 )
		return;

	if (bState && *pBitmap2)
		c->bitmap(pBitmap2, rect->topLeft()+nOffset+nShift, dwMode);
	else
		c->bitmap(pBitmap1, rect->topLeft()+nOffset+nShift, dwMode);

	if (bShadow)
	{
		if (bState)
			c->pushPen(ShadowPen);
		else
			c->pushPen(new zPen(WhitePen));
	 	
		c->moveTo(rect->left()+nOffset-1, rect->bottom()-nOffset);
	 	c->lineTo(rect->left()+nOffset-1, rect->top()+nOffset-1);
	 	c->lineTo(rect->right()-nOffset+1, rect->top()+nOffset-1);
	
		if (!bState)
		{
			delete c->popPen();
			c->pushPen(ShadowPen);
	 		c->moveTo(rect->right()-nOffset, rect->top()+nOffset-1);
	 		c->lineTo(rect->right()-nOffset, rect->bottom()-nOffset);
	 		c->lineTo(rect->left()+nOffset-2, rect->bottom()-nOffset);
	 		c->moveTo(rect->right()-nOffset-1, rect->top()+nOffset);
	 		c->lineTo(rect->right()-nOffset-1, rect->bottom()-nOffset-1);
	 		c->lineTo(rect->left()+nOffset-1, rect->bottom()-nOffset-1);
			c->popPen();	 // Don't delete shadowpen
		}
		else
		{
			c->popPen();	 // Don't delete shadowpen
			if (!bBorder) 	
			{
				c->pushPen(new zPen(WhitePen));
				c->moveTo(rect->right()-nOffset, rect->top()+nOffset);
		 		c->lineTo(rect->right()-nOffset, rect->bottom()-nOffset);
	 			c->lineTo(rect->left()+nOffset-1, rect->bottom()-nOffset);
				delete c->popPen();	 
			}
		}
	}

	delete c->popPen();
	c->popBrush();
}

ToolBar::ToolBar(zWindow *w, zSizer *siz)
 : zPane(w, siz)
{

	bButtonDown = FALSE;
	ShadowPen = new zPen(GetSysColor(COLOR_BTNSHADOW), Solid, 1);
	ButtonBrush = new zBrush(GetSysColor(COLOR_BTNFACE));
	SelToolItem = NULL;
	WndBackColor=GetSysColor(COLOR_BTNFACE);
}
ToolBar::~ToolBar()
{
	delete ShadowPen;
	delete ButtonBrush;
}
int ToolBar::draw(zDrawEvt*) 
{
	zRect r, r1;
	ToolItemDlistIter trav(&ToolItems);
	
	getInterior(r1);

	canvas()->lock();

	canvas()->pushPen(new zPen(BlackPen));
	canvas()->moveTo(0, r1.bottom()-1); canvas()->lineTo(r1.right(), r1.bottom()-1);
	
	canvas()->pushPen(ShadowPen);
	canvas()->moveTo(0, r1.bottom()-2); canvas()->lineTo(r1.right(), r1.bottom()-2);
	
	canvas()->popPen();	 // Don't delete shadowpen
	
	canvas()->getDirty(r);
	delete canvas()->popPen();
	
	ToolItem* t;
	while((t=trav())!=0) 
		if (r.intersects(*t->rect)) 
			t->Show(canvas(), ButtonBrush, ShadowPen);
	
	canvas()->unlock();
	return 1;
}

int ToolBar::mouseButtonDown(zMouseClickEvt *e)
{
	if (e->isButton(1)) 
	{
		SelToolItem = NULL;
		ToolItemDlistIter trav(&ToolItems);

		ToolItem* t;
		while((t=trav())!=0) 
		{
			if (t->rect->contains(e->pos())) 
			{
				SelToolItem = t;
				SelToolItem->SetState( !SelToolItem->GetState() );
				canvas()->lock();
				SelToolItem->Show(canvas(), ButtonBrush, ShadowPen);
				canvas()->unlock();
				
				if ( !SelToolItem->bButton )	// Tell Parent the CheckBox has been set
					PostMessage(*parent(), WM_COMMAND, SelToolItem->GetID(), 0);	
				break;
			}
		}
	}
	bButtonDown = TRUE;
	SetCapture(*this);
	return 0;
}
int ToolBar::mouseMove(zMouseMoveEvt *e)
{
	if ( bButtonDown && SelToolItem && SelToolItem->bButton )
	{
		if ( SelToolItem->rect->contains(e->pos()) != SelToolItem->GetState())
		{
			SelToolItem->SetState( !SelToolItem->GetState() );
			canvas()->lock();
			SelToolItem->Show(canvas(), ButtonBrush, ShadowPen);
			canvas()->unlock();
		}
	}
	return 0;
}
int ToolBar::mouseButtonUp(zMouseClickEvt *e)
{
	if (e->isButton(1)) 
	{
		ToolItemDlistIter trav(&ToolItems);
		ToolItem* t;
		while((t=trav())!=0) 
		{
			if ( t->rect->contains(e->pos()) && t->GetState())
			{
				if ( t->bButton )
				{
			 		t->SetState( !t->GetState() );
					canvas()->lock();
					t->Show(canvas(), ButtonBrush, ShadowPen);
					canvas()->unlock();

			 		PostMessage(*parent(), WM_COMMAND, t->GetID(), 0);	// Tell Parent the button has been set
				}
			}
			else
			{
				if ( t->bButton && t->GetState())
				{
					t->SetState( !t->GetState() );
					canvas()->lock();
					t->Show(canvas(), ButtonBrush, ShadowPen);
					canvas()->unlock();
				}
			}
		}
	}
  bButtonDown = FALSE;
	ReleaseCapture();
	return 0;
}
void ToolBar::SetItemState(int ID, int bState)
{
	ToolItemDlistIter trav(&ToolItems);
	ToolItem* t;
	while((t=trav())!=0) 
	{
	 	if(t->GetID() == ID)
	 	{
	 		t->SetState(bState);
	 		break;
	 	}
	}
}
