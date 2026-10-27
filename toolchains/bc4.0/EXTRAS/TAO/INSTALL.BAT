@echo off

if .%1 == .             goto EXP

md %1
md %1\gib30        
data -d -o %1
copy %1\gib*.* %1\gib30
copy %1\*.log %1\gib30
cd %1
goto END

:EXP

@echo.
@echo   Installation Messages:

@echo   Provide full target path name,  including drive.
@echo      usage:  install  [path]                
@echo      eg.     install  c:\gibdemo     
@echo   .
@echo   ** note **                              
@echo   Make sure the target path exist to ensure a successful installation.
goto Abort


:END
@echo.
@echo           ********************************************************     
@echo           *      GIB Demo Disk Installation Completed            *
@echo           *                                                      *
@echo           *  Press Ctrl + C to end this installation,            *
@echo           *                     or                               *
@echo           *  to start the self-running demonstration, by         *
@echo           *  hitting any key for continuing...                   *
@echo           *                                                      *
@echo           *  ps:                                                 *
@echo           *  You can enter - demo, to launch the demo program.   *
@echo           *                                                      *
@echo           *         TAO Research Corp. (510) 770-1344            *
@echo           ********************************************************
@echo.
@pause 

@echo           ********************************************************
@echo           *  Due to the large image files in the self-running    *
@echo           *  demonstration, 520K byte memory is recommended.     *
@echo           ********************************************************
@echo.
@echo           Press Ctrl+C, to quit the demonstration, 
@echo                            or
@echo           To continue the demonstration, enter the driver name,
@echo           where this demo is installed:
@echo           [such as C for C drive, D for D drive,...]
@echo.
@prompt C,D,..?
:abort
@echo on
