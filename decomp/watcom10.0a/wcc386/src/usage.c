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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


#include <conio.h>
#include <stdio.h>
#include <unistd.h>
#include "command.h"
#include <recon.h>

void CCusage(void)
{
    /* Encountered complete help-line table, shared by the printing loop. */
    static char *usage[]={
        "Usage: wcc386 [options] file [options]",
        "Options:",
        "\t     ( /option is also accepted )",
        "-b{m,d,w}\tbuild (Multi thread,Dynamic link,Default windowing)",
        "-bt[=<os>]\tbuild target for operating system <os>",
        "-db\t\tgenerate browsing information",
        "-d1{+}\t\tline number debugging information",
        "-d2\t\tfull symbolic debugging information",
        "-d3\t\tfull symbolic debugging with unreferenced type names",
        "-d+\t\tallow extended -d macro definitions",
        "-d<name>[=text] precompilation #define name [text]",
        "-e<number>\tset error limit number",
        "-fh=<file_name>  use pre-compiled headers",
        "-fhq=<file_name> use pre-compiled headers quietly",
        "-fi=<file_name> force file_name to be included",
        "-fo=<file_name> set object or preprocessor output file name",
        "-ei\t\tforce enums to be type int",
        "-en\t\temit routine names in the code segment",
        "-ep[<number>]\tcall prologue hook routine with number stack bytes available",
        "-ee\t\tcall epilogue hook routine",
        "-ez\t\tgenerate PharLap EZ-OMF object files",
        "-fpc\t\tcalls to floating-point library",
        "-fpi\t\tinline 80x87 instructions with emulation",
        "-fpi87\t\tinline 80x87 instructions",
        "-fp2\t\toptimize floating-point for 287",
        "-fp3\t\toptimize floating-point for 387",
        "-fp5\t\toptimize floating-point for Pentium",
        "-fpr\t\tgenerate backward compatible 80x87 code",
        "-g=<codegroup>\tset code group name",
        "-h{w,d,c}\tset debug output format (Watcom,Dwarf,Codeview)",
        "-i=<directory>\tanother include directory",
        "-j\t\tchange char default from unsigned to signed",
        "-m{s,m,c,l,f}\tmemory model (Small,Medium,Compact,Large,Flat)",
        "-3r\t\t386 register calling conventions",
        "-3s\t\t386 stack calling conventions",
        "-4r\t\t486 register calling conventions",
        "-4s\t\t486 stack calling conventions",
        "-5r\t\tPentium register calling conventions",
        "-5s\t\tPentium stack calling conventions",
        "-7\t\tinline 80x87 instructions",
        "-nc=<name>\tset code class name",
        "-nd=<name>\tset data segment name",
        "-nm=<name>\tset module name",
        "-nt=<name>\tset name of text segment",
        "-o{a,c,d,e,f[+],i,l,m,n,o,p,r,s,t,u,x} control optimization",
        "    a\t\t-> relax aliasing constraints",
        "    c\t\t-> disable <call followed by return> to <jump> optimization",
        "    d\t\t-> disable all optimizations",
        "    e[=<num>]\t-> expand user functions inline. <num> controls max size",
        "    f\t\t-> generate traceable stack frames as needed",
        "    f+\t\t-> always generate traceable stack frames",
        "    i\t\t-> expand intrinsic functions inline",
        "    l\t\t-> enable loop optimizations",
        "    l+\t\t-> enable loop unrolling",
        "    m\t\t-> generate inline 80x87 code for math functions",
        "    n\t\t-> allow numerically unstable optimizations",
        "    o\t\t-> continue compilation if low on memory",
        "    p\t\t-> generate consistent floating-point results",
        "    r\t\t-> reorder instructions for best pipeline usage",
        "    s\t\t-> favor code size over execution time in optimizations",
        "    t\t\t-> favor execution time over code size in optimizations",
        "    u\t\t-> all functions must have unique addresses",
        "    x\t\t-> equivalent to -omiler -s",
        "    z\t\t-> do not assume a pointer deref implies pointer not NULL",
        "-p{l,c,w=<num>} preprocess file",
        "    l\t\t-> insert #line directives",
        "    c\t\t-> preserve comments",
        "    w=<num>\t-> wrap output lines at <num> columns. Zero means no wrap.",
        "-r\t\tsave/restore segment registers across calls",
        "-ri\t\treturn chars and shorts as ints",
        "-s\t\tremove stack overflow checks",
        "-sg\t\tgenerate calls to grow the stack",
        "-st\t\ttouch stack through SS first",
        "-u<name>\tundefine macro name",
        "-v\t\toutput function declarations to .def",
        "-w<number>\tset warning level number",
        "-we\t\ttreat all warnings as errors",
        "-z{a,e} \tdisable/enable language extensions",
        "-zc\t\tplace literal strings in the code segment",
        "-zdf\t\tDS floats i.e. not fixed to DGROUP",
        "-zdp\t\tDS is pegged to DGROUP",
        "-zdl\t\tLoad DS directly from DGROUP",
        "-zff\t\tFS floats i.e. not fixed to a segment",
        "-zfp\t\tFS is pegged to a segment",
        "-zgf\t\tGS floats i.e. not fixed to a segment",
        "-zgp\t\tGS is pegged to a segment",
        "-zg\t\tgenerate function prototypes using base types",
        "-zk{0,1,2}\tdouble-byte char support: 0=Kanji,1=Chinese/Taiwanese,2=Korean",
        "-zk0u\t\ttranslate double-byte Kanji to UNICODE",
        "-zku=<codepage> load UNICODE translate table for specified code page",
        "-zl\t\tremove default library information",
        "-zld\t\tremove file dependency information",
        "-zm\t\tplace each function in separate segment",
        "-zp{1,2,4,8}\tpack structure members",
        "-zq\t\toperate quietly",
        "-zs\t\tsyntax check only",
        "-zt<number>\tset data threshold",
        "-zu\t\tSS != DGROUP",
        "-zw\t\tgenerate code for Microsoft Windows",
        NULL
    };
    unsigned count=0;
    char **line;
    if (_Copyright!=NULL) {
        BannerMsg(_Copyright);
        count=1;
    }
    for (line=usage;*line!=NULL;++line) {
        if (++count>20) {
            Wait_for_return();
            count=0;
        }
        ConsMsg(*line);
    }
    MyExit(1);
}

void Wait_for_return(void)
{
    if (isatty(fileno(stdout))) {
        ConsMsg("    (Press return to continue)");
        fflush(stdout);
        getch();
    }
}
