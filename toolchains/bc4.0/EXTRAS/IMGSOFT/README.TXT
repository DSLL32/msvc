Note: If this demo is contained within a .ZIP file, you MUST unzip the file
onto a diskette before installing.  You must use the -d option so that the
directories will be unzipped onto the diskette.

This document contains important information that is not included in the
Object/Designer manual or in online help.

Contents
--------

 1.0 Object/Designer Demo Installation
 2.0 Description of Object/Designer and the Object/Designer Demo
 3.0 Working with the Object/Designer Demo
 4.0 Working with the Object/Designer Samples
 5.0 Supported Compilers, Languages, and Class Libraries
 6.0 Configuring Object/Designer for your System and Compiler
 7.0 Using Custom Controls
 8.0 Creating Dialog Resource Files
 9.0 What's New in Object/Designer 2.1
10.0 Object/Designer Conference on ImageSoft's BBS

1.0 Object/Designer Demo Installation
--- ---------------------------------

Hardware Requirements 
   Object/Designer requires hardware capable of running Microsoft Windows 
   and a Windows development environment. Object/Designer requires 2.0 meg 
   of free disk space for installation. 
	
Software Requirements   
   Object/Designer requires the following software: 
 
   - Microsoft Windows 3.0 or above running in standard or enhanced mode. 
   - Supported Development Environment - See Section 5.0 for a list of 
     supported compilers
   - Resource Editor for editing dialogs, bitmaps, and icons 
 
Install Procedures 
   The Object/Designer install program is a Windows application that copies all 
   necessary Object/Designer files to your hard disk.  It will automatically 
   add Object/Designer to the Windows Program Manager.   
 
   - If you are not already in Windows, start Windows by typing win at the 
     DOS prompt. 
   - Insert the Object/Designer installation diskette into your floppy drive. 
   - Run setup.exe from the installation diskette 
 
   The default directory for installation is C:\OBJDES21.  You can overrride 
   the default path by entering a new path complete with drive letter.  If 
   the directory does not exist, it will be created. 

   - The setup program will then copy all necessary files to your target 
     directory. 
   - Setup will attempt to create a Object/Designer group within your 
     Program Manager. 


2.0 Description of Object/Designer and the Object/Designer Demo
--- -----------------------------------------------------------
 
Object/Designer is a powerful source code generator designed to allow you to 
quickly develop Microsoft Windows applications.  It allows you to 
interactively design and test the many components of the user interface 
including, dialogs, windows, menus, and tool bars.  After testing your 
application design, you can quickly generate a complete working application.  
Object/Designer supports MDI, custom controls, main window dialog use, 
multiple modules, and more!

Object/Designer is different from most of the other development tools on the 
market. Object/Designer is designed to be completely extensible.  This is 
accomplished in two ways.  Object/Designer uses a scripting language to 
allow the developer to control all aspects of code generation.  These scripts 
are referred to as source drivers within Object/Designer.  The second method 
allows the developer to create and modify attributes of all objects within 
the system. These attributes are then used in the source drivers to generate 
custom source code. 

These features make Object/Designer especially useful when working with 
object-oriented languages like C++. Part of the beauty of object-oriented 
programming (OOP)lies in its use of inheritance. In Windows this feature 
lets you derive your own objects based on the standard interface objects of 
Windows. With Object/Designer, you can easily generate the source code 
supporting these newly defined objects. Object/Designer was designed from 
the ground up to support OOP. Object/Designer was developed entirely in C++.

Object/Designer is also well suited for developing applications that 
interface with 3rd party libraries. If you've used other application 
generators, you probably generated your application framework and then 
spent days adding hooks to your database, graphics, or other 3rd party 
library. All of these tasks can be automated with Object/Designer.  This 
automation leads to higher productivity, better coding standards, and less 
errors.

Other application generators force you to adopt their standards. You have
to live with their naming conventions, source indentions, source file 
organization, comments, everything. With Object/Designer, you have complete 
control. If you don't like it, change it!


The Object/Designer demo is a slightly modified version of the retail 
version of Object/Designer. It is intended to let you evaluate the 
usefulness of Object/Designer before purchasing.  The only feature that is 
disabled in the demo is the actual source code generation. This is offset by 
complete examples of generated source code. A tutorial is included to 
demonstrate the use of Object/Designer.


3.0 Working with the Object/Designer Demo
--- -------------------------------------

A tutorial has been included to help you become familiar with 
Object/Designer. The document TUTOR.WRI is the Object/Designer tutorial. 
This file is in Microsoft Write format. We highly recommend that you work 
through the entire tutorial. The sample code included with this demo was 
generated through the tutorial. 


4.0 Working with the Object/Designer Samples
--- ----------------------------------------

The Object/Designer demo includes some very instructive sample programs. 
Included in these samples, is a stand-alone tool bar module that can be 
easily incorporated into your own applications. These samples are also very 
interesting because they illustrate how to write a Windows application in a
variety of different languages.

The samples are located in the following directories:

   C:\OBJDES\SAMPLES\C	- Standard C code for Tutor

   C:\OBJDES\SAMPLES\MFCCPP - Microsoft C/C++ 7.0/MFC code for Tutor

   C:\OBJDES\SAMPLES\OWLCPP - Borland C++/OWL 3.1 code for Tutor

   C:\OBJDES\SAMPLES\OWLPAS - Borland Pascal 7.0 code for Tutor

   C:\OBJDES\SAMPLES\CV3B - CommonView 3B code for Tutor

   C:\OBJDES\SAMPLES\ZAPP - zApp 1.1f code for Tutor

The sample programs represent complete examples of generated source code. To 
build the sample program for a given language, you MUST copy the sample code 
from the proper SAMPLES directory to the TUTOR directory. The resources 
necessary to build the program are located in the TUTOR directory. Once the 
files have been copied into the TUTOR directory, you can create the 
executable by invoking the make program from the DOS command line.
   
	Microsoft C             nmake -ftutor.ms
									 	
	Microsoft C++/MFC       nmake -ftutor.mak

	Borland C               make -ftutor.bor

	Borland C++/OWL 3.1     make -ftutor.mak

	Borland Pascal 7.0      make -ftutor.mak

	CommonView 3B           make -ftutor.mak

	zApp                    make -ftutor.mak


Several of the generated make files expect your compiler to be setup in the
default directories of your local drive. If your compiler or application
framework libraries are located in other directories, you will need to edit 
the make files.  

To use Turbo Pascal for Windows 1.5, you should replace all references to the
falling units: 
	Objects, OMemory, OWindows, ODialogs
to
	WObjects.

5.0 Supported Compilers, Languages, and Class Libraries
--- ---------------------------------------------------

Object/Designer currently supports the following compilers, languages, and 
class libraries for Windows development:
	
   Microsoft C/C++ 7.0 with Microsoft Foundation Classes
   Borland C++ 3.1 with Object Windows Library
   Borland Turbo Pascal for Windows 1.5 or Borland Pascal 7.0
   ANSI C for Borland and Microsoft compilers 
   Inmark's zApp for Windows 1.1f
   ImageSoft's CommonView 3B for Windows
   Microsoft C/C++ for Windows NT (October Release)

Object/Designer can easily be modified to generate any type of source code.  
If you are using a compiler or library that is not directly supported, you 
can modify the source drivers within Object/Designer to generate the 
appropriate source.  You should also check with us here at J Systems.  We 
will continue to extend the number of products directly supported.


6.0 Configuring Object/Designer for your System and Compiler
--- --------------------------------------------------------

There are a couple of options that must be set within Object/Designer to 
generate and compile applications successfully.  In the information that 
follows, it is assumed that you installed Object/Designer into the default 
directory, C:\OBJDES21.  If you installed into a different directory, adjust 
your input accordingly. It is also assumed that your compiler is installed 
correctly to create Windows applications. 


   1. Set the source driver directory to the appropriate directory.  You do
   this by selecting OPTIONS from the SETUP menu within Object/Designer.  
   Enter the source driver directory based on the language you intend to 
   generate. 

      Microsoft C++/MFC - C:\OBJDES21\DRIVERS\MFCCPP

      Borland C++/OWL - C:\OBJDES21\DRIVERS\OWLCPP

      Borland Pascal for Windows - C:\OBJDES21\DRIVERS\OWLPAS

      ANSI C - C:\OBJDES21\DRIVERS\C

      zApp 1.1f - C:\OBJDES21\DRIVERS\ZAPP

      CommonView 3B - C:\OBJDES21\DRIVERS\CV3B

   2. In the same dialog box you need to specify the correct command line
   to invoke the make program for your compiler. This command line typically
   calls a PIF file to invoke the make program. Object/Designer includes two 
   sample PIF files to invoke the Borland and Microsoft make programs. The 
   following describes the command line needed for each of these PIFs.

      Borland Make - C:\OBJDES21\MAKE.PIF -f%s.mak

      Microsoft Make - C:\OBJDES21\NMAKE.PIF -f%s.mak

   If you are using Object/Designer for Windows NT, there is a batch file
   that calls the compiler. This batch file is used so that the compiler 
   pauses before exiting. Use the following command line for Windows NT:

      NT Make - C:\OBJDES21\NMAKE.BAT %s.mak

   3. The generated make file assumes that your compiler is located in the
   default directory of your local drive. You may to need edit the MAKE 
   source driver to overide this path.

      a. Choose the SOURCE DRIVERS option from the SETUP menu of 
      Object/Designer. This will display all of the source drivers in your 
      selected source driver directory. The driver path is displayed at the 
      top of the dialog box.  This should match the path you specified in 
      step 1. If this is not the case, you need to check step 1.

      b. Double-click on MAK.SDV in the list box.  This will invoke the 
      default editor to edit the source driver. This source driver controls 
      the generation of the make file.

      c. You should make sure your correct compiler path is used. The 
      Microsoft compiler make files utilize the INCLUDE and LIB environmental
      variables. These must also be correct.


   4. If you are using a Microsoft compiler or Turbo Pascal 1.5, there is 
   one more step that must be done to have Object/Designer generate code
   for your compiler.  It is necessary to edit a source driver to indicate
   which compiler your are using. 

      a. Choose the SOURCE DRIVERS option from the SETUP menu of 
      Object/Designer. This will display all of the source drivers in your 
      selected source driver directory. The driver path is displayed at the 
      top of the dialog box.  Make sure your are editing source drivers in
      the correct directory.  If this is not the case, you need to check 
      step 1.

      b. Double-click on APP.SDV in the list box.  This will invoke the 
      default editor to edit the primary application driver. This source
      driver controls the flow of the generation process.

      c. The first line will contain a line similar to the following:
				
            $VAR VCompiler = "Borland"$

         For Microsoft C under Windows change the line to:

            $VAR VCompiler = "Microsoft"$

         For Microsoft C or C++ under Windows NT change the line to:

            $VAR VCompiler = "MS_NT"$

         For Turbo Pascal for Windows 1.5 change the line to:

            $VAR VCompiler = "1.5"$


   5. Most of the C and C++ drivers attempt to create applications in the 
   medium memory model. If your compiler is not setup for the medium model, 
   you can either modify your compiler, or modify the source driver MAK.SDV. 
   See number 3 above about modifying MAK.SDV.


Using Borland Project Files
---------------------------

Object/Designer defaults to using the command-line compiler. If you are using
Borland C++ 3.1 you can also use the Windows based BCW.EXE. BCW uses project 
files rather than make files. Object/Designer creates a project file based on 
a model. The model project files are located in \OBJDES21. You will probably 
need to edit the appropriate model project to work with your directory 
structure.

The project is created from within the main application source driver, 
APP.SDV.  This driver executes COPYPRJ.EXE. You may have to modify this
driver to use the appropriate path for this program. The COPYPRJ program adds
all source files in the working directory to the project.

You may want to add BCW to your tools menu. You could also use BCW for your
MAKE command line in SETUP...OPTIONS.  The command-line would look like this:

	BCW %s.prj
	or
	BCW -m %s.prj

The -m switch allows BCW to run in batch mode.



7.0 Using Custom Controls
--- ---------------------

Custom controls are fully supported in Object/Designer. Refer to Working 
with Custom Controls in Object/Designer's online help.

Some custom controls work in conjunction with a specific dialog class. This
special dialog class handles the painting of the dialog background. These 
dialog classes are not used within Object/Designer. The compiled application 
should display the dialog using the special dialog class. 

You can overide the way dialogs are displayed within Object/Designer. 
Object/Designer defaults to displaying the dialog with the standard window 
color. You can configure Object/Designer to use either a flat gray 
background or a gray textured background. These options have no effect on 
code generation.

          Example of flat gray background for dialogs from objdes.ini
          [Setup]
          GrayDialogs=1

          Example of gray textured background for dialogs from objdes.ini
          [Setup]
          GrayPatternDialogs=1

Note: Object/Designer displays all dialogs with the class "BorDlgxxx" with 
a gray textured background.


8.0 Creating Dialog Resource Files
--- ------------------------------

The source drivers that are distributed with Object/Designer expect your 
resource files to be in a specific format. The following is an example from 
the tutorial:

  TUTOR.RC - Generated by Object/Designer. This is the resource script file.
             It contains menu definitions, and #includes tutor.h and 
             tutor.dlg.

  TUTOR.H - Generated by Object/Designer. It contains identifiers for all 
            controls, menu items, tool items etc.

  TUTOR.DLG - Designed with standard dialog editor. It contains all dialog
              definitions for this application. It does not include tutor.h.


Using Borland's Resource Workshop
---------------------------------

Here are the steps necessary to create a new resource file similar to the
tutorial.

1. Choose [File] [New project...] to create a new .RC file
2. Choose [File] [Add to project...] to add an .H identifier file. Specify
   TUTOR.H for the filename.
3. Choose [File] [Add to project...] to add a .DLG dialog script file. 
   Specify TUTOR.DLG for the filename.
4. Choose [Resource] [New...] to create a new dialog definition. In the 
   popup dialog specify the resource type is DIALOG. You MUST specify that
   you want this dialog placed in TUTOR.DLG.  All future dialogs that you 
   add will automatically be placed here.
5. When you create controls for your dialogs, make sure you use constant 
   identifiers.  They will default to being placed in TUTOR.H. Most control
   identifiers are entered in uppercase with an "ID_" as the prefix.
6. Choose [File] [Save project...] to save everything. Specify TUTOR.RC as
   the file name.

Note: Both the .RC and the .H files will be generated over by 
      Object/Designer. The .DLG is the only file used as input for 
      Object/Designer. 
      

Using Microsoft's SDK Dialog Editor
-----------------------------------

Dialog Editor opens and saves .RES files. The .RES file is the compiled 
version of a resource file. Dialog Editor also saves the other 
components in script form. It creates both a .DLG file and a .H file.

Here are the steps necessary to create a new resource file similar to the
tutorial.

1. Choose [File] [New] to create a new file.
2. Dialog editor creates a default dialog for you to edit.  You should 
   specify an Identifier constant for the name. In the Dlg. Sym.: edit box
   enter the dialog name.
3. When you create controls for your dialogs, make sure you use constant 
   identifiers.  For each control specify a unique constant in the Symbol
   edit box. Most control identifiers are entered in uppercase with an 
   "ID_" as the prefix.
4. Choose [Edit] [New dialog] to add additional dialogs to this file.
5. Choose [File] [Save] to save everything. Specify TUTOR.H as the include 
   file name. Specify TUTOR.RES as the resource file name. The editor will
   also save TUTOR.DLG.

Note: The .H file will be generated over by Object/Designer. The .DLG is the 
      only file used as input for Object/Designer. 



9.0 What's New in Object/Designer 2.1
--- ---------------------------------

1. New Source Driver Statement

  $EXEC CmdLine$ - Executes the cmdline from within the script. Useful for
  running other Windows and DOS programs directly from source drivers.


2. New Source Driver Functions

  $EXISTS(FileName)$ - Returns "Y" if file exists, else "N"

  $LENSTR(String)$	- Returns length of string

  $FINDSTR(String1, String2, Position)$	- Searches for string1 within 
  string2. Returns the position of the matching string. Position paramater 
  allows you to key a starting position for the search of string1. It is 
  optional. Returns -1 if the string is not found.

  $SUBSTR(String, StartPos, Count)$ - Returns a string consisting of Count 
  chars from String located beginning at the StartPos. The Count parameter 
  is optional. It defaults to being the length of String.


3. Object/Designer allows you to easily add your own functions to the source
driver language.  See \OBJDES21\ODUSER\README.TXT for information.


4. Object/Designer allows you to write your own interface for editing 
attributes. See \OBJDES21\ODUSER\README.TXT for information.


5. Support for Windows NT and Borland Pascal 7.0


6. Ability to create Borland project files from within source drivers. 


7. Bug Fixes from Object/Designer 2.0

  SETSEPARATOR didn't work for user arrays. 

  The TRIM() function failed to remove international characters.

  Count(Dialog.Controls) always returned -1.

  Stack Overflow when using Object/Designer with applications that subclassed
  the main window. These applications were often utilities that attached 
  themselves to the window caption or system menu.

  Unable to add menu items with the same name within a menu. A new menuitem
  attribute has been added to correct this problem. The attribute is 
  POPUPNAME. It is used to concatenate the popup menu name with the menuitem 
  name to make the name unique. This results in menu IDs like IDM_FILEOPEN 
  instead of IDM_OPEN.


10.0 Object/Designer Conference on ImageSoft's BBS
---- ---------------------------------------------

A conference has been setup on the ImageSoft BBS.  This	conference will
make it very easy to issue updates to the source drivers. We also hope 
this will provide an opportunity for sharing source drivers among the 
Object/Designer users. Please upload any source drivers you think might	be 
useful to other users.

The ImageSoft BBS numbers are (516) 767-5189 and (517) 767-9074. The first
number has a high speed modem.

