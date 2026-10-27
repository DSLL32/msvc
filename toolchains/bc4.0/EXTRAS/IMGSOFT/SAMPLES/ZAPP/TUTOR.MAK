#
# Tutor - Generic Make File
#
INCDIR = C:\borlandc\include
ZAPPDIR = C:\zApp
COMPILER = b3

NAME = Tutor
OFILES = $(NAME).OBJ \
				jtools.obj \

LFILES = $(OFILES)

RCOPT = 

!if "$(COMPILER)"=="c7"
CC = cl
CPPFLAGS= -AL -G2 -GA -Gs -Gy -Gx -GEs -Zpe -Ox
INC=-I$(ZAPPDIR)\INCLUDE -I$(INCDIR)

c7: Tutor.mak c7exe

!endif

!if "$(COMPILER)"=="b3"
CC = bcc
CPPFLAGS=-ml -WE -V -w-par -2 -O -d -X -Vf -v
INC=-I$(ZAPPDIR)\INCLUDE;$(INCDIR)

b3: Tutor.mak b3exe

!endif

!if "$(COMPILER)"=="z3"
CC = ztc
CPPFLAGS =-a -bx -c -W2 -ml -o+all
INC=-i$(ZAPPDIR)\INCLUDE -i$(INCDIR)

z3: Tutor.mak z3exe

!endif

.cpp.obj:
	$(CC) $(CPPFLAGS) $(INC) -c $*.CPP

c7exe: $(OFILES) makec7.lnk $(NAME).res
	link @makec7.lnk
	rc $(RCOPT) $(NAME).res

z3exe: $(OFILES) makez3.lnk $(NAME).res
	blink @makez3.lnk
	rc $(RCOPT) $(NAME).res

b3exe: $(OFILES) makeb3.lnk $(NAME).res
	tlink /v /P=16000/yx @makeb3.lnk
	rc $(RCOPT) $(NAME).res

$(NAME).res: $(NAME).rc
	rc -r $(NAME).rc

clean:
	del *.obj
	del *.lnk
	del *.res
	del *.exe
	del *.map

makeb3.lnk: $(NAME).mak
	echo c0wl.obj+$(LFILES) > makeb3.lnk
	echo $(NAME) >>makeb3.lnk
	echo $(NAME) >> makeb3.lnk
	echo $(ZAPPDIR)\LIB\zappbc3.lib+ >>makeb3.lnk
	echo cwl.lib+import.lib+ >>makeb3.lnk
	echo mathwl.lib+cl.lib >>makeb3.lnk
	echo $(NAME).def >>makeb3.lnk

makez3.lnk: $(NAME).mak
	echo $(LFILES) >makez3.lnk
	echo $(NAME)/align:16, >>makez3.lnk
	echo $(NAME)/NOD/PACKC:16000/FAR/NOE, >>makez3.lnk
	echo $(ZAPPDIR)\LIB\zappz3.lib libw zwl, >>makez3.lnk
	echo $(NAME).def >>makez3.lnk

makec7.lnk: $(NAME).mak
	echo $(LFILES) > makec7.lnk
	echo $(NAME)/align:16, >>makec7.lnk
	echo $(NAME)/NOD/PACKD/PACKF/PACKC:16000/FAR/NOE, >>makec7.lnk
	echo $(ZAPPDIR)\LIB\zappc7.lib llibcew libw, >>makec7.lnk
	echo $(NAME).def >> makec7.lnk

