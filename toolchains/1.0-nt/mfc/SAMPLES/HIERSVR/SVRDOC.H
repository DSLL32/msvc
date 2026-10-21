// svrdoc.h : interface of the CServerDoc class
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


class CServerItem;

class CServerDoc : public COleServerDoc
{
protected: // create from serialization only
	CServerDoc();
	DECLARE_SERIAL(CServerDoc)

// Attributes
public:
	CServerItem*    m_pRoot;

// Operations
public:

// Implementation
public:
	virtual COleServerItem* OnGetEmbeddedItem();
	virtual ~CServerDoc();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	virtual void DeleteContents();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:
	virtual BOOL    OnNewDocument();

// Generated message map functions
protected:
	//{{AFX_MSG(CServerDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
