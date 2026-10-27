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
#include "pattern.h"
#include "zerobits.h"
#include "stubs.h"
#include <recon.h>

extern unsigned char HalfClass[11];
opcode_entry *FindGenEntry(instruction *ins,unsigned char *dummy);
unsigned char HasZero(score *sc,name *op);
unsigned char CheckIns(instruction **ins);
name *NonZeroPart(name *op,zero_bits bits,unsigned char type_class);
zero_bits AZeroHalf(zero_bits left,zero_bits right);
instruction *MakeClear(name *res,unsigned char type_class);

unsigned char ScoreZero( score *sc, instruction **pins ) {
/***********************************************************/

    instruction         *ins;
    unsigned char       change;
    unsigned char       class;
    unsigned char       hc;
    zero_bits           op1zpart;
    zero_bits           op2zpart;
    zero_bits           zeropart;
    zero_bits           nonzeropart;
    name                *op1;
    name                *op2;
    name                *res;
    instruction         *ins1;
    instruction         *ins2;
    name                *tmp1;
    name                *tmp2;
    name                *tmp3;
    name                *tmp4;


#define NonZeroOP2Part( x )  NonZeroPart( op2, x, hc )
#define NonZeroOP1Part( x )  NonZeroPart( op1, x, hc )
#define NonZeroRESPart( x )  NonZeroPart( res, x, hc )

    ins = *pins;
    if( ins->num_operands != 2 ) return( 0 );
    if( ins->result == NULL ) return( 0 );
    if( ( ins->head.opcode != 1 && ins->head.opcode != 9
       && ins->head.opcode != 10 && ins->head.opcode != 11
       && ins->head.opcode != 3 ) ) return( 0 );
    if( ins->ins_flags & 0x20 ) return( 0 );
    if( VolatileIns( ins ) ) return( 0 );
    op1 = ins->operands[ 0 ];
    op2 = ins->operands[ 1 ];
    res = ins->result;
    if( op1->n.size != op2->n.size ) return( 0 );
    if( res->n.size != op2->n.size ) return( 0 );
    class = ins->type_class;
    op1zpart = HasZero( sc, op1 );
    op2zpart = HasZero( sc, op2 );
    hc = HalfClass[  class      ];
    ins1 = NULL;
    ins2 = NULL;
    change = 0;
    switch( ins->head.opcode ) {
    case 1:
        if( _IsZero( op1zpart ) && _IsZero( op2zpart ) ) {
            ins1 = MakeClear( res, class );
        } else if( _IsZero( op1zpart ) ) {
            ins1 = MakeMove( op2, res, class );
        } else if( _IsZero( op2zpart ) ) {
            ins1 = MakeMove( op1, res, class );
        } else if( _OpposZero( op1zpart, op2zpart ) ) {
            tmp1 = NonZeroOP1Part( op1zpart );
            tmp2 = NonZeroRESPart( op1zpart );
            tmp3 = NonZeroOP2Part( op2zpart );
            tmp4 = NonZeroRESPart( op2zpart );
            if( tmp1==0 || tmp2==0 || tmp3==0 || tmp4==0 ) return( 0 );
            ins1 = MakeMove( tmp1, tmp2, hc );
            ins2 = MakeMove( tmp3, tmp4, hc );
        } else {
            zeropart = _ZeroHalf( op1zpart, op2zpart );
            if( _LoZero( zeropart ) ) {
                nonzeropart = _OtherHalf( zeropart );
                tmp1 = NonZeroRESPart( nonzeropart );
                tmp2 = NonZeroOP1Part( zeropart );
                tmp3 = NonZeroOP2Part( zeropart );
                tmp4 = NonZeroRESPart( zeropart );
                if( tmp1==0 || tmp2==0 || tmp3==0 || tmp4==0 ) return( 0 );
                ins2 = MakeClear( tmp1, hc );
                ins1 = MakeBinary( 1, tmp2, tmp3, tmp4, hc );
            }
        }
        break;
    case 3:
        if( _IsZero( op1zpart ) && _IsZero( op2zpart ) ) {
            ins1 = MakeClear( res, class );
        } else if( _IsZero( op2zpart ) ) {
            ins1 = MakeMove( op1, res, class );
        } else if( _IsZero( op1zpart ) ) {
            ins1 = MakeUnary( 17, op2, res, class );
        }
        break;
    case 9:
        if( _IsZero( op1zpart ) || _IsZero( op2zpart ) || _OpposZero( op1zpart,op2zpart ) ) {
            ins1 = MakeClear( res, class );
        } else {
            zeropart = AZeroHalf( op1zpart, op2zpart );
            if( zeropart ) {
                nonzeropart = _OtherHalf( zeropart );
                tmp1 = NonZeroRESPart( nonzeropart );
                tmp2 = NonZeroOP1Part( zeropart );
                tmp3 = NonZeroOP2Part( zeropart );
                tmp4 = NonZeroRESPart( zeropart );
                if( tmp1==0 || tmp2==0 || tmp3==0 || tmp4==0 ) return( 0 );
                ins1 = MakeClear( tmp1, hc );
                ins2 = MakeBinary( 9, tmp2, tmp3, tmp4, hc );
            }
        }
        break;
    case 10:
    case 11:
        if( _IsZero( op1zpart ) && _IsZero( op2zpart ) ) {
            if( op1 == res || op2 == res ) { /* result already has zero*/
                *pins = ins->head.next;
                FreeIns( ins );
                change = 1;
            } else {
                ins1 = MakeClear( res, class );
            }
        } else if( _IsZero( op1zpart ) ) {
            ins1 = MakeMove( op2, res, class );
        } else if( _IsZero( op2zpart ) ) {
            ins1 = MakeMove( op1, res, class );
        } else if( _OpposZero( op1zpart, op2zpart ) ) {
            tmp1 = NonZeroOP1Part( op1zpart );
            tmp2 = NonZeroRESPart( op1zpart );
            tmp3 = NonZeroOP2Part( op2zpart );
            tmp4 = NonZeroRESPart( op2zpart );
            if( tmp1==0 || tmp2==0 || tmp3==0 || tmp4==0 ) return( 0 );
            ins1 = MakeMove( tmp1, tmp2, hc );
            ins2 = MakeMove( tmp3, tmp4, hc );
        } else if( _MatchZero( op1zpart, op2zpart ) ) {
            zeropart = _ZeroHalf( op1zpart, op2zpart );
            nonzeropart = _OtherHalf( zeropart );
            tmp1 = NonZeroRESPart( nonzeropart );
            tmp2 = NonZeroOP1Part( zeropart );
            tmp3 = NonZeroOP2Part( zeropart );
            tmp4 = NonZeroRESPart( zeropart );
            if( tmp1==0 || tmp2==0 || tmp3==0 || tmp4==0 ) return( 0 );
            ins2 = MakeClear( tmp1, hc );
            ins1 = MakeBinary( ins->head.opcode, tmp2, tmp3, tmp4, hc );
        }
        break;
    }
    if( CheckIns( &ins1 ) == 0 && ins2 != NULL ) {
        ins2->head.next = ins2;
        ins2->head.prev = ins2;
        FreeIns( ins2 );
        ins2 = NULL;
    }
    if( CheckIns( &ins2 ) == 0 && ins1 != NULL ) {
        ins1->head.next = ins1;
        ins1->head.prev = ins1;
        FreeIns( ins1 );
        ins1 = NULL;
    }
    if( ins1 == NULL && ins2 == NULL ) return( change );
    if( ins2 != NULL ) {
        SuffixIns( ins, ins2 );
    }
    if( ins1 != NULL ) {
        SuffixIns( ins, ins1 );
    }
    FreeIns( ins );
    *pins = ins1;
    return( 1 );
}

unsigned char HasZero(score *sc,name *op);
unsigned char CheckIns( instruction **pins ) {
/**********************************************/

    unsigned char ok;
    opcode_entry        *try;
    instruction         *ins;
    unsigned char dummy;
    int                 i;

    ins = *pins;
    ok = 1;
    if( ins != NULL ) {
        i = ins->num_operands;
        while( --i >= 0 ) {
            if( ins->operands[i]->n.class == 3 &&
                ins->operands[i]->r.reg_index == -1 ) {
                ok = 0;
            }
        }
        if( ok && ins->result != NULL ) {
            if( ins->result->n.class == 3 &&
                ins->result->r.reg_index == -1 ) {
                ok = 0;
            }
        }
        if( ok ) {
            try = FindGenEntry( ins, &dummy );
            if( try == NULL ) {
                ok = 0;
            } else if( try->generate >= 0x67 ) {
                ok = 0;
            } else {
                ins->table = try;
                ins->u.gen_table = try;
            }
        }
    }
    if( !ok ) {
        ins->head.next = ins;
        ins->head.prev = ins;
        FreeIns( ins );
        *pins = NULL;
    }
    return( ok );
}



name *NonZeroPart(name *op,zero_bits bits,unsigned char type_class) { STUB_TRAP();return NULL; }
zero_bits AZeroHalf(zero_bits left,zero_bits right) { STUB_TRAP();return 0; }
instruction *MakeClear(name *res,unsigned char type_class) { STUB_TRAP();return NULL; }
