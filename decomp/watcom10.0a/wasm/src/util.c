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

/* Adapted from bld/wasm/c/util.c, initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CFf6c0..f6f5. */
#include <stdio.h>
#include <stdlib.h>
#include "asminit.h"
#include <recon.h>

#define CMD_POBJ_MIN_CMD 0x6c
#define CMD_MAX_CMD 0xce
pobj_filter jumpTable[CMD_MAX_CMD - CMD_POBJ_MIN_CMD + 1];

void PObjRegList(const pobj_list *list, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i) {
        jumpTable[list[i].command - CMD_POBJ_MIN_CMD] = list[i].func;
    }
}

/* CF27268: current object writer pass and owned output stream. */
pobj_state pobjState;

/* bld/womp/c/queue.c QInit, actualCF1c180..1c18e. */
void QInit(qdesc *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
}

/* bld/womp/c/queue.c QEnqueue, actualCF1c190..1c1ca. */
void QEnqueue(qdesc *queue, void *item)
{
    if (queue->head == NULL) {
        queue->head = queue->tail = item;
    } else {
        if (queue->tail == NULL) InternalError("g:\\bld\\womp\\c\\queue.c", 32);
        *(void **)queue->tail = item;
        queue->tail = item;
    }
    *(void **)item = NULL;
}

/* CF1c210..1c22d, released queue.c head removal. */
void *QDequeue(qdesc *queue)
{
    void *item;
    if (queue->head == NULL) return NULL;
    item = queue->head;
    queue->head = *(void **)item;
    if (queue->head == NULL) queue->tail = NULL;
    return item;
}

/* CFf6f4 is both PObjRegList's terminal RET and this independent empty
 * interface entry. Statically qualified against releasedutil.c66..72 and
 * the sole C3; seeded out of entry tracing, with zero new physical bytes. */
void PObjUnRegList(const pobj_list *list, size_t len)
{
    (void)list;
    (void)len;
}

/* Released util.c ObjWriteError, physical CFf72c..f740 DiskError identity. */
void DiskError(void)
{
    MsgPrintf(1105);
    exit(255);
}

/* bld/wasm/c/util.c InternalError, actual CFf6f8..f724.
 * The English1104 format is24 bytes plus NUL, within the original80-byte
 * owner despite MsgGet's general128-byte capacity. Preserve resource lookup,
 * unsigned line formatting and the actual1994 exit255, not later EXIT_FAILURE.
 */
int InternalError(const char *file, unsigned line)
{
    char buffer[80];
    MsgGet(1104,buffer);
    printf(buffer,file,line);
    exit(255);
}
