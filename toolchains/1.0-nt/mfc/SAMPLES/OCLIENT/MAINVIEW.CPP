// mainview.cpp : implementation of the CMainView class
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
#include "mainview.h"
#include "rectitem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainView

IMPLEMENT_DYNCREATE(CMainView, CView)

BEGIN_MESSAGE_MAP(CMainView, CView)
	//{{AFX_MSG_MAP(CMainView)
	ON_COMMAND(ID_EDIT_PASTE, OnPaste)
	ON_COMMAND(ID_EDIT_PASTE_LINK, OnPasteLink)
	ON_COMMAND(ID_OLE_INSERT_NEW, OnInsertObject)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, OnUpdateEditMenu)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_UPDATE_COMMAND_UI(ID_OBJECT_TRACK_SIZE, OnUpdateTrackSize)
	ON_COMMAND(ID_OBJECT_TRACK_SIZE, OnToggleTrackSize)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditMenu)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditMenu)
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainView construction/destruction

CMainView::CMainView()
{
	m_pSelection = NULL;
}

CMainView::~CMainView()
{
}

/////////////////////////////////////////////////////////////////////////////
// Static members for tracking state

CRectItem* NEAR CMainView::pItemTracking = NULL;        // NULL => no tracking
CRect NEAR CMainView::rectTrackOrigin;
BOOL NEAR CMainView::bTrackSizing;                  // TRUE => resizing, FALSE => moving
CPoint NEAR CMainView::ptTrackOrigin;

/////////////////////////////////////////////////////////////////////////////
// CMainView drawing


void CMainView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	CView::OnPrepareDC(pDC, pInfo);
	// set up a reasonable default context
	pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
	pDC->SetBkColor(::GetSysColor(COLOR_WINDOW));
}

void CMainView::OnDraw(CDC* pDC)
{
	CMainDoc* pDoc = GetDocument();

	ASSERT_VALID(pDC);

	// Draw all the CRectItems
	POSITION pos = pDoc->GetStartPosition();
	while (pos)
	{
		CRectItem* pItem = (CRectItem*)pDoc->GetNextItem(pos);
		if (pItem->IsKindOf(RUNTIME_CLASS(CRectItem)))
		{
			if (pItem->IsVisible() && pDC->RectVisible(pItem->m_rect))
			{
				pItem->Draw(pDC, pItem->m_rect, NULL, NULL);
				if (pItem == m_pSelection)
				{
					// hilite the selection
					pDC->PatBlt(pItem->m_rect.left, pItem->m_rect.top,
						pItem->m_rect.Width(), pItem->m_rect.Height(),
						PATINVERT);
				}
			}
		}
	}
}

// pHint is the deleted item or NULL if deselect/delete all
void CMainView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (pHint == NULL || pHint == m_pSelection)
		m_pSelection = NULL;
	if (pHint != NULL && pHint->IsKindOf(RUNTIME_CLASS(CRectItem)))
	{
		// just invalidate the one deleted item
		InvalidateRect(((CRectItem*)pHint)->m_rect);
	}
	else
	{
		// complete update
		CView::OnUpdate(pSender, lHint, pHint);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMainView printing

BOOL CMainView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

/////////////////////////////////////////////////////////////////////////////
// Selection support

BOOL CMainView::IsSelected(const CObject* pDocItem) const
{
	return (pDocItem == m_pSelection);
}


void CMainView::SetSelection(CRectItem* pNewSel)
{
	if (pNewSel == m_pSelection)
		return;

	if (m_pSelection != NULL)
	{
		// de-select the old item
		InvalidateRect(m_pSelection->m_rect);
	}

	if ((m_pSelection = pNewSel) != NULL)
	{
		ASSERT(m_pSelection->IsVisible());
		// select the new item
		InvalidateRect(m_pSelection->m_rect);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMainView diagnostics

#ifdef _DEBUG
void CMainView::AssertValid() const
{
	CView::AssertValid();
}

void CMainView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// Main 'Edit' menu commands

void CMainView::OnUpdateEditMenu(CCmdUI* pCmdUI)
{
	// most Edit menu commands are enabled only if we have a selection
	pCmdUI->Enable(m_pSelection != NULL);
}


void CMainView::OnEditCut()
{
	if (!DoCopySelection())
	{
		AfxMessageBox(IDP_CLIPBOARD_CUT_FAILED);
		return;
	}
	// cut it out
	OnEditClear();
}

void CMainView::OnEditCopy()
{
	if (!DoCopySelection())
		AfxMessageBox(IDP_CLIPBOARD_COPY_FAILED);
}

void CMainView::OnEditClear()
{
	ASSERT(m_pSelection != NULL);
	ASSERT(m_pSelection->IsVisible());
	GetDocument()->DeleteItem(m_pSelection);
	m_pSelection = NULL;
}

void CMainView::OnPaste()
{
	if (DoPasteItem(FALSE) == NULL)
		AfxMessageBox(IDP_GET_FROM_CLIPBOARD_FAILED);
	else
		GetDocument()->SetModifiedFlag();
}

void CMainView::OnPasteLink()
{
	if (DoPasteItem(TRUE) == NULL)
		AfxMessageBox(IDP_GET_FROM_CLIPBOARD_FAILED);
	else
		GetDocument()->SetModifiedFlag();
}


// Helper for cut/copy
BOOL CMainView::DoCopySelection()
{
	ASSERT(m_pSelection != NULL);

	if (!OpenClipboard())
		return FALSE;

	// Empty the clipboard
	EmptyClipboard();

	TRY
		m_pSelection->CopyToClipboard();
	CATCH (CException, e)
		return FALSE;
	END_CATCH

	CloseClipboard();
	return TRUE;
}

// Helper for paste/pastelink
CRectItem* CMainView::DoPasteItem(BOOL bLink)
{
	if (!OpenClipboard())
		return NULL;        // Couldn't open the clipboard

	BeginWaitCursor();

	CRectItem* pItem = GetDocument()->CreateItem();
	ASSERT_VALID(pItem);
	ASSERT(!pItem->IsVisible());
	char szName[OLE_MAXNAMESIZE];
	CreateNewUniqueName(szName);

	TRY
	{
		if (bLink)
		{
			if (!pItem->CreateLinkFromClipboard(szName))
				AfxThrowMemoryException();      // any exception will do
		}
		else
		{
			// paste embedded
			if (pItem->CreateFromClipboard(szName))
				TRACE("embedded an embedded OLE object\n");
			else if (pItem->CreateStaticFromClipboard(szName))
				TRACE("embedded a static picture object\n");
			else
				AfxThrowMemoryException();      // any exception will do
		}
	}
	CATCH (CException, e)
	{
		// general cleanup
		TRACE("failed to embed/link an OLE object\n");
		delete pItem;
		pItem = NULL;
	}
	END_CATCH

	CloseClipboard();
	EndWaitCursor();

	if (pItem == NULL)
		return NULL;
	// create as a live health item
	if (!pItem->IsVisible())
	{
		if (!pItem->UpdateItemRectFromServer() || !pItem->IsVisible())
		{
			AfxMessageBox(IDP_CLIPBOARD_ZERO_SIZE);
			delete pItem;
			return NULL;
		}
	}

	GetDocument()->SetModifiedFlag();
	GetDocument()->UpdateAllViews(NULL, 0, pItem);      // including this view
	SetSelection(pItem);
	return pItem;
}

/////////////////////////////////////////////////////////////////////////////
// Insert New Object and Activate Object

void CMainView::OnInsertObject()
{
	CString strTypeName;

	if (!AfxOleInsertDialog(strTypeName))
		return;     // no OLE type selected

	TRACE("Trying to Insert OLE item with type '%s'\n",
			(const char*)strTypeName);

	// First create the C++ object
	CRectItem* pItem = GetDocument()->CreateItem();
	ASSERT_VALID(pItem);
	ASSERT(!pItem->IsVisible());

	// Now create the OLE object/item
	char szTmp[OLE_MAXNAMESIZE];
	TRY
	{
		if (!pItem->CreateNewObject(strTypeName, CreateNewUniqueName(szTmp)))
			AfxThrowMemoryException();      // any exception will do
	}
	CATCH (CException, e)
	{
		AfxMessageBox(IDP_FAILED_TO_CREATE);
		// clean up item
		GetDocument()->DeleteItem(pItem);
		return;
	}
	END_CATCH

	// Created invisible (will become visible later when updated)
}


void CMainView::OnLButtonDblClk(UINT, CPoint)
{
	// Double click will activate the main verb
	if (m_pSelection != NULL)
		m_pSelection->DoVerb(OLEVERB_PRIMARY);
}

/////////////////////////////////////////////////////////////////////////////
// Hit detection, moving and resizing items

CRectItem* CMainView::GetHitItem(CPoint point)
{
	CMainDoc* pDoc = GetDocument();
	CRectItem* pItemHit = NULL;

	// Find the item hit by the mouse
	POSITION pos = pDoc->GetStartPosition();
	while (pos)
	{
		CRectItem* pItem = (CRectItem*)pDoc->GetNextItem(pos);
		if (pItem->IsKindOf(RUNTIME_CLASS(CRectItem)))
		{
			if (pItem->IsVisible() & pItem->m_rect.PtInRect(point))
			{
				pItemHit = pItem;
				// items later in the list are drawn on top - so keep looking
			}
		}
	}
	return pItemHit;
}

void CMainView::OnLButtonDown(UINT, CPoint point)
{
	SetSelection(NULL); // no selection while tracking
				// and will de-select if nothing hit
	if ((pItemTracking = GetHitItem(point)) != NULL)
	{
		ptTrackOrigin = point;
		rectTrackOrigin = pItemTracking->m_rect;
		SetCapture();
		// limit the tracking to the client area of the view
		CRect rectLimit;
		GetClientRect(rectLimit);
		ClientToScreen(rectLimit);
		ClipCursor(rectLimit);
		bTrackSizing = pItemTracking->IsSizing(point);
	}
}


void CMainView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CRectItem* pItem;
	if (GetCapture() == this && (pItem = pItemTracking) != NULL)
	{
		OnMouseMove(nFlags, point);     // does move tracking
		pItemTracking = NULL;
		ReleaseCapture();
		ClipCursor(NULL);
		SetSelection(pItem);

		// set dirty only if the item actually moved
		if (pItem->m_rect != rectTrackOrigin)
		{
			GetDocument()->SetModifiedFlag();
			if (bTrackSizing)
			{
				pItem->SetItemRectToServer();
				pItem->m_bTrackServerSize = FALSE;
					// stop size tracking
			}
		}
	}
}


#define MIN_DIMENSION   8

// Turn off warnings for /W4
#pragma warning(disable: 4204)  // non-constant aggregate initializer

static HCURSOR cursors[2] =
{
	::LoadCursor(NULL, IDC_ARROW),
	::LoadCursor(NULL, IDC_SIZENWSE)
};

void CMainView::OnMouseMove(UINT, CPoint point)
{
	if (pItemTracking != NULL && GetCapture() == this)
	{
		// in tracking mode
		CRect rectNew = rectTrackOrigin;
		if (bTrackSizing)
		{
			// sizing (limit size so it doesn't get too small)
			rectNew.right += (point.x - ptTrackOrigin.x);
			if (rectNew.Width() < MIN_DIMENSION)
				rectNew.right = rectNew.left + MIN_DIMENSION;
			rectNew.bottom += (point.y - ptTrackOrigin.y);
			if (rectNew.Height() < MIN_DIMENSION)
				rectNew.bottom = rectNew.top + MIN_DIMENSION;
		}
		else
		{
			// moving
			rectNew.OffsetRect(point - ptTrackOrigin);
		}
		if (rectNew != pItemTracking->m_rect)
		{
			// Track in all views (including ourself)
			GetDocument()->UpdateAllViews(NULL, 0, pItemTracking);
			pItemTracking->m_rect = rectNew;
			GetDocument()->UpdateAllViews(NULL, 0, pItemTracking);
		}
	}
	else if (m_pSelection != NULL)
	{
		// change the mouse to resizing cursor if over the selected item
		// system cursors (loaded at startup, no need to destroy them)
		SetCursor(cursors[m_pSelection->IsSizing(point) != 0]);
	}
}


BOOL CMainView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd == this && pItemTracking == NULL)
		return TRUE;        // will be handled in mouse move

	return CView::OnSetCursor(pWnd, nHitTest, message);
}


/////////////////////////////////////////////////////////////////////////////
// Right mouse for popup context sensitive menu

void CMainView::OnRButtonDown(UINT, CPoint point)
{
	SetSelection(GetHitItem(point));    // reselect item if appropriate

	if (m_pSelection != NULL)
	{
		CMenu bar;
		if (bar.LoadMenu(ID_OBJECT_POPUP_MENU))
		{
			CMenu& popup = *bar.GetSubMenu(0);
			ASSERT(popup.m_hMenu != NULL);

			ClientToScreen(&point);
			popup.TrackPopupMenu(TPM_RIGHTBUTTON,
				point.x, point.y,
				AfxGetApp()->m_pMainWnd); // route commands through main window
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Item commands

void CMainView::OnUpdateTrackSize(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pSelection != NULL);
	pCmdUI->SetCheck(m_pSelection != NULL &&
		m_pSelection->m_bTrackServerSize);
}

void CMainView::OnToggleTrackSize()
{
	if (m_pSelection != NULL)
		m_pSelection->m_bTrackServerSize = !m_pSelection->m_bTrackServerSize;
}

/////////////////////////////////////////////////////////////////////////////
