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
* Description:  Split memory-based indices into backend temporaries.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

void NoMemIndex(instruction *ins)
{
    int i;
    name *bad_index;
    for (;;) {
        i=ins->num_operands;
        bad_index=NULL;
        while (--i>=0) {
            if (ins->operands[i]->n.class==4 && ins->operands[i]->i.index->n.class==1)
                bad_index=ins->operands[i];
        }
        if (ins->result!=NULL && ins->result->n.class==4 &&
            ins->result->i.index->n.class==1) bad_index=ins->result;
        if (bad_index==NULL) break;
        IndexToTemp(ins,bad_index);
    }
}

name *IndexToTemp(instruction *ins,name *index) { STUB_TRAP(); return NULL; }

/* Semantic operand counts from released fixindex.c, adapted to the old
 * 76-opcode enum: no P5DIV/later SET or target-specific additions. Extra
 * instruction operands beyond this count are segment overrides. */
static unsigned char NumTab[0x4c]={
    0, /* OP_NOP */
    2, /* OP_ADD */
    2, /* OP_EXT_ADD */
    2, /* OP_SUB */
    2, /* OP_EXT_SUB */
    2, /* OP_MUL */
    2, /* OP_EXT_MUL */
    2, /* OP_DIV */
    2, /* OP_MOD */
    2, /* OP_AND */
    2, /* OP_OR */
    2, /* OP_XOR */
    2, /* OP_RSHIFT */
    2, /* OP_LSHIFT */
    2, /* OP_POW */
    2, /* OP_ATAN2 */
    2, /* OP_FMOD */
    1, /* OP_NEGATE */
    1, /* OP_COMPLEMENT */
    1, /* OP_LOG */
    1, /* OP_COS */
    1, /* OP_SIN */
    1, /* OP_TAN */
    1, /* OP_SQRT */
    1, /* OP_FABS */
    1, /* OP_ACOS */
    1, /* OP_ASIN */
    1, /* OP_ATAN */
    1, /* OP_COSH */
    1, /* OP_EXP */
    1, /* OP_LOG10 */
    1, /* OP_SINH */
    1, /* OP_TANH */
    1, /* OP_PTR_TO_NATIVE */
    1, /* OP_PTR_TO_FORIEGN */
    0, /* OP_SLACK_19 */
    1, /* OP_CONVERT */
    1, /* OP_ROUND */
    1, /* OP_MOV */
    1, /* old careful address operation */
    1, /* OP_LA */
    3, /* OP_CALL_INDIRECT */
    1, /* OP_PUSH */
    0, /* OP_POP */
    0, /* OP_PARM_DEF */
    1, /* OP_SELECT */
    2, /* OP_BIT_TEST_TRUE */
    2, /* OP_BIT_TEST_FALSE */
    2, /* OP_CMP_EQUAL */
    2, /* OP_CMP_NOT_EQUAL */
    2, /* OP_CMP_GREATER */
    2, /* OP_CMP_LESS_EQUAL */
    2, /* OP_CMP_LESS */
    2, /* OP_CMP_GREATER_EQUAL */
    2, /* OP_CALL */
    0, /* old unused opcode0x37 */
    0, /* old unused opcode0x38 */
    0, /* old unused opcode0x39 */
    0, /* old unused opcode0x3a */
    0, /* old unused opcode0x3b */
    0, /* old unused opcode0x3c */
    0, /* old unused opcode0x3d */
    0, /* old unused opcode0x3e */
    0, /* old unused opcode0x3f */
    0, /* old unused opcode0x40 */
    0, /* old unused opcode0x41 */
    0, /* old unused opcode0x42 */
    0, /* old unused opcode0x43 */
    0, /* old unused opcode0x44 */
    0, /* old unused opcode0x45 */
    0, /* old unused opcode0x46 */
    0, /* old unused opcode0x47 */
    0, /* old unused opcode0x48 */
    0, /* old unused opcode0x49 */
    0, /* old unused opcode0x4a */
    0, /* OP_BLOCK */
};
int NumOperands(instruction *ins)
{
    if (ins->head.opcode>=sizeof(NumTab)) STUB_TRAP();
    return NumTab[ins->head.opcode];
}

instruction *OneMemRef(instruction *ins);
void FixIndex(void)
{
    block *blk;
    instruction *ins;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            if (ins->type_class!=10 && ins->head.opcode!=0x28 &&
                ins->head.opcode!=0x27) ins=OneMemRef(ins);
            ins=ins->head.next;
        }
    }
}
void FixFPConsts(instruction *ins);
unsigned char RTLeaveOp2(instruction *ins);
name *OpTemp(instruction *ins,unsigned i,unsigned j);
instruction *Split3(instruction *ins);
instruction *ResTemp(instruction *ins);
instruction *OneMemRef( instruction *ins ) {
/*******************************************************
    see FixIndex
*/

    name        *op1;
    name        *op2;
    name        *res;

    FixFPConsts( ins );
    if( ins->num_operands == 2 ) {
        if( ins->result != NULL ) { /* the tough case*/
            op1 = ins->operands[ 0 ];
            if( op1->n.class != 4 && op1->n.class != 1 ) {
                op1 = NULL;
            }
            op2 = ins->operands[ 1 ];
            if( op2->n.class != 4 && op2->n.class != 1 ) {
                op2 = NULL;
            }
            res = ins->result;
            if( res->n.class != 4 && res->n.class != 1 ) {
                res = NULL;
            }
            if( op1 != NULL && op2 != NULL && res != NULL ) {
                /* three memory references*/
                if( RTLeaveOp2( ins ) ) {
                    OpTemp( ins, 0, 0 );
                    ins = ResTemp( ins );
                } else if( op1 == op2 ) {
                    OpTemp( ins, 0, 1 );
                } else if( op1 == res ) {
                    OpTemp( ins, 1, 1 );
                } else if( op2 == res ) {
                    OpTemp( ins, 0, 0 );
                } else {
                    ins = Split3( ins );
                }
            } else if( op1 == NULL && op2 == NULL ) {
                /* one memory reference*/
            } else if( op1 == NULL && res == NULL ) {
                /* one memory reference*/
            } else if( op2 == NULL && res == NULL ) {
                /* one memory reference*/
            } else if( op1 != res && op2 != res && op1 != op2 ) {
               /* two memory references*/
                if( RTLeaveOp2( ins ) ) {
                    OpTemp( ins, 0, 0 );
                    ins = ResTemp( ins );
                } else {
                    ins = Split3( ins );
                }
            }
        } else {
            op1 = ins->operands[ 0 ];
            op2 = ins->operands[ 1 ];
            if( op1->n.class != 4 && op1->n.class != 1 ) {
                return( ins );
            }
            if( op2->n.class != 4 && op2->n.class != 1 ) {
                return( ins );
            }
            if( op1 == op2 ) return( ins );
            OpTemp( ins, 0, 0 );
        }
    } else if( ins->num_operands == 1 && ins->result != NULL ) {
        op1 = ins->operands[ 0 ];
        res = ins->result;
        if( op1->n.class != 4 && op1->n.class != 1 ) {
            return( ins );
        }
        if( res->n.class != 4 && res->n.class != 1 ) {
            return( ins );
        }
        if( op1 == res ) return( ins );
        if( ins->type_class == 10 ) return( ins );
        if( ( ins->head.opcode == 0x24 || ins->head.opcode == 0x25 ) && ins->type_class > ins->base_type_class ) {
            ResTemp( ins ); /* 90-Nov-29 */
        } else {
            OpTemp( ins, 0, 0 );
        }
    }
    return( ins );
}

unsigned char FPInsClass(instruction *ins);
name *OpTemp(instruction *ins,unsigned i,unsigned j)
{
    name *temp;
    unsigned char class;
    instruction *new_ins;
    class=FPInsClass(ins);
    if (class==10) {
        if (ins->operands[i]->n.name_class==10) class=ins->type_class;
        else class=ins->operands[i]->n.name_class;
    }
    temp=AllocTemp(class);
    FPSetStack(temp);
    new_ins=MakeMove(ins->operands[i],temp,class);
    ins->operands[i]=temp;
    ins->operands[j]=temp;
    PrefixIns(ins,new_ins);
    return temp;
}
unsigned char FPIsStack(name *opnd);
instruction *Split3(instruction *ins)
{
    name *temp;
    instruction *new_ins;
    if (ins->operands[0]->n.class==4 || ins->operands[0]->n.class==1) {
        temp=OpTemp(ins,0,0);
        if (temp->n.name_class!=ins->result->n.name_class || FPIsStack(temp)) {
            temp=AllocTemp(ins->result->n.name_class);
            FPSetStack(temp);
        }
        new_ins=MakeMove(temp,ins->result,temp->n.name_class);
        ins->result=temp;
        SuffixIns(ins,new_ins);
        ins=new_ins;
    } else OpTemp(ins,1,1);
    return ins;
}
instruction *ResTemp(instruction *ins) { STUB_TRAP();return NULL; }




unsigned char IndexOkay(instruction *ins,name *index);
name *FindIndex(instruction *ins)
{
    name *index;
    int i;
    for (i=0;i<ins->num_operands;++i) {
        index=ins->operands[i];
        if (index->n.class==4 && !IndexOkay(ins,index)) return index;
    }
    index=ins->result;
    if (index==NULL || index->n.class!=4 || IndexOkay(ins,index)) return NULL;
    return index;
}

void ReplaceOperand(instruction *ins,name *old_op,name *new_op)
{
    int i;
    i=ins->num_operands;
    while (--i>=0) {
        if (ins->operands[i]==old_op) ins->operands[i]=new_op;
    }
    if (ins->result==old_op) ins->result=new_op;
}
