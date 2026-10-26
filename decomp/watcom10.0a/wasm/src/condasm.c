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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Adapted from bld/wasm/c/condasm.c at6b9cb443; actual1994 CF17ad0..17bd8. */
#include <string.h>
#include <ctype.h>
#include "asmparse.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(strchr)
#endif
char CurState; /* CF26788; ACTIVE0, LOOKING_FOR_TRUE_COND1, DONE2. */
void prep_line_for_conditional_assembly(char *line)
{
    char *ptr, *end;
    int count;
    char delim, fix;
    if (Comment(0, 0)) {
        delim = (char)Comment(1, 0);
        if (strchr(line, delim) != NULL) Comment(3, 0);
        *line = '\0';
        return;
    }
    if (*line == '\0') return;
    ptr = line + strspn(line, " \t");
    for (end = ptr; isalnum((unsigned char)*end) || *end == '#'; ++end) ;
    fix = *end;
    *end = '\0';
    if (*ptr == '#') { *ptr = ' '; ++ptr; }
    count = get_instruction_position(ptr);
    *end = fix;
    if (count == -2) {
        if (CurState == 1 || CurState == 2) *line = '\0';
        return;
    }
    switch (AsmOpTable[count].token) {
    case T_IFDEF: case T_IFNDEF: case T_IFB: case T_IFNB:
    case T_IF: case T_IF1: case T_IF2: case T_IFE:
    case T_ELSEIF: case T_ELSE: case T_ENDIF:
    case T_IFDIF: case T_IFDIFI: case T_IFIDN: case T_IFIDNI:
        break;
    default:
        if (CurState == 1 || CurState == 2) *line = '\0';
    }
}

/* CF17fe0..1842f; byte state owners CF26789/2678a. */
/* CF17bd8..17c16, including the original repeated NUL stores. */
char check_defd(char *string)
{
    char *ptr, *end;
    struct asm_sym **sym_ptr;
    ptr = string + strspn(string, " \t");
    end = ptr + strcspn(ptr, " \t");
    *end = '\0';
    sym_ptr = AsmFind(ptr);
    if (*sym_ptr != NULL) {
        *end = '\0';
        return TRUE;
    }
    *end = '\0';
    return FALSE;
}
char check_blank(char *text) { WASM_PENDING(); return 0; }
char check_dif(unsigned char sensitive, char *first, char *second)
{ WASM_PENDING(); return 0; }
static signed char    NestLevel = 0;

int conditional_assembly_directive( int i )
/*****************************************/
{
    unsigned short         direct;
    static signed char    falseblocknestlevel = 0;

    direct = AsmBuffer[i]->value;

    switch( CurState ) {
    case 0:
        switch( direct ) {
        case T_IF:
        case T_IF1:
        case T_IF2:
        case T_IFE:
        case T_IFDEF:
        case T_IFNDEF:
        case T_IFB:
        case T_IFNB:
        case T_IFDIF:
        case T_IFDIFI:
        case T_IFIDN:
        case T_IFIDNI:
            NestLevel++;
            if( NestLevel > 20 ) {
                /*fixme */
                AsmError( 1081 );
                return( ERROR );
            }
        }
        break;
    case 1:
        if( direct == T_ELSE || direct == T_ELSEIF ) {
            if( falseblocknestlevel == 0 ) {
                break;
            }
        }
        /* fall through for other cases */
    case 2:
        switch( direct ) {
        case T_IF:
        case T_IF1:
        case T_IF2:
        case T_IFE:
        case T_IFDEF:
        case T_IFNDEF:
        case T_IFB:
        case T_IFNB:
        case T_IFDIF:
        case T_IFDIFI:
        case T_IFIDN:
        case T_IFIDNI:
            falseblocknestlevel++;
            return( NOT_ERROR );
            break;
        case T_ENDIF:
            if( falseblocknestlevel > 0 ) {
                falseblocknestlevel--;
                return( NOT_ERROR );
            }
            break;
        case T_ELSE:
        case T_ELSEIF:
            return( NOT_ERROR );
        default:
            InternalError("..\\c\\condasm.c",324);
            return( ERROR );
        }
    }

    /* expand any constants if necessary */
    switch( direct ) {
    case T_IF:
    case T_IFE:
    case T_ELSEIF:
    case T_IFDIF:
    case T_IFDIFI:
    case T_IFIDN:
    case T_IFIDNI:
        ExpandTheWorld( i+1, FALSE );
    }

    switch( direct ) {
    case T_IF1:
        CurState = Parse_Pass == 0 ? 0 : 1;
        break;
    case T_IF2:
        CurState = Parse_Pass == 0 ? 1 : 0;
        break;
    case T_IF:
        CurState = ( AsmBuffer[i+1]->token == T_NUM && AsmBuffer[i+1]->value )
                   ? 0 : 1;
        break;
    case T_IFE:
        CurState = ( AsmBuffer[i+1]->token == T_NUM && !AsmBuffer[i+1]->value )
                   ? 0 : 1;
        break;
    case T_IFDEF:
        CurState = check_defd( AsmBuffer[i+1]->string_ptr )  ? 0 : 1;
        break;
    case T_IFNDEF:
        CurState = !check_defd( AsmBuffer[i+1]->string_ptr )  ? 0 : 1;
        break;
    case T_IFB:
        CurState = check_blank( AsmBuffer[i+1]->string_ptr ) ? 0 : 1;
        break;
    case T_IFNB:
        CurState = !check_blank( AsmBuffer[i+1]->string_ptr ) ? 0 : 1;
        break;
    case T_IFDIF:
        CurState = check_dif( TRUE, AsmBuffer[i+1]->string_ptr, AsmBuffer[i+3]->string_ptr ) ? 0 : 1;
        break;
    case T_IFDIFI:
        CurState = check_dif( FALSE, AsmBuffer[i+1]->string_ptr, AsmBuffer[i+3]->string_ptr ) ? 0 : 1;
        break;
    case T_IFIDN:
        CurState = !check_dif( TRUE, AsmBuffer[i+1]->string_ptr, AsmBuffer[i+3]->string_ptr ) ? 0 : 1;
        break;
    case T_IFIDNI:
        CurState = !check_dif( FALSE, AsmBuffer[i+1]->string_ptr, AsmBuffer[i+3]->string_ptr ) ? 0 : 1;
        break;
    case T_ELSE:
    case T_ELSEIF:
        switch( CurState ) {
        case 0:
            CurState = 2;
            break;
        case 1:
            if( direct == T_ELSE ) {
                CurState = 0;
            } else {
                CurState = ( AsmBuffer[i+1]->token == T_NUM &&
                             AsmBuffer[i+1]->value )
                           ? 0 : 1;
            }
            break;
        }
        break;
    case T_ENDIF:
        NestLevel--;
        if( NestLevel < 0 ) {
            NestLevel = 0;
            AsmError( 1007 );
            return( ERROR );
        }
        CurState = 0;
    }
    return( NOT_ERROR );
}
