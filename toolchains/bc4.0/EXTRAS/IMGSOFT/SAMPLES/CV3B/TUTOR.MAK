#
# Tutor - Borland Make File
#
.autodepend
.swap

# CommonView Directory
CVDIR=\cv3b
LIBPATH=$(CVDIR)\lib;$(LIB)
INCLUDEPATH=$(CVDIR)\INCLUDE;$(INCLUDE)

CC=bcc
LINK=tlink
LFLAGS= /e/v/s/n/P-/Twe/L$(LIBPATH)
CFLAGS= -a- -c -WE -ml -Vf -P -v -DMSWIND=1
LIBS=libcvbw mathwl import cwl 

PGM=Tutor
OBJS = $(PGM).obj	\
				jtools.obj \

$(PGM).exe: turboc.cfg $(OBJS) $(PGM).res
     $(LINK) $(LFLAGS) @&&!
	  c0wl.obj $(OBJS)
	  $(PGM)
	  nul
	  $(LIBS)
	  $(PGM).def
!
     rc -K $(PGM).res $(PGM).exe

$(PGM).res: $(PGM).rc 
     rc -r -I$(INCLUDEPATH) $(PGM).rc

turboc.cfg: $(PGM).mak
	echo -I$(INCLUDEPATH) -L$(LIBPATH)>turboc.cfg

.cpp.obj :
    $(CC) $(CFLAGS) $<
.cxx.obj :
    $(CC) $(CFLAGS) $<

clean:
	del *.obj
	del *.exe
	del *.res
	del *.map
	del *.sym

