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
* Description:  Allocate, construct and release backend instructions.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include <string.h>
#include "codegen.h"
#include "block.h"
#include "stubs.h"
#include <recon.h>
static void *InsFrl;

void InitIns(void)
{
    InitFrl(&InsFrl);
}

instruction *MakeNop(void)
{
    instruction *entry;
    entry=NewIns(0);
    entry->head.opcode=0; /* OP_NOP */
    entry->head.state=2; /* INS_READY */
    entry->type_class=XX;
    DoNothing(entry);
    return entry;
}

instruction *NewIns(int operands)
{
    instruction *entry;
    int index;
    if (operands>4) Zoiks(26);
    entry=AllocFrl(&InsFrl,sizeof(*entry));
    entry->head.state=0; /* INS_NEEDS_WORK */
    entry->t.index_needs=0;
    entry->num_operands=(unsigned char)operands;
    entry->result=NULL;
    memset(&entry->flags,0,sizeof(entry->flags));
    entry->ins_flags=0;
    entry->head.line_num=0;
    entry->sequence=0;
    entry->stk_entry=0;
    entry->stk_exit=0;
    entry->s.stk_extra=0;
    index=4;
    while (--index>=0) entry->operands[index]=NULL;
    entry->table=NULL;
    entry->u.gen_table=NULL;
    entry->base_type_class=XX;
    entry->zap=(register_name *)AllocRegName(0);
    return entry;
}

/* optab.c: the old table_def DONOTHING ordinal is 69. */
void DoNothing(instruction *entry)
{
    entry->table=OpcodeTable(69);
    entry->u.gen_table=entry->table;
}

instruction *MakeConvert(name *source,name *dest,unsigned char dest_class,unsigned char source_class)
{
    return MakeNary(0x24,source,NULL,dest,dest_class,source_class,1); /* OP_CONVERT */
}

instruction *MakeNary(unsigned char opcode,name *left,name *right,name *result,
    unsigned char type_class,unsigned char base_class,int operands)
{
    instruction *ins;
    ins=NewIns(operands);
    ins->head.opcode=opcode;
    ins->operands[0]=left;
    ins->operands[1]=right;
    ins->result=result;
    ins->type_class=type_class;
    ins->base_type_class=base_class;
    return ins;
}

instruction *MakeCondition(unsigned char op,name *left,name *right,
                           int true_index,int false_index,unsigned char type_class)
{
    instruction *ins;
    ins=MakeNary(op,left,right,NULL,type_class,XX,2);
    ins->flags.byte=(unsigned char)true_index|((unsigned char)false_index<<4);
    return ins;
}

instruction *MakeMove(name *source,name *dest,unsigned char type_class)
{
    return MakeNary(0x26,source,NULL,dest,type_class,XX,1); /* OP_MOV */
}

void FreeIns(instruction *ins)
{
    instruction *next;
    next=ins->head.next;
    if (next->head.opcode!=0x4b && next->head.line_num==0)
        next->head.line_num=ins->head.line_num;
    ins->head.prev->head.next=next;
    next->head.prev=ins->head.prev;
    FrlFreeSize(&InsFrl,ins,sizeof(*ins));
}

instruction *MakeUnary(unsigned char opcode,name *source,name *result,unsigned char type_class)
{
    return MakeNary(opcode,source,NULL,result,type_class,XX,1);
}

instruction *MakeBinary(unsigned char opcode,name *left,name *right,name *result,unsigned char type_class)
{
    return MakeNary(opcode,left,right,result,type_class,XX,2);
}
