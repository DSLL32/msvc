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


/* Adapted initial6b9cb443 segment.c; actual1994 SegmentInit
 *4bf23..4c013 has the old four-argument allocation API, no TLS/P5 setup,
 * and fixed CONST/CONST2/_DATA/_BSS plus init/fini/stack segments.
 * Attribute/control values are actual old values, not later CG enums. */
#include <stddef.h>
#include <string.h>
#include "compiler.h"
#include "cmdline.h"
#include "frontend.h"
#include "symbol.h"
#include "utility.h"
#include "pending.h"
#include <recon.h>
typedef struct pc_segment PC_SEGMENT;
#pragma pack(1)
struct pc_segment {
    PC_SEGMENT *next;
    void *label;
    unsigned short seg_id,attrs;
    unsigned align,offset,binding;
    unsigned dgroup:1,lab_gened:1,used:1,fixed_alignment:1,cg_defed:1;
    unsigned module_prefix:1,has_data:1,only_strings:1;
    int:0;
    char name[1];
};
struct seg_look {
    unsigned attrs;
    unsigned short seg_id;
    unsigned align,sym_size,sym_align;
    const char *seg_name;
    unsigned char use_seg_id:1,use_attrs:1,use_align:1,use_sym_size_align:1;
    unsigned char use_name:1,use_only_strings:1;
};
#pragma pack()
typedef char PcSegmentSize[(sizeof(PC_SEGMENT)==29)?1:-1];
typedef char PcSegmentName[(offsetof(PC_SEGMENT,name)==28)?1:-1];
typedef char SegLookSize[(sizeof(struct seg_look)==23)?1:-1];
typedef struct def_seg {
    PC_SEGMENT *pcseg;
    unsigned ctr;
    unsigned ds_used:1;
} DEF_SEG;
typedef char DefSegSize[(sizeof(DEF_SEG)==12)?1:-1];
static DEF_SEG code_def_seg,data_def_seg;
static PC_SEGMENT *seg_list;
static unsigned dgroup_size;
static unsigned short seg_max,seg_import;
PC_SEGMENT *segmentAlloc(char *name,unsigned short id,unsigned attrs,unsigned control);
PC_SEGMENT *segmentAllocRom(char *name,unsigned short id);
PC_SEGMENT *segmentDefine(char *name,unsigned short id,unsigned attrs,unsigned control);
void initDefaultDataSeg(char *name);
void *RingPrune(void **head,void *entry);
void *RingLookup(void *head,int (*compare)(void *,const void *),const void *comparand);
unsigned PragRegName(char *name);
unsigned SegmentAdjust(unsigned short id,unsigned offset,unsigned align);
void segmentCgDefine(PC_SEGMENT *segment);
void CFatal(const char *message);
static int in_back_end;
void checkSegmentOverflow(void);
int same_segment(void *element,const void *comparand);
void initDefSeg(DEF_SEG *def_seg,PC_SEGMENT *pcseg)
{
    /* Old body resets unconditionally, before the later identity check. */
    def_seg->ctr=0;
    def_seg->pcseg=pcseg;
    def_seg->ds_used=0;
}
void pruneDefSeg(DEF_SEG *def_seg)
{
    PC_SEGMENT *pcseg;
    if (!def_seg->ds_used) {
        pcseg=def_seg->pcseg;
        if (pcseg!=0) {
            RingPrune((void **)&seg_list,pcseg);
            CMemFree(pcseg);
        }
    }
}
void initDefaultCodeSeg(char *name)
{
    PC_SEGMENT *seg;
    unsigned control;
    pruneDefSeg(&code_def_seg);
    control=0;
    if (strpref(ModuleName,name)) control=2;
    seg=segmentDefine(name,0,0x87,control);
    initDefSeg(&code_def_seg,seg);
}
void SegmentInit(char *name)
{
    seg_list=0;
    seg_import=(unsigned short)-1;
    seg_max=0;
    dgroup_size=0;
    initDefaultCodeSeg(name);
    segmentAlloc("CONST",2,0x9c,1);
    segmentAllocRom("CONST2",3);
    initDefaultDataSeg("_DATA");
    segmentAlloc("_BSS",5,0x82,1);
    segmentAllocRom("XIB",6);
    segmentAllocRom("XI",7);
    segmentAllocRom("XIE",8);
    segmentAllocRom("YIB",9);
    segmentAllocRom("YI",10);
    segmentAllocRom("YIE",11);
    segmentAlloc("STACK",12,0x82,1);
}
/* Actual1994 CF4b749..4b83a: no later class/sibling or target-CPU
 * abstraction. Old initial alignments are2 for ids6..11 and4 otherwise. */
PC_SEGMENT *segmentAlloc(char *name,unsigned short id,unsigned attrs,unsigned control)
{
    PC_SEGMENT *curr;
    unsigned size;
    size=strlen(name);
    curr=RingAlloc((void **)&seg_list,sizeof(PC_SEGMENT)+size);
    stvcpy(curr->name,name,size);
    curr->offset=0;
    curr->dgroup=(control&1)!=0;
    curr->lab_gened=0;
    curr->used=0;
    curr->module_prefix=(control&2)!=0;
    curr->label=0;
    curr->binding=0;
    curr->fixed_alignment=0;
    curr->cg_defed=0;
    curr->has_data=0;
    curr->only_strings=0;
    curr->attrs=attrs;
    if (id==0) { id=++seg_max; checkSegmentOverflow(); }
    else if (id>seg_max) { seg_max=id; checkSegmentOverflow(); }
    curr->seg_id=id;
    if (id>=6 && id<=11) {
        curr->align=2;
        curr->fixed_alignment=1;
    } else {
        curr->align=4;
        if (in_back_end) segmentCgDefine(curr);
    }
    return curr;
}
PC_SEGMENT *segmentAllocRom(char *name,unsigned short id)
{ return segmentAlloc(name,id,0x8c,1); }
PC_SEGMENT *segmentDefine(char *name,unsigned short id,unsigned attrs,unsigned control)
{
    PC_SEGMENT *curr;
    struct seg_look lk;
    char *pc_reg;
    char pc_reg_name[8];
    for (pc_reg=name;; ++pc_reg) {
        if (*pc_reg==0) { pc_reg_name[0]=0; break; }
        if (*pc_reg==':') {
            stvcpy(pc_reg_name,name,pc_reg-name);
            name=pc_reg+1;
            break;
        }
    }
    lk.seg_id=id;
    lk.use_seg_id=1;
    lk.attrs=attrs;
    lk.use_attrs=1;
    lk.use_align=0;
    lk.use_sym_size_align=0;
    lk.seg_name=name;
    lk.use_name=1;
    lk.use_only_strings=0;
    curr=RingLookup(seg_list,same_segment,&lk);
    if (curr==0) {
        curr=segmentAlloc(lk.seg_name,lk.seg_id,lk.attrs,control);
        if (*pc_reg_name!=0) curr->binding=PragRegName(pc_reg_name);
    }
    return curr;
}
int same_segment(void *element,const void *comparand)
{
    PC_SEGMENT *curr;
    const struct seg_look *lk;
    unsigned align_adjust,new_offset;
    curr=element;
    lk=comparand;
    if (lk->use_seg_id && lk->seg_id!=0 && curr->seg_id!=lk->seg_id) return 0;
    if (lk->use_attrs && curr->attrs!=lk->attrs) return 0;
    if (lk->use_align && curr->align!=lk->align) return 0;
    if (lk->use_sym_size_align) {
        align_adjust=SegmentAdjust(curr->seg_id,curr->offset,lk->sym_align);
        new_offset=curr->offset+align_adjust+lk->sym_size;
        if (new_offset<curr->offset) new_offset=0;
        if (new_offset==0) return 0;
    }
    if (lk->use_name && strcmp(curr->name,lk->seg_name)!=0) return 0;
    if (lk->use_only_strings && !curr->only_strings) return 0;
    return 1;
}
/* The old limit compares import and local unsigned ids directly. */
void checkSegmentOverflow(void)
{
    if (seg_import<=seg_max) CFatal("Too many segments -- sub-divide source module");
}
unsigned SegmentAdjust(unsigned short id,unsigned offset,unsigned align)
{
    unsigned rounded;
    if(id>=6 && id<=11) return 0;
    rounded=(offset+align-1)&~(align-1);
    return rounded<offset?0:rounded-offset;
}
void segmentCgDefine(PC_SEGMENT *segment) { (void)segment; STUB_TRAP(); }
/* Old data-default body mirrors the code-default selection, but uses
 * DGROUP control1/3 and writable initialized data attributes086. */
void initDefaultDataSeg(char *name)
{
    PC_SEGMENT *seg;
    unsigned control;
    pruneDefSeg(&data_def_seg);
    control=1;
    if (strpref(ModuleName,name)) control=3;
    seg=segmentDefine(name,0,0x86,control);
    initDefSeg(&data_def_seg,seg);
}

TYPE BasedType(TYPE type);
unsigned short SegmentFindBased(TYPE type) { (void)type; STUB_TRAP(); return 0; }
unsigned short nextZmSegment(void) { STUB_TRAP(); return 0; }
static unsigned short seg_code_comdat;
int same_seg_id(void *element,const void *id)
{ return ((PC_SEGMENT *)element)->seg_id==*(const unsigned short *)id; }
PC_SEGMENT *segIdLookup(unsigned short id)
{ return RingLookup(seg_list,same_seg_id,&id); }
void SegmentMarkUsed(unsigned short id)
{
    PC_SEGMENT *segment;
    if (id!=0) {
        segment=segIdLookup(id);
        if (segment) segment->used=1;
    }
}
unsigned short SegmentForDefinedFunc(SYMBOL sym)
{
    unsigned short id;
    TYPE type;
    id=sym->segid;
    if (id==0) {
        type=FunctionDeclarationType(sym->sym_type);
        if (BasedType(sym->sym_type)) id=SegmentFindBased(sym->sym_type);
        else if (CompFlags.zm_switch_used) {
            if (type->flag&0x808) {
                id=seg_code_comdat;
                if (id==0) {
                    id=nextZmSegment();
                    seg_code_comdat=id;
                }
            } else id=nextZmSegment();
        } else {
            id=code_def_seg.pcseg->seg_id;
            SegmentMarkUsed(id);
            code_def_seg.ds_used=1;
        }
    }
    return id;
}
unsigned short SegmentImport(void)
{
    --seg_import;
    checkSegmentOverflow();
    return seg_import;
}
unsigned short SegmentDefaultCode(void)
{
    code_def_seg.ds_used=1;
    SegmentMarkUsed(code_def_seg.pcseg->seg_id);
    return code_def_seg.pcseg->seg_id;
}

unsigned CgMemorySize(TYPE type);
TYPE AlignmentType(TYPE type);
unsigned PackAlignment(unsigned packing,unsigned align);
unsigned segmentTypeSize(TYPE type)
{
    unsigned size=CgMemorySize(type);
    return size?size:1;
}
unsigned SegmentAlignment(SYMBOL sym)
{
    unsigned align;
    TYPE type;
    if((int)OptSize>50 && PackAmount==1) return 1;
    type=sym->sym_type;
    align=segmentTypeSize(AlignmentType(type));
    if(align==1) align=segmentTypeSize(type);
    return PackAlignment(4,align);
}
void accumAlignment(PC_SEGMENT *segment,unsigned align)
{
    if(!segment->fixed_alignment && align>segment->align)
        segment->align=align;
}
int SymIsInitialized(SYMBOL sym);
int SymIsExtern(SYMBOL sym);
void __cdecl CErr(unsigned number,...);
unsigned short SegmentAddSym(SYMBOL sym,unsigned short id,unsigned size,unsigned align)
{
    PC_SEGMENT *segment;
    unsigned adjusted,calculated,total;
    if(id==4) {
        segment=data_def_seg.pcseg;
        id=segment->seg_id;
    } else if(id==1) {
        segment=code_def_seg.pcseg;
        id=segment->seg_id;
    } else segment=segIdLookup(id);
    if(!segment) {
        CFatal("segment: cannot find default segment");
        return id;
    }
    accumAlignment(segment,align);
    if(SymIsInitialized(sym) || !SymIsExtern(sym)) {
        adjusted=SegmentAdjust(segment->seg_id,segment->offset,align);
        calculated=segment->offset+adjusted+size;
        if(calculated<segment->offset) calculated=0;
        if(calculated==0) {
            if(size!=0) CErr(439,segment->name,sym);
            return 0;
        }
        if(segment->dgroup) {
            total=dgroup_size+size+adjusted;
            if(total<dgroup_size) calculated=0;
            if(calculated==0) {
                if(size!=0) CErr(461,sym,segment->name);
                return 0;
            }
            dgroup_size+=size+adjusted;
        }
        segment->offset=calculated;
    }
    segment->used=1;
    segment->has_data=1;
    data_def_seg.ds_used=1;
    return segment->seg_id;
}

/* The old entry has only the real code-segment used-bit pass; it predates
 * the later default-segment resets and debug-support requests. */
void SegmentCodeCgInit(void)
{
    PC_SEGMENT *segment,*last=seg_list;
    if (last) {
        segment=last->next;
        for (;;) {
            if ((segment->attrs&1) && !segment->has_data) segment->used=0;
            if (segment==last) break;
            segment=segment->next;
        }
    }
}

unsigned SegmentBoundReg(unsigned short segment)
{ (void)segment;STUB_TRAP();return 0; }
