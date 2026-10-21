// svrdoc.h : interface of the CServerItem class
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



// in this example a CServerItem represents a node in a graph

class CServerDoc;

class CServerItem : public COleServerItem
{
// Constructors
public:
	CServerItem(CServerDoc* pContainerDoc);
	~CServerItem();
	static CServerItem* CreateRootNode(CServerDoc* pDoc);

	// create from parent node
	CServerItem* CreateChildNode(LPCSTR lpszDescription);
	CServerItem* PromptNewChildNode();  // create with user interface

// Attributes
	CServerDoc* GetDocument() const // return type-safe container
		{ return (CServerDoc*)COleServerItem::GetDocument(); }

	// visual attributes of the node
		// COleServerItem::m_strItemName = node name
	CString         m_strDescription;       // node description/caption
	CSize           m_sizeNode;             // size of bounding box

	enum EShape
	{
		shapeRect,
		shapeRound,
		shapeOval,
		shapeMax
	} m_shape;      // shape to draw

	// tree
	CServerItem*    m_pNextSibling;         // NULL if no sibling
	CServerItem*    m_pFirstChild;          // NULL if no children
	BOOL IsChild(const CServerItem* pPotentialChild) const;

	// for popup context sensitive menus (in IDR_POPUPS)
	virtual int GetPopupMenuIndex()
					{ return 0; }   // 0 for simple menu

// Operations
	virtual void CalcNodeSize();        // assumes screen size
	virtual BOOL DoDraw(CDC* pDC, CPoint pt, BOOL bSelected);
	virtual OLESTATUS OnSetData(OLECLIPFORMAT nFormat, HANDLE hData);

	// simple UI helpers
	BOOL PromptChangeNode();

	// moved from protected so it can be called from CServerView/Doc
	virtual void    Serialize(CArchive& ar);        // for native data

// Overridables
protected:
	virtual BOOL        OnDraw(CDC* pDC);
	virtual BOOL        OnGetTextData(CString& rStringReturn) const;
};

/////////////////////////////////////////////////////////////////////////////
