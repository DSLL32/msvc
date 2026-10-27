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


#include "block.h"
#include "cgmemory.h"
#include "genopt.h"
#include "pattern.h"
#include "stubs.h"
#include <recon.h>

#pragma pack(4)
typedef struct temp_entry {
    struct temp_entry *next;
    name *op,*actual_op;
    unsigned char actual_locn;
    instruction *first,*last,*entry,*exit;
    int savings;
    unsigned cached:1,defined:1,killed:1,global:1,whole_block:1;
} temp_entry;
#pragma pack()
typedef char old_temp_entry_size[(sizeof(temp_entry)==40)?1:-1];
temp_entry *TempList;
int MaxSeq;
unsigned char *SeqCurDepth,*SeqMaxDepth;
temp_entry *LookupTempEntry(name *op)
{
    temp_entry *temp;
    if (op==NULL) return NULL;
    if (op->n.class==2) op=DeAlias(op);
    for (temp=TempList;temp!=NULL;temp=temp->next)
        if (temp->op==op) return temp;
    return NULL;
}
temp_entry *AddTempEntry(name *op)
{
    temp_entry *temp;
    temp=LookupTempEntry(op);
    if (temp==NULL) {
        temp=CGAlloc(sizeof(*temp));
        temp->next=TempList;
        TempList=temp;
        temp->actual_op=op;
        if (op->n.class==2) op=DeAlias(op);
        temp->op=op;
        temp->first=NULL;
        temp->last=NULL;
        temp->savings=0;
        temp->actual_locn=8;
        temp->cached=0;
        temp->defined=0;
        temp->killed=0;
        temp->global=0;
        temp->whole_block=0;
    }
    return temp;
}
void KillTempEntry(name *op)
{
    temp_entry *temp;
    temp=AddTempEntry(op);
    temp->killed=1;
}
void DefUseTemp(name *op,instruction *ins,unsigned char defined)
{
    temp_entry *temp;
    temp=AddTempEntry(op);
    if (op->v.offset!=temp->actual_op->v.offset ||
        op->n.size!=temp->actual_op->n.size) {
        temp->killed=1;
        return;
    }
    if (temp->first==NULL) temp->first=ins;
    temp->last=ins;
    ++temp->savings;
    if (defined) temp->defined=1;
}
void CheckTemp(instruction *ins,name *op,unsigned char defined)
{
    unsigned char generate;
    if (op->n.class==1) {
        if (!(Model&0x04000000U)) return;
    } else if (op->n.class!=2) return;
    if (op->v.usage&8) return;
    generate=ins->u.gen_table->generate;
    if (!((generate>=0x24 && generate<=0x3e) || (generate>=0x60 && generate<=0x64)))
        KillTempEntry(op);
    else
        DefUseTemp(op,ins,defined);
}
unsigned char Better(void *left,void *right)
{
    temp_entry *t1,*t2;
    t1=left;
    t2=right;
    return t1->savings>t2->savings;
}
void *SortList(void *list,unsigned next_offset,unsigned char (*before)(void *,void *));
void InitTempEntries(block *blk)
{
    instruction *ins;
    int i;
    TempList=NULL;
    for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
        if (ins->ins_flags&1) continue;
        for (i=0;i<ins->num_operands;++i) CheckTemp(ins,ins->operands[i],0);
        if (ins->result!=NULL) CheckTemp(ins,ins->result,1);
    }
    TempList=SortList(TempList,offsetof(temp_entry,next),Better);
}
static block *Entry,*Exit;
unsigned char OKToCache(temp_entry *temp)
{
    instruction *ins;
    name *seg;
    ins=temp->first;
    if (ins->num_operands<=NumOperands(ins)) return 1;
    seg=ins->operands[ins->num_operands-1];
    ins=ins->head.prev;
    while (ins->head.opcode!=0x4b) {
        if (ReDefinedBy(ins,seg)) return 0;
        ins=ins->head.prev;
    }
    return (block *)ins->table==Entry;
}
void KillRelatedTemps(name *op)
{
    temp_entry *temp;
    if (op->n.class!=1) return;
    for (temp=TempList;temp!=NULL;temp=temp->next)
        if (temp->op->n.class==1 && temp->op->v.symbol==op->v.symbol) temp->killed=1;
}
static opcode_entry CacheRFLD={PRESERVE,0,45,0,0};
static opcode_entry CacheRCOMP={PRESERVE,0,57,0,0};
static opcode_entry CacheRRFBIN={PRESERVE,0,36,0,0};
static opcode_entry CacheRNFBIN={PRESERVE,0,37,0,0};
opcode_entry *RegAction(instruction *ins)
{
    switch (ins->u.gen_table->generate) {
    case 44: return &CacheRFLD;
    case 56: return &CacheRCOMP;
    case 42: return &CacheRRFBIN;
    case 43: return &CacheRNFBIN;
    default: return NULL;
    }
}
unsigned char StackBetween(instruction *first,instruction *last,int depth);
block *AddPreBlock(block *post) { STUB_TRAP();return NULL; }
void MoveEdge(block_edge *edge,block *destination) { STUB_TRAP(); }
void    CacheTemps( block *blk ) {
/****************************************/

    temp_entry *temp, **owner;
    block_edge  *exit_edge;

    Entry = NULL;
    Exit = NULL;
    if( blk->class & 0x100U
        && blk->inputs == 2 && blk->targets == 2 ) {
        if( blk->edge[0].destination == blk ) {
            Exit = blk->edge[1].destination;
            exit_edge = &blk->edge[1];
        } else if( blk->edge[1].destination == blk ) {
            Exit = blk->edge[0].destination;
            exit_edge = &blk->edge[0];
        } else {
            Exit = NULL;
        }
        if( Exit != NULL ) {
            if( blk->input_edges->source == blk ) {
                Entry = blk->input_edges->next_source->source;
            } else if( blk->input_edges->next_source->source == blk ) {
                Entry = blk->input_edges->source;
            } else {
                Entry = NULL;
                Exit = NULL;
            }
        }
        if( Entry != NULL ) {
            if( Entry->class & 2U ) {
                if( Exit->inputs != 1 ) {
                    Exit = AddPreBlock( Exit );
                    MoveEdge( exit_edge, Exit );
                }
            }
        }
    }
    for( temp = TempList; temp != NULL; temp = temp->next ) {
        if( temp->killed ) continue;
        if( !OKToCache( temp ) ) {
            temp->killed = 1;
        }
    }
    for( temp = TempList; temp != NULL; temp = temp->next ) {
        if( temp->killed ) {
            KillRelatedTemps( temp->op );
        }
    }
    for( temp = TempList; temp != NULL; temp = temp->next ) {
        if( temp->killed ) continue;
        if( temp->op->v.usage & 2U ) {
            if( Entry != NULL ) {
                temp->first = blk->ins.hd.next;
                temp->last = blk->ins.hd.prev;
                temp->whole_block = 1;
            } else {
                if( temp->defined ) continue;
                if( temp->first == temp->last ) continue;
            }
            temp->global = 1;
        } else {
            if( !temp->defined ) continue; // I'm not sure if these save anything
            if( temp->defined && temp->first->u.gen_table->generate!=47 ) continue;
            if( RegAction( temp->last ) == NULL ) continue;
            if( temp->first == temp->last ) continue;
        }
        if( StackBetween( temp->first, temp->last, 0 ) ) {
            StackBetween( temp->first, temp->last, 1 );
            temp->cached = 1;
        }
    }
    owner = &TempList;
    for( ;; ) {
        temp = *owner;
        if( temp == NULL ) break;
        if( temp->cached ) {
            owner = &temp->next;
        } else {
            *owner = temp->next;
            CGFreeSize( temp, sizeof( temp_entry ) );
        }
    }
}


extern int Max87Stk;
unsigned char StackBetween(instruction *first,instruction *last,int depth)
{
    instruction *ins;
    unsigned char enough;
    enough=1;
    for (ins=first;ins!=last;ins=ins->head.next) {
        if (ins->s.stk_depth>=Max87Stk-1 ||
            ins->head.opcode==0x36 || ins->head.opcode==0x29) enough=0;
        ins->s.stk_depth+=depth;
    }
    return enough;
}
unsigned char FPStackIns(instruction *ins);
int FPStkReq(instruction *ins);
int InsMaxDepth(instruction *ins)
{
    return ins->stk_entry>ins->stk_exit ? ins->stk_entry : ins->stk_exit;
}
void FPPreSched(block *blk)
{
    temp_entry *temp;
    instruction *ins;
    int i,depth;
    MaxSeq=0;
    ins=blk->ins.hd.next;
    while (ins->head.opcode!=0x4b) {
        if (ins->sequence>MaxSeq) MaxSeq=ins->sequence;
        ins=ins->head.next;
    }
    ++MaxSeq;
    InitTempEntries(blk);
    CacheTemps(blk);
    for (temp=TempList;temp!=NULL;temp=temp->next) StackBetween(temp->first,temp->last,-1);
    SeqCurDepth=CGAlloc(MaxSeq*sizeof(*SeqCurDepth));
    SeqMaxDepth=CGAlloc(MaxSeq*sizeof(*SeqMaxDepth));
    for (i=0;i<MaxSeq;++i) {
        SeqCurDepth[i]=0xff;
        SeqMaxDepth[i]=0;
    }
    ins=blk->ins.hd.prev;
    while (ins->head.opcode!=0x4b) {
        if (SeqCurDepth[ins->sequence]==0xff && FPStackIns(ins))
            SeqCurDepth[ins->sequence]=ins->stk_exit;
        ins=ins->head.prev;
    }
    for (i=0;i<MaxSeq;++i) if (SeqCurDepth[i]==0xff) SeqCurDepth[i]=0;
    ins=blk->ins.hd.next;
    while (ins->head.opcode!=0x4b) {
        ins->s.stk_extra=FPStkReq(ins);
        depth=InsMaxDepth(ins)+ins->s.stk_extra;
        ins->t.stk_max=SeqMaxDepth[ins->sequence];
        if (depth>SeqMaxDepth[ins->sequence]) SeqMaxDepth[ins->sequence]=depth;
        ins=ins->head.next;
    }
}

unsigned char FPInsIntroduced(instruction *ins) { STUB_TRAP();return 0; }

int FPStackExit(block *blk)
{
    instruction *curr;
    for (curr=blk->ins.hd.prev;curr->head.opcode!=0x4b;curr=curr->head.prev)
        if (FPStackIns(curr)) return curr->stk_exit;
    return 0;
}
extern int Max87Stk;
int FPStkOver(instruction *ins,int stk_depth)
{
    int i,depth,max_depth;
    max_depth=0;
    for (i=ins->sequence+1;i<MaxSeq;++i) {
        depth=SeqMaxDepth[i]-SeqCurDepth[i];
        if (depth>max_depth) max_depth=depth;
    }
    return SeqMaxDepth[ins->sequence]-SeqCurDepth[ins->sequence]+max_depth+stk_depth-Max87Stk;
}
void FPCalcStk(instruction *ins,int *pdepth)
{
    int affect;
    if (FPStackIns(ins)) SeqMaxDepth[ins->sequence]=ins->t.stk_max;
    affect=ins->stk_entry-ins->stk_exit;
    SeqCurDepth[ins->sequence]+=affect;
    ins->stk_exit=*pdepth;
    ins->stk_entry=*pdepth+affect;
    ins->s.stk_depth=*pdepth+ins->s.stk_extra;
    if (affect>0) ins->s.stk_depth+=affect;
    *pdepth+=affect;
}

unsigned char FPFreeIns(instruction *ins)
{
    temp_entry *temp;
    for (temp=TempList;temp!=NULL;temp=temp->next) {
        if (temp->whole_block) {
            if (ins->u.gen_table->generate==47 && temp->actual_op==ins->result) return 1;
        } else if (ins==temp->first && temp->defined) {
            return 1;
        } else if (ins==temp->last && ins->u.gen_table->generate==44) {
            return 1;
        }
    }
    return 0;
}

enum {
    NEEDS_ST0=1,NEEDS_ST0_ST1=2,PUSHES=4,POPS=8,SETS_ST1=0x10,
    EXCHANGES=0x20,POPS_ALL=0x40,POPS2=0x80
};
void FiniTempEntries(void)
{
    temp_entry *temp,*junk;
    temp=TempList;
    while (temp!=NULL) {
        junk=temp;
        temp=temp->next;
        CGFreeSize(junk,sizeof(temp_entry));
    }
    TempList=NULL;
}
typedef struct st_seq { unsigned char actual_locn[8]; } st_seq;
st_seq *STLocations;
void InitStackLocations(void)
{
    temp_entry *temp;
    int i,j;
    STLocations=CGAlloc(MaxSeq*sizeof(*STLocations));
    for (i=0;i<MaxSeq;++i)
        for (j=0;j<8;++j) STLocations[i].actual_locn[j]=8;
    for (temp=TempList;temp!=NULL;temp=temp->next) temp->actual_locn=8;
}
instruction *PrefFLDOp(instruction *ins,unsigned short type,name *op);
void InitGlobalTemps(void)
{
    temp_entry *temp;
    instruction *ins;
    int locn;
    locn=0;
    for (temp=TempList;temp!=NULL;temp=temp->next) if (temp->whole_block) ++locn;
    for (temp=TempList;temp!=NULL;temp=temp->next) {
        if (temp->whole_block) {
            ins=Entry->ins.hd.prev;
            while (ins->head.opcode==0) {
                if (ins->flags.nop_flags&8) break;
                ins=ins->head.prev;
            }
            ins=ins->head.next;
            PrefFLDOp(ins,3,temp->actual_op);
            ins->head.prev->ins_flags|=1;
            temp->actual_locn=--locn;
        }
    }
}
instruction *SuffFSTPRes(instruction *ins,name *op,int result_type);
void FiniGlobalTemps(void)
{
    temp_entry *temp;
    instruction *ins;
    for (temp=TempList;temp!=NULL;temp=temp->next) {
        if (temp->whole_block) {
            ins=(instruction *)&Exit->ins;
            SuffFSTPRes(ins,temp->actual_op,temp->defined ? 2 : 4);
            ins->head.next->ins_flags|=1;
        }
    }
}
void FiniStackLocations(void)
{
    CGFreeSize(STLocations,MaxSeq*sizeof(st_seq));
    STLocations=NULL;
}
int NumOperands(instruction *ins);
unsigned char FPAttr(instruction *ins)
{
    /* Actual 1994 generate-entry numbers, preserving their semantic names. */
    enum {
        G_RRFBIN=36,G_RNFBIN=37,G_RRFBINP=38,G_RNFBINP=39,
        G_MRFBIN=42,G_MNFBIN=43,G_MFLD=44,G_RFLD=45,
        G_MFST=47,G_RFST=48,G_FCHS=50,G_FRNDINT=51,G_FLDZ=52,G_FLD1=53,
        G_FCOMPP=55,G_MCOMP=56,G_RCOMP=57,G_MFSTNP=60,G_RFSTNP=61,G_FTST=62,
        G_FCHOP=85,G_FMATH=96,G_IFUNC=97,G_FXCH=100
    };
    unsigned char generate;
    if ((ins->head.opcode==0x36 || ins->head.opcode==0x29) && ins->result!=NULL) {
        if (ins->result->n.class!=3 || !(ins->result->r.reg&0x00ff0000U)) return POPS_ALL;
        return PUSHES|POPS_ALL;
    }
    generate=ins->u.gen_table->generate;
    if (generate==G_FCHOP) return NEEDS_ST0;
    if (!((generate>=36 && generate<=62) || (generate>=96 && generate<=100))) return 0;
    switch (generate) {
    case G_RRFBINP: case G_RNFBINP: return NEEDS_ST0|POPS|SETS_ST1;
    case G_MFLD: case G_FLDZ: case G_FLD1: case G_RFLD: return PUSHES;
    case G_FRNDINT: case G_FCHS: case G_MFSTNP: case G_RFSTNP: case G_FTST:
    case G_FMATH: case G_RRFBIN: case G_RNFBIN: case G_MRFBIN: case G_MNFBIN:
        return NEEDS_ST0;
    case G_FCOMPP: return NEEDS_ST0_ST1|POPS2;
    case G_MFST: case G_RFST: case G_MCOMP: case G_RCOMP: return NEEDS_ST0|POPS;
    case G_IFUNC: return NumOperands(ins)==2 ? NEEDS_ST0_ST1|POPS : NEEDS_ST0;
    case G_FXCH: return NEEDS_ST0|EXCHANGES;
    default: Zoiks(75);return 0;
    }
}
name *ST(int reg);
void PrefixExchange(instruction *ins,int actual);
unsigned char *ActualStackOwner(int actual);
void GetToTopOfStack(instruction *ins,int virtual)
{
    unsigned char actual_locn,*actual_top_owner;
    actual_locn=STLocations[ins->sequence].actual_locn[virtual];
    if (actual_locn==0) return;
    actual_top_owner=ActualStackOwner(0);
    if (actual_top_owner!=NULL) {
        PrefixExchange(ins,actual_locn);
        STLocations[ins->sequence].actual_locn[virtual]=0;
        *actual_top_owner=actual_locn;
        return;
    }
    Zoiks(76);
}
instruction *FComppKluge(instruction *ins) { STUB_TRAP();return NULL; }
instruction *GetST0andST1(instruction *ins) { STUB_TRAP();return NULL; }
int Count87Regs(hw_reg_set regs);
void PopStack(instruction *ins);
void PrefixExchange(instruction *ins,int actual) { STUB_TRAP(); }
unsigned char *ActualStackOwner(int actual) { STUB_TRAP();return NULL; }
void XchForCall(instruction *ins,int i)
{
    unsigned char *actual_i_owner,*actual_0_owner;
    /* Actual tail recursion is a backwards scan after the one prologue. */
    while (i>0) {
        if (STLocations[ins->sequence].actual_locn[i]!=i) {
            GetToTopOfStack(ins,i);
            if (STLocations[ins->sequence].actual_locn[i]!=i) {
                PrefixExchange(ins,i);
                actual_i_owner=ActualStackOwner(i);
                actual_0_owner=ActualStackOwner(0);
                *actual_i_owner=0;
                *actual_0_owner=(unsigned char)i;
            }
        }
        --i;
    }
}
void ReOrderForCall(instruction *ins)
{
    int i,count;
    count=Count87Regs(ins->operands[0]->r.reg);
    XchForCall(ins,count-1);
    for (i=0;i<count;++i) PopStack(ins);
}
static opcode_entry CacheRFST={PRESERVE,0,48,0,0};
static opcode_entry CacheRFSTNP={PRESERVE,0,61,0,0};
void PopVirtualStack(instruction *ins)
{
    int i;
    for (i=0;i<7;++i)
        STLocations[ins->sequence].actual_locn[i]=STLocations[ins->sequence].actual_locn[i+1];
    STLocations[ins->sequence].actual_locn[7]=8;
}
unsigned char ResultToReg(instruction *ins,temp_entry *temp,unsigned char attr)
{
    if (ins==temp->first && !temp->whole_block) {
        DoNothing(ins);
        temp->actual_locn=STLocations[ins->sequence].actual_locn[0];
        PopVirtualStack(ins);
        attr&=~POPS;
    } else {
        GetToTopOfStack(ins,0);
        if (ins->u.gen_table->generate==47) ins->u.gen_table=&CacheRFST;
        else ins->u.gen_table=&CacheRFSTNP;
        ins->result=ST(temp->actual_locn);
    }
    return attr;
}
void SetResultReg(instruction *ins,int virtual) { STUB_TRAP(); }
/* Cache rewrites have the old scheduler's FU_NONE selectors, distinct
 * from the arithmetic expansion tables in i87exp.c. */
static opcode_entry CacheFCOMPP={PRESERVE,0,55,0,0};
static opcode_entry CacheRNFBINP={PRESERVE,0,39,0,0};
static opcode_entry CacheRRFBINP={PRESERVE,0,38,0,0};
instruction *PrefFXCH(instruction *ins,int actual);
void PushVirtualStack(instruction *ins);
void IncrementAll(void);
void PushStack(instruction *ins);
void DecrementAll(void)
{
    int i,j;
    temp_entry *temp;
    for (i=0;i<MaxSeq;++i) {
        for (j=0;j<7;++j) {
            if (STLocations[i].actual_locn[j]!=8)
                --STLocations[i].actual_locn[j];
        }
    }
    for (temp=TempList;temp!=NULL;temp=temp->next) {
        if (temp->actual_locn!=8) --temp->actual_locn;
    }
}
instruction *OpToReg(instruction *ins,temp_entry *temp,unsigned char attr)
{
    unsigned char *actual_owner;
    if (ins==temp->last && !temp->whole_block) {
        switch (ins->u.gen_table->generate) {
        case 44: /* G_MFLD */
            PushVirtualStack(ins);
            STLocations[ins->sequence].actual_locn[0]=temp->actual_locn;
            DoNothing(ins);
            break;
        case 56: /* G_MCOMP */
            ins->operands[0]=ST(1);
            ins->operands[1]=ST(0);
            if (temp->actual_locn!=1) {
                actual_owner=ActualStackOwner(1);
                *actual_owner=temp->actual_locn;
                PrefFXCH(ins,1);
                PrefFXCH(ins,temp->actual_locn);
                RevCond(ins);
            }
            ins->u.gen_table=&CacheFCOMPP;
            PopStack(ins);
            PopStack(ins);
            break;
        case 42: case 43: /* G_MRFBIN/G_MNFBIN */
            if (ins->u.gen_table->generate==42) ins->u.gen_table=&CacheRNFBINP;
            else ins->u.gen_table=&CacheRRFBINP;
            ins->operands[0]=ST(temp->actual_locn);
            ins->result=ins->operands[0];
            ins->operands[1]=ST(0);
            STLocations[ins->sequence].actual_locn[0]=temp->actual_locn;
            DecrementAll();
            break;
        }
        temp->actual_locn=8;
    } else {
        if (ins==temp->first && !temp->defined && !temp->whole_block) {
            PrefFLDOp(ins,3,temp->actual_op);
            IncrementAll();
            temp->actual_locn=0;
            if (ins->u.gen_table->generate==44) ins->operands[0]=ST(0);
            else {
                GetToTopOfStack(ins,0);
                ins->operands[0]=ST(temp->actual_locn);
            }
        } else ins->operands[0]=ST(temp->actual_locn);
        if (attr&PUSHES) PushStack(ins);
        else if (attr&POPS2) {
            PopStack(ins);
            PopStack(ins);
        } else if (attr&POPS) PopStack(ins);
        ins->u.gen_table=RegAction(ins);
    }
    return ins;
}
void IncrementAll(void)
{
    int i,j;
    temp_entry *temp;
    for (i=0;i<MaxSeq;++i) {
        for (j=0;j<7;++j) {
            if (STLocations[i].actual_locn[j]!=8)
                ++STLocations[i].actual_locn[j];
        }
    }
    for (temp=TempList;temp!=NULL;temp=temp->next) {
        if (temp->actual_locn!=8) ++temp->actual_locn;
    }
}
void PushVirtualStack(instruction *ins)
{
    int i;
    for (i=7;i>0;--i)
        STLocations[ins->sequence].actual_locn[i]=STLocations[ins->sequence].actual_locn[i-1];
    STLocations[ins->sequence].actual_locn[0]=0;
}
void PushStack(instruction *ins)
{
    IncrementAll();
    PushVirtualStack(ins);
}
void PopStack(instruction *ins)
{
    PopVirtualStack(ins);
    DecrementAll();
}
int FPRegNum(name *op);
void    FPPostSched( block *blk ) {
/*****************************************/

    unsigned char     attr;
    instruction *ins;
    instruction *next;
    int         virtual;
    temp_entry  *temp;

    CGFreeSize( SeqCurDepth, MaxSeq * sizeof( *SeqCurDepth ) );
    CGFreeSize( SeqMaxDepth, MaxSeq * sizeof( *SeqMaxDepth ) );
    FiniTempEntries();
    InitStackLocations();
    InitTempEntries( blk );
    CacheTemps( blk );
    InitGlobalTemps();
    ins = blk->ins.hd.next;
    while( ins->head.opcode != 0x4b ) {
        next = ins;
        if( !( ins->ins_flags & 1 ) ) {
            attr = FPAttr( ins );
            temp = LookupTempEntry( ins->result );
            if( attr & NEEDS_ST0 && temp == NULL ) {
                GetToTopOfStack( ins, 0 );
            } else if( attr & NEEDS_ST0_ST1 ) {
                if( attr & POPS2 ) {
                    // the dreaded FCOMPP
                    next = FComppKluge( ins );
                } else {
                    next = GetST0andST1( ins );
                }
            } else if( attr & POPS_ALL ) {
                ReOrderForCall( ins );
            }
            if( temp != NULL ) {
                attr = ResultToReg( ins, temp, attr );
            } else if( attr & SETS_ST1 ) {
                SetResultReg( ins, 1 );
            } else if( attr & EXCHANGES ) {
                virtual = FPRegNum( ins->result );
                SetResultReg( ins, virtual );
            }
            temp = LookupTempEntry( ins->operands[0] );
            if( temp != NULL ) {
                next = OpToReg( ins, temp, attr );
            } else if( attr & PUSHES ) {
                PushStack( ins );
            } else if( attr & POPS2 ) {
                PopStack( ins );
                PopStack( ins );
            } else if( attr & POPS ) {
                PopStack( ins );
            }
        }
        ins = next->head.next;
    }
    FiniGlobalTemps();
    FiniStackLocations();
    FiniTempEntries();
}
