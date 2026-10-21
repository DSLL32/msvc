// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// class CRectItem - main COleClient item bounded by a rectangle

class CMainDoc;

class CRectItem : public COleClientItem
{
	DECLARE_DYNAMIC(CRectItem)
public:
	CRectItem(COleClientDoc* pContainer);
	~CRectItem();

// Attributes
	CRect   m_rect;         // where the item is on the screen (MM_TEXT)
							// COleClientDoc's list decides Z order
	BOOL    m_bTrackServerSize; // TRUE => resize item depending on server

	BOOL    IsVisible()       // BLANK objects are incomplete (and invisible)
			{ return !m_rect.IsRectEmpty(); }
	BOOL    IsSizing(POINT pt);

	CMainDoc* GetDocument()
		{
			ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMainDoc)));
			return (CMainDoc*) m_pDocument;
		}

// Operations
	void    Dirty()
				{ GetDocument()->SetModifiedFlag(); }
	void    Invalidate(CView* pNotThisView = NULL);
	BOOL    UpdateItemRectFromServer();
	BOOL    SetItemRectToServer();

	virtual void Serialize(CArchive& ar);       // from CObject - public to call directly

// Overridables
protected:
	virtual void OnChange(OLE_NOTIFICATION wNotification);
};


