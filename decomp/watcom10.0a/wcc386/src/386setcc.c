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
* Description:  Use i386 set-on-condition instructions for conditional flow.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned char FindFlowOut(block *blk);
unsigned char SetOnCondition(void)
{
    block *blk;
    unsigned char change;
    if ((CGProcessorVersion&0xf)<3) return 0;
    blk=HeadBlock;
    change=0;
    while (blk!=NULL) {
        if (blk->class&4) change|=FindFlowOut(blk);
        blk=blk->next_block;
    }
    return change;
}
instruction *SetToConst(block *blk,long *pcons);
unsigned char FindFlowOut(block *blk)
{
    long           false_cons;
    long           true_cons;
    instruction         *ins;
    instruction         *ins0;
    instruction         *ins1;
    block               *true;
    block               *false;
    block               *join;
    block_edge          *new_edge;
    unsigned char       reverse;
    name                *u1temp;
    name                *temp;
    name                *result;
    unsigned char      class;

    ins = blk->ins.hd.prev;
    while( !(ins->head.opcode>=0x2e && ins->head.opcode<=0x35) ) {
        ins = ins->head.prev;
    }
    if( TypeClassSize[ ins->type_class ] > 4 ) return( 0 );
    true = blk->edge[ (ins->flags.byte&0xf) ].destination;
    if( true->inputs != 1 ) return( 0 );
    if( true->targets != 1 ) return( 0 );

    false = blk->edge[ (ins->flags.byte>>4) ].destination;
    if( false->inputs != 1 ) return( 0 );
    if( false->targets != 1 ) return( 0 );

    join = false->edge[ 0 ].destination;
    if( join != true->edge[ 0 ].destination ) return( 0 );
    if( join->inputs != 2 ) return( 0 );
    if( join->class & 0x2000 ) return( 0 );

    ins0 = SetToConst( false, &false_cons );
    if( ins0 == NULL ) return( 0 );
    ins1 = SetToConst( true, &true_cons );
    if( ins1 == NULL ) return( 0 );
    if( (long)((unsigned long)true_cons-(unsigned long)false_cons) == -1 ) {
        true_cons = false_cons;
        false_cons = (long)((unsigned long)true_cons-1);
        reverse = 1;
    } else {
        if( (long)((unsigned long)true_cons-(unsigned long)false_cons) != 1 ) return( 0 );
        reverse = 0;
    }
    result = ins0->result;
    if( result != ins1->result ) return( 0 );
    class = ins0->type_class;
    if( class != ins1->type_class ) return( 0 );

    if( reverse ) FlipCond( ins );

    u1temp = AllocTemp( U1 );
    temp = AllocTemp( class );

    ins->result = u1temp;
    ins1 = MakeConvert( u1temp, temp, class, U1 );
    SuffixIns( ins, ins1 );
    ins = ins1;

    if( false_cons != 0 ) {
        ins1 = MakeBinary( 1, temp, AllocS32Const( false_cons ),
                           result, class );
    } else {
        ins1 = MakeMove( temp, result, class );
    }
    SuffixIns( ins, ins1 );

    RemoveInputEdge( join->input_edges );
    RemoveInputEdge( join->input_edges );
    RemoveInputEdge( true->input_edges );
    RemoveInputEdge( false->input_edges );
    true->targets = 0;
    false->targets = 0;
    /* The old body leaves these now-detached blocks for BlockTrim. */

    blk->targets = 1;
    new_edge = &blk->edge[ 0 ];
    new_edge->destination = join;
    new_edge->next_source = NULL;
    join->input_edges = new_edge;
    join->inputs = 1;
    blk->class &= ~6U;
    blk->class |= 2;
    return( 1 );
}
instruction *SetToConst(block *blk,long *pcons)
{
    instruction *ins,*next;
    name *op;
    ins=blk->ins.hd.next;
    while (ins->head.opcode==0) ins=ins->head.next;
    if (ins->head.opcode!=0x26) return NULL;
    if (ins->type_class==FS || ins->type_class==FD) return NULL;
    next=ins->head.next;
    while (next->head.opcode==0) next=next->head.next;
    if (next->head.opcode!=0x4b) return NULL;
    op=ins->operands[0];
    if (op->n.class!=0 || op->c.const_type!=0) return NULL;
    *pcons=op->c.int_value;
    return ins;
}

/* Result register restrictions for condition-code materialization. */
unsigned char SpecialPossible(instruction *ins)
{
    if (ins->result==NULL) return 0;
    if (ins->head.opcode>=0x2e && ins->head.opcode<=0x35) return 4; /* RL_BYTE */
    return 0;
}
