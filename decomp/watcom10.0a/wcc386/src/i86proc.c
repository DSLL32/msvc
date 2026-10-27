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
* Description:  i386 procedure-frame and register availability decisions.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "block.h"
#include "genopt.h"
#include "dbinfo.h"
#include "options.h"
#include "stubs.h"
#include <recon.h>

#define WINDOWS_CHEAP ((TargetModel&0x4000) && !(CurrProc->prolog_state&0xc))
#define DO_WINDOWS_CRAP ((TargetModel&0x2000) && (!WINDOWS_CHEAP || CurrProc->contains_call))
#define DO_BP_CHAIN ((((TargetModel&0x20000) || (Model&0x8000000)) && CurrProc->contains_call) || (CurrProc->prolog_state&8))
#define CHAIN_FRAME (DO_WINDOWS_CRAP || DO_BP_CHAIN)
#define FAR_RET_ON_STACK (((CurrProc->state.attr&0x2002)==2) && !(CurrProc->state.attr&0x10))
#define CHEAP_FRAME ((TargetModel&0x20000) || !(TargetModel&0x2000) || WINDOWS_CHEAP)

int AskDisplaySize(int level)
{
    return level*4;
}

unsigned char ScanInstructions(void);
unsigned char CanZapBP(void);
unsigned char ScanLabelCalls(void);
void ScanForFDOps(void);
void AddCacheRegs(void)
{
    if (CurrProc->targ.never_sp_frame) return;
    if (!(Model&0x100000)) return; /* MEMORY_LOW_FAILS */
    if (OptForSize>50) return;
    if (TargetModel&0x600) return; /* FLOATING_DS | FLOATING_SS */
    if (!ScanInstructions()) return;
    if (!CanZapBP()) return;
    if (!ScanLabelCalls()) return;
    if (CurrProc->state.attr&0x80) return;
    if (CurrProc->lex_level!=0) return;
    if ((CGProcessorVersion&0x70)>=0x30) ScanForFDOps();
    if (CurrProc->targ.has_fd_temps) {
        CurrProc->targ.sp_frame=1;
        CurrProc->targ.sp_align=1;
    } else if (!((((TargetModel&0x20000) || (Model&0x8000000)) &&
                   CurrProc->contains_call) || (CurrProc->prolog_state&8)) &&
               !(TargetModel&0x2000)) {
        /* Old DO_BP_CHAIN uses GENERATE_FAT_PROLOG at bit8; this body
         * predates the later independent interrupt-attribute guard. */
        CurrProc->targ.sp_frame=1;
        CurrProc->state.unalterable&=~0x400U; /* HW_BP */
    }
}
unsigned char AskIfRTLabel(struct code_lbl *label);
unsigned char ScanInstructions(void)
{
    block *blk;
    instruction *ins;
    name *addr;
    unsigned char sp_constant;
    CurrProc->contains_call=1;
    if (BlockByBlock) return 0;
    CurrProc->contains_call=0;
    sp_constant=1;
    blk=HeadBlock;
    while (blk!=NULL) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            if (ins->head.opcode==0x36 || ins->head.opcode==0x29) {
                if (ins->head.opcode==0x29) CurrProc->contains_call=1;
                else {
                    addr=ins->operands[2]; /* CALL_OP_ADDR */
                    if (addr->n.class!=1 || addr->m.memory_type!=1 ||
                        !AskIfRTLabel(addr->v.symbol)) CurrProc->contains_call=1;
                }
                if (ins->zap->reg&0x800) {
                    CurrProc->state.attr|=0x20;
                    sp_constant=0;
                }
            }
            ins=ins->head.next;
        }
        blk=blk->next_block;
    }
    return sp_constant;
}
unsigned char CanZapBP(void)
{
    unsigned char windows_cheap,windows_crap,bp_chain;
    /* Old WINDOWS_CHEAP predates the later ROUTINE_LOADS_DS case. */
    windows_cheap=(TargetModel&0x4000) && !(CurrProc->prolog_state&0xc);
    windows_crap=(TargetModel&0x2000) && (!windows_cheap || CurrProc->contains_call);
    bp_chain=(((TargetModel&0x20000) || (Model&0x8000000)) &&
              CurrProc->contains_call) || (CurrProc->prolog_state&8);
    return !(windows_crap || bp_chain);
}
block *ScanForLabelReturn(block *blk);
unsigned char ScanLabelCalls(void)
{
    block *blk;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        if (!(blk->class&0x40)) continue;
        if (ScanForLabelReturn(blk->edge[0].destination)==NULL) return 0;
    }
    return 1;
}
block *ScanForLabelReturn(block *blk) { STUB_TRAP(); return NULL; }
void ScanForFDOps(void) { STUB_TRAP(); }

/* Pending original i86 procedure/encoder/debug entries; no body credit. */
/* Earlier inline export query, before the later SymIsExported vertex. */
void FindIfExported(void)
{
    void *symbol,*aux;
    unsigned *call_class;
    symbol=AskForLblSym(CurrProc->label);
    if (symbol==NULL) return;
    aux=FEAuxInfo(symbol,0); /* AUX_LOOKUP */
    call_class=FEAuxInfo(aux,2); /* CALL_CLASS */
    if (*call_class&0x200) CurrProc->prolog_state|=4;
}
void EmitNameInCode(void) { STUB_TRAP();  }
void CodeLineNum(unsigned short line,unsigned char label) { STUB_TRAP();  }
void EmitRtnBeg(void) { STUB_TRAP();  }
struct code_lbl * GenFar16Thunk(struct code_lbl *label,unsigned short size,unsigned char removes) { STUB_TRAP(); return NULL; }
unsigned DepthAlign(unsigned depth);
void CodeLabel(struct code_lbl *label,unsigned align);
void GenP5ProfilingProlog(struct code_lbl *label) { STUB_TRAP();  }
void QuickSave(hw_reg_set reg,unsigned char op);
void GenPushC(long value) { STUB_TRAP();  }
void GenUnkPush(void *patch) { STUB_TRAP();  }
int FEStackChk(void *symbol);
unsigned char NeedStackCheck(void)
{
    return FEStackChk(AskForLblSym(CurrProc->label));
}
void RTCall(int routine,unsigned char pop) { STUB_TRAP();  }
void CalcUsedRegs(void)
{
    block *blk;
    instruction *ins;
    name *result;
    hw_reg_set used;
    used=0;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            result=ins->result;
            if (result!=NULL && result->n.class==3) used|=result->r.reg;
            if (ins->head.opcode!=0) {
                used|=ins->zap->reg;
                if (ins->zap->reg&0x800) CurrProc->prolog_state|=0x80;
            }
        }
    }
    if (!CurrProc->targ.sp_frame || CurrProc->targ.sp_align) used&=~0x400U;
    CurrProc->state.used|=used;
}
hw_reg_set SaveRegs(void);
int PushAll(void) { STUB_TRAP(); return 0; }
unsigned char NeedBPProlog(void)
{
    if (CurrProc->parms.size!=0 || CurrProc->locals.size!=0 ||
        CurrProc->targ.push_local_size!=0 || CurrProc->lex_level!=0 ||
        CurrProc->targ.sp_align || BlockByBlock || (CurrProc->state.attr&0x20)) return 1;
    if (FAR_RET_ON_STACK && CHAIN_FRAME) return 1;
    return 0;
}
void GenWindowsProlog(void) { STUB_TRAP();  }
void GenCypWindowsProlog(void) { STUB_TRAP();  }
void GenRegMove(hw_reg_set from,hw_reg_set to) { STUB_TRAP();  }
void PrologHook(void) { STUB_TRAP();  }
void DoStackCheck(void)
{
    if (CurrProc->prolog_state&0x20) return;
    if (CurrProc->prolog_state&0x40) {
        if (BlockByBlock || CurrProc->locals.size>=4096) {
            GenUnkPush(&CurrProc->targ.stack_check);
            RTCall(0x34,0x40); /* RT_GROW, ATTR_POP */
        }
        return;
    }
    if (NeedStackCheck()) {
        GenUnkPush(&CurrProc->targ.stack_check);
        RTCall(0x33,0x40); /* RT_CHK, ATTR_POP */
    }
}
void DoLoadDS(void) { STUB_TRAP(); }
int LoadDS(void)
{
    int size;
    size=0;
    if ((CurrProc->state.attr&0x8000) && (CurrProc->state.unalterable&0x1000)) {
        QuickSave(0x1000,0x2a); /* HW_DS, OP_PUSH */
        DoLoadDS();
        size=4;
    }
    return size;
}
/* Source-owned 1994 PushRegs at79c30, thirteen DWORDs including sentinel.
 * This body predates the later FlowSave call. */
/* Remaining names were not yet needed by the option parser header. */
#define HW_EDI 0x20000200U
#define HW_SS 0x8000U
#define HW_BP 0x400U
hw_reg_set PushRegs[]={HW_EAX,HW_EBX,HW_ECX,HW_EDX,HW_ESI,HW_EDI,
                       HW_DS,HW_ES,HW_FS,HW_GS,HW_SS,HW_BP,0};
int Push(hw_reg_set regs)
{
    hw_reg_set *curr_push;
    int size;
    curr_push=PushRegs;
    size=0;
    while (regs!=0) {
        if (*curr_push&regs) {
            QuickSave(*curr_push,0x2a);
            size+=4;
            regs&=~*curr_push;
        }
        ++curr_push;
        if (*curr_push==0) break;
    }
    return size;
}
void GenUnkSub(hw_reg_set reg,void *patch) { STUB_TRAP(); }
void GenTouchStack(unsigned char unknown_size) { STUB_TRAP(); }
void GenRegAnd(hw_reg_set reg,long value) { STUB_TRAP(); }
void GenRegSub(hw_reg_set reg,long value);
void AllocStack(void)
{
    long size;
    size=(CurrProc->locals.size+3)&~3L;
    CurrProc->locals.size=size;
    if (BlockByBlock) {
        GenUnkSub(0x800,&CurrProc->targ.prolog_loc);
        if (CurrProc->prolog_state&0x400) GenTouchStack(1);
    } else if (size<=8 && OptForSize>50) {
        while (size>0) {
            QuickSave(HW_EAX,0x2a);
            size-=4;
        }
    } else if (size!=0) {
        GenRegSub(0x800,size);
        if (CurrProc->prolog_state&0x400) GenTouchStack(0);
    }
    if (CurrProc->targ.sp_align) {
        GenRegAnd(0x800,-8);
        CurrProc->prolog_state|=0x80;
    }
}
void AdjustPushLocal(name *temp);
void AdjustPushLocals(void)
{
    instruction *ins;
    ins=HeadBlock->ins.hd.next;
    for (;;) {
        if (DoesSomething(ins)) break;
        if (ins->head.opcode==0x26 && ins->head.state==1) {
            /* Old entry pushes a register SET through Push, not QuickSave. */
            Push(ins->operands[0]->r.reg);
            AdjustPushLocal(ins->result);
        }
        ins=ins->head.next;
    }
}
void DoEnter(int level) { STUB_TRAP(); }
void GenPushOffset(unsigned char offset) { STUB_TRAP(); }
void GenRegAdd(hw_reg_set reg,long value);
void Enter(void)
{
    int lex_level,i;
    lex_level=CurrProc->lex_level;
    if (!CurrProc->targ.sp_frame && (CGProcessorVersion&15)>=1 &&
        CurrProc->locals.size<=65535 &&
        (lex_level!=0 || (CurrProc->locals.size!=0 && OptForSize>50))) {
        DoEnter(lex_level);
        CurrProc->state.used|=0x400;
        CurrProc->state.attr|=0x20;
    } else {
        if (NeedBPProlog()) {
            if (!CurrProc->targ.sp_frame || CurrProc->targ.sp_align) {
                CurrProc->state.used|=0x400;
                CurrProc->parms.base+=4;
                QuickSave(0x400,0x2a);
                i=0;
                while (--lex_level>0) {
                    i-=2;
                    GenPushOffset((unsigned char)i);
                }
                GenRegMove(0x800,0x400);
                if (CurrProc->lex_level>1) GenRegAdd(0x400,(CurrProc->lex_level-1)*4);
                if (CurrProc->lex_level!=0) QuickSave(0x400,0x2a);
            }
            CurrProc->state.attr|=0x20;
        }
        AllocStack();
    }
}
long AdjustBase(void) { STUB_TRAP(); return 0; }
void GenRegSub(hw_reg_set reg,long value);
void RelocParms(void);
extern name *Parm8087[9];
void GFstpM(void *operand) { STUB_TRAP(); }
void MoveParms(void)
{
    int i;
    i=0;
    while (Parm8087[i]!=NULL) {
        GFstpM(Parm8087[i]);
        ++i;
    }
}
void DbgRetOffset(long offset) { STUB_TRAP();  }
void EmitProEnd(void) { STUB_TRAP();  }
void OutDLLExport(unsigned words,void *symbol) { STUB_TRAP();  }

/* Full earlier GenProlog, before the released FE_NAKED/flow-save additions.
 * ROUTINE class uses bits 0x2002: interrupt=0x2000, far=2, far16=0x2002.
 * HW_BP=0x400, HW_SP=0x800, HW_FLTS=0x00ff0000 in this register model. */

void GenProlog(void)
{
    unsigned short old;
    hw_reg_set to_push;
    int ret_size;
    struct code_lbl *label;
    ScanInstructions();
    FindIfExported();
    old=SetOP(AskCodeSeg());
    if (CurrProc->prolog_state&0x10) EmitNameInCode();
    if (Model&0x40000000) CodeLineNum(HeadBlock->ins.hd.line_num,0);
    if (CurrProc->state.attr&0x80) {
        EmitRtnBeg();
        CurrProc->state.attr|=0x20;
    }
    CurrProc->parms.size=CurrProc->state.parm.offset;
    CurrProc->parms.base=0;
    label=CurrProc->label;
    if ((CurrProc->state.attr&0x2002)==0x2002)
        label=GenFar16Thunk(label,(unsigned short)CurrProc->parms.size,
                           (unsigned char)(CurrProc->state.attr&1));
    CodeLabel(label,DepthAlign(-1));
    if (TargetModel&0x200000) GenP5ProfilingProlog(label);
    if (CurrProc->prolog_state&0x20) {
        QuickSave(0x800,0x2a); /* OP_PUSH */
        GenPushC(CurrProc->parms.size);
        GenUnkPush(&CurrProc->targ.stack_check);
        if (NeedStackCheck()) RTCall(0x36,0x40);
        else RTCall(0x35,0x40);
        CurrProc->parms.base+=4;
    }
    if ((CurrProc->state.attr&0x2002)==0x2000 || (CurrProc->state.attr&0x10)) ret_size=0;
    else if ((CurrProc->state.attr&0x2002)==2 || (CurrProc->state.attr&0x2002)==0x2002) ret_size=8;
    else ret_size=4;
    CurrProc->parms.base+=ret_size;
    CalcUsedRegs();
    to_push=SaveRegs();
    to_push&=~0x00ff0000U;
    if (!CurrProc->targ.sp_frame || CurrProc->targ.sp_align) to_push&=~0x400U;
    if ((CurrProc->state.attr&0x2002)==0x2000) {
        ret_size=-PushAll();
        CurrProc->targ.base_adjust=0;
    } else {
        if (CHAIN_FRAME) {
            CurrProc->targ.base_adjust=0;
            if (NeedBPProlog()) {
                CurrProc->state.used|=0x400;
                CurrProc->parms.base+=4;
                if (FAR_RET_ON_STACK) {
                    if (CHEAP_FRAME) GenCypWindowsProlog();
                    else {
                        GenWindowsProlog();
                        CurrProc->targ.base_adjust+=2;
                    }
                } else {
                    QuickSave(0x400,0x2a);
                    GenRegMove(0x800,0x400);
                }
                PrologHook();
            }
            DoStackCheck();
            CurrProc->targ.base_adjust+=LoadDS();
            CurrProc->targ.base_adjust+=Push(to_push);
            CurrProc->parms.base+=CurrProc->targ.base_adjust;
            AllocStack();
            AdjustPushLocals();
        } else {
            DoStackCheck();
            CurrProc->parms.base+=LoadDS();
            if (!(CurrProc->state.attr&0x10)) CurrProc->parms.base+=Push(to_push);
            Enter();
            AdjustPushLocals();
            if ((Model&0x80000000U) || CurrProc->targ.sp_frame) CurrProc->targ.base_adjust=0;
            else {
                CurrProc->targ.base_adjust=AdjustBase();
                if (CurrProc->targ.base_adjust!=0) GenRegSub(0x400,-CurrProc->targ.base_adjust);
            }
        }
        RelocParms();
    }
    MoveParms();
    CurrProc->prolog_state|=1;
    if (CurrProc->state.attr&0x80) {
        DbgRetOffset(CurrProc->parms.base-CurrProc->targ.base_adjust-ret_size);
        EmitProEnd();
    }
    SetOP(old);
    if (CurrProc->prolog_state&4)
        OutDLLExport((CurrProc->parms.size+3)/4,AskForLblSym(CurrProc->label));
}

long StackDepth;
void InitStackDepth(block *blk)
{
    /* Before the later per-block stack_depth field. */
    StackDepth=(blk->edge[0].flags&0x10)?4:0; /* DOWN_ONE_CALL */
}

void AdjustStackDepth(instruction *ins)
{
    name *op;
    long adjust;
    if (!DoesSomething(ins)) return;
    switch (ins->head.opcode) {
    case 1:case 3: /* OP_ADD/OP_SUB */
        if (ins->operands[0]!=ins->result || ins->result->n.class!=3 ||
            ins->result->r.reg!=0x800) return;
        op=ins->operands[1];
        if (op->n.class!=0) { Zoiks(77); return; }
        adjust=op->c.int_value;
        if (ins->head.opcode==3) StackDepth+=adjust;
        else StackDepth-=adjust;
        break;
    case 0x2a: StackDepth+=4; break;
    case 0x2b: StackDepth-=4; break;
    case 0x29:case 0x36:
        if (ins->flags.call_flags&8) {
            op=ins->operands[1]; /* CALL_OP_POPS */
            if (op->n.class==0) StackDepth-=op->c.int_value;
        }
        break;
    }
}

void PatchBigLabels(long stack);
void EmitEpiBeg(void) { STUB_TRAP(); }
void EmitRtnEnd(void) { STUB_TRAP(); }
void PopAll(void) { STUB_TRAP(); }
void GenLeaSP(long value) { STUB_TRAP(); }
void GenLeave(void) { STUB_TRAP(); }
void UnloadDS(void)
{
    if ((CurrProc->state.attr&0x8000) && (CurrProc->state.unalterable&0x1000))
        QuickSave(0x1000,0x2b);
}
void EpilogHook(void) { STUB_TRAP(); }
void GenCypWindowsEpilog(void) { STUB_TRAP(); }
void GenWindowsEpilog(void) { STUB_TRAP(); }
void GenP5ProfilingEpilog(struct code_lbl *label) { STUB_TRAP(); }
void GenReturn(long pop,unsigned char is_long,unsigned char is_interrupt);
void DoEpilog(void)
{
    hw_reg_set to_pop,*reg;
    unsigned char is_long;
    long size;
    if ((CurrProc->state.attr&0x2002)==0x2000) PopAll();
    else {
        to_pop=SaveRegs()&~0x00ff0000U;
        if (CHAIN_FRAME) {
            if ((CurrProc->state.attr&0x20) ||
                CurrProc->locals.size+CurrProc->targ.push_local_size!=0) {
                if (CurrProc->targ.base_adjust==0) GenRegMove(0x400,0x800);
                else GenLeaSP(-CurrProc->targ.base_adjust);
            }
            to_pop&=~0x400U;
        } else if (CurrProc->state.attr&0x20) {
            size=CurrProc->locals.size+CurrProc->targ.push_local_size;
            if ((CurrProc->prolog_state&0x80) || size!=0) {
                if (CurrProc->targ.sp_frame) {
                    if (CurrProc->targ.sp_align) {
                        GenRegMove(0x400,0x800);
                        QuickSave(0x400,0x2b);
                    } else if (size!=0) GenRegAdd(0x800,size);
                } else if (CurrProc->targ.base_adjust!=0)
                    GenLeaSP(-CurrProc->targ.base_adjust);
                else if ((CGProcessorVersion&15)>=1 &&
                         ((CGProcessorVersion&15)<4 || OptForSize>50)) {
                    GenLeave();
                    to_pop&=~0x400U;
                } else GenRegMove(0x400,0x800);
            }
        }
        /* Pop was inlined into this old body; retain its reverse PushRegs
         * walk without introducing a reconstructed helper vertex. */
        reg=PushRegs;
        while (*reg!=0) ++reg;
        while (to_pop!=0) {
            --reg;
            if (*reg&to_pop) {
                QuickSave(*reg,0x2b);
                to_pop&=~*reg;
            }
        }
        UnloadDS();
        if (CHAIN_FRAME && NeedBPProlog()) {
            EpilogHook();
            if (FAR_RET_ON_STACK) {
                if (CHEAP_FRAME) GenCypWindowsEpilog();
                else GenWindowsEpilog();
            } else QuickSave(0x400,0x2b);
        }
    }
    is_long=(CurrProc->state.attr&0x2002)==2 ||
            (CurrProc->state.attr&0x2002)==0x2002;
    if (CurrProc->prolog_state&0x20) QuickSave(0x800,0x2b);
    /* Actual older target has P5_PROFILING only, not the later additional flag. */
    if (TargetModel&0x200000) GenP5ProfilingEpilog(CurrProc->label);
    if ((CurrProc->state.attr&0x2002)==0x2000) GenReturn(0,0,1);
    else if (CurrProc->state.attr&1) GenReturn(CurrProc->parms.size,is_long,0);
    else GenReturn(0,is_long,0);
}
void AbsPatch(void *handle,long value) { STUB_TRAP(); }
void GenEpilog(void)
{
    long stack;
    stack=-CurrProc->locals.size-CurrProc->locals.base-CurrProc->targ.push_local_size;
    PatchBigLabels(stack);
    /* This earlier binary uses the routine debug attribute directly, before
     * the released FE_NAKED test and Model DBG_LOCALS conversion. */
    if (CurrProc->state.attr&0x80) EmitEpiBeg();
    if (!(CurrProc->state.attr&0x10)) DoEpilog();
    if (BlockByBlock) AbsPatch(CurrProc->targ.prolog_loc,CurrProc->locals.size);
    if (CurrProc->targ.stack_check!=NULL)
        AbsPatch(CurrProc->targ.stack_check,CurrProc->locals.size+CurrProc->parms.base+
                 4*CurrProc->lex_level+CurrProc->targ.push_local_size+MaxStack);
    CurrProc->prolog_state|=2;
    if (CurrProc->state.attr&0x80) EmitRtnEnd();
}

unsigned char BaseIsSP(name *op)
{
    if (!CurrProc->targ.sp_frame) return 0;
    if (CurrProc->targ.sp_align && (op->t.temp_flags&0x20)) return 0;
    return 1;
}

long NewBase(name *operand)
{
    if (!BaseIsSP(operand))
        return (long)((unsigned long)operand->t.location-
                      (unsigned long)CurrProc->targ.base_adjust);
    return (long)((unsigned long)operand->t.location+
                  (unsigned long)CurrProc->locals.size+
                  (unsigned long)CurrProc->targ.push_local_size+
                  (unsigned long)StackDepth);
}

long PushSize(long size)
{
    if (size<4) return 4;
    return (long)(((unsigned long)size+3UL)&~3UL);
}
