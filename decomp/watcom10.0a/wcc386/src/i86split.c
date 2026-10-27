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
* Description:  Target reduction callback identities from intel/h/r.h.
*
****************************************************************************/


#include "block.h"
#include "options.h"
#include "codegen.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

typedef instruction *(*reduction)(instruction *);
instruction *rCHANGESHIFT(instruction *ins);
instruction *rCLRHI_BW(instruction *ins);
instruction *rCONVERT_LOW(instruction *ins);
instruction *rCYPHIGH(instruction *ins);
instruction *rCYPLOW(instruction *ins);
instruction *rDOCVT(instruction *ins);
instruction *rDOLONGPUSH(instruction *ins);
instruction *rDOUBLEHALF(instruction *ins);
instruction *rMAKECALL(instruction *ins);
instruction *rOP1CMEM(instruction *ins);
instruction *rOP1MEM(instruction *ins);
instruction *rOP2CMEM(instruction *ins);
instruction *rFORCERESMEM(instruction *ins);
instruction *rFSCONSCMP(instruction *ins);
instruction *rHIGHCMP(instruction *ins);
instruction *rHIGHLOWMOVE(instruction *ins);
instruction *rLOADLONGADDR(instruction *ins);
instruction *rMAKECYPMUL(instruction *ins);
instruction *rMAKEFNEG(instruction *ins);
instruction *rMAKEMOVE(instruction *ins);
instruction *rMAKESTRCMP(instruction *ins);
instruction *rMAKESTRMOVE(instruction *ins);
instruction *rMAKEU2(instruction *ins);
instruction *rMAKEU4(instruction *ins);
instruction *rMAKEXORRR(instruction *ins);
instruction *rMOVEINDEX(instruction *ins);
instruction *rMOVELOW(instruction *ins);
instruction *rMOVOP1TEMP(instruction *ins);
instruction *rMOVOP2(instruction *ins);
instruction *rMOVOP2TEMP(instruction *ins);
instruction *rMOVOP1MEM(instruction *ins);
instruction *rOP1REG(instruction *ins);
instruction *rMOVOP1RES(instruction *ins);
instruction *rRESREG(instruction *ins);
instruction *rOP2CL(instruction *ins);
instruction *rOP2CX(instruction *ins);
instruction *rOP1RESTEMP(instruction *ins);
instruction *rSPLIT8(instruction *ins);
instruction *rSPLITMOVE(instruction *ins);
instruction *rSPLITUNARY(instruction *ins);
instruction *rSWAPCMP(instruction *ins);
instruction *rSWAPOPS(instruction *ins);
instruction *rUSEREGISTER(instruction *ins);
instruction *rMULREGISTER(instruction *ins);
instruction *rDIVREGISTER(instruction *ins);
instruction *rOP1RESREG(instruction *ins);
instruction *rLOADOP2(instruction *ins);
/* Encountered prefix of the released named reduction table. The actual
 * 1994 DWORD callbacks7bda0..7bf04 agree, including the shared MAKECALL
 * and byte/word CLRHI vertices. Unimplemented entries have genuine traps. */
instruction *rMAKEADD(instruction *ins);
instruction *rMAKENEG(instruction *ins);
instruction *rMAKESUB(instruction *ins);
instruction *rU_TEST(instruction *ins);
instruction *rCMPTRUE(instruction *ins);
instruction *rCMPFALSE(instruction *ins);
instruction *rCPSUB(instruction *ins);
instruction *rPTSUB(instruction *ins);
instruction *rNEGADD(instruction *ins);
instruction *rEXTPT(instruction *ins);
instruction *rMAYBSTRMOVE(instruction *ins);
instruction *rOP2MEM(instruction *ins);
instruction *rSEG_SEG(instruction *ins);
instruction *rCHPPT(instruction *ins);
instruction *rCLRHI_D(instruction *ins);
instruction *rMOVRESMEM(instruction *ins);
instruction *rMAKEU4CONS(instruction *ins);
instruction *rADDRR(instruction *ins);
instruction *rSPLITPUSH(instruction *ins);
instruction *rEXT_PUSHC(instruction *ins);
instruction *rEXT_PUSH1(instruction *ins);
instruction *rEXT_PUSH2(instruction *ins);
instruction *rCLRHI_R(instruction *ins);
instruction *rMOVOP2RES(instruction *ins);
instruction *rINTCOMP(instruction *ins);
instruction *rCDQ(instruction *ins);
instruction *rCYP_SEX(instruction *ins);
instruction *rFLIPSIGN(instruction *ins);
instruction *rTEMP2CONST(instruction *ins);
reduction ReduceTab[89]={
    rCHANGESHIFT,
    rCLRHI_BW,
    rCLRHI_BW,
    rCONVERT_LOW,
    rCYPHIGH,
    rCYPLOW,
    rDOCVT,
    rDOLONGPUSH,
    rDOUBLEHALF,
    rMAKECALL,
    rOP1CMEM,
    rOP1MEM,
    rOP2CMEM,
    rFORCERESMEM,
    rFSCONSCMP,
    rHIGHCMP,
    rHIGHLOWMOVE,
    rLOADLONGADDR,
    rMAKECALL,
    rMAKECALL,
    rMAKECYPMUL,
    rMAKEFNEG,
    rMAKEMOVE,
    rMAKESTRCMP,
    rMAKESTRMOVE,
    rMAKEU2,
    rMAKEU4,
    rMAKEXORRR,
    rMOVEINDEX,
    rMOVELOW,
    rMOVOP1TEMP,
    rMOVOP2,
    rMOVOP2TEMP,
    rMOVOP1MEM,
    rOP1REG,
    /* Actual folded entries retain one physical representative: MOVRESREG
     * and RESREG; SPLITCMP/OP/NEG/BYTESHIFT/CYPSHIFT all use rSPLIT8. */
    rMOVOP1RES,
    rRESREG,
    rOP2CL,
    rOP2CX,
    rOP1RESTEMP,
    rRESREG,
    rSPLIT8,
    rSPLIT8,
    rSPLITMOVE,
    rSPLIT8,
    rSPLITUNARY,
    rSWAPCMP,
    rSWAPOPS,
    rUSEREGISTER,
    rMULREGISTER,
    rDIVREGISTER,
    rOP1RESREG,
    rSPLIT8,
    rSPLIT8,
    rSPLIT8,
    rLOADOP2,
    rMAKEADD,
    rMAKENEG,
    rMAKESUB,
    rU_TEST,
    rCMPTRUE,
    rCMPFALSE,
    NULL,
    NULL,
    NULL,
    NULL,
    rCPSUB,
    rPTSUB,
    rNEGADD,
    rEXTPT,
    rMAYBSTRMOVE,
    rOP2MEM,
    rSEG_SEG,
    rCHPPT,
    rCLRHI_D,
    rMOVRESMEM,
    rMAKEU4CONS,
    rADDRR,
    rSPLITPUSH,
    rEXT_PUSHC,
    rEXT_PUSH1,
    rEXT_PUSH2,
    rCLRHI_R,
    rMOVOP2RES,
    rINTCOMP,
    rCDQ,
    rCYP_SEX,
    rFLIPSIGN,
    rTEMP2CONST
};
instruction *rCHANGESHIFT(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCLRHI_BW(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCONVERT_LOW(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCYPHIGH(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCYPLOW(instruction *ins);

instruction *SplitPush(instruction *ins,long size)
{
    instruction *new_ins,*first_ins;
    name *op,*new_op;
    size=(size+3)&~3L;
    op=ins->operands[0];
    first_ins=NULL;
    for (;;) {
        size-=4;
        switch (op->n.class) {
        case 1:
            new_op=AllocMemory(op->v.symbol,op->v.offset+size,op->m.memory_type,U4);
            break;
        case 2:
            /* This body predates the later CG_INTRODUCED flag write. */
            new_op=TempOffset(op,size,U4);
            break;
        case 4:
            new_op=ScaleIndex(op->i.index,op->i.base,op->i.constant+size,
                             U4,0,op->i.scale,op->i.index_flags);
            break;
        default:
            Zoiks(51);
            STUB_TRAP(); /* No initialized new_op on the original error path. */
            return NULL;
        }
        new_ins=MakeUnary(0x2a,new_op,NULL,U4);
        if (size==0) break;
        if (first_ins==NULL) first_ins=new_ins;
        DupSeg(ins,new_ins);
        PrefixIns(ins,new_ins);
    }
    DupSeg(ins,new_ins);
    ReplIns(ins,new_ins);
    if (first_ins==NULL) return new_ins;
    return first_ins;
}
instruction *LoadStringOps(instruction *ins,name **op1,name **op2);
instruction *rDOLONGPUSH(instruction *ins)
{
    name *sp,*at_sp,*temp;
    long size;
    instruction *move,*sub_sp,*temp_ins;
    size=ins->operands[0]->n.size;
    if (size<=16) return SplitPush(ins,size);
    sp=AllocRegName(0x800U); /* HW_SP */
    if (TargetModel&0x200U) {
        temp=AllocTemp(CP);
        temp_ins=MakeMove(AllocRegName(0x8800U),temp,CP); /* SS:SP */
    } else {
        temp=AllocTemp(U4);
        temp_ins=MakeMove(sp,temp,U4);
    }
    at_sp=AllocIndex(temp,NULL,0,XX);
    at_sp->n.size=size;
    move=MakeMove(ins->operands[0],at_sp,XX);
    ReplIns(ins,move);
    ins=LoadStringOps(move,&move->operands[0],&move->result);
    sub_sp=MakeBinary(3,sp,AllocIntConst((size+3)&~3L),sp,U4);
    if (ins->result!=NULL && ins->result->r.reg==0x04000030U)
        SuffixIns(ins,sub_sp); /* HW_CX */
    else {
        PrefixIns(ins,sub_sp);
        ins=sub_sp;
    }
    SuffixIns(sub_sp,temp_ins);
    return ins;
}
instruction *rDOUBLEHALF(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMAKECALL(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rOP1CMEM(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rOP1MEM(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rOP2CMEM(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rFORCERESMEM(instruction *ins);
instruction *rFSCONSCMP(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rHIGHCMP(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rHIGHLOWMOVE(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rLOADLONGADDR(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMAKECYPMUL(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMAKEFNEG(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMAKEMOVE(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMAKESTRCMP(instruction *ins) { STUB_TRAP();return NULL; }
unsigned char UseRepForm(unsigned size)
{
    unsigned count,extra,startup,iteration,move_cost,cpu;
    count=size/4;
    if (count>10) return 1;
    if (OptForSize>50) {
        switch (size%4) {
        case 0: extra=0;break;
        case 1: case 2: extra=1;break;
        default: extra=2;break;
        }
        return count+extra>7;
    }
    cpu=CGProcessorVersion&15;
    if (cpu>=5) { startup=11;iteration=1;move_cost=4; }
    else if (cpu>=4) { startup=12;iteration=3;move_cost=7; }
    else if (cpu>=3) { startup=8;iteration=4;move_cost=8; }
    else if (cpu>=2) { startup=5;iteration=4;move_cost=5; }
    else { startup=9;iteration=17;move_cost=18; }
    return startup+count*iteration+4<move_cost*count;
}
name *SegName(name *value);
unsigned char SegmentFloats(name *value)
{
    name *segment;
    segment=SegName(value);
    if (segment->n.class!=3) return 1;
    if (segment->r.reg&HW_DS) return (TargetModel&FLOATING_DS)!=0;
    if (segment->r.reg&0x8000U) return (TargetModel&0x200U)!=0; /* SS. */
    return 1;
}
name *FakeIndex(name *value,hw_reg_set index)
{
    name *base;
    unsigned char flags;
    if (value->n.class==2 || value->n.class==1) {
        base=value;
        flags=0x10; /* X_FAKE_BASE. */
    } else { base=NULL;flags=0; }
    return ScaleIndex(AllocRegName(index),base,0,value->n.name_class,
                       value->n.size,0,flags);
}
instruction *MoveConst(unsigned long value,name *dest,unsigned char type_class)
{ STUB_TRAP();return NULL; }
void DelSeg(instruction *ins);
void UpdateLive(instruction *first,instruction *last);
extern opcode_entry String[];
#define STRING_DI 0x20000200U
#define STRING_ES_DI (HW_ES|STRING_DI)
#define STRING_DS_SI (HW_DS|HW_ESI)
instruction *LoadStringOps(instruction *ins,name **op1,name **op2)
{
    instruction *load_op1,*load_op2,*load_len,*first_ins,*pop;
    name *ds_reg,*es_reg;
    hw_reg_set new_op1;
    unsigned char ds_needs_save,es_needs_save;
    if (ins->head.opcode==0x26 && !UseRepForm((*op1)->n.size)) {
        load_len=NULL;
        new_op1=0;
    } else {
        if (((*op1)->n.size&3)==0 || (OptForSize<=50 && ins->head.opcode==0x26))
            load_len=MoveConst((*op1)->n.size/4,AllocRegName(HW_ECX),U4);
        else load_len=MoveConst((*op1)->n.size,AllocRegName(HW_ECX),U4);
        PrefixIns(ins,load_len);
        new_op1=HW_ECX;
    }
    /* Load DS last; preserve actual operand addresses through each insertion. */
    if (ins->num_operands>NumOperands(ins)) {
        if ((*op1)->n.class==4 || (*op1)->n.class==1) {
            load_op1=MakeUnary(0x28,*op2,AllocRegName(STRING_ES_DI),CP);
            PrefixIns(ins,load_op1);
            load_op2=MakeUnary(0x28,*op1,AllocRegName(HW_ESI),U4);
            PrefixIns(ins,load_op2);
            load_op2=MakeMove(ins->operands[ins->num_operands-1],AllocRegName(HW_DS),U2);
            PrefixIns(ins,load_op2);
            es_needs_save=SegmentFloats(*op2);
            ds_needs_save=1;
        } else {
            load_op1=MakeMove(ins->operands[ins->num_operands-1],AllocRegName(HW_ES),U2);
            PrefixIns(ins,load_op1);
            load_op2=MakeUnary(0x28,*op2,AllocRegName(STRING_DI),U4);
            PrefixIns(ins,load_op2);
            load_op2=MakeUnary(0x28,*op1,AllocRegName(STRING_DS_SI),CP);
            PrefixIns(ins,load_op2);
            ds_needs_save=SegmentFloats(*op1);
            es_needs_save=1;
        }
        DelSeg(ins);
    } else if ((TargetModel&0x20) && !SegmentFloats(*op1) && !SegmentFloats(*op2)) {
        load_op1=MakeUnary(0x28,*op2,AllocRegName(STRING_DI),U4);
        PrefixIns(ins,load_op1);
        load_op2=MakeUnary(0x28,*op1,AllocRegName(HW_ESI),U4);
        PrefixIns(ins,load_op2);
        ds_needs_save=es_needs_save=0;
    } else {
        load_op1=MakeUnary(0x28,*op2,AllocRegName(STRING_ES_DI),CP);
        PrefixIns(ins,load_op1);
        load_op2=MakeUnary(0x28,*op1,AllocRegName(STRING_DS_SI),CP);
        PrefixIns(ins,load_op2);
        ds_needs_save=SegmentFloats(*op1);
        es_needs_save=SegmentFloats(*op2);
    }
    if (!(TargetModel&FLOATING_DS) && ds_needs_save) {
        ds_reg=AllocRegName(HW_DS);
        pop=MakeUnary(0x2b,NULL,ds_reg,U2);
        pop->num_operands=0;
        SuffixIns(ins,pop);
        PrefixIns(load_op2,MakeUnary(0x2a,ds_reg,NULL,U2));
    }
    if (!(TargetModel&FLOATING_ES) && es_needs_save) {
        es_reg=AllocRegName(HW_ES);
        pop=MakeUnary(0x2b,NULL,es_reg,U2);
        pop->num_operands=0;
        SuffixIns(ins,pop);
        PrefixIns(load_op1,MakeUnary(0x2a,es_reg,NULL,U2));
        load_op1=load_op1->head.prev;
    }
    new_op1|=STRING_DS_SI|STRING_ES_DI;
    ins->table=String;
    ins->head.state=0;
    *op1=FakeIndex(ins->operands[0],new_op1);
    ins->operands[1]=ins->operands[0];
    new_op1&=~(0x8000U|0x800U|HW_ES|HW_DS); /* SS/SP/ES/DS. */
    ins->zap=&AllocRegName(new_op1)->r;
    if (ins->head.opcode==0x26) ins->result=FakeIndex(ins->result,STRING_ES_DI);
    else ins->result=NULL;
    first_ins=load_len!=NULL?load_len:load_op1;
    UpdateLive(first_ins,ins);
    return first_ins;
}
instruction *rMAKESTRMOVE(instruction *ins)
{
    return LoadStringOps(ins,&ins->operands[0],&ins->result);
}
instruction *rMAKEU2(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMAKEU4(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMAKEXORRR(instruction *ins);
instruction *rMOVEINDEX(instruction *ins);
instruction *rMOVELOW(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMOVOP1TEMP(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMOVOP2(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMOVOP2TEMP(instruction *ins);
instruction *rMOVOP1MEM(instruction *ins) { STUB_TRAP();return NULL; }

extern opcode_entry String[];
unsigned char UnChangeable(instruction *ins)
{
    return ins->table==String;
}


instruction *rRESREG(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rOP2CL(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rOP2CX(instruction *ins) { STUB_TRAP();return NULL; }

instruction *rSPLIT8(instruction *ins);
instruction *rSPLITMOVE(instruction *ins);
instruction *rSPLITUNARY(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rUSEREGISTER(instruction *ins);
instruction *rMULREGISTER(instruction *ins) { STUB_TRAP();return NULL; }
void MoveSegOp(instruction *ins,instruction *new_ins,int duplicate);
void MoveSegRes(instruction *ins,instruction *new_ins);
hw_reg_set Op1Reg(instruction *ins);
hw_reg_set ResultReg(instruction *ins);
hw_reg_set ZapReg(instruction *ins);
/* split.c DoubleClass before U8/I8 were introduced: actual11-byte mapping. */
static unsigned char DoubleClass[]={U2,I2,U4,I4,XX,XX,0,0,0,0,XX};
instruction *rDIVREGISTER(instruction *ins)
{
    instruction *new_ins,*move;
    name *first,*result;
    if (ins->type_class>XX) STUB_TRAP();
    first=AllocRegName(Op1Reg(ins));
    new_ins=MakeConvert(ins->operands[0],first,DoubleClass[ins->type_class],ins->type_class);
    ins->operands[0]=first;
    MoveSegOp(ins,new_ins,0);
    PrefixIns(ins,new_ins);
    result=AllocRegName(ResultReg(ins));
    move=MakeMove(result,ins->result,ins->type_class);
    ins->result=result;
    ins->zap=AllocRegName(ZapReg(ins));
    MoveSegRes(ins,move);
    SuffixIns(ins,move);
    return new_ins;
}
instruction *rOP1RESREG(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rLOADOP2(instruction *ins);

extern opcode_entry Move2CC[];
void CheckCC(instruction *ins,instruction *new_ins)
{
    if (ins->head.opcode==2 || ins->head.opcode==4) {
        /* The actual1994 CheckCC uses Move2 at7b792, rather than the
         * later386 Move4 selection. Skip its zero-to-XOR row. */
        new_ins->table=Move2CC+1;
        new_ins->ins_flags|=0x20;
    }
}

instruction *rMAKEADD(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMAKENEG(instruction *ins) { STUB_TRAP();return NULL; }


instruction *rU_TEST(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCMPTRUE(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCMPFALSE(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCPSUB(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rPTSUB(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rNEGADD(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rEXTPT(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMAYBSTRMOVE(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rOP2MEM(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rSEG_SEG(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCHPPT(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCLRHI_D(instruction *ins);
instruction *rMOVRESMEM(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMAKEU4CONS(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rADDRR(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rSPLITPUSH(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rEXT_PUSHC(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rEXT_PUSH1(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rEXT_PUSH2(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCLRHI_R(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rMOVOP2RES(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rINTCOMP(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCDQ(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rCYP_SEX(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rFLIPSIGN(instruction *ins) { STUB_TRAP();return NULL; }
instruction *rTEMP2CONST(instruction *ins)
{
    int i;
    name *op;
    for (i=ins->num_operands-1;i>=0;--i) {
        op=ins->operands[i];
        if (op->n.class==2 && (op->t.temp_flags&0x100))
            ins->operands[i]=(name *)op->v.symbol;
    }
    return ins;
}
