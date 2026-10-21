// svrview.h : interface of the CServerView class
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



class CServerView : public CScrollView
{
protected: // create from serialization only
	CServerView();
	DECLARE_DYNCREATE(CServerView)

// Attributes
public:
	CServerDoc* GetDocument()
		{
			ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CServerDoc)));
			return (CServerDoc*) m_pDocument;
		}
	CServerItem*    GetSelection();
	void SetSelection(CServerItem* pNewSel);

// Operations
public:

// Implementation
public:
	virtual ~CServerView();
	virtual void OnDraw(CDC* pDC);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
protected:
	// current selection - store the node and the last drawn rect
	CServerItem*    m_pSelectedNode;
	// upper left of drawing
	CPoint m_ptStart;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CServerView)
	afx_msg void OnChangeName();
	afx_msg void OnAddNode();
	afx_msg void OnUpdateHasSel(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnImportText();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
