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
#include "minsvr.h"

/////////////////////////////////////////////////////////////////////////////
// OLE Server Document functionality

// Disable excessive compiler warning
#pragma warning(disable: 4355)  // this used in base initializer list

CMinDoc::CMinDoc()
	: m_item(this)
{
}

COleServerItem* CMinDoc::OnGetEmbeddedItem()
{
	ASSERT(m_docItemList.GetCount() == 1);
	return &m_item;
}

/////////////////////////////////////////////////////////////////////////////
