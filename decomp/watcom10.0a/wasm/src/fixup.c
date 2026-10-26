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

/* Adapted bld/womp/c/fixup.c at initialcommit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CF1c250..1c267. */
#include "wasm.h"
#include <recon.h>

static carve_t myCarver;

void FixInit(void)
{
    /* The actual1994 constructor supplies18-byte fixups,64 per block. */
    myCarver = CarveCreate(18, 64);
}

/* CF1c274..1c27e, shared carve owner from FixInit. */
fixup *FixNew(void)
{
    return CarveAlloc(myCarver);
}

/* CF1c55c..1c6e3; seven-way location encoding table at1c540. */
size_t FixGenFix(fixup *fix, char *buffer, int type)
{
    unsigned char byte;
    unsigned short data_rec_offset;
    char *p = buffer;
    if (fix == NULL) InternalError("g:\\bld\\womp\\c\\fixup.c",357);
    if (buffer == NULL) InternalError("g:\\bld\\womp\\c\\fixup.c",358);
    if (type != FIX_GEN_INTEL && type != FIX_GEN_PHARLAP && type != FIX_GEN_MS386)
        InternalError("g:\\bld\\womp\\c\\fixup.c",360);
    byte = fix->self_relative ? 0x80 : 0xc0;
    switch (fix->loc_method) {
    case FIX_LO_BYTE: break;
    case FIX_OFFSET:
        byte |= 1 << 2;
        if (fix->loader_resolved) {
            if (type == FIX_GEN_PHARLAP) {
                WompPrtMsg(0x8039);
            } else {
                if (type != FIX_GEN_INTEL && type != FIX_GEN_MS386)
                    InternalError("g:\\bld\\womp\\c\\fixup.c",373);
                byte |= 5 << 2;
            }
        }
        break;
    case FIX_BASE: byte |= 2 << 2; break;
    case FIX_POINTER: byte |= 3 << 2; break;
    case FIX_HI_BYTE: byte |= 4 << 2; break;
    case FIX_OFFSET386:
        if (type == FIX_GEN_PHARLAP) {
            if (fix->loader_resolved) WompPrtMsg(0x8039);
            byte |= 5 << 2;
        } else {
            byte |= (fix->loader_resolved ? 13 : 9) << 2;
        }
        break;
    case FIX_POINTER386: byte |= (type == FIX_GEN_PHARLAP ? 6 : 11) << 2; break;
    default: InternalError("g:\\bld\\womp\\c\\fixup.c",407);
    }
    if (fix->loc_offset >= 1024)
        InternalError("g:\\bld\\womp\\c\\fixup.c",409);
    data_rec_offset = fix->loc_offset;
    byte |= data_rec_offset >> 8;
    *p++ = byte;
    *p++ = (unsigned char)data_rec_offset;
    p += FixGenLRef(&fix->lr,p,type);
    return p - buffer;
}

/* CF1c378..1c459, logical frame/target datum and displacement encoding. */
size_t FixGenLRef(struct logref *ref, char *buffer, int type)
{
    char *p;
    unsigned char target;
    if (ref == NULL) InternalError("g:\\bld\\womp\\c\\fixup.c",294);
    if (buffer == NULL) InternalError("g:\\bld\\womp\\c\\fixup.c",295);
    if (type != FIX_GEN_INTEL && type != FIX_GEN_PHARLAP && type != FIX_GEN_MS386)
        InternalError("g:\\bld\\womp\\c\\fixup.c",297);
    target = ref->target;
    if (ref->target_offset == 0 && ref->is_secondary) target |= 4;
    p = buffer;
    *p++ = (ref->frame << 4) | target;
    p = putFrameDatum(p,ref->frame,ref->frame_datum);
    p = putTargetDatum(p,target,ref->target_datum);
    if ((target & 4) == 0) {
        if (type == FIX_GEN_MS386 || type == FIX_GEN_PHARLAP)
            p = put32(p,ref->target_offset);
        else
            p = put16(p,ref->target_offset);
    }
    return p - buffer;
}

/* CF1c2dc..1c330; frame methods0..2 carry indices,3 an absolute word. */
char *putFrameDatum(char *p, unsigned char method, unsigned short datum)
{
    if (p == NULL) InternalError("g:\\bld\\womp\\c\\fixup.c",267);
    switch (method) {
    case F_SEG: case F_GRP: case F_EXT: return putIndex(p,datum);
    case F_ABS: return put16(p,datum);
    }
    return p;
}

/* CF1c2b0..1c2cc, variable-width Intel OMF index. */
char *putIndex(char *p, unsigned short value)
{
    if (value > 0x7f) *p++ = 0x80 | (value >> 8);
    *p++ = value;
    return p;
}

/* CF1c330..1c377, secondary target flag does not change datum encoding. */
char *putTargetDatum(char *p, unsigned char method, unsigned short datum)
{
    if (p == NULL) InternalError("g:\\bld\\womp\\c\\fixup.c",282);
    if ((method & 3) == T_ABSWD) return put16(p,datum);
    return putIndex(p,datum);
}

/* CF1c2a0..1c2af, return the relocation to its FixInit carve owner. */
void FixKill(fixup *item)
{
    CarveFree(myCarver,item);
}

/* CF1c268..1c272, same carve owner allocated by FixInit. */
void FixFini(void)
{
    CarveDestroy(myCarver);
}
