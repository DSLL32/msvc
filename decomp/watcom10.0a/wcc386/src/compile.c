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


#include <stdlib.h>
#include <setjmp.h>
#include "compiler.h"
#include <recon.h>

char *WholeFName,*ForceInclude;
void *Environment;
int PrintWhiteSpace;

/* This earlier driver performs EmitInit/forced-include setup before Parse,
 * and leaves error recovery without the later MyExit/FreeRDir additions. */
void DoCCompile(char **cmdline)
{
    jmp_buf env;

    Environment=&env;
    if (setjmp(env)) {
        if (!CompFlags.quiet_mode) PrintStats();
        EmitAbort();
        PragmaFini();
        CloseFiles();
        FreeFNames();
        ErrCount=1;
        return;
    }
    ParseInit();
    if (!CompFlags.command_line_processed) {
        ForceInclude=getenv("FORCE");
        ParseCmdLine(cmdline);
    }
    if (WholeFName==NULL) {
        CErr1(0x472);
        return;
    }
    DelErrFile();
    OpenErrFile();
    MergeInclude();
    CPragmaInit();
    EmitInit();
    if (ForceInclude) {
        if (CompFlags.cpp_output) PrtChar('\n');
        OpenSrcFile(ForceInclude,0);
        PrintWhiteSpace=0;
    }
    Parse();
    PragmaFini();
    CloseFiles();
    FreeFNames();
}

void ParseInit(void)
{
    ScanInit();
    CTypeInit();
    if (!CompFlags.command_line_processed) MacroInit();
    SymInit();
    SpcSymInit();
    StringInit();
    QuadInit();
    ExprInit();
    SegInit();
}
