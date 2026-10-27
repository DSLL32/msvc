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
* Description:  Determine whether instructions redefine variable values.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned char ReDefinedBy(instruction *ins,name *op)
{
    unsigned char zaps;
    if (op->n.class==3 && (ins->zap->reg&op->r.reg)) return 1;
    if (NameIsConstant(op)) return 0;
    /* The 386 target has no later Alpha ZappedBySTQ_U instruction hook. */
    zaps=ZapsTheOp(ins->result,op);
    if (zaps) return zaps;
    if (ins->head.opcode!=0x36 && ins->head.opcode!=0x29) return 0;
    return VisibleToCall(ins,op,1);
}
unsigned char NameIsConstant(name *op)
{
    if (op->n.class==2 && (op->v.usage&0x100)) return 1; /* VAR_CONSTANT */
    if (op->n.class!=1) return 0;
    if (op->v.usage&0x20) return 0;
    if (op->v.usage&0x100) return 1;
    if (op->m.memory_type==CG_FE && (FEAttr(op->v.symbol)&0x10)) return 1;
    return AskNameROM(op->v.symbol,op->m.memory_type);
}
unsigned char ZapsTheOp(name *result,name *op)
{
    if (result==NULL) return 0;
    switch (op->n.class) {
    case 1:
        if (op->v.usage&0x20) return 1;
        return ZapsMemory(result,op,0);
    case 4:
        if (op->i.index_flags&2) return 1;
        return ZapsIndexed(result,op);
    case 2:
        if (op->v.usage&0x20) return 1;
        return ZapsTemp(result,op,0);
    case 3:
        if (result->n.class==3 && (result->r.reg&op->r.reg)) return 1;
        return 0;
    case 0: return 0;
    default: Zoiks(23); return 0;
    }
}
unsigned char ZapsMemory(name *result,name *op,unsigned char for_index)
{
    /* The target folds the base recursion into its class-dispatch loop. */
    for (;;) {
        switch (result->n.class) {
        case 2: return 0;
        case 1:
            if (result->v.symbol!=op->v.symbol) return 0;
            if (for_index) return 1;
            return TempsOverlap(result,op);
        case 4:
            if (result->i.base==NULL) {
                if (Model&0x20000000U) return 0; /* FORTRAN_ALIASING */
                if (op->v.usage&8) return 1; /* USE_ADDRESS */
                if (Model&0x04000000U) return 0; /* RELAX_ALIAS */
                return 1;
            }
            if (result->i.base->n.class==2) return (Model&0x04000000U)==0;
            result=result->i.base;
            for_index=1;
            break;
        case 3: return 0;
        default: Zoiks(23);return 0;
        }
    }
}
unsigned char ZapsTemp(name *result,name *op,unsigned char for_index)
{
    switch (result->n.class) {
    case 2:
        if (DeAlias(result)!=DeAlias(op)) return 0;
        if (for_index) return 1;
        return TempsOverlap(op,result);
    case 1: return 0;
    case 4:
        if (result->i.base!=NULL && result->i.base->n.class==2) {
            if (DeAlias(result->i.base)==DeAlias(op)) return 1;
        }
        if (!(op->v.usage&8)) return 0;
        if (result->i.base==NULL) return 1;
        return 0;
    case 3: return (result->r.reg&DisplayReg())!=0;
    default: Zoiks(23); return 0;
    }
}
unsigned char TempsOverlap(name *left,name *right)
{
    long left_end,right_end;
    left_end=(long)((unsigned long)left->v.offset+left->n.size-1);
    right_end=(long)((unsigned long)right->v.offset+right->n.size-1);
    return right->v.offset<=left_end && right_end>=left->v.offset;
}
unsigned char    ZapsIndexed( name *result, name *op ) {
/******************************************************
    Could redefining "result" redefine 4 name "op"?
*/

    switch( result->n.class ) {
    case 2:
        result = DeAlias( result );
        if( op->i.base == NULL ) { /* free index */
            if( result->v.usage & 8 ) return( 1 );
        } else if( op->i.base->n.class == 2 ) {
            if( result == DeAlias( op->i.base ) ) return( 1 );
        }
        if( op->i.index->n.class != 2 ) return( 0 );
        if( result == DeAlias( op->i.index ) ) return( 1 );
        return( 0 );
    case 1:
        if( op->i.base == NULL ) {
            if( (Model&0x20000000U) ) return( 0 );
            if( op->v.usage & 8 ) return( 1 );
            if( (Model&0x04000000U) ) return( 0 );
            return( 1 );
        } else if( op->i.base->n.class == 2 ) {
            if( (Model&0x04000000U) ) return( 0 );
            return( 1 );
        } else {
            return( ZapsMemory( result, op->i.base, 1 ) );
        }
    case 4:
        if( result->i.base == op->i.base &&
            result->i.index == op->i.index &&
            result->i.index_flags == op->i.index_flags &&
            result->i.scale == op->i.scale &&
            ( result->i.constant >= op->i.constant + op->n.size ||
              op->i.constant >= result->i.constant + result->n.size ) ) {
            return( 2 ); // no overlap if index is the same
        }
        if( result->i.base == NULL ) return( 1 );
        if( op->i.base == NULL ) return( 1 );
        if( result->i.base->n.class != op->i.base->n.class ) return( 0 );
        if( result->i.base->n.class == 2 ) {
            return( ZapsTemp( result->i.base, op->i.base, 1 ) );
        }
        return( ZapsMemory( result->i.base, op->i.base, 1 ) );
    case 3:
        if( op->i.base != NULL && ZapsTheOp( result, op->i.base ) ) {
            return( 1 );
        }
        return( ZapsTheOp( result, op->i.index ) );
    default:
        Zoiks(23);
        return( 0 );
    }
}



unsigned char VisibleToCall(instruction *ins,name *op,unsigned char redef)
{
    switch (op->n.class) {
    case 1:
        if (redef && (ins->flags.call_flags&2)) return 0;
        if (Model&0x20000000U) {
            switch (op->m.memory_type) {
            case CG_FE:return (FEAttr(op->v.symbol)&0x40)!=0;
            case CG_BACK:case CG_TBL:case CG_VTB:return 0;
            default:return 1;
            }
        }
        return 1;
    case 4:
        if (op->i.base==NULL) return 1;
        return ReDefinedBy(ins,op->i.base);
    case 2:return (op->v.usage&8)!=0;
    default:return 0;
    }
}
