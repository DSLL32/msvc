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


/* The 1994 outer driver owns stream setup and shutdown. Later ccmain.c
 * redistributed these operations between FrontEnd and its host wrappers. */
#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include <recon.h>

struct comp_flags CompFlags;
int SwitchChar;

void FrontEnd(char **cmdline)
{
    InitGlobalVars();
    setvbuf(stdout,(char *)CPermAlloc(128),_IOLBF,128);
    fclose(stdaux);
    fclose(stdprn);
    InitMsg();
    atexit(CloseFiles);
    if (!isatty(fileno(stdout))) CompFlags.no_conmsg=1;
    SwitchChar=_dos_switch_char();
    ClearGlobals();
    DoCCompile(cmdline);
    PurgeMemory();
}

/* The original cdata contribution is a single RET. Its globals already
 * have their static initial values in this version. */
void InitGlobalVars(void)
{
}

/* Messages are compiled into this 1994 configuration; its InitMsg is RET. */
void InitMsg(void)
{
}

/* swchar.c NT/OS2 host branch; the CF loader also provides flat Win32 IO. */
int _dos_switch_char(void)
{
    return '/';
}

/* Persistent work-name spelling is mutable: temporary-file retries advance
 * its digit, and ClearGlobals restores it for another compilation. */
static char WorkFile[]="__wrk0__";
char IsStdIn;
struct fname_list *FNames;
FCB *SrcFile;
FILE *ErrFile,*DefFile,*CppFile,*PageFile;
char *HFileList;
int PageHandle,SegmentNum;

void ClearGlobals(void)
{
    InitStats();
    IsStdIn=0;
    FNames=NULL;
    SrcFile=NULL;
    ErrFile=NULL;
    DefFile=NULL;
    CppFile=NULL;
    HFileList=NULL;
    PageHandle=-1;
    WorkFile[5]='0';
    SegmentNum=11;
}

/* Empty old cpragma hook: its original C3 is shared with CTypeInit. */
void CPragmaInit(void)
{
}

void CloseFiles(void)
{
    char fname[260];
    if (CppFile!=NULL) {
        fflush(CppFile);
        fclose(CppFile);
        CppFile=NULL;
    }
    if (ErrFile!=NULL) {
        fclose(ErrFile);
        ErrFile=NULL;
        if (!CompFlags.errfile_written) DelErrFile();
    }
    if (DefFile!=NULL) {
        fclose(DefFile);
        DefFile=NULL;
    }
    if (PageFile!=NULL) {
        fclose(PageFile);
        MakeTmpName(fname);
        remove(fname);
        PageHandle=-1;
        PageFile=NULL;
    }
}
