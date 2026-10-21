; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CServerView
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "hiersvr.h"

ClassCount=7
Class1=COLEServerApp
Class2=CAboutDlg
Class3=CMainFrame
Class4=CServerDoc
Class5=CChangeNameDlg
Class6=CAddNodeDlg
Class7=CServerView

ResourceCount=6
Resource1=IDR_POPUPS
Resource2=IDR_HIERSVRTYPE
Resource3=IDR_MAINFRAME
Resource4=IDD_ABOUTBOX
Resource5=IDD_ADD_NODE
Resource6=IDD_CHANGE_NAME

[CLS:COLEServerApp]
Type=0
HeaderFile=hiersvr.h
ImplementationFile=hiersvr.cpp

[CLS:CAboutDlg]
Type=0
HeaderFile=hiersvr.cpp
ImplementationFile=hiersvr.cpp

[CLS:CMainFrame]
Type=0
HeaderFile=mainfrm.h
ImplementationFile=mainfrm.cpp

[CLS:CServerDoc]
Type=0
HeaderFile=svrdoc.h
ImplementationFile=svrdoc.cpp

[CLS:CChangeNameDlg]
Type=0
HeaderFile=svritem.cpp
ImplementationFile=svritem.cpp

[CLS:CAddNodeDlg]
Type=0
HeaderFile=svritem.cpp
ImplementationFile=svritem.cpp

[CLS:CServerView]
Type=0
HeaderFile=svrview.h
ImplementationFile=svrview.cpp

[MNU:IDR_MAINFRAME]
Type=1
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_MRU_FILE1
Command4=ID_APP_EXIT
Command5=ID_VIEW_TOOLBAR
Command6=ID_VIEW_STATUS_BAR
Command7=ID_APP_ABOUT
CommandCount=7
Class=?

[MNU:IDR_HIERSVRTYPE]
Type=1
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_CLOSE
Command4=ID_FILE_SAVE
Command5=ID_FILE_SAVE_AS
Command6=ID_FILE_MRU_FILE1
Command7=ID_APP_EXIT
Command8=ID_EDIT_COPY
Command9=ID_CHANGE_NAME
Command10=ID_ADD_NODE
Command11=ID_IMPORT_TEXT
Command12=ID_VIEW_TOOLBAR
Command13=ID_VIEW_STATUS_BAR
Command14=ID_WINDOW_NEW
Command15=ID_WINDOW_CASCADE
Command16=ID_WINDOW_TILE_HORZ
Command17=ID_WINDOW_ARRANGE
Command18=ID_APP_ABOUT
CommandCount=18
Class=?

[MNU:IDR_POPUPS]
Type=1
Command1=ID_EDIT_COPY
Command2=ID_CHANGE_NAME
Command3=ID_ADD_NODE
CommandCount=3
Class=?

[ACL:IDR_MAINFRAME]
Type=1
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_EDIT_CUT
Command8=ID_EDIT_COPY
Command9=ID_EDIT_PASTE
CommandCount=9
Class=?

[DLG:IDD_ABOUTBOX]
Type=1
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDR_MAINFRAME,button,1342373889
Class=CAboutDlg

[DLG:IDD_ADD_NODE]
Type=1
ControlCount=8
Control1=IDC_STATIC,static,1342308354
Control2=IDC_EDIT1,edit,1350631552
Control3=IDC_STATIC,static,1342308354
Control4=IDC_EDIT2,edit,1350631552
Control5=IDC_STATIC,static,1342308354
Control6=IDC_COMBO1,combobox,1344339971
Control7=IDOK,button,1342242816
Control8=IDCANCEL,button,1342242816
Class=CAddNodeDlg

[DLG:IDD_CHANGE_NAME]
Type=1
ControlCount=8
Control1=IDC_STATIC,static,1342308354
Control2=IDC_EDIT1,edit,1350631552
Control3=IDC_STATIC,static,1342308354
Control4=IDC_EDIT2,edit,1350631552
Control5=IDC_STATIC,static,1342308354
Control6=IDC_COMBO1,combobox,1344339971
Control7=IDOK,button,1342242816
Control8=IDCANCEL,button,1342242816
Class=CChangeNameDlg

