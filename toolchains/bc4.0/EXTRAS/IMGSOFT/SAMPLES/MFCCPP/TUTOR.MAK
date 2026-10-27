# 
# Tutor
# Microsoft Make File
DEBUG=1
MODEL = M
CPPFLAGS= /A$(MODEL) /W3 /Zp /GA /GEs /G2
LFLAGS=/NOD /ONERROR:NOEXE

!if "$(DEBUG)"=="1"
CPPFLAGS=/D_DEBUG $(CPPFLAGS) /Od /f /Zi
LFLAGS=$(LFLAGS) /COD
LIBS=$(MODEL)afxcwd libw $(MODEL)libcew mmsystem commdlg
!else
CPPFLAGS=$(CPPFLAGS) /Oselg /Gs
LFLAGS=$(LFLAGS)
LIBS=$(MODEL)afxcw libw $(MODEL)libcew mmsystem commdlg
!endif


PGM=Tutor
OBJS = $(PGM).obj	\
				jtools.obj \

.SUFFIXES: .rc

.rc.res:
    rc -r $*.rc

$(PGM).Exe: $(OBJS) $(PGM).def $(PGM).res
     link $(LFLAGS) @<<$(PGM).lnk
$(OBJS)
$(PGM), $(PGM), $(LIBS), $(PGM)
<<
     rc $(PGM).res

$(PGM).obj:			$*.cpp $(PGM).hpp
jtools.obj: $*.cpp $*.hpp	

$(PGM).res:   $*.rc $*.h

