// maindoc.cpp : implementation of the CMainDoc class
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
// CMainDoc

IMPLEMENT_DYNCREATE(CMainDoc, COleClientDoc)

BEGIN_MESSAGE_MAP(CMainDoc, COleClientDoc)
	//{{AFX_MSG_MAP(CMainDoc)
	ON_COMMAND(ID_EDIT_CLEAR_ALL, OnEditClearAll)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR_ALL, OnUpdateEditClearAll)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainDoc construction/destruction

CMainDoc::CMainDoc()
{
}

CMainDoc::~CMainDoc()
{
}

BOOL CMainDoc::OnNewDocument()
{
	if (!COleClientDoc::OnNewDocument())
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainDoc item management

CRectItem* CMainDoc::CreateItem()
{
	CRectItem* pItem = new CRectItem(this); // does 'AddItem' automatically
	ASSERT(!pItem->IsVisible());
	// item is invisible - no need to update views or dirty document
	return pItem;
}

// safe delete that notifies views
void CMainDoc::DeleteItem(CRectItem* pItem)
{
	if (pItem->IsVisible())
	{
		// only visible items really count
		SetModifiedFlag();
		pItem->Invalidate();
	}
	else
	{
		TRACE("Warning deleting an invisible RectItem\n");
	}
	ASSERT(pItem->GetDocument() == this);
	delete pItem;       // does a 'RemoveItem' automatically
}

/////////////////////////////////////////////////////////////////////////////
// CMainDoc serialization

void CMainDoc::Serialize(CArchive& ar)
{
	WORD wMagic = 0x0DAF;

	if (ar.IsStoring())
	{
		ar << wMagic;
		// First count up number of OLE Objects to write
		WORD cCompleteObjectsToWrite = 0;
		WORD cIncompleteObjects = 0;
		POSITION pos;

		pos = GetStartPosition();
		while (pos)
		{
			CRectItem* pItem = (CRectItem*)GetNextItem(pos);
			if (pItem->IsKindOf(RUNTIME_CLASS(CRectItem)))
			{
				if (pItem->IsVisible())
					cCompleteObjectsToWrite++;
				else
					cIncompleteObjects++;
			}
		}

		TRACE("%d complete objects to write, %d incomplete ones to ignore\n",
			cCompleteObjectsToWrite, cIncompleteObjects);

		if (cIncompleteObjects > 0 &&
			AfxMessageBox(IDP_SAVEINCOMPLETE,
			  MB_YESNO | MB_ICONQUESTION) != IDYES)
		{
			TRACE("Aborting save\n");
			AfxThrowArchiveException(CArchiveException::generic);
		}

		ar << cCompleteObjectsToWrite;

#ifdef _DEBUG
		WORD    cObjectsWritten = 0;
#endif
		// save items in correct order
		pos = GetStartPosition();
		while (pos)
		{
			CRectItem* pItem = (CRectItem*)GetNextItem(pos);
			if (pItem->IsKindOf(RUNTIME_CLASS(CRectItem)) &&
			   pItem->IsVisible())
			{
				pItem->Serialize(ar);
				// NOTE: don't use WriteObject since we know the actual
				//  class for pItem
#ifdef _DEBUG
				cObjectsWritten++;
#endif
			}
		}
#ifdef _DEBUG
		ASSERT(cObjectsWritten == cCompleteObjectsToWrite);
#endif
	}
	else // loading
	{
		WORD w;
		ar >> w;

		if (w != wMagic)
		{
			TRACE("invalid magic number at start of file\n");
			AfxThrowArchiveException(CArchiveException::generic);
		}

		WORD cObjectsToRead;
		ar >> cObjectsToRead;

		for (UINT iObj = 0; iObj < cObjectsToRead; iObj++)
		{
			CRectItem* pItem = CreateItem();
			pItem->Serialize(ar);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMainDoc diagnostics

#ifdef _DEBUG
void CMainDoc::AssertValid() const
{
	COleClientDoc::AssertValid();
}

void CMainDoc::Dump(CDumpContext& dc) const
{
	COleClientDoc::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainDoc commands


void CMainDoc::OnEditClearAll()
{
	DeleteContents();
	// everything is gone now !
	SetModifiedFlag();
	UpdateAllViews(NULL);
}


void CMainDoc::OnUpdateEditClearAll(CCmdUI* pCmdUI)
{
	// Enable ClearAll if there is anything to clear
	pCmdUI->Enable(GetStartPosition() != NULL);
}

