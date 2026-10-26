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

/* Adapted from bld/womp/c/genmsomf.c at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Actual table1abe0..1ac70,
 * packed six-byte entries; all callback bodies remain named traps. */
#include <string.h>
#include "wasm.h"
#include <recon.h>

/* Historical WOMP declaration: its cold warning calls resolve to the
 * assembler's incompatible four-register PrtMsg at CF16380. Keep those
 * period calls, but reject the undefined native argument state. */
extern void PrtMsg(unsigned message, ...);
#ifdef __WATCOMC__
#pragma intrinsic(strlen)
#endif


#define POBJ_WRITE_PASS 2

/* External visibility lets the period MAP bound this code-resident table. */
const pobj_list myFuncs[] = {
    { CMD_THEADR, POBJ_WRITE_PASS, writeTheadr },
    { CMD_COMENT, POBJ_WRITE_PASS, writeComent },
    { CMD_MODEND, POBJ_WRITE_PASS, writeModend },
    { CMD_EXTDEF, POBJ_WRITE_PASS, writeMisc },
    { CMD_LNAMES, POBJ_WRITE_PASS, writeMisc },
    { CMD_SEGDEF, POBJ_WRITE_PASS, writeSegdef },
    { CMD_GRPDEF, POBJ_WRITE_PASS, writeMisc },
    { CMD_LEDATA, POBJ_WRITE_PASS, writeLedata },
    { CMD_FIXUP, POBJ_WRITE_PASS, writeFixup },
    { CMD_PUBDEF, POBJ_WRITE_PASS, writePubdef },
    { CMD_TYPDEF, POBJ_WRITE_PASS, writeMisc },
    { CMD_LINNUM, POBJ_WRITE_PASS, writeLinnum },
    { CMD_LIDATA, POBJ_WRITE_PASS, writeLidata },
    { CMD_COMDEF, POBJ_WRITE_PASS, writeMisc },
    { CMD_CEXTDF, POBJ_WRITE_PASS, writeMisc },
    { CMD_STATIC_EXTDEF, POBJ_WRITE_PASS, writeMisc32 },
    { CMD_STATIC_PUBDEF, POBJ_WRITE_PASS, writePubdef },
    { CMD_STATIC_COMDEF, POBJ_WRITE_PASS, writeMisc },
    { CMD_BAKPAT, POBJ_WRITE_PASS, writeMisc32 },
    { CMD_COMDAT, POBJ_WRITE_PASS, writeComdat },
    { CMD_LINSYM, POBJ_WRITE_PASS, writeLinsym },
    { CMD_ALIAS, POBJ_WRITE_PASS, writeMisc },
    { CMD_NBKPAT, POBJ_WRITE_PASS, writeMisc32 },
    { CMD_LLNAMES, POBJ_WRITE_PASS, writeMisc },
};

void GenMSOmfInit(void)
{
    PObjRegList(myFuncs, sizeof(myFuncs) / sizeof(myFuncs[0]));
}

/* CF27b90: LIDATA relocation list, initialized by the module header. */
static lifix_list lifList;
/* CF1b48c..1b4f7; original assertion lines377..380. */
int writeTheadr(obj_rec *objr, pobj_state *state)
{
    if (objr == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",377);
    if (objr->command != CMD_THEADR)
        InternalError("g:\\bld\\womp\\c\\genmsomf.c",378);
    if (state == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",379);
    if (state->pass != POBJ_WRITE_PASS)
        InternalError("g:\\bld\\womp\\c\\genmsomf.c",380);
    LifixInit(&lifList);
    return writeMisc(objr,state);
}

/* bld/womp/c/lifix.c at the same released commit; CF1c750..1c757. */
void LifixInit(lifix_list *list)
{
    list->head = NULL;
}

/* CF1ac70..1ad11, unchanged16-bit record representation. */
int writeMisc(obj_rec *objr, pobj_state *state)
{
    unsigned char *ptr;
    unsigned short len, save;
    if (objr == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",52);
    if (objr->data == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",53);
    if (state == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",54);
    if (state->pass != POBJ_WRITE_PASS)
        InternalError("g:\\bld\\womp\\c\\genmsomf.c",55);
    save = objr->curoff;
    objr->curoff = 0;
    len = objr->length - objr->curoff;
    ptr = ObjGet(objr,len);
    ObjWriteRec(state->file_out,objr->command,len,(const char *)ptr);
    objr->curoff = save;
    return 0;
}

/* CF1aea0..1b039, seven alignment targets atCF1ae84..1aea0. */
int writeSegdef(obj_rec *objr, pobj_state *state)
{
    OBJ_WFILE *out;
    int is32;
    unsigned char acbp, align;
    char buf[11];
    size_t len;
    if (objr == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",131);
    if (objr->command != CMD_SEGDEF)
        InternalError("g:\\bld\\womp\\c\\genmsomf.c",132);
    if (state == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",133);
    if (state->pass != POBJ_WRITE_PASS)
        InternalError("g:\\bld\\womp\\c\\genmsomf.c",134);
    out = state->file_out;
    is32 = objr->d.segdef.use_32;
    ObjWBegRec(out,is32 ? 0x99 : CMD_SEGDEF); /* CMD_SEGD32 */
    acbp = (objr->d.segdef.combine << 2) | (is32 != 0);
    align = objr->d.segdef.align;
    switch (align) {
    case 0: break; /* absolute */
    case 1: acbp |= 1 << 5; break; /* byte */
    case 2: acbp |= 2 << 5; break; /* word */
    case 3: acbp |= 3 << 5; break; /* paragraph */
    case 4: acbp |= 4 << 5; break; /* page */
    case 5: acbp |= 5 << 5; break; /* dword */
    case 6:
        acbp |= 4;
#ifdef __linux__
        WASM_PENDING();
#endif
        PrtMsg(0x8040); /* WRN | MSG_MS386_NO_4KPAGE */
        break;
    default: InternalError("g:\\bld\\womp\\c\\genmsomf.c",162);
    }
    if (!is32 && objr->d.segdef.seg_length == 0x10000) acbp |= 2;
    ObjWrite8(out,acbp);
    if (align == 0) {
        len = FixGenPRef(&objr->d.segdef.abs,buf,is32 ? 2 : 0);
        ObjWrite(out,buf,len);
    }
    if (is32) ObjWrite32(out,objr->d.segdef.seg_length);
    else ObjWrite16(out,(unsigned short)objr->d.segdef.seg_length);
    ObjWriteIndex(out,objr->d.segdef.seg_name_idx);
    ObjWriteIndex(out,objr->d.segdef.class_name_idx);
    ObjWriteIndex(out,objr->d.segdef.ovl_name_idx);
    if (objr->d.segdef.access_valid) {
#ifdef __linux__
        WASM_PENDING();
#endif
        PrtMsg(0x41); /* MSG_MS386_NO_ACCESS */
    }
    ObjWEndRec(out);
    return 0;
}

/* CF1b7ec..1b92c,8-byte public entries with16-bit NameArray handles. */
int writePubdef(obj_rec *objr, pobj_state *state)
{
    int is32;
    OBJ_WFILE *out;
    const char *name;
    unsigned char name_len;
    pubdef_data *pubdata, *pubstop;
    if (objr == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",516);
    if (objr->command != CMD_PUBDEF && objr->command != CMD_STATIC_PUBDEF)
        InternalError("g:\\bld\\womp\\c\\genmsomf.c",518);
    if (state == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",519);
    if (state->pass != POBJ_WRITE_PASS)
        InternalError("g:\\bld\\womp\\c\\genmsomf.c",520);
    is32 = objr->is_32 || objr->is_phar;
    out = state->file_out;
    ObjWBegRec(out,objr->command | (is32 ? 1 : 0));
    writeBase(objr,out);
    pubdata = objr->d.pubdef.pubs;
    if (pubdata != NULL) {
        pubstop = pubdata + objr->d.pubdef.num_pubs;
        while (pubdata < pubstop) {
            name = NameGet(pubdata->name);
            name_len = strlen(name);
            ObjWrite8(out,name_len);
            ObjWrite(out,name,(size_t)name_len);
            if (is32) ObjWrite32(out,pubdata->offset);
            else ObjWrite16(out,(unsigned short)pubdata->offset);
            ObjWriteIndex(out,pubdata->type.idx);
            ++pubdata;
        }
    }
    ObjWEndRec(out);
    return 0;
}

/* CF1b608..1b64c, common base prefix of public/line records. */
void writeBase(obj_rec *objr, OBJ_WFILE *out)
{
    unsigned short grp_idx = objr->d.base.grp_idx;
    unsigned short seg_idx = objr->d.base.seg_idx;
    ObjWriteIndex(out,grp_idx);
    ObjWriteIndex(out,seg_idx);
    if (grp_idx == 0 && seg_idx == 0)
        ObjWrite16(out,objr->d.base.frame);
}

/* CF1adcc..1ae81, attribute/class prefix plus remaining owned data bytes. */
int writeComent(obj_rec *objr, pobj_state *state)
{
    unsigned char *ptr;
    unsigned short len, save;
    OBJ_WFILE *out;
    if (objr == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",100);
    if (objr->data == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",101);
    if (state == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",102);
    if (state->pass != POBJ_WRITE_PASS)
        InternalError("g:\\bld\\womp\\c\\genmsomf.c",103);
    out = state->file_out;
    ObjWBegRec(out,CMD_COMENT);
    ObjWrite8(out,objr->d.coment.attr);
    ObjWrite8(out,objr->d.coment.class);
    save = objr->curoff;
    len = objr->length - objr->curoff;
    ptr = ObjGet(objr,len);
    ObjWrite(out,(const char *)ptr,len);
    objr->curoff = save;
    ObjWEndRec(out);
    return 0;
}

/* CF1b150..1b284. */
int writeLedata(obj_rec *objr, pobj_state *state)
{
    OBJ_WFILE *out;
    unsigned short save, len;
    unsigned char *ptr;
    int is32;
    if (objr == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",271);
    if (objr->command != CMD_LEDATA)
        InternalError("g:\\bld\\womp\\c\\genmsomf.c",272);
    if (state == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",273);
    if (state->pass != POBJ_WRITE_PASS)
        InternalError("g:\\bld\\womp\\c\\genmsomf.c",274);
    LifixDestroy(&lifList);
    LifixInit(&lifList);
    out = state->file_out;
    is32 = (objr->is_32 || objr->is_phar) != 0;
    ObjWBegRec(out,is32 ? 0xa1 : CMD_LEDATA); /* CMD_LEDA32 */
    ObjWriteIndex(out,objr->d.ledata.idx);
    if (is32) ObjWrite32(out,objr->d.ledata.offset);
    else ObjWrite16(out,(unsigned short)objr->d.ledata.offset);
    save = objr->curoff;
    len = objr->length - objr->curoff;
    ptr = ObjGet(objr,len);
    if (len > 1024) InternalError("g:\\bld\\womp\\c\\genmsomf.c",291);
    ObjWrite(out,(const char *)ptr,len);
    ObjWEndRec(out);
    objr->curoff = save;
    return 0;
}

/* bld/womp/c/lifix.c, CF1c774..1c78a. The caller resets head separately. */
void LifixDestroy(lifix_list *list)
{
    lifix *cur, *next;
    cur = list->head;
    while (cur != NULL) {
        next = cur->next;
        MemFree(cur);
        cur = next;
    }
}

/* CF1b03c..1b14e, initial-source writeFixup with original11B staging. */
int writeFixup(obj_rec *objr, pobj_state *state)
{
    OBJ_WFILE *out;
    int is32;
    fixup *walk;
    char buffer[11];
    size_t length, written;
    if (objr == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",219);
    if (objr->command != CMD_FIXUP) InternalError("g:\\bld\\womp\\c\\genmsomf.c",220);
    if (state == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",221);
    if (state->pass != POBJ_WRITE_PASS) InternalError("g:\\bld\\womp\\c\\genmsomf.c",222);
    walk = objr->d.fixup.fixup;
    if (walk == NULL) return 0;
    out = state->file_out;
    is32 = objr->is_32 || objr->is_phar;
    do {
        written = 0;
        ObjWBegRec(out,is32 ? CMD_FIXUP + 1 : CMD_FIXUP);
        while (walk != NULL && written < 1024 - sizeof(buffer)) {
            walk->loc_offset += LifixDelta(&lifList,walk->loc_offset);
            length = FixGenFix(walk,buffer,is32 ? 2 : 0);
            ObjWrite(out,buffer,length);
            walk = walk->next;
            written += length;
        }
        ObjWEndRec(out);
    } while (walk != NULL);
    return 0;
}

/* CF1c758..1c772, released lifix.c adjustment lookup. */
short LifixDelta(lifix_list *list, unsigned short old_offset)
{
    lifix *walk;
    for (walk = list->head; walk != NULL; walk = walk->next) {
        if (old_offset >= walk->lower_bound) return walk->delta;
    }
    return 0;
}

/* CF1b4f8..1b608, ordinary MODEND serialization and LIDATA fixup cleanup. */
int writeModend(obj_rec *objr, pobj_state *state)
{
    size_t length = 1;
    char is32, is_logical;
    char buffer[12];
    if (objr == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",400);
    if (objr->command != CMD_MODEND) InternalError("g:\\bld\\womp\\c\\genmsomf.c",401);
    if (state == NULL) InternalError("g:\\bld\\womp\\c\\genmsomf.c",402);
    if (state->pass != POBJ_WRITE_PASS) InternalError("g:\\bld\\womp\\c\\genmsomf.c",403);
    is32 = objr->is_32 || objr->is_phar;
    buffer[0] = objr->d.modend.main_module ? 0x80 : 0;
    if (objr->d.modend.start_addrs) {
        is_logical = objr->d.modend.is_logical ? 1 : 0;
        buffer[0] |= 0x40 | is_logical;
        length += FixGenRef(&objr->d.modend.ref,is_logical,buffer + 1,
            is32 ? FIX_GEN_MS386 : FIX_GEN_INTEL);
    }
    if (buffer[0] == 0) is32 = 0;
    ObjWriteRec(state->file_out,is32 ? CMD_MODEND + 1 : CMD_MODEND,length,buffer);
    LifixDestroy(&lifList);
    return 0;
}

/* CF1bb64..1bb76, same24-entry registration list supplied at startup. */
void GenMSOmfFini(void)
{
    PObjUnRegList(myFuncs,sizeof(myFuncs) / sizeof(myFuncs[0]));
}

/* CF1b9f0..1ba96, line-number record framing. */
int writeLinnum( obj_rec *objr, pobj_state *state )
{
    OBJ_WFILE *out;
    int is32;
    if( objr == NULL ) InternalError("g:\\bld\\womp\\c\\genmsomf.c",596);
    if( objr->command != CMD_LINNUM ) InternalError("g:\\bld\\womp\\c\\genmsomf.c",597);
    if( state == NULL ) InternalError("g:\\bld\\womp\\c\\genmsomf.c",598);
    if( state->pass != POBJ_WRITE_PASS ) InternalError("g:\\bld\\womp\\c\\genmsomf.c",599);
    out = state->file_out;
    is32 = objr->is_32 || objr->is_phar;
    ObjWBegRec( out, is32 ? CMD_LINNUM + 1 : CMD_LINNUM );
    writeBase( objr, out );
    writeLinnumData( objr, out );
    ObjWEndRec( out );
    return 0;
}

/* CF1b92c..1b9ed, little-endian line-number serialization. */
void writeLinnumData( obj_rec *objr, OBJ_WFILE *out )
{
    int is32;
    struct linnum_data *cur, *stop;
    if( objr == NULL ) InternalError("g:\\bld\\womp\\c\\genmsomf.c",550);
    if( out == NULL ) InternalError("g:\\bld\\womp\\c\\genmsomf.c",551);
    is32 = objr->is_32 || objr->is_phar;
    if( is32 ) {
        ObjWrite( out, (char *)objr->d.linnum.lines,
            6 * objr->d.linnum.num_lines );
    } else {
        cur = objr->d.linnum.lines;
        stop = cur + objr->d.linnum.num_lines;
        while( cur < stop ) {
            ObjWrite16( out, cur->number );
            if( (cur->offset & 0xffff0000UL) != 0 )
                InternalError("g:\\bld\\womp\\c\\genmsomf.c",566);
            ObjWrite16( out, (unsigned short)cur->offset );
            ++cur;
        }
    }
}
