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
* Description:  Expand and classify x87 stack instructions.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "block.h"
#include "genopt.h"
#include "pragma.h"
#include "stubs.h"
#include "pattern.h"
#include <recon.h>

int Max87Stk;
unsigned char Used87;

void InitFP(void)
{
    Max87Stk=(int)FEAuxInfo(NULL,40);
    if (Max87Stk>8) Max87Stk=8;
    if (Max87Stk<4) Max87Stk=4;
    Used87=0;
    InitFPStkReq();
}

/* i87reg.c's387 intrinsic stack requirements. These nineteen operation
 * roles precede the later P5 division addition. COS/SIN are slots6/7. */
static unsigned char StackReq387[]={
    2, /* POW */
    0, /* ATAN2 */
    0, /* FMOD */
    0, /* NEGATE */
    0, /* COMPLEMENT */
    1, /* LOG */
    1, /* COS */
    1, /* SIN */
    1, /* TAN */
    0, /* SQRT */
    0, /* FABS */
    2, /* ACOS */
    2, /* ASIN */
    1, /* ATAN */
    2, /* COSH */
    2, /* EXP */
    1, /* LOG10 */
    2, /* SINH */
    2  /* TANH */
};

void InitFPStkReq(void)
{
    if (TargetModel&1) {
        StackReq387[7]=0;
        StackReq387[6]=0;
    }
}

int FPRegNum(name *reg_name)
{
    hw_reg_set reg;
    if (reg_name!=NULL && reg_name->n.class==3) {
        reg=reg_name->r.reg&0x00ff0000U;
        switch (reg) {
        case 1U<<16: return 0;
        case 1U<<17: return 1;
        case 1U<<18: return 2;
        case 1U<<19: return 3;
        case 1U<<20: return 4;
        case 1U<<21: return 5;
        case 1U<<22: return 6;
        case 1U<<23: return 7;
        }
    }
    return -1;
}

static name *ST0,*ST1;
/* i86enc.c owns the shared ST(0)..ST(7) encoding table. */
extern hw_reg_set FPRegs[];
name *ST(int reg)
{
    return AllocRegName(FPRegs[reg]);
}
/* Source RFST selector: preserve flags, store/pop a physical x87
 * register, G_RFST48/FU_FOP5; exact6B object at7a5c6. */
static opcode_entry RFST={PRESERVE,0,48,0,5};
instruction *SuffixFSTPRes(instruction *ins);
instruction *ExpCall(instruction *ins)
{
    instruction *new_ins;
    new_ins=ins;
    if (ins->result!=NULL && ins->result->n.class==3 &&
        ins->result->r.reg==0x10000U && (ins->flags.call_flags&4)) {
        new_ins=SuffixFSTPRes(ins);
        new_ins->u.gen_table=&RFST;
    }
    return new_ins;
}
/* gen8087.h's operand/result roles use the earlier six/five-value enums. */
typedef enum fp_operand_type {
    FP_OP_STK0,FP_OP_STK1,FP_OP_STKI,FP_OP_MEM,FP_OP_CONS,FP_OP_NONE
} fp_operand_type;
typedef enum fp_result_type {
    FP_RES_STK0,FP_RES_STKI,FP_RES_MEM,FP_RES_NONE,FP_RES_MEM_THROWAWAY
} fp_result_type;
instruction *SuffFSTPRes(instruction *ins,name *opnd,int result_type);
instruction *SuffixFSTPRes(instruction *ins)
{
    instruction *new_ins;
    name *opnd;
    opnd=ins->result;
    ins->result=ST0;
    new_ins=SuffFSTPRes(ins,opnd,FP_RES_MEM);
    new_ins->stk_entry=ins->stk_exit+1;
    ++ins->stk_exit;
    return new_ins;
}
/* Exact encountered selectors retain old generate/FU identifiers. */
static opcode_entry FCHS={PRESERVE,0,50,0,5};
static opcode_entry FMATH={PRESERVE,0,96,0,9};
static opcode_entry IFUNC={0,0,97,0,9};
void ExpBinFunc(instruction *ins,fp_operand_type op1,fp_operand_type op2) { STUB_TRAP(); }
/* Arithmetic selector triplets are ADD/SUB, MUL, DIV with their real FUs. */
static opcode_entry RNFBIN[]={
    {PRESERVE,0,37,0,6},{PRESERVE,0,37,0,7},{PRESERVE,0,37,0,8}
};
static opcode_entry RNFBINP[]={
    {PRESERVE,0,39,0,6},{PRESERVE,0,39,0,7},{PRESERVE,0,39,0,8}
};
static opcode_entry MRFBIN[]={
    {PRESERVE,0,42,0,6},{PRESERVE,0,42,0,7},{PRESERVE,0,42,0,8}
};
static opcode_entry RRFBINP[]={
    {PRESERVE,0,38,0,6},{PRESERVE,0,38,0,7},{PRESERVE,0,38,0,8}
};
static opcode_entry MNFBIN[]={
    {PRESERVE,0,43,0,6},{PRESERVE,0,43,0,7},{PRESERVE,0,43,0,8}
};
opcode_entry *GenTab(instruction *ins,opcode_entry *array)
{
    switch (ins->head.opcode) {
    case 7: return array+2; /* OP_DIV */
    case 5: return array+1; /* OP_MUL */
    default: return array;
    }
}
void ExchangeOps(instruction *ins)
{
    name *temp;
    temp=ins->operands[0];
    ins->operands[0]=ins->operands[1];
    ins->operands[1]=temp;
}
void PrefixFLDOp(instruction *ins,unsigned short op,int operand);
void ExpBinary(instruction *ins,fp_operand_type op1,fp_operand_type op2)
{
    switch (op1+op2*FP_OP_NONE) {
    case FP_OP_STK1+FP_OP_STK1*FP_OP_NONE:
    case FP_OP_MEM+FP_OP_STK1*FP_OP_NONE:
    case FP_OP_CONS+FP_OP_STK1*FP_OP_NONE:
    case FP_OP_STK1+FP_OP_MEM*FP_OP_NONE:
    case FP_OP_STK1+FP_OP_CONS*FP_OP_NONE:
    case FP_OP_CONS+FP_OP_CONS*FP_OP_NONE:
        Zoiks(10);
        break;
    case FP_OP_STK0+FP_OP_STK0*FP_OP_NONE:
        ins->u.gen_table=GenTab(ins,RNFBIN);
        break;
    case FP_OP_STK1+FP_OP_STK0*FP_OP_NONE:
        ins->u.gen_table=GenTab(ins,RNFBINP);
        break;
    case FP_OP_MEM+FP_OP_STK0*FP_OP_NONE:
        ins->u.gen_table=GenTab(ins,MRFBIN);
        break;
    case FP_OP_CONS+FP_OP_STK0*FP_OP_NONE:
        PrefixFLDOp(ins,(unsigned short)op1,0);
        ins->u.gen_table=GenTab(ins,RRFBINP);
        ins->operands[0]=ST1;
        break;
    case FP_OP_STK0+FP_OP_STK1*FP_OP_NONE:
        ins->u.gen_table=GenTab(ins,RRFBINP);
        ExchangeOps(ins);
        break;
    case FP_OP_STK0+FP_OP_MEM*FP_OP_NONE:
        ins->u.gen_table=GenTab(ins,MNFBIN);
        ExchangeOps(ins);
        break;
    case FP_OP_MEM+FP_OP_MEM*FP_OP_NONE:
    case FP_OP_CONS+FP_OP_MEM*FP_OP_NONE:
        PrefixFLDOp(ins,(unsigned short)op1,0);
        ins->u.gen_table=GenTab(ins,MNFBIN);
        ExchangeOps(ins);
        break;
    case FP_OP_STK0+FP_OP_CONS*FP_OP_NONE:
        PrefixFLDOp(ins,(unsigned short)op2,1);
        ins->u.gen_table=GenTab(ins,RNFBINP);
        ins->operands[0]=ST1;
        ins->operands[1]=ST0;
        break;
    case FP_OP_MEM+FP_OP_CONS*FP_OP_NONE:
        PrefixFLDOp(ins,(unsigned short)op2,1);
        ins->u.gen_table=GenTab(ins,MRFBIN);
        break;
    default:
        Zoiks(11);
        break;
    }
}
void ExpCompare(instruction *ins,fp_operand_type op1,fp_operand_type op2) { STUB_TRAP(); }
instruction *ExpUnary(instruction *ins,fp_operand_type op,
    fp_result_type res,opcode_entry *table) { STUB_TRAP();return NULL; }
/* i87exp.c RC: register/constant subtraction, actual old FU_FOP5. */
static opcode_entry RC={PRESERVE,0,1,0,5};
instruction *ExpMove(instruction *ins,fp_operand_type src,fp_result_type dst);
instruction *ExpPush(instruction *ins,unsigned short op)
{
    instruction *new_ins;
    name *sp,*index;
    int size;
    sp=AllocRegName(0x800U);
    size=TypeClassSize[ins->type_class];
    new_ins=MakeBinary(3,sp,AllocIntConst(size),sp,U4);
    new_ins->u.gen_table=&RC;
    PrefixIns(ins,new_ins);
    index=AllocIndex(sp,NULL,0,ins->type_class);
    new_ins=MakeMove(ins->operands[0],index,ins->type_class);
    ReplIns(ins,new_ins);
    return ExpMove(new_ins,(fp_operand_type)op,FP_RES_MEM);
}
static opcode_entry MFLD={PRESERVE,0,44,0,5};
static opcode_entry MFST={PRESERVE,0,47,0,5};
static opcode_entry FLDZ={PRESERVE,0,52,0,5};
static opcode_entry FLD1={PRESERVE,0,53,0,5};
instruction *PrefFLDOp(instruction *ins,unsigned short type,name *op);
void PrefixFLDOp(instruction *ins,unsigned short op,int operand)
{
    name *opnd;
    instruction *new_ins;
    opnd=ins->operands[operand];
    ins->operands[operand]=ST0;
    new_ins=PrefFLDOp(ins,op,opnd);
    new_ins->stk_exit=new_ins->stk_entry+1;
    ++ins->stk_entry;
    ins->operands[operand]=ST0;
}
static opcode_entry FCHOP={PRESERVE,0,85,0,5};
void PrefixChop(instruction *ins)
{
    instruction *new_ins;
    if (ins->head.opcode==0x25) return; /* OP_ROUND */
    if (ins->result->n.name_class==FS || ins->result->n.name_class==FD) return;
    new_ins=MakeUnary(0x26,ST0,ST0,FD);
    new_ins->u.gen_table=&FCHOP;
    PrefixIns(ins,new_ins);
    new_ins->stk_exit=ins->stk_entry;
}
instruction *ExpMove(instruction *ins,fp_operand_type src,fp_result_type dst)
{
    switch (dst+src*FP_RES_NONE) {
    case FP_RES_STK0+FP_OP_STK1*FP_RES_NONE:
    case FP_RES_MEM+FP_OP_STK1*FP_RES_NONE:
        Zoiks(8);
        break;
    case FP_RES_MEM+FP_OP_MEM*FP_RES_NONE:
        PrefixFLDOp(ins,(unsigned short)src,0);
        PrefixChop(ins);
        DoNothing(ins);
        ins=SuffixFSTPRes(ins);
        break;
    case FP_RES_STK0+FP_OP_STK0*FP_RES_NONE:
        DoNothing(ins);
        break;
    case FP_RES_MEM+FP_OP_STK0*FP_RES_NONE:
        PrefixChop(ins);
        ins->u.gen_table=&MFST;
        break;
    case FP_RES_STK0+FP_OP_MEM*FP_RES_NONE:
        ins->u.gen_table=&MFLD;
        break;
    case FP_RES_STK0+FP_OP_CONS*FP_RES_NONE:
        if (CFTest(ins->operands[0]->c.value)!=0) ins->u.gen_table=&FLD1;
        else ins->u.gen_table=&FLDZ;
        ins->result=ST0;
        break;
    case FP_RES_MEM+FP_OP_CONS*FP_RES_NONE:
        PrefixFLDOp(ins,(unsigned short)src,0);
        DoNothing(ins);
        ins=SuffixFSTPRes(ins);
        break;
    default:
        Zoiks(9);
        break;
    }
    return ins;
}
instruction *ExpandFPIns(instruction *ins,fp_operand_type op1,
    fp_operand_type op2,fp_result_type res)
{
    if (ins->type_class!=FS && ins->type_class!=FD &&
        ins->base_type_class!=FS && ins->base_type_class!=FD) return ins;
    if (op2!=FP_OP_NONE) {
        if (ins->head.opcode>=0x0e && ins->head.opcode<=0x10) {
            ExpBinFunc(ins,op1,op2);
            if (res!=FP_RES_STK0) ins=SuffixFSTPRes(ins);
        } else {
            switch (res) {
            case FP_RES_MEM:
                ExpBinary(ins,op1,op2);
                ins=SuffixFSTPRes(ins);
                break;
            case FP_RES_STK0:
                ExpBinary(ins,op1,op2);
                break;
            case FP_RES_NONE:
                ExpCompare(ins,op1,op2);
                break;
            }
        }
    } else {
        switch (ins->head.opcode) {
        case 0x2a: /* OP_PUSH */
            ins=ExpPush(ins,(unsigned short)op1);
            break;
        case 0x11: /* OP_NEGATE */
            ins=ExpUnary(ins,op1,res,&FCHS);
            break;
        case 0x18: /* OP_FABS */
            ins=ExpUnary(ins,op1,res,&FMATH);
            break;
        case 0x14: case 0x15: case 0x16: case 0x17: case 0x1b:
            if ((CGProcessorVersion&0x70)>=0x20 && (TargetModel&1))
                ins=ExpUnary(ins,op1,res,&FMATH);
            else ins=ExpUnary(ins,op1,res,&IFUNC);
            break;
        case 0x13: case 0x1d: case 0x1e: /* LOG/EXP/LOG10 */
            if ((CGProcessorVersion&0x70)>=0x20 && (TargetModel&1) && OptForSize<=50)
                ins=ExpUnary(ins,op1,res,&FMATH);
            else ins=ExpUnary(ins,op1,res,&IFUNC);
            break;
        case 0x19: case 0x1a: case 0x1c: case 0x1f: case 0x20:
            ins=ExpUnary(ins,op1,res,&IFUNC);
            break;
        case 0x24: case 0x25: case 0x26: /* CONVERT/ROUND/MOV */
            if (ins->result->n.class==3 && ins->result->r.reg==0) {
                ins->result=ST0;
                ins->u.gen_table=&RFST;
            } else ins=ExpMove(ins,op1,res);
            break;
        }
    }
    return ins;
}
instruction *DoExpand(instruction *ins)
{
    int i,reg_num;
    fp_operand_type op1_type,op2_type;
    fp_result_type res_type;
    i=NumOperands(ins);
    /* Zero operands leave op2 undefined in the original source; that domain
     * is not qualified by these floating arithmetic/move reductions. */
    if (i<=0) STUB_TRAP();
    op1_type=FP_OP_NONE;
    while (--i>=0) {
        op2_type=op1_type;
        reg_num=FPRegNum(ins->operands[i]);
        if (reg_num!=-1) {
            if (reg_num==0) op1_type=FP_OP_STK0;
            else {
                op1_type=(fp_operand_type)(ins->stk_entry-reg_num);
                ins->operands[i]=ST(ins->stk_entry-reg_num);
            }
        } else if (ins->operands[i]->n.class==0) op1_type=FP_OP_CONS;
        else op1_type=FP_OP_MEM;
    }
    if (ins->head.opcode>=0x2e && ins->head.opcode<=0x35)
        res_type=FP_RES_NONE;
    else {
        reg_num=FPRegNum(ins->result);
        if (reg_num!=-1) {
            res_type=FP_RES_STK0;
            ins->result=ST0;
        } else res_type=FP_RES_MEM;
    }
    return ExpandFPIns(ins,op1_type,op2_type,res_type);
}
void FPExpand(void)
{
    block *blk;
    instruction *ins;
    if ((CGProcessorVersion&0x70U)<0x10U) return;
    ST0=ST(0);
    ST1=ST(1);
    /* Expand is inlined in this image; retain its loop without a new vertex. */
    blk=HeadBlock;
    while (blk!=NULL) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            if (DoesSomething(ins)) {
                if (ins->head.opcode==0x36 || ins->head.opcode==0x29)
                    ins=ExpCall(ins);
                else if (ins->u.gen_table->generate==103)
                    ins=DoExpand(ins);
            }
            ins=ins->head.next;
        }
        blk=blk->next_block;
    }
}

/* Older8087 intrinsic requirements: no later P5DIV operation slot. */
static unsigned char StackReq8087[]={
    2, /* POW */
    0, /* ATAN2 */
    0, /* FMOD */
    0, /* NEGATE */
    0, /* COMPLEMENT */
    1, /* LOG */
    3, /* COS */
    3, /* SIN */
    2, /* TAN */
    0, /* SQRT */
    0, /* FABS */
    2, /* ACOS */
    2, /* ASIN */
    1, /* ATAN */
    2, /* COSH */
    2, /* EXP */
    1, /* LOG10 */
    2, /* SINH */
    2  /* TANH */
};
int FPStkReq(instruction *ins)
{
    if (ins->head.opcode<0xe || ins->head.opcode>0x20) return 0;
    if ((CGProcessorVersion&0x70U)>=0x20U) return StackReq387[ins->head.opcode-0xe];
    return StackReq8087[ins->head.opcode-0xe];
}

unsigned char FPStackReg(name *op)
{
    int reg_num;
    reg_num=FPRegNum(op);
    return reg_num!=-1 && reg_num<Max87Stk;
}

static opcode_entry RFLD={PRESERVE,0,45,0,5};
void MoveSegOp(instruction *ins,instruction *new_ins,int operand);
instruction *PrefFLDOp(instruction *ins,unsigned short type,name *op)
{
    instruction *new_ins;
    switch (type) {
    case FP_OP_STK0: case FP_OP_STKI:
        new_ins=MakeUnary(0x26,op,ST0,FD);
        new_ins->u.gen_table=&RFLD;
        break;
    case FP_OP_MEM:
        new_ins=MakeUnary(0x26,op,ST0,FD);
        new_ins->u.gen_table=&MFLD;
        MoveSegOp(ins,new_ins,0);
        break;
    case FP_OP_CONS:
        new_ins=MakeUnary(0x26,op,ST0,FD);
        if (CFTest(op->c.value)) new_ins->u.gen_table=&FLD1;
        else new_ins->u.gen_table=&FLDZ;
        break;
    default:
        /* The original switch leaves its instruction owner undefined. */
        STUB_TRAP();
        return NULL;
    }
    PrefixIns(ins,new_ins);
    return new_ins;
}

/* Distinct table identity permits the scheduler's later throwaway rewrite. */
static opcode_entry MFST2={PRESERVE,0,47,0,5};
void MoveSegRes(instruction *ins,instruction *new_ins);
instruction *SuffFSTPRes(instruction *ins,name *op,int result_type)
{
    instruction *new_ins;
    new_ins=MakeUnary(0x26,ST0,op,FD);
    if (result_type==FP_RES_STKI) new_ins->u.gen_table=&RFST;
    else if (result_type==FP_RES_MEM_THROWAWAY) new_ins->u.gen_table=&MFST2;
    else new_ins->u.gen_table=&MFST;
    if (ins->head.opcode!=0x4b) MoveSegRes(ins,new_ins);
    SuffixIns(ins,new_ins);
    return new_ins;
}

unsigned char FPResultNotNeeded(instruction *ins)
{
    return ins->u.gen_table==&MFST2;
}
void ToRFstp(instruction *ins) { STUB_TRAP(); }

instruction *PrefFXCH(instruction *ins,int actual) { STUB_TRAP();return NULL; }

void ToPopBin(instruction *ins)
{
    if (ins->u.gen_table->generate==36) {
        ins->u.gen_table=GenTab(ins,RNFBINP);
        --ins->stk_exit;
    } else if (ins->u.gen_table->generate==37) {
        ins->u.gen_table=GenTab(ins,RRFBINP);
        --ins->stk_exit;
    }
}
