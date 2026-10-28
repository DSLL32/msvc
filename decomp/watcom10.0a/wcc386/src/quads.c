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


#define WCC_CSUICIDE_ABORTS
#include <string.h>
#include "compiler.h"
#include "quads.h"
#include "compiler.h"
#include "scanner.h"
#include "expression.h"
#include <recon.h>
#if defined(__WATCOMC__)
/* Intrinsic flags are global at deferred codegen, not lexical per-function.
 * Keep buffer copies as real memcpy calls; the supported _inline_ spelling
 * forces only the fixed record copies below through the compiler intrinsic. */
#pragma function(memcpy)
void *_inline_memcpy(void *destination,const void *source,size_t size);
#endif

unsigned LeafFreeList;
/* Provisional descriptive names for the older quad-chain state. */
unsigned FirstQuad,FirstCallQuad,LastCallQuad,DefaultLeaf;
struct seg_info QuadSegs[49],LeafSegs[98];
void CSegFree(SEGADDR_T segment);
/* Descriptive name for actual2b63f. This is the same allocated/farmem
 * segment ownership rule as csym.c SymFini, applied to the quad/leaf arrays. */
void FreeSegments(struct seg_info *segments,unsigned count)
{
    unsigned index;
    for (index=0;index<count;++index) {
        if (segments[index].allocated && segments[index].in_farmem)
            CSegFree(segments[index].index);
    }
}
void FreeDataQuads(void);
/* The old serialized-quad module finalizer, named by cpurge.c's surviving
 * QuadFini declaration. Table bounds49/98 are the actual initialized ABI. */
void QuadFini(void)
{
    FreeSegments(QuadSegs,49);
    FreeSegments(LeafSegs,98);
    FreeDataQuads();
}

void QuadInit(void)
{
    unsigned i;
    LEAF leaf;
    QuadIndex=0;
    FirstQuad=0;
    FirstCallQuad=0;
    LastCallQuad=0;
    LeafIndex=0;
    LeafFreeList=0;
    InitIntermediateFiles();
    for (i=0;i<49;++i) QuadSegs[i].allocated=0;
    for (i=0;i<98;++i) LeafSegs[i].allocated=0;
    InitDataQuads();
    InitLeaf(&leaf);
    leaf.leaf_class=8;
    leaf.leaf_type=GetType(TYPE_INT);
    GenLeaf(&leaf);
    InitLeaf(&leaf);
    leaf.leaf_class=19;
    leaf.leaf_type=GetType(TYPE_INT);
    DefaultLeaf=GenLeaf(&leaf);
    CompFlags.far_strings=0;
}

struct int_file_info QuadFileInfo,LeafFileInfo;
void InitIntermediateFiles(void)
{
    LeafFileInfo.seg_table=LeafSegs;
    InitFileInfo(&QuadFileInfo,QuadSegs,12);
    InitFileInfo(&LeafFileInfo,LeafSegs,sizeof(LEAF));
}

void InitFileInfo(struct int_file_info *info,struct seg_info *segments,unsigned item_size)
{
    info->seg_table=segments;
    info->size_of_item=item_size;
    info->items_per_buf=512/item_size;
    info->curr_buf_num=0;
    info->last_buf_num=0;
    info->curr_seg_num=0;
    info->curr_buf_seg=0;
    info->curr_buf_dirty=0;
    info->buffer=CPermAlloc(512);
}

void InitLeaf(LEAFPTR leaf)
{
    memset(leaf,0,sizeof(LEAF));
}

unsigned GenLeaf(LEAFPTR leaf)
{
    unsigned index=NewLeaf();
    LeafReplace(leaf,index);
    return index;
}

extern int ReclaimCount;
unsigned NewLeaf(void)
{
    unsigned index;
    LEAF leaf;
    if (LeafFreeList) {
        index=LeafFreeList;
        LeafGet(&leaf,index);
        ++ReclaimCount;
        LeafFreeList=leaf.u.words[1];
    } else {
        if (LeafIndex>=0xfffb) {
            CErr1(0x479);
            CSuicide();
        }
        index=++LeafIndex;
    }
    return index;
}

void *AccessFileInfo(unsigned index,struct int_file_info *info)
{
    unsigned buf_num=index/info->items_per_buf;
    unsigned seg_num;
    struct seg_info *si;
    if (buf_num!=info->curr_buf_num) {
        if (info->curr_buf_dirty) {
            si=&info->seg_table[info->curr_seg_num];
            if (!si->allocated) info->curr_buf_seg=AllocSegment(si);
            if (info->curr_buf_seg) {
                memcpy((char *)info->curr_buf_seg+(info->curr_buf_num%32)*512,
                       info->buffer,512);
            } else {
                PageSeek(si->index*0x4000+(info->curr_buf_num%32)*512);
                PageWrite(info->buffer,512);
            }
            info->curr_buf_dirty=0;
        }
        seg_num=buf_num/32;
        si=&info->seg_table[seg_num];
        info->curr_buf_seg=AccessSegment(si);
        if (buf_num<=info->last_buf_num) {
            if (info->curr_buf_seg) {
                memcpy(info->buffer,(char *)info->curr_buf_seg+(buf_num%32)*512,512);
            } else {
                PageSeek(si->index*0x4000+(buf_num%32)*512);
                PageRead(info->buffer,512);
            }
        } else {
            info->last_buf_num=buf_num;
            memset(info->buffer,0,512);
        }
        info->curr_buf_num=buf_num;
        info->curr_seg_num=seg_num;
    }
    return info->buffer+(index%info->items_per_buf)*info->size_of_item;
}

unsigned GenQuad(unsigned op1,int opr,unsigned op2,unsigned result)
{
    QUAD quad;
    unsigned index;
    if (!FoldQuad(op1,opr,op2,result) && SizeOfCount==0) {
        quad.quad_class=opr;
        quad.quad_flags=0;
        quad.src_line=SrcLineNum;
        quad.src_file=SrcFno;
        quad.operand1=op1;
        quad.operand2=op2;
        quad.result=result;
        switch (opr) {
        case T_FUNC_DEFN:
            CurFunc->u.func.start_of_func=QuadIndex;
            /* fall through */
        case 0x0a: case 0x0b: /* braces */
        case T_SWITCH: case T_FUNC_END: case T_CALL_NO_PARM:
            quad.operand2=GetQuadOperand(quad.operand2);
            break;
        case T_CASE: case T_FOR: case T_EXCEPT: case T_FINALLY:
            quad.operand1=GetQuadOperand(quad.operand1);
            break;
        case T_CALL: case T_END_FOR: case T_TRY: case T_UNROLL: case T_UNWIND:
            break;
        default:
            quad.operand1=GetQuadOperand(quad.operand1);
            quad.operand2=GetQuadOperand(quad.operand2);
        }
        if (SrcFile!=NULL && SrcFile->prev_file!=NULL) quad.quad_flags|=0x80;
        if (DeadCode!=0 && opr!=0x0a && opr!=0x0b) {
            quad.quad_flags=0x40;
            if (DeadCode==1) {
                CWarn1(0x11,0xc9);
                DeadCode=2;
            }
        } else if (opr!=T_CASE && opr!=T_LABEL_DEFN && CurFunc!=NULL) {
            ++CurFunc->u.func.number_of_quads;
        }
        index=NewQuad();
        QuadReplace(&quad,index);
    }
    return result;
}

/* Canonicalize the old leaf stream's small constants and symbol references. */
unsigned GetQuadOperand(unsigned index)
{
    LEAF leaf,previous;
    SYM_ENTRY sym;
    unsigned result;
    result=index;
    if (index!=0) {
        LeafGet(&leaf,index);
        if (leaf.leaf_class==0x0a && leaf.leaf_type->decl_type==TYPE_INT) {
            switch (leaf.u.constant.value.integer) {
            case 0: result=0xffff; break;
            case 1: result=0xfffe; break;
            case 2: result=0xfffd; break;
            case 4: result=0xfffc; break;
            }
        } else if (leaf.leaf_class==0x0c && leaf.leaf_type->decl_type==TYPE_LONG) {
            if (leaf.u.constant.value.integer==0) result=0xfffb;
        } else if (leaf.leaf_class==0) {
            SymGet(&sym,leaf.u.symbol.handle);
            if (index!=sym.sym_leaf && sym.sym_leaf!=0) {
                LeafGet(&previous,sym.sym_leaf);
                if (previous.leaf_class==0 &&
                    (previous.leaf_flags&~0x20U)==(leaf.leaf_flags&~0x20U) &&
                    previous.leaf_type==leaf.leaf_type &&
                    previous.u.symbol.handle==leaf.u.symbol.handle &&
                    previous.u.symbol.offset==leaf.u.symbol.offset) result=sym.sym_leaf;
            }
        }
        if (result!=index && !(leaf.leaf_flags&2)) FreeLeaf(index);
    }
    return result;
}

unsigned NewQuad(void)
{
    if (QuadIndex>=0xffff) {
        CErr1(0x479);
        CSuicide();
    }
    return QuadIndex++;
}

void QuadReplace(QUAD *quad,unsigned index)
{
    QUAD *dst;
    dst=AccessFileInfo(index,&QuadFileInfo);
    _inline_memcpy(dst,quad,sizeof(QUAD));
    QuadFileInfo.curr_buf_dirty=1;
}

/* One-based quad links reserve zero for the empty function chain. */
unsigned LastFunctionQuad;
void LinkFunctionQuads(unsigned index)
{
    QUAD quad;
    ++index;
    if (FirstQuad==0) FirstQuad=index;
    else {
        QuadGet(&quad,LastFunctionQuad-1);
        quad.operand1=index;
        QuadReplace(&quad,LastFunctionQuad-1);
    }
    LastFunctionQuad=index;
}

void QuadGet(QUAD *quad,unsigned index)
{
    QUAD *source;
    source=AccessFileInfo(index,&QuadFileInfo);
    _inline_memcpy(quad,source,sizeof(QUAD));
}

unsigned GenQuadNoResult(unsigned first,int op,unsigned second)
{
    return GenQuad(first,op,second,0);
}
