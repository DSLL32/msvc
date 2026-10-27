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
#include "options.h"
#include "encode.h"
#include "memory.h"
#include "genopt.h"
#include "generate-code.h"
#include "i86opcode.h"
#include "stubs.h"
#include <recon.h>

/* pcencode.h instruction template, encountered by the real _Code path. */
typedef struct code_template { oc_header oc;unsigned char data[128]; } code_template;
code_template Temp;
unsigned char Inst[128],FPPatchType;
static int ICur,IEsc;
int ILen;
void ReFormat(unsigned char kind);
void Format(unsigned char kind)
{
    FPPatchType=0;
    Temp.oc.objlen=0;
    Temp.oc.reclen=sizeof(oc_header);
    ICur=0;
    ILen=0;
    IEsc=0;
    ReFormat(kind);
}
/* Separate 1994 entry: Format falls through here; indirect calls enter here. */
void ReFormat(unsigned char kind)
{
    Temp.oc.kind=kind;
}
void PushSeg(hw_reg_set reg) { STUB_TRAP(); }
void PopSeg(hw_reg_set reg) { STUB_TRAP(); }
/* Initial i86enc.c's three register widths in x86 encoding order. */
enum { HW_AL=2,HW_AX=3,HW_CL=0x20,HW_CX=0x30,HW_DL=0x80,HW_DX=0xc0,
       HW_BL=8,HW_BX=0xc,HW_AH=1,HW_SP=0x800,HW_CH=0x10,HW_BP=0x400,
       HW_DH=0x40,HW_SI=0x100,HW_BH=4,HW_DI=0x200,HW_EDI=0x20000200 };
static hw_reg_set RegTab[]={
    HW_AL,HW_AX,HW_EAX, HW_CL,HW_CX,HW_ECX,
    HW_DL,HW_DX,HW_EDX, HW_BL,HW_BX,HW_EBX,
    HW_AH,HW_SP,HW_SP, HW_CH,HW_BP,HW_BP,
    HW_DH,HW_SI,HW_ESI, HW_BH,HW_DI,HW_EDI
};
unsigned char RegTrans(hw_reg_set reg)
{
    int i;
    reg&=~0xc000f000U;
    for (i=0;i<24;++i) if (reg==RegTab[i]) break;
    if (i>=24) Zoiks(31);
    return i/3;
}
void LayOpbyte(unsigned short op)
{
    Inst[0]=(unsigned char)op;
    ICur=1;
    ILen=1;
    IEsc=1;
}
void AddToTemp(unsigned char value)
{
    if (value==0x6b)
        Temp.data[Temp.oc.reclen++-sizeof(oc_header)]=value;
    Temp.data[Temp.oc.reclen++-sizeof(oc_header)]=value;
    ++Temp.oc.objlen;
}
unsigned char OpndSize(hw_reg_set reg)
{
    if (reg&0xc000f000U) return 2; /* U2 */
    if (reg&0x3f000c00U) { /* HW_32_BP_SP */
        if (!(TargetModel&0x800)) AddToTemp(0x66);
        return 4; /* U4 */
    }
    if (TargetModel&0x800) AddToTemp(0x66);
    return 2;
}
void TransferIns(void)
{
    int i,j;
    i=0;
    j=Temp.oc.reclen-sizeof(oc_header);
    while (i<IEsc) {
        if (Inst[i]==0x6b) { /* ESC */
            Temp.data[j++]=0x6b;
            ++Temp.oc.reclen;
        }
        Temp.data[j++]=Inst[i++];
    }
    Copy((char *)&Inst[i],(char *)&Temp.data[j],ICur-i);
    Temp.oc.reclen+=ICur;
    Temp.oc.objlen+=ILen;
}
void EjectInst(void)
{
    TransferIns();
    ICur=0;
    ILen=0;
    IEsc=0;
}
void DoFunnyRef(unsigned char kind) { STUB_TRAP(); }
void Finalize(void)
{
    EjectInst();
    if (FPPatchType!=0) {
        DoFunnyRef(FPPatchType);
        FPPatchType=0;
    }
    if (Temp.oc.objlen!=0) InputOC((any_oc *)&Temp);
}

void QuickSave(hw_reg_set reg,unsigned char op)
{
    Format(2); /* OC_CODE */
    if (reg&0xc000f000U) { /* HW_SEGS */
        if (op==0x2a) PushSeg(reg);
        else PopSeg(reg);
    } else {
        if (op==0x2a) LayOpbyte(0x50|RegTrans(reg));
        else LayOpbyte(0x58|RegTrans(reg));
        OpndSize(reg);
    }
    Finalize();
}

void LayOpword(unsigned short op);
void LayW(unsigned char class);
void LayInitial(instruction *ins,unsigned char gen)
{
    int index;
    pccode_def *table;
    table=PCCodeTable;
    for (;;) {
        if (gen<table->low_gen) break;
        if (table->width==0) return;
        ++table;
    }
    --table;
    index=0;
    for (;;) {
        if (table->opcode_list[index]==ins->head.opcode || table->opcode_list[index]==0) break;
        ++index;
    }
    index=index*table->width+gen-table->low_gen;
    if (table->flags&NEED_WAIT) Used87=1;
    if (table->flags&BYTE_OPCODE) LayOpbyte(table->opcode_table[index]);
    else LayOpword(table->opcode_table[index]);
    if (table->flags&BYTE_WORD) LayW(ins->type_class);
    if (table->flags&SIGN_UNSIGN) {
        if (ins->type_class==1 || ins->type_class==3 || ins->type_class==5) {
            if (ins->head.opcode>=5 && ins->head.opcode<=8) Inst[1]|=8;
            else if (ins->head.opcode==12) Inst[1]|=0x10;
        }
    }
}

unsigned char NeedOpndSize(instruction *ins) { STUB_TRAP(); return 0; }
unsigned char LayOpndSize(instruction *ins,unsigned char gen)
{
    switch (ins->head.opcode) {
    case 0x24:case 0x29:case 0x2d:case 0x36: return 0;
    }
    switch (gen) {
    case G_SR:case G_RS:case G_MS1:case G_SM1:case G_SEG_SEG:
    case G_SEGR1:case G_POW2DIV:case G_DIV2: return 0;
    }
    if (TargetModel&0x800) {
        if ((ins->type_class==2 || ins->type_class==3) && NeedOpndSize(ins)) {
            AddToTemp(0x66);
            return 1;
        }
    } else if (ins->type_class==4 || ins->type_class==5 || ins->type_class==8) {
        AddToTemp(0x66);
        return 1;
    }
    return 0;
}

/* Encountered six segment roles and their emulator fixup kinds. */
static hw_reg_set SegTab[]={0x2000U,0x4000U,0x8000U,0x1000U,0x40000000U,0x80000000U};
static unsigned char SegPatchTab[]={3,4,5,6,7,8};
unsigned char SegTrans(hw_reg_set reg);
void GenSeg(hw_reg_set reg)
{
    hw_reg_set segment;
    int i;
    segment=reg&0xc000f000U;
    if (segment==0) return;
    if (reg&0x400U) { /* BP based addressing defaults to SS. */
        if (segment==0x8000U) return;
        if (segment==HW_DS && !(TargetModel&0x600U)) return;
    } else if (segment==HW_DS) return;
    if (reg&0x40000000U) AddToTemp(0x64);
    else if (reg&0x80000000U) AddToTemp(0x65);
    else AddToTemp((unsigned char)(0x26|(SegTrans(reg)<<3)));
    if (CGProcessorVersion&0x80) {
        for (i=0;i<6;++i) if (segment==SegTab[i]) break;
        if (FPPatchType!=0) {
            /* Combined/unknown segment masks have no proved patch owner. */
            if (i==6) STUB_TRAP();
            FPPatchType=SegPatchTab[i];
        }
    }
}

void LayRegRM(hw_reg_set reg);
/* Actual3B entry normalizes name* to its register set before the separately
 * called LayRegRM entry. Keep that real helper vertex, not an alias. */
void LayRMRegOp(name *operand)
{
    LayRegRM(operand->r.reg);
}

void AddSData(long value,unsigned char kind);
void AddWData(long value,unsigned char kind);
void AddSWData(unsigned char op,long value,unsigned char kind)
{
    /* Contiguous opcode groups: ADD..MUL, AND..XOR, and six comparisons. */
    if ((op>=1 && op<=5) || (op>=9 && op<=11) || (op>=0x30 && op<=0x35))
        AddSData(value,kind);
    else AddWData(value,kind);
}
void DoRelocConst(name *operand,int kind);
void AddSWCons(unsigned char op,name *constant,unsigned char kind)
{
    if (constant->c.const_type==0) AddSWData(op,constant->c.int_value,kind);
    else if (kind>=2 && kind<=5) DoRelocConst(constant,kind);
    else Zoiks(45);
}

void DoRelocConst(name *operand,int kind);
void AddByte(unsigned char value);
void AddWData(long value,unsigned char kind)
{
    AddByte((unsigned char)value);
    if (kind==0 || kind==1) return;
    value>>=8;
    AddByte((unsigned char)value);
    if (kind==2 || kind==3) return;
    value>>=8;
    AddByte((unsigned char)value);
    value>>=8;
    AddByte((unsigned char)value);
}
void AddWCons(name *operand,unsigned char type_class)
{
    if (operand->c.const_type==0) AddWData(operand->c.int_value,type_class);
    else if (type_class>=2 && type_class<=5) DoRelocConst(operand,type_class);
    else Zoiks(45);
}

void LayModRM(name *operand);

void LayReg(hw_reg_set reg);
/* Genuine three-byte pointer-to-register-set entry, then LayReg fallthrough. */
void LayRegOp(name *operand)
{
    LayReg(operand->r.reg);
}

void LayOpword(unsigned short op)
{
    Inst[0]=(unsigned char)op;
    ICur=2;
    ILen=2;
    IEsc=2;
    Inst[1]=(unsigned char)(op>>8);
}

name * IntEquivalent(name *operand) { STUB_TRAP(); return NULL; }

void AddSCons(name *constant,unsigned char class)
{
    if (constant->c.const_type==0) AddSData(constant->c.int_value,class);
    else if (class>=2 && class<=5) DoRelocConst(constant,class);
    else Zoiks(45);
}

void LayACRegOp(name *operand)
{
    Inst[0]|=RegTrans(operand->r.reg);
}

void AddByte(unsigned char value)
{
    if (value==0x6b) Inst[ICur++]=value;
    Inst[ICur++]=value;
    ++ILen;
}

void LayLeaRegOp(instruction *ins);

void LaySROp(name *operand) { STUB_TRAP();  }

void LayW(unsigned char class)
{
    if ((class>=2 && class<=5) || class==8) Inst[0]|=1;
}

void DoMAddr(name *operand);

unsigned char SegTrans(hw_reg_set reg) { STUB_TRAP(); return 0; }

unsigned char DoMDisp(name *operand,unsigned char alt);

void DoRepOp(instruction *ins);

unsigned char AskIsFrameIndex(name *operand);

void * NextFramePatch(void) { STUB_TRAP(); return NULL; }

void GenUnkLea(void *patch) { STUB_TRAP();  }

long NewBase(name *operand);
long TmpLoc(name *base,name *operand)
{
    return (long)((unsigned long)NewBase(base)+(unsigned long)operand->v.offset-
                  (unsigned long)base->v.offset);
}

unsigned char BaseIsSP(name *operand);

void LayReg(hw_reg_set reg)
{
    Inst[1]|=RegTrans(reg)<<3;
}

void Do4Shift(instruction *ins) { STUB_TRAP();  }

void Do4CXShift(instruction *ins,void (*routine)(instruction *)) { STUB_TRAP();  }

void GenCall(instruction *ins);

void AdjustStackDepth(instruction *ins);

void GenRCall(instruction *ins) { STUB_TRAP();  }

void GenICall(instruction *ins);

void GenRJmp(instruction *ins) { STUB_TRAP();  }

void GenMJmp(instruction *ins) { STUB_TRAP();  }

void LayMF(name *operand)
{
    if (operand->n.class==0) return;
    switch (operand->n.name_class) {
    case FS: break;
    case FD: Inst[0]|=4;break;
    case U2: case I2: Inst[0]|=6;break;
    case U4: case I4: Inst[0]|=2;break;
    case XX:
        Inst[0]|=6;
        Inst[1]|=0x28;
        break;
    default: Zoiks(29);break;
    }
}

/* Shared i86enc.c table at7a492, also used by the stack-name constructor. */
hw_reg_set FPRegs[]={
    1U<<16,1U<<17,1U<<18,1U<<19,1U<<20,1U<<21,1U<<22,1U<<23
};
void LayST(name *operand)
{
    int i;
    for (i=0;i<8;++i) if (operand->r.reg==FPRegs[i]) break;
    if (i==8) STUB_TRAP();
    Inst[1]|=(unsigned char)i;
}

void CallMathFunc(instruction *ins) { STUB_TRAP();  }

void MathFunc(instruction *ins) { STUB_TRAP();  }

void GCondFwait(void) { STUB_TRAP();  }

void DoCall(struct code_lbl *label,unsigned char imported,unsigned char big,unsigned char pop);

struct code_lbl * RTLabel(int routine) { STUB_TRAP(); return NULL; }

void SetCC(void) { STUB_TRAP();  }

void EmitDbgInfo(instruction *ins) { STUB_TRAP();  }

void Pow2Div(instruction *ins) { STUB_TRAP();  }

void By2Div(instruction *ins) { STUB_TRAP();  }

void GenCondJump(instruction *ins);

/* 1994 dispatcher, before P5 divide/profiling, block-profile callbacks and
 * the later FS/GS special prefix handling at G_SEGR1. G_NO still performs
 * stack-depth/conditional-branch bookkeeping. */
void GenObjCode(instruction *ins)
{
    unsigned char gen,opnd_size;
    name *result,*left,*right;
    int i;
    gen=ins->u.gen_table->generate;
    if (gen!=G_NO) {
        result=ins->result;
        if (ins->num_operands!=0) {
            left=ins->operands[0];
            if (ins->num_operands!=1) right=ins->operands[1];
        }
        Format(2);
        LayInitial(ins,gen);
        i=ins->num_operands-1;
        if (i>=NumOperands(ins)) {
            if (ins->operands[i]->n.class==3) GenSeg(ins->operands[i]->r.reg);
            else Zoiks(27);
        }
        opnd_size=LayOpndSize(ins,gen);
        switch (gen) {
        case G_RC:
            LayRMRegOp(left); AddSWCons(ins->head.opcode,right,ins->type_class); break;
        case G_AC:
            --ICur; --ILen; --IEsc;
            AddWCons(right,ins->type_class); break;
        case G_MC:
            LayModRM(left); AddSWCons(ins->head.opcode,right,ins->type_class); break;
        case G_RR2:
            LayRegOp(right); LayRMRegOp(left); break;
        case G_0FRR2:
            LayOpword(0x00af); LayRegOp(left); LayRMRegOp(right); AddToTemp(0x0f); break;
        case G_0FRM2:
            LayOpword(0x00af); LayModRM(right); LayRegOp(left); AddToTemp(0x0f); break;
        case G_PUSHFS:
            LayOpbyte(0x68); AddSCons(IntEquivalent(left),4); break;
        case G_RM2:
            LayModRM(right); LayRegOp(left); break;
        case G_MR2:
            LayModRM(left); LayRegOp(right); break;
        case G_WORDR1:
            if (ins->head.opcode==0x2b) { LayACRegOp(result); break; }
            LayACRegOp(left);
            if (NumOperands(ins)!=1 && right->c.int_value==2) {
                TransferIns(); if (opnd_size) AddToTemp(0x66);
            }
            break;
        case G_R1:
            LayRMRegOp(left);
            if (ins->num_operands!=1 && right->c.int_value==2) {
                TransferIns(); if (opnd_size) AddToTemp(0x66);
            }
            break;
        case G_R1SHIFT:case G_RCLSHIFT: LayRMRegOp(left); break;
        case G_M1:case G_1SHIFT:case G_CLSHIFT: LayModRM(left); break;
        case G_RNSHIFT: LayRMRegOp(left); AddByte((unsigned char)right->c.int_value); break;
        case G_NSHIFT: LayModRM(left); AddByte((unsigned char)right->c.int_value); break;
        case G_R2: LayRMRegOp(right); break;
        case G_M2: LayModRM(right); break;
        case G_RR1: LayRegOp(left); LayRMRegOp(result); break;
        case G_RM1: LayRegOp(result); LayModRM(left); break;
        case G_MR1: LayRegOp(left); LayModRM(result); break;
        case G_LEA:
            LayRegOp(result);
            if (left->n.class==3) LayLeaRegOp(ins);
            else LayModRM(left);
            break;
        case G_LDSES:
            LayRegOp(result); LayModRM(left);
            if (result->r.reg&0xc0000000U) { Inst[0]-=0x10; AddToTemp(0x0f); }
            if (result->r.reg&0x80001000U) Inst[0]|=1;
            break;
        case G_MS1: LaySROp(left); LayModRM(result); break;
        case G_SM1: LaySROp(result); LayModRM(left); break;
        case G_RS: LaySROp(left); LayRMRegOp(result); break;
        case G_SR: LaySROp(result); LayRMRegOp(left); break;
        case G_MOVMC:
            LayW(ins->type_class); LayModRM(result); AddWCons(left,ins->type_class); break;
        case G_MADDR: LayModRM(result); DoMAddr(left); break;
        case G_TEST: LayW(ins->type_class); LayRegOp(left); LayRMRegOp(left); break;
        case G_SEGR1:
            if (ins->head.opcode==0x2b) { Inst[0]|=1; i=SegTrans(result->r.reg); }
            else i=SegTrans(left->r.reg);
            Inst[0]|=i<<3;
            break;
        case G_MOVAM: LayW(ins->type_class); DoMDisp(left,0); break;
        case G_MOVMA: LayW(ins->type_class); DoMDisp(result,0); break;
        case G_MOVRC:
            if (ins->type_class==2 || ins->type_class==3 || ins->type_class==4 ||
                ins->type_class==5 || ins->type_class==8) Inst[0]|=8;
            LayACRegOp(result); AddWCons(left,ins->type_class); break;
        case G_RADDR: LayACRegOp(result); DoMAddr(left); break;
        case G_REPOP: DoRepOp(ins); break;
        case G_LOADADDR:
            if (AskIsFrameIndex(left)) GenUnkLea(NextFramePatch());
            else if (left->n.class==2 && TmpLoc(DeAlias(left),left)==0) {
                LayOpword(0xc089);
                if (BaseIsSP(left)) LayReg(0x800); else LayReg(0x400);
                LayRMRegOp(result);
            } else { LayOpword(0x008d); LayRegOp(result); LayModRM(left); }
            break;
        case G_4SHIFT:case G_4RSHIFT:case G_4RNEG:
            /* Three released empty386 helpers share this exact entry. */
            Do4Shift(ins); break;
        case G_CXSHIFT:case G_RCXSHIFT:
            Do4CXShift(ins,Do4Shift); break;
        case G_4NEG:
            /* Original Gen4Neg is folded into this exact C3. NULL is only
             * a source representation of an unused folded argument: it is
             * NOT observed EDX or evidence Gen4Neg had two formals. */
            Do4CXShift(ins,NULL); break;
        case G_ENTER: break;
        case G_CALL:
            GenCall(ins); AdjustStackDepth(ins); return;
        case G_ICALL:
            if (ins->operands[2]->n.class==3) GenRCall(ins); else GenICall(ins);
            AdjustStackDepth(ins); return;
        case G_RJMP: GenRJmp(ins); AdjustStackDepth(ins); return;
        case G_MJMP: GenMJmp(ins); break;
        case G_SIGNEX:
            switch (ins->type_class) {
            case 10:
                LayOpbyte(0x99); if (!(TargetModel&0x800)) AddToTemp(0x66); break;
            case 4:case 5:
                if (ins->base_type_class==0 || ins->base_type_class==1) {
                    if (TargetModel&0x800) AddToTemp(0x66);
                    AddToTemp(0x98);
                }
                if (result->r.reg==0xc3) {
                    LayOpbyte(0x99); if (TargetModel&0x800) AddToTemp(0x66);
                } else {
                    LayOpbyte(0x98); if (!(TargetModel&0x800)) AddToTemp(0x66);
                }
                break;
            case 2:case 3:
                LayOpbyte(0x98); if (TargetModel&0x800) AddToTemp(0x66); break;
            }
            break;
        case G_C1: AddSCons(left,ins->type_class); break;
        case G_186RMUL:
            LayOpword(0xc069); LayRegOp(result); LayRMRegOp(left); AddSCons(right,ins->type_class); break;
        case G_186MUL:
            LayOpword(0x0069); LayRegOp(result); LayModRM(left); AddSCons(right,ins->type_class); break;
        case G_MRFBIN:case G_MNFBIN:case G_MFLD:
            LayMF(left); LayModRM(left); break;
        case G_RFLD:case G_RRFBIN:case G_RRFBINP:case G_RRFBIND:
        case G_RNFBIN:case G_RNFBINP:case G_RNFBIND: LayST(left); break;
        case G_RFST:case G_RFSTNP: LayST(result); break;
        case G_MFST:case G_MFSTNP: LayMF(result); LayModRM(result); break;
        case G_FCHS:case G_FLD1:case G_FLDZ:case G_FINIT: break;
        case G_IFUNC: CallMathFunc(ins); break;
        case G_FMATH: MathFunc(ins); break;
        case G_FSINCOS: GCondFwait(); LayOpword(0xfbd9); break;
        case G_FCHOP: DoCall(RTLabel(0x1f),1,(TargetModel&8)!=0,0); break;
        case G_FTST:case G_FCOMPP: Used87=1; SetCC(); break;
        case G_FWAIT:
            Used87=1;
            if (CGProcessorVersion&0x80) { FPPatchType=1; LayOpword(0x9b90); }
            else LayOpbyte(0x9b);
            break;
        case G_FXCH: Used87=1; GCondFwait(); LayOpword(0xc8d9); LayST(result); break;
        case G_MCOMP: LayMF(left); LayModRM(left); SetCC(); break;
        case G_RCOMP: LayST(left); SetCC(); break;
        case G_DEBUG: EmitDbgInfo(ins); break;
        case G_SEG_SEG: PushSeg(left->r.reg); EjectInst(); PopSeg(result->r.reg); break;
        case G_POW2DIV: Pow2Div(ins); break;
        case G_DIV2: By2Div(ins); break;
        case G_MOVSX:case G_MOVZX:
            LayOpword(gen==G_MOVSX?0x00be:0x00b6);
            if (left->n.size==2) Inst[0]|=1;
            else {
                switch (ins->type_class) {
                case 2:case 3: if (TargetModel&0x800) AddToTemp(0x66); break;
                case 4:case 5: if (!(TargetModel&0x800)) AddToTemp(0x66); break;
                }
            }
            LayModRM(left); LayRegOp(result); AddToTemp(0x0f); break;
        default: Zoiks(28); break;
        }
        Finalize();
    }
    AdjustStackDepth(ins);
    if (ins->head.opcode>=0x2e && ins->head.opcode<=0x35) GenCondJump(ins);
}

void LayRegRM(hw_reg_set reg)
{
    Inst[1]|=RegTrans(reg)+0xc0;
}
void AddSData(long value,unsigned char kind)
{
    unsigned long bits;
    bits=(unsigned long)value;
    if ((kind==U2 || kind==I2) &&
        ((bits&0xff80UL)==0xff80UL || (bits&0xff80UL)==0)) {
        Inst[0]|=2; /* B_KEY_S. */
        AddByte((unsigned char)value);
    } else if ((kind==U4 || kind==I4) &&
        ((bits&0xffffff80UL)==0xffffff80UL || (bits&0xffffff80UL)==0)) {
        Inst[0]|=2;
        AddByte((unsigned char)value);
    } else AddWData(value,kind);
}
void GenRegOp(hw_reg_set reg,long value,int op)
{
    unsigned char kind;
    Format(2);
    LayOpword(op|1); /* B_KEY_W. */
    kind=OpndSize(reg);
    LayRegRM(reg);
    AddSData(value,kind);
    Finalize();
}
void GenRegSub(hw_reg_set reg,long value)
{
    GenRegOp(reg,value,0xe880); /* M_SUBRC. */
}

void GenRegAdd(hw_reg_set reg,long value)
{
    GenRegOp(reg,value,0xc080); /* M_ADDRC. */
}

/* Raw escape-record byte: unlike AddByte it neither quotes ESC nor adds
 * to machine instruction length. */
void EmitByte(unsigned char value)
{
    Inst[ICur++]=value;
}

/* EmitPtr and EmitOffset share this physical entry in the1994 image.
 * Both transport one DWORD; explicit bytes permit unaligned buffer cursors. */
void EmitOffset(long value)
{
    unsigned long bits;
    bits=(unsigned long)value;
    Inst[ICur]=(unsigned char)bits;
    Inst[ICur+1]=(unsigned char)(bits>>8);
    Inst[ICur+2]=(unsigned char)(bits>>16);
    Inst[ICur+3]=(unsigned char)(bits>>24);
    ICur+=4;
}
