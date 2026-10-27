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
* Description:  Preserve segment operands when splitting i386 instructions.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

void DupSeg(instruction *ins,instruction *new_ins)
{
    if (ins->head.opcode==0x4b) return;
    if (ins->num_operands<=NumOperands(ins)) return;
    if (new_ins->num_operands>NumOperands(new_ins)) return;
    new_ins->operands[new_ins->num_operands++]=ins->operands[ins->num_operands-1];
    new_ins->t.index_needs=ins->t.index_needs;
}
void MoveSeg(instruction *ins,instruction *new_ins,name *op,unsigned char duplicate);
void MoveSegOp(instruction *ins,instruction *new_ins,int i)
{
    MoveSeg(ins,new_ins,new_ins->operands[i],0);
}
unsigned char SegMemLoc(name *op);
void DelSeg(instruction *ins);
void MoveSeg(instruction *ins,instruction *new_ins,name *op,unsigned char duplicate)
{
    int i;
    if (ins->head.opcode==0x4b) return;
    if (ins->num_operands<=NumOperands(ins)) return;
    if (!SegMemLoc(op)) return;
    DupSeg(ins,new_ins);
    if (duplicate) return;
    i=ins->num_operands;
    while (--i>=0) if (op==ins->operands[i]) return;
    if (op==ins->result) return;
    DelSeg(ins);
}

unsigned char SegMemLoc(name *op);
void DelSeg(instruction *ins);
void DelSegOp(instruction *ins,int operand)
{
    int j;
    if (ins->num_operands<=NumOperands(ins)) return;
    if (!SegMemLoc(ins->operands[operand])) return;
    if (ins->result==ins->operands[operand]) return;
    j=ins->num_operands;
    while (--j>=0) {
        if (operand!=j && ins->operands[j]==ins->operands[operand]) return;
    }
    DelSeg(ins);
}

void DelSegRes(instruction *ins)
{
    int i;
    if (ins->num_operands<=NumOperands(ins)) return;
    if (!SegMemLoc(ins->result)) return;
    i=ins->num_operands;
    while (--i>=0) if (ins->result==ins->operands[i]) return;
    DelSeg(ins);
}
unsigned char SegMemLoc(name *op) { STUB_TRAP();return 0; }
void DelSeg(instruction *ins) { STUB_TRAP(); }

void MoveSegRes(instruction *ins,instruction *new_ins)
{
    MoveSeg(ins,new_ins,new_ins->result,0);
}

void DupSegRes(instruction *ins,instruction *new_ins)
{
    MoveSeg(ins,new_ins,new_ins->result,1);
}

void DupSegOp(instruction *ins,instruction *new_ins,int operand) { STUB_TRAP(); }
