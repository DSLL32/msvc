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

/* Adapted from bld/womp/c/objrec.c at commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CF 1bc90..1bca7. */
#include <string.h>
#include "wasm.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif

static carve_t myCarver;

void ObjRecInit(void)
{
    myCarver = CarveCreate(sizeof(obj_rec), 16);
}

/* Initial womp/c/objrec.c ObjNewRec; CF1bcb4..1bce2.  Only its
 * named header fields are initialized; union payload stays caller-owned. */
obj_rec *ObjNewRec(unsigned char command)
{
    obj_rec *new;
    new = CarveAlloc(myCarver);
    new->command = command;
    new->data = NULL;
    new->length = 0;
    new->curoff = 0;
    new->is_32 = 0;
    new->is_phar = 0;
    new->free_data = 0;
    return new;
}

/* CF1bce4..1bd84. Each record kind retains its original owned payload
 * cleanup before returning the common object to the carve allocator. */
void ObjKillRec(obj_rec *record)
{
    fixup *item, *next;
    if (record == NULL) InternalError("g:\\bld\\womp\\c\\objrec.c", 58);
    if (record->free_data && record->data != NULL) ObjDetachData(record);
    switch (record->command) {
    case CMD_FIXUP:
        for (item = record->d.fixup.fixup; item != NULL; item = next) {
            next = item->next;
            FixKill(item);
        }
        break;
    case CMD_LINNUM: case CMD_LINSYM:
        if (record->d.linnum.lines != NULL) MemFree(record->d.linnum.lines);
        break;
    case CMD_PUBDEF: case CMD_STATIC_PUBDEF:
        if (record->d.pubdef.free_pubs) {
            if (record->d.pubdef.pubs == NULL)
                InternalError("g:\\bld\\womp\\c\\objrec.c", 91);
            MemFree(record->d.pubdef.pubs);
        }
        break;
    }
    CarveFree(myCarver, record);
}

/* CF1bd84..1bdbc;1994 assertion line102. */
void ObjAllocData(obj_rec *objr, unsigned short len)
{
    if (objr->data != NULL) InternalError("g:\\bld\\womp\\c\\objrec.c",102);
    objr->data = MemAlloc(len);
    objr->length = len;
    objr->free_data = 1;
}

/* CF1c0dc..1c11b. */
void ObjPutName(obj_rec *objr, const unsigned char *name, unsigned char len)
{
    if (objr == NULL || objr->data == NULL)
        InternalError("g:\\bld\\womp\\c\\objrec.c",243);
    ObjPut8(objr,len);
    ObjPut(objr,name,len);
}

/* CF1bfa4..1bfda; the postincrement wraps the original16-bit cursor. */
void ObjPut8(obj_rec *objr, unsigned char value)
{
    if (objr == NULL || objr->data == NULL)
        InternalError("g:\\bld\\womp\\c\\objrec.c",193);
    objr->data[objr->curoff++] = value;
}

/* CF1c090..1c0da. */
void ObjPut(obj_rec *objr, const unsigned char *data, unsigned short len)
{
    if (objr == NULL || objr->data == NULL)
        InternalError("g:\\bld\\womp\\c\\objrec.c",236);
    memcpy(objr->data + objr->curoff,data,len);
    objr->curoff += len;
}

/* CF1bf50..1bfa3. Cursor advancement precedes the bounds assertion. */
unsigned char *ObjGet(obj_rec *objr, unsigned short len)
{
    unsigned char *ptr;
    if (objr == NULL || objr->data == NULL)
        InternalError("g:\\bld\\womp\\c\\objrec.c",183);
    ptr = objr->data + objr->curoff;
    objr->curoff += len;
    if (objr->curoff > objr->length)
        InternalError("g:\\bld\\womp\\c\\objrec.c",187);
    return ptr;
}

/* CF1bdec..1be36. free_data/cursor remain unchanged after detachment. */
void ObjDetachData(obj_rec *objr)
{
    if (objr == NULL) InternalError("g:\\bld\\womp\\c\\objrec.c",118);
    if (objr->data == NULL) InternalError("g:\\bld\\womp\\c\\objrec.c",119);
    if (objr->free_data) MemFree(objr->data);
    objr->data = NULL;
    objr->length = 0;
}

/* CF1bdbc..1bdea. Attached caller storage remains borrowed. */
void ObjAttachData(obj_rec *objr, void *data, unsigned short len)
{
    if (objr->data != NULL) InternalError("g:\\bld\\womp\\c\\objrec.c",110);
    objr->data = data;
    objr->length = len;
    objr->free_data = 0;
}

/* CF1c044..1c08f, variable-length index into owned record storage. */
void ObjPutIndex(obj_rec *objr, unsigned short value)
{
    if (objr == NULL || objr->data == NULL)
        InternalError("g:\\bld\\womp\\c\\objrec.c",227);
    if (value > 0x7f) ObjPut8(objr,(unsigned char)((value >> 8) | 0x80));
    ObjPut8(objr,(unsigned char)value);
}

/* CF1bca8..1bcb2, release the same record carve allocated by ObjRecInit. */
void ObjRecFini(void)
{
    CarveDestroy(myCarver);
}
