// svrdoc.cpp : implementation of the CServerDoc class
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

#include "svrdoc.h"
#include "svritem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerDoc

IMPLEMENT_SERIAL(CServerDoc, COleServerDoc, 0 /* schema number*/ )

BEGIN_MESSAGE_MAP(CServerDoc, COleServerDoc)
	//{{AFX_MSG_MAP(CServerDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerDoc construction/destruction

CServerDoc::CServerDoc()
{
	m_pRoot = NULL;
}

CServerDoc::~CServerDoc()
{
	delete m_pRoot;     // delete kills child nodes
}

void CServerDoc::DeleteContents()
{
	COleServerDoc::DeleteContents();
	delete m_pRoot;
	m_pRoot = NULL;  // no more items
}

BOOL CServerDoc::OnNewDocument()
{
	if (!COleServerDoc::OnNewDocument())
		return FALSE;
	ASSERT(m_pRoot == NULL);
	// create an empty root node
	m_pRoot = CServerItem::CreateRootNode(this);
	return (m_pRoot != NULL);
}

COleServerItem* CServerDoc::OnGetEmbeddedItem()
{
	return m_pRoot;
}

/////////////////////////////////////////////////////////////////////////////
// CServerDoc serialization

// this serializes the OLE Server document as a stand-alone file
void CServerDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ASSERT(m_pRoot != NULL);
		// Serialize below will do the work
	}
	else
	{
		ASSERT(m_pRoot == NULL);
		m_pRoot = CServerItem::CreateRootNode(this);
		if (m_pRoot == NULL)
			AfxThrowMemoryException();
	}

	m_pRoot->Serialize(ar);
}

/////////////////////////////////////////////////////////////////////////////
// CServerDoc diagnostics

#ifdef _DEBUG
void CServerDoc::AssertValid() const
{
	COleServerDoc::AssertValid();
}

void CServerDoc::Dump(CDumpContext& dc) const
{
	COleServerDoc::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CServerDoc commands

