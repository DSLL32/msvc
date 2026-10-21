// hiersvr.cpp : Defines the class behaviors for the application.
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

#include "mainfrm.h"
#include "svrdoc.h"
#include "svrview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COLEServerApp

BEGIN_MESSAGE_MAP(COLEServerApp, CWinApp)
	//{{AFX_MSG_MAP(COLEServerApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COLEServerApp construction
// Place all significant initialization in InitInstance

COLEServerApp::COLEServerApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only COLEServerApp object

COLEServerApp NEAR theApp;

/////////////////////////////////////////////////////////////////////////////
// COLEServerApp initialization

BOOL COLEServerApp::InitInstance()
{
	// Standard initialization
	LoadStdProfileSettings(); // Load standard INI file options (including MRU)

	// Register document templates
	CDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_HIERSVRTYPE,
			RUNTIME_CLASS(CServerDoc),
			RUNTIME_CLASS(CMDIChildWnd),    // standard MDI child frame
			RUNTIME_CLASS(CServerView));
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();
	m_pMainWnd = pMainFrame;

	SetDialogBkColor();     // gray look

	// enable file manager drag/drop and DDE Execute open
	m_pMainWnd->DragAcceptFiles();
	EnableShellOpen();
	RegisterShellFileTypes();

	// try to launch as an OLE server
	if (m_server.RunEmbedded(pDocTemplate, FALSE /* MDI */, m_lpCmdLine))
	{
		// program launched embedded - RunEmbedded will open file if appropriate
	}
	else if (m_lpCmdLine[0] == '\0')
	{
		// create a new (empty) document
		OnFileNew();
	}
	else
	{
		// open an existing document
		OpenDocumentFile(m_lpCmdLine);
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg() : CDialog(CAboutDlg::IDD)
		{
			//{{AFX_DATA_INIT(CAboutDlg)
			//}}AFX_DATA_INIT
		}

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
		enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void COLEServerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// COLEServerApp commands
