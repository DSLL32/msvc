// superpad.cpp : Defines the class behaviors for the application.
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
#include "superpad.h"
#include "mainfrm.h"
#include "padview.h"
#include "paddoc.h"
#include "padframe.h"

BEGIN_MESSAGE_MAP(CSuperPadApp, CWinApp)
	//{{AFX_MSG_MAP(CSuperPadApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_PAGE_SETUP, OnPageSetup)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

CSuperPadApp SuperPadApp;

CSuperPadApp::CSuperPadApp()
{
}

BOOL CSuperPadApp::InitInstance()
{
	SetDialogBkColor();
	CPadFrame::Initialize();

	AddDocTemplate(
		new CMultiDocTemplate(IDR_TEXTTYPE,
		RUNTIME_CLASS(CPadDoc),
		RUNTIME_CLASS(CPadFrame),
		RUNTIME_CLASS(CPadView)));

	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	UINT nCmdShow = m_nCmdShow;
	m_nCmdShow = SW_HIDE;

	((CMainFrame*)m_pMainWnd)->InitialShowWindow(nCmdShow);
	m_pMainWnd->UpdateWindow();

	if (!m_pMainWnd->IsIconic() && m_lpCmdLine[0] == 0 &&
		m_splash.Create(m_pMainWnd))
	{
		m_splash.ShowWindow(SW_SHOW);
		m_splash.UpdateWindow();
	}

	m_dwSplashTime = ::GetCurrentTime();

	LoadStdProfileSettings();
	CPadView::Initialize();
	dlgPageSetup.Initialize();

	if (m_lpCmdLine[0] == '\0')
		OnFileNew();
	else
		OpenDocumentFile(m_lpCmdLine);

	m_nCmdShow = nCmdShow;
	m_pMainWnd->UpdateWindow();

	return TRUE;
}

BOOL CSuperPadApp::OnIdle(LONG lCount)
{
	// call base class idle first
	BOOL bResult = CWinApp::OnIdle(lCount);

	// then do our work
	if (m_splash.m_hWnd != NULL)
	{
		if (::GetCurrentTime() - m_dwSplashTime > 2500)
		{
			// timeout expired, destroy the splash window
			m_splash.DestroyWindow();
			m_pMainWnd->UpdateWindow();

			// NOTE: don't set bResult to FALSE,
			//  CWinApp::OnIdle may have returned TRUE
		}
		else
		{
			// check again later...
			bResult = TRUE;
		}
	}
	return bResult;
}

BOOL CSuperPadApp::PreTranslateMessage(MSG* pMsg)
{
	BOOL bResult = CWinApp::PreTranslateMessage(pMsg);

	if (m_splash.m_hWnd != NULL &&
		(pMsg->message == WM_KEYDOWN ||
		 pMsg->message == WM_SYSKEYDOWN ||
		 pMsg->message == WM_LBUTTONDOWN ||
		 pMsg->message == WM_RBUTTONDOWN ||
		 pMsg->message == WM_MBUTTONDOWN ||
		 pMsg->message == WM_NCLBUTTONDOWN ||
		 pMsg->message == WM_NCRBUTTONDOWN ||
		 pMsg->message == WM_NCMBUTTONDOWN))
	{
		m_splash.DestroyWindow();
		m_pMainWnd->UpdateWindow();
	}

	return bResult;
}

int CSuperPadApp::ExitInstance()
{
	dlgPageSetup.Terminate();
	CPadView::Terminate();
	CPadFrame::Terminate();

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CSuperPadApp message handlers

void CSuperPadApp::OnAppAbout()
{
	CAboutBox about;
	about.DoModal();
}

CPageSetupDlg dlgPageSetup;
void CSuperPadApp::OnPageSetup()
{
	dlgPageSetup.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
