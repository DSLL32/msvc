; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=COleClientApp
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "oclient.h"

ClassCount=4
Class1=CMainFrame
Class2=CMainDoc
Class3=CMainView
Class4=COleClientApp

ResourceCount=4
Resource1=ID_OBJECT_POPUP_MENU
Resource2=IDR_OLECLITYPE
Resource3=IDR_MAINFRAME
Resource4=IDD_ABOUTBOX

[CLS:CMainFrame]
Type=0
HeaderFile=frame.h
ImplementationFile=frame.cpp

[CLS:CMainDoc]
Type=0
HeaderFile=maindoc.h
ImplementationFile=maindoc.cpp

[CLS:CMainView]
Type=0
HeaderFile=mainview.h
ImplementationFile=mainview.cpp

[CLS:COleClientApp]
Type=0
HeaderFile=oclient.h
ImplementationFile=oclient.cpp
LastObject=ID_EDIT_CLEAR
Filter=Not a Window

[MNU:IDR_MAINFRAME]
Type=1
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_PRINT_SETUP
Command4=ID_FILE_MRU_FILE1
Command5=ID_APP_EXIT
Command6=ID_APP_ABOUT
CommandCount=6
Class=?

[MNU:IDR_OLECLITYPE]
Type=1
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_CLOSE
Command4=ID_FILE_SAVE
Command5=ID_FILE_SAVE_AS
Command6=ID_FILE_PRINT
Command7=ID_FILE_PRINT_PREVIEW
Command8=ID_FILE_PRINT_SETUP
Command9=ID_FILE_MRU_FILE1
Command10=ID_APP_EXIT
Command11=ID_EDIT_CUT
Command12=ID_EDIT_COPY
Command13=ID_EDIT_PASTE
Command14=ID_EDIT_PASTE_LINK
Command15=ID_EDIT_CLEAR
Command16=ID_EDIT_CLEAR_ALL
Command17=ID_OLE_EDIT_LINKS
Command18=ID_OLE_INSERT_NEW
Command19=ID_OLE_VERB_FIRST
Command20=ID_OBJECT_TRACK_SIZE
Command21=ID_WINDOW_NEW
Command22=ID_WINDOW_CASCADE
Command23=ID_WINDOW_TILE_HORZ
Command24=ID_WINDOW_ARRANGE
Command25=ID_APP_ABOUT
CommandCount=25
Class=?

[MNU:ID_OBJECT_POPUP_MENU]
Type=1
Command1=ID_EDIT_CUT
Command2=ID_EDIT_COPY
Command3=ID_EDIT_CLEAR
Command4=ID_OBJECT_TRACK_SIZE
Command5=ID_OLE_VERB_FIRST
CommandCount=5
Class=?

[ACL:IDR_MAINFRAME]
Type=1
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_PRINT
Command5=ID_EDIT_CUT
Command6=ID_EDIT_COPY
Command7=ID_EDIT_PASTE
Command8=ID_EDIT_CUT
Command9=ID_EDIT_COPY
Command10=ID_EDIT_PASTE
Command11=ID_EDIT_CLEAR
CommandCount=11
Class=?

[DLG:IDD_ABOUTBOX]
Type=1
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Class=?

