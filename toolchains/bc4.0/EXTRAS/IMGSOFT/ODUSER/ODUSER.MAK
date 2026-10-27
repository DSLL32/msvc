LIBPATH = C:\BORLANDC\LIB
INCLUDEPATH = C:\BORLANDC\INCLUDE
CC = bcc -R -ml! -WD -2 -I$(INCLUDEPATH)
LINK = tlink /x/P-/Twd/L$(LIBPATH)
RC = brc


.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

.rc.res:
    $(RC) -r {$< }


oduser.dll: oduser.def oduser.obj oduser.res
  $(LINK)  @&&|
c0dl.obj+
oduser.obj
oduser.dll
		# no map file
mathwl.lib+
import.lib+
cwl.lib
oduser.def
|
  RC  oduser.res oduser.dll

oduser.obj: oduser.c 
