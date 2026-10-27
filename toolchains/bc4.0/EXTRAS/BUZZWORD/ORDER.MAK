.AUTODEPEND

#               *Translator Definitions*
CC = bcc
TASM = TASM
TLIB = tlib
TLINK = tlink
INCLUDEPATH = C:\BORLANDC\INCLUDE
LIBPATH = C:\BORLANDC\LIB
EXE     = order
EXT     = .c
LIBS    = cwl.lib
OBJS    = EVENT.OBJ IFIELD.OBJ IBUTTON.OBJ ISTRING.OBJ IPAINT.OBJ IDBINFO.OBJ IFORM.OBJ\
          IBITMAP.OBJ ITEXT.OBJ IRECT.OBJ ILINE.OBJ IFLDLIST.OBJ ITABLE.OBJ IMENU.OBJ\
          DBENGINE.OBJ IFONT.OBJ SCREEN.OBJ DIB.OBJ
CCFLAGS = -P-.C -DINC_H="order.h"
LLFLAGS =


#               *Implicit Rules*
.c.obj:
  $(CC) -c $(CCFLAGS) +$(EXE).CFG {$&$(EXT) }

.cpp.obj:
  $(CC) -c $(CCFLAGS) +$(EXE).CFG {$&$(EXT) }

.cxx.obj:
  $(CC) -c $(CCFLAGS) +$(EXE).CFG {$< }

#               *List Macros*
Link_Exclude =  \
 $(EXE).res

Link_Include =  \
 $(EXE).obj $(OBJS) \
 $(EXE).def \
 {$(LIBPATH)}import.lib

#               *Explicit Rules*
$(EXE).exe: $(EXE).cfg $(Link_Include) $(Link_Exclude)
  $(TLINK) $(LLFLAGS)/v/s/i/c/Vt/Twe/L$(LIBPATH) @&&|
c0wl.obj+
$(EXE).obj $(OBJS)
$(EXE),$(EXE)
import.lib+
mathwl.lib+
$(LIBS)
$(EXE).def
|
  RC -K $(EXE).res $(EXE).exe


#               *Individual File Dependencies*
$(EXE).obj: $(EXE).cfg $(EXE).c

$(EXE).res: $(EXE).cfg $(EXE).rc
        BRCC -DRCCOMP -R -I$(INCLUDEPATH) -FO $(EXE).res $(EXE).RC

#               *Compiler Configuration File*
$(EXE).cfg: $(EXE).mak
  copy &&|
-Ff=10
-3
-ml
-G
-O
-Og
-Oe
-Om
-Ov
-Ol
-Op
-Oi
-Z
-k-
-W
-vi-
-H=$(EXE).SYM
-wpin
-wamb
-wamp
-wasm
-wpro
-wcln
-wdef
-wsig
-wnod
-wstv
-wucp
-wuse
-weas
-wpre
-w-ccc
-w-par
-w-ofp
-I$(INCLUDEPATH)
-L$(LIBPATH)
| $(EXE).cfg
