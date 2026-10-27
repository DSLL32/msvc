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
* Description:  Expand backend operations into target instruction patterns.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "pattern.h"
#include "regchoices.h"
#include "stubs.h"
#include <recon.h>

instruction *ExpandIns(instruction *ins);
int ExpandOps(unsigned char keep_on_truckin)
{
    block *blk;
    instruction *ins;
    int unknowns;
    unknowns=0;
    blk=HeadBlock;
    while (blk!=NULL) {
        /* Released EXBlip shares the target's existing DGBlip vertex. */
        DGBlip();
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            if (!keep_on_truckin && MemLow()) {
                unknowns=-1;
                break;
            }
            ins=ExpandIns(ins);
            if (ins->head.state==0) ins=ExpandIns(ins);
            else {
                if (ins->head.state==1) ++unknowns;
                ins=ins->head.next;
            }
        }
        if (unknowns==-1) break;
        blk=blk->next_block;
    }
    return unknowns;
}
opcode_entry *FindGenEntry(instruction *ins,unsigned char *has_index);
instruction *DoReduce(instruction *ins,opcode_entry *entry,unsigned char has_index);
instruction *ExpandIns(instruction *ins)
{
    opcode_entry *entry;
    unsigned char has_index;
    entry=FindGenEntry(ins,&has_index);
    return DoReduce(ins,entry,has_index);
}
opcode_entry *CodeTable(instruction *ins);
unsigned short ClassifyOps(instruction *ins,unsigned char *has_index);
unsigned char DoVerify(unsigned verify,instruction *ins);
unsigned char VerifyRegs(instruction *ins,unsigned short ops);
unsigned char CallIPossible(instruction *ins);
unsigned char SpecialPossible(instruction *ins);
void MarkPossible(instruction *ins,name *opnd,unsigned char index);
opcode_entry *FindGenEntry(instruction *ins,unsigned char *has_index)
{
    opcode_entry *entry;
    unsigned short ops;
    unsigned verify;
    ins->head.state=2; /* INS_READY */
    entry=ins->table;
    if (entry==NULL) {
        entry=CodeTable(ins);
        ins->table=entry;
    }
    if (entry==NULL) return entry;
    ops=ClassifyOps(ins,has_index);
    for (;;++entry) {
        if ((ops&entry->op_type)!=ops) continue;
        verify=entry->verify;
        if (verify!=0) {
            if (!DoVerify(verify&0x7f,ins)) continue;
            if ((verify&0x80) && VolatileIns(ins)) continue;
        }
        ins->u.gen_table=entry;
        if (entry->reg_set!=0 && !VerifyRegs(ins,ops)) continue;
        if (ins->head.opcode==0x29)
            MarkPossible(ins,ins->operands[2],CallIPossible(ins));
        break;
    }
    return entry;
}
unsigned short ClassifyOps( instruction *ins, unsigned char *has_index ) {
/************************************************************************/

    name                *opnd;
    operand_types       ops;
    int                 num_operands;

    num_operands = NumOperands( ins );
    *has_index = 0;
    ops = NONE;
    opnd = ins->result;
    if( opnd != NULL && num_operands != 0 ) {
        if( ins->operands[ 0 ] == opnd ) {
            ops &= EQ_R1;
        }
        if( num_operands != 1 ) {
            if( ins->operands[ 1 ] == opnd ) {
                ops &= EQ_R2;
            }
        }
    }
    if( opnd != NULL ) {
        switch( opnd->n.class ) {
        case 3:
            ops |= R_R;
            break;
        case 4:
            *has_index = 1;
            ops |= M_R;
            break;
        case 1:
        case 2:
            if( opnd->v.conflict == NULL ) {
                ops |= M_R;
            } else if( opnd->v.usage & 0x10 ) {
                ops |= M_R;
            } else {
                ops |= U_R;
                ins->head.state = 1;
            }
            break;
        }
    }
    if( num_operands != 0 ) {
        opnd = ins->operands[ 0 ];
        switch( opnd->n.class ) {
        case 0:
            ops |= C_1;
            break;
        case 3:
            ops |= R_1;
            break;
        case 4:
            *has_index = 1;
            ops |= M_1;
            break;
        case 1:
        case 2:
            if( opnd->v.conflict == NULL ) {
                ops |= M_1;
            } else if( opnd->v.usage & 0x10 ) {
                ops |= M_1;
            } else {
                ops |= U_1;
                ins->head.state = 1;
            }
            break;
        }
    }
    if( num_operands > 1 ) {
        opnd = ins->operands[ 1 ];
        switch( opnd->n.class ) {
        case 0:
            ops |= C_2;
            break;
        case 3:
            ops |= R_2;
            break;
        case 4:
            *has_index = 1;
            ops |= M_2;
            break;
        case 1:
        case 2:
            if( opnd->v.conflict == NULL ) {
                ops |= M_2;
            } else if( opnd->v.usage & 0x10 ) {
                ops |= M_2;
            } else {
                ops |= U_2;
                ins->head.state = 1;
            }
            break;
        }
        if( num_operands > 2 ) {
            opnd = ins->operands[ 2 ];
            if( opnd->n.class == 4 ) {
                *has_index = 1;
            }
            if( ins->head.opcode == 0x29 ) {
                switch( opnd->n.class ) {
                case 1:
                case 2:
                    if( opnd->v.conflict != NULL && !( opnd->v.usage & 0x10 ) ) {
                        ins->head.state = 1;
                    }
                }
            }
        }
    }
    return( ops );
}



unsigned char VerifyRegs( instruction *ins, operand_types ops ) {
/*****************************************************************/

#define _Any( ops, mul )        ( ( ops & (ANY*mul) ) == (ANY*mul) )

    opcode_entry        *try;
    hw_reg_set          *possible;
    name                *name;
    op_regs             *need;
    hw_reg_set          regs;
    unsigned char       left_index;
    unsigned char       right_index;
    unsigned char       result_index;
    unsigned char       special_index;

    try = ins->u.gen_table;
    need = &RegList[  try->reg_set  ];
    result_index = RL_;
    name = ins->result;
    if( ( ops & R_R ) == 0 ) {
        if( ops & U_R ) {
            result_index = need->result;
        }
    } else if( need->result != RL_ ) {
        if( name->n.class != 3 ) {
            result_index = need->result;
        } else if( !_Any( try->op_type, RESULT_MUL ) ) {
            regs = name->r.reg;
            possible = RegSets[  need->result  ];
            for(;;) {
                if( (*possible == 0) ) return( 0 );
                if( (*possible == regs) ) break;
                ++ possible;
            }
        }
    }
    left_index = RL_;
    if( ( ops & R_1 ) == 0 ) {
        if( ops & U_1 ) {
            left_index = need->left;
        }
    } else if( need->left != RL_ ) {
        name = ins->operands[ 0 ];
        if( name->n.class != 3 ) {
            left_index = need->left;
        } else if( !_Any( try->op_type, OP1_MUL ) ) {
            regs = name->r.reg;
            possible = RegSets[  need->left  ];
            for(;;) {
                if( (*possible == 0) ) return( 0 );
                if( (*possible == regs) ) break;
                ++ possible;
            }
        }
    }
    right_index = RL_;
    if( ( ops & R_2 ) == 0 ) {
        if( ops & U_2 ) {
            right_index = need->right;
        }
    } else if( need->right != RL_ ) {
        name = ins->operands[ 1 ];
        if( name->n.class != 3 ) {
            right_index = need->right;
        } else if( !_Any( try->op_type, OP2_MUL ) ) {
            regs = name->r.reg;
            possible = RegSets[  need->right  ];
            for(;;) {
                if( (*possible == 0) ) return( 0 );
                if( (*possible == regs) ) break;
                ++ possible;
            }
        }
    }
    if( try->generate < 0x68 ) {
        special_index = SpecialPossible( ins );
        if( special_index != RL_ ) {
            MarkPossible( ins, ins->result, special_index );
        } else if( result_index != RL_ ) {
            MarkPossible( ins, ins->result, result_index );
        }
        if( left_index != RL_ ) {
            MarkPossible( ins, ins->operands[ 0 ], left_index );
        }
        if( right_index != RL_ ) {
            MarkPossible( ins, ins->operands[ 1 ], right_index );
        }
    }
    return( 1 );
}



instruction *Reduce(instruction *ins);
instruction *NeedIndex(instruction *ins);
instruction *DoReduce(instruction *ins,opcode_entry *entry,unsigned char has_index)
{
    hw_reg_set *zap,zap_all;
    if (entry==NULL || entry->generate==0) return ins;
    if (entry->generate>=0x68) return Reduce(ins);
    zap=RegSets[RegList[entry->reg_set].zap];
    zap_all=ins->zap->reg;
    while (*zap!=0) {
        zap_all|=*zap;
        ++zap;
    }
    ins->zap=(register_name *)AllocRegName(zap_all);
    if (has_index || ins->num_operands>NumOperands(ins)) ins=NeedIndex(ins);
    else ins->t.index_needs=0;
    return ins;
}



void FixGenEntry(instruction *ins)
{
    opcode_entry *old;
    unsigned char dummy;
    old=ins->u.gen_table;
    FindGenEntry(ins,&dummy);
    if (ins->u.gen_table->generate>=0x68) ins->u.gen_table=old;
    ins->head.state=0;
}

