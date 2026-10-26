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

/* front_end: actual1994 CF43f6e..43fef. Later plusplus.c moved
 * stream setup and shutdown into DLL lifetime handling. This executable
 * configures stdout twice because its errout alias is also stdout. */
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "compiler.h"
#include "frontend.h"
#include <recon.h>

struct comp_flags CompFlags;
int SwitchChar;

int front_end(char **argv)
{
    int status;

    IoSuppSetLineBuffering(stdout, 256);
    IoSuppSetLineBuffering(stdout, 256);
    SrcFileFClose(stdaux);
    SrcFileFClose(stdprn);
    frontend_host_hook();
    atexit(resetHandlers);
    if (!isatty(fileno(stdout))) CompFlags.no_conmsg = 1;
    SwitchChar = _dos_switch_char();
    PpInit();
    status = doCCompile(argv);
    CtxSetContext(1);
    return status;
}

/* Original name is not recovered. The reached e1d1 entry is exactly RET0,
 * called by both setup and resetHandlers in this configuration. */
void frontend_host_hook(void)
{
}

/* Shipped swchar.c NT/OS2 branch, also used behind the CF gateway. */
int _dos_switch_char(void)
{
    return '/';
}

void resetHandlers(void)
{
    frontend_host_hook();
    CloseFiles();
}

FILE *DefFile;

void CloseFiles(void)
{
    if (CppFile != NULL) {
        fflush(CppFile);
        IoSuppCloseFile(&CppFile);
    }
    IoSuppCloseFile(&DefFile);
}
