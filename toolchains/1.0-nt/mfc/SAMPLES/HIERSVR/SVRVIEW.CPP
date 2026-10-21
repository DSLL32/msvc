// svrview.cpp : implementation of the CServerView class
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
#include "afxdlgs.h"        // for FileOpen
#include "hiersvr.h"

#include "svrdoc.h"
#include "svrview.h"
#include "svritem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerView

IMPLEMENT_DYNCREATE(CServerView, CScrollView)

BEGIN_MESSAGE_MAP(CServerView, CScrollView)
	//{{AFX_MSG_MAP(CServerView)
	ON_COMMAND(ID_CHANGE_NAME, OnChangeName)
	ON_COMMAND(ID_ADD_NODE, OnAddNode)
	ON_UPDATE_COMMAND_UI(ID_CHANGE_NAME, OnUpdateHasSel)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_UPDATE_COMMAND_UI(ID_ADD_NODE, OnUpdateHasSel)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateHasSel)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_TEXT, OnUpdateHasSel)
	ON_COMMAND(ID_IMPORT_TEXT, OnImportText)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerView construction/destruction

CServerView::CServerView()
{
	m_pSelectedNode = NULL;
	m_ptStart = CPoint(8, 4);       // upper left position
}

CServerView::~CServerView()
{
}


/////////////////////////////////////////////////////////////////////////////
// CServerView drawing


#define CX_INDENT   12
#define CX_BACKDENT 5
#define CY_SEPARATOR 4


static int DrawTree(CDC* pDC, CPoint ptStart, CServerItem* pRoot,
	CServerItem* pItemSel)
{
	ASSERT(pDC != NULL);
	ASSERT(pRoot != NULL);
	ASSERT(ptStart.x >= 0 && ptStart.y >= 0);

	// root node starts here
	int cyDrawn = 0;
	pRoot->DoDraw(pDC, ptStart, pRoot == pItemSel);
	cyDrawn += pRoot->m_sizeNode.cy + CY_SEPARATOR;

	if (pRoot->m_pFirstChild == NULL)
		return cyDrawn;     // nothing more to draw

	// indent for the kids
	CPoint ptKid(ptStart.x + CX_INDENT, ptStart.y + cyDrawn);

	// draw the kids
	int yMid = 0;
	for (CServerItem* pChild = pRoot->m_pFirstChild;
	  pChild != NULL; pChild = pChild->m_pNextSibling)
	{
		int cyKid;
		if ((cyKid = DrawTree(pDC, ptKid, pChild, pItemSel)) == -1)
			return -1;      // error

		yMid = ptKid.y + (pChild->m_sizeNode.cy)/2;
		// add the little line
		pDC->MoveTo(ptStart.x + CX_BACKDENT, yMid);
		pDC->LineTo(ptKid.x, yMid);
		cyDrawn += cyKid;
		ptKid.y += cyKid;
	}
	ASSERT(ptKid.y == ptStart.y + cyDrawn);

	// draw the connecting line (down to last yMid)
	ASSERT(yMid != 0);
	int xLine = ptStart.x + CX_BACKDENT;
	pDC->MoveTo(xLine, ptStart.y + pRoot->m_sizeNode.cy);
	pDC->LineTo(xLine, yMid);
	return cyDrawn;
}

void CServerView::OnDraw(CDC* pDC)
{
	if (DrawTree(pDC, m_ptStart, GetDocument()->m_pRoot, m_pSelectedNode) == -1)
	{
		TRACE("Error drawing tree!!\n");
		::MessageBeep(-1);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Simple selection

// calculate the bounding rect for a given item in the context of this view
static BOOL FindItemRect(CServerItem* pItemFind, LPRECT lpRect,
	CPoint& ptStart, CServerItem* pRoot)
{
	ASSERT(pItemFind != NULL);
	ASSERT(lpRect != NULL);
	ASSERT(ptStart.x >= 0 && ptStart.y >= 0);
	ASSERT(pRoot != NULL);

	if (pItemFind == pRoot)
	{
		// item rect does not include separator
		lpRect->right = (lpRect->left = ptStart.x) + pRoot->m_sizeNode.cx;
		lpRect->bottom = (lpRect->top = ptStart.y) + pRoot->m_sizeNode.cy;
		return TRUE;
	}
	ptStart.x += CX_INDENT;     // not essential for calculation
	ptStart.y += pRoot->m_sizeNode.cy + CY_SEPARATOR;

	// check the kids
	for (CServerItem* pChild = pRoot->m_pFirstChild;
	  pChild != NULL; pChild = pChild->m_pNextSibling)
	{
		if (FindItemRect(pItemFind, lpRect, ptStart, pChild))
			return TRUE;    // found
	}
	ptStart.x -= CX_INDENT;
	return FALSE;
}

// calculate the bounding rect for a given item in the context of this view
static CServerItem* HitDetect(CPoint point, CPoint& ptStart, CServerItem* pRoot)
{
	ASSERT(point.x >= 0 && point.y >= 0);
	ASSERT(ptStart.x >= 0 && ptStart.y >= 0);
	ASSERT(pRoot != NULL);

	CRect rect(ptStart, pRoot->m_sizeNode);
	if (rect.PtInRect(point))
		return pRoot;

	ptStart.x += CX_INDENT;
	ptStart.y += pRoot->m_sizeNode.cy + CY_SEPARATOR;
	if (ptStart.y >= point.y)
		return FALSE;       // no point in looking any lower

	// check the kids
	for (CServerItem* pChild = pRoot->m_pFirstChild;
	  pChild != NULL; pChild = pChild->m_pNextSibling)
	{
		CServerItem* pItem;
		if ((pItem = HitDetect(point, ptStart, pChild)) != NULL)
			return pItem;   // found
	}
	ptStart.x -= CX_INDENT;
	return NULL;
}

static void CalcBounding(CPoint& ptStart, CSize& sizeMax, CServerItem* pRoot)
{
	ASSERT(sizeMax.cx >= 0 && sizeMax.cy >= 0);
	ASSERT(ptStart.x >= 0 && ptStart.y >= 0);
	ASSERT(pRoot != NULL);

	ptStart.y += pRoot->m_sizeNode.cy + CY_SEPARATOR;
	if (ptStart.y > sizeMax.cy)
		sizeMax.cy = ptStart.y;

	if (ptStart.x + pRoot->m_sizeNode.cx > sizeMax.cx)
		sizeMax.cx = ptStart.x + pRoot->m_sizeNode.cx;
	ptStart.x += CX_INDENT;
	// add in the kids
	for (CServerItem* pChild = pRoot->m_pFirstChild;
	  pChild != NULL; pChild = pChild->m_pNextSibling)
	{
		CalcBounding(ptStart, sizeMax, pChild);
	}
	ptStart.x -= CX_INDENT;
}


CServerItem* CServerView::GetSelection()
{
	return m_pSelectedNode;
}

void CServerView::SetSelection(CServerItem* pNewSel)
{
	if (pNewSel == m_pSelectedNode)
		return;     // already set
	if (m_pSelectedNode != NULL)
	{
		// invalidate old selection
		CRect rect;
		CPoint pt = m_ptStart;
		VERIFY(FindItemRect(m_pSelectedNode, &rect, pt,
			GetDocument()->m_pRoot));
		rect -= GetDeviceScrollPosition();
		InvalidateRect(rect);
	}
	if ((m_pSelectedNode = pNewSel) != NULL)
	{
		// invalidate new selection
		CRect rect;
		CPoint pt = m_ptStart;
		VERIFY(FindItemRect(m_pSelectedNode, &rect, pt,
			GetDocument()->m_pRoot));
		rect -= GetDeviceScrollPosition();
		InvalidateRect(rect);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CServerView diagnostics

#ifdef _DEBUG
void CServerView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CServerView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CServerView commands

void CServerView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CScrollView::OnUpdate(pSender, lHint, pHint);

	// update our selection
	CServerItem* pRoot = GetDocument()->m_pRoot;
	ASSERT(pRoot != NULL);
	if (m_pSelectedNode == NULL)
	{
		m_pSelectedNode = pRoot;
	}
	else if (!pRoot->IsChild(m_pSelectedNode))
	{
		TRACE("Selected node is deleted - setting to no selection\n");
		m_pSelectedNode = NULL;
	}

	// Find the total size of the view
	CPoint pt = m_ptStart;
	CSize sizeTotal(0,0);
	CalcBounding(pt, sizeTotal, pRoot);
	CSize sizeLine(sizeDefault.cx, pRoot->m_sizeNode.cy + CY_SEPARATOR);
	CSize sizePage(sizeDefault.cx, sizeLine.cy * 10);
	SetScrollSizes(MM_TEXT, sizeTotal, sizePage, sizeLine);
}


void CServerView::OnUpdateHasSel(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_pSelectedNode != NULL);
}

void CServerView::OnChangeName()
{
	if (m_pSelectedNode != NULL &&
	  m_pSelectedNode->PromptChangeNode())
	{
		GetDocument()->SetModifiedFlag();
		GetDocument()->UpdateAllViews(NULL);

		if (m_pSelectedNode->IsConnected())
			m_pSelectedNode->NotifyChanged();
				// notify client immediately - will update automatic links
	}
}

void CServerView::OnAddNode()
{
	if (m_pSelectedNode == NULL)
		AfxThrowNotSupportedException();
	CServerItem* pNew;
	if ((pNew = m_pSelectedNode->PromptNewChildNode()) == NULL)
		return;

	// wire it in
	GetDocument()->SetModifiedFlag();
	m_pSelectedNode = pNew;
	GetDocument()->UpdateAllViews(NULL);
}




void CServerView::OnEditCopy()
{
	if (m_pSelectedNode == NULL)
		AfxThrowNotSupportedException();
	if (!m_pSelectedNode->CopyToClipboard(TRUE, TRUE))
		AfxMessageBox("Copy to clipboard failed");

}


void CServerView::OnLButtonDown(UINT, CPoint point)
{
	// change the selection as appropriate
	CPoint ptLog = m_ptStart;
	SetSelection(HitDetect(point + (CSize)GetDeviceScrollPosition(),
		ptLog, GetDocument()->m_pRoot));
}



void CServerView::OnRButtonDown(UINT nFlags, CPoint point)
{
	OnLButtonDown(nFlags, point);   // select first

	int iSub;
	if (m_pSelectedNode == NULL ||
	  (iSub = m_pSelectedNode->GetPopupMenuIndex()) == -1)
	   return;  // nothing if no selection or no popup

	CMenu popups;
	if (!popups.LoadMenu(IDR_POPUPS))
		AfxThrowResourceException();
	CMenu* pMenu = popups.GetSubMenu(iSub);
	ASSERT(pMenu != NULL);

	ClientToScreen(&point);
	pMenu->TrackPopupMenu(TPM_CENTERALIGN | TPM_RIGHTBUTTON,
		point.x, point.y, AfxGetApp()->m_pMainWnd);
}


void CServerView::OnImportText()
{
	if (m_pSelectedNode == NULL)
		AfxThrowNotSupportedException();

	CFileDialog dlg(TRUE, "txt", NULL,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||");

	if (dlg.DoModal() != IDOK)
		return;                 // stay with old data file

	CStdioFile file;
	if (!file.Open(dlg.GetPathName(), CFile::modeRead | CFile::typeText))
	{
		AfxMessageBox("Failed to open text file");
		return;
	}
	// read in lines appending items from this node
	char szT[256];
	int nCurLevel = 0;
#define MAX_LEVEL   20
	CServerItem* parents[MAX_LEVEL];
	parents[0] = m_pSelectedNode;   // must have a parent at the current level
	while (file.ReadString(szT, sizeof(szT)) != NULL)
	{
		int cch = strlen(szT);
		if (cch < 1 || szT[cch-1] != '\n')
		{
			AfxMessageBox("Text file contains too long a line - aborting read\n");
			break;      // leaves items loaded so far
		}
		szT[cch-1] = '\0';
		// check the indentation level
		char* pch = szT;
		while (*pch == '\t')
			pch++;
		int nLevel = (pch - szT)+1;
		ASSERT(nLevel >= 1);        // levels are 1 based, 0 is the root

		if (*pch == '\0')
			continue;       // skip blank lines
		if (nLevel > nCurLevel + 1 || nLevel >= MAX_LEVEL)
		{
			AfxMessageBox("Text file contains too many tabs = aborting read\n");
			break;      // leaves items loaded so far
		}

		parents[nLevel] = parents[nLevel-1]->CreateChildNode(pch);
		nCurLevel = nLevel;
	}
	file.Close();
	if (nCurLevel == 0)
		return;     // nothing added
	GetDocument()->UpdateAllViews(NULL);    // including this one
	GetDocument()->SetModifiedFlag();
}

