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

/* Recovered CFf284..f469 pass driver. Released bld/wasm/c/write.c,
 * commit6b9cb4438959d845a2b4dd4bf19f7908ecd608e9 supplies names;
 * the1994 body has no later OnePass/AsmEvalInit wrappers and saves fpos4B.
 * Globals below are the encountered fields and original phase counters. */
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "asminit.h"
#include "direct.h"
#include "asmparse.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen,strcpy,memcpy)
#endif

typedef char wasm_fpos_size_check[sizeof(fpos_t) == 4 ? 1 : -1];

int MacroLocalVarCounter = 0;
char EndDirectiveFound = 0;
global_vars Globals = { 0, 0, 0 };
/* CF27270/27274: pass0 total and current nonzero-pass total. */
unsigned long PassTotal[2];
signed char PhaseError;
char write_to_file, Modend;
signed char Use32, DefineProc; /* CF27285, CF27287 respectively. */
struct fixup *FixupListHead, *FixupListTail;
obj_rec *ModendRec;
dir_node *CurrProc;

/* CFde70..dedf; the older driver also initializes its pass totals. */
void write_init(void)
{
    BufSize = 0;
    PassTotal[0] = PassTotal[1] = 0;
    FixupListTail = FixupListHead = NULL;
    ModendRec = NULL;
    CurrProc = NULL;
    write_to_file = 1;
    Modend = 0;
    Use32 = 0;
    DefineProc = 0;
    IdxInit();
    LnameInsert("");
    ModuleInit();
    FixInit();
    AssumeInit();
}


void WriteObjModule(void)
{
    char codebuf[1012], string[256];
    fpos_t start_pos;
    CodeBuffer = codebuf;
    PhaseError = 0;
    if (fgetpos(AsmFiles.file[ASM], &start_pos) != 0) Fatal(3);
    write_init();
    Parse_Pass = 0;
    while ((unsigned char)Parse_Pass <= 1 || PhaseError) {
        PhaseError = 0;
        LineNumber = 0;
        while (ScanLine(string, sizeof(string)) != NULL) {
            AsmLine(string);
            if (EndDirectiveFound && Options.stop_at_end) break;
        }
        if (!EndDirectiveFound) {
            AsmError(1080); /* END_DIRECTIVE_REQUIRED */
            break;
        }
        EndDirectiveFound = 0;
        if (!write_to_file || Options.error_count > 0) break;
        if (Parse_Pass == 0) {
            writepass1stuff();
            if (!write_to_file || Options.error_count > 0) break;
        } else {
            if (!PhaseError && PassTotal[0] == PassTotal[1]) {
                PhaseError = 0;
                write_modend();
                break;
            }
            ObjWriteClose(pobjState.file_out);
            pobjState.file_out = ObjWriteOpen(AsmFiles.fname[OBJ]);
            if (pobjState.file_out == NULL) Fatal(1, AsmFiles.fname[OBJ]);
            writepass1stuff();
            Modend = 0;
            PhaseError = 1;
        }
        if (fsetpos(AsmFiles.file[ASM], &start_pos) != 0) Fatal(4);
        if (Parse_Pass != 0) {
            PassTotal[0] = PassTotal[1];
            PassTotal[1] = 0;
        }
        reset_seg_len();
        BufSize = 0;
        MacroLocalVarCounter = 0;
        Globals.data_in_code = 0;
        PrepAnonLabels();
        ++Parse_Pass;
    }
    AsmSymFini();
    FreeIncludePath();
    /* The tiny later write_fini wrapper is folded into this direct edge. */
    FixFini();
}

/* CFf0e0..f1a4 and its seven-entry switch table atf0c4..f0e0. */
void FlushCurrSeg(void)
{
    unsigned size;
    if (CurrSeg == NULL) return;
    if (Options.output_data_in_code_records && Globals.data_in_code &&
        CurrSeg->seg->e.seginfo->segrec->d.segdef.class_name_idx ==
            FindClassLnameIdx("code")) {
        Globals.data_in_code = 0;
        OutSelect(0);
        return;
    }
    if (FixupListTail != NULL) {
        switch (FixupListTail->loc_method) {
        case FIX_LO_BYTE:
        case FIX_HI_BYTE: size = 1; break;
        case FIX_OFFSET:
        case FIX_BASE: size = 2; break;
        case FIX_POINTER:
        case FIX_OFFSET386: size = 4; break;
        case FIX_POINTER386: size = 6; break;
        }
        if (FixupListTail->loc_offset + size > BufSize) return;
    }
    write_ledata();
    BufSize = 0;
}

/* CFec58..ed1e;1994 uses current_loc-BufSize and advances start_loc to
 * the recorded segment length. The later explicit LEDATA is_32 test is absent. */
void write_ledata(void)
{
    obj_rec *record;
    if (BufSize > 0) {
        record = ObjNewRec(CMD_LEDATA);
        ObjAttachData(record, CodeBuffer, BufSize);
        record->d.ledata.idx = CurrSeg->seg->e.seginfo->segrec->d.segdef.idx;
        record->d.ledata.offset = CurrSeg->seg->e.seginfo->current_loc - BufSize;
        CurrSeg->seg->e.seginfo->start_loc =
            CurrSeg->seg->e.seginfo->segrec->d.segdef.seg_length;
        write_record(record, 1);
        if (FixupListHead != NULL) {
            record = ObjNewRec(CMD_FIXUP);
            record->d.fixup.fixup = FixupListHead;
            check_need_32bit(record);
            write_record(record, 1);
            FixupListHead = FixupListTail = NULL;
        }
        if (Options.debug_flag) write_linnum();
    }
}

/* CFf1ec..f24c; final write_end_of_pass1 is an original fallthrough. */
void writepass1stuff(void)
{
    if (CurrProc != NULL) { AsmError(1057); return; }
    write_header();
    if (Globals.dosseg) write_dosseg();
    write_lib();
    write_lnames();
    write_seg();
    write_grp();
    write_global();
    write_ext();
    write_comm();
    write_alias();
    if (write_pub() == ERROR) return;
    write_export();
    write_end_of_pass1();
}

/* CFea68..eb05. Unlike the released2002 version,1994 uses the
 * source spelling directly and does not call _fullpath. */
void write_header(void)
{
    obj_rec *objr;
    unsigned len;
    char *name;
    objr = ObjNewRec(CMD_THEADR);
    name = Options.module_name != NULL ? Options.module_name : AsmFiles.fname[ASM];
    objr->is_32 = Use32;
    len = strlen(name);
    ObjAllocData(objr,(unsigned short)(len + 1));
    ObjPutName(objr,(const unsigned char *)name,(unsigned char)len);
    objr->length = objr->curoff; /* original inlined ObjTruncRec */
    write_record(objr,TRUE);
}

/* CFdee8..df37, dispatch through the existing registered OMF writer table. */
void write_record(obj_rec *objr, unsigned char kill)
{
    if (objr == NULL) InternalError("..\\c\\write.c",144);
    objr->curoff = 0; /* original inlined ObjRSeek */
    jumpTable[objr->command - 0x6c](objr,&pobjState);
    if (kill) ObjKillRec(objr);
}

/* CFe060..e0b9. */
void write_lib(void)
{
    obj_rec *objr;
    dir_node *curr;
    char *name;
    for (curr = Tables[TAB_LIB].head; curr != NULL; curr = curr->next) {
        name = curr->sym.name;
        objr = ObjNewRec(CMD_COMENT);
        objr->d.coment.attr = 0x80;
        objr->d.coment.class = 0x9f; /* CMT_DEFAULT_LIBRARY */
        ObjAttachData(objr,name,(unsigned short)strlen(name));
        write_record(objr,TRUE);
    }
}

/* CFe3c4..e519.1994 uppercases every copied LNAMES spelling;
 * the corresponding2002 strupr is disabled there and is restored here. */
void write_lnames(void)
{
    obj_rec *objr;
    unsigned total_size = 0, total_num = 0, i = 0, len;
    char *lname = NULL;
    dir_node *dir;
    queuenode *curr;
    if (LnameQueue == NULL) return;
    objr = ObjNewRec(CMD_LNAMES);
    objr->d.lnames.first_idx = 1;
    objr->d.lnames.num_names = LnamesIdx;
    for (curr = LnameQueue->head; curr != NULL; curr = curr->next) {
        dir = (dir_node *)curr->data;
        if (dir == NULL) InternalError("..\\c\\write.c",376);
        total_size += 1 + strlen(dir->sym.name);
        ++total_num;
    }
    if (total_size > 0) {
        lname = AsmAlloc(total_size + 1);
        for (curr = LnameQueue->head; curr != NULL; curr = curr->next) {
            dir = (dir_node *)curr->data;
            len = strlen(dir->sym.name);
            lname[i++] = (char)len;
            strcpy(lname+i,dir->sym.name);
            strupr(lname+i);
            i += len;
        }
        ObjAttachData(objr,lname,(unsigned short)total_size);
    }
    objr->free_data = 1;
    if (total_num != LnamesIdx) InternalError("..\\c\\write.c",397);
    if (total_num == 0) return;
    write_record(objr,TRUE);
}

/* CFe33c..e3c2.1994 assumes each counted segment index exists;
 * its later NULL check/reset is not added to this original logic. */
void write_seg(void)
{
    dir_node *curr;
    obj_rec *objr;
    unsigned seg_index, total_segs = 0;
    for (curr = Tables[TAB_SEG].head; curr != NULL; curr = curr->next)
        ++total_segs;
    curr = Tables[TAB_SEG].head;
    for (seg_index = 1; seg_index <= total_segs; ++seg_index) {
        if (curr->sym.segidx != seg_index) {
            for (curr = Tables[TAB_SEG].head; curr != NULL; curr = curr->next)
                if (curr->sym.segidx == seg_index) break;
        }
        if (curr->sym.state != SYM_SEG) { AsmError(1019); continue; }
        objr = curr->e.seginfo->segrec;
        objr->is_32 = TRUE;
        objr->d.segdef.ovl_name_idx = 1;
        objr->d.segdef.seg_name_idx = GetLnameIdx(curr->sym.name);
        write_record(objr,FALSE);
    }
}

/* CFe1f4..e339.1994 inserts the separately numbered FLAT group here,
 * before a normal group or after the group list, unlike the2002 source. */
void write_grp(void)
{
    dir_node *curr, *segment;
    seg_list *seg;
    obj_rec *grp;
    unsigned line = LineNumber, i = 1;
    for (curr = Tables[TAB_GRP].head; curr != NULL; curr = curr->next, ++i) {
        if (ModuleInfo.model == MOD_FLAT && i == ModuleInfo.flat_idx) {
            write_flat_grp();
            ++i;
        }
        grp = ObjNewRec(CMD_GRPDEF);
        if (grp == NULL) InternalError("..\\c\\write.c",295);
        grp->d.grpdef.idx = curr->e.grpinfo->idx;
        ObjAllocData(grp,(unsigned short)(1 + 3 * curr->e.grpinfo->numseg));
        ObjPut8(grp,(unsigned char)GetLnameIdx(curr->sym.name));
        for (seg = curr->e.grpinfo->seglist; seg != NULL; seg = seg->next) {
            segment = seg->seg;
            if (segment->sym.state != SYM_SEG) {
                LineNumber = curr->line;
                AsmError(1019);
                write_to_file = FALSE;
                LineNumber = line;
            } else {
                ObjPut8(grp,0xff); /* GRP_SEGIDX */
                ObjPutIndex(grp,segment->e.seginfo->segrec->d.segdef.idx);
            }
        }
        if (write_to_file) {
            grp->length = grp->curoff;
            write_record(grp,TRUE);
        }
    }
    if (ModuleInfo.model == MOD_FLAT && i == ModuleInfo.flat_idx)
        write_flat_grp();
}

/* CF26634, shared with the pending GLOBAL directive. */
qdesc *GlobalQueue;
/* CFe51c..e5ba. The1994 undefined branch reloads the old symbol's
 * mangler after MakeExtern and restores its segment/group fields there. */
void write_global(void)
{
    queuenode *curr;
    asm_sym *sym, *new;
    unsigned seg, grp;
    if (GlobalQueue == NULL) return;
    for (;;) {
        curr = QDequeue(GlobalQueue);
        if (curr == NULL) break;
        sym = curr->data;
        if (sym->state == SYM_UNDEFINED) {
            seg = sym->segidx;
            grp = sym->grpidx;
            new = MakeExtern(sym->name,sym->mem_type,TRUE);
            new->mangler = sym->mangler;
            sym->segidx = seg;
            sym->grpidx = grp;
            AsmFree(curr);
        } else {
            sym->public = TRUE;
            if (PubQueue == NULL) {
                PubQueue = AsmAlloc(sizeof(qdesc));
                QInit(PubQueue);
            }
            QEnqueue(PubQueue,curr);
        }
    }
    AsmFree(GlobalQueue);
    GlobalQueue = NULL;
}

/* CFe5bc..e6f7; each EXTDEF borrows this call's1024-byte name buffer. */
void write_ext(void)
{
    obj_rec *objr;
    dir_node *curr, *next = Tables[TAB_EXT].head;
    unsigned num, total_size, i, len;
    char name[1024], buffer[256];
    for (;;) {
        if (next == NULL) break;
        num = total_size = i = 0;
        objr = ObjNewRec(CMD_EXTDEF);
        objr->d.extdef.first_idx = 0;
        for (curr = next; curr != NULL; curr = curr->next) {
            Mangle(&curr->sym,buffer);
            len = strlen(buffer);
            if (total_size + len >= 1024) break;
            total_size += len + 2;
            ++num;
            name[i++] = (char)len;
            memcpy(name+i,buffer,len);
            i += len;
            name[i++] = 0;
        }
        next = curr;
        ObjAttachData(objr,name,(unsigned short)total_size);
        objr->d.extdef.num_names = num;
        if (objr->d.extdef.num_names == 0) return;
        write_record(objr,TRUE);
    }
}

/* CFe7d4..ea66, actual five-way leaf table CFe7c0..e7d4. */
void write_comm( void )
/****************************/
{
    obj_rec     *objr;
    dir_node    *curr;
    unsigned    num = 0;
    unsigned    total_size = 0;
    unsigned    varsize = 0;
    unsigned    mem_type;
    unsigned    i = 0;
    unsigned    j = 0;
    char        *name;
    unsigned    len;
    unsigned long value;
    char        buffer[256];
    char        *ptr;


    objr = ObjNewRec( CMD_COMDEF );
    objr->d.comdef.first_idx = 0;

    for( curr = Tables[TAB_COMM].head; ; curr = curr->next ) {
        if( curr == NULL ) break;
        ptr = Mangle( &curr->sym, buffer );
        total_size += 3 + strlen( ptr );
        /* 3 = 1 for string len + 1 for type index + 1 for data type */

        varsize = opsize( curr->sym.mem_type );
        if( curr->e.comminfo->distance == T_FAR ) {
            total_size += get_number_of_bytes_for_size_in_commdef( varsize );
            total_size += get_number_of_bytes_for_size_in_commdef( curr->e.comminfo->size );
        } else {
            total_size += get_number_of_bytes_for_size_in_commdef( curr->e.comminfo->size );
        }
        num++;
    }

    if( total_size > 0 ) {
        name = AsmAlloc( total_size * sizeof( char ) );
        for( curr = Tables[TAB_COMM].head; ; curr = curr->next ) {
            if( curr == NULL ) break;
            ptr = Mangle( &curr->sym, buffer );
            len = strlen( ptr );
            name[i] = (char)len;
            i++;
            memcpy( name+i, ptr, len );
            i += len;
            name[i++] = 0;      // for the type index

            /* now add the data type & communal length */
            if( curr->e.comminfo->distance == T_FAR ) {
                name[i++] = 0x61;
            } else {
                name[i++] = 0x62;
            }

            value = curr->e.comminfo->size;
            varsize = get_number_of_bytes_for_size_in_commdef( value );
            switch( varsize ) {
            case 1:
                break;
            case 3:
                name[i++] = 0x81;
                break;
            case 4:
                name[i++] = 0x84;
                break;
            case 5:
                name[i++] = 0x88;
                break;
            }
            if( varsize > 1 ) varsize--; /* we already output 1 byte */

            mem_type = opsize( curr->sym.mem_type );
            if( curr->e.comminfo->distance != T_FAR ) {
                value *= mem_type;
            }

            for( j=0; j < varsize; j++ ) {
                name[i++] = value % ( 255 + 1 );
                value >>= 8;
            }

            if( curr->e.comminfo->distance == T_FAR ) {
                /* mem type always needs <= 1 byte */
                if (mem_type >= 255) InternalError("..\\c\\write.c",621);
                name[i++] = mem_type;
            }
        }
        ObjAttachData( objr, name, total_size );
    }

    objr->d.comdef.num_names = num;
    objr->free_data = 1;
    if( objr->d.extdef.num_names == 0 ) return;
    write_record( objr, TRUE );
}


/* CF26638, the still-pending ALIAS directive owns the paired strings. */
qdesc *AliasQueue;
/* CFed7c..eea6.1994 uses checked stack alloca, not later AsmTmpAlloc.
 * Its length fields are zero-extended8-bit values. */
void write_alias(void)
{
    obj_rec *objr;
    queuenode *curr;
    char *alias, *subst, *new;
    unsigned char len1, len2;
    if (AliasQueue == NULL) return;
    for (curr = AliasQueue->head; curr != NULL; curr = curr->next) {
        alias = curr->data;
        subst = alias + strlen(alias) + 1;
        len1 = strlen(alias);
        len2 = strlen(subst);
        new = alloca(len1 + len2 + 2);
        *new++ = len1;
        strcpy(new,alias);
        new += len1;
        *new++ = len2;
        strcpy(new,subst);
        new -= len1 + 2;
        objr = ObjNewRec(CMD_ALIAS);
        ObjAttachData(objr,new,(unsigned short)(len1 + len2 + 2));
        write_record(objr,TRUE);
    }
}

/* CF27278: names borrowed by NameGet while a PUBDEF is serialized. */
char **NameArray;
/* CFeea8..f0aa.1994 tests PubQueue before adding public procedures and
 * does not include the later offset>ffff promotion of the record width. */
int write_pub(void)
{
    queuenode *curr, *first;
    asm_sym *sym;
    dir_node *pub;
    obj_rec *objr;
    pubdef_data *data;
    unsigned i, count, seg, grp, static_def;
    if (PubQueue == NULL) return NOT_ERROR;
    put_public_procs_in_public_table();
    for (first = PubQueue->head; first != NULL; first = curr) {
        static_def = FALSE;
        sym = first->data;
        if (sym->state == SYM_UNDEFINED) {
            AsmErr(1148,sym->name);
            return ERROR;
        }
        seg = sym->segidx;
        grp = sym->grpidx;
        for (curr = first, count = 0; curr != NULL; curr = curr->next) {
            if (count == 100) break;
            sym = curr->data;
            if (sym->segidx != seg) break;
            if (sym->state == SYM_PROC &&
                ((dir_node *)sym)->e.procinfo->visibility == 0 && !static_def) {
                if (curr != first) break;
                static_def = TRUE;
            }
            ++count;
        }
        NameArray = AsmAlloc(count * sizeof(char *));
        for (i = 0; i < count; ++i) NameArray[i] = NULL;
        objr = ObjNewRec(static_def ? CMD_STATIC_PUBDEF : CMD_PUBDEF);
        objr->d.pubdef.base.grp_idx = grp;
        objr->d.pubdef.base.seg_idx = seg;
        objr->d.pubdef.base.frame = 0;
        data = AsmAlloc(count * sizeof(pubdef_data));
        for (curr = first, i = 0; i < count; ++i, curr = curr->next) {
            sym = curr->data;
            if (sym->segidx != seg) break;
            NameArray[i] = Mangle(sym,NULL);
            data[i].name = i;
            if (sym->state != SYM_CONST) data[i].offset = sym->offset;
            else {
                pub = (dir_node *)sym;
                if (pub->e.constinfo->data[0].token != T_NUM) {
                    AsmWarn(2,1119);
                    data[i].offset = 0;
                } else data[i].offset = pub->e.constinfo->data[0].value;
            }
            data[i].type.idx = 0;
        }
        objr->d.pubdef.num_pubs = count;
        objr->d.pubdef.pubs = data;
        objr->d.pubdef.free_pubs = TRUE;
        write_record(objr,TRUE);
        for (i = 0; i < count; ++i) {
            if (NameArray[i] != NULL) AsmFree(NameArray[i]);
        }
        AsmFree(NameArray);
    }
    return NOT_ERROR;
}

/* CFed20..ed7b, queue each procedure at most once by its public bit. */
void put_public_procs_in_public_table(void)
{
    dir_node *proc;
    queuenode *qnode;
    for (proc = Tables[TAB_PROC].head; proc != NULL; proc = proc->next) {
        if (!proc->sym.public) {
            proc->sym.public = TRUE;
            qnode = AsmAlloc(sizeof(queuenode));
            qnode->data = proc;
            if (PubQueue == NULL) {
                PubQueue = AsmAlloc(sizeof(qdesc));
                QInit(PubQueue);
            }
            QEnqueue(PubQueue,qnode);
        }
    }
}

/* CFf0ac..f0c3, direct16-bit handle into the current public-name owner. */
const char *NameGet(unsigned short handle)
{
    return NameArray[handle];
}

/* CFe0bc..e186. write_one_export is inlined in this1994 body. */
void write_export(void)
{
    dir_node *dir;
    obj_rec *objr;
    char *name;
    for (dir = Tables[TAB_PROC].head; dir != NULL; dir = dir->next) {
        if (dir->e.procinfo->visibility == 2) { /* VIS_EXPORT */
            objr = ObjNewRec(CMD_COMENT);
            objr->d.coment.attr = 0;
            objr->d.coment.class = 0xa0; /* CMT_DLL_ENTRY */
            name = Mangle(&dir->sym,NULL);
            ObjAllocData(objr,(unsigned short)(4 + strlen(name)));
            ObjPut8(objr,2);
            ObjPut8(objr,0);
            ObjPutName(objr,(const unsigned char *)name,(unsigned char)strlen(name));
            ObjPut8(objr,0);
            write_record(objr,TRUE);
            AsmFree(name);
        }
    }
}

/* CFf24c..f282, a borrowed one-byte OMF pass marker. */
void write_end_of_pass1(void)
{
    obj_rec *objr = ObjNewRec(CMD_COMENT);
    objr->d.coment.attr = 0;
    objr->d.coment.class = 0xa2; /* CMT_MS_END_PASS_1 */
    ObjAttachData(objr,"\001",1);
    write_record(objr,TRUE);
}

/* CFf1a4..f1ea. Its terminalRET byte is shared with writepass1stuff. */
void reset_seg_len(void)
{
    dir_node *curr;
    for (curr = Tables[TAB_SEG].head; curr != NULL; curr = curr->next) {
        if (curr->sym.state != SYM_SEG) { AsmError(1019); continue; }
        curr->e.seginfo->segrec->d.segdef.seg_length = 0;
        curr->e.seginfo->start_loc = 0;
        curr->e.seginfo->current_loc = 0;
    }
}

/* CFec1c..ec55, relocation-width selection from the actual linked owners. */
void check_need_32bit(obj_rec *record)
{
    fixup *fix;
    for (fix = record->d.fixup.fixup; fix != NULL; fix = fix->next) {
        switch (fix->loc_method) {
        case FIX_OFFSET386:
        case FIX_POINTER386: record->is_32 = 1; break;
        }
        if ((unsigned long)fix->lr.target_offset > 0xffffUL) record->is_32 = 1;
        if (record->is_32) break;
    }
}

/* CFeb08..eb37; consume the single module-end record. */
int write_modend(void)
{
    if (ModendRec == NULL) {
        AsmError(135);
        return ERROR;
    }
    write_record(ModendRec,TRUE);
    return NOT_ERROR;
}

/* CFe02c..e05f: the standard DOS segment-order comment record. */
void write_dosseg( void )
{
    obj_rec *objr;
    objr = ObjNewRec( CMD_COMENT );
    objr->d.coment.attr = 0x80;
    objr->d.coment.class = 0x9e;
    ObjAttachData( objr, "", 0 );
    write_record( objr, TRUE );
}

/* CFe6f8..e793: memory width used by communal object records. */
int opsize( unsigned mem_type )
{
    switch( mem_type ) {
    case T_SBYTE: case T_BYTE: return 1;
    case T_SWORD: case T_WORD: return 2;
    case T_SDWORD: case T_DWORD: return 4;
    case T_FWORD: case T_PWORD: return 6;
    case T_QWORD: return 8;
    case T_TBYTE: return 10;
    default: return 0;
    }
}

/* CFe794..e7bf. The original accepts128 in one byte and actually compares
 * against21 in the third branch, unlike the later source's24-bit maximum.
 * That branch cannot be reached after the <=65535 test; retain the old test. */
int get_number_of_bytes_for_size_in_commdef( unsigned long value )
{
    if( value <= 128 ) return 1;
    else if( value <= 65535UL ) return 3;
    else if( value <= 21 ) return 4;
    else return 5;
}

/* CFeb5c..ec19. The older body always emits16-bit LINNUM records;
 * the later per-offset need_32 test is absent. */
void write_linnum( void )
{
    queuenode *node;
    struct linnum_data *ldata, *next;
    int count, i;
    obj_rec *objr;

    count = QCount( LinnumQueue );
    if( count == 0 ) return;
    ldata = AsmAlloc( count * sizeof(struct linnum_data) );
    for( i = 0; i < count; i++ ) {
        node = QDequeue( LinnumQueue );
        next = node->data;
        ldata[i].number = next->number;
        ldata[i].offset = next->offset;
        AsmFree( next );
        AsmFree( node );
    }
    AsmFree( LinnumQueue );
    LinnumQueue = NULL;
    objr = ObjNewRec( CMD_LINNUM );
    objr->d.linnum.num_lines = count;
    objr->d.linnum.lines = ldata;
    objr->d.linnum.d.base.grp_idx = GetCurrGrp();
    objr->d.linnum.d.base.seg_idx = CurrSeg->seg->e.seginfo->segrec->d.segdef.idx;
    objr->d.linnum.d.base.frame = 0;
    write_record( objr, TRUE );
}

/* CFeb38..eb5b, the writer's queue count and overflow sentinel. */
long QCount( qdesc *queue )
{
    long count = 0;
    queuenode *curr;
    if( queue == NULL ) return 0;
    for( curr = queue->head; curr != NULL; curr = curr->next ) {
        count++;
        if( count < 0 ) return -1;
    }
    return count;
}
