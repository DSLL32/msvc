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
#include "genopt.h"
#include "score.h"
#include "zerobits.h"
#include "stubs.h"
#include <recon.h>

void FreeJunk(block *blk)
{
    instruction *ins,*next;
    ins=blk->ins.hd.next;
    while (ins->head.opcode!=0x4b) {
        next=ins->head.next;
        if (!DoesSomething(ins) && !SideEffect(ins) &&
            ins->head.opcode<0x2e && ins->head.opcode!=0) FreeIns(ins);
        ins=next;
    }
}

unsigned char RemDeadCode(block *blk);
unsigned char RegThrash(block *blk);
void ScoreSegments(score *sc);
unsigned char ScoreZero(score *sc,instruction **ins);
unsigned char UnChangeable(instruction *ins);
void ScoreInfo(score_info *info,name *op);
void ScoreKillInfo(score *sc,name *op,score_info *info,hw_reg_set except);
void RegKill(score *sc,hw_reg_set reg);
void MemChanged(score *sc,unsigned char statics_too);
unsigned char FindRegOpnd(score *sc,instruction *ins);
unsigned char ScoreMove(score *sc,instruction *ins);
unsigned char ScoreLA(score *sc,instruction *ins);
unsigned char StupidMove(score *sc,instruction *ins);
void ScZeroCheck(score *sc,instruction *ins);

unsigned char DoScore( block *blk ) {
/**************************************
    Do register scoreboarding on a basic block. Remember which
    registers contain which values as we run through the block and
    then use that information to try to replace memory references
    with register references.
*/

    instruction *next_ins;
    score       *scoreboard;
    name        *dst;
    unsigned char change;
    unsigned char had_condition;
    score_info  info;
    instruction *ins;
    hw_reg_set  tmp;

    change = 0;
    for(;;) {
        DGBlip();
        while( RemDeadCode( blk ) ) {
            UpdateLive( blk->ins.hd.next, blk->ins.hd.prev );
            change = 1;
        }
        if( RegThrash( blk ) == 0 ) break;
        change = 1;
    }
    scoreboard = blk->cc;
    ins = blk->ins.hd.next;
    had_condition = 0;
    while( ins->head.opcode != 0x4b ) {
        ScoreSegments( scoreboard );
        /* May all intel designers rot in hell forever and ever, amen*/
        if( (ins->head.opcode>=0x2e && ins->head.opcode<=0x35) && ins->result == NULL ) {
            if( had_condition ) {
                blk->class |= 0x8000U;
            }
            had_condition = 1;
        }
        if( ScoreZero( scoreboard, &ins ) ) {
            change = 1;
            UpdateLive( blk->ins.hd.next, blk->ins.hd.prev );
        }
        if( ins->head.opcode == 0x4b ) break;
        /* ScoreZero freed the last instr!*/
        next_ins = ins->head.next;
        dst = ins->result;
        if( UnChangeable( ins ) ) {
            if( dst != NULL ) {
                ScoreInfo( &info, dst );
                ScoreKillInfo( scoreboard, dst, &info, 0 );
                RegKill( scoreboard, ins->zap->reg );
            }
        } else if( DoesSomething( ins ) ) {
            if( ins->head.opcode == 0x36
                            || ins->head.opcode == 0x29 ) {
                MemChanged( scoreboard,
                    ( ins->flags.call_flags & 2 ) == 0 );
                RegKill( scoreboard, ins->zap->reg );
                if( dst != NULL && dst->n.class != 3 ) {
                    ScoreInfo( &info, dst );
                    ScoreKillInfo( scoreboard, dst, &info, 0 );
                }
            } else {
                if( FindRegOpnd( scoreboard, ins ) ) {
                    change = 1;
                    UpdateLive( blk->ins.hd.next, blk->ins.hd.prev );
                }
                if( ins->head.opcode == 0x26 && had_condition == 0 ) {
                    if( ScoreMove( scoreboard, ins ) ) {
                        change = 1;
                        UpdateLive( blk->ins.hd.next, blk->ins.hd.prev );
                    }
                    if( next_ins->head.prev == ins ) {
                        RegKill( scoreboard, ins->zap->reg );
                    }
                } else if( ins->head.opcode == 0x28 && had_condition == 0 ) {
                    if( ScoreLA( scoreboard, ins ) ) {
                        change = 1;
                        UpdateLive( blk->ins.hd.next, blk->ins.hd.prev );
                    }
                    if( next_ins->head.prev == ins ) {
                        RegKill( scoreboard, ins->zap->reg );
                    }
                } else if( dst == NULL ) {
                    RegKill( scoreboard, ins->zap->reg );
                } else if( StupidMove( scoreboard, ins ) ) {
                    change = 1;
                } else {
                    if( dst->n.class == 3 ) {
                        tmp = ins->zap->reg;
                        tmp|=dst->r.reg;
                        RegKill( scoreboard, tmp );
                    } else {
                        ScoreInfo( &info, dst );
                        ScoreKillInfo( scoreboard, dst, &info, 0 );
                        RegKill( scoreboard, ins->zap->reg );
                    }
                    if( had_condition == 0 ) {
                        ScZeroCheck( scoreboard, ins );
                    }
                }
            }
        } else {
            if( ins->head.opcode == 0 && dst != NULL ) {
                ScoreInfo( &info, dst );
                ScoreKillInfo( scoreboard, dst, &info, 0 );
            }
            RegKill( scoreboard, ins->zap->reg );
        }
        ins = next_ins;
    }
    return( change );
}



unsigned char ScConvert(instruction *ins);
unsigned char RemDeadCode(block *blk)
{
    unsigned char change;
    name *result;
    instruction *ins,*next;
    change=0;
    for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=next) {
        next=ins->head.next;
        result=ins->result;
        if (ins->head.opcode!=0x36 && ins->head.opcode!=0x29 &&
            !UnChangeable(ins) && !SideEffect(ins) && result!=NULL &&
            !ScConvert(ins) && result->n.class==3 &&
            !(ins->head.next->head.live.regs&result->r.reg)) {
            FreeIns(ins);
            change=1;
        }
    }
    return change;
}



unsigned char ScoreLA(score *sc,instruction *ins);
unsigned char RegsEqual(score *sc,int left,int right);
unsigned char ScoreEqual(score *sc,int index,score_info *info);
unsigned char StupidMove(score *sc,instruction *ins)
{
    score_info info;
    int dst_index;
    if (ins->head.opcode!=0x26 || ins->result->n.class!=3) return 0;
    dst_index=ins->result->r.reg_index;
    if (ins->operands[0]->n.class==3) {
        if (!RegsEqual(sc,ins->operands[0]->r.reg_index,dst_index)) return 0;
    } else {
        ScoreInfo(&info,ins->operands[0]);
        if (!ScoreEqual(sc,dst_index,&info)) return 0;
    }
    /* After a condition, remove the redundant assignment without changing CC. */
    DoNothing(ins);
    return 1;
}


extern score_info *ScZero;
extern score_reg **ScoreList;
unsigned char ScoreLookup(score *sc,score_info *info);

unsigned char HasZero( score *sc, name *n ) {
/**********************************************
    given a scoreboard "sc", determine if name "n" is equal to
    zero or has any portions which are equal to zero. This is recursive
    since a regisiter like EAX on the 386 has pieces AX, AH, and AL.
*/

    unsigned char bits;
    int         i;
    int         hi;
    int         lo;

    bits = 0;
    if( n->n.class == 0 ) {
        if( n->c.const_type == 0 ) {
            if( n->c.int_value == 0 ) {
                bits = LO_HALF | HI_HALF;
            } else if( n->n.size == 2 ) {
                if( ( n->c.int_value & 0x00ff ) == 0 ) {
                    bits |= LO_HALF;
                }
                if( ( n->c.int_value & 0xff00 ) == 0 ) {
                    bits |= HI_HALF;
                }
            } else if( n->n.size == 4 ) {
                if( ( n->c.int_value & 0x0000ffff ) == 0 ) {
                    bits |= LO_HALF;
                }
                if( ( n->c.int_value & 0xffff0000 ) == 0 ) {
                    bits |= HI_HALF;
                }
            }
        }
    } else if( n->n.class == 3 ) {
        i  = n->r.reg_index;
        if( ScoreLookup( &sc[ i ], ScZero ) ) {
            bits |= LO_HALF | HI_HALF;
        } else {
            hi = ScoreList[ i ]->high;
            lo = ScoreList[ i ]->low;
            if( hi != NO_INDEX && lo != NO_INDEX ) {
                if( _IsZero( HasZero( sc, (name *)ScoreList[ lo ]->reg_name ) ) ) {
                    bits |= LO_HALF;
                }
                if( _IsZero( HasZero( sc, (name *)ScoreList[ hi ]->reg_name ) ) ) {
                    bits |= HI_HALF;
                }
            }
        }
    }
    if( n->n.size == 1 ) {
        bits |= IS_BYTE;
    }
    return( bits );
}
