// svritem.cpp : implementation of the CServerItem class
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
#include "hiersvr.h"

#include "svritem.h"
#include "svrdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerItem

CServerItem::CServerItem(CServerDoc* pContainerDoc)
	: COleServerItem(pContainerDoc)
{
	m_pNextSibling = NULL;
	m_pFirstChild = NULL;
	m_sizeNode.cx = m_sizeNode.cy = 0;
	m_shape = shapeRect;
}


// Special constructor for root node
CServerItem* CServerItem::CreateRootNode(CServerDoc* pDoc)
{
	CServerItem* pRoot = NULL;
	TRY
	{
		pRoot = new CServerItem(pDoc);
		pRoot->SetItemName("%ROOT%");       // name that link will resolve to
		pRoot->m_strDescription = "Root node";
		pRoot->CalcNodeSize();  // fills in m_size
	}
	CATCH(CException, e)
	{
		delete pRoot;
		pRoot = NULL;
	}
	END_CATCH

	return pRoot;
}


CServerItem::~CServerItem()
{
	// if we are the root of a document, clear it
	if (GetDocument()->m_pRoot == this)
		GetDocument()->m_pRoot = NULL;
	// delete child nodes
	CServerItem* pNext;
	for (CServerItem* pChild = m_pFirstChild;
	  pChild != NULL; pChild = pNext)
	{
		pNext = pChild->m_pNextSibling;
		delete pChild;
	}
}

BOOL CServerItem::IsChild(const CServerItem* pPotentialChild) const
{
	if (pPotentialChild == this)
		return TRUE;
	// walk through the children
	for (CServerItem* pChild = m_pFirstChild;
	  pChild != NULL; pChild = pChild->m_pNextSibling)
	{
		if (pChild->IsChild(pPotentialChild))
			return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////

#define CX_INSET    4
#define CY_INSET    2


void CServerItem::CalcNodeSize()
{
	CClientDC dcScreen(NULL);

	m_sizeNode = dcScreen.GetTextExtent(m_strDescription,
	   m_strDescription.GetLength());
	m_sizeNode += CSize(CX_INSET * 2, CY_INSET * 2);

	// set suggested HIMETRIC size
	m_rectBounds.SetRect(0, 0, m_sizeNode.cx, m_sizeNode.cy);
	dcScreen.SetMapMode(MM_HIMETRIC);
	dcScreen.DPtoLP(&m_rectBounds);
}


BOOL CServerItem::DoDraw(CDC* pDC, CPoint pt, BOOL bSelected)
{
	CRect rect(pt, m_sizeNode);
	// draw a the bounding shape
	BOOL bOK = TRUE;

	CPen* pOldPen = NULL;
	CPen penHilite;
	if (bSelected &&
	   penHilite.CreatePen(PS_SOLID, 1, RGB(255, 0, 0)))    // selection in red
		pOldPen = pDC->SelectObject(&penHilite);

	switch (m_shape)
	{
	case shapeRect:
		bOK = pDC->Rectangle(rect);
		break;
	case shapeRound:
		bOK = pDC->RoundRect(rect, CPoint(7, 7));
		break;
	case shapeOval: // really just a rounder rounded rect
		bOK = pDC->RoundRect(rect, CPoint(14, 14));
		break;
	default:
		TRACE("Error: unknown shape %d\n", m_shape);
		bOK = FALSE;
		break;
	}
	if (pOldPen != NULL)
		pDC->SelectObject(pOldPen);

	if (!bOK)
		return FALSE;

	// inset a bit
	rect.InflateRect(-CX_INSET, -CY_INSET);

	if (!pDC->ExtTextOut(rect.left, rect.top,
	  ETO_OPAQUE | ETO_CLIPPED, rect,
	  m_strDescription, m_strDescription.GetLength(), NULL))
		return FALSE;

	// all OK
	return TRUE;
}


BOOL CServerItem::OnDraw(CDC* pDC)
{
	// request from OLE to draw node
	pDC->SetMapMode(MM_TEXT); // always in pixels
	return DoDraw(pDC, CPoint(0,0), FALSE);
}

OLESTATUS CServerItem::OnSetData(OLECLIPFORMAT nFormat, HANDLE hData)
{
	OLESTATUS sc = COleServerItem::OnSetData(nFormat, hData);
	if (sc != OLE_OK)
		return sc;

	// when data changes mark as dirty - update all views
	GetDocument()->SetModifiedFlag(FALSE);  // clean now
	GetDocument()->UpdateAllViews(NULL);
	return OLE_OK;
}

/////////////////////////////////////////////////////////////////////////////

// Serializes to file or to embedded OLE stream
void CServerItem::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		CString strEmpty;
		ar << (WORD)m_shape;

		if (GetItemName() == m_strDescription)
			ar << strEmpty; // avoid saving redundant data
		else
			ar << GetItemName();
		ar << m_strDescription;

		// walk the kids as well
		WORD nKids = 0;
		for (CServerItem* pChild = m_pFirstChild;
		  pChild != NULL;
		  pChild = pChild->m_pNextSibling)
			nKids++;

		ar << nKids;
		for (pChild = m_pFirstChild;
		  pChild != NULL;
		  pChild = pChild->m_pNextSibling)
			pChild->Serialize(ar);
	}
	else
	{
		WORD shape;
		ar >> shape;
		m_shape = (EShape)shape;
		if (m_shape >= shapeMax)
			m_shape = shapeRect;
		CString strItemName;
		ar >> strItemName >> m_strDescription;
		if (strItemName.IsEmpty())
			SetItemName(m_strDescription);      // usually the same
		else
			SetItemName(strItemName);

		CalcNodeSize();     // calculate our new node size

		WORD nKids;
		ar >> nKids;
		CServerItem** ppLast = &m_pFirstChild;
		while (nKids--)
		{
			CServerItem* pChild = new CServerItem(GetDocument());
			pChild->Serialize(ar);

			*ppLast = pChild;
			ppLast = &(pChild->m_pNextSibling);
		}
		ASSERT(*ppLast == NULL);
	}
}

BOOL CServerItem::OnGetTextData(CString& rStringReturn) const
{
	// text is really the description
	rStringReturn = m_strDescription;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CChangeNameDlg dialog

class CChangeNameDlg : public CDialog
{
// Construction
public:
	CChangeNameDlg(CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CChangeNameDlg)
	enum { IDD = IDD_CHANGE_NAME };
	int     m_shape;
	CString m_strDescription;
	CString m_strLinkKey;
	CEdit   m_editDescription;
	CButton m_btnOK;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	// Generated message map functions
	//{{AFX_MSG(CChangeNameDlg)
	afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CChangeNameDlg::CChangeNameDlg(CWnd* pParent)
	: CDialog(CChangeNameDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CChangeNameDlg)
	//}}AFX_DATA_INIT
}

BOOL CChangeNameDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	OnChange(); // after DDX
	return FALSE;
}

void CChangeNameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChangeNameDlg)
	DDX_CBIndex(pDX, IDC_COMBO1, m_shape);
	DDX_Text(pDX, IDC_EDIT1, m_strDescription);
	DDX_Text(pDX, IDC_EDIT2, m_strLinkKey);
	DDX_Control(pDX, IDC_EDIT1, m_editDescription);
	DDX_Control(pDX, IDOK, m_btnOK);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CChangeNameDlg, CDialog)
	//{{AFX_MSG_MAP(CChangeNameDlg)
	ON_EN_CHANGE(IDC_EDIT1, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CChangeNameDlg::OnChange()
{
	if (m_btnOK.m_hWnd)
		m_btnOK.EnableWindow(m_editDescription.GetWindowTextLength() > 0);
}


////////////////////////////////////////////////////////////////

BOOL CServerItem::PromptChangeNode()
{
	CChangeNameDlg dlg;
	dlg.m_strDescription = m_strDescription;
	if (GetItemName() != m_strDescription)
		dlg.m_strLinkKey = GetItemName();
				// leave empty for tracking link name
	dlg.m_shape = m_shape;
	if (dlg.DoModal() == IDOK)
	{
		m_strDescription = dlg.m_strDescription;
		if (dlg.m_strLinkKey.IsEmpty())
			SetItemName(dlg.m_strDescription);      // same as description
		else
			SetItemName(dlg.m_strLinkKey);          // different key
		m_shape = (EShape)dlg.m_shape;
		CalcNodeSize(); // fills in m_size
		return TRUE;
	}
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CAddNodeDlg dialog

class CAddNodeDlg : public CDialog
{
// Construction
public:
	CAddNodeDlg(CWnd* pParent = NULL);  // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddNodeDlg)
	enum { IDD = IDD_ADD_NODE };
	CString m_strDescription;
	CString m_strLinkKey;
	int     m_shape;
	CEdit   m_editDescription;
	CButton m_btnOK;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CAddNodeDlg)
	afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAddNodeDlg::CAddNodeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddNodeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddNodeDlg)
	//}}AFX_DATA_INIT
}

BOOL CAddNodeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	OnChange(); // after DDX
	return FALSE;
}

void CAddNodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddNodeDlg)
	DDX_Text(pDX, IDC_EDIT1, m_strDescription);
	DDX_Text(pDX, IDC_EDIT2, m_strLinkKey);
	DDX_CBIndex(pDX, IDC_COMBO1, m_shape);
	DDX_Control(pDX, IDC_EDIT1, m_editDescription);
	DDX_Control(pDX, IDOK, m_btnOK);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddNodeDlg, CDialog)
	//{{AFX_MSG_MAP(CAddNodeDlg)
	ON_EN_CHANGE(IDC_EDIT1, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CAddNodeDlg::OnChange()
{
	if (m_btnOK.m_hWnd)
		m_btnOK.EnableWindow(m_editDescription.GetWindowTextLength() > 0);
}

////////////////////////////////////////////////////////////////
// Creating child nodes

// Special constructor for child node
CServerItem* CServerItem::CreateChildNode(LPCSTR lpszDescription)
{
	CServerItem* pNew = NULL;
	TRY
	{
		pNew = new CServerItem(GetDocument());
		pNew->m_strDescription = lpszDescription;
		pNew->SetItemName(pNew->m_strDescription);
			// both item name and description start out the same
		pNew->CalcNodeSize();   // fills in m_size

		// add as the last child
		CServerItem** ppLast = &m_pFirstChild;
		while (*ppLast != NULL)
			ppLast = &((*ppLast)->m_pNextSibling);
		*ppLast = pNew;
	}
	CATCH (CException, e)
	{
		delete pNew;
		pNew = NULL;
	}
	END_CATCH

	return pNew;
}

CServerItem* CServerItem::PromptNewChildNode()
{
	CAddNodeDlg dlg;
	dlg.m_shape = (EShape)shapeRect;
	if (dlg.DoModal() != IDOK)
		return NULL;

	CServerItem* pNew = CreateChildNode(dlg.m_strDescription);
	if (pNew == NULL)
		AfxThrowMemoryException();
	pNew->m_shape = (EShape)dlg.m_shape;

	ASSERT(pNew->GetItemName() == pNew->m_strDescription);
	if (!dlg.m_strLinkKey.IsEmpty())
		pNew->SetItemName(dlg.m_strLinkKey);    // otherwise same as description

	return pNew;
}


/////////////////////////////////////////////////////////////////////////////
