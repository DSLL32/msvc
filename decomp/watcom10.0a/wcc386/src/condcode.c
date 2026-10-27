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
#include "cgmemory.h"
#include "stubs.h"
#include <recon.h>

#pragma pack(4)
typedef struct cc_control {
    name *left_op,*right_op,*result_op;
    instruction *ins;
    unsigned char op_type,state;
} cc_control;
#pragma pack()
typedef char old_cc_control_size[(sizeof(cc_control)==20)?1:-1];
void    GatherSources( block *blk ) {
/********************************************
    See what the condition code state is at the end of each of the
    ancestors of "blk", and if they are all the same, use this as the
    initial condition code setting for "blk".

*/

    block_edge  *edge;
    cc_control  *source_cc;
    cc_control  *cc;

    edge = blk->input_edges;
    cc = blk->cc;
    if( edge == NULL ) {
        cc->state = 0;
    } else {
        source_cc = edge->source->cc;
        cc->state = source_cc->state;
        cc->left_op = source_cc->left_op;
        cc->right_op = source_cc->right_op;
        cc->result_op = source_cc->result_op;
        cc->op_type = source_cc->op_type;
        for(;;) {
            if( cc->state == 0 ) break;
            edge = edge->next_source;
            if( edge == NULL ) break;
            source_cc = edge->source->cc;
            if( source_cc->state == 0 ) {
                cc->state = 0;
            } else {
                if( source_cc->state < cc->state ) {
                    cc->state = source_cc->state;
                }
                if( source_cc->left_op != cc->left_op
                  || source_cc->right_op != cc->right_op ) {
                    cc->left_op = NULL;
                    cc->right_op = NULL;
                }
                if( source_cc->result_op != cc->result_op ) {
                    cc->result_op = NULL;
                }
                if( cc->result_op == NULL && cc->left_op == NULL ) {
                    cc->state = 0;
                }

            }
        }
    }
}


unsigned char UselessCompare(instruction *ins,cc_control *cc,name *zero)
{
    unsigned char opcode;
    opcode=ins->head.opcode;
    if (cc->state==0) return 0;
    if (ins->operands[0]==cc->left_op && ins->operands[1]==cc->right_op) {
        if (cc->state==3 || opcode==0x30 || opcode==0x31) return 1;
        if (cc->state==1) return 0;
        if (ins->type_class==I1 || ins->type_class==I2) return 1;
    } else if (ins->operands[0]==cc->result_op && ins->operands[1]==zero) {
        if (opcode==0x30 || opcode==0x31) return 1;
    }
    return 0;
}
unsigned char OverLap(name *result,name *ccop)
{
    if (ccop==NULL || result==ccop) return 1;
    if (result->n.class==3) {
        if (ccop->n.class==3 && (result->r.reg&ccop->r.reg)) return 1;
        if (ccop->n.class==4 && (result->r.reg&ccop->i.index->r.reg)) return 1;
    }
    return 0;
}
void MarkUsedCC(block *blk)
{
    instruction *ins;
    block_edge *edge;
    ins=((cc_control *)blk->cc)->ins;
    if (ins!=NULL) {
        ins->ins_flags|=0x20;
    } else {
        /* The old body clears each recursion mark on returning; it has no
         * later out-of-line DoMarkUsedCC/ClearBlockBits pair. */
        blk->class|=0x800U;
        for (edge=blk->input_edges;edge!=NULL;edge=edge->next_source)
            if (!(edge->source->class&0x800U)) MarkUsedCC(edge->source);
        blk->class&=~0x800U;
    }
}
unsigned char VolatileIns(instruction *ins);
unsigned char Traverse(block *blk,name *zero)
{
    instruction *ins;
    cc_control *cc;
    unsigned short cc_affect;
    unsigned char change,opcode;
    int jumps;
    change=0;
    cc=blk->cc;
    cc->ins=NULL;
    ins=blk->ins.hd.next;
    jumps=0;
    while (ins->head.opcode!=0x4b) {
        opcode=ins->head.opcode;
        if ((opcode>=0x2e && opcode<=0x35) || opcode==0x2d) ++jumps;
        cc_affect=ins->u.gen_table->op_type&0xc000;
        /* The later setcc-result special case is absent in the 1994 body. */
        if (VolatileIns(ins) || cc_affect==0) {
            cc->state=0;
        } else if (cc_affect==0x4000) {
            cc->state=(!(opcode>=9 && opcode<=0xb) && (ins->ins_flags&4)) ? 1 : 2;
            cc->result_op=ins->result;
            cc->op_type=ins->type_class;
            if (opcode==3) {
                cc->left_op=ins->operands[0];
                cc->right_op=ins->operands[1];
            } else {
                cc->left_op=NULL;
                cc->right_op=NULL;
            }
            cc->ins=ins;
        } else if (cc_affect==0xc000) {
            if (opcode>=0x30 && opcode<=0x35) {
                if (UselessCompare(ins,cc,zero)) {
                    DoNothing(ins);
                    change=1;
                    MarkUsedCC(blk);
                } else if (ins->u.gen_table->generate!=0) {
                    cc->state=3;
                    cc->result_op=NULL;
                    cc->op_type=ins->type_class;
                    cc->left_op=ins->operands[0];
                    cc->right_op=ins->operands[1];
                    cc->ins=ins;
                }
            } else {
                cc->state=(!(opcode>=9 && opcode<=0xb) && (ins->ins_flags&4)) ? 1 : 3;
                cc->result_op=(opcode==0x2e || opcode==0x2f) ? NULL : ins->result;
                cc->op_type=ins->type_class;
                if (opcode==3) {
                    cc->left_op=ins->operands[0];
                    cc->right_op=ins->operands[1];
                } else {
                    cc->left_op=NULL;
                    cc->right_op=NULL;
                }
                cc->ins=ins;
            }
        }
        if (ins->result!=NULL) {
            if (OverLap(ins->result,cc->left_op) || OverLap(ins->result,cc->right_op)) {
                cc->left_op=NULL;
                cc->right_op=NULL;
            }
            if (cc_affect!=0xc000 && cc_affect!=0x4000 && OverLap(ins->result,cc->result_op))
                cc->result_op=NULL;
        }
        ins=ins->head.next;
    }
    if (jumps>1) cc->state=0;
    return change;
}
void FlowConditions(void)
{
    block *blk;
    name *zero;
    unsigned char change;
    zero=AllocIntConst(0);
    change=1;
    for (;;) {
        blk=HeadBlock;
        while (blk!=NULL) {
            GatherSources(blk);
            change|=Traverse(blk,zero);
            blk=blk->next_block;
        }
        if (!change) break;
        change=0;
    }
}
void Conditions(void)
{
    block *blk;
    cc_control *cc;
    blk=HeadBlock;
    while (blk!=NULL) {
        cc=CGAlloc(sizeof(cc_control));
        cc->state=0;
        cc->left_op=NULL;
        cc->right_op=NULL;
        cc->result_op=NULL;
        cc->ins=NULL;
        cc->op_type=XX;
        blk->cc=cc;
        blk->class&=~0x800U;
        blk=blk->next_block;
    }
    FlowConditions();
    blk=HeadBlock;
    while (blk!=NULL) {
        CGFreeSize(blk->cc,sizeof(cc_control));
        blk=blk->next_block;
    }
}
