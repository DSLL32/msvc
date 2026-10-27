#ifndef __JTOOLS_HXX
#define __JTOOLS_HXX
#include <CommonVu.hxx>
#include <drawobj.hxx>

#define MAX_TOOLS	50 	// Can be safely increased

class ToolItem
{
  int nID;
public:
  Bitmap	*pBitmap1, *pBitmap2;
  Bool bState, bButton, bBorder, bShadow, bEnabled;
  Rectangle *rect;

  ToolItem(const char *Type, int id, Point Pt, Dimension Size, ResID BitmapID1, ResID BitmapID2, const char *Shadow, const char *Border);
  ~ToolItem();
  void Show(pWindow c);
  void SetState(Bool State){bState = State;}
  Bool GetState(){return bState;}
  void Enable(Bool bFlag){bEnabled = bFlag;}
  int  GetID(){return nID;}
};

class ToolBar : public ChildAppWindow
{
protected:
  Pen *ShadowPen;
  Brush *ButtonBrush;
  Bool bButtonDown;
  ToolItem *SelToolItem;
	int nNumTools;
public:
	ToolItem *ToolItems[MAX_TOOLS];
	ToolBar(pWindow w, int nHeight);
	~ToolBar();
	void AddTool(const char *Type, int id, Point Pt, Dimension Size, ResID BitmapID1, ResID BitmapID2, const char *Shadow, const char *Border)
	{ 
		if (nNumTools < MAX_TOOLS)
			ToolItems[nNumTools++] = new ToolItem(Type, id, Pt, Size, BitmapID1, BitmapID2, Shadow, Border); 
	}
	void SetItemState(int ID, Bool bState);

	virtual void Expose(ExposeEvt e);
	virtual void MouseButtonUp(MouseEvt e);
	virtual void MouseButtonDn(MouseEvt e);
	virtual void MouseDrag(MouseEvt e);
};

#endif
