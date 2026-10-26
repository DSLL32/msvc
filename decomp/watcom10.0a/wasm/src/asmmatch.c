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


/* The file-local original output is exposed as asmmatch_output to avoid
 * the independent diagnostic output function.
 * match_phase_1 CF9be4..9f8e, no later MMX branch.
 * CPU/operand fields are admitted only for encountered symbolic table rows. */
#include <limits.h>
#include "asmparse.h"
#include <recon.h>

/* CF9730..9a17. Unlike2002 this always emits OPSIZ for opsiz, has
 * explicit JCXZ/JECXZ address-size tests, and has no MMX cases.
 * The inlined addr_32 test retains the original absence of a call edge. */
int asmmatch_output(int i)
{
    const struct asm_ins *ins = &AsmOpTable[i];
    struct asm_code *rCode = Code;
    unsigned char tmp;

    /* Only encoding families consumed on the reconstructed instruction path. */
    switch (ins->token) {
    case T_PUSH: case T_MOV: case T_AND: case T_XOR: case T_ROL: case T_ADD: case T_SUB: case T_CMP:
    case T_MOVZX: case T_SHLD: case T_BT: case T_ADC: case T_XCHG:
    case T_INC: case T_DEC: case T_NEG: case T_JNZ: case T_JGE:
    case T_NOP: case T_FWAIT: case T_FLDZ: case T_FILD: case T_FMUL: case T_FADDP: case T_LOOP:
    case T_FSTP: case T_IMUL: case T_IDIV: case T_TEST: case T_JZ:
    case T_JMP: case T_CALLF: case T_CALL: case T_POP: case T_RETN: case T_RETF: case T_RET: case T_INT: case T_JECXZ:
        break;
    default: WASM_PENDING();
    }
    if (Options.debug_flag) AddLinnumData();
    if (Options.floating_point == 0 && !rCode->use32 &&
        (ins->allowed_prefix == FWAIT || (ins->cpu & P_FPU_MASK) != P_NO87)) {
        if (AddFloatingPointEmulationFixup(ins,FALSE) == ERROR) return ERROR;
    }
    if ((ins->cpu & P_CPU_MASK) > (rCode->info.cpu & P_CPU_MASK) ||
        (ins->cpu & P_FPU_MASK) > (rCode->info.cpu & P_FPU_MASK)) {
        AsmError(3);
        return ERROR;
    }
    switch (ins->byte1_info) {
    case F_16: if (rCode->use32) rCode->opsiz = NOT_EMPTY; break;
    case F_32: if (!rCode->use32) rCode->opsiz = NOT_EMPTY; break;
    case F_0F: rCode->extended_ins = NOT_EMPTY; break;
    }
    if (ins->token == T_JCXZ && rCode->use32) rCode->adrsiz = NOT_EMPTY;
    if (ins->token == T_JECXZ && !rCode->use32) rCode->adrsiz = NOT_EMPTY;
    if (rCode->prefix != EMPTY) {
        if (rCode->prefix == T_LOCK) {
            if (ins->allowed_prefix != LOCK) { AsmError(4); return ERROR; }
        } else {
            if (ins->allowed_prefix != REPxx) { AsmError(5); return ERROR; }
        }
        /* Prefix instruction rows have not yet supplied their opcode. */
        WASM_PENDING();
        AsmCodeByte(AsmOpTable[AsmOpcode[rCode->prefix].position].opcode);
    }
    if (ins->token == T_FWAIT &&
        (rCode->info.cpu & P_CPU_MASK) < P_386 &&
        (ins->cpu & P_FPU_MASK) == P_87) {
        if (Options.floating_point == 0) AsmCodeByte(OP_NOP);
    } else if (ins->allowed_prefix == FWAIT ||
        ((rCode->info.cpu & P_CPU_MASK) < P_386 &&
         (ins->cpu & P_FPU_MASK) != P_NO87)) {
        AsmCodeByte(OP_WAIT);
    }
    if (Options.floating_point == 0 &&
        (ins->allowed_prefix == FWAIT ||
         ((rCode->info.cpu & P_CPU_MASK) < P_386 &&
          (ins->cpu & P_FPU_MASK) != P_NO87))) {
        if (AddFloatingPointEmulationFixup(ins,TRUE) == ERROR) return ERROR;
    }
    if (rCode->adrsiz != EMPTY) AsmCodeByte(ADRSIZ);
    if (rCode->opsiz != EMPTY) AsmCodeByte(OPSIZ);
    if (rCode->seg_prefix != EMPTY) AsmCodeByte(rCode->seg_prefix);
    if (rCode->extended_ins != EMPTY) AsmCodeByte(EXTENDED_OPCODE);
    switch (ins->rm_info) {
    case R_in_OP:
        AsmCodeByte(ins->opcode | (rCode->info.rm_byte & NOT_BIT_67));
        break;
    case no_RM:
        AsmCodeByte(ins->opcode | rCode->info.opcode);
        break;
    case no_WDS:
        rCode->info.opcode = 0;
    default:
        AsmCodeByte(ins->opcode | rCode->info.opcode);
        tmp = ins->rm_byte | rCode->info.rm_byte;
        AsmCodeByte(tmp);
        if (rCode->use32 ? rCode->adrsiz == EMPTY : rCode->adrsiz == NOT_EMPTY) {
            switch (tmp & NOT_BIT_345) {
            case 0x04:
            case 0x44:
            case 0x84: AsmCodeByte(rCode->sib);
            }
        }
        break;
    }
    return NOT_ERROR;
}

int match_phase_1( void )
/*
- this routine will look up the assembler opcode table and try to match
  the first operand in table with what we get;
- if first operand match then it will call match_phase_2() to determine if the
  second operand also match; if not, it must be error;
*/
{
    int                 i;
    int                 retcode;
    signed char         temp_opsiz = 0;
    unsigned long       cur_opnd;
    unsigned long       asm_op1;

    // if nothing inside, no need to output anything
    if( Code->info.token == T_NULL ) {
        if( Code->seg_prefix != EMPTY ) {
            /* we have:     REG: on line */
            AsmError( 51 );
            return( ERROR );
        } else if( Code->info.opnd_type[OPND1] == OP_NONE ) {
            return( NOT_ERROR );
        } else {
            AsmError( 51 );
            return( ERROR );
        }
    }
    switch( Code->info.token ) {
    case T_LIDT:
    case T_LGDT:
        /* kludge fix to allow "LIDT fword ptr foo" */
        switch( Code->mem_type ) {
        case T_PWORD:
        case T_FWORD:
            Code->info.opnd_type[OPND1] = OP_M_DW;
            break;
        }
        break;
    }

    // look up the hash table to get the real position of instruction
    i = AsmOpcode[Code->info.token].position;
    /* Unrecovered instruction matching rows must deny both builds. */
    if (AsmOpTable[i].opnd_type[0] == OP_NONE &&
        Code->info.token != T_FLDZ && Code->info.token != T_FMUL &&
        Code->info.token != T_RETN && Code->info.token != T_RET &&
        Code->info.token != T_FWAIT && Code->info.token != T_RETF &&
        Code->info.token != T_NOP)
        WASM_PENDING();

    // make sure the user want 80x87 ins
    if( ( AsmOpTable[i].cpu & P_FPU_MASK ) != P_NO87 ) {
        if( ( Code->info.cpu & P_FPU_MASK ) == P_NO87 ) {
            AsmError( 3 );
            return( ERROR );
        } else {
            Code->opsiz = EMPTY;
        }
    }

    // make sure the user want 80x86 protected mode ins
    if( ( AsmOpTable[i].cpu & P_PM ) != 0 ) {
        if( ( Code->info.cpu & P_PM ) == 0 ) {
            AsmError( 3 );
            return( ERROR );
        }
    }

    // fixme
    // moved down 4 lines cur_opnd = Code->info.opnd_type[OPND1];
    while( AsmOpTable[i].token == Code->info.token ) {
        // get the real opnd
        asm_op1 = AsmOpTable[i].opnd_type[OPND1];
        cur_opnd = Code->info.opnd_type[OPND1];

        /* fixme -- temporary fix for all OP_M types */
        // if( cur_opnd & OP_M ) { I think there is a problem here
        if( cur_opnd & OP_M_ANY ) {
            if( ! ( cur_opnd & asm_op1 ) ) {
                if( MEM_TYPE( Code->mem_type, BYTE ) ) {
                    cur_opnd = OP_M_B;
                } else if( MEM_TYPE( Code->mem_type, WORD ) ) {
                    cur_opnd = OP_M_W;
                } else if( MEM_TYPE( Code->mem_type, DWORD ) ) {
                    cur_opnd = OP_M_DW;
                } else if( Code->mem_type == T_QWORD ) {
                    cur_opnd = OP_M_QW;
                } else if( Code->mem_type == T_TBYTE ) {
                    cur_opnd = OP_M_TB;
                }
            }
        }

        /* fixme -- temporary fix for all OP_I types */
        if( cur_opnd & OP_I ) {
            if( cur_opnd == OP_I8 ) {
                cur_opnd = OP_GE_8;
            } else if( cur_opnd == OP_I16 ) {
                cur_opnd = OP_GE_16;
            }
        }

        switch( asm_op1 ) {
        case OP_R16:
            if( cur_opnd & asm_op1 ) {
                temp_opsiz = Code->opsiz;
                Code->opsiz = EMPTY;
                switch( match_phase_2( &i ) ) {
                case EMPTY:
                    Code->opsiz = temp_opsiz;
                    break;
                case ERROR:
                    return( ERROR );
                case NOT_ERROR:
                    return( NOT_ERROR );
                }
            }
            break;
        case OP_M16:
            if( cur_opnd & asm_op1 ) {
                temp_opsiz = Code->opsiz;
                switch( match_phase_2( &i ) ) {
                case EMPTY:
                    Code->opsiz = temp_opsiz;
                    break;
                case ERROR:
                    return( ERROR );
                case NOT_ERROR:
                    return( NOT_ERROR );
                }
            }
            break;
        case OP_I32:
        case OP_I16:
            if( cur_opnd & asm_op1 ) {
                Code->info.opnd_type[OPND1] = asm_op1;
                switch( match_phase_2( &i ) ) {
                case EMPTY:
                    break;
                case ERROR:
                    return( ERROR );
                case NOT_ERROR:
                    return( NOT_ERROR );
                }
            }
            break;
        case OP_I8_U:
            if( cur_opnd & OP_I ) {
                if( Code->data[OPND1] <= UCHAR_MAX ) {
                    temp_opsiz = Code->opsiz;
                    Code->info.opnd_type[OPND1] = OP_I8;
                    switch( match_phase_2( &i ) ) {
                    case EMPTY:
                        Code->opsiz = temp_opsiz;
                        break;
                    case ERROR:
                        return( ERROR );
                    case NOT_ERROR:
                        return( NOT_ERROR );
                    }
                }
            }
            break;
        case OP_I_3:                    // for INT only
            if( ( ( cur_opnd & OP_I8 ) && Code->data[0] == 3 ) &&
                                Code->info.opnd_type[OPND2] == OP_NONE ) {
                return( asmmatch_output( i ) );
            }
            break;
        case OP_NONE:
            if( cur_opnd == OP_NONE &&
                    Code->info.opnd_type[OPND2] == OP_NONE ) {
                return( asmmatch_output( i ) );
            }
            break;
        default:
            /* this is the check to see if the operand we have matches
             * the type of the one we need
             */
            if( asm_op1 & cur_opnd ) {
                retcode = match_phase_2( &i );
                if( retcode != EMPTY ) {
                    return( retcode );
                }
            }
            break;
        }
        i++;
    }
    AsmError( 65 );
    return( ERROR );
}


/* CF9f90..9feb, same two-operand/one-operand dispatch as released source. */
int match_phase_2(int *index)
{
    if (Code->info.opnd_type[OPND2] != OP_NONE)
        return match_phase_3(index, AsmOpTable[*index].opnd_type[OPND1]);
    if (AsmOpTable[*index].opnd_type[OPND2] == OP_NONE) {
        if (asmmatch_output(*index) == ERROR) return ERROR;
        return output_data(Code->info.opnd_type[OPND1], OPND1);
    }
    return EMPTY;
}

/* CF9a18..9be1. Both address-size decisions are inlined in1994. */
int output_data(unsigned long determinant, int index)
{
    int out = 0;
    mark_fixupp(determinant,index);
    switch (Code->info.token) {
    case T_CMPS: case T_LODS: case T_MOVS: case T_OUTS:
    case T_INS2: case T_SCAS: case T_STOS: case T_XLAT:
        return NOT_ERROR;
    }
    store_fixup(index);
    if (determinant & OP_I8) out = 1;
    else if (determinant & OP_I16) out = 2;
    else if (determinant & (OP_I32 | OP_J32)) out = 4;
    else if (determinant & OP_J48) out = 6;
    else if (determinant == OP_M16_IND || determinant == OP_M32_IND ||
             determinant == OP_M48_IND) out = Code->use32 ? 4 : 2;
    else if (determinant & OP_M_ANY) {
        switch (Code->info.rm_byte & BIT_67) {
        case 0x40: out = 1; break;
        case 0:
            if (!(Code->use32 ? Code->adrsiz == EMPTY : Code->adrsiz == NOT_EMPTY)) {
                if ((Code->info.rm_byte & BIT_012) == 6) out = 2;
            } else {
                switch (Code->info.rm_byte & BIT_012) {
                case 4:
                    if ((Code->sib & BIT_012) != 5) break;
                case 5: out = 4;
                }
            }
            break;
        case 0x80:
            out = (Code->use32 ? Code->adrsiz == EMPTY : Code->adrsiz == NOT_EMPTY) ? 4 : 2;
            break;
        }
    }
    while (out > 0) {
        AsmCodeByte(Code->data[index]);
        Code->data[index] >>= 8;
        --out;
    }
    return NOT_ERROR;
}

/* CF9fec..a55e:1994 has no MMX and rejects oversized register
 * immediates with AsmError66, not the later pass1 warning. */
int match_phase_3( int *i, unsigned long determinant )
/*
- this routine will look up the assembler opcode table and try to match
  the second operand with what we get;
- if second operand match then it will output code; if not, pass back to
  match_phase_1() and try again;
- call by match_phase_2() only;
*/
{
    unsigned long       cur_opnd;
    unsigned long       last_opnd;
    unsigned long       asm_op2;
    unsigned            instruction;

    instruction = AsmOpTable[*i].token;

    last_opnd = Code->info.opnd_type[OPND1];
    cur_opnd  = Code->info.opnd_type[OPND2];

    while( AsmOpTable[*i].opnd_type[OPND1] == determinant &&
           AsmOpTable[*i].token == instruction ) {
        asm_op2 = AsmOpTable[*i].opnd_type[OPND2];
        switch( asm_op2 ) {
        case OP_CR:
        case OP_DR:
        case OP_TR:
        case OP_ST:
        case OP_STI:
            if( cur_opnd & asm_op2 ) {
                if( asmmatch_output( *i ) == ERROR ) return( ERROR );
                return( NOT_ERROR );
            }
            break;
        case OP_SR:
        case OP_DX:
        case OP_A:
        case OP_R:
        case OP_R1632:
        case OP_R32:
            if( cur_opnd & asm_op2 ) {
                if( asmmatch_output( *i ) == ERROR ) return( ERROR );
                return( output_data( last_opnd, OPND1 ) );
            }
            break;
        case OP_CL:
            if( cur_opnd & asm_op2 ) {
                // CL is encoded in bit 345 of rm_byte, but we don't need it
                // so clear it here
                Code->info.rm_byte &= NOT_BIT_345;
                if( asmmatch_output( *i ) == ERROR ) return( ERROR );
                return( output_data( last_opnd, OPND1 ) );
            }
            break;
        case OP_R16:
            if( cur_opnd & asm_op2 ) {
                Code->opsiz = EMPTY;
                if( asmmatch_output( *i ) == ERROR ) return( ERROR );
                return( output_data( last_opnd, OPND1 ) );
            }
            break;
        case OP_I:
            if( cur_opnd & asm_op2 ) {
                long operand = Code->data[OPND2];
                if( last_opnd & OP_R8 ) {
                    // 8-bit register, so output 8-bit data
                    if (!InRange(operand,1)) {
                        AsmError(66);
                        return ERROR;
                    }
                    Code->opsiz = EMPTY;
                    cur_opnd = OP_I8;
                } else if( last_opnd & OP_R16 ) {
                    // 16-bit register, so output 16-bit data
                    if (!InRange(operand,2)) {
                        AsmError(66);
                        return ERROR;
                    }
                    cur_opnd = OP_I16;
                } else if( last_opnd & OP_R32 ) {
                    // 32-bit register, so output 32-bit data
                    Code->opsiz = Code->use32 ? EMPTY : NOT_EMPTY;/* 12-feb-92 */
                    cur_opnd = OP_I32;
                } else if( last_opnd & OP_M_ANY ) {
                    /* there is no reason this should be only for T_MOV */
                    switch( OperandSize( last_opnd ) ) {
                    case 1:
                        cur_opnd = OP_I8;
                        Code->opsiz = EMPTY;
                        break;
                    case 2:
                        cur_opnd = OP_I16;
                        Code->opsiz = Code->use32 ? NOT_EMPTY : EMPTY;
                        break;
                    case 4:
                        cur_opnd = OP_I32;
                        Code->opsiz = Code->use32 ? EMPTY : NOT_EMPTY;
                        break;
                    }
                }
                if( asmmatch_output( *i ) == ERROR ) {
                    return( ERROR );
                }
                if( output_data( last_opnd, OPND1 ) == ERROR ) return( ERROR );
                return( output_data( cur_opnd, OPND2 ) );
            }
            break;
        case OP_I8_U:
            if( cur_opnd != OP_I8 && cur_opnd != OP_I16 ) {
                break;
            }
            // range of unsigned 8-bit is 0 - 255
            if( Code->data[OPND2] > UCHAR_MAX ) {
                break;
            }
            if( asmmatch_output( *i ) == ERROR ) {
                return( ERROR );
            }
            if( output_data( last_opnd, OPND1 ) == ERROR ) return( ERROR );
            return( output_data( OP_I8, OPND2 ) );
            break;
        case OP_I8:
            if( cur_opnd == asm_op2 ) {
                /* do nothing yet */
            } else if( ( last_opnd & OP_M16_R16 ) && ( cur_opnd & OP_I ) &&
                ( MEM_TYPE( Code->mem_type, WORD ) ) &&
                InsFixups[OPND2] == NULL ) {
                if( (signed char)Code->data[OPND2] ==
                    (short)Code->data[OPND2] ) {
                    Code->info.opnd_type[OPND2] = OP_I8;
                    Code->data[OPND2] = (signed char)Code->data[OPND2];
                } else {
                    break;
                }
            } else if( ( last_opnd & OP_M32_R32 ) && ( cur_opnd & OP_I ) &&
                ( MEM_TYPE( Code->mem_type, DWORD ) ) &&
                InsFixups[OPND2] == NULL ) {
                if( (signed char)Code->data[OPND2] ==
                    (long)Code->data[OPND2] ) {
                    Code->info.opnd_type[OPND2] = OP_I8;
                    Code->data[OPND2] = (signed char)Code->data[OPND2];
                } else {
                    break;
                }
            } else {
                break;
            }
            if( asmmatch_output( *i ) == ERROR ) return( ERROR );
            if( output_data( last_opnd, OPND1 ) == ERROR ) return( ERROR );
            return( output_data( OP_I8, OPND2 ) );
        case OP_I_1:
            if( cur_opnd == OP_I8  &&  Code->data[OPND2] == 1 ) {
                if( asmmatch_output( *i ) == ERROR ) return( ERROR );
                return( output_data( last_opnd, OPND1 ) );
            }
            break;
        case OP_M16:
            if( cur_opnd & OP_M &&
                   ( MEM_TYPE( Code->mem_type, WORD ) ||
                     Code->mem_type == EMPTY ) ) {
                if( asmmatch_output( *i ) == ERROR ) return( ERROR );
                if( output_data( last_opnd, OPND1 ) == ERROR ) return( ERROR );
                return( output_data( cur_opnd, OPND2 ) );
            }
            break;
        case OP_M:
            if( cur_opnd & asm_op2 ) {
                if( asmmatch_output( *i ) == ERROR ) return( ERROR );
                if( output_data( last_opnd, OPND1 ) == ERROR ) return( ERROR );
                return( output_data( cur_opnd, OPND2 ) );
            }
            break;
        }
        (*i)++;
    }
    (*i)--;
    return( EMPTY );
}

/* CF25fa0: the line-number queue consumed by the object writer. */
qdesc *LinnumQueue;

/* CFa560..a5cd. The1994 condition is nonzero Parse_Pass && debug_flag;
 * the later line-number-range test is absent. */
void AddLinnumData( void )
{
    struct linnum_data *curr;
    queuenode *node;
    if( Parse_Pass != PASS_1 && Options.debug_flag ) {
        curr = AsmAlloc( sizeof(struct linnum_data) );
        curr->number = LineNumber;
        curr->offset = GetCurrAddr();
        node = AsmAlloc( sizeof(queuenode) );
        node->data = curr;
        if( LinnumQueue == NULL ) {
            LinnumQueue = AsmAlloc( sizeof(qdesc) );
            QInit( LinnumQueue );
        }
        QEnqueue( LinnumQueue, node );
    }
}
