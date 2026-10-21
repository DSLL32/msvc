# This is a part of the Microsoft Foundation Classes C++ library.
# Copyright (C) 1992,93 Microsoft Corporation
# All rights reserved.
#
# This source code is only intended as a supplement to the
# Microsoft Foundation Classes Reference and Microsoft
# QuickHelp and/or WinHelp documentation provided with the library.
# See these sources for detailed information regarding the
# Microsoft Foundation Classes product.

# Common include for building MFC Sample programs
#
#  typical usage
#       PROJ = foo
#       OBJS = foo.obj bar.obj ...
#       !INCLUDE ..\SAMPLE_.MAK
#
# NOTE: do not include 'stdafx.obj' in the OBJS list - the correctly
#    built version will be included for you
#
# Options to NMAKE:
#     "DEBUG=0" => use retail (default debug)
#     "OPT" => use to set custom compile options
#     "BROWSE=1" => build Visual C++ compatible browse file (.BSC file)
#     "NO_PCH=1" => do not use precompiled headers (defaults to use pch)
#     "PLATFORM=INTEL"     (defaults to INTEL)
#           This option chooses the appropriate tools and sources for the
#           different platforms support by Windows/NT.  Currently INTEL and
#           MIPS are supported; more will be aded as they become available.

!ifndef PROJ
	!ERROR You forgot to define 'PROJ' !!
!endif
!ifndef OBJS
	!ERROR You forgot to define 'OBJS' !!
!endif

!if "$(PLATFORM)"==""
PLATFORM=INTEL
!endif

STDAFX=stdafx

DLLENTRY = @12

!ifdef CONSOLE
MFCFLAGS=/D_CONSOLE
!else
MFCFLAGS=/D_WINDOWS
!endif

!ifdef CONSOLE
!if "$(DEBUG)" == "1"
MFCLIB = ..\..\lib\nafxcrd.lib
DEBUGFLAGS = /Z7 /Od /D_DEBUG
!else
MFCLIB = ..\..\lib\nafxcr.lib
DEBUGFLAGS = /Otx /Gs 
!endif
!else
!if "$(DEBUG)" == "1"
MFCLIB = ..\..\lib\nafxcwd.lib
DEBUGFLAGS = /Z7 /Od /D_DEBUG
!else
MFCLIB = ..\..\lib\nafxcw.lib
!if "$(PLATFORM)"=="INTEL"
DEBUGFLAGS = /O1
!else
!if "$(PLATFORM)"=="MIPS"
DEBUGFLAGS = /Osg /Gs
!endif
!endif
!endif
!endif

!if "$(PLATFORM)"=="INTEL"
CC = cl
CFLAGS  = /c /Gd /W3 /D_X86_ $(DEBUGFLAGS) $(MFCFLAGS)

CPP = cl
CPPMAIN_FLAGS = /c /Gd /W3 /D_X86_ $(DEBUGFLAGS) $(MFCFLAGS)
!else
!if "$(PLATFORM)"=="MIPS"
CC = mcl
CFLAGS  = /c /Gd /W3 /D_MIPS_ $(DEBUGFLAGS) $(MFCFLAGS)

CPP = mcl
CPPMAIN_FLAGS = /c /Gd /W3 /D_MIPS_ $(DEBUGFLAGS) $(MFCFLAGS)
!endif
!endif

!if "$(NO_PCH)"=="1"
CPPFLAGS=$(CPPMAIN_FLAGS)
!else
CPPFLAGS=$(CPPMAIN_FLAGS) /Yustdafx.h /Fp..\$(STDAFX).pch
!endif

!if "$(DEBUG)" == "1"
LINKDEBUG = -debug:full -debugtype:both
!else
LINKDEBUG = -debug:none
!endif
!if "$(PLATFORM)"=="INTEL"
LINK = link $(LINKDEBUG)
!else
!if "$(PLATFORM)"=="MIPS"
LINK = link32 $(LINKDEBUG)
!endif
!endif

# link flags - must be specified after $(link)
#
# conflags : creating a character based console application
# guiflags : creating a GUI based "Windows" application

CONFLAGS =  -subsystem:console -entry:mainCRTStartup
GUIFLAGS =  -subsystem:windows

# Link libraries - system import and C runtime libraries
#
# conlibs  : libraries to link with for a console application
# guilibs  : libraries to link with for a "Windows" application
# baselibs : libraries to link with for all application
#
# note : $(LIB) is set in environment variables

CONLIBS = netapi32.lib

GUILIBS = user32.lib gdi32.lib   \
	winspool.lib comdlg32.lib advapi32.lib \
	olecli32.lib olesvr32.lib shell32.lib

!if "$(PLATFORM)"=="INTEL"
BASELIBS = 
!else
!if "$(PLATFORM)"=="MIPS"
BASELIBS = libc.lib kernel32.lib
!endif
!endif

.SUFFIXES : .cpp
.cpp.obj :
	$(CPP) $(CPPFLAGS) $*.cpp

.c.obj :
	$(CC) $(CFLAGS) $(CVARS) $*.c

.rc.res :
!if "$(PLATFORM)"=="INTEL"
	rc -r $<
!else
!if "$(PLATFORM)"=="MIPS"
	rc -r -fo $*.rct $<
	cvtres -MIPS -o $*.res $*.rct
	-erase $*.rct
!endif
!endif

#############################################################################

!if "$(DEBUG)" == "1"
STDAFX=stdafxnd
!else
STDAFX=stdafxn
!endif

$(PROJ).exe: ..\$(STDAFX).obj $(PROJ).def $(PROJ).res $(OBJS)
	$(LINK) $(GUIFLAGS) -out:$(PROJ).exe -map:$(PROJ).map ..\$(STDAFX).obj \
			$(OBJS) $(PROJ).res $(MFCLIB) $(GUILIBS) $(EXTRA_LIBS) $(BASELIBS)

..\$(STDAFX).obj ..\$(STDAFX).pch: stdafx.h stdafx.cpp
	echo "BUILDING SHARED PCH and PCT files"
	$(CPP) $(CPPMAIN_FLAGS) /Ycstdafx.h /Fp..\$(STDAFX).pch /Fo..\$(STDAFX).obj /c ..\stdafx.cpp

$(OBJS): ..\$(STDAFX).pch
$(PROJ).res:  resource.h

clean::
	-erase $(PROJ).exe
	-erase $(PROJ).res
	-erase *.aps
	-erase *.pch
	-erase *.map
	-erase *.obj

#############################################################################
