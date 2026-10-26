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


/* ChangeCurrentLocation CF14050..140a2, from released posndir.c.
 * The1994 segment length is the maximum of the old and new locations. */
#include "asmparse.h"
#include "direct.h"
#include "breakout.h"
#include <recon.h>

int ChangeCurrentLocation(unsigned char relative, long amount)
{
    if (CurrSeg == NULL) return ERROR;
    if (relative) amount += GetCurrAddr();
    FlushCurrSeg();
    CurrSeg->seg->e.seginfo->current_loc = amount;
    CurrSeg->seg->e.seginfo->start_loc = amount;
    if (CurrSeg->seg->e.seginfo->current_loc >=
        CurrSeg->seg->e.seginfo->segrec->d.segdef.seg_length)
        CurrSeg->seg->e.seginfo->segrec->d.segdef.seg_length =
            CurrSeg->seg->e.seginfo->current_loc;
    return NOT_ERROR;
}

/* CF140a4..14117; the numeric and symbol forms keep the1994 behavior. */
int OrgDirective( int i )
{
    asm_sym *sym;
    long value = 0;

    if( AsmBuffer[i+1]->token == T_NUM ) {
        return ChangeCurrentLocation( 0, AsmBuffer[i+1]->value );
    } else if( AsmBuffer[i+1]->token == T_ID ) {
        sym = AsmLookup( AsmBuffer[i+1]->string_ptr );
        if( AsmBuffer[i+2]->token == T_OP_SQ_BRACKET &&
            AsmBuffer[i+3]->token == T_NUM ) {
            value = AsmBuffer[i+3]->value;
        }
        return ChangeCurrentLocation( 0, sym->offset + value );
    }
    AsmError( 34 );
    return ERROR;
}

/* CF14220..142e3. GetCurrSegAlign's zero result passes the original
 * signed-negative test; preserve that test rather than the2002 <=0 change. */
int AlignDirective( unsigned short directive, int i )
{
    long align_val;
    int seg_align;
    switch( directive ) {
    case T_ALIGN:
        if( AsmBuffer[i+1]->token == T_NUM ) {
            int power;
            align_val = AsmBuffer[i+1]->value;
            for( power = 1; power < align_val; power <<= 1 );
            if( power != align_val ) {
                AsmError( 1153 );
                return ERROR;
            }
        } else if( Token_Count == i+1 ) {
            align_val = GetCurrSegAlign();
        } else {
            AsmError( 34 );
            return ERROR;
        }
        break;
    case T_EVEN:
        align_val = 2;
        break;
    }
    seg_align = GetCurrSegAlign();
    if( seg_align < 0 ) {
        AsmError( 1017 );
        return ERROR;
    }
    if( align_val > seg_align ) {
        AsmWarn( 1, 1256 );
        return ERROR;
    }
    seg_align = GetCurrAddr() % align_val;
    if( seg_align ) {
        align_val -= seg_align;
        fill_in_objfile_space( align_val );
    }
    return NOT_ERROR;
}
