#
# Tutor - Borland Make File
#
.autodepend
.swap
WINMODE = -DWIN31 -DSTRICT
MODEL=l
BCROOT=\BORLANDC
OWLROOT=$(BCROOT)\owl
LIBDIR=$(BCROOT)\lib
INCLDIR=$(BCROOT)\include;$(BCROOT)\owl\include;$(BCROOT)\classlib\include

CC=bcc
LINK=tlink
LFLAGS= /Tw /n /x /c /C	/v
CFLAGS= -WE -c -m$(MODEL) -w -N -v
LIBS=cw$(MODEL) c$(MODEL) import
OWLLIBS=$(BCROOT)\owl\lib\owlw$(MODEL) $(BCROOT)\classlib\lib\tclass$(MODEL)

PGM=Tutor
OBJS = $(PGM).obj	\
				jtools.obj \

$(PGM).exe: turboc.cfg $(OBJS) $(PGM).res
     $(LINK) $(LFLAGS) @&&!
	  $(BCROOT)\lib\c0w$(MODEL) $(OBJS)
	  $(PGM)
	  nul
	  $(OWLLIBS) $(LIBS)
	  $(PGM).def
!
     rc -K $(PGM).res $(PGM).exe

$(PGM).res: $(PGM).rc 
     rc -r -I$(INCLDIR) $(PGM).rc

turboc.cfg: $(PGM).mak
	echo -I$(INCLDIR) $(WINMODE) -L$(LIBDIR)>turboc.cfg

.cpp.obj :
    $(CC) $(CFLAGS) $<

clean:
	del *.obj
	del *.exe
	del *.res
	del *.map
	del *.sym

