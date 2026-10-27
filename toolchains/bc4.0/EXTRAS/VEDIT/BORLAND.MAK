# The following "trick" allows this makefile to generate two
# independent programs
#
demo.abc : ega43.exe adder.exe
     echo The MAKE process is done

adder.exe : adder.obj
    tcc adder.obj

adder.obj : adder.c
    tcc -c adder.c

ega43.exe : ega43.obj
     tlink ega43.obj

ega43.obj : ega43.asm
     tasm /zi ega43.asm
