; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CChangeDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "minsvr.h"

ClassCount=2
Class1=CMainWnd
Class2=CChangeDlg

ResourceCount=3
Resource1="MAINMENU"
Resource2=IDD_ABOUTBOX
Resource3=IDD_CHANGEDLG

[CLS:CMainWnd]
Type=0
HeaderFile=minsvr.h
ImplementationFile=mainwnd.cpp

[CLS:CChangeDlg]
Type=0
HeaderFile=minitem.cpp
ImplementationFile=minitem.cpp
LastObject=IDC_EDIT1

[MNU:"MAINMENU"]
Type=1
Command1=IDM_UPDATE
Command2=IDM_EXIT
Command3=IDM_CHANGESTRING
Command4=IDM_ABOUT
CommandCount=4
Class=?

[DLG:IDD_ABOUTBOX]
Type=1
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Class=?

[DLG:IDD_CHANGEDLG]
Type=1
ControlCount=4
Control1=IDC_EDIT1,edit,1350631552
Control2=IDC_STATIC,static,1342308352
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816
Class=CChangeDlg

