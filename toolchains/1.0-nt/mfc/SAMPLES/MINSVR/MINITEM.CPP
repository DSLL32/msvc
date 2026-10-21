// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "afxext.h"     // for CMetaFileDC
#include "minsvr.h"

/////////////////////////////////////////////////////////////////////////////

CMinItem::CMinItem(CMinDoc* pContainerDoc)
	: COleServerItem(pContainerDoc)
{
	m_data = "example string";
}

void CMinItem::Serialize(CArchive& ar)
{
	// Customize this to store real data
	if (ar.IsStoring())
	{
		ar << m_data;
	}
	else
	{
		ar >> m_data;
	}
}


OLESTATUS CMinItem::OnShow(BOOL /* bTakeFocus */)
{
	// make sure server is the topmost window
	AfxGetApp()->m_pMainWnd->BringWindowToTop();
	return OLE_OK;
}


BOOL CMinItem::OnGetTextData(CString& rStringReturn)
{
	rStringReturn = m_data;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Simple dialog for changing the string

class CChangeDlg : public CDialog
{
// Construction
public:
	CChangeDlg::CChangeDlg();

// Dialog Data
	//{{AFX_DATA(CChangeDlg)
	enum { IDD = IDD_CHANGEDLG };
	CString m_str;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//{{AFX_MSG(CChangeDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CChangeDlg::CChangeDlg()
	: CDialog(CChangeDlg::IDD)
{
}

void CChangeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChangeDlg)
	DDX_Text(pDX, IDC_EDIT1, m_str);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CChangeDlg, CDialog)
	//{{AFX_MSG_MAP(CChangeDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CMinItem::PromptChangeString()
{
	CChangeDlg dlg;
	dlg.m_str = m_data;
	if (dlg.DoModal() != IDOK)
		return FALSE;
	m_data = dlg.m_str;
	return TRUE;        // changed
}

/////////////////////////////////////////////////////////////////////////////
// Drawing items into bitmap or metafile

BOOL CMinItem::OnDraw(CDC* pDC)
{
	ASSERT_VALID(pDC);
	CSize textSize;

	// first calculate the text size in MM_TEXT units
	{
		CWindowDC screenDC(NULL);
		textSize = screenDC.GetTextExtent(m_data, m_data.GetLength());
	}

	// if you want the item to always be drawn in a specific mapping
	// mode set it here.

	// Otherwise the OLE DLLs will scale the metafile to fit the
	// client specified size.  Setting the viewport size/extent
	// determines the relative scale of everything.

	int cx = textSize.cx + 100;
	int cy = (cx * 4) / 3;      // nice aspect ratio
	TRACE("Item drawing size is %d x %d\n", cx, cy);
	pDC->SetWindowExt(cx, cy);

	// to keep some OLE client apps like MS-Write happy, you should
	// set the bounding rect to the suggested size in MM_HIMETRIC units
	if (m_rectBounds.IsRectNull())
	{
		CClientDC dcTmp(NULL);
		dcTmp.SetMapMode(MM_HIMETRIC);
		m_rectBounds.SetRect(0, 0, cx, cy);
		dcTmp.DPtoLP(&m_rectBounds);        // rectBounds in HIMETRIC
	}

	// Draw a shaded circle
	pDC->SelectStockObject(LTGRAY_BRUSH);
	pDC->Ellipse(0, 0, cx, cy);

	// draw the text in the middle (as best we can)
	pDC->SetBkMode(TRANSPARENT);
	pDC->TextOut((cx - textSize.cx) / 2, (cy - textSize.cy) / 2, m_data);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
