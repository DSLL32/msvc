#ifndef __JTOOLS_HPP
#define __JTOOLS_HPP

_CLASSDEF(TToolItem)
class TToolItem	: public Object
{
  int nID;
public:
  HBITMAP	hBitmap1, hBitmap2;
  BOOL bState, bButton, bBorder, bShadow, bEnabled;
  RECT rect;

  TToolItem(PTWindowsObject AParent, Pchar Type, int id, int X,int Y,int W,int H, Pchar lpBitmap1, Pchar lpBitmap2, Pchar Shadow, Pchar Border);
  ~TToolItem();
  void Show(HDC hdc, HBRUSH hButtonBrush, HPEN hShadowPen);
  void SetState(BOOL State){bState = State;}
  BOOL GetState(){return bState;}
  void Enable(BOOL bFlag){bEnabled = bFlag;}
  int  HitTest(int nX,int nY);
  int  GetID(){return nID;}

  // Object stuff so it can be stored in container
  virtual classType isA() const { return __firstUserClass; }
  virtual Pchar nameOf() const { return "TToolItem"; }
  virtual hashValueType hashValue() const { return 0; } 
  virtual int isEqual(RCObject AToolItem) const
    { return nID == ((RTToolItem)AToolItem).nID; }
  virtual void printOn(Rostream) const {  }
};

_CLASSDEF(TToolBar)
class TToolBar : public TWindow
{
protected:
  HPEN hShadowPen;
  HBRUSH hButtonBrush;
  BOOL bButtonDown;
  PTToolItem SelToolItem;
public:
  PArray ToolItems;
  TToolBar(PTWindowsObject AParent, int nHeight);
  ~TToolBar();
  virtual LPSTR GetClassName() {return "TToolBar";}
  virtual void GetWindowClass(WNDCLASS&);
  void SetItemState(int ID, int bState);
  virtual void Paint(HDC DC, PAINTSTRUCT& PS); 
  virtual void WMLButtonDown(RTMessage Msg)= [WM_FIRST + WM_LBUTTONDOWN];
  virtual void WMLButtonUp(RTMessage Msg)  = [WM_FIRST + WM_LBUTTONUP];
  virtual void WMMouseMove(RTMessage Msg)  = [WM_FIRST + WM_MOUSEMOVE];
};

#endif
