// padview.cpp : implementation of the CPadView class
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
#include "padview.h"
#include "tabstop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPadView

IMPLEMENT_DYNCREATE(CPadView, CEditView)

BEGIN_MESSAGE_MAP(CPadView, CEditView)
	//{{AFX_MSG_MAP(CPadView)
	ON_WM_CREATE()
	ON_COMMAND(ID_SET_TABSTOPS, OnSetTabStops)
	ON_COMMAND(ID_CHOOSE_FONT, OnChooseFont)
	ON_COMMAND(ID_WORD_WRAP, OnWordWrap)
	ON_UPDATE_COMMAND_UI(ID_WORD_WRAP, OnUpdateWordWrap)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_CHOOSE_PRINT_FONT, OnChoosePrintFont)
	ON_COMMAND(ID_MIRROR_DISPLAY_FONT, OnMirrorDisplayFont)
	ON_UPDATE_COMMAND_UI(ID_MIRROR_DISPLAY_FONT, OnUpdateMirrorDisplayFont)
	ON_UPDATE_COMMAND_UI(ID_CHOOSE_PRINT_FONT, OnUpdateChoosePrintFont)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

UINT CPadView::m_nDefTabStops;
UINT CPadView::m_nDefTabStopsOld;
BOOL CPadView::m_bDefWordWrap;
BOOL CPadView::m_bDefWordWrapOld;
LOGFONT CPadView::m_lfDefFont;
LOGFONT CPadView::m_lfDefFontOld;
LOGFONT CPadView::m_lfDefPrintFont;
LOGFONT CPadView::m_lfDefPrintFontOld;

/////////////////////////////////////////////////////////////////////////////
// Static initialization/termination

static char BASED_CODE szSettings[] = "Settings";
static char BASED_CODE szTabStops[] = "TabStops";
static char BASED_CODE szFont[] = "Font";
static char BASED_CODE szPrintFont[] = "PrintFont";
static char BASED_CODE szHeight[] = "Height";
static char BASED_CODE szWeight[] = "Weight";
static char BASED_CODE szItalic[] = "Italic";
static char BASED_CODE szUnderline[] = "Underline";
static char BASED_CODE szPitchAndFamily[] = "PitchAndFamily";
static char BASED_CODE szFaceName[] = "FaceName";
static char BASED_CODE szSystem[] = "System";
static char BASED_CODE szWordWrap[] = "WordWrap";

static void GetProfileFont(LPCSTR szSec, LOGFONT* plf)
{
	CWinApp* pApp = AfxGetApp();
	plf->lfHeight = pApp->GetProfileInt(szSec, szHeight, 0);
	if (plf->lfHeight != 0)
	{
		plf->lfWeight = pApp->GetProfileInt(szSec, szWeight, 0);
		plf->lfItalic = (BYTE)pApp->GetProfileInt(szSec, szItalic, 0);
		plf->lfUnderline = (BYTE)pApp->GetProfileInt(szSec, szUnderline, 0);
		plf->lfPitchAndFamily = (BYTE)pApp->GetProfileInt(szSec, szPitchAndFamily, 0);
		CString strFont = pApp->GetProfileString(szSec, szFaceName, szSystem);
		strncpy((char*)plf->lfFaceName, strFont, sizeof plf->lfFaceName);
		plf->lfFaceName[sizeof plf->lfFaceName-1] = 0;
	}
}

static void WriteProfileFont(LPCSTR szSec, const LOGFONT* plf, LOGFONT* plfOld)
{
	CWinApp* pApp = AfxGetApp();

	if (plf->lfHeight != plfOld->lfHeight)
		pApp->WriteProfileInt(szSec, szHeight, plf->lfHeight);
	if (plf->lfHeight != 0)
	{
		if (plf->lfHeight != plfOld->lfHeight)
			pApp->WriteProfileInt(szSec, szHeight, plf->lfHeight);
		if (plf->lfWeight != plfOld->lfWeight)
			pApp->WriteProfileInt(szSec, szWeight, plf->lfWeight);
		if (plf->lfItalic != plfOld->lfItalic)
			pApp->WriteProfileInt(szSec, szItalic, plf->lfItalic);
		if (plf->lfUnderline != plfOld->lfUnderline)
			pApp->WriteProfileInt(szSec, szUnderline, plf->lfUnderline);
		if (plf->lfPitchAndFamily != plfOld->lfPitchAndFamily)
			pApp->WriteProfileInt(szSec, szPitchAndFamily, plf->lfPitchAndFamily);
		if (strcmp(plf->lfFaceName, plfOld->lfFaceName) != 0)
			pApp->WriteProfileString(szSec, szFaceName, (LPCSTR)plf->lfFaceName);
	}
	*plfOld = *plf;
}

void CPadView::Initialize()
{
	CWinApp* pApp = AfxGetApp();
	m_bDefWordWrap = pApp->GetProfileInt(szSettings, szWordWrap, 0);
	m_bDefWordWrapOld = m_bDefWordWrap;
	m_nDefTabStops = pApp->GetProfileInt(szSettings, szTabStops, 8*4);
	m_nDefTabStopsOld = m_nDefTabStops;
	GetProfileFont(szFont, &m_lfDefFont);
	m_lfDefFontOld = m_lfDefFont;
	GetProfileFont(szPrintFont, &m_lfDefPrintFont);
	m_lfDefPrintFontOld = m_lfDefPrintFont;
}

void CPadView::Terminate()
{
	CWinApp* pApp = AfxGetApp();
	if (m_nDefTabStops != m_nDefTabStopsOld)
		pApp->WriteProfileInt(szSettings, szTabStops, m_nDefTabStops);
	if (m_bDefWordWrap != m_bDefWordWrapOld)
		pApp->WriteProfileInt(szSettings, szWordWrap, m_bDefWordWrap);
	WriteProfileFont(szFont, &m_lfDefFont, &m_lfDefFontOld);
	WriteProfileFont(szPrintFont, &m_lfDefPrintFont, &m_lfDefPrintFontOld);
}

/////////////////////////////////////////////////////////////////////////////
// CPadView construction/destruction

CPadView::CPadView()
{
	m_nTabStops = m_nDefTabStops;
	m_bRecreating = FALSE;
}

BOOL CPadView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CEditView::PreCreateWindow(cs))
		return FALSE;

	if (m_bDefWordWrap)
		cs.style &= ~(WS_HSCROLL|ES_AUTOHSCROLL);

	return TRUE;
}

int CPadView::OnCreate(LPCREATESTRUCT lpcs)
{
	if (CEditView::OnCreate(lpcs) != 0)
		return -1;
	if (m_lfDefFont.lfHeight != 0)
	{
		m_font.CreateFontIndirect(&m_lfDefFont);
		SetFont(&m_font);
	}
	if (m_lfDefPrintFont.lfHeight != 0)
	{
		m_fontPrint.CreateFontIndirect(&m_lfDefPrintFont);
		SetPrinterFont(&m_fontPrint);
	}
	return 0;
}

void CPadView::PostNcDestroy()
{
	if (m_bRecreating)
	{
		m_bRecreating = FALSE;
		return;
	}
	CEditView::PostNcDestroy();
}

/////////////////////////////////////////////////////////////////////////////
// CPadView Word Wrap support

BOOL CPadView::IsWordWrap() const
{
	return (GetStyle() & ES_AUTOHSCROLL) == 0;
}

BOOL CPadView::SetWordWrap(BOOL bWordWrap)
{
	bWordWrap = !!bWordWrap;    // make sure ==TRUE || ==FALSE
	if (IsWordWrap() == bWordWrap)
		return FALSE;

	// preserve original control's state.
	CFont* pFont = GetFont();
	int nLen = GetBufferLength();
	char FAR* pSaveText = new far char[GetBufferLength()+1];
	GetWindowText(pSaveText, nLen+1);

	// create new edit control with appropriate style and size.
	DWORD dwStyle = dwStyleDefault & ~(ES_AUTOHSCROLL|WS_HSCROLL|WS_VISIBLE);
	if (!bWordWrap)
		dwStyle |= ES_AUTOHSCROLL|WS_HSCROLL;

	CWnd* pParent = GetParent();
	CRect rect;
	GetWindowRect(rect);
	pParent->ScreenToClient(rect);
	CWnd* pFocus = GetFocus();

	UINT nID = GetDlgCtrlID();
	CFrameWnd* pFrame = GetParentFrame();
	ASSERT(pFrame != NULL);
	CView* pActiveView = pFrame->GetActiveView();

	HWND hWnd = ::CreateWindow("edit", NULL, dwStyle,
		rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
		pParent->m_hWnd, (HMENU)nID,
		(HINSTANCE)m_segText, NULL);

	if (hWnd == NULL)
	{
		delete[] pSaveText;
		return FALSE;
	}

	// set the window text to nothing to make sure following set doesn't fail
	SetWindowText(NULL);

	// restore visual state
	::SetWindowText(hWnd, pSaveText);
	delete[] pSaveText;
	if (pFont != NULL)
	{
		ASSERT(pFont->m_hObject != NULL);
		::SendMessage(hWnd, WM_SETFONT, (WPARAM)pFont->m_hObject, 0);
	}
	UINT nTabStops = m_nTabStops;
	::SendMessage(hWnd, EM_SETTABSTOPS, 1, (LPARAM)(LPINT)&nTabStops);
	::GetClientRect(hWnd, &rect);
	::SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
		SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_SHOWWINDOW);
	::SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
		SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_DRAWFRAME);
	::UpdateWindow(hWnd);

	// destroy old and attach new.
	ASSERT(m_hWnd != NULL);
	ASSERT(!m_bRecreating);
	SetWindowPos(NULL, 0, 0, 0, 0,
		SWP_HIDEWINDOW|SWP_NOREDRAW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|
		SWP_NOZORDER);
	m_bRecreating = TRUE;
	DestroyWindow();
	ASSERT(!m_bRecreating); // should be reset in PostNcDestroy()
	ASSERT(m_hWnd == NULL);
	SubclassWindow(hWnd);
	ASSERT(m_hWnd == hWnd);

	// restore rest of state...
	GetEditCtrl().LimitText(nMaxSize);
	if (pFocus == this)
		SetFocus();
	if (pActiveView == this)
		pFrame->SetActiveView(this);

	ASSERT_VALID(this);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPadView Printing support

void CPadView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	CEditView::OnBeginPrinting(pDC, pInfo);

	const char* pszFileName = GetDocument()->GetPathName();
	BOOL bForceSysTime = strchr(pszFileName, '.') == NULL;
	CTime timeSys = CTime::GetCurrentTime();
	CFileStatus status;
	CFile::GetStatus(pszFileName, status);

	if (dlgPageSetup.m_iHeaderTime != 0 || bForceSysTime)
		m_timeHeader = timeSys;
	else
		m_timeHeader = status.m_mtime;

	if (dlgPageSetup.m_iFooterTime != 0 || bForceSysTime)
		m_timeFooter = timeSys;
	else
		m_timeFooter = status.m_mtime;

	if (!pInfo->m_bPreview)
		return;

	CWaitCursor wait;
	pInfo->m_nCurPage = 0xFFFF;
	OnPrepareDC(pDC, pInfo);

	UINT nIndex = LOWORD(GetEditCtrl().GetSel());
	UINT nCurPage = 1;
	while (nCurPage < (UINT)m_aPageStart.GetSize())
	{
		if (nIndex < m_aPageStart[nCurPage])
			break;
		nCurPage++;
	}
	pInfo->m_nCurPage = nCurPage;
	pInfo->SetMaxPage(m_aPageStart.GetSize());
	m_nPreviewPage = nCurPage;
}

void
CPadView::OnPrint(CDC *pDC, CPrintInfo* pInfo)
{
	// get string to show as "filename" in header/footer
	const char* pszFileName = GetDocument()->GetPathName();
	if (pszFileName[0] == 0)
		pszFileName = GetDocument()->GetTitle();

	// go thru global CPageSetupDlg to format the header and footer
	CString strHeader;
	dlgPageSetup.FormatHeader(strHeader, m_timeHeader, pszFileName,
		pInfo->m_nCurPage);
	CString strFooter;
	dlgPageSetup.FormatFooter(strFooter, m_timeFooter, pszFileName,
		pInfo->m_nCurPage);

	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	int cyChar = tm.tmHeight;
	CRect rectPage = pInfo->m_rectDraw;

	// draw and exclude space for header
	if (!strHeader.IsEmpty())
	{
		pDC->TextOut(rectPage.left, rectPage.top, strHeader);
		rectPage.top += cyChar + cyChar / 4;
		pDC->MoveTo(rectPage.left, rectPage.top);
		pDC->LineTo(rectPage.right, rectPage.top);
		rectPage.top += cyChar / 4;
	}

	// allow space for footer
	pInfo->m_rectDraw = rectPage;
	if (!strFooter.IsEmpty())
		pInfo->m_rectDraw.bottom -= cyChar + cyChar/4 + cyChar/4;

	// draw body text
	CEditView::OnPrint(pDC, pInfo);

	// draw footer
	if (!strFooter.IsEmpty())
	{
		rectPage.bottom -= cyChar;
		pDC->TextOut(rectPage.left, rectPage.bottom, strFooter);
		rectPage.bottom -= cyChar / 4;
		pDC->MoveTo(rectPage.left, rectPage.bottom);
		pDC->LineTo(rectPage.right, rectPage.bottom);
		rectPage.bottom -= cyChar / 4;
	}
}

void
CPadView::OnScrollTo(CDC*, CPrintInfo* pInfo, POINT)
{
	UINT nPage = pInfo->m_nCurPage;
	ASSERT(nPage < (UINT)m_aPageStart.GetSize());
	if (nPage != m_nPreviewPage)
	{
		UINT nIndex = m_aPageStart[nPage];
		GetEditCtrl().SetSel((int)nIndex, (int)nIndex);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPadView Font Handling

void CPadView::OnChooseFont()
{
   // get current font description
   CFont* pFont = GetFont();
   LOGFONT lf;
   if (pFont != NULL)
	   pFont->GetObject(sizeof(LOGFONT), &lf);
   else
	   ::GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);

	CFontDialog dlg(&lf, CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT);
	if (dlg.DoModal() == IDOK)
	{
		// switch to new font.
		m_font.DeleteObject();
		if (m_font.CreateFontIndirect(&lf))
		{
			CWaitCursor wait;
			SetFont(&m_font);
			m_lfDefFont = lf;
		}
	}
}

static void ScaleLogFont(LPLOGFONT plf, const CDC& dcFrom, const CDC& dcTo)
	// helper to scale log font member from one DC to another!
{
	plf->lfHeight = MulDiv(plf->lfHeight,
		dcTo.GetDeviceCaps(LOGPIXELSY), dcFrom.GetDeviceCaps(LOGPIXELSY));
	plf->lfWidth = MulDiv(plf->lfWidth,
		dcTo.GetDeviceCaps(LOGPIXELSX), dcFrom.GetDeviceCaps(LOGPIXELSX));
}

void CPadView::OnChoosePrintFont()
{
	CWaitCursor wait;
	CFont* pFont = GetPrinterFont();
	LOGFONT lf;
	LPLOGFONT plf = NULL;
	if (pFont != NULL)
	{
		pFont->GetObject(sizeof(LOGFONT), &lf);
		plf = &lf;
	}

	// magic to get printer dialog that would be used if we were printing!
	CPrintDialog dlgPrint(FALSE);
	if (!AfxGetApp()->GetPrinterDeviceDefaults(&dlgPrint.m_pd))
	{
		AfxMessageBox(IDP_ERR_GET_DEVICE_DEFAULTS);
		return;
	}
	wait.Restore();
	HDC hdcPrint = dlgPrint.CreatePrinterDC();
	if (hdcPrint == NULL)
	{
		AfxMessageBox(IDP_ERR_GET_PRINTER_DC);
		return;
	}

	CDC dcScreen;
	dcScreen.Attach(::GetDC(NULL));
	CDC dcPrint;
	dcPrint.Attach(hdcPrint);

	if (plf != NULL)
	{
		// need to map initial logfont to screen metrics.
		::ScaleLogFont(plf, dcPrint, dcScreen);
	}

	// now bring up the dialog since we know the printer DC
	CFontDialog dlg(plf, CF_PRINTERFONTS, &dcPrint);
	if (dlg.DoModal() == IDOK)
	{
		// map the resulting logfont back to printer metrics.
		lf = dlg.m_lf;
		::ScaleLogFont(&lf, dcScreen, dcPrint);

		m_fontPrint.DeleteObject();
		if (m_fontPrint.CreateFontIndirect(&lf))
		{
			SetPrinterFont(&m_fontPrint);
			m_lfDefPrintFont = lf;
		}
	}
	//NOTE: destructor will call dcPrint.DeleteDC

	::ReleaseDC(NULL, dcScreen.Detach());
}

void CPadView::OnMirrorDisplayFont()
{
	SetPrinterFont(NULL);
	m_lfDefPrintFont.lfHeight = 0;
}

void CPadView::OnUpdateChoosePrintFont(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(GetPrinterFont() != NULL);
}

void CPadView::OnUpdateMirrorDisplayFont(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(GetPrinterFont() == NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CPadView Tab Stops

void CPadView::OnSetTabStops()
{
	CSetTabStops dlg;
	dlg.m_nTabStops = m_nTabStops/4;
	if (dlg.DoModal() == IDOK)
	{
		CWaitCursor wait;
		SetTabStops(dlg.m_nTabStops*4);
		m_nDefTabStops = m_nTabStops;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPadView Word Wrap

void CPadView::OnUpdateWordWrap(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(IsWordWrap());
}

void CPadView::OnWordWrap()
{
	CWaitCursor wait;
	SetWordWrap(!IsWordWrap());
	m_bDefWordWrap = IsWordWrap();
}

/////////////////////////////////////////////////////////////////////////////
// CPadView commands

void CPadView::OnRButtonDown(UINT, CPoint)
{
	GetParentFrame()->BringWindowToTop();
}

void CPadView::OnSize(UINT nType, int cx, int cy)
{
	CWaitCursor wait;
	CEditView::OnSize(nType, cx, cy);
}

/////////////////////////////////////////////////////////////////////////////
// CPadView diagnostics

#ifdef _DEBUG
void CPadView::AssertValid() const
{
	CEditView::AssertValid();
}

void CPadView::Dump(CDumpContext& dc) const
{
	CEditView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
