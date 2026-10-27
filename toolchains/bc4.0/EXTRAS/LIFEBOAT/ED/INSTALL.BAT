@echo off
echo Log onto the floppy disk drive containing the ED Demonstration Programs
echo and Run the Install program.
rem If drive spec'd use it, otherwise default to drive A.
if ~ == ~%1 goto no_drive_specd
set DRIVE=%1
goto doit
:no_drive_specd
set DRIVE=a:
:doit
%DRIVE%
win install
set DRIVE=
