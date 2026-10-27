//***************************************************************************
// tutor.hpp - Application Class Definitions
// 9/04/92 - 2:15:02pm
// John Doe
//***************************************************************************
//PROTECT_BEGIN Revisions 
//PROTECT_END 
#include <afxwin.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <mmsystem.h>		// Multimedia...needed for sndPlaySound()
#include <commdlg.h>		// Common Dialogs
#include "Tutor.h"				// Resource Constants
#include "jtools.hpp"				// Module Class Definitions
//PROTECT_BEGIN AppIncludes 
//PROTECT_END 

class TutorApp : public CWinApp
{
public:
	BOOL InitInstance();
	BOOL InitApplication();		 // Create window & dlg classes one time
//PROTECT_BEGIN AppClass 
//PROTECT_END 
};
class Main : public CFrameWnd 
{
	TToolBar *pToolbar;	// Pointer to tool bar
public:
	Main(CWnd* Parent);
	~Main();

	afx_msg void OnMainApp();
	afx_msg void OnMainDialog();
	afx_msg void OnMainWindow();
	afx_msg void OnMainSound();
	afx_msg void OnNew();
	afx_msg void OnOpen();
	afx_msg void OnSave();
	afx_msg void OnPrint();
	afx_msg void OnExit();
	afx_msg void OnAbout();
	afx_msg void OnDestroy() {delete this;}	// Allocated from heap, so delete
	DECLARE_MESSAGE_MAP()
};
class NewWin : public CFrameWnd 
{
public:
	NewWin(CWnd* Parent);
	~NewWin();

	afx_msg void OnDestroy() {delete this;}	// Allocated from heap, so delete
	DECLARE_MESSAGE_MAP()
};
class About : public CModalDialog 
{
	char szWorkStr[81], *p;		// Work Buffer
public:
	About(CWnd* pParentWnd);
	~About();

	BOOL CanClose();
	virtual BOOL OnInitDialog();
	afx_msg void OnOK();

	DECLARE_MESSAGE_MAP()
	//PROTECT_BEGIN dABOUTClass 
	//PROTECT_END 
};

class Address : public CDialog 
{
	char szWorkStr[81], *p;		// Work Buffer
public:
	Address(CWnd* pParentWnd);
	~Address();

	BOOL CanClose();
	virtual BOOL OnInitDialog();

	// Implement Modeless dialog functions similar to modal
	afx_msg void OnDestroy() {delete this;}	// Allocated from heap, so delete
	afx_msg void OnOK();
	afx_msg void OnCancel(){DestroyWindow();}

	afx_msg void OnBtnSearch();
	DECLARE_MESSAGE_MAP()
	//PROTECT_BEGIN dADDRESSClass 
	//PROTECT_END 
};

class Search : public CModalDialog 
{
	char szWorkStr[81], *p;		// Work Buffer
public:
	Search(CWnd* pParentWnd);
	~Search();

	BOOL CanClose();
	virtual BOOL OnInitDialog();
	afx_msg void OnOK();

	DECLARE_MESSAGE_MAP()
	//PROTECT_BEGIN dSEARCHClass 
	//PROTECT_END 
};


