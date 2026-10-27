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
* Description:  i386 segmented-pointer type selection.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "options.h"
#include "stubs.h"
#include <recon.h>

name *OffsetPart(name *opnd) { STUB_TRAP();return NULL; }
name *SegmentPart(name *opnd) { STUB_TRAP();return NULL; }

hw_reg_set Low16Reg(hw_reg_set reg);
hw_reg_set Low32Reg(hw_reg_set reg);
hw_reg_set Low64Reg(hw_reg_set reg);
name *AllocUIntConst(unsigned value);
struct constant_defn *GetFloat(name *cons,unsigned char type_class);
name *AddrConst(name *value,int seg,unsigned char kind);
void Zoiks(int code);
name *LowPart(name *value,unsigned char type_class)
{
    name *part;
    struct constant_defn *float_value;
    unsigned long low;
    switch (value->n.class) {
    case 0:
        if (value->c.const_type==0) {
            if (type_class==U1) part=AllocUIntConst((unsigned char)value->c.int_value);
            else if (type_class==I1) part=AllocIntConst((signed char)value->c.int_value);
            else if (type_class==U2) part=AllocUIntConst((unsigned short)value->c.int_value);
            else if (type_class==I2) part=AllocIntConst((short)value->c.int_value);
            else {
                /* The1994 body predates later I4/U4 special cases. */
                float_value=GetFloat(value,FD);
                low=((unsigned long)float_value->value[1]<<16)+float_value->value[0];
                part=AllocConst(CFCnvU32F(low));
            }
        } else if (value->c.const_type==1) {
            part=AddrConst(value->c.value,value->c.int_value,2);
        } else { Zoiks(44);STUB_TRAP();return NULL; }
        break;
    case 3:
        if (type_class==U1 || type_class==I1)
            part=AllocRegName(Low16Reg(value->r.reg));
        else if (type_class==U2 || type_class==I2)
            part=AllocRegName(Low32Reg(value->r.reg));
        else part=AllocRegName(Low64Reg(value->r.reg));
        break;
    case 2:
        part=TempOffset(value,0,type_class);
        if (part->t.temp_flags&0x100)
            part->v.symbol=LowPart(value->v.symbol,type_class);
        break;
    case 1:
        part=AllocMemory(value->v.symbol,value->v.offset,value->m.memory_type,type_class);
        part->v.usage=value->v.usage;
        break;
    case 4:
        part=ScaleIndex(value->i.index,value->i.base,value->i.constant,type_class,
                        0,value->i.scale,value->i.index_flags);
        break;
    default: STUB_TRAP();return NULL;
    }
    return part;
}

void ChangeType(instruction *ins,unsigned char type_class);
instruction *rSPLIT8(instruction *ins);
/* The old fixed EDX:EAX conversion precedes the later generic HalfClass /
 * LowPart implementation: retain its actual XOR clearing instruction. */
instruction *rCLRHI_D(instruction *ins)
{
    name *high;
    instruction *new_ins;
    ChangeType(ins,U4);
    ins->head.opcode=0x26;
    ins->result=AllocRegName(HW_EAX);
    high=AllocRegName(HW_EDX);
    new_ins=MakeBinary(11,high,high,high,U4);
    PrefixIns(ins,new_ins);
    /* Actual restored-register prefix falls into the independent identity
     * entry at65661; its EAX pointer result is meaningful, not a void hook. */
    return rSPLIT8(new_ins);
}

/* Initial386splt2.c folds these unavailable-on-386 split reductions to a
 * genuine identity function. It consumes/returns the instruction pointer;
 * this is not one of the statically excluded empty void hooks. */
instruction *rSPLIT8(instruction *ins)
{
    return ins;
}

hw_reg_set High16Reg(hw_reg_set reg);
hw_reg_set High32Reg(hw_reg_set reg);
hw_reg_set High64Reg(hw_reg_set reg);
name *HighPart(name *value,unsigned char type_class)
{
    name *part;
    struct constant_defn *float_value;
    unsigned long high;
    switch (value->n.class) {
    case 0:
        if (value->c.const_type==0) {
            if (type_class==U1) part=AllocUIntConst((unsigned char)((unsigned long)value->c.int_value>>8));
            else if (type_class==I1) part=AllocIntConst((signed char)((unsigned long)value->c.int_value>>8));
            else if (type_class==U2) part=AllocUIntConst((unsigned short)((unsigned long)value->c.int_value>>16));
            else if (type_class==I2) part=AllocIntConst((short)((unsigned long)value->c.int_value>>16));
            else {
                /* Earlier U4/I4 high halves also go through target FD bits. */
                float_value=GetFloat(value,FD);
                high=((unsigned long)float_value->value[3]<<16)+float_value->value[2];
                part=AllocConst(CFCnvU32F(high));
            }
        } else if (value->c.const_type==1) {
            part=AddrConst(value->c.value,value->c.int_value,3); /* CONS_SEGMENT */
        } else { Zoiks(44);STUB_TRAP();return NULL; }
        break;
    case 3:
        if (type_class==U1 || type_class==I1)
            part=AllocRegName(High16Reg(value->r.reg));
        else if (type_class==U2 || type_class==I2)
            part=AllocRegName(High32Reg(value->r.reg));
        else part=AllocRegName(High64Reg(value->r.reg));
        break;
    case 2:
        part=TempOffset(value,value->n.size/2,type_class);
        if (part->t.temp_flags&0x100)
            part->v.symbol=HighPart(value->v.symbol,type_class);
        break;
    case 1:
        part=AllocMemory(value->v.symbol,value->v.offset+value->n.size/2,
                         value->m.memory_type,type_class);
        part->v.usage=value->v.usage;
        break;
    case 4:
        part=ScaleIndex(value->i.index,value->i.base,value->i.constant+value->n.size/2,
                         type_class,0,value->i.scale,value->i.index_flags);
        break;
    default:STUB_TRAP();return NULL;
    }
    return part;
}
