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

/* errFileInit from bld/plusplus/c/message.c, initial commit
 *6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Actual1994 initializes
 * three pointer cells, a2048-byte reserve and its cleanup callback only;
 * later international-message loading and additional state writes are absent. */
#include <stdio.h>
#include "compiler.h"
#include "frontend.h"
#include "pending.h"
#include "cgio.h"
#include <recon.h>

static FILE *err_file;
static void *err_locn_source;
static SUICIDE_CALLBACK *suicideCallbacks;
static unsigned reserveSize;
static void *reserveMem;

void errFileInit(INITFINI *entry)
{
    (void)entry;
    err_file = 0;
    err_locn_source = 0;
    suicideCallbacks = 0;
    reserveSize = 2048;
    reserveMem = CMemAlloc(2048);
    CMemRegisterCleanup(reserveRelease);
}

char *IoSuppOutFileName(unsigned kind)
{ (void)kind; STUB_TRAP(); return 0; }
void errFileErase(const char *name)
{ (void)name; STUB_TRAP(); }
FILE *SrcFileFOpen(char *name, unsigned kind);
void IoSuppSetBuffering(FILE *file, unsigned size)
{ (void)file; (void)size; STUB_TRAP(); }

/* Actual old implementation retries by source-name availability and the
 * no_conmsg flag; it has no later errfile_opened/NULL-name test. */
void ErrFileOpen(void)
{
    char *name;
    if (!CompFlags.no_conmsg && SrcFName!=0) {
        name=IoSuppOutFileName(2);
        errFileErase(name);
        err_file=SrcFileFOpen(name,3);
        if (err_file!=0) IoSuppSetBuffering(err_file,128);
    }
}

/* Actual1994 CF417db..417ea; same registration in the released message.c. */
void RegisterSuicideCallback(SUICIDE_CALLBACK *ctl)
{
    RingAppend((void **)&suicideCallbacks, ctl);
}

void CErrCheckpoint(unsigned *state)
{
    *state=ErrCount;
}

void SetErrLoc(CGIO_LOCATION *location)
{ (void)location; STUB_TRAP(); }

int CErrOccurred(unsigned *state)
{ return *state!=ErrCount; }
