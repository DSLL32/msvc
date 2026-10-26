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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Adapted from bld/wasm/c/wasmmsg.c at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. The actual 1994 MsgInit
 * takes argv0, returns zero on success, and has no later language/probe call. */
#include <unistd.h>
#include <stdio.h>
#include <conio.h>
#include "wasm.h"
#include <recon.h>

static HANDLE_INFO hInstance = { 0, NULL };
static unsigned MsgShift;

int MsgInit(const char *program)
{
    int initerror;
    hInstance.filename = program;
    initerror = (OpenResFile(&hInstance) == -1);
    if (!initerror) {
        initerror = FindResources(&hInstance);
    }
    if (!initerror) {
        initerror = InitResources(&hInstance);
    }
    if (initerror) {
        MsgShift = 255;
        initerror = write(1, "Error: could not open message resource file.\r\n", 46);
    } else {
        MsgShift = 0;
    }
    return MsgShift;
}

/* CF16688..166a2: resource-relative SEEK_SET only. */
long res_seek(int handle, long position, int where)
{
    if (where == SEEK_SET) {
        return lseek(handle, position + FileShift, where) - FileShift;
    }
    return lseek(handle, position, where);
}

/* CF167e0..16841. The original loader's fifth startup parameter is NULL,
 * copied to this CRT-owned public data slot; no extra banner is supplied. */
char *_Copyright;
void PrintfUsage(unsigned first_line)
{
    char msg_buff[128];
    unsigned count;
    count = trademark();
    if (_Copyright != NULL) {
        puts(_Copyright);
        ++count;
    }
    for (;; ++first_line) {
        if (++count > 23) {
            Wait_for_return();
            count = 0;
        }
        MsgGet(first_line, msg_buff);
        if (msg_buff[0] == '.' && msg_buff[1] == 0) break;
        puts(msg_buff);
    }
}

/* CF16710..16737: actual return is LoadString's status (zero success),
 * unlike the later released boolean result; no MsgShift addition appears. */
int MsgGet(int resourceid, char *buffer)
{
    int status;
    status = LoadString(&hInstance, resourceid, buffer, 128);
    if (status != 0) buffer[0] = 0;
    return status;
}

/* CF167b8..167df with shared RET byte167b4. Native stdout is the shared
 * owner, so fileno supplies its descriptor; no raw FILE26 pointer crosses.
 * This case uses redirected stdout and takes the nonterminal branch. */
void Wait_for_return(void)
{
    if (isatty(fileno(stdout))) {
        output((const unsigned char *)"    (Press return to continue)");
        fflush(stdout);
        getch();
    }
}

/* bld/wasm/c/asmmsg.c DelErrFile; actualCF163f8..16402. */
void DelErrFile(void)
{
    remove(AsmFiles.fname[ERR]);
}

/* CF16844..1687a: older MsgFini tests initialization status, returns0/255,
 * and marks MsgShift255 after closing. Later sources test the handle. */
int MsgFini(void)
{
    unsigned status = 0;
    if (MsgShift == 0) {
        if (CloseResFile(&hInstance) != -1) MsgShift = 255;
        else status = 255;
    }
    return status;
}

/* CF164b0..16511, original asmmsg.c statistics with no clock fields. */
void PrintStats( void )
{
    printf( "%s: ", AsmFiles.fname[ASM] );
    printf( "%u lines, ", LineNumber );
    printf( "%u warnings, ", (unsigned)Options.warning_count );
    printf( "%u errors\n", (unsigned)Options.error_count );
    fflush( stdout );
}

/* CF16738..16772: resource text is the original printf format. */
void MsgPrintf(int resourceid)
{
    char buffer[128];
    if (!Options.banner_printed) {
        Options.banner_printed = 1;
        trademark();
    }
    MsgGet(resourceid, buffer);
    printf(buffer);
}

/* CF16774..167b5; its RET is also used by Wait_for_return. */
void MsgPrintf1(int resourceid, const char *arg)
{
    char buffer[128];
    if (!Options.banner_printed) {
        Options.banner_printed = 1;
        trademark();
    }
    MsgGet(resourceid, buffer);
    printf(buffer, arg);
}
