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


#include "block.h"
#include "options.h"
#include "regchoices.h"
#include "stubs.h"
#include <recon.h>

/* 386rgtbl.c's one-DWORD register-set representation. */
hw_reg_set FixedRegs(void)
{
    hw_reg_set fixed;
    fixed=0xcc00; /* SP | BP | SS | CS */
    if (!(TargetModel&FLOATING_DS)) fixed|=HW_DS;
    if (!(TargetModel&FLOATING_ES)) fixed|=HW_ES;
    if (!(TargetModel&FLOATING_FS)) fixed|=HW_FS;
    if (!(TargetModel&FLOATING_GS)) fixed|=HW_GS;
    if (TargetModel&0x1000) fixed|=HW_EBX; /* INDEXED_GLOBALS */
    return fixed;
}

hw_reg_set StackReg(void)
{
    return 0x800; /* HW_SP in the original one-DWORD register map. */
}

/* Only each return list's first set is consumed here. Keep those semantic
 * return choices without copying the unrelated register-allocation lists. */
static hw_reg_set ReturnSets[]={
    0x2,0x2,0x3,0x3,HW_EAX,HW_EAX,HW_EAX|0xc0,0,
    HW_EAX,HW_EDX|HW_EAX,0
};
static hw_reg_set Return8087[]={
    0x2,0x2,0x3,0x3,HW_EAX,HW_EAX,HW_EAX|0xc0,0,
    0x10000,0x10000,0
};
hw_reg_set ReturnReg(unsigned char type_class,unsigned char use87)
{
    if ((CGProcessorVersion&0x70)>=0x10 && use87) return Return8087[type_class];
    return ReturnSets[type_class];
}

/* ST1 through ST7, in the original one-bit register-set encoding. */
static hw_reg_set STIReg[]={
    0x20000,0x40000,0x80000,0x100000,0x200000,0x400000,0x800000,0
};
unsigned char RegClass(hw_reg_set reg)
{
    hw_reg_set *possible;
    if (reg&HW_FLTS) {
        if (reg==0x10000) return FD; /* ST0 */
        for (possible=STIReg;*possible!=0;++possible)
            if (*possible==reg) return FD;
        return XX;
    }
    return NotFloatRegClass(reg);
}

/* Encountered 386rgtbl.c classes, in the older one-DWORD register encoding.
 * The old DoubleRegs order is EAX,EDX,EBX,ECX; its far-pointer list predates
 * the later CS pairs. These tables define register choices, not raw data. */
#define HW_AH 0x1U
#define HW_AL 0x2U
#define HW_BH 0x4U
#define HW_BL 0x8U
#define HW_CH 0x10U
#define HW_CL 0x20U
#define HW_DH 0x40U
#define HW_DL 0x80U
#define HW_AX (HW_AH|HW_AL)
#define HW_BX (HW_BH|HW_BL)
#define HW_CX (HW_CH|HW_CL)
#define HW_DX (HW_DH|HW_DL)
#define HW_SI 0x100U
#define HW_DI 0x200U
#define HW_BP 0x400U
#define HW_SP 0x800U
#define HW_CS 0x4000U
#define HW_SS 0x8000U
#define HW_EDI 0x20000200U
#define HW_SEGS (HW_DS|HW_ES|HW_FS|HW_GS|HW_CS|HW_SS)
static hw_reg_set EmptyReg[]={0};
static hw_reg_set ByteRegs[]={HW_AL,HW_AH,HW_DL,HW_DH,HW_BL,HW_BH,HW_CL,HW_CH,0};
static hw_reg_set WordRegs[]={HW_AX,HW_DX,HW_BX,HW_CX,HW_SI,HW_DI,0};
static hw_reg_set SegRegs[]={HW_DS,HW_ES,HW_FS,HW_GS,HW_CS,HW_SS,0};
static hw_reg_set DoubleRegs[]={HW_EAX,HW_EDX,HW_EBX,HW_ECX,HW_ESI,HW_EDI,HW_BP,HW_SP,0};
static hw_reg_set DXAXReg[]={HW_DX|HW_AX,0};
static hw_reg_set FarPointerRegs[]={
    HW_DX|HW_EAX,HW_CX|HW_EBX,HW_CX|HW_EAX,HW_CX|HW_ESI,HW_DX|HW_EBX,
    HW_DI|HW_EAX,HW_CX|HW_EDI,HW_DX|HW_ESI,HW_DI|HW_EBX,HW_SI|HW_EAX,
    HW_CX|HW_EDX,HW_DX|HW_EDI,HW_DI|HW_ESI,HW_SI|HW_EBX,HW_BX|HW_EAX,
    HW_DS|HW_EAX,HW_DS|HW_EDX,HW_DS|HW_EBX,HW_DS|HW_ECX,HW_DS|HW_ESI,HW_DS|HW_EDI,HW_DS|HW_BP,
    HW_ES|HW_EAX,HW_ES|HW_EDX,HW_ES|HW_EBX,HW_ES|HW_ECX,HW_ES|HW_ESI,HW_ES|HW_EDI,HW_ES|HW_BP,
    HW_FS|HW_EAX,HW_FS|HW_EDX,HW_FS|HW_EBX,HW_FS|HW_ECX,HW_FS|HW_ESI,HW_FS|HW_EDI,HW_FS|HW_BP,
    HW_GS|HW_EAX,HW_GS|HW_EDX,HW_GS|HW_EBX,HW_GS|HW_ECX,HW_GS|HW_ESI,HW_GS|HW_EDI,HW_GS|HW_BP,
    HW_SS|HW_EAX,HW_SS|HW_EDX,HW_SS|HW_EBX,HW_SS|HW_ECX,HW_SS|HW_ESI,HW_SS|HW_EDI,HW_SS|HW_BP,0
};
static hw_reg_set QuadReg[]={
    HW_EDX|HW_EAX,HW_ECX|HW_EBX,HW_ECX|HW_EAX,HW_ECX|HW_ESI,HW_EDX|HW_EBX,
    HW_EDI|HW_EAX,HW_ECX|HW_EDI,HW_EDX|HW_ESI,HW_EDI|HW_EBX,HW_ESI|HW_EAX,
    HW_ECX|HW_EDX,HW_EDX|HW_EDI,HW_EDI|HW_ESI,HW_ESI|HW_EBX,HW_EBX|HW_EAX,
    HW_BP|HW_EAX,HW_BP|HW_EDX,HW_BP|HW_EBX,HW_BP|HW_ECX,HW_BP|HW_ESI,HW_BP|HW_EDI,0
};
/* Only this classifier consumes ClassSets/RegSets indirection. */
static hw_reg_set *ClassSets[]={
    ByteRegs,EmptyReg,WordRegs,SegRegs,DoubleRegs,DXAXReg,
    FarPointerRegs,EmptyReg,EmptyReg,QuadReg
};
unsigned char NotFloatRegClass(hw_reg_set regs)
{
    hw_reg_set *possible;
    unsigned char class;
    for (class=U1;class!=XX;++class) {
        possible=ClassSets[class];
        while (*possible!=0) {
            if (*possible==regs) return class;
            ++possible;
        }
    }
    if (regs&HW_SEGS) {
        regs&=~HW_SEGS;
        if (NotFloatRegClass(regs)==FD) return CP;
    }
    return XX;
}

extern hw_reg_set STParmReg[]; /* Also truncated by InitRegTbl. */
static hw_reg_set *ParmSets[]={
    ByteRegs,ByteRegs,WordRegs,WordRegs,DoubleRegs,DoubleRegs,
    FarPointerRegs,FarPointerRegs,DoubleRegs,QuadReg,EmptyReg
};
static hw_reg_set *Parm8087[]={
    ByteRegs,ByteRegs,WordRegs,WordRegs,DoubleRegs,DoubleRegs,
    FarPointerRegs,FarPointerRegs,STParmReg,STParmReg,EmptyReg
};
hw_reg_set *ParmChoices(unsigned char type_class)
{
    if (((CGProcessorVersion&0xf0)&0x7f)>=0x10) return Parm8087[type_class];
    return ParmSets[type_class];
}

hw_reg_set DisplayReg(void)
{
    if (CurrProc->targ.sp_frame) return HW_SP;
    return HW_BP;
}

hw_reg_set ActualParmReg(hw_reg_set reg)
{
    if (reg&HW_FLTS) return 0x10000; /* ST0 receives all x87 arguments. */
    return reg;
}

/* Encountered allocator lists from 386rgtbl.c and rl.h. Existing parameter
 * and classifier lists above retain their actual 1994 ordering. */
static hw_reg_set ALReg[]={HW_AL,0},AHReg[]={HW_AH,0},CLReg[]={HW_CL,0};
static hw_reg_set AXReg[]={HW_AX,0},CXReg[]={HW_CX,0},DXReg[]={HW_DX,0};
static hw_reg_set EAXReg[]={HW_EAX,0},EDXReg[]={HW_EDX,0},ECXReg[]={HW_ECX,0};
static hw_reg_set DXEAXReg[]={HW_DX|HW_EAX,0},EDXEAXReg[]={HW_EDX|HW_EAX,0};
static hw_reg_set TwoByteRegs[]={HW_AX,HW_DX,HW_BX,HW_CX,0};
static hw_reg_set WordOrSegReg[]={
    HW_AX,HW_DX,HW_BX,HW_CX,HW_SI,HW_DI,HW_DS,HW_ES,HW_FS,HW_GS,HW_CS,HW_SS,0
};
static hw_reg_set ABCDRegs[]={HW_EAX,HW_EDX,HW_EBX,HW_ECX,0};
static hw_reg_set LongIndexRegs[]={
    HW_DS| HW_EAX,
    HW_DS| HW_EDX,
    HW_DS| HW_EBX,
    HW_DS| HW_ECX,
    HW_DS| HW_ESI,
    HW_DS| HW_EDI,
    HW_DS| HW_BP,
    HW_ES| HW_EAX,
    HW_ES| HW_EDX,
    HW_ES| HW_EBX,
    HW_ES| HW_ECX,
    HW_ES| HW_ESI,
    HW_ES| HW_EDI,
    HW_ES| HW_BP,
    HW_FS| HW_EAX,
    HW_FS| HW_EDX,
    HW_FS| HW_EBX,
    HW_FS| HW_ECX,
    HW_FS| HW_ESI,
    HW_FS| HW_EDI,
    HW_FS| HW_BP,
    HW_GS| HW_EAX,
    HW_GS| HW_EDX,
    HW_GS| HW_EBX,
    HW_GS| HW_ECX,
    HW_GS| HW_ESI,
    HW_GS| HW_EDI,
    HW_GS| HW_BP,
    HW_SS| HW_EAX,
    HW_SS| HW_EDX,
    HW_SS| HW_EBX,
    HW_SS| HW_ECX,
    HW_SS| HW_ESI,
    HW_SS| HW_EDI,
    HW_SS| HW_BP,
    HW_CS| HW_EAX,
    HW_CS| HW_EDX,
    HW_CS| HW_EBX,
    HW_CS| HW_ECX,
    HW_CS| HW_ESI,
    HW_CS| HW_EDI,
    HW_CS| HW_BP,
    0,
};
static hw_reg_set ST0Reg[]={1U<<16,0};
extern hw_reg_set FPParm2Reg[];
hw_reg_set *RegSets[]={
    EmptyReg,ALReg,AHReg,CLReg,ByteRegs,AXReg,CXReg,DXReg,WordRegs,TwoByteRegs,
    SegRegs,WordOrSegReg,EAXReg,EDXReg,DXAXReg,DoubleRegs,ABCDRegs,ECXReg,
    DXEAXReg,LongIndexRegs,FarPointerRegs,EDXEAXReg,FPParm2Reg,QuadReg,ST0Reg,STIReg,NULL
};
/* All64 named rl.h/rg.h rows match the 1994 five-byte records at79984.
 * These are source register constraints, not copied binary table bytes. */
op_regs RegList[]={
    {RL_,RL_,RL_,RL_,RL_}, /* RG_ */
    {RL_EAX,RL_EDX,RL_EAX,RL_,RL_}, /* RG_4CRTN */
    {RL_EDX_EAX,RL_FPPARM2,RL_EDX_EAX,RL_,RL_}, /* RG_8CRTN */
    {RL_STI,RL_STI,RL_STI,RL_,RL_}, /* RG_8087 */
    {RL_STI,RL_STI,RL_STI,RL_,RL_DOUBLE}, /* RG_8087_NEED */
    {RL_STI,RL_,RL_8,RL_,RL_}, /* RG_8087_8 */
    {RL_STI,RL_STI,RL_STI,RL_EAX,RL_}, /* RG_8087_ZAP_ACC */
    {RL_STI,RL_,RL_DOUBLE,RL_,RL_}, /* RG_8087_DBL */
    {RL_8,RL_,RL_STI,RL_,RL_}, /* RG_8_8087 */
    {RL_8,RL_8,RL_8,RL_,RL_DOUBLE}, /* RG_8_NEED_DBL */
    {RL_DOUBLE,RL_,RL_STI,RL_,RL_}, /* RG_DBL_8087 */
    {RL_DOUBLE,RL_,RL_,RL_,RL_}, /* RG_DBL_BYTE */
    {RL_ST0,RL_,RL_STI,RL_,RL_}, /* RG_ST0_STI */
    {RL_STI,RL_,RL_ST0,RL_,RL_}, /* RG_STI_ST0 */
    {RL_BYTE,RL_BYTE,RL_BYTE,RL_,RL_}, /* RG_BYTE */
    {RL_TWOBYTE,RL_TWOBYTE,RL_TWOBYTE,RL_,RL_}, /* RG_TWOBYTE */
    {RL_WORD,RL_WORD,RL_WORD,RL_,RL_}, /* RG_WORD */
    {RL_WS,RL_WS,RL_WS,RL_,RL_}, /* RG_ANYWORD */
    {RL_DOUBLE,RL_DOUBLE,RL_DOUBLE,RL_,RL_}, /* RG_DBL */
    {RL_8,RL_8,RL_8,RL_,RL_}, /* RG_8 */
    {RL_AL,RL_AL,RL_AL,RL_,RL_}, /* RG_BYTE_ACC */
    {RL_AX,RL_AX,RL_AX,RL_,RL_}, /* RG_WORD_ACC */
    {RL_EAX,RL_EAX,RL_EAX,RL_,RL_}, /* RG_DBL_ACC */
    {RL_AX,RL_BYTE,RL_AL,RL_AH,RL_}, /* RG_BYTE_DIV */
    {RL_DX_AX,RL_WORD,RL_AX,RL_DX,RL_}, /* RG_WORD_DIV */
    {RL_EDX_EAX,RL_DOUBLE,RL_EAX,RL_EDX,RL_}, /* RG_DBL_DIV */
    {RL_AL,RL_,RL_AX,RL_,RL_}, /* RG_CBW */
    {RL_AL,RL_,RL_DX_AX,RL_,RL_}, /* RG_CBD */
    {RL_AL,RL_,RL_EAX,RL_,RL_}, /* RG_CBDE */
    {RL_AX,RL_,RL_DX_AX,RL_,RL_}, /* RG_CWD */
    {RL_AX,RL_,RL_EAX,RL_,RL_}, /* RG_CWDE */
    {RL_EAX,RL_,RL_EDX_EAX,RL_,RL_}, /* RG_CDQ */
    {RL_BYTE,RL_,RL_WORD,RL_,RL_}, /* RG_BYTE_WORD */
    {RL_BYTE,RL_,RL_WORD,RL_,RL_WORD}, /* RG_BYTE_WORD_NEED_WORD */
    {RL_BYTE,RL_,RL_WORD,RL_,RL_BYTE}, /* RG_BYTE_WORD_NEED_BYTE */
    {RL_BYTE,RL_,RL_DOUBLE,RL_,RL_}, /* RG_BYTE_DBL */
    {RL_BYTE,RL_,RL_DOUBLE,RL_,RL_DOUBLE}, /* RG_BYTE_DBL_NEED_DBL */
    {RL_WORD,RL_,RL_DOUBLE,RL_,RL_}, /* RG_WORD_DBL */
    {RL_WORD,RL_,RL_DOUBLE,RL_,RL_DOUBLE}, /* RG_WORD_DBL_NEED_DBL */
    {RL_WORD,RL_,RL_DOUBLE,RL_,RL_WORD}, /* RG_WORD_DBL_NEED_WORD */
    {RL_AX,RL_BYTE,RL_AH,RL_AL,RL_}, /* RG_BYTE_MOD */
    {RL_DX_AX,RL_WORD,RL_DX,RL_AX,RL_}, /* RG_WORD_MOD */
    {RL_EDX_EAX,RL_DOUBLE,RL_EDX,RL_EAX,RL_}, /* RG_DBL_MOD */
    {RL_AL,RL_BYTE,RL_AX,RL_,RL_}, /* RG_BYTE_MUL */
    {RL_AX,RL_WORD,RL_DX_AX,RL_,RL_}, /* RG_WORD_MUL */
    {RL_EAX,RL_WORD,RL_EDX_EAX,RL_,RL_}, /* RG_DBL_MUL */
    {RL_BYTE,RL_BYTE,RL_BYTE,RL_,RL_BYTE}, /* RG_BYTE_NEED */
    {RL_WORD,RL_WORD,RL_WORD,RL_,RL_WORD}, /* RG_WORD_NEED */
    {RL_DOUBLE,RL_DOUBLE,RL_DOUBLE,RL_,RL_DOUBLE}, /* RG_DBL_NEED */
    {RL_BYTE,RL_CL,RL_BYTE,RL_,RL_}, /* RG_BYTE_SHIFT */
    {RL_WORD,RL_CL,RL_WORD,RL_,RL_}, /* RG_WORD_SHIFT */
    {RL_DOUBLE,RL_CL,RL_DOUBLE,RL_,RL_}, /* RG_DBL_SHIFT */
    {RL_BYTE,RL_CL,RL_BYTE,RL_,RL_BYTE}, /* RG_BYTE_SHIFT_NEED */
    {RL_WORD,RL_CL,RL_WORD,RL_,RL_WORD}, /* RG_WORD_SHIFT_NEED */
    {RL_DOUBLE,RL_CL,RL_DOUBLE,RL_,RL_DOUBLE}, /* RG_DBL_SHIFT_NEED */
    {RL_WS,RL_WS,RL_WS,RL_,RL_WORD}, /* RG_ANYWORD_NEED */
    {RL_DOUBLE,RL_,RL_DOUBLE,RL_,RL_}, /* RG_LEA */
    {RL_SEG,RL_,RL_WORD,RL_,RL_}, /* RG_SEG_WORD */
    {RL_WORD,RL_,RL_SEG,RL_,RL_}, /* RG_WORD_SEG */
    {RL_SEG,RL_SEG,RL_SEG,RL_,RL_}, /* RG_SEG_SEG */
    {RL_FAR_POINTER,RL_FAR_POINTER,RL_FAR_POINTER,RL_,RL_}, /* RG_FAR_POINTER */
    {RL_WORD,RL_WORD,RL_,RL_,RL_}, /* RG_WORD_OP */
    {RL_BYTE,RL_,RL_ABCD,RL_,RL_}, /* RG_BYTE_4BYTE */
    {RL_BYTE,RL_,RL_TWOBYTE,RL_,RL_}, /* RG_BYTE_2BYTE */
};

/* Released 386rgtbl.c intersection classes and named register lists.
 * All seven matrices and the two-byte class/index records match the 1994
 * operand-choice tables; they are not inferred from host register sets. */
enum intersect_class { ONE_BYTE,TWO_BYTE,FOUR_BYTE,SIX_BYTE,EIGHT_BYTE,FLOATING,OTHER };
typedef struct reg_class { unsigned char class,index; } reg_class;
/* Public data identity keeps this real CONST contribution separate from
 * the preceding translation unit's function extent in the period MAP. */
const reg_class IntersectInfo[]={
    {OTHER,0}, /* RL_ */
    {ONE_BYTE,0}, /* RL_AL */
    {ONE_BYTE,1}, /* RL_AH */
    {ONE_BYTE,2}, /* RL_CL */
    {ONE_BYTE,3}, /* RL_BYTE */
    {TWO_BYTE,0}, /* RL_AX */
    {TWO_BYTE,1}, /* RL_CX */
    {TWO_BYTE,2}, /* RL_DX */
    {TWO_BYTE,3}, /* RL_WORD */
    {TWO_BYTE,4}, /* RL_TWOBYTE */
    {TWO_BYTE,5}, /* RL_SEG */
    {TWO_BYTE,6}, /* RL_ANYWORD */
    {FOUR_BYTE,0}, /* RL_EAX */
    {FOUR_BYTE,1}, /* RL_EDX */
    {FOUR_BYTE,2}, /* RL_DX_AX */
    {FOUR_BYTE,3}, /* RL_DOUBLE */
    {FOUR_BYTE,4}, /* RL_ABCD */
    {FOUR_BYTE,5}, /* RL_ECX */
    {SIX_BYTE,0}, /* RL_DX_EAX */
    {SIX_BYTE,1}, /* RL_LONG_INDEX */
    {SIX_BYTE,2}, /* RL_FAR_POINTER */
    {EIGHT_BYTE,0}, /* RL_EDX_EAX */
    {EIGHT_BYTE,1}, /* RL_FPPARM2 */
    {EIGHT_BYTE,2}, /* RL_8 */
    {FLOATING,0}, /* RL_ST0 */
    {FLOATING,1}, /* RL_STI */
    {OTHER,0} /* RL_NUMBER_OF_SETS */
};
static unsigned char Width[]={4,7,6,3,3,2,0};
#define RL_WD RL_WORD
#define RL_2B RL_TWOBYTE
#define RL_SG RL_SEG
static  unsigned char   OneByteInter[] = {
/*      AL      AH      CL      BYTE                 */
        RL_AL,  RL_,    RL_,    RL_AL,          /* AL*/
        RL_,    RL_AH,  RL_,    RL_AH,          /* AH*/
        RL_,    RL_,    RL_CL,  RL_CL,          /* CL*/
        RL_AL,  RL_AH,  RL_CL,  RL_BYTE         /* BYTE*/
};
static  unsigned char   TwoByteInter[] = {
/*AX   CX     DX     WD     2B     SG     WS*/
RL_AX, RL_,   RL_,   RL_AX, RL_AX, RL_,   RL_AX,  /* AX*/
RL_,   RL_CX, RL_,   RL_CX, RL_CX, RL_,   RL_CX,  /* CX*/
RL_,   RL_,   RL_DX, RL_DX, RL_DX, RL_,   RL_DX,  /* DX*/
RL_AX, RL_CX, RL_DX, RL_WD, RL_2B, RL_,   RL_WD,  /* WD*/
RL_AX, RL_CX, RL_CX, RL_2B, RL_2B, RL_,   RL_2B,  /* 2B*/
RL_,   RL_,   RL_,   RL_,   RL_,   RL_SG, RL_SG,  /* SG*/
RL_AX, RL_CX, RL_DX, RL_WD, RL_2B, RL_SG, RL_WS,  /* WS*/
RL_ };
static  unsigned char   FourByteInter[] = {
/* RL_EAX  RL_EDX      RL_DX_AX    RL_DOUBLE   RL_ABCD  RL_ECX */
RL_EAX,    RL_,        RL_,        RL_EAX,     RL_EAX,  RL_,   /* RL_EAX     */
RL_,       RL_EDX,     RL_,        RL_EDX,     RL_EDX,  RL_,   /* RL_EDX     */
RL_,       RL_,        RL_DX_AX,   RL_,        RL_,     RL_,   /* RL_DX_AX   */
RL_EAX,    RL_EDX,     RL_,        RL_DOUBLE,  RL_ABCD, RL_,   /* RL_DOUBLE  */
RL_EAX,    RL_EDX,     RL_,        RL_ABCD,    RL_ABCD, RL_,   /* RL_ABCD    */
RL_,       RL_,        RL_,        RL_,        RL_,     RL_ECX /* RL_ECX     */
};
static  unsigned char   SixByteInter[] = {
/* RL_DX_EAX    RL_LONG_INDEX     RL_FAR_POINTER */
   RL_DX_EAX,   RL_,              RL_DX_EAX,      /* RL_DX_EAX */
   RL_,         RL_LONG_INDEX,    RL_LONG_INDEX,  /* RL_LONG_INDEX */
   RL_DX_EAX,   RL_LONG_INDEX,    RL_FAR_POINTER  /* RL_FAR_POINTER */
};
static  unsigned char   EightByteInter[] = {
/* RL_EDX_EAX   RL_FPPARM2      RL_8 */
RL_EDX_EAX,     RL_,            RL_EDX_EAX,     /* RL_EDX_EAX */
RL_,            RL_FPPARM2,     RL_FPPARM2,     /* RL_FPPARM2 */
RL_EDX_EAX,     RL_FPPARM2,     RL_8            /* RL_8 */
};
static  unsigned char   FloatingInter[] = {
/*      RL_ST0  RL_STI                  */
        RL_ST0, RL_,      /*  RL_ST0    */
        RL_,    RL_STI };
static  unsigned char   OtherInter[] = {
        RL_ };
static unsigned char *InterTable[]={OneByteInter,TwoByteInter,FourByteInter,SixByteInter,EightByteInter,FloatingInter,OtherInter};
unsigned char RegIntersect(unsigned char left,unsigned char right)
{
    reg_class *set1,*set2;
    unsigned char class;
    if (left>RL_NUMBER_OF_SETS || right>RL_NUMBER_OF_SETS) STUB_TRAP();
    set1=&IntersectInfo[left];
    set2=&IntersectInfo[right];
    class=set1->class;
    if (class==set2->class)
        return InterTable[class][set1->index*Width[class]+set2->index];
    if (left==RL_NUMBER_OF_SETS) return right;
    if (right==RL_NUMBER_OF_SETS) return left;
    return RL_;
}


/* Index register classes: the 1994 type universe has ten scalar classes
 * followed by XX, before later 64-bit integer and long-double additions. */
static unsigned char IndexSets[]={RL_,RL_,RL_,RL_,RL_DOUBLE,RL_DOUBLE,
    RL_LONG_INDEX,RL_LONG_INDEX,RL_,RL_,RL_};
unsigned char IsIndexReg(hw_reg_set regs,unsigned char class,unsigned char is_temp_index)
{
    hw_reg_set *list;
    is_temp_index=is_temp_index;
    if (class>XX) STUB_TRAP();
    list=RegSets[IndexSets[class]];
    while (*list!=0) {
        if (*list==regs) break;
        ++list;
    }
    return *list==regs;
}

unsigned char IndexIntersect(unsigned char current,unsigned char class,unsigned char is_temp_index)
{
    is_temp_index=is_temp_index;
    if (class>XX) STUB_TRAP();
    return RegIntersect(current,IndexSets[class]);
}

unsigned char NoSegments(unsigned char possible)
{
    if (possible==RL_ANYWORD) return RL_WORD;
    return possible;
}

unsigned char IsSegReg(hw_reg_set reg)
{
    return (reg&HW_SEGS)==reg;
}
unsigned char SegIndex(void) { STUB_TRAP();return 0; }

hw_reg_set FullReg(hw_reg_set regs)
{
    if (regs&HW_EAX) regs|=HW_EAX;
    if (regs&HW_EBX) regs|=HW_EBX;
    if (regs&HW_ECX) regs|=HW_ECX;
    if (regs&HW_EDX) regs|=HW_EDX;
    if (regs&HW_EDI) regs|=HW_EDI;
    if (regs&HW_ESI) regs|=HW_ESI;
    return regs;
}

hw_reg_set Low64Reg(hw_reg_set reg);
hw_reg_set Low32Reg(hw_reg_set reg);
hw_reg_set Low16Reg(hw_reg_set reg);
hw_reg_set LowOffsetReg(hw_reg_set reg)
{
    switch (RegClass(reg)) {
    case CP: return reg&~HW_SEGS;
    case FD: return Low64Reg(reg);
    case U4: return Low32Reg(reg);
    case U2: return Low16Reg(reg);
    default: return 0;
    }
}
hw_reg_set Low32Reg(hw_reg_set reg)
{
    if (reg==0xc3U) return 3U; /* HW_DX_AX -> HW_AX. */
    return reg&0x3ffU; /* HW_AX_BX_CX_DX_SI_DI. */
}
hw_reg_set Low16Reg(hw_reg_set reg)
{
    return reg&0xaaU; /* HW_AL_BL_CL_DL in this1994 bit assignment. */
}
hw_reg_set High64Reg(hw_reg_set reg);
hw_reg_set High32Reg(hw_reg_set reg);
hw_reg_set High16Reg(hw_reg_set reg);
hw_reg_set HighOffsetReg(hw_reg_set reg)
{
    /* The target expands the released HighReg wrapper at this vertex. */
    switch (RegClass(reg)) {
    case CP: return reg&HW_SEGS;
    case FD: return High64Reg(reg);
    case U4: case I4: return High32Reg(reg);
    case U2: return High16Reg(reg);
    default: return 0;
    }
}
hw_reg_set High64Reg(hw_reg_set reg)
{
    hw_reg_set low;
    low=Low64Reg(reg);
    if (low!=0) return reg&~low;
    return 0;
}
hw_reg_set High32Reg(hw_reg_set reg)
{
    return reg==0xc3U ? 0xc0U : 0; /* HW_DX_AX -> HW_DX. */
}
hw_reg_set High16Reg(hw_reg_set reg)
{
    return reg&0x55U; /* HW_AH_BH_CH_DH in this1994 bit assignment. */
}

unsigned char IsStackReg(name *op)
{
    return op!=NULL && op->n.class==3 && op->r.reg==0x800U;
}

hw_reg_set StructReg(void)
{
    return HW_ESI;
}

/* IsSets maps signed/unsigned forms to the same machine-register list;
 * pointer forms use the far-pointer combinations, float values use their
 * software register tuples in this classifier. */
static unsigned char IsSets[]={
    RL_BYTE,RL_BYTE,RL_WORD,RL_WORD,RL_DOUBLE,RL_DOUBLE,
    RL_FAR_POINTER,RL_FAR_POINTER,RL_DOUBLE,RL_8,RL_
};
unsigned char IsRegClass(hw_reg_set regs,unsigned char type_class)
{
    hw_reg_set *list;
    list=RegSets[IsSets[type_class]];
    while (*list!=0) {
        if (*list==regs) break;
        ++list;
    }
    return *list!=0;
}

/* Actual Reg64Order79590..795b0; distinct from allocation DoubleRegs. */
static hw_reg_set Reg64Order[]={HW_EAX,HW_EBX,HW_ESI,HW_EDI,HW_EDX,HW_ECX,HW_BP,HW_SP};
hw_reg_set Low64Reg(hw_reg_set reg)
{
    hw_reg_set low;
    unsigned i;
    if (reg==0) return 0;
    /* The1994 unbounded scan for ST0/ST1 runs beyond Reg64Order into the
     * adjacent initialized cells at798cc/798d4. Each has low==reg, hence0.
     * Preserve only these encountered layout-dependent results without
     * relying on our object's placement; other unsupported masks trap below. */
    if (reg==0x10000U || reg==0x20000U) return 0;
    i=0;
    while (!(Reg64Order[i]&reg)) {
        /* Valid physical register pairs always intersect this complete order.
         * Do not read unrelated data for an invalid register-only domain. */
        if (++i==sizeof(Reg64Order)/sizeof(Reg64Order[0])) STUB_TRAP();
    }
    low=reg&Reg64Order[i];
    if (low==reg) low=0;
    return low;
}

unsigned char Op1Possible(instruction *ins);
hw_reg_set Op1Reg(instruction *ins)
{
    return *RegSets[Op1Possible(ins)];
}
unsigned char ResultPossible(instruction *ins);
hw_reg_set ResultReg(instruction *ins)
{
    return *RegSets[ResultPossible(ins)];
}
hw_reg_set ZapReg(instruction *ins)
{
    if (ins->u.gen_table->reg_set>=64) STUB_TRAP();
    return *RegSets[RegList[ins->u.gen_table->reg_set].zap];
}
