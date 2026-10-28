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


#include "compiler.h"
#include "scanner.h"
#include "files.h"
#include "statement.h"
#include <recon.h>

void QuadFini(void);
void SymsPurge(void);
/* Actual1994 cleanup omits the later separate FEfree(src_buf) call.
 * Its POP EDX/RET is physically shared with the real Purge helper. */
void SrcPurge(void)
{
    FCB *file;
    while ((file=SrcFile)!=NULL) {
        SrcFile=file->prev_file;
        CClose(file->src_fp);
        CMemFree(file);
    }
}
void SwitchPurge(void)
{
    struct switch_definition *sw;
    struct case_entry *entry,*next;
    while ((sw=SwitchStack)!=NULL) {
        SwitchStack=sw->prev_switch;
        for (entry=sw->case_list;entry!=NULL;entry=next) {
            next=entry->next_case;
            CMemFree(entry);
        }
        CMemFree(sw);
    }
}
void Purge(void **pointer)
{
    if (*pointer!=NULL) {
        CMemFree(*pointer);
        *pointer=NULL;
    }
}
/* The1994 driver lacks the later LitsPurge/FreeRDir/PCH cleanup calls.
 * Its final SavedId purge is a real known-entry fallthrough to Purge. */
void PurgeMemory(void)
{
    SymsPurge();
    PragmaFini();
    /* Surviving cpurge.c QuadFini declaration names the old intermediate
     * storage finalizer2b619; it is distinct from FreeDataQuads4bb9. */
    QuadFini();
    FreeFNames();
    SrcPurge();
    SwitchPurge();
    Purge((void **)&HFileList);
    Purge((void **)&SavedId);
}
