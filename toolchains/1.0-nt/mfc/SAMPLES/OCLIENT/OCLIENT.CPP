// oclient.cpp : Defines the class behaviors for the application.
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

#include "frame.h"
#include "maindoc.h"
#include "mainview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COleClientApp

BEGIN_MESSAGE_MAP(COleClientApp, CWinApp)
	//{{AFX_MSG_MAP(COleClientApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COleClientApp construction
// Place all significant initialization in InitInstance

COleClientApp::COleClientApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only COleClientApp object

COleClientApp NEAR theApp;

/////////////////////////////////////////////////////////////////////////////
// COleClientApp initialization

BOOL COleClientApp::InitInstance()
{
	// Standard initialization

	SetDialogBkColor();        // set dialog background color
	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register document templates

	AddDocTemplate(new CMultiDocTemplate(IDR_OLECLITYPE,
			RUNTIME_CLASS(CMainDoc),
			RUNTIME_CLASS(CMDIChildWnd),        // standard MDI child frame
			RUNTIME_CLASS(CMainView)));

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();
	m_pMainWnd = pMainFrame;

	// enable file manager drag/drop and DDE Execute open
	m_pMainWnd->DragAcceptFiles();
	EnableShellOpen();
	RegisterShellFileTypes();

	// simple command line parsing
	if (m_lpCmdLine[0] == '\0')
	{
		// create a new (empty) document
		OnFileNew();
	}
	else if ((m_lpCmdLine[0] == '-' || m_lpCmdLine[0] == '/') &&
		(m_lpCmdLine[1] == 'e' || m_lpCmdLine[1] == 'E'))
	{
		// program launched embedded - wait for DDE or OLE open
	}
	else
	{
		// open an existing document
		OpenDocumentFile(m_lpCmdLine);
	}


	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// COleClientApp commands

void COleClientApp::OnAppAbout()
{
	CDialog(IDD_ABOUTBOX).DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// utility functions

LPSTR CreateNewUniqueName(LPSTR lpstr)
{
	static int CurrentNumber = 0;
	wsprintf(lpstr, "%s%04d", (LPSTR)OBJ_NAME_PREFIX, CurrentNumber++);
	return(lpstr);
}

/////////////////////////////////////////////////////////////////////////////
