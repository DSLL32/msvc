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

/* Source NOP patterns, checked against actual CF26734 and26738.
 * Index0 is the longest pattern; shorter alternatives follow. */
static unsigned char NopList16[] = {
    2, 0x89,0xc0, /* MOV AX,AX */
    0xfc         /* CLD */
};
static unsigned char NopList32[] = {
    6, 0x8d,0x80,0,0,0,0, /* LEA EAX,[EAX] */
    0x8d,0x40,0,0x8b,0xc9,0x8d,0x44,0x20,0,
    0x8d,0x40,0,0x8b,0xc0,0x90
};
static unsigned char *NopLists[] = { NopList16, NopList32 };
unsigned char is_code_seg(void);

/* CF14144..1421f. */
void fill_in_objfile_space(unsigned size)
{
    int i, nop_type;
    if (!is_code_seg()) {
        for (i = 0; i < size; ++i) AsmByte(0);
    } else {
        if (Use32 != 0 && Use32 != 1)
            InternalError("g:\\bld\\wasm\\c\\posndir.c",134);
        while (size > NopLists[Use32][0]) {
            for (i = 1; i <= NopLists[Use32][0]; ++i)
                AsmByte(NopLists[Use32][i]);
            size -= NopLists[Use32][0];
        }
        if (size == 0) return;
        i = 1;
        for (nop_type = NopLists[Use32][0]; nop_type > size; --nop_type)
            i += nop_type;
        for (; nop_type > 0; --nop_type, ++i)
            AsmByte(NopLists[Use32][i]);
    }
}
/* CF14118..14142: class-name index, not segment name. */
unsigned char is_code_seg(void)
{
    int code = FindClassLnameIdx("CODE");
    return CurrSeg->seg->e.seginfo->segrec->d.segdef.class_name_idx == code;
}
