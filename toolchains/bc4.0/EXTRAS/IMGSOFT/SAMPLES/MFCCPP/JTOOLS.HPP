#ifndef __JTOOLS_HPP
#define __JTOOLS_HPP
#include <afxcoll.h>		// Collections - Needed for CobArray

class TToolItem	: public CObject
{
  int nID;
public:
  CBitmap	Bitmap1, Bitmap2;
  BOOL bState, bButton, bBorder, bShadow, bEnabled;
  CRect rect;

  TToolItem(char *Type, int id, int X,int Y,int W,int H, 
		char *szBitmap1, char *szBitmap2, char *szShadow, char *szBorder);
  void Show(CDC *PaintDC, HBRUSH hButtonBrush, CPen *ShadowPen);
  void SetState(BOOL State){bState = State;}
  BOOL GetState(){return bState;}
  void Enable(BOOL bFlag){bEnabled = bFlag;}
  int  HitTest(CPoint Point);
  int  GetID(){return nID;}
};

class TToolBar : public CWnd
{
protected:
	CPen *pShadowPen;
	HBRUSH hButtonBrush;
	BOOL bButtonDown;
	TToolItem *SelToolItem;
public:
	CObArray ToolItems;
	TToolBar(CWnd *AParent, int nHeight);
	~TToolBar();
	void Add(char *Type, int id, int X,int Y,int W,int H, 
		char *szBitmap1, char *szBitmap2, char *szShadow, char *szBorder);
	afx_msg void OnDestroy() {delete this;}	// Allocated from heap, so delete

	void SetItemState(int ID, int bState);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint Point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint Point);
	afx_msg void OnMouseMove(UINT nButtons, CPoint pos);
	DECLARE_MESSAGE_MAP()
};

#endif
