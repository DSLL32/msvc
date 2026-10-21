=======================================================================
				   MICROSOFT (R) FOUNDATION CLASSES 
							Version 2.0
=======================================================================


This sub directory contains the source code and example code for
the Microsoft Foundation Classes 2.0 (MFC 2.0) C++ library.  
MFC 2.0 is a C++ Application Framework designed for 
use in developing applications for Microsoft Windows.  MFC 2.0
works in concert with the entire Microsoft Visual C++ development
system, including the Visual WorkBench, App Studio, AppWizard,
and ClassWizard.

What's New
----------
	o   Recompile compatibility with MFC 1.0 (see technical note 
		TN019 for migration tips).

	o   Architectural classes to support commands, documents,
		views, printing, and help.

	o   High level abstractions including ToolBars, status bars,
		edit views, form views, print previews, scrolling views,
		splitter windows.

	o   Increased "canned" functionality for all the common
		Windows user-interface commands, including file management,
		printing, and MDI.

	o   Improved support for Windows "wrapper" classes for basic
		window management, GDI, and standard dialogs.

	o   Enhanced general-purpose classes for time, date, data-
		structures, and file I/O.

	o   Integrated support for Object Linking and Embedding 1.0,
		including support for the standard OLE user-interface.

	o   AppWizard helps you to get a fast start in using the
		MFC 2.0 application framework by creating a starter 
		project with your application's key features.

	o   ClassWizard assists you in maintaining the mapping of
		user-interface elements to C++ code.  With its "Edit Code"
		button you can seamlessly jump from your user-interface
		elements to the code that manages them.

	o   App Studio is a full blown resource editor that is integrated
		with MFC 2.0.  App Studio automatically maintains resource
		IDs and lets you design and edit all the major Windows resource
		types.

	o   New sample applications including demonstrations of
		an MDI OLE Server, customized ToolBar and status bar management,
		custom controls, and startup splash screens.

	o   Better documentation of all MFC 2.0 APIs, including significantly
		more overview material.

	o   Entirely new tutorial which exploits many of the advanced
		features of MFC 2.0.

	o   All documentation, including technical notes, is now
		provided in on-line WinHelp format.  


Directory Structure
-------------------

MFC 2.0 is located in the MFC sub directory of the Microsoft Visual C++
installation by default.  If you installed Visual C++ on your
C drive then MFC 2.0 would be located in C:\MSVCNT\MFC by default.

The contents of the MFC sub directory are as follows:

	INCLUDE
		
		This directory contains included C++ header files, resource
		definitions, help contexts, and resource IDs.  These files
		are used by both the MFC 2.0 source code and by applications
		built using MFC 2.0.

	LIB

		This directory contains the static link libraries for 
		MFC 2.0.  If you decide to build custom variants of the
		MFC 2.0 application framework, your library will be placed in
		this directory as well.

	SAMPLES
		
		This directory contains all of the MFC 2.0 sample applications.
		There are over 20 sample programs that demonstrate most of the
		common MFC 2.0 features and show off many advanced idioms and
		techniques.

		The WinHelp file MFCSAMP.HLP is installed as a program
		item in the Microsoft Visual C++ Program Manager group which can
		be used to launch this file.  This file describes all of the
		sample applications and any information required to build
		them.

		The SCRIBBLE application is the tutorial.  All six major
		steps are included so there is no need to type in the code
		as you follow along with the tutorial text.

		The APSTUDIO directory contains a collection of cursors, icons,
		and bitmaps for you to use freely or modify.  This directory
		also contains the resources used by Windows for the standard
		dialogs (COMMDLG).

		A batch file is provided for building most of the samples from
		the command line.  Simply run the MKSAMPLE.BAT file from
		a command window.  Be sure that your INCLUDE and LIB environment
		variables are correctly set.  Three of the sample
		applications must be built by hand because they require
		variants of the MFC 2.0 library that are not in the installation.
		See the MFCSAMP.HLP file for more information on these samples.


	SRC

		This directory contains the source code to the MFC 2.0 application
		framework.  You should take care not to modify accidentally any
		of the files in this directory.  

		Should you require custom build options a standard makefile is 
		provided that can be used to build MFC 2.0.  Please see the file 
		SRC\README.TXT for instructions on building MFC 2.0 yourself.


	OTHER FILES

		The on-line documentation for MFC 2.0 is placed in the
		\MSVCNT\HELP sub directory with all of the Visual C++
		documentation.  The MFC 2.0 Technical Notes and
		sample program help files are also there.

		Three MFC 2.0 helper tools are placed in the \MSVCNT\BIN
		directory.  These are MAKEHM (for help support), TEMPLDEF
		(for limited template functionality), and TRACER (a Windows
		application for customizing MFC 2.0 diagnostic output).


NOTE
----

The Microsoft Foundation Classes (MFC) library source code is
shipped "as is" and is designed for your own use and modification.
Microsoft Product Support can only offer limited support regarding
the MFC source code because of this benefit. 

Microsoft grants you a non-exclusive royalty-free right to use and 
modify the source code contained in any Microsoft Foundation Classes 
source code file for purposes of creating a software product.  
However, you may not include this code in source form (or 
any modified version of it) within any software product.

Notwithstanding the Microsoft End User License Agreement, THIS SOURCE 
CODE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND.  YOU AGREE THAT 
NEITHER MICROSOFT NOR ANYONE ELSE INVOLVED IN CREATING, PRODUCING OR 
DELIVERING THE SOURCE CODE SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, 
CONSEQUENTIAL, OR INCIDENTAL DAMAGES RELATING TO THE SOURCE CODE.

