// maindoc.h : interface of the CMainDoc class
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

class CMainDoc : public COleClientDoc
{
protected: // create from serialization only
	CMainDoc();
	DECLARE_DYNCREATE(CMainDoc)

// Attributes
public:

// Operations
public:
	CRectItem* CreateItem();        // this document only stores CRectItems
	void DeleteItem(CRectItem* pItem);

// Implementation
public:
	virtual ~CMainDoc();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:
	virtual BOOL    OnNewDocument();

// Generated message map functions
protected:
	//{{AFX_MSG(CMainDoc)
	afx_msg void OnEditClearAll();
	afx_msg void OnUpdateEditClearAll(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
