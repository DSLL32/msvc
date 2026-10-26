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

/* Adapted from bld/wasm/c/asmins.c AsmInit at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9; actual CF6e68..6ffa.
 * Token projection preserves original 939 row indices. CPU5 shares CPU4's
 * branch in the 1994 switch; newer Pentium/686 source branches are absent. */
#include <stdlib.h>
#include <limits.h>
#include "breakout.h"
#include "asminit.h"
#include "asmparse.h"
#include <recon.h>

static struct asm_tok tokens[100];
struct asm_tok *AsmBuffer[100];
static struct asm_code Code_Info;
struct asm_code *Code = &Code_Info;

void AsmInit(int cpu, int fpu, int use32)
{
    int pos = 0;
    unsigned short token_value = 1;
    int count;
    for (count = 0; count < 100; ++count) {
        AsmBuffer[count] = &tokens[count];
    }
    if (AsmOpcode[1].position == 0) {
        while (AsmOpcode[token_value].len != 0) {
            do {
                ++pos;
            } while (AsmOpTable[pos].token != token_value && pos <= 939);
            if (pos > 939) {
                AsmError(138);
                exit(-1);
            }
            AsmOpcode[token_value].position = pos;
            ++token_value;
        }
        switch (use32) {
        case 0: Code->use32 = 0; break;
        case 1: Code->use32 = 1; break;
        }
        if (fpu < 0) fpu = 0;
        switch (cpu) {
        case 0:
            if (fpu) Code->info.cpu |= 1;
            break;
        case 1:
            Code->info.cpu |= 0x10;
            if (fpu) Code->info.cpu |= 1;
            break;
        case 2:
            Code->info.cpu |= 0x28;
            if (fpu) Code->info.cpu |= 2;
            break;
        case 3:
            Code->info.cpu |= 0x38;
            if (fpu) Code->info.cpu |= 4;
            break;
        case 4:
        case 5:
            Code->info.cpu |= 0x48;
            if (fpu) Code->info.cpu |= 4;
            break;
        }
    }
    make_inst_hash_table();
}

/* bld/wasm/c/asmins.c, cpu_directive; CF5054..50d1.  The original
 * NO87 branch changes only Code's FPU bits, not Options.floating_point
 * (that latter assignment was added after this toolchain). */
int cpu_directive(unsigned short token)
{
    int processor;
    MakeCPUConstant((long)token);
    processor = comp_opt(token);
    if (processor == EMPTY) {
        AsmError(26);
        return ERROR;
    }
    if (processor == 0) {
        Code->info.cpu &= ~7;
    } else if (processor & 7) {
        Code->info.cpu &= ~7;
        Code->info.cpu |= processor;
    } else {
        Code->info.cpu &= ~0x78;
        Code->info.cpu |= processor;
    }
    return NOT_ERROR;
}

/* Original CF5014..5051; switch table CF4fe8..5014.  The 1994
 * fallthrough hierarchy ends at 586; no later 686 directives exist. */
void MakeCPUConstant(long token)
{
    MakeConstant(token);
    switch (token) {
    case T_586P:
    case T_586:
        MakeConstant(T_586);
    case T_486P:
    case T_486:
        MakeConstant(T_486);
    case T_386P:
    case T_386:
        MakeConstant(T_386);
        break;
    case T_286P:
    case T_286:
        MakeConstant(T_286);
    }
}

/* Source-derived CPU option records, actual CF25c00..25c2d.  The
 * 1994 terminator is0x60; the later686 entries and0x70 sentinel are absent.
 * CPU levels occupy bits4..6, protected mode bit3, FPU bits0..2. */
int comp_opt(unsigned short token)
{
    int i;
#pragma pack(1)
    static struct option {
        unsigned short direct;
        signed char value;
    } processor[] = {
        {T_NO87,0}, {T_8086,0}, {T_8087,1}, {T_186,0x10},
        {T_286,0x20}, {T_287,2}, {T_286P,0x28},
        {T_386,0x30}, {T_387,4}, {T_386P,0x38},
        {T_486,0x40}, {T_486P,0x48},
        {T_586,0x50}, {T_586P,0x58}, {0,0x60}
    };
#pragma pack()
    for (i = 0; processor[i].value != 0x60; ++i) {
        if (token == processor[i].direct) return processor[i].value;
    }
    return EMPTY;
}

/* CF270ed/270f0; shared override-parser owners used by later addressing. */
signed char Label_Idx;
unsigned char AssumeError;

/* CF4164..4255; the existing row identity supplies the encountered segment
 * prefix byte. Other register encoding domains remain unreconstructed. */
int check_override(int *i)
{
    int index = *i;
    int reg;
    if (index + 2 < Token_Count && AsmBuffer[index + 1]->token == T_COLON) {
        switch (AsmBuffer[index]->token) {
        case T_REG:
            reg = AsmBuffer[index]->value;
            if (reg != T_ES && reg != T_CS && reg != T_SS && reg != T_DS &&
                reg != T_FS && reg != T_GS) WASM_PENDING();
            Code->seg_prefix = AsmOpTable[AsmOpcode[reg].position].opcode;
            *i += 2;
            if (*i >= Token_Count) { AsmError(1052); return -1; }
            break;
        case T_ID:
            AssumeError = 0;
            Label_Idx = index;
            check_assume(-2);
            if (AssumeError) return -1;
            *i += 2;
            if (*i >= Token_Count) { AsmError(1052); return -1; }
            return FixOverride(*i);
        }
    }
    return 1;
}

/* check_size CF6758..6e4e, adapted from initial2002 source. No MMX,
 * old LSL requires a16-bit second operand; string sizing stays inline. */
int check_size( void )
/*
- use to make sure the size of first operand match the size of second operand;
- optimize MOV instruction;
*/
{
    unsigned long       op1 = Code->info.opnd_type[OPND1];
    unsigned long       op2 = Code->info.opnd_type[OPND2];
    int                 state = NOT_ERROR;
    int                 temp;
    int                 op1_size;
    int                 op2_size;
    int                 op_size = 0;

    switch( Code->info.token ) {
    case T_IN:
        if( op2 == OP_DX ) {
            switch( op1 ) {
            case OP_AX:
                break;
            case OP_AL:
                Code->info.opcode &= NOT_W_BIT;         // clear w-bit
            case OP_EAX:
                if( Code->use32 ) Code->opsiz = EMPTY;
            }
        }
        break;
    case T_OUT:
        if( op1 == OP_DX ) {
            switch( op2 ) {
            case OP_AX:
                break;
            case OP_AL:
                Code->info.opcode &= NOT_W_BIT;         // clear w-bit
            case OP_EAX:
                if( Code->use32 ) Code->opsiz = EMPTY;
            }
        }
        break;
    case T_INS2:
    case T_CMPS:
        op_size = OperandSize( op1 );
        /* fall through */
    case T_MOVS:
    case T_OUTS:
        if( op_size == 0 ) op_size = OperandSize( op2 );

        /* SizeString is inlined in the shipped1994 body. */
        switch (op_size) {
        case 1:
            Code->mem_type = T_BYTE;
            Code->info.opcode &= NOT_W_BIT;
            if (Code->use32) Code->opsiz = EMPTY;
            break;
        case 2:
            Code->mem_type = T_WORD;
            Code->info.opcode |= W_BIT;
            if (!Code->use32) Code->opsiz = NOT_EMPTY;
            break;
        case 4:
            Code->mem_type = T_DWORD;
            Code->info.opcode |= W_BIT;
            if (Code->use32) Code->opsiz = EMPTY;
            break;
        }
        break;
    case T_LEA:
        switch( OperandSize( op1 ) ) {
        case 2:
        case 4:
            break;
        default:
            AsmError( 58 );
            state = ERROR;
        }
        break;
    case T_RCL:
    case T_RCR:
    case T_ROL:
    case T_ROR:
    case T_SAL:
    case T_SAR:
    case T_SHL:
    case T_SHR:
    case T_LDS:
    case T_LES:
    case T_LFS:
    case T_LGS:
    case T_LSS:
        // checking will be done later
        break;
    case T_ENTER:
        // ENTER has to be OP_I16, OP_I8
        if( op1 == OP_I32 ) {
            //parse_phase_1 will treat 16-bit data as OP_I32 if CPU is 386
            if( Code->data[OPND1] > (signed long)USHRT_MAX ) {
                // if op1 is really 32-bit data, then error
                AsmError( 72 );
                state = ERROR;
            }
        }
        // type cast op1 to OP_I16
        Code->info.opnd_type[OPND1] = OP_I16;
        // op2 have to be 8-bit data
        if( op2 >= OP_I16 ) {
            if( Code->data[OPND2] > UCHAR_MAX ) {
                AsmError( 72 );
                state = ERROR;
            }
            Code->info.opnd_type[OPND2] = OP_I8;
        }
        break;
    case T_CALLF:
    case T_JMPF:
        // segment can only be 16-bit
        if( op1 > OP_I16 ) {
            AsmError( 84 );
            state = ERROR;
        }
        // offset can only be 16-bit if CPU is 286 and down
        if( !Code->use32 && op2 > OP_I16 ) {
            AsmError( 85 );
            state = ERROR;
        }
        // swap the 2 opnds to make output easier
        temp = Code->data[OPND1];
        Code->data[OPND1] = Code->data[OPND2];
        Code->data[OPND2] = temp;
        Code->info.opnd_type[OPND1] = Code->use32 ? OP_I32 : OP_I16;
        Code->info.opnd_type[OPND2] = OP_I16;
        break;
    case T_MOVSX:
    case T_MOVZX:
        Code->info.opcode &= NOT_W_BIT;
        op2_size = OperandSize( op2 );
        switch( OperandSize( op1 ) ) {
        case 4:
            switch( op2_size ) {
            case 2:
                Code->info.opcode |= W_BIT;
                break;
            case 4:
                AsmError( 86 );
                state = ERROR;
            }
            if( Code->use32 ) {
                Code->opsiz = EMPTY;     // - don't need opnd size prefix
            }
            break;
        case 2:
            if( op2_size >= 2 ) {
                AsmError( 86 );
                state = ERROR;
            }
            break;
        default:
            // op1 have to be r16/r32
            AsmError( 87 );
            state = ERROR;
        }
        break;
    case T_LSL:                                 /* 19-sep-93 */
        op1_size = OperandSize( op1 );
        switch( op1_size ) {
        case 2:
            if( Code->use32 ) Code->opsiz = NOT_EMPTY;
            break;
        case 4:
            if( Code->use32 ) Code->opsiz = EMPTY;
            break;
        default:
            AsmError( 148 );
            return( ERROR );
        }
        op2_size = OperandSize( op2 );
        switch( op2_size ) {
        case 2:
            break;
        default:
            AsmError( 148 );
            state = ERROR;
            break;
        }
        break;
    case T_MOV:
        // to optimize MOV
        if( op1 == OP_M || op2 == OP_M ) {
            if( ( (Code->use32 ? Code->adrsiz == EMPTY : Code->adrsiz == NOT_EMPTY) && Code->info.rm_byte == 5 ) ||
                   ( !(Code->use32 ? Code->adrsiz == EMPTY : Code->adrsiz == NOT_EMPTY) && Code->info.rm_byte == 6 ) ) {
                // DS:[d32] or DS:[d16] can use MOV Mem with Acc (short form)
            } else {
                // we have to change OP_A to OP_R
                if( op1 == OP_M ) {
                    temp = OPND2;
                } else {
                    temp = OPND1;
                }
                switch( Code->info.opnd_type[temp] ) {
                case OP_AL:
                    /* fixme ? */
                    // Code->info.opnd_type[temp] = OP_R8;
                    Code->info.opnd_type[temp] = OP_R8_GEN;
                    break;
                case OP_AX:
                    Code->info.opnd_type[temp] = OP_R16_GEN;
                    break;
                case OP_EAX:
                    Code->info.opnd_type[temp] = OP_R32_GEN;
                    break;
                }
            }
        }
        // no break;
    default:
        // make sure the 2 opnds are of the same type
        op1_size = OperandSize( op1 );
        op2_size = OperandSize( op2 );
        if( op1_size > op2_size ) {
            if( op2 >= OP_I8  &&  op2 <= OP_I32 ) {     /* immediate */
                op2_size = op1_size;    /* promote to larger size */
            }
        }
        if( op1_size == 1 && op2 == OP_I16 && Code->data[OPND1] <= UCHAR_MAX ) {
            return( state ); // OK cause no sign extension
        }
        if( op1_size == 2 && op2 == OP_I32 &&
            Code->data[OPND1] <= (signed long)USHRT_MAX ) {
            return( state ); // OK cause no sign extension
        }
        if( op1_size != op2_size ) {
            /* if one or more are !defined, set them appropriately */
            if( op1_size != 0  &&  op2_size != 0 ) {
                AsmError( 58 );
                state = ERROR;
            }
            if( op1_size == 0 ) {
                switch( op2_size ) {
                case 1:
                    Code->mem_type = T_BYTE;
                    #if 1
                        if( Parse_Pass == PASS_1 && ( op2 & OP_I ) ) {
                            AsmWarn( 1, 150 );
                        }
                    #endif
                    break;
                case 2:
                    Code->mem_type = T_WORD;
                    Code->info.opcode |= W_BIT;
                    #if 1
                        if( Parse_Pass == PASS_1 && ( op2 & OP_I ) ) {
                            AsmWarn( 1, 151 );
                        }
                    #endif
                    if( Code->use32 ) Code->opsiz = NOT_EMPTY;
                    break;
                case 4:
                    Code->mem_type = T_DWORD;
                    #if 1
                        if( Parse_Pass == PASS_1 && ( op2 & OP_I ) ) {
                            AsmWarn( 1, 152 );
                        }
                    #endif
                    Code->info.opcode |= W_BIT;
                    break;
                }
            }
        }
    }
    return( state );
}

/* CF4bbc..4ca8; no later MMX operand case. */
int OperandSize( unsigned long opnd )
/***********************************/
{
    if( opnd == OP_NONE || opnd & OP_SPECIAL ) {
        return( 0 );

    } else if( opnd == OP_M ) {
        /* fixme */
        switch( Code->mem_type ) {
        case EMPTY:     return( 0 );
#if 1
        case T_SBYTE:
#endif
        case T_BYTE:    return( 1 );
#if 1
        case T_SWORD:
#endif
        case T_WORD:    return( 2 );
#if 1
        case T_SDWORD:
#endif
        case T_DWORD:   return( 4 );
        case T_FWORD:   return( 6 );
        case T_PWORD:   return( 6 );
        case T_QWORD:   return( 8 );
        case T_TBYTE:   return( 10 );
        }
    } else if( opnd & ( OP_M8_R8 | OP_M_B | OP_I8 | OP_I_1 | OP_I_3 | OP_I8_U ) ) {
        return( 1 );
    } else if( opnd & ( OP_M16_R16 | OP_M_W | OP_I16 | OP_SR ) ) {
        return( 2 );
    } else if( opnd & ( OP_R32 | OP_M_DW | OP_I32 | OP_J32 | OP_SPEC_REG ) ) {
        return( 4 );
    } else if( opnd & ( OP_I | OP_J48 ) ) {
        return( 6 );
    } else if( opnd & OP_M_QW ) {
        return( 8 );
    } else if( opnd & ( OP_STI | OP_M_TB ) ) {
        return( 10 );
    }
    return( 0 );
}

/* CF3e84..3edc, six-way dispatch table CF3e6c..3e84.
 * Frame datum really is a byte in this version, not the later wide field. */
void find_frame(asm_sym *sym)
{
    switch (sym->state) {
    case SYM_INTERNAL:
    case SYM_PROC:
        if (Frame != EMPTY) break;
        if (sym->grpidx != 0) {
            Frame = 1;
            Frame_Datum = sym->grpidx;
        } else {
            Frame = 0;
            Frame_Datum = sym->segidx;
        }
        break;
    case SYM_GRP:
        Frame = 1;
        Frame_Datum = sym->grpidx;
        break;
    case SYM_SEG:
        Frame = 0;
        Frame_Datum = sym->segidx;
        break;
    }
}

/* CF4278..4b4a: no later ignored-STRUCT branch, Label_Idx reset or
 * field-only PTR application. Modend missing-symbol is an assertion. */
int mem( int i )
/*
  parse the memory reference operand
*/
{
    char                ss = SCALE_FACTOR_1;
    char                count = Token_Count;
    int                 index = EMPTY;
    int                 base = EMPTY;
    struct asm_sym      *sym;
    struct asm_sym      *field;
    struct asmfixup     *fixup;
    char                base_lock = FALSE;
    char                *string_ptr;
    char                id_flag = 0;
#if 1
    char                field_flag = 0;
    int                 fix_type;

    Label_Idx = -1;
    AssumeError = FALSE;
    sym = NULL;
#endif
    fixup = NULL;
    ConstantOnly = TRUE;
    if( Code->seg_prefix != EMPTY ) {
        //fixme
        /* temporary fix -- we will probably have to change the structure of
         * the asm_code structure, to allow multiple seg. overrides per line
         * for "MOVS ds:si, es:di" for example
         *
         * we need to have Constantonly set IFF we have a number which does NOT
         * have a seg override
         * for now check if the token before this one is a colon
         * or if the prev. token was a [ and a : before that
         */
        if( ( AsmBuffer[i-1]->token == T_COLON ) ||
            ( ( AsmBuffer[i-1]->token == T_OP_SQ_BRACKET ) &&
              ( AsmBuffer[i-2]->token == T_COLON ) ) ) {
            ConstantOnly = FALSE;
        }
    }
    while( AsmBuffer[i]->token != T_FINAL && AsmBuffer[i]->token != T_COMMA ) {
        switch( AsmBuffer[i]->token ) {
        case T_REG:
            ConstantOnly = FALSE;
            switch( AsmBuffer[i]->value ) {     // check for index registers
            case T_EAX:
            case T_EBX:
            case T_ECX:
            case T_EDX:
            case T_ESP:
            case T_EBP:
            case T_ESI:
            case T_EDI:
                if( (Code->info.cpu&(P_CPU_MASK|P_PM)) <= P_286p ) {
                    // 286 and down cannot use 386 registers
                    AsmError( 7 );
                    return( ERROR );
                }
                if( !Code->use32 ) Code->adrsiz = NOT_EMPTY;
                break;
            case T_BX:
            case T_BP:
            case T_SI:
            case T_DI:
                if( Code->use32 ) Code->adrsiz = NOT_EMPTY;
                break;
            default:
                AsmError( 6 );
                return( ERROR );
            }
            switch( AsmBuffer[i + 1]->token ) {
            case T_REG:  // cur reg is base
                if( *(AsmBuffer[i + 1]->string_ptr) != '+' ) {
                    AsmError( 51 );
                    return( ERROR );
                }
                // insert base
                base = AsmBuffer[i]->value;
                break;
            case '-':
                if( AsmBuffer[i+2]->token != T_NUM ) {
                    AsmError( 106 );
                    return( ERROR );
                }
                AsmBuffer[i+1]->token = '+';
                AsmBuffer[i+2]->value*=-1;
                /* FALL THROUGH */
            case '+':
            case T_DOT:     // for structure members
                /* fixme - this could be combined with the case below */
                if( base != EMPTY ) {
                    if( base_lock == TRUE ) {
                        // [reg + data][reg + data] is not allowed
                        AsmError( 8 );
                        return( ERROR );
                    } else {
                        index = base;
                    }
                }
                base = AsmBuffer[i]->value;
                switch( AsmBuffer[i+2]->token ) {
                case T_NUM:
                    base_lock = TRUE;   // add lock
                    i+=2;
                    Code->data[Opnd_Count] += AsmBuffer[i]->value;
                    break;
                case T_REG:
                    i++;
                    break;
                }
                break;

            case T_NUM : // cur reg is base
                if( *(AsmBuffer[i + 1]->string_ptr) != '+' ) {
                    AsmError( 51 );
                    return( ERROR );
                }
                if( base != EMPTY ) {
                    if( base_lock == TRUE ) {
                        // [reg + data][reg + data] is not allowed
                        AsmError( 8 );
                        return( ERROR );
                    } else {
                        index = base;
                    }
                }
                base = AsmBuffer[i]->value;
                base_lock = TRUE;   // add lock
                i++;
                Code->data[Opnd_Count] += AsmBuffer[i]->value;
                break;
            case '*' :  // 386, cur reg is index
                if( (Code->info.cpu&P_CPU_MASK) >= P_386 ) {
                    if( !Code->use32 ) Code->adrsiz = NOT_EMPTY;
                    index = AsmBuffer[i]->value;
                    switch( index ) {
                    case T_ESP:
                    case T_BX:
                    case T_BP:
                    case T_SI:
                    case T_DI:
                        // cannot use ESP or 16-bit reg as index
                        AsmError( 9 );
                        return( ERROR );
                    default:
                        if( !Code->use32 ) Code->adrsiz = NOT_EMPTY;
                        i += 2;
                        if( AsmBuffer[i]->token == T_NUM ) {
                            switch( AsmBuffer[i]->value ) {
                            case 1:
                                // ss = 00
                                break;
                            case 2:
                                // ss = 01
                                ss = SCALE_FACTOR_2;
                                break;
                            case 4:
                                // ss = 10
                                ss = SCALE_FACTOR_4;
                                break;
                            case 8:
                                // ss = 11
                                ss = SCALE_FACTOR_8;
                                break;
                            default: // must be * 1, 2, 4 or 8
                                AsmError( 10 );
                                return( ERROR );
                            }
                        } else {
                            // '*' must be followed by a number
                            AsmError( 34 );
                            return( ERROR );
                        }
                    }
                } else {
                    // 286 and down cannot use this memory mode
                    AsmError( 17 );
                    return( ERROR );
                }
                break;
            case T_CL_SQ_BRACKET:
                if( base == EMPTY ) {
                    // cur reg is base
                    base = AsmBuffer[i]->value;
                } else if( index == EMPTY ) {
                    if( AsmBuffer[i]->value == T_ESP ) {
                        if( base_lock == TRUE ) {
                            AsmError( 18 );
                            return( ERROR );
                        } else {
                            index = base;
                            base = AsmBuffer[i]->value;
                        }
                    } else {
                        index = AsmBuffer[i]->value;
                    }
                } else {
                    AsmError( 19 );
                    return( ERROR );
                }
                i++;
                break;
            case T_ID:
                // insert base
                base = AsmBuffer[i]->value;
                break;
            }
            break;
        case '-':
            if( AsmBuffer[i+1]->token != T_NUM ) {
                AsmError( 106 );
                return( ERROR );
            }
            i++;
            Code->data[Opnd_Count] -= AsmBuffer[i]->value;
            break;
        case T_NUM: // direct address
            Code->data[Opnd_Count] += AsmBuffer[i]->value;
            break;
        case T_COMMA:
            count = i;
            i--;
            break;
        case T_OP_SQ_BRACKET:
        case T_CL_SQ_BRACKET:
        case T_DOT:
        case '+':
            break;
        case T_ID:
            // ConstantOnly = FALSE;
            string_ptr = AsmBuffer[i]->string_ptr;
            if( *string_ptr == '+' ) {
                string_ptr++;
            } else if( *string_ptr == '-' ) {
                AsmError( 21 );
                return( ERROR );
            }
            field = sym;
            sym = AsmLookup( string_ptr );
            if( sym == NULL ) return ERROR;
#if 1
            if( sym->state == SYM_STRUCT_FIELD ) {
                //ConstantOnly = TRUE;
                Code->data[Opnd_Count] += sym->offset;
                // fixme
                if( operator( sym->mem_type, FALSE ) == ERROR ) return( ERROR );
                //if( operator( T_PTR, FALSE ) == ERROR ) return( ERROR );
                i++;
                sym = field;
                field_flag = 1;
                continue;
            } else {
#endif
                ConstantOnly = FALSE;
                if( id_flag > 0 ) {
                    AsmError( 20 );
                    return( ERROR );
                } else {
                    id_flag++;
                }
#if 1
            }
#endif

            switch( sym->state ) {
            case SYM_UNDEFINED:
                // forward reference
                if( Code->seg_prefix == EMPTY ) {
                    Code->seg_prefix = 0x2e;
                }
                break;
            case SYM_STACK:
                if( base != EMPTY ) {
                    if( base_lock == TRUE ) {
                        // [reg + data][reg + data] is not allowed
                        AsmError( 8 );
                        return( ERROR );
                    } else {
                        index = base;
                    }
                }
                if( Code->use32 ) {
                    base = T_EBP;
                } else {
                    base = T_BP;
                }
                base_lock = TRUE;   // add lock
                /* fall through */
            default:
                if( Code->mem_type == EMPTY ) {
#if 1
                    if( !Modend ) {
#endif
                        if( operator( sym->mem_type, FALSE ) == ERROR ) {
                            return ERROR;
                        }
                        if( operator( T_PTR, FALSE ) == ERROR ) return( ERROR );
#if 1
                    }
#endif
                }
#if 1
                Label_Idx = i;  // used in check_assume()
#endif
                break;
            }
#if 1
            fix_type = FIX_OFF16; /* might be changed to OFF32 later */

            switch( sym->state ) {
            case SYM_GRP:
            case SYM_SEG:
                ConstantOnly = TRUE;
                fix_type = FIX_SEG;
                Code->mem_type = T_WORD;
                Code->info.opnd_type[Opnd_Count] = OP_I16;
                Code->info.opcode |= W_BIT;
                break;
            }

            find_frame( sym );
            fixup = AddFixup( sym, fix_type );
#else
            fixup = AddFixup( sym, FIX_OFF16 );
#endif
//          if( fixup == NULL )  return( ERROR );
            break;
        default:
            AsmError( 23 );
            return( ERROR );
        }
        i++;
    }
#if 1
    if( !ConstantOnly && !Modend )
#else
    if( !ConstantOnly )
#endif
    {
#if 1
#endif
        if( mem2code( ss, index, base ) == ERROR ) {
            return( ERROR );
        }
    }
#if 1
    if( Modend ) {
        if( sym == NULL ) {
            InternalError("..\\c\\asmins.c",721);
        }
        GetAssume( sym, ASSUME_NOTHING );
    }
    if( sym != NULL ) {
        add_frame();
    } else if( field_flag ) {
        if( ConstantOnly ) {
            if( idata( Code->data[Opnd_Count] ) == ERROR ) {
                return( ERROR );
            }
        }
    }
#endif
    if( fixup != NULL && fixup->fix_type == FIX_OFF16 && (Code->use32 ? Code->adrsiz == EMPTY : Code->adrsiz == NOT_EMPTY) ) {
        fixup->fix_type = FIX_OFF32;
    }
    return( i - 1 );
}

/* CF50d4..5391. Actual DWORD unsigned mode rejects negative values;
 * that error check disappeared from the later released source. */
int idata( long value )
/*
  determine the correct data size of immediate operand;
*/
{
    unsigned long       op_type;

    /* note that we now have code DISTANCE ( short / near / far ) and
     *                            mem_type ( byte / word / dword ... )
     */

    switch( Code->mem_type ) {
    case EMPTY:
        switch( Code->distance ) {
        case EMPTY:
            if( Code->info.token == T_PUSH ) { // sigh. another special case
                if( value < SCHAR_MAX  &&  value >= SCHAR_MIN ) {
                    op_type = OP_I8;
                } else if( Code->use32 ) {
                    op_type = OP_I32;
                } else {
                    op_type = OP_I16;
                }
                break;
            }
            // fall through
        case T_FAR:
            if( value > SHRT_MAX  ||  value < SHRT_MIN ) {
                op_type = OP_I32;
            } else if( value > SCHAR_MAX  ||  value < SCHAR_MIN ) {
                op_type = OP_I16;
            } else {
                op_type = OP_I8;
            }
            break;
        case T_NEAR:
            if( !Code->use32 ) {
                op_type = OP_I16;
            } else {
                op_type = OP_I32;
            }
            break;
        case T_SHORT:
            if( value > SCHAR_MAX  ||  value < SCHAR_MIN ) {
                // expect 8-bit but got 16 bit
                AsmError( 68 );
                return( ERROR );
            } else {
                op_type = OP_I8;
            }
            break;
        // no other possibilities
        }
        break;
    case T_BYTE:
        if( !InRange( value, 1 ) ) {
            // expect 8-bit but got 16 bit
            AsmError( 73 );
            return( ERROR );
        } else {
            op_type = OP_I8;
        }
        break;
#if 1
    case T_SBYTE:
        if( value > SCHAR_MAX || value < SCHAR_MIN ) {
            AsmError( 73 );
            return( ERROR );
        } else {
            op_type = OP_I8;
        }
        break;
    case T_SWORD:
        if( value > SHRT_MAX  ||  value < SHRT_MIN ) {
            AsmError( 73 );
            return( ERROR );
        } else if( value > SCHAR_MAX  ||  value < SCHAR_MIN ){
            op_type = OP_I16;
        } else {
            op_type = OP_I8;
        }
        // set w-bit
        Code->info.opcode |= W_BIT;
        break;
#endif
    case T_WORD:
#if 1
        if( Options.sign_value ) {
            if( !InRange( value, 2 ) ) {
                AsmError( 73 );
                return( ERROR );
            } else if( value > UCHAR_MAX ){
                op_type = OP_I16;
            } else {
                op_type = OP_I8;
            }
            // set w-bit
            Code->info.opcode |= W_BIT;
        } else {
#endif
            if( !InRange( value, 2 ) ) {
                AsmError( 73 );
                return( ERROR );
            } else if( value > SCHAR_MAX  ||  value < SCHAR_MIN ){
                op_type = OP_I16;
            } else {
                op_type = OP_I8;
            }
            // set w-bit
            Code->info.opcode |= W_BIT;
#if 1
        }
#endif
        break;
#if 1
   case T_SDWORD:
        if( value > SCHAR_MAX  ||  value < SCHAR_MIN ){
            op_type = OP_I32;
        } else {
            op_type = OP_I8;
        }
        // set w-bit
        Code->info.opcode |= W_BIT;
        break;
#endif
    case T_DWORD:
#if 1
        if( Options.sign_value ) {
            if (value < 0) {
                AsmError(73);
                return ERROR;
            }
            if( value > UCHAR_MAX ) {
                op_type = OP_I32;
            } else {
                op_type = OP_I8;
            }
            // set w-bit
            Code->info.opcode |= W_BIT;
        } else {
#endif
            if( value > SCHAR_MAX  ||  value < SCHAR_MIN ){
                op_type = OP_I32;
            } else {
                op_type = OP_I8;
            }
            // set w-bit
            Code->info.opcode |= W_BIT;
#if 1
        }
#endif
        break;
    }
    Code->info.opnd_type[Opnd_Count] = op_type;
    Code->data[Opnd_Count] = value;
    return( NOT_ERROR );
}

/* CF4cd8..4fa7. The old caller truncates comp_mem to a byte before
 * comparing with EMPTY; preserve that older temp width. */
int mem2code( char ss, int index, int base )
/*
  encode the memory operand to machine code
*/
{
    struct asm_code     *rCode = Code;
    unsigned char       temp;

    if( Opnd_Count == OPND2 ) {
        if( rCode->info.opnd_type[OPND1] & OP_R ) {
            // opnd is register
            // shift the register field to left by 3 bit
            rCode->info.rm_byte |= ( ( rCode->info.rm_byte & BIT_012 ) << 3 );
            // clear the rightmost 3 bit and mod field
            rCode->info.rm_byte &= 0x38;
        }
    }
    // clear mod
    rCode->info.rm_byte &= NOT_BIT_67;
    if( InsFixups[Opnd_Count] != NULL ) {
        rCode->info.rm_byte |= 0x80;
    } else {
        if( rCode->data[Opnd_Count] == 0 ) {
            //rCode->info.rm_byte |= MOD_00;
        } else if( rCode->data[Opnd_Count] > SCHAR_MAX ||
                   rCode->data[Opnd_Count] < SCHAR_MIN ) {
            rCode->info.rm_byte |= 0x80;
        } else {
            rCode->info.rm_byte |= 0x40;
        }
    }
    if( index == EMPTY  &&  base == EMPTY ) {
        // direct memory
        // clear the rightmost 3 bits
        rCode->info.rm_byte &= 0x38;
        if( !rCode->use32 ) {
            if( !InRange( rCode->data[Opnd_Count], 2 ) ) {
                // expect 16-bit but got 32-bit address
                AsmError( 69 );
                return( ERROR );
            }
            // mod = 00, r/m = 110
            rCode->info.rm_byte |= 0x6;
        } else {
            // mod = 00, r/m = 101
            rCode->info.rm_byte |= 0x5;
        }
        // default is DS:[], DS: segment override is not needed
        seg_override( T_DS );
    } else if( index == EMPTY  &&  base != EMPTY ) {
        rCode->info.rm_byte &= ~BIT_012; /* clear out r/m field */
        switch( base ) {
        case T_SI:
            // r/m = 100
            rCode->info.rm_byte |= 0x04;
            // default is DS:[], DS: segment override is not needed
            break;
        case T_DI:
            // r/m = 101
            rCode->info.rm_byte |= 0x05;
            // default is DS:[], DS: segment override is not needed
            break;
        case T_BP:
            if( ( rCode->info.rm_byte & BIT_67 ) == 0 ) {
                // mod = 01, r/m field = 110
                rCode->info.rm_byte |= 0x46;
            } else {
                // r/m field = 110
                rCode->info.rm_byte |= 0x6;
            }
            // default is SS:[], SS: segment override is not needed
            break;
        case T_BX:
            // r/m = 111
            rCode->info.rm_byte |= 0x07;
            // default is DS:[], DS: segment override is not needed
            break;
        case T_EBP:
            if( ( rCode->info.rm_byte & BIT_67 ) == 0 ) {
                // mod = 01, r/m field = 101
                rCode->info.rm_byte |= 0x45;
            } else {
                // r/m field = 101
                rCode->info.rm_byte |= 0x5;
            }
            // default is SS:[], SS: segment override is not needed
            break;
        case T_ESP:
            // r/m = 100
            rCode->info.rm_byte |= 0x4;
            // ss = 00, index = 100 ( no index ), base = 100 ( 0x4 )
            rCode->sib = 0x24;
            // default is SS:[], SS: segment override is not needed
            break;
        default: // for 386 and up
            rCode->info.rm_byte |= Reg386( base );
            // default is DS:[], DS: segment override is not needed
        }
        seg_override( base );
    } else if( index != EMPTY  &&  base == EMPTY ) {
        // mod field is 00
        rCode->info.rm_byte &= 0x38;
        // s-i-b is present ( r/m = 100 )
        rCode->info.rm_byte |= 0x4;
        // scale factor, index, base ( 0x05 => no base reg )
        rCode->sib = ( ss | ( Reg386(index) << 3 ) | 0x05 );
        // default is DS:[], DS: segment override is not needed
        seg_override( T_DS );
    } else {
        rCode->info.rm_byte &= ~BIT_012; /* clear out r/m field */
        // base != EMPTY, index != EMPTY
        switch( index ) {
        case T_BX:
        case T_BP:
            if( ( temp = comp_mem( index, base ) ) != EMPTY ) {
                rCode->info.rm_byte |= temp;
            }
            seg_override( index );
            break;
        case T_SI:
        case T_DI:
            if( ( temp = comp_mem( base, index ) ) != EMPTY ) {
                rCode->info.rm_byte |= temp;
            }
            seg_override( base );
            break;
        case T_ESP:
            AsmError( 18 );
            return( ERROR );
        default:
            if( base < T_EAX ) {
                AsmError( 24 );
                return( ERROR );
            } else if( base == T_EBP ) {
                if( ( rCode->info.rm_byte & BIT_67 ) == 0 ) {
                    rCode->info.rm_byte |= 0x40;
                }
            }
            // s-i-b is present ( r/m = 100 )
            rCode->info.rm_byte |= 0x4;
            rCode->sib = ( ss | (Reg386(index) << 3) | Reg386(base) );
            seg_override( base );
        }
    }
#if 1
    if( AssumeError ) {
        return( ERROR );
    }
#endif
    return( NOT_ERROR );
}

/* CF4b4c..4bb9, original three-bit register encoding. */
int Reg386(int token)
{
    switch (token) {
    case T_EAX: return 0;
    case T_ECX: return 1;
    case T_EDX: return 2;
    case T_EBX: return 3;
    case T_ESP: return 4;
    case T_EBP: return 5;
    case T_ESI: return 6;
    case T_EDI: return 7;
    }
    return 0;
}

/* CF3f34..404a. The override branch changes default_seg to an ASSUME
 * index before the final comparison, as in the shipped/source body. */
void seg_override(int seg_reg)
{
    int default_seg;
    asm_sym *sym;
    switch (seg_reg) {
    case T_SS: case T_BP: case T_EBP: case T_ESP:
        default_seg = 0x36;
        break;
    default: default_seg = 0x3e;
    }
    if (Code->seg_prefix == EMPTY) {
        if (Label_Idx != -1) check_assume(default_seg);
    } else if (Label_Idx != -1) {
        sym = AsmLookup(AsmBuffer[Label_Idx]->string_ptr);
        if (sym == NULL) InternalError("..\\c\\asmins.c",223);
        switch (Code->seg_prefix) {
        case 0x26: default_seg = ASSUME_ES; break;
        case 0x2e: default_seg = ASSUME_CS; break;
        case 0x36: default_seg = ASSUME_SS; break;
        case 0x3e: default_seg = ASSUME_DS; break;
        case 0x64: default_seg = ASSUME_FS; break;
        case 0x65: default_seg = ASSUME_GS; break;
        }
        if (GetPrefixAssume(sym,default_seg) == ASSUME_NOTHING)
            AsmWarn(3,1041);
    }
    if (Code->seg_prefix == default_seg) Code->seg_prefix = EMPTY;
}

/* CF4ca8..4cd8, accept unsigned magnitude or a valid sign extension. */
int InRange(unsigned long value, unsigned bytes)
{
    unsigned long max, mask;
    max = (1UL << (bytes * 8)) - 1;
    if (value <= max) return 1;
    mask = ~(max >> 1);
    if ((value & mask) == mask) return 1;
    return 0;
}

/* CF4064..4162, six-way ASSUME prefix dispatch at404c..4064. */
void check_assume( int default_reg )
/* Check if an assumed register is found, and prefix a register if necessary */
{
    struct asm_sym      *sym;
    unsigned            reg;
    unsigned            def_reg;

    sym = AsmLookup( AsmBuffer[Label_Idx]->string_ptr );
    if (sym == NULL) InternalError("..\\c\\asmins.c",282);

    if( sym->state == SYM_UNDEFINED ) return;

    switch( default_reg ) {
        case 0x36:
            def_reg = ASSUME_SS;
            break;
        case 0x3e:
            def_reg = ASSUME_DS;
            break;
        case EMPTY:
            def_reg = ASSUME_NOTHING;
            break;
    }

    reg = GetAssume( sym, def_reg );

    if( reg == ASSUME_NOTHING ) {
        if( sym->state != SYM_EXTERNAL && sym->state != SYM_PROC ) {
            // AsmError( 1041 );
            // AssumeError = TRUE;
            AsmWarn( 3, 1041 );
        } else {
            Code->seg_prefix = default_reg;
        }
    } else {
        switch( reg ) {
            case ASSUME_ES:
                Code->seg_prefix = 0x26;
                break;
            case ASSUME_CS:
                Code->seg_prefix = 0x2e;
                break;
            case ASSUME_DS:
                Code->seg_prefix = 0x3e;
                break;
            case ASSUME_GS:
                Code->seg_prefix = 0x65;
                break;
            case ASSUME_FS:
                Code->seg_prefix = 0x64;
                break;
            case ASSUME_SS:
                Code->seg_prefix = 0x36;
                break;
        }
    }
}
