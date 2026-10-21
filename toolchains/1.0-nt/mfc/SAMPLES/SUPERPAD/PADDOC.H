// paddoc.h : interface of the CPadDoc class
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



#ifndef __PADDOC_H__
#define __PADDOC_H__

class CPadDoc : public CDocument
{
protected:
	DECLARE_DYNCREATE(CPadDoc)
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	virtual void DeleteContents();
	//{{AFX_MSG(CPadDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif  // __PADDOC_H__

/////////////////////////////////////////////////////////////////////////////
