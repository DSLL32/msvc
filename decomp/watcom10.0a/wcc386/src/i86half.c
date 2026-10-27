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
* Description:  Split wide moves into native DWORD operations.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned char Overlaps(name *left,name *right);
name *LowPart(name *op,unsigned char type_class);
name *HighPart(name *op,unsigned char type_class);
void HalfType(instruction *ins);
void DupSeg(instruction *ins,instruction *new_ins);
void DupSegOp(instruction *ins,instruction *new_ins,int operand);
void DupSegRes(instruction *ins,instruction *new_ins);
/* 1994 i86half.c uses the general Overlaps query directly, before later
 * IndexOverlaps/CnvOpToInt and the added reverse-register-copy handling. */
instruction *rSPLITMOVE(instruction *ins)
{
    instruction *new_ins,*second;
    name *temp;
    if (Overlaps(ins->result,ins->operands[0])) {
        temp=AllocTemp(FD); /* Old eight-byte type, before U8 existed. */
        new_ins=MakeMove(LowPart(ins->operands[0],U4),LowPart(temp,U4),U4);
        second=MakeMove(HighPart(ins->operands[0],U4),HighPart(temp,U4),U4);
        DupSegOp(ins,new_ins,0);
        DupSegOp(ins,second,0);
        ins->operands[0]=temp;
        PrefixIns(ins,new_ins);
        PrefixIns(ins,second);
        second=MakeMove(LowPart(temp,U4),LowPart(ins->result,U4),U4);
        DupSegRes(ins,second);
        PrefixIns(ins,second);
        second=MakeMove(HighPart(temp,U4),HighPart(ins->result,U4),U4);
        ReplIns(ins,second);
    } else {
        HalfType(ins);
        new_ins=MakeMove(LowPart(ins->operands[0],ins->type_class),
                         LowPart(ins->result,ins->type_class),ins->type_class);
        DupSeg(ins,new_ins);
        ins->operands[0]=HighPart(ins->operands[0],ins->type_class);
        ins->result=HighPart(ins->result,ins->type_class);
        PrefixIns(ins,new_ins);
    }
    return new_ins;
}
