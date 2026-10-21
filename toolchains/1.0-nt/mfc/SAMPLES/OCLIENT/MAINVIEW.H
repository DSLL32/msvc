// mainview.h : interface of the CMainView class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.



class CRectItem;

class CMainView : public CView
{
protected: // create from serialization only
	CMainView();
	DECLARE_DYNCREATE(CMainView)

// Attributes
public:
	CMainDoc* GetDocument()
		{
			ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMainDoc)));
			return (CMainDoc*) m_pDocument;
		}

	CRectItem* GetSelection() const
		{ return m_pSelection; } // Just the top-most window
	void SetSelection(CRectItem* pNewSel); // supports single selection

// Operations
public:

// Implementation
protected:
	CRectItem* m_pSelection;
	// Tracking state (global with capture)
	static CRectItem* NEAR pItemTracking;   // NULL => no tracking
	static CRect NEAR rectTrackOrigin;
	static BOOL NEAR bTrackSizing;          // TRUE => resizing, FALSE => moving
	static CPoint NEAR ptTrackOrigin;

	CRectItem* GetHitItem(CPoint point);

public:
	virtual ~CMainView();
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Printing support
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);

	// OLE Client support
	virtual BOOL IsSelected(const CObject* pDocItem) const;
	BOOL DoCopySelection();
	CRectItem* DoPasteItem(BOOL bLink);

// Generated message map functions
protected:
	//{{AFX_MSG(CMainView)
	afx_msg void OnPaste();
	afx_msg void OnPasteLink();
	afx_msg void OnInsertObject();
	afx_msg void OnUpdateEditMenu(CCmdUI* pCmdUI);
	afx_msg void OnEditClear();
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnUpdateTrackSize(CCmdUI* pCmdUI);
	afx_msg void OnToggleTrackSize();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
