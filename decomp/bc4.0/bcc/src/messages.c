#include "bcc.h"
#include <stdio.h>
#include <process.h>
#include <recon.h>

extern void _exit(int);

char * __stdcall lookup_message(unsigned char id)
{
    if (id == 25)
        return "Borland C++ Version ";
    if (id == 26)
        return " for Win32 Copyright (c) 1993 Borland International\r\n";
    if (id == 41)
        return "Syntax is: BCC32 [ options ] file[s]     * = default; -x- = turn switch x off\r\n"
               "  -3    * 80386 Instructions        -4      80486 Instructions\r\n"
               "  -Ax     Disable extensions        -B      Compile via assembly\r\n"
               "  -C      Allow nested comments     -Dxxx   Define macro\r\n"
               "  -Exxx   Alternate Assembler name  -Hxxx   Use pre-compiled headers\r\n"
               "  -Ixxx   Include files directory   -K      Default char is unsigned\r\n"
               "  -Lxxx   Libraries directory       -M      Generate link map\r\n"
               "  -N      Check stack overflow      -Ox     Optimizations\r\n"
               "  -P      Force C++ compile         -R      Produce browser info\r\n"
               "  -RT   * Generate RTTI             -S      Produce assembly output\r\n"
               "  -Txxx   Set assembler option      -Uxxx   Undefine macro\r\n"
               "  -Vx     Virtual table control     -Wxxx   Create Windows application\r\n"
               "  -X      Suppress autodep. output  -aN     Align on N bytes\r\n"
               "  -b    * Treat enums as integers   -c      Compile only\r\n"
               "  -d      Merge duplicate strings   -exxx   Executable file name\r\n"
               "  -fxx    Floating point options    -gN     Stop after N warnings\r\n"
               "  -iN     Max. identifier length    -jN     Stop after N errors\r\n"
               "  -k    * Standard stack frame      -lx     Set linker option\r\n"
               "  -nxxx   Output file directory     -oxxx   Object file name\r\n"
               "  -p      Pascal calls              -u    * Underscores on externs\r\n"
               "  -v      Source level debugging    -wxxx   Warning control\r\n"
               "  -xxxx   Exception handling        -y      Produce line number info\r\n"
               "  -zxxx   Set segment names\r\n";
    /* The remaining message table has not yet been recovered. */
    BCC_TRAP();
    return 0;
}

void __stdcall print_text(const char *text)
{
    printf("%s", text);
}

void print_usage(void)
{
    print_text(lookup_message(41));
    _exit(0);
}
