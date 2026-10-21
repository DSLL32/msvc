// rectitem.cpp : implementation of the CRectItem class
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



#include "stdafx.h"
#include "oclient.h"

#include "maindoc.h"
#include "rectitem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CRectItem, COleClientItem)

CRectItem::CRectItem(COleClientDoc* pContainer)
	: COleClientItem(pContainer)
{
	m_rect.SetRectEmpty();      // invisible item initially
	m_bTrackServerSize = TRUE;
}

CRectItem::~CRectItem()
{
}

void CRectItem::Invalidate(CView* pNotThisView)
{
	if (IsVisible())
		GetDocument()->UpdateAllViews(pNotThisView, 0, this);
}

/////////////////////////////////////////////////////////////////////////////

BOOL CRectItem::UpdateItemRectFromServer()
{
	ASSERT(m_bTrackServerSize);
	CRect rect;
	if (!GetBounds(&rect))
		return FALSE;       // blank

	// map from HIMETRIC to screen coordinates
	{
		CClientDC screenDC(NULL);
		screenDC.SetMapMode(MM_HIMETRIC);
		screenDC.LPtoDP(&rect);
	}
	// just set the item size
	if (m_rect.Size() != rect.Size())
	{
		Invalidate();   // invalidate the old size/position
		m_rect.right = m_rect.left + rect.Width();
		m_rect.bottom = m_rect.top + rect.Height();
		Invalidate();   // as well as the new size/position
	}
	return TRUE;
}

BOOL CRectItem::SetItemRectToServer()
{
	// set the official bounds for the embedded item
	CRect rect = m_rect;
	{
		CClientDC screenDC(NULL);
		screenDC.SetMapMode(MM_HIMETRIC);
		screenDC.DPtoLP(&rect);
	}
	TRY
	{
		SetBounds(rect);    // may do a wait
	}
	CATCH(CException, e)
	{
		return FALSE;   // links will not allow SetBounds
	}
	END_CATCH
	return TRUE;
}

// Sizing is permitted near the lower right of the selected rectangle
BOOL CRectItem::IsSizing(POINT pt)
{
	static int cxSizing = ::GetSystemMetrics(SM_CXFRAME);
	static int cySizing = ::GetSystemMetrics(SM_CYFRAME);

	return (IsVisible() && m_rect.PtInRect(pt) &&
		pt.x > m_rect.right - cxSizing &&
		pt.y > m_rect.bottom - cySizing);
}


void CRectItem::OnChange(OLE_NOTIFICATION wNotification)
{
	if (m_bTrackServerSize && !UpdateItemRectFromServer())
	{
		// Blank object
		if (wNotification == OLE_CLOSED)
		{
			// no data received for the object - destroy it
			ASSERT(!IsVisible());
			GetDocument()->DeleteItem(this);
			return;     // no update (item is gone now)
		}
	}
	if (wNotification == OLE_SAVED || wNotification == OLE_CHANGED)
		Dirty();
	Invalidate();   // any change will cause a redraw
}



void CRectItem::Serialize(CArchive& ar)
{
	CRect rect;

	if (ar.IsStoring())
	{
		ASSERT(IsVisible());        // only save non-blank items
		WORD w = 0x5500;        // magic value
		if (m_bTrackServerSize)
			w += 1;
		ar << w << m_rect;
		// default COleClientItem::Serialize will store type and name
	}
	else
	{
		WORD w;
		ar >> w >> m_rect;
		if (HIBYTE(w) != 0x55)
		{
			TRACE("Bad magic number in front of an item wnd\n");
			AfxThrowArchiveException(CArchiveException::generic);
		}
		m_bTrackServerSize = (w & 1) != 0;
	}
	COleClientItem::Serialize(ar);      // embedded object
}


/////////////////////////////////////////////////////////////////////////////


