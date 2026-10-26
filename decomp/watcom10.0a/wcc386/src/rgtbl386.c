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


#include "procdef.h"
#include "options.h"
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

hw_reg_set ActualParmReg(hw_reg_set reg)
{
    if (reg&HW_FLTS) return 0x10000; /* ST0 receives all x87 arguments. */
    return reg;
}
