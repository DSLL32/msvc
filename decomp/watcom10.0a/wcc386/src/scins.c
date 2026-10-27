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
#include "score.h"
#include "stubs.h"
#include <recon.h>

unsigned char IsStackReg(name *op);
unsigned char TryRegOp(score *sc,instruction *ins,name **operand);
unsigned char TryOldIndex(score *sc,instruction *ins,name **operand);
int NumOperands(instruction *ins);
unsigned char FindRegOpnd(score *sc,instruction *ins)
{
    int i;
    unsigned char change;
    if (IsStackReg(ins->result)) return 0;
    i=NumOperands(ins);
    change=0;
    while (--i>=0)
        if (TryRegOp(sc,ins,&ins->operands[i])) change=1;
    if (ins->result!=NULL && TryOldIndex(sc,ins,&ins->result)) change=1;
    return change;
}
extern score_reg **ScoreList;
unsigned char CanReplace(instruction *ins);
unsigned char ChangeIns(instruction *ins,name *op,name **owner,int change);
void ScoreInfo(score_info *info,name *op);
unsigned char ScoreEqual(score *sc,int index,score_info *info);
unsigned char TryRegOp( score *sc, instruction *ins, name **opp ) {
/********************************************************************
    See if we can find an equivalent register operand for the operand
    "*opp" in instruction "ins", given that the current state of
    registers is reflected by scoreboard "sc".
*/

    name        *op;
    int         i;
    hw_reg_set  live;
    score       *this_reg;
    score       *curr_reg;
    score_info  info;

    if( CanReplace( ins ) == 0 ) return( 0 );
    op = *opp;
    if( op->n.class == 3 ) {
        live = ins->head.next->head.live.regs;
        this_reg = &sc[ op->r.reg_index ];
        if( !(live&op->r.reg) ) {
            curr_reg = this_reg->next_reg;
            while( curr_reg != this_reg ) {
                if( (live&ScoreList[curr_reg->index]->reg)
                && curr_reg->generation < this_reg->generation
                && ChangeIns( ins, (name *)ScoreList[curr_reg->index]->reg_name,
                              opp, 2 ) )
                     return( 1 );
                curr_reg = curr_reg->next_reg;
            }
        }
        curr_reg = this_reg->next_reg;
        while( curr_reg != this_reg ) {
            if( curr_reg->generation < this_reg->generation
             && ChangeIns( ins, (name *)ScoreList[curr_reg->index]->reg_name,
                           opp, 2 ) )
                return( 1 );
            curr_reg = curr_reg->next_reg;
        }
        return( 0 );
    } else {
        ScoreInfo( &info, op );
        if( info.class == 0 ) {
            if( (ins->head.opcode>=0x2e && ins->head.opcode<=0x35) &&
                info.symbol.p == NULL && info.offset == 0 ) {
                /* don't change cmp x,0 */
                return( 0 );
            }
            if( (ins->type_class==FS || ins->type_class==FD) ) {
                /* careful -- info->offset is NOT right for FP consts! */
                return( 0 );
            }
        }
        i = ScoreCount;
        while( -- i >= 0 ) {
            if( ScoreEqual( sc, i, &info )
             && ChangeIns( ins, (name *)ScoreList[ i ]->reg_name, opp, 2 ) )
                return( 1 );
        }

        /*% couldn't find a register operand, try for an older index*/
        return( TryOldIndex( sc, ins, opp ) );
    }
}



opcode_entry *FindGenEntry(instruction *ins,unsigned char *has_index);
instruction *rSWAPOPS(instruction *ins);
instruction *rSWAPCMP(instruction *ins);
opcode_entry *ResetGenEntry(instruction *ins)
{
    opcode_entry *try;
    unsigned char dummy;
    try=FindGenEntry(ins,&dummy);
    switch (try->generate) {
    case 151: rSWAPOPS(ins);try=FindGenEntry(ins,&dummy);break;
    case 150: rSWAPCMP(ins);try=FindGenEntry(ins,&dummy);break;
    }
    return try;
}
unsigned char ChangeIns(instruction *ins,name *to,name **op,int flags)
{
    opcode_entry *try,*table,*gen_table;
    name *save_result,*old_op,*save_ops[4];
    int i;
    unsigned char class,ok;
    ok=1;
    table=ins->table;
    gen_table=ins->u.gen_table;
    old_op=*op;
    class=ins->head.opcode==0x24 ? ins->base_type_class : ins->type_class;
    if (to->n.class!=0 && TypeClassSize[class]!=to->n.size) return 0;
    i=ins->num_operands;
    while (--i>=0) save_ops[i]=ins->operands[i];
    save_result=ins->result;
    if ((flags&1) || to->n.class==4) {
        i=ins->num_operands;
        while (--i>=0) if (ins->operands[i]==old_op) ins->operands[i]=to;
        if (ins->result==old_op) ins->result=to;
    } else {
        *op=to;
    }
    if (flags&2) {
        try=ResetGenEntry(ins);
        if (try->generate<103 && (!(ins->ins_flags&0x20)
          || (ins->u.gen_table->op_type&0xc000)==(gen_table->op_type&0xc000))) {
            if (to->n.class!=4 && old_op->n.class==4) ins->num_operands=NumOperands(ins);
        } else {
            ok=0;
        }
    }
    if (!ok || (flags&4)) {
        i=ins->num_operands;
        while (--i>=0) ins->operands[i]=save_ops[i];
        ins->result=save_result;
        ins->table=table;
        ins->u.gen_table=gen_table;
    }
    return ok;
}


unsigned char IndexOkay(instruction *ins,name *index);
unsigned char TryOldIndex(score *sc,instruction *ins,name **opp)
{
    name *op,*index,*reg_name;
    score *this_reg,*curr_reg;
    op=*opp;
    if (op->n.class!=4 || op->i.index->n.class!=3
      || op->i.index->r.reg_index==NO_INDEX) return 0;
    this_reg=&sc[op->i.index->r.reg_index];
    curr_reg=this_reg->next_reg;
    while (curr_reg!=this_reg) {
        if (curr_reg->generation<this_reg->generation) {
            reg_name=(name *)ScoreList[curr_reg->index]->reg_name;
            if (*opp!=ins->result || !(reg_name->r.reg&ins->zap->reg)) {
                index=ScaleIndex(reg_name,op->i.base,op->i.constant,
                    op->n.name_class,op->n.size,op->i.scale,op->i.index_flags);
                if (IndexOkay(ins,index) && ChangeIns(ins,index,opp,0)) return 1;
            }
        }
        curr_reg=curr_reg->next_reg;
    }
    return 0;
}

unsigned char RegsEqual(score *sc,int left,int right);
void RegKill(score *sc,hw_reg_set reg);
void RegAdd(score *sc,int dst,int src);
void ScoreAssign(score *sc,int index,score_info *info);
void ScoreKillInfo(score *sc,name *op,score_info *info,hw_reg_set except);
unsigned char FPIsConvert(instruction *ins);
unsigned char    ScoreMove( score *sc, instruction *ins ) {
/*********************************************************
    Update "sc" to reflect the affect of an OP_MOV instruction "ins" on
    the registers and memory locations.
*/

    name        *src;
    name        *dst;
    int         src_index;
    int         dst_index;
    score_info  info;

    src = ins->operands[ 0 ];
    dst = ins->result;
    if( src->n.class == 3 ) {
        src_index = src->r.reg_index;
    }
    if( dst->n.class == 3 ) {
        dst_index = dst->r.reg_index;
        if( src->n.class == 3 ) {
            if( RegsEqual( sc,  dst_index, src_index ) ) {
                FreeIns( ins );
                return( 1 );
            } else {
                RegKill( sc, dst->r.reg );
                RegAdd( sc, dst_index, src_index );
            }
        } else {
            ScoreInfo( &info, src );
            if( ScoreEqual( sc, dst->r.reg_index, &info ) ) {
                FreeIns( ins );
                return( 1 );
            } else {
                RegKill( sc, dst->r.reg );
                /* NB: reg can never have the value x[reg]*/
               if( info.index_reg == NO_INDEX
                || !(dst->r.reg&ScoreList[info.index_reg]->reg) ) {
                    if( !FPIsConvert( ins ) ) {
                        ScoreAssign( sc, dst_index, &info );
                    }
                }
            }
        }
    } else {
        ScoreInfo( &info, dst );
        if( src->n.class == 3 ) {       /* and dst is not a register*/
            if( ScoreEqual( sc, src->r.reg_index, &info ) ) {
                FreeIns( ins );
                return( 1 );
            } else {
                ScoreKillInfo( sc, dst, &info,
                                  src->r.reg );
                if( !FPIsConvert( ins ) ) {
                    ScoreAssign( sc, src_index, &info );
                }
            }
        } else {
            ScoreKillInfo( sc, dst, &info, 0 );
        }
    }
    return( 0 );
}


extern score_info *ScZero;
void ScZeroCheck(score *sc,instruction *ins)
{
    if (ins->head.opcode!=0xb && ins->head.opcode!=8 && ins->head.opcode!=3) return;
    if (ins->operands[0]!=ins->operands[1] || ins->operands[0]!=ins->result
      || ins->operands[0]->n.class!=3) return;
    ScoreAssign(sc,ins->result->r.reg_index,ScZero);
}

unsigned char ScoreLAInfo(score_info *info,name *op);
unsigned char ScoreLA(score *sc,instruction *ins)
{
    name *source,*dest;
    int dest_index;
    score_info info;
    source=ins->operands[0];
    dest=ins->result;
    if (dest->n.class==3) {
        dest_index=dest->r.reg_index;
        if (!ScoreLAInfo(&info,source)) RegKill(sc,dest->r.reg);
        else if (ScoreEqual(sc,dest->r.reg_index,&info)) {
            FreeIns(ins);
            return 1;
        } else {
            RegKill(sc,dest->r.reg);
            ScoreAssign(sc,dest_index,&info);
        }
    } else {
        ScoreInfo(&info,dest);
        ScoreKillInfo(sc,dest,&info,0);
    }
    return 0;
}

void ScoreMakeEqual(score *sc,name *left,name *right)
{
    int index;
    name *tmp;
    score_info info;
    if (left->n.class==3) {
        tmp=left;
        left=right;
        right=tmp;
    }
    if (right->n.class==3) {
        index=right->r.reg_index;
        if (left->n.class==3) RegAdd(sc,index,left->r.reg_index);
        else {
            ScoreInfo(&info,left);
            /* A register cannot be equivalent to an address indexed by
             * that same register. */
            if (info.index_reg==NO_INDEX || !(right->r.reg&ScoreList[info.index_reg]->reg))
                ScoreAssign(sc,index,&info);
        }
    }
}
