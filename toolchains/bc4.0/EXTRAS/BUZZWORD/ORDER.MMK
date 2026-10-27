##### Module Macros ####
CC      = cl
TASM    = masm
TLIB    = lib
TLINK   = link
EXE     = order
LIBS = libw llibcew ddeml.lib
OBJS    = order.OBJ EVENT.OBJ IFIELD.OBJ\
          IBUTTON.OBJ ISTRING.OBJ IPAINT.OBJ IDBINFO.OBJ IFORM.OBJ IBITMAP.OBJ ITEXT.OBJ\
          IRECT.OBJ ILINE.OBJ IFLDLIST.OBJ ITABLE.OBJ IMENU.OBJ DBENGINE.OBJ IFONT.OBJ\
          SCREEN.OBJ DIB.OBJ
CCFLAGS = /DINC_H="order.h"
LLFLAGS =

##### C7 Macro #####
C7  = 1

##### Library Macros #####
##### LIBS    = libw llibcew              // moved up
MOD = -AL

CLOPT   = -Os

##### Tool Macros #####
ASM = masm -Mx
CC  = cl -nologo -c -G2sw -Zp -W0 $(MOD) $(CLOPT) /D__C700__ /DWINDOWS
LINK = link /NOD /NOE /NOI
RC  = rc -dRCCOMP $(DEF)
HC  = hc

##### Inference Rules #####
.c.obj:
    $(CC) $(CCFLAGS) -NT _$* $*.c

.asm.obj:
    $(ASM) $*.asm;

.rc.res:
    $(RC) -r $*

##### Main (default) Target #####
goal: $(EXE).exe

##### Dependents For Goal and Command Line #####
$(EXE).obj: $(EXE).c
    $(CC) $*.c

$(EXE).exe: $(OBJS) $(EXE).def $(EXE).res
    $(LINK) $(LLFLAGS) @<<
    $(OBJS),
    $(EXE).exe,
    $(EXE).map,
    $(LIBS),
    $(EXE).def
<<
    $(RC) -T -K $(EXE).res

