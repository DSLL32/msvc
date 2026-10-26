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


/* data_init CFb49c..b7d6, adapted from the initial released asmdata.c.
 * The1994 parser lacks the later empty-initializer check. */
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include "asmparse.h"
#include "breakout.h"
#include "direct.h"
#include <recon.h>

char first;        /* CF27240: first element in the current declaration. */
char struct_field; /* CF27241: declaration contributes to a structure. */

/* CFa794..b31a, adapted from released asmdata.c with actual1994 changes:
 * eight-bit cursor, no extra_value reload for large numeric initializers,
 * and unchanged byte-width accounting for strings in structures. */
int array_element( asm_sym *sym, char start_pos, char no_of_bytes )
{
    unsigned char cur_pos = (unsigned char)start_pos;
    unsigned char count;
    long value;
    char *char_ptr;
    char negative = 0;
        asm_sym *the_struct;
        the_struct = (asm_sym*)Definition.curr_struct;
    while( AsmBuffer[cur_pos]->token != T_FINAL ) {
            int tmp;
            tmp = cur_pos;
            if( check_override( &tmp ) == -1 ) {
                return( -1 );
            }
            cur_pos = tmp;
        if( cur_pos == Token_Count - 1 &&
            AsmBuffer[cur_pos]->token == ')' ) break;
        switch( AsmBuffer[cur_pos]->token ) {
        case T_QUESTION_MARK:
            if( cur_pos != start_pos ) {
                if( AsmBuffer[cur_pos - 1]->token != T_COMMA ) {
                    AsmError( 33 );
                    return( -1 );
                }
            }
                if( !struct_field ) {
                    ChangeCurrentLocation( 1, no_of_bytes );
                } else {
                    Definition.curr_struct->e.structinfo->size += no_of_bytes;
                    the_struct->total_size+=no_of_bytes;
                    the_struct->total_length++;
                    if( first ) {
                        the_struct->first_size+=no_of_bytes;
                        the_struct->first_length++;
                    }
                }
                if( sym && Parse_Pass == PASS_1 ) {
                    sym->total_size+=no_of_bytes;
                    if( first ) sym->first_size+=no_of_bytes;
                }
            break;
        case '-':
            switch( AsmBuffer[cur_pos+1]->token ) {
            case T_NUM:
                AsmBuffer[cur_pos+1]->value *= -1;
                AsmBuffer[cur_pos]->token = '+';
                break;
            case T_FLOAT:
                negative = 1;
                break;
            default:
                AsmError( 34 );
                return( -1 );
            }
            break;
        case '+':
        case T_DOT:
            break;
        case T_NUM:
        case T_FLOAT:
            if( AsmBuffer[cur_pos+1]->token == T_RES_ID &&
                AsmBuffer[cur_pos+1]->value == T_DUP ) {
                cur_pos = dup_array( sym, cur_pos, no_of_bytes );
                if( cur_pos == (unsigned char)-1 ) return( -1 );
                break;
            }
            if( cur_pos != start_pos ) {
                switch( AsmBuffer[cur_pos - 1]->token ) {
                case T_COMMA:
                    break;
                case '+':
                case '-':
                    if( AsmBuffer[cur_pos - 2]->token == T_COMMA ) break;
                    if( cur_pos - 1 == start_pos ) break;
                default:
                    AsmError( 33 );
                    return( -1 );
                }
            }
            if( AsmBuffer[cur_pos]->token == T_FLOAT ) {
                output_float( cur_pos, no_of_bytes, negative );
                negative = 0;
                break;
            }
            value = AsmBuffer[cur_pos]->value;
            count = 0;
            if( sym && Parse_Pass == PASS_1 ) {
                sym->total_length++;
                sym->total_size += no_of_bytes;
                if( first ) {
                    sym->first_length++;
                    sym->first_size += no_of_bytes;
                }
            }
            if( !struct_field ) {
                while( count < no_of_bytes ) {
                    AsmDataByte( value );
                    value >>= 8;
                    count++;
                }
            } else {
                if( the_struct == NULL ) break;
                Definition.curr_struct->e.structinfo->size += no_of_bytes;
                the_struct->total_size+=no_of_bytes;
                the_struct->total_length++;
                if( first ) {
                    the_struct->first_size+=no_of_bytes;
                    the_struct->first_length++;
                }
            }
            break;
        case T_COMMA:
            first = 0;
            if( cur_pos != start_pos ) {
                if( AsmBuffer[cur_pos - 1]->token == T_COMMA ) {
                    AsmError( 34 );
                    return( -1 );
                }
            } else {
                AsmError( 34 );
                return( -1 );
            }
            if( cur_pos == ( Token_Count - 1 ) ) {
                More_Array_Element = 1;
                Last_Element_Size = no_of_bytes;
            }
            break;
        case T_STRING:
            if( no_of_bytes != 1 ) {
                if( AsmBuffer[cur_pos]->value > no_of_bytes ) {
                    AsmError( 70 );
                    return( -1 );
                }
            }
            count = 0;
            char_ptr = AsmBuffer[cur_pos]->string_ptr;
            little_endian( char_ptr, no_of_bytes );

            if( sym && Parse_Pass == PASS_1 ) {
                sym->total_length++;
                sym->total_size += no_of_bytes;
                if( first ) {
                    sym->first_length++;
                    sym->first_size += no_of_bytes;
                }
            }
            if( !struct_field ) {
                while( count < AsmBuffer[cur_pos]->value ) {
                    AsmDataByte( *char_ptr );
                    char_ptr++;
                    count++;
                }
            } else {
                if( the_struct == NULL ) break;
                Definition.curr_struct->e.structinfo->size += no_of_bytes;
                the_struct->total_size+=no_of_bytes;
                the_struct->total_length++;
                if( first ) {
                    the_struct->first_size+=no_of_bytes;
                    the_struct->first_length++;
                }
            }
            break;
        case T_ID:
        {
            int i;
            int temp;
            asm_sym *init_sym;
            char *ptr;
            long data = 0;
            struct asmfixup *fixup;
            i = ++cur_pos;
            if( check_override( &i ) == -1 ) {
                return( -1 );
            }
            i--;
            cur_pos = i;
            switch( ExpandSymbol( i, 0 ) ) {
            case -1:
                return( -1 );
            case 2:
                continue;
            }
            init_sym = AsmLookup( AsmBuffer[cur_pos]->string_ptr );
            if( init_sym == NULL ) return( -1 );
            switch( init_sym->state ) {
            case SYM_STRUCT_FIELD:
                AsmBuffer[cur_pos]->token = T_NUM;
                AsmBuffer[cur_pos]->value = init_sym->offset;
                continue;
            case SYM_GRP:
            case SYM_SEG:
                temp = FIX_SEG;
                break;
            default:
            switch( no_of_bytes ) {
            case 1:
                AsmError( 81 );
                return( -1 );
            case 2:
                temp = FIX_OFF16;
                break;
            case 4:
                if( Code->use32 ) {
                    temp = FIX_OFF32;
                } else {
                    temp = FIX_PTR16;
                }
                break;
            case 6:
                temp = FIX_PTR32;
                Code->info.opnd_type[0] = OP_J48;
                break;
            default:
                AsmError( 137 );
                return( -1 );
            }
            }
            fixup = AddFixup( init_sym, temp );
            InsFixups[0] = fixup;
            data += fixup->offset;
            for( cur_pos++;
                AsmBuffer[cur_pos]->token != T_FINAL && AsmBuffer[cur_pos]->token != T_COMMA;
                cur_pos++ ) {
                switch( AsmBuffer[cur_pos]->token ) {
                case '+':
                case T_DOT:
                case '[':
                    break;
                case T_ID:
                    init_sym = AsmLookup( AsmBuffer[cur_pos]->string_ptr );
                    data += init_sym->offset;
                    break;
                case '-':
                    if( AsmBuffer[cur_pos+1]->token != T_NUM ) {
                        AsmError( 34 );
                        return( -1 );
                    }
                    AsmBuffer[cur_pos+1]->value *=-1;
                    break;
                case T_NUM:
                    data += AsmBuffer[cur_pos]->value;
                }
            }
            cur_pos--;
                if( store_fixup( 0 ) == -1 ) return( -1 );
            ptr = (char *)&data;
            if( sym && Parse_Pass == PASS_1 ) {
                sym->total_length++;
                sym->total_size += no_of_bytes;
                if( first ) {
                    sym->first_length++;
                    sym->first_size += no_of_bytes;
                }
            }
            if( !struct_field ) {
                for( i = 0; i < (no_of_bytes < 4 ? no_of_bytes : 4); i++ ) {
                    AsmDataByte( *ptr );
                    ptr++;
                }
                for( ; i < no_of_bytes; i++ ) {
                    AsmDataByte( 0 );
                }
            } else {
                if( the_struct == NULL ) break;
                Definition.curr_struct->e.structinfo->size += no_of_bytes;
                the_struct->total_size+=no_of_bytes;
                the_struct->total_length++;
                if( first ) {
                    the_struct->first_size+=no_of_bytes;
                    the_struct->first_length++;
                }
            }
            break;
        }
        case T_UNARY_OPERATOR:
        {
            int i;
            int temp;
            int seg_off_operator_loc = 0;
            asm_sym *init_sym;
            char *ptr;
            long data = 0;
            struct asmfixup *fixup;
            if( AsmBuffer[cur_pos]->value == T_OFFSET ||
                AsmBuffer[cur_pos]->value == T_SEG2 ) {
                if( no_of_bytes < 2 ) {
                    AsmError( 81 );
                    return( -1 );
                }
            }
            seg_off_operator_loc = cur_pos;
            i = ++cur_pos;
            if( check_override( &i ) == -1 ) {
                return( -1 );
            }
            i--;
            cur_pos = i;
            if( cur_pos + 1 < Token_Count ) {
                if( AsmBuffer[++cur_pos]->token == T_ID ) {
                    init_sym = AsmLookup( AsmBuffer[cur_pos]->string_ptr );
                    if( init_sym == NULL ) return( -1 );
                    if( AsmBuffer[seg_off_operator_loc]->value == T_OFFSET ) {
                        if( init_sym->state == SYM_STACK ) {
                            AsmError( 82 );
                            return( -1 );
                        } else if( init_sym->state == SYM_GRP ) {
                            AsmError( 145 );
                            return( -1 );
                        }
                        switch( no_of_bytes ) {
                        case 1:
                            AsmError( 81 );
                            return( -1 );
                        case 2:
                            temp = FIX_OFF16;
                            break;
                        case 4:
                            temp = FIX_OFF32;
                            break;
                        default:
                            AsmError( 137 );
                            return( -1 );
                        }
                    } else if( AsmBuffer[seg_off_operator_loc]->value == T_SEG2 ) {
                        if( init_sym->state == SYM_STACK ) {
                            AsmError( 83 );
                        }
                        temp = FIX_SEG;
                    }
                    switch( AsmBuffer[seg_off_operator_loc]->value ) {
                    case T_OFFSET:
                            if( init_sym->state == SYM_STRUCT_FIELD ) {
                                data = init_sym->offset;
                                break;
                            }
                    case T_SEG2:
                        fixup = AddFixup( init_sym, temp );
                        InsFixups[0] = fixup;
                        if( AsmBuffer[seg_off_operator_loc]->value == T_OFFSET ) {
                            data += fixup->offset;
                        }
                            if( store_fixup( 0 ) == -1 ) return( -1 );
                        break;
                    case T_LENGTH:
                        data = sym->first_length;
                        break;
                    case T_LENGTHOF:
                        data = sym->total_length;
                        break;
                    case T_SIZE:
                        data = sym->first_size;
                        break;
                    case T_SIZEOF:
                        data = sym->total_size;
                        break;
                    default:
                        AsmError( 51 );
                        return( -1 );
                    }
                    for( cur_pos++; AsmBuffer[cur_pos]->token != T_FINAL; cur_pos++ ) {
                        switch( AsmBuffer[cur_pos]->token ) {
                        case '+':
                        case T_DOT:
                        case '[':
                            break;
                        case T_ID:
                            init_sym = AsmLookup( AsmBuffer[cur_pos]->string_ptr );
                            data += init_sym->offset;
                            break;
                        case '-':
                            if( AsmBuffer[cur_pos+1]->token != T_NUM ) {
                                AsmError( 34 );
                                return( -1 );
                            }
                            AsmBuffer[cur_pos+1]->value *=-1;
                            break;
                        case T_NUM:
                            data += AsmBuffer[cur_pos]->value;
                        }
                        if( AsmBuffer[cur_pos]->token == T_COMMA ) break;
                    }
                    cur_pos--;
                    ptr = (char *)&data;
                    if( sym && Parse_Pass == PASS_1 ) {
                        sym->total_length++;
                        sym->total_size += no_of_bytes;
                        if( first ) {
                            sym->first_length++;
                            sym->first_size += no_of_bytes;
                        }
                    }
                    if( !struct_field ) {
                        for( i = 0; i < no_of_bytes; i++ ) {
                            AsmDataByte( *ptr );
                            ptr++;
                        }
                    } else {
                        if( the_struct == NULL ) break;
                        Definition.curr_struct->e.structinfo->size += no_of_bytes;
                        the_struct->total_size+=no_of_bytes;
                        the_struct->total_length++;
                        if( first ) {
                            the_struct->first_size+=no_of_bytes;
                            the_struct->first_length++;
                        }
                    }
                } else {
                    AsmError( 51 );
                    return( -1 );
                }
            }
            break;
        }
        case ')':
            return( cur_pos );
        default:
            AsmError( 34 );
            return( -1 );
        }
        cur_pos++;
    }
    return( cur_pos );
}

/* CFb31c..b49c; repeated initializers reset the original first-element
 * owner on every repetition. The zero-repeat returned_pos defect remains
 * in the original source; this case does not exercise that path. */
int dup_array(asm_sym *sym, char start_pos, char no_of_bytes)
{
    int cur_pos = (unsigned char)start_pos;
    int returned_pos, count;
    char was_first;
    ExpandTheWorld((unsigned char)start_pos, 0);
    while (cur_pos + 2 < Token_Count) {
        if (AsmBuffer[cur_pos + 1]->token == T_RES_ID &&
            AsmBuffer[cur_pos + 1]->value == T_DUP) {
            if (AsmBuffer[cur_pos]->token != T_NUM) { AsmError(51); return -1; }
            count = AsmBuffer[cur_pos]->value;
            cur_pos += 2;
            if (AsmBuffer[cur_pos]->token != '(') { AsmError(51); return -1; }
            ++cur_pos;
            was_first = first;
            while (count > 0) {
                first = was_first;
                returned_pos = array_element(sym, cur_pos, no_of_bytes);
                if (returned_pos == -1) return -1;
                --count;
            }
            if (AsmBuffer[returned_pos]->token != ')') { AsmError(105); return -1; }
        } else {
            returned_pos = array_element(sym, cur_pos, no_of_bytes);
            if (returned_pos == -1) return -1;
            if (AsmBuffer[returned_pos]->token != ')') return returned_pos;
        }
        if (AsmBuffer[returned_pos + 1]->token != T_COMMA) return returned_pos;
        cur_pos = returned_pos + 2;
        first = 0;
    }
    return array_element(sym, cur_pos, no_of_bytes);
}

int data_init(int sym_loc, int initializer_loc)
{
    char no_of_bytes, label_dir = 0;
    int mem_type;
    unsigned old_offset;
    asm_sym *sym = NULL;
    struct_field = 0;
    first = 1;
    if (sym_loc >= 0) {
        sym = AsmLookup(AsmBuffer[sym_loc]->string_ptr);
        if (sym == NULL) return -1;
    }
    switch (AsmBuffer[initializer_loc]->value) {
    case T_SBYTE: mem_type = T_SBYTE; no_of_bytes = 1; break;
    case T_SWORD: mem_type = T_SWORD; no_of_bytes = 2; break;
    case T_SDWORD: mem_type = T_SDWORD; no_of_bytes = 4; break;
    case T_DQ: case T_QWORD: mem_type = T_QWORD; no_of_bytes = 8; break;
    case T_DT: case T_TBYTE: mem_type = T_TBYTE; no_of_bytes = 10; break;
    case T_STRUC: case T_STRUCT:
        mem_type = T_STRUCT;
        no_of_bytes = GetStructSize(initializer_loc);
        if (Definition.struct_depth == 0) InitializeStructure(sym, initializer_loc);
        break;
    case T_DB: case T_BYTE: mem_type = T_BYTE; no_of_bytes = 1; break;
    case T_DW: case T_WORD: mem_type = T_WORD; no_of_bytes = 2; break;
    case T_DD: case T_DWORD: mem_type = T_DWORD; no_of_bytes = 4; break;
    case T_DF: case T_FWORD: case T_DP: case T_PWORD:
        mem_type = T_FWORD; no_of_bytes = 6; break;
    default:
        AsmError(35);
        return -1;
    }
    if (sym_loc >= 0 && AsmBuffer[sym_loc]->value == T_LABEL) {
        label_dir = 1;
        --sym_loc;
    }
    if (sym_loc < 0 && Definition.struct_depth != 0) {
        if (Parse_Pass == PASS_1) {
            AddFieldToStruct(initializer_loc);
            struct_field = 1;
        } else return 1;
    }
    if (More_Array_Element == 1) More_Array_Element = 0;
    else if (sym_loc >= 0) {
        if (Definition.struct_depth != 0) {
            if (Parse_Pass == PASS_1) {
                sym->offset = AddFieldToStruct(initializer_loc);
                struct_field = 1;
                sym->state = SYM_STRUCT_FIELD;
                sym->mem_type = mem_type;
                if (dup_array(sym, initializer_loc + 1, no_of_bytes) == -1) return -1;
            }
            return 1;
        }
        if (Parse_Pass == PASS_1) {
            if (sym->state != SYM_UNDEFINED) { AsmError(71); return -1; }
        } else old_offset = sym->offset;
        GetSymInfo(sym);
        if (Parse_Pass != PASS_1 && sym->offset != old_offset) PhaseError = 1;
        sym->state = 1; /* SYM_INTERNAL */
        sym->mem_type = mem_type;
        BackPatch(sym);
    }
    if (mem_type == T_STRUCT || label_dir) return 1;
    if (dup_array(sym, initializer_loc + 1, no_of_bytes) == -1) return -1;
    return 1;
}

/* CFa6a0..a6aa: the unsigned byte comparison is present in1994. */
void little_endian(char *text, char size)
{
    if ((unsigned char)size >= 2) strrev(text);
}

/* CFa6ac..a77b: ordinary C arithmetic; original Watcom uses its soft-float
 * CRT helpers. The10-byte temporary is on the stack in1994. */
void output_float(char index, char size, char negative)
{
    double double_value;
    float float_value;
    char *bytes;
    unsigned char count;
    double_value = strtod(AsmBuffer[(unsigned char)index]->string_ptr, NULL);
    if (negative) double_value *= -1;
    switch ((unsigned char)size) {
    case 4:
        float_value = double_value;
        bytes = (char *)&float_value;
        break;
    case 8:
        bytes = (char *)&double_value;
        break;
    case 10:
        bytes = alloca(11);
        for (count = 0; count <= 10; count++) bytes[count] = '\0';
        double2tbyte(double_value, bytes);
        break;
    }
    count = 0;
    while (count < (unsigned char)size) {
        AsmDataByte(*bytes);
        bytes++;
        count++;
    }
}
