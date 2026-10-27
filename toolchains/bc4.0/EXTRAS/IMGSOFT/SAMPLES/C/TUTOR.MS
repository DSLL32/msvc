# 
# Tutor
# Microsoft Make File

MODEL = M
CFLAGS = -A$(MODEL) -Gsw -W3 -Zpe -Oas
LFLAGS = /NOD
LIBS = $(MODEL)libcew + libw + mmsystem + commdlg

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

$(PGM).obj:			$*.c $(PGM).h
jtools.obj: $*.c $*.h	

$(PGM).res:   $*.rc $*.h

