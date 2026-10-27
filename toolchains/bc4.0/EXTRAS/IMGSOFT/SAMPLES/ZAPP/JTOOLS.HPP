#ifndef __JTOOLS_HPP
#define __JTOOLS_HPP

#ifndef MDI
#include "mdi.hpp" 	 // Only include once
#endif

class ToolItem
{
  int nID;
public:
  zBitmap	*pBitmap1, *pBitmap2;
  BOOL bState, bButton, bBorder, bShadow, bEnabled;
  zRect *rect;

  ToolItem(const char *Type, int id, int X,int Y,int W,int H, const char *Bitmap1, const char *Bitmap2, const char *Shadow, const char *Border);
  ~ToolItem();
  void Show(zDisplay *c, zBrush *ButtonBrush, zPen *ShadowPen);
  void SetState(BOOL State){bState = State;}
  BOOL GetState(){return bState;}
  void Enable(BOOL bFlag){bEnabled = bFlag;}
  int  GetID(){return nID;}
};

declPtr(ToolItem);
declDlist(ToolItem);

class ToolBar : public zPane
{
protected:
  zPen *ShadowPen;
  zBrush *ButtonBrush;
  BOOL bButtonDown;
  ToolItem *SelToolItem;
public:
	ToolItemDlist ToolItems;
  ToolBar(zWindow *w, zSizer *siz);
  ~ToolBar();
  void AddTool(const char *Type, int id, int X,int Y,int W,int H, const char *Bitmap1, const char *Bitmap2, const char *Shadow, const char *Border)
	{ ToolItems.append(new ToolItem(Type, id, X, Y, W, H, Bitmap1, Bitmap2, Shadow, Border)) ;}
  void SetItemState(int ID, int bState);

	virtual int draw(zDrawEvt* Ev);
	virtual int mouseButtonUp(zMouseClickEvt*);
	virtual int mouseButtonDown(zMouseClickEvt*);
	virtual int mouseMove(zMouseMoveEvt*);
};

// Margin Frame is needed for MDI Apps with Tool Bar
class MDIToolBarFrame : public zMDIMarginFrame
{
public:
// Use WS_BORDER for now because of placement of margin at bottom
	MDIToolBarFrame(zFrameWin *w, zSizer *siz, SIDE s) :
	zMDIMarginFrame(w,siz,WS_BORDER,"",s){};
	
	// Route tool bar commands to MDIFrame
	virtual int command(zCommandEvt *Ev) {return parent()->command(Ev);}
};

// Size Pane below toolbar
class zSizeBelowToolBar:public zSizer
{
	int nWinHt;
public:
	zSizeBelowToolBar(int Height): zSizer(){nWinHt=Height;}
	virtual void resize(zWindow *w,zSizeEvt *Ev)
	{
		MoveWindow(*w,0,nWinHt,Ev->width(),Ev->height()-nWinHt,1);
	}
};

#endif
