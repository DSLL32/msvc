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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


#include <stddef.h>
#include <string.h>
#include "memory.h"
#include "compiler.h"
#include "symbols.h"
#include "data.h"
#include "options.h"
#include "declarations.h"
#include "codegen.h"
#include "macros.h"
#include <recon.h>

unsigned NextFilePage;

#pragma pack(1)
struct segment_list {
    struct segment_list *next_segment;
    int segment_number;
    unsigned short size_left;
};
#pragma pack()
extern struct segment_list *SegListHead;

void SetSegAlign(SYMPTR sym)
{
    unsigned segment,align;
    segment=sym->u.var.segment;
    if (segment<11 && OptSize==0) {
        align=GetTypeAlignment(sym->sym_type);
        if (align>SegAlignment[segment]) SegAlignment[segment]=align;
    }
}

unsigned FESymAttr(SYMPTR sym)
{
    unsigned attr;
    attr=0;
    if (sym->stg_class==0x73) attr=0x0e;
    if (sym->flags&1) attr|=3;
    if (sym->flags&0x2000) attr|=0x420;
    if (sym->stg_class==0) attr|=6;
    else if (sym->stg_class==0x61) attr|=0x0e;
    else if (sym->stg_class==0x5d) attr|=0x42;
    if (sym->level!=0) attr|=0x1000;
    if (sym->sym_type->decl_flags&2) attr|=0x20;
    else if (sym->sym_type->decl_flags&1) attr|=0x10;
    if (CompFlags.unique_functions && ((attr&4) || (sym->flags&0x80))) attr|=0x100;
    return attr;
}

int SymSegId(SYMPTR sym)
{
    unsigned attr;
    attr=FESymAttr(sym);
    if (attr&1) return 1;
    if (IsConstType(sym->sym_type)) return 3;
    return 4;
}

void SetSegment(SYMPTR sym)
{
    struct segment_list *seg;
    unsigned long size;
    if ((sym->attrib&8) || (TargetSwitches&0x20)) {
        if ((sym->sym_type->decl_flags&3)==1 ||
            (sym->stg_class==0x5d && (sym->flags&0x20))) {
            sym->u.var.segment=1;
            return;
        }
    }
    if (sym->attrib&0x18) {
        size=SizeOfArg(sym->sym_type);
        seg=SegListHead;
        if (seg==NULL) {
            if (SegListHead==NULL) {
                seg=(struct segment_list *)CMemAlloc(sizeof(struct segment_list));
                SegListHead=seg;
            } else {
                for (seg=SegListHead;seg->next_segment;seg=seg->next_segment) ;
                seg->next_segment=(struct segment_list *)CMemAlloc(sizeof(struct segment_list));
                seg=seg->next_segment;
            }
            seg->segment_number=SegmentNum++;
        } else seg->size_left-=size;
        sym->u.var.segment=seg->segment_number;
    } else sym->u.var.segment=SymSegId(sym);
}

SEGADDR_T AllocSegment(struct seg_info *segment)
{
    segment->in_farmem=0;
    segment->in_page_file=0;
    segment->index=(SEGADDR_T)FEmalloc(0x4000);
    if (segment->index!=0) {
        segment->in_farmem=1;
        segment->allocated=1;
        return segment->index;
    }
    if (PageHandle==-1) OpenPageFile();
    segment->index=NextFilePage;
    segment->in_page_file=1;
    segment->allocated=1;
    ++NextFilePage;
    return 0;
}

SEGADDR_T AccessSegment(struct seg_info *segment)
{
    if (!segment->allocated) AllocSegment(segment);
    if (segment->in_farmem) return segment->index;
    return 0;
}

unsigned SegAlignment[11];
void SegInit(void)
{
    __STOSD(SegAlignment,4,11);
}

extern int SegImport;
void AssignSeg(SYMPTR sym)
{
    SetFarHuge(sym,1);
    if (sym->stg_class!=SC_EXTERN) {
        if (!(sym->flags&0x10)) {
            if (sym->u.var.segment==0) SetSegment(sym);
            if (sym->u.var.segment==4) {
                CompFlags.bss_segment_used=1;
                sym->u.var.segment=9;
            }
            SetSegAlign(sym);
        }
    } else if (sym->attrib&0x18) sym->u.var.segment=SegImport--;
}

#pragma pack(1)
struct user_seg {
    struct user_seg *next;
    char *name;
    SYM_HANDLE sym_handle;
    int segtype,segment;
    char *class_name;
    hw_reg_set pegged_register;
    unsigned char used;
};
struct textsegment {
    struct textsegment *next;
    int segment_number,index;
    char segname[1];
};
#pragma pack()
typedef char old_user_segment[(offsetof(struct user_seg,segment)==14)?1:-1];
typedef char old_user_class[(offsetof(struct user_seg,class_name)==18)?1:-1];
struct user_seg *UserSegments;
struct textsegment *TextSegList;

void SetSegs(void)
{
    int segment;
    struct user_seg *user;
    struct textsegment *text;
    unsigned flags,alignment;
    char *name;
    CompFlags.low_on_memory_printed=0;
    flags=7;
    if (*TextSegName=='\0') name="_TEXT";
    else { name=TextSegName;flags=0x87; }
    alignment=OptSize==0 ? BETypeLength(T_INTEGER):1;
    BEDefSeg(1,flags,name,alignment);
    BEDefSeg(2,0x1c,"CONST",SegAlignment[2]);
    BEDefSeg(3,0x0c,"CONST2",SegAlignment[3]);
    BEDefSeg(4,6,"_DATA",SegAlignment[4]);
    if (CompFlags.ec_switch_used) {
        BEDefSeg(5,6,"YIB",2);
        BEDefSeg(6,6,"YI",2);
        BEDefSeg(7,6,"YIE",2);
        BEDefSeg(8,6,"YC",2);
    }
    if (CompFlags.bss_segment_used) BEDefSeg(9,2,"_BSS",SegAlignment[9]);
    /* This old version reserves the string segment by incrementing directly. */
    if (CompFlags.far_strings) ++SegmentNum;
    for (segment=11;segment<SegmentNum;++segment) {
        sprintf(Buffer,"%s%d_DATA",ModuleName,segment);
        BEDefSeg(segment,0x44,Buffer,16);
    }
    for (user=UserSegments;user!=NULL;user=user->next) {
        segment=user->segment;
        switch (user->segtype) {
        case 2:BEDefSeg(segment,6,user->name,4);break;
        case 3:BEDefSeg(segment,0x46,user->name,4);break;
        case 4:
            sprintf(Buffer,"%sB",user->name);
            BEDefSeg(segment-1,6,Buffer,1);
            BEDefSeg(segment,6,user->name,1);
            sprintf(Buffer,"%sE",user->name);
            BEDefSeg(segment-2,6,Buffer,1);
            break;
        }
    }
    for (text=TextSegList;text!=NULL;text=text->next) {
        text->segment_number=++SegmentNum;
        alignment=OptSize==0 ? BETypeLength(T_INTEGER):1;
        BEDefSeg(text->segment_number,0x87,text->segname,alignment);
    }
}

char *SegClassName(unsigned requested)
{
    struct user_seg *user;
    struct textsegment *text;
    char *name;
    int length;
    if (requested==1) return CodeClassName;
    for (user=UserSegments;user!=NULL;user=user->next) {
        if (user->segment==requested && user->class_name!=NULL) {
            name=user->class_name;
            if (*name=='\0') {
                length=strlen(user->name);
                if (length>=4) {
                    if (stricmp(user->name+length-4,"DATA")==0) name="FAR_DATA";
                    else if (stricmp(user->name+length-4,"TEXT")==0) name="CODE";
                }
            }
            return name;
        }
    }
    for (text=TextSegList;text!=NULL;text=text->next) {
        if (text->segment_number==requested) {
            name=strchr(text->segname,'\0')+1;
            return *name=='\0' ? CodeClassName:name;
        }
    }
    if (DataSegName!=NULL && *DataSegName!='\0') return "FAR_DATA";
    return NULL;
}

void EmitSegLabels(void)
{
    int segment;
    struct user_seg *user;
    void *back;
    SYM_ENTRY sym;
    segment=10000;
    for (user=UserSegments;user!=NULL;user=user->next) {
        if (user->sym_handle!=0) {
            SymGet(&sym,user->sym_handle);
            /* This older backend call passes NULL, unlike the later symbol. */
            back=BENewBack(NULL);
            sym.info.backinfo=back;
            SymReplace(&sym,user->sym_handle);
            BESetSeg(segment);
            DGLabel(back);
        }
        ++segment;
    }
}

void BEFiniBack(void *back);
void BEFreeBack(void *back);
/* This older combined cleanup finalizes and releases each user-segment
 * back handle; later source separates FiniSegBacks from FiniSegLabels. */
void FiniSegLabels(void)
{
    struct user_seg *user;
    SYM_ENTRY sym;
    for (user=UserSegments;user!=NULL;user=user->next) {
        if (user->sym_handle!=0) {
            SymGet(&sym,user->sym_handle);
            BEFiniBack(sym.info.backinfo);
            BEFreeBack(sym.info.backinfo);
        }
    }
}

void *FEBack(SYM_HANDLE handle)
{
    void *back;
    SYMPTR pointer;
    SYM_ENTRY sym;
    pointer=SymGetPtr(handle);
    back=pointer->info.backinfo;
    if (back==NULL) {
        back=BENewBack((void *)(unsigned)handle);
        SymGet(&sym,handle);
        sym.info.backinfo=back;
        SymReplace(&sym,handle);
    }
    return back;
}

unsigned FEAttr(void *symbol)
{
    return FESymAttr(SymGetPtr((SYM_HANDLE)(unsigned)symbol));
}

int FESegID(void *symbol)
{
    SYMPTR sym;
    unsigned attributes;
    int segment;
    sym=SymGetPtr((SYM_HANDLE)(unsigned)symbol);
    attributes=FESymAttr(sym);
    if (attributes&1) {
        segment=1;
        if (sym->seginfo!=NULL) segment=sym->seginfo->segment_number;
        else if ((attributes&8) && ((sym->attrib&8) || (TargetSwitches&8)) &&
                 (sym->flags&0x80)) segment=SegImport--;
    } else if (sym->u.var.segment!=0) segment=sym->u.var.segment;
    else segment=(attributes&4) ? 4:2;
    return segment;
}

char *FEName(void *symbol)
{
    SYM_HANDLE handle;
    handle=(SYM_HANDLE)(unsigned)symbol;
    if (handle==0) return "*** NULL ***";
    return SymGetPtr(handle)->name;
}

/* cinfo.c's public lookup returns the actual packed user-segment cell. */
hw_reg_set *SegPeggedReg(unsigned segment)
{
    struct user_seg *user;
    for (user=UserSegments;user!=NULL;user=user->next) {
        if (user->segment==segment) return &user->pegged_register;
    }
    return NULL;
}
