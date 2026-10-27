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


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "procdef.h"
#include "block.h"
#include "address.h"
#include "callnode.h"
#include "stubs.h"
#include <recon.h>

unsigned SaveModel;

void BGProcDecl(void *symbol,type_def *type)
{
    unsigned char type_class;
    unsigned short old;
    struct code_lbl *label;
    name *temp;
    hw_reg_set reg;
    SaveModel=TargetModel;
    type_class=AddCallBlock(symbol,type);
    if (type!=TypeNone && type_class==XX) {
        if (CurrProc->state.attr&0x40) {
            old=SetOP(AskBackSeg());
            label=AskForNewLabel();
            DataLabel(label);
            DGUBytes(type->length);
            CurrProc->targ.return_points=SAllocMemory(label,0,1,TypeClass(type),type->length);
            SetOP(old);
        } else {
            reg=CurrProc->state.return_reg;
            if (reg==0) {
                temp=DoParmDecl(NULL,TypeInteger,0);
            } else {
                temp=AllocTemp(U4);
                temp->v.usage|=2;
                AddIns(MakeMove(AllocRegName(reg),temp,U4));
                CurrProc->state.parm.used|=reg;
            }
            CurrProc->targ.return_points=temp;
        }
    }
    if ((CurrProc->state.attr&0x2002)==0x2000) TargetModel|=0x200;
}

void InitTargProc(void)
{
    CurrProc->targ.stack_check=NULL;
    CurrProc->targ.push_local_size=0;
    CurrProc->targ.debug=NULL;
    CurrProc->targ.return_points=NULL;
    CurrProc->targ.sp_frame=0;
    CurrProc->targ.sp_align=0;
    CurrProc->targ.has_fd_temps=0;
    CurrProc->targ.never_sp_frame=0;
}

unsigned char CallIPossible(instruction *ins)
{
    if (ins->operands[2]->n.name_class==CP ||
        ins->operands[2]->n.name_class==PT) return 0; /* RL_ */
    return 8; /* Actual1994 near-call register-set index. */
}

name *OffsetPart(name *value);
name *SegmentPart(name *value);
name *SAllocIndex(name *index,name *base,long offset,unsigned char class,unsigned long size);
name *StReturn(struct address_name *value,type_def *type,instruction **last)
{
    name *result,*ptr,*off,*seg;
    hw_reg_set reg;
    if (CurrProc->state.attr&0x40) {
        result=CurrProc->targ.return_points;
        AddIns(MakeUnary(0x28,result,AllocRegName(CurrProc->state.return_reg),U4));
        *last=NULL;
    } else {
        if (TargetModel&0x600) {
            ptr=AllocTemp(CP);
            off=OffsetPart(ptr);
            seg=SegmentPart(ptr);
            AddIns(MakeMove(AllocRegName(0x8000),seg,U2)); /* SS */
        } else {
            ptr=AllocTemp(U4);
            off=ptr;
        }
        AddIns(MakeMove(CurrProc->targ.return_points,off,U4));
        result=SAllocIndex(ptr,NULL,0,TypeClass(value->tipe),type->length);
        reg=ReturnReg(U4,0);
        *last=MakeMove(CurrProc->targ.return_points,AllocRegName(reg),U4);
        CurrProc->state.return_reg=reg;
    }
    return result;
}

type_def *PassParmType(void *symbol,type_def *type,unsigned call_class)
{
    if (call_class&0x40000000U) return type; /* FAR16_CALL. */
    return QParmType(symbol,NULL,type);
}

void SaveDisplay(unsigned char op);
void Far16Parms(struct call_node *call);
unsigned char AssgnParms(struct call_node *call,unsigned char in_line);
void AddCall(instruction *ins,struct call_node *call);
void FreeCallNode(struct call_node *call);
struct address_name *MakeTempAddr(name *value,type_def *type);
name *SAllocIndex(name *index,name *base,long offset,unsigned char type_class,unsigned long size);
struct address_name *BGCall(struct call_node *call,unsigned char use_return,unsigned char in_line)
{
    instruction *call_ins,*ret_ins;
    call_state *state;
    name *result,*ret_ptr,*temp,*reg_name;
    hw_reg_set return_reg,zap_reg;
    type_def *result_type;
    if (call->name->tipe==TypeProcParm) SaveDisplay(0x2a); /* OP_PUSH. */
    state=call->state;
    result=BGNewTemp(call->tipe);
    call_ins=call->ins;
    if (call_ins->type_class==XX) {
        if ((state->attr&0x2002)==0x2002) {
            if (state->attr&0x40) state->return_reg=0x01000003; /* HW_EAX. */
            else state->return_reg=0x0200000c; /* HW_EBX. */
        }
        if (!(state->attr&0x40)) { /* ROUTINE_ALLOCS_RETURN. */
            if (state->return_reg==0) ret_ptr=AllocTemp(U4);
            else ret_ptr=AllocRegName(state->return_reg);
            ret_ins=MakeUnary(0x28,result,ret_ptr,U4);
            state->parm.used|=state->return_reg;
            call_ins->flags.call_flags|=0x20; /* CALL_RETURNS_STRUCT. */
        }
    }
    if ((TargetModel&0x400) && (state->attr&4)) state->parm.used|=0x1000;
    if ((state->attr&0x2002)==0x2002) Far16Parms(call);
    else if (AssgnParms(call,in_line) && (state->attr&1))
        call_ins->flags.call_flags|=8; /* CALL_POPS_PARMS. */
    if (state->attr&0x0810) call_ins->flags.call_flags|=2;
    if (state->attr&0x1000) call_ins->flags.call_flags|=1;
    if (state->attr&0x10) call_ins->flags.call_flags|=0x80;
    if ((state->attr&0x2002)==0x2000) call_ins->flags.call_flags|=0x18;
    if (!use_return) call_ins->flags.call_flags|=4;
    if (call_ins->type_class==XX) {
        reg_name=AllocRegName(state->return_reg);
        if (state->attr&0x40) {
            call_ins->result=reg_name;
            AddCall(call_ins,call);
            if (use_return) {
                temp=AllocTemp(U4);
                AddIns(MakeMove(reg_name,temp,U4));
                temp=SAllocIndex(temp,NULL,0,result->n.name_class,call->tipe->length);
                AddIns(MakeMove(temp,result,result->n.name_class));
            }
        } else {
            call_ins->result=result;
            AddIns(ret_ins);
            if (state->return_reg==0) {
                AddIns(MakeUnary(0x2a,ret_ptr,NULL,U4));
                state->parm.offset+=TypeClassSize[U4];
                call_ins->operands[1]=AllocS32Const(call_ins->operands[1]->c.int_value+
                                                    TypeClassSize[U4]);
                if (state->attr&1) call_ins->flags.call_flags|=8;
            }
            AddCall(call_ins,call);
        }
    } else {
        return_reg=state->return_reg;
        zap_reg=call_ins->zap->reg|0x00ff0000; /* HW_FLTS. */
        return_reg&=zap_reg;
        call_ins->result=AllocRegName(return_reg);
        reg_name=AllocRegName(state->return_reg);
        AddCall(call_ins,call);
        if (use_return) {
            ret_ins=MakeMove(reg_name,result,result->n.name_class);
            if (reg_name->r.reg&0x00ff0000) {
                ret_ins->stk_entry=1;
                ret_ins->stk_exit=0;
            }
            AddIns(ret_ins);
        }
    }
    if (state->parm.offset!=0 && !(state->attr&1)) {
        reg_name=AllocRegName(0x800); /* HW_SP. */
        AddIns(MakeBinary(1,reg_name,AllocS32Const(state->parm.offset),reg_name,U4));
    }
    /* Retain the typed result before FreeCallNode releases the owner. The
     * original reads its unchanged tipe field afterward; no host freed-owner
     * access or allocator-lifetime equivalence is assumed here. */
    result_type=call->tipe;
    FreeCallNode(call);
    return MakeTempAddr(result,result_type);
}

extern unsigned char ClassPointer;
name *NearSegment(void);
void SetDisplay(name *value);
void AddCallIns(instruction *ins,struct call_node *call);
void AddCall(instruction *ins,struct call_node *call)
{
    name *proc_name;
    if ((TargetModel&0x400) && (call->state->attr&4))
        AddIns(MakeMove(NearSegment(),AllocRegName(0x1000),U2));
    if (call->name->tipe==TypeProcParm) {
        proc_name=AllocTemp(ClassPointer);
        AddIns(MakeMove(ins->operands[2],proc_name,ClassPointer));
        ins->operands[2]=proc_name;
        SetDisplay(GenIns(call->name));
        AddIns(ins);
        SaveDisplay(0x2b); /* OP_POP. */
    } else AddCallIns(ins,call);
}
void SetDisplay(name *value) { STUB_TRAP(); }

/* i86call.c PreCall and PostCall are both empty in the released source.
 * The 1994 target folds the two roles into this same physical entry. */
void PreCall(struct call_node *call) { }

void PushInSameBlock(instruction *ins)
{
    while (ins->head.opcode!=0x4b) ins=ins->head.next;
    if ((block *)ins->table!=CurrBlock) CurrProc->targ.never_sp_frame=1;
}

/* The1994 ABI uses a fourth pushed-byte owner, before the later unused
 * offset/call-state formals. Its ring splice is inlined, not SuffixIns. */
long PushSize(long size);
instruction *PushOneParm(instruction *ins,name *value,unsigned char type_class,
    unsigned *pushed_bytes)
{
    instruction *new_ins;
    unsigned size;
    new_ins=MakeUnary(0x2a,value,NULL,type_class);
    new_ins->head.line_num=0;
    new_ins->head.next=ins->head.next;
    new_ins->head.prev=ins;
    ins->head.next->head.prev=new_ins;
    ins->head.next=new_ins;
    if (value->n.class==0) size=TypeClassSize[type_class];
    else size=value->n.size;
    *pushed_bytes+=PushSize(size);
    return new_ins;
}
