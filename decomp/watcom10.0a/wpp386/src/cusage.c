/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

/* Banner source correlated to initial Open Watcom watcom.c (6b9cb443).
 * Keep the actual 1994 product text and stdio calls, before the later
 * MsgDisplayBanner redirection. Help display shares this source context. */
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <conio.h>
#include "frontend.h"
#include <recon.h>

/* Mutable vendor fingerprint; its leading '$' suppresses its display. */
char Token[] = "$123456789$ For use by WATCOM Systems Inc. ****";

/* The encountered read-only help table retains its known Usage name.
 * A file-scope data public also lets the period MAP distinguish this object
 * from the preceding function; it is never registered as a function. */
const char * const Usage[] = {
    "Usage: wpp386 [options] file [options]",
    "Options:",
    "             ( /option is also accepted )",
    "-bm             build target is a multi-thread environment",
    "-bd             build target is a dynamic link library (DLL)",
    "-bw             build target is a default windowing application",
    "-bt[=<os>]      build target is operating system <os>",
    "-db             generate browsing information",
    "-d1             line number debugging information",
    "-d2             full symbolic debugging information",
    "-d3             full symbolic debugging with unreferenced type names",
    "-d+             allow extended -d macro definitions",
    "-d<name>[=text] precompilation #define name [text]",
    "-e<number>      set error limit number",
    "-en             emit routine names in the code segment",
    "-ep[<number>]   call prologue hook routine with number stack bytes available",
    "-ee             call epilogue hook routine",
    "-ew             alternate error message formatting",
    "-ez             generate PharLap EZ-OMF object files",
    "-fh             use pre-compiled headers",
    "-fhq\t\tdo not display pre-compiled header activity warnings",
    "-fh=<file_name> set pre-compiled header file name",
    "-fi=<file_name> force file_name to be included",
    "-fo=<file_name> set object or preprocessor output file name",
    "-ft             check for truncated versions of file names",
    "-fx             do not check for truncated versions of file names",
    "-fpc            calls to floating-point library",
    "-fpi            inline 80x87 instructions with emulation",
    "-fpi87          inline 80x87 instructions",
    "-fpr            generate backward compatible 80x87 code",
    "-fp2            generate 287 floating-point code",
    "-fp3            generate 387 floating-point code",
    "-fp5            optimize floating-point for Pentium",
    "-g=<codegroup>  set code group name",
    "-h{w,d,c}       set debug format (Watcom,Dwarf,Codeview)",
    "-i=<directory>  another include directory",
    "-j              change char default from unsigned to signed",
    "-m{s,m,c,l,f}   memory model (Small,Medium,Compact,Large,Flat)",
    "-3r             386 register calling conventions",
    "-3s             386 stack calling conventions",
    "-4r             486 register calling conventions",
    "-4s             486 stack calling conventions",
    "-5r             Pentium register calling conventions",
    "-5s             Pentium stack calling conventions",
    "-7              inline 387 instructions",
    "-nc=<name>      set code class name",
    "-nd=<name>      set data segment name",
    "-nm=<name>      set module name",
    "-nt=<name>      set name of text segment",
    "-o{a,c,d,e,f[+],i,l[+],m,n,o,p,r,s,t,u,x,z} control optimization",
    "    a           -> relax aliasing constraints",
    "    c           -> disable <call followed by return> to <jump> optimization",
    "    d           -> disable all optimizations",
    "    e[=<num>]   -> expand user functions inline. <num> controls max size",
    "    f           -> generate traceable stack frames as needed",
    "    f+          -> always generate traceable stack frames",
    "    i           -> expand intrinsic functions inline",
    "    l           -> enable loop optimizations",
    "    l+          -> enable loop unrolling optimizations",
    "    m           -> generate inline 80x87 code for math functions",
    "    n           -> allow numerically unstable optimizations",
    "    o           -> continue compilation if low on memory",
    "    p           -> generate consistent floating-point results",
    "    r           -> reorder instructions for best pipeline usage",
    "    s           -> favor code size over execution time in optimizations",
    "    t           -> favor execution time over code size in optimizations",
    "    u           -> all functions must have unique addresses",
    "    x           -> equivalent to -omiler -s",
    "    z           -> NULL points to valid memory in the target environment",
    "-p{e,l,c,w=<num>} preprocess file",
    "    e           -> encrypt identifiers",
    "    l           -> insert #line directives",
    "    c           -> preserve comments",
    "    w=<num>     -> wrap output lines at <num> columns. Zero means no wrap.",
    "-r              save/restore segment registers across calls",
    "-ri             promote to int all function arguments and returns",
    "-s              remove stack overflow checks",
    "-sg             generate calls to grow the stack",
    "-st             touch stack through SS first",
    "-u<name>        undefine macro name",
    "-v              output function declarations to .def",
    "-w<number>      set warning level number",
    "-we             treat all warnings as errors",
    "-wx             set warning level to maximum setting",
    "-xd             disable exception handling (default) ",
    "-xdt            disable exception handling (same as -xd)",
    "-xds            disable exception handling (table-driven destructors)",
    "-xs             enable exception handling ",
    "-xst            enable exception handling (direct calls for destruction)",
    "-xss            enable exception handling (table-driven destructors)",
    "-za             disable extensions (i.e., accept only ISO/ANSI C++)",
    "-ze             enable extensions (i.e., near, far, export, etc.)",
    "-zc             place literal strings in the code segment",
    "-zdf            DS floats i.e. not fixed to DGROUP",
    "-zdl            Load DS directly from DGROUP",
    "-zdp            DS is pegged to DGROUP",
    "-zff            FS floats i.e. not fixed to a segment",
    "-zfp            FS is pegged to a segment",
    "-zgf            GS floats i.e. not fixed to a segment",
    "-zgp            GS is pegged to a segment",
    "-zg             generate function prototypes using base types",
    "-zk{0,1,2}      double-byte char support: 0=Kanji,1=Chinese/Taiwanese,2=Korean",
    "-zk0u           translate double-byte Kanji to UNICODE",
    "-zku=<codepage> load UNICODE translate table for specified code page",
    "-zl             remove default library information",
    "-zld            remove file dependency information",
    "-zm             place each function in separate segment",
    "-zp{1,2,4,8}    pack structure members",
    "-zq             operate quietly",
    "-zs             syntax check only",
    "-zt<number>     set far data threshold (i.e., larger objects go in far memory)",
    "-zu             SS != DGROUP",
    "-zw             generate code for Microsoft Windows",
    NULL
};

void CBanner(void)
{
    if (CompFlags.banner_printed || CompFlags.quiet_mode) return;
    puts("WATCOM C++32 Optimizing Compiler  Version 10.0a");
    puts("Copyright by WATCOM International Corp. 1989, 1994. All rights reserved.");
    puts("WATCOM is a trademark of WATCOM International Corp.");
    if (Token[0] != '$') puts(Token);
    fflush(stdout);
    CompFlags.banner_printed = 1;
}

void output(const char *text)
{
    /* The original enters with a nonempty help line. */
    do {
        putchar(*text++);
    } while (*text != '\0');
    putchar('\n');
}
void Wait_for_return(void)
{
    if (isatty(fileno(stdin))) {
        output("\n    (Press return to continue)");
        fflush(stdout);
        getch();
    }
}

/* OSI startup leaves this optional vendor text null in the shipped image. */
char *_Copyright;

void CCusage(void)
{

    const char * const *line;
    unsigned rows = 18;
    unsigned count = 0;

    if (_Copyright != NULL) {
        count = 1;
        output(_Copyright);
    }
    for (line = Usage; *line != NULL; ++line) {
        if (++count > rows) {
            Wait_for_return();
            rows = 20;
            count = 0;
        }
        output(*line);
    }
    exit(1);
}
