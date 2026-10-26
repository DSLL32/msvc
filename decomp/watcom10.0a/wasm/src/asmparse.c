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

/* Recovered CF5908..6758 using released bld/wasm/c/asmins.c as a guide.
 * Older operand-width fallback and pass tail retained; no2002 fixup widening
 * or SizeString calls. Named pending consumers deny unrecovered encoding. */
#include "asmparse.h"
#include <recon.h>
struct asmfixup *InsFixups[3]; /* CF27228 */
unsigned char More_Array_Element = 0; /* CF25bf8 */
unsigned char Last_Element_Size; /* CF270ee */
char ConstantOnly; /* CF270ef */
unsigned char CheckSeg; /* CF270f1 */
unsigned char Opnd_Count; /* CF270f3 */
signed char Frame; /* CF270f4 */
unsigned char Frame_Datum; /* actual CF270f2 stores low byte of group/segment index */
int AsmParse(void)
{
    int i;
    int temp;
    char *char_ptr;
    struct asm_sym *sym;
    struct asmfixup *fixup;
    unsigned long cur_opnd;
    unsigned long last_opnd = OP_NONE;
    struct asm_code *rCode = Code;
    int use32;
    if( Use32 ) {
        Code->use32 = TRUE;
    } else {
        Code->use32 = FALSE;
    }
    i = proc_check();
    if( i == ERROR ) return( ERROR );
    if( i == TRUE ) return( NOT_ERROR );
    rCode->info.token = T_NULL;
    rCode->info.opcode = 0;
    rCode->info.rm_byte = 0;
    rCode->adrsiz = EMPTY;
    rCode->opsiz = EMPTY;
    rCode->mem_type = EMPTY;
    rCode->mem_type_fixed = FALSE;
    rCode->distance = EMPTY;
    rCode->seg_prefix = EMPTY;
    rCode->prefix = EMPTY;
    rCode->extended_ins = EMPTY;
    rCode->sib = 0;
    i = 2;
    while( i > 0 ) {
        i--;
        rCode->info.opnd_type[i] = OP_NONE;
        rCode->data[i] = 0;
        InsFixups[i] = NULL;
    }
    Opnd_Count = 0;
    if( More_Array_Element == TRUE ) {
        More_Array_Element = FALSE;
        return( dup_array( NULL, 0, Last_Element_Size ) );
    }
    CheckSeg = TRUE;
    Frame = EMPTY;
    while( i < Token_Count ) {
        switch( AsmBuffer[i]->token ) {
        case T_INS:
                ExpandTheWorld( i, FALSE );
            if( last_opnd != OP_NONE ) {
                AsmError( 51 /* SYNTAX_ERROR */ );
                return( ERROR );
            }
            cur_opnd = OP_NONE;
            switch( AsmBuffer[i]->value ) {
            case T_LOCK:
            case T_REP:
            case T_REPE:
            case T_REPNE:
            case T_REPNZ:
            case T_REPZ:
                rCode->prefix = AsmBuffer[i]->value;
                if( AsmBuffer[i+1]->token != T_INS ) {
                    AsmError( 52 /* PREFIX_MUST_BE_FOLLOWED_BY_AN_INSTRUCTION */ );
                    return( ERROR );
                }
                break;
            case T_RET:
                if( CurrProc != NULL ) {
                    return Ret( i, Token_Count );
                }
                rCode->info.token = AsmBuffer[i]->value;
                break;
            default:
                rCode->info.token = AsmBuffer[i]->value;
            }
            break;
        case T_UNARY_OPERATOR:
        {
            int operator_loc = i;
            if( MEM_TYPE( rCode->mem_type, BYTE ) ) {
                AsmError( 81 /* OFFSET_TOO_SMALL */ );
                return( ERROR );
            }
            i++;
            if( check_override( &i ) == ERROR ) {
                return ERROR;
            }
            i--;
            if( i + 1 < Token_Count ) {
                if( AsmBuffer[++i]->token == T_ID ) {
                    sym = AsmLookup( AsmBuffer[i]->string_ptr );
                    if( sym == NULL ) return ERROR;
                    switch( AsmBuffer[operator_loc]->value ) {
                    case T_OFFSET:
                        if( sym->state == SYM_STACK ) {
                            AsmError( 82 /* CANNOT_OFFSET_AUTO */ );
                            return( ERROR );
                        } else if( sym->state == SYM_GRP ) {
                            AsmError( 145 /* CANNOT_OFFSET_GRP */ );
                            return( ERROR );
                        }
                        break;
                    case T_SEG2:
                        if( sym->state == SYM_STACK ) {
                            AsmError( 83 /* CANNOT_SEG_AUTO */ );
                        }
                    }
                    switch( AsmBuffer[operator_loc]->value ) {
                    case T_OFFSET:
                    case T_SEG2:
                        use32 = rCode->use32;
                        if( AsmBuffer[operator_loc]->value == T_OFFSET ) {
                            dir_node *seg;
                            seg = GetSeg( sym );
                            if( seg != NULL ) {
                                use32 = seg->e.seginfo->segrec->d.segdef.use_32;
                            } else
                            {
                                use32 = (rCode->info.opnd_type[0] & (OP_M16 | OP_R16)) == 0;
                            }
                            if( use32 ) {
                                temp = FIX_OFF32;
                                rCode->mem_type = T_DWORD;
                            } else {
                                temp = FIX_OFF16;
                                rCode->mem_type = T_WORD;
                            }
                        } else {
                            temp = FIX_SEG;
                            rCode->mem_type = T_WORD;
                        }
                            find_frame( sym );
                            if( sym->state != SYM_STRUCT_FIELD ) {
                                fixup = AddFixup( sym, temp );
                                if( fixup == NULL ) return( ERROR );
                            }
                    case T_LENGTH:
                    case T_LENGTHOF:
                    case T_SIZE:
                    case T_SIZEOF:
                        switch( rCode->mem_type ) {
                        case EMPTY:
                            rCode->mem_type = T_WORD;
                        case T_WORD:
                            rCode->info.opcode |= W_BIT;
                            rCode->info.opnd_type[Opnd_Count] = OP_I16;
                            break;
                        case T_SDWORD:
                        case T_DWORD:
                            rCode->info.opcode |= W_BIT;
                            rCode->info.opnd_type[Opnd_Count] = OP_I32;
                            break;
                        }
                    }
                    switch( AsmBuffer[operator_loc]->value ) {
                    case T_OFFSET:
                        if( sym->state == SYM_STRUCT_FIELD ) {
                            Code->data[Opnd_Count] = sym->offset;
                        }
                        break;
                    case T_SEG2:
                        break;
                    case T_LENGTH:
                        Code->data[Opnd_Count] = sym->first_length;
                        break;
                    case T_LENGTHOF:
                        Code->data[Opnd_Count] = sym->total_length;
                        break;
                    case T_SIZE:
                        Code->data[Opnd_Count] = sym->first_size;
                        break;
                    case T_SIZEOF:
                        Code->data[Opnd_Count] = sym->total_size;
                        break;
                    default:
                        AsmError( 51 /* SYNTAX_ERROR */ );
                        return( ERROR );
                    }
                } else {
                    AsmError( 51 /* SYNTAX_ERROR */ );
                    return( ERROR );
                }
            }
            cur_opnd = OP_I;
            break;
        }
        case T_RES_ID:
            if( rCode->info.token == T_NULL ) {
                int name;
                if( i == 0 ) {
                    name=-1;
                } else if( i == 1 ) {
                    name=0;
                } else if( i == 2 ) {
                    name=0;
                }
                return( ( data_init( name, i ) == ERROR ) ? ERROR : NOT_ERROR );
            } else if( last_opnd != OP_NONE && last_opnd != OP_M ) {
                AsmError( 51 /* SYNTAX_ERROR */ );
                return( ERROR );
            }
            if( AsmBuffer[i]->value == T_PTR ) {
                if( AsmBuffer[i - 1]->token == T_RES_ID ) {
                    if( AsmBuffer[i - 1]->value != T_PTR ) {
                        if( operator( AsmBuffer[i]->value, TRUE ) == ERROR ) {
                            return( ERROR );
                        }
                        break;
                    }
                }
                AsmError( 53 /* NO_SIZE_GIVEN_BEFORE_PTR_OPERATOR */ );
                return( ERROR );
            }
             else if( operator( AsmBuffer[i]->value, TRUE ) == ERROR ) {
                return( ERROR );
            }
            cur_opnd = OP_NONE;
            break;
        case T_DIRECTIVE:
            return( directive( i, AsmBuffer[i]->value ) );
            break;
        case T_DIRECT_EXPR:
            if( Parse_Pass != PASS_1 ) {
                Modend = TRUE;
                if( Token_Count > 1 ) {
                    if( mem( i+1 ) == ERROR ) {
                        return ERROR;
                    }
                }
                return( directive( i, AsmBuffer[i]->value ) );
            } else {
                EndDirectiveFound = TRUE;
                return NOT_ERROR;
            }
            break;
        case T_ID:
            if( !( AsmBuffer[i+1]->token == T_DIRECTIVE &&
                ( AsmBuffer[i+1]->value == T_EQU ||
                  AsmBuffer[i+1]->value == T_EQU2 ||
                  AsmBuffer[i+1]->value == T_TEXTEQU ) ) ) {
                switch( ExpandSymbol( i, FALSE ) ) {
                case ERROR:
                    return( ERROR );
                case STRING_EXPANDED:
                    continue;
                }
            }
            if( last_opnd != OP_NONE && last_opnd != OP_M && last_opnd != OP_I ) {
                AsmError( 51 /* SYNTAX_ERROR */ );
                return( ERROR );
            }
            if( i == 0 ) {
                sym = AsmGetSymbol( AsmBuffer[i]->string_ptr );
                if( sym != NULL && sym->state == SYM_STRUCT &&
                    AsmBuffer[i+1]->token != T_DIRECTIVE ) {
                    AsmBuffer[i]->token = T_DIRECTIVE;
                    AsmBuffer[i]->value = T_STRUCT;
                    return( ( data_init( -1,0 ) == ERROR ) ? ERROR : NOT_ERROR );
                }
                switch( AsmBuffer[i + 1]->token ) {
                case T_COLON:
                    cur_opnd = OP_LABEL;
                    break;
                case T_ID:
                    sym = AsmGetSymbol( AsmBuffer[i+1]->string_ptr );
                    if( sym == NULL || sym->state != SYM_STRUCT ) {
                        AsmError( 51 /* SYNTAX_ERROR */ );
                        return( ERROR );
                    } else {
                        AsmBuffer[i + 1]->token = T_DIRECTIVE;
                        AsmBuffer[i + 1]->value = T_STRUCT;
                    }
                case T_RES_ID:
                    if( data_init( i, i+1 ) == ERROR ) {
                        return( ERROR );
                    } else {
                        return( NOT_ERROR );
                    }
                    break;
                case T_DIRECTIVE:
                    return( directive( i+1, AsmBuffer[i+1]->value ) );
                    break;
                default:
                    AsmError( 51 /* SYNTAX_ERROR */ );
                    return( ERROR );
                }
            } else {
                if( check_override( &i ) == ERROR ) {
                    return ERROR;
                }
                if( AsmBuffer[i]->token != T_ID ) {
                    AsmError( 51 /* SYNTAX_ERROR */ );
                    return( ERROR );
                }
                temp = NOT_ERROR;
                if( IS_ANY_BRANCH( rCode->info.token ) ) {
                    temp = jmp( i );
                    switch( temp ) {
                    case ERROR:
                    case SCRAP_INSTRUCTION:
                        return( temp );
                    case INDIRECT_JUMP:
                        temp = NOT_ERROR;
                        break;
                    default:
                        cur_opnd = rCode->info.opnd_type[Opnd_Count];
                        temp = ERROR;
                        break;
                    }
                }
                if( temp != NOT_ERROR ) {
                } else if( ( i = mem( i ) ) != ERROR ) {
                    cur_opnd = OP_NONE;
                    if( rCode->info.opnd_type[Opnd_Count] == OP_NONE ) {
                        rCode->info.opnd_type[Opnd_Count] = OP_M;
                    }
                } else {
                    return( ERROR );
                }
            }
            break;
        case T_STRING:
            if( last_opnd != OP_NONE ) {
                AsmError( 51 /* SYNTAX_ERROR */ );
                return( ERROR );
            }
            if( AsmBuffer[i]->value > ( (rCode->info.cpu&P_CPU_MASK) >= P_386 ? BYTE_4 : BYTE_2 ) ) {
                AsmError( 72 /* IMMEDIATE_DATA_TOO_BIG */ );
                return( ERROR );
            } else {
                unsigned len;
                unsigned long int value = 0;
                len = AsmBuffer[i]->value;
                temp = 0;
                char_ptr = AsmBuffer[i]->string_ptr;
                while( temp < len ) {
                    value <<= 8;
                    value |= (unsigned char)*char_ptr;
                    temp++;
                    char_ptr++;
                }
                if( idata( value ) == ERROR ) {
                    return( ERROR );
                }
            }
            break;
        case T_COMMA:
            cur_opnd = OP_NONE;
            if( Opnd_Count == OPND2 ) {
                switch( rCode->info.token ) {
                case T_IMUL:
                    if( rCode->info.opnd_type[OPND1] & OP_R ) {
                        if( AsmBuffer[i + 1]->token == T_NUM ) {
                            rCode->info.opnd_type[OPND1] =
                                                rCode->info.opnd_type[OPND2];
                            rCode->data[OPND1] = rCode->data[OPND2];
                            rCode->data[OPND2] = 0;
                            InsFixups[OPND1] = InsFixups[OPND2];
                            InsFixups[OPND2] = NULL;
                            break;
                        }
                    }
                    AsmError( 54 /* INVALID_IMUL_FORMAT */ );
                    return( ERROR );
                case T_SHLD:
                case T_SHRD:
                    switch( AsmBuffer[i + 1]->token ) {
                    case T_NUM:
                        break;
                    case T_REG:
                        i++;
                        if( AsmBuffer[i]->value == T_CL ) {
                            break;
                        }
                    default:
                        AsmError( 55 /* INVALID_SHLD_SHRD_FORMAT */ );
                        return( ERROR );
                    }
                    break;
                case T_NULL:
                    break;
                default:
                    AsmError( 56 /* TOO_MANY_COMMAS */ );
                    return( ERROR );
                }
            } else {
                Opnd_Count++;
                    Frame = EMPTY;
            }
            break;
        case T_COLON:
            if( rCode->info.opnd_type[Opnd_Count] & OP_SR ) {
                rCode->info.opnd_type[Opnd_Count] = OP_M;
                cur_opnd = OP_M;
            } else if ( last_opnd == OP_LABEL ) {
                if( AsmBuffer[i+1]->token != T_RES_ID ) {
                    if( MakeLabel( AsmBuffer[i-1]->string_ptr,EMPTY )==ERROR ) {
                         return( ERROR );
                    }
                }
                cur_opnd = OP_NONE;
            } else if( rCode->info.token == T_CALLF ||
                       rCode->info.token == T_JMPF ) {
                if( AsmBuffer[i - 1]->token == T_NUM ) {
                    rCode->info.opnd_type[Opnd_Count] = OP_I16;
                    Opnd_Count++;
                    cur_opnd = OP_NONE;
                } else {
                    AsmError( 57 /* SYNTAX_ERROR_UNEXPECTED_COLON */ );
                    return( ERROR );
                }
            } else {
                AsmError( 57 /* SYNTAX_ERROR_UNEXPECTED_COLON */ );
                return( ERROR );
            }
            break;
        case T_DOT:
            switch( last_opnd ) {
            case OP_M:
            case OP_I:
                break;
            case OP_NONE:
                return( directive( i, AsmBuffer[i]->value ) );
            default :
                AsmError( 26 /* UNKNOWN_DIRECTIVE */ );
                return( ERROR );
            }
            break;
        case T_OP_SQ_BRACKET:
            i++;
        case T_NUM:
            switch( last_opnd ) {
            case OP_M16_IND:
            case OP_M32_IND:
            case OP_M48_IND:
            case OP_I8:
            case OP_I16:
            case OP_I32:
            case OP_J32:
            case OP_J48:
                if( IS_BRANCH( Code->info.token ) ) {
                    i = mem( i );
                }
                break;
            case OP_NONE:
            case OP_M:
            case OP_I:
                i = mem( i );
                if( i != ERROR ) {
                    cur_opnd = OP_NONE;
                    if( ConstantOnly ) {
                        if( idata( rCode->data[Opnd_Count] ) == ERROR ) {
                            return( ERROR );
                        }
                    } else {
                        rCode->info.opnd_type[Opnd_Count] = OP_M;
                    }
                } else {
                    return( ERROR );
                }
                break;
            default :
                AsmError( 51 /* SYNTAX_ERROR */ );
                return( ERROR );
            }
            break;
        case T_CL_SQ_BRACKET:
            break;
        case T_REG:
            switch( last_opnd ) {
            case OP_NONE:
                if( ( i = reg( i ) ) != ERROR ) {
                    cur_opnd = OP_R;
                } else {
                    return( ERROR );
                }
                break;
            default:
                AsmError( 51 /* SYNTAX_ERROR */ );
                return( ERROR );
            }
            break;
        case T_PERCENT:
            cur_opnd = OP_NONE;
            ExpandTheWorld( i, FALSE );
            break;
        case T_NOOP:
            break;
        }
        last_opnd = cur_opnd;
        i++;
    }
    switch( rCode->info.token ) {
    case T_PUSH:
        if( rCode->use32
         && rCode->info.opnd_type[OPND1] == OP_I16 ) {
            rCode->info.opnd_type[OPND1] = OP_I32;
        }
        break;
    }
    if( Opnd_Count > OPND1 ) {
        if( check_size() == ERROR ) {
            return( ERROR );
        }
    }
    return( match_phase_1() );
}

/* CF5894..5907: queue the first instruction after a procedure declaration
 * only after the separate prologue writer succeeds. */
int proc_check(void)
{
    if (CurrProc == NULL || Token_Count == 0 || !DefineProc) return 0;
    if (AsmBuffer[0]->token == T_DIRECTIVE) return 0;
    if (Token_Count > 1) {
        if (AsmBuffer[1]->token == T_DIRECTIVE ||
            AsmBuffer[1]->token == T_DIRECT_EXPR) return 0;
    }
    if (WritePrologue() == -1) return -1;
    InputQueueLine(CurrString);
    DefineProc = 0;
    return 1;
}

/* reg CF53e0..5894: no later MMX/CMOV paths; test-register validity
 * ends at the actual586 CPU boundary. Only recovered register rows admit data. */
int reg( int i )
/*
- parse and encode the register operand;
*/
{
    int                 temp;
    int                 reg;

    temp = AsmOpcode[AsmBuffer[i]->value].position;
    if (AsmOpTable[temp].opnd_type[1] == OP_NONE) WASM_PENDING();
    reg = AsmOpTable[temp].opcode;
    Code->info.opnd_type[Opnd_Count] = AsmOpTable[temp].opnd_type[1];
    switch( AsmOpTable[temp].opnd_type[OPND2] ) {
    case OP_AL:
    case OP_R8:
    case OP_R8_GEN:
        Code->info.opcode &= NOT_W_BIT;         // clear w-bit
        break;
    case OP_CL: /* only appears in "shift opnd,CL" instructions */
        break;
    case OP_AX:
    case OP_DX: /* only appears in "in" and "out" instructions  */
    case OP_R16:
    case OP_R16_GEN:
        Code->info.opcode |= W_BIT;             // set w-bit
        if( Code->use32 ) Code->opsiz = NOT_EMPTY;
        break;
    case OP_ST:
        switch( AsmBuffer[i + 1]->token ) {
        case '(':
            i++;
            /* fall through ... "st 1" will be treated the same as "st(1)" */
        case T_NUM:
            i++;
            switch( AsmBuffer[i]->token ) {
            case T_NUM:
                temp = AsmBuffer[i]->value & BIT_012;
                Code->info.rm_byte |= temp;
                if( temp != 0 ) {
                    Code->info.opnd_type[Opnd_Count] = OP_ST_REG;
                }
                if( AsmBuffer[i+1]->token == ')' ) {
                    // the next token should be ')'
                    i++;
                }
                break;
            default:
                AsmError( 34 );
                return( ERROR );
            }
            break;
        case T_COMMA:
        case T_FINAL:
            break;
        default:
            AsmError( 34 );
            return( ERROR );
        }
        break;
    case OP_SR3:                        // 386 segment register
        if( (Code->info.cpu&(P_CPU_MASK|P_PM)) <= P_286p ) {
            // 8086 ins cannot use 80386 segment register
            AsmError( 38 );
            return( ERROR );
        }
    case OP_SR:                                 // any seg reg
    case OP_SR2:                                // 8086 segment register
        if( AsmBuffer[i + 1]->token == T_COLON ) {
            /* segment override */
            Code->seg_prefix = AsmOpTable[temp].opcode;
            /* fixme - setup the frame here */
            return( i );
        }
        if( AsmBuffer[i]->value == T_CS ) {
            // POP CS is not allowed
            if( Code->info.token == T_POP ) {
                AsmError( 39 );
                return( ERROR );
            }
        }
        reg = get_sr_rm_byte( AsmOpTable[temp].opcode );
        Code->info.rm_byte |= reg << 3;                 // put in bit-345
        return( i );
        break;
    case OP_EAX:
    case OP_R32_GEN:
    case OP_R32:
        if( (Code->info.cpu&(P_CPU_MASK|P_PM)) <= P_286p ) {
            // 8086 ins cannot use 386 register
            AsmError( 40 );
            return( ERROR );
        }
        Code->info.opcode |= W_BIT;             // set w-bit
        if( !Code->use32 ) Code->opsiz = NOT_EMPTY;
        break;
    case OP_TR:                 // Test registers
        switch( AsmBuffer[i]->value ) {
        case T_TR3:
        case T_TR4:
        case T_TR5:
            if( ( (Code->info.cpu&P_CPU_MASK) < P_486
               || (Code->info.cpu&P_CPU_MASK) >= P_586 )
                &&  (AsmOpTable[temp].cpu&P_CPU_MASK) >= P_486 ) {
                // TR3, TR4, TR5 are available on 486 only
                AsmError( 42 );
                return( ERROR );
            }
            break;
        case T_TR6:
        case T_TR7:
            if( ( (Code->info.cpu&P_CPU_MASK) < P_386
               || (Code->info.cpu&P_CPU_MASK) >= P_586 )
                &&  (AsmOpTable[temp].cpu&P_CPU_MASK) >= P_386 ) {
                // TR6, TR7 are available on 386...586 only
                AsmError( 42 );
                return( ERROR );
            }
            break;
        }
    case OP_CR:                 // Control registers
    case OP_DR:                 // Debug registers
        if( Code->info.token != T_MOV ) {
            AsmError( 41 );
            return( ERROR );
        }
//      Code->info.rm_byte |= MOD_11;                   /* 13-feb-92 */
        Code->info.rm_byte |= reg << 3;
        return( i );
        break;
    }
    if( Opnd_Count == OPND1 ) {
        // the first operand
        // r/m is treated as a 'reg' field
        Code->info.rm_byte |= MOD_11;
        // fill the r/m field
        Code->info.rm_byte |= reg;
        // special case for IMUL ( it has 3 opnds )
        if( Code->info.token == T_IMUL ) {
            /* if IMUL has 2 opnds only, and we are
               now parsing the second opnd, then fill bit-345 field */
            if( AsmBuffer[i + 1]->token == T_COMMA  &&  i+3 >= Token_Count ) {
                Code->info.rm_byte |= ( reg << 3 );
            }
        }
    } else {
        switch( Code->info.token ) {
        case T_BSF:
        case T_BSR:
        case T_MOVSX:
        case T_MOVZX:
        case T_LAR:
        case T_LSL:
        case T_IMUL:
            /*some instructions reverse the order of dest
              and source, so shift bit-012 to left by 3-bit
              and then move op2 to bit-012 */
            Code->info.rm_byte =
                       ( ( ( Code->info.rm_byte & BIT_012 ) << 3 )
                         | ( Code->info.rm_byte & BIT_67 ) | reg );
            break;
        case T_XCHG:
            // XCHG can use short form if op1 is OP_A
            if( Code->info.opnd_type[OPND1] & OP_A ) {
                Code->info.rm_byte |= reg;
                break;
            }
        default:
            if( Code->info.opnd_type[OPND1] & ( OP_I | OP_SPEC_REG |
                                                OP_SR | OP_J32 | OP_J48 ) ) {
                /* last operand is idata, segreg or special reg
                   they used reg field, use r/m field */
                Code->info.rm_byte &= NOT_BIT_012;
                Code->info.rm_byte |= reg;
            } else {
                // fill reg field with reg
                Code->info.rm_byte |= ( reg << 3 );
            }
        }
    }
    return( i );
}

/* CF5394..53e0, segment-register field from its prefix byte. */
unsigned char get_sr_rm_byte( unsigned char seg_prefix )
{
    switch( seg_prefix ) {
    case PREFIX_ES: return 0;
    case PREFIX_CS: return 1;
    case PREFIX_SS: return 2;
    case PREFIX_DS: return 3;
    case PREFIX_FS: return 4;
    case PREFIX_GS: return 5;
    }
    InternalError( "..\\c\\asmins.c", 1236 );
    return 0;
}
