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

/* doCCompile: CF43d7d..43f6e. The2002 driver added batching, browsing,
 * DLL state, expanded status flags and additional cleanup. Only the actual
 * 1994 branches and calls are retained here. */
#include <stdlib.h>
#include <process.h>
#include <ctype.h>
#include <setjmp.h>
#include "compiler.h"
#include "frontend.h"
#include "initstate.h"
#include "cmdline.h"
#include "pending.h"
#include <recon.h>

unsigned WngCount, ErrCount;
char *ForceInclude, *WholeFName, *SrcFName;
void *Environment;

int doCCompile(char **argv)
{
    int status, gen_code;
    jmp_buf env;

    ExitPointAcquireRtn(&ExitPoint_cpp_front_end);
    Environment = env;
    if (setjmp(env)) {
        CompFlags.fatal_error = 1;
        status = makeExitStatus(1);
        CtxSetContext(1);
    } else {
        ScanInit();
        ForceInclude = getenv("FORCE");
        parseCmdLine(argv);
        if (!WholeFName) {
            CErr1(107);
        } else {
            PTypeCheckInit();
            ErrFileOpen();
            if (CompFlags.cpp_output) PpOpen();
            OpenPgmFile();
            if (ForceInclude) {
                CtxSetContext(5);
                if (CompFlags.cpp_output) PrtChar('\n');
                OpenSrcFile(ForceInclude, 0);
            }
            CtxSetContext(6);
            ExitPointAcquireRtn(&ExitPoint_cpp_preproc);
            if (CompFlags.cpp_output) {
                ExitPointAcquireRtn(&ExitPoint_cpp_preproc_only);
                PpParse();
            } else {
                ExitPointAcquireRtn(&ExitPoint_cpp_object);
                ExitPointAcquireRtn(&ExitPoint_cpp_analysis);
                CgFrontModInitInit();
                if (CompFlags.use_pcheaders) CompFlags.watch_for_pcheader = 1;
                NextToken();
                CompFlags.watch_for_pcheader = 0;
                CompFlags.external_defn_found = 0;
                ParseDecls();
                CtxSetContext(7);
                RtnGenerate();
                CtxSetContext(10);
                ModuleInitFini();
                ScopeEndFileScope();
                if (!CompFlags.external_defn_found) CErr1(99);
                gen_code = 1;
                ExitPointReleaseRtn(&ExitPoint_cpp_analysis);
                if (CompFlags.warnings_cause_bad_exit && WngCount)
                    gen_code = 0;
                else if (ErrCount || CompFlags.check_syntax)
                    gen_code = 0;
                if (gen_code) {
                    DwarfBrowseEmit();
                    CgBackEnd();
                }
                CtxSetContext(1);
                ExitPointReleaseRtn(&ExitPoint_cpp_object);
            }
        }
        status = makeExitStatus(0);
    }
    CgFrontFini();
    CloseFiles();
    ExitPointReleaseRtn(&ExitPoint_cpp_front_end);
    return status;
}

/* The 1994 EXE ignores the driver vector here and reads the CRT's original
 * command tail into a ten-kilobyte compiler-owned buffer. The later DLL
 * source accepts argv directly and has different help-return behavior. */
void parseCmdLine(char **argv)
{
    char *args[2];
    char *p;

    (void)argv;
    args[1] = 0;
    args[0] = (char *)CMemAlloc(0x2800);
    getcmd(args[0]);
    if (!args[0]) args[0] = "";
    p = args[0];
    while (isspace((unsigned char)*p)) ++p;
    if (*p == '?' || !*p) {
        CBanner();
        CCusage();
    }
    GenCOptions(args);
    if (WholeFName) MakePgmName(WholeFName);
    CMemFree(args[0]);
}

/* Initial plusplus.c MakePgmName;1994 ModuleName shares the interned name
 * instead of the later source's separate strsave allocation. */
void MakePgmName(char *filename)
{
    char buffer[260+3];
    char *drive, *directory, *name, *extension;
    _splitpath2(filename,buffer,&drive,&directory,&name,&extension);
    SrcFName=FNameAdd(name);
    if (ModuleName==NULL) ModuleName=SrcFName;
}

void OpenPgmFile(void)
{
    if (IoSuppOpenSrc(WholeFName,0)) {
        PpStartFile();
    } else {
        CErr2p(59,WholeFName);
        CSuicide();
    }
}

/* Actual first preprocessing initializer43aec is a standalone RET. Its
 * original source name remains unknown; the registration supplies its role. */
void preprocInit0(INITFINI *entry)
{
    (void)entry;
}

void PrtString(char *text) { (void)text; STUB_TRAP(); }
int OpenSrcFile(char *filename, int is_lib)
{
    int saved;
    if (IoSuppOpenSrc(filename,is_lib ? 2 : 1)) {
        PpStartFile();
        return 1;
    }
    saved=CompFlags.cpp_output;
    if (CompFlags.cpp_output) {
        PrtChar(PreProcChar);
        PrtString("include ");
        PrtChar(is_lib ? '<' : '"');
        PrtString(filename);
        PrtChar(is_lib ? '>' : '"');
        CompFlags.cpp_output=0;
    }
    CErr2p(59,filename);
    CompFlags.cpp_output=saved;
    return 0;
}
