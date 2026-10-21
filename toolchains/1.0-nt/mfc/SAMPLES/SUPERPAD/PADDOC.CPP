// paddoc.cpp : implementation of the CPadDoc class
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
#include "paddoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPadDoc

IMPLEMENT_DYNCREATE(CPadDoc, CDocument)

BEGIN_MESSAGE_MAP(CPadDoc, CDocument)
	//{{AFX_MSG_MAP(CPadDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPadDoc delegation to CEditView

void CPadDoc::DeleteContents()
{
	if (m_viewList.IsEmpty())
		return;
	CEditView* pView = (CEditView*)m_viewList.GetHead();
	ASSERT(pView->IsKindOf(RUNTIME_CLASS(CEditView)));
	pView->DeleteContents();
}

void CPadDoc::Serialize(CArchive& ar)
{
	CEditView* pView = (CEditView*)m_viewList.GetHead();
	ASSERT(pView->IsKindOf(RUNTIME_CLASS(CEditView)));
	pView->SerializeRaw(ar);
}

/////////////////////////////////////////////////////////////////////////////
