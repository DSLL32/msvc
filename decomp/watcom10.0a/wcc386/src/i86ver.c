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
* Description:  Verify 386 instruction-selection conditions.
*
****************************************************************************/


#include "block.h"
#include "stubs.h"
#include <recon.h>

/* Original selector numbering is unchanged through all 57 values in the
 * initial released intel/h/v.h. The 1994 OSI variant has the explicitly
 * noted older CYP2SHIFT and INTCOMP cases below. */
enum verify_kind {
    V_NO,
    V_80186,
    V_CMPEQ,
    V_DIFF_TYPES,
    V_HIGHEQLOW,
    V_OFFSETZERO,
    V_OP1ADDR,
    V_OP1LOC,
    V_OP1ONE,
    V_OP1ZERO,
    V_OP2HIGH_B_ZERO,
    V_OP2LOW_B_FF,
    V_OP2LOW_B_ZERO,
    V_OP2LOW_W_FFFF,
    V_OP2NEG,
    V_OP2ONE,
    V_OP2TWO,
    V_OP2ZERO,
    V_RESLOC,
    V_SAME_LOCN,
    V_BYTESHIFT,
    V_CYP2SHIFT,
    V_CYP4SHIFT,
    V_OP2HIGH_B_FF,
    V_OP2HIGH_W_FFFF,
    V_OP2HIGH_W_ZERO,
    V_OP2LOW_W_ZERO,
    V_U_TEST,
    V_CMPTRUE,
    V_CMPFALSE,
    V_OP2PTR,
    V_SPEED,
    V_SIZE,
    V_DIV_BUG,
    V_OP1SP,
    V_WORDREG_AVAIL,
    V_LEA,
    V_SWAP_GOOD,
    V_OP1RELOC,
    V_AC_BETTER,
    V_OP2_FFFFFFFF,
    V_LSHONE,
    V_SAME_TYPE,
    V_OP2POW2,
    V_OP2POW2_286,
    V_OP2TWO_SIZE,
    V_GOOD_CLR,
    V_LEA_GOOD,
    V_P5_FXCH,
    V_CYP_SEX,
    V_OP2HIGH_W_FFFF_REG,
    V_OP2HIGH_W_ZERO_REG,
    V_NEGATE,
    V_CONSTTEMP,
    V_INTCOMP,
    V_80386,
    V_SWAP_GOOD_386
};
#define TRUE 1
#define FALSE 0
#define CONS_ABSOLUTE 0
#define N_MEMORY 1
#define N_TEMP 2
#define N_REGISTER 3
#define N_INDEXED 4
#define CONST_TEMP 0x100
#define CPU_186 1
#define CPU_286 2
#define CPU_386 3
#define CPU_486 4
#define CPU_586 5
#define _CPULevel(cpu) ((CGProcessorVersion&15)>=(cpu))
#define INDEXED_GLOBALS 0x1000U
#define _IsTargetModel(bit) (TargetModel&(bit))
#define HW_BP 0x400U
#define HW_SP 0x800U
#define HW_AX 3U
#define HW_BX 12U
#define HW_CX 48U
#define HW_DX 192U
#define HW_SI 256U
#define HW_DI 512U
#define HW_ABCD 255U
#define HW_CEqual(a,b) ((a)==(b))
#define HW_COvlap(a,b) ((a)&(b))
#define HW_Ovlap(a,b) ((a)&(b))
#define WORD_SIZE 4
#define OP_ADD 1
#define OP_SUB 3
#define OP_MUL 5
#define OP_LSHIFT 13
#define OP_NEGATE 17

unsigned char OtherVerify(unsigned kind,instruction *ins,name *op1,name *op2,name *result);
int NumOperands(instruction *ins);

unsigned char DoVerify( unsigned kind, instruction *ins ) {
/**********************************************************/

    name        *op1=NULL;
    name        *op2=NULL;
    name        *result;

    result = ins->result;
    if( ins->num_operands != 0 ) {
        op1 = ins->operands[ 0 ];
        if( ins->num_operands != 1 ) {
            op2 = ins->operands[ 1 ];
        }
    }
    switch( kind ) {
    case V_80186:
         if( _CPULevel( CPU_186 ) ) return( TRUE );
         break;
    case V_80386:
         if( _CPULevel( CPU_386 ) ) return( TRUE );
         break;
    case V_DIFF_TYPES:
        if( op1->n.name_class != result->n.name_class ) return( TRUE );
        break;
    case V_HIGHEQLOW:
        if( op1->c.const_type != CONS_ABSOLUTE ) return( FALSE );
        if(!CFIsI32( op1->c.value ) && !CFIsU32( op1->c.value )) return(FALSE);
        if( ( op1->c.int_value & 0xffff0000 )
         == ( op1->c.int_value << 16 ) ) return( TRUE );
        break;
    case V_OP2HIGH_B_ZERO:
        if( op2->c.const_type == CONS_ABSOLUTE
         && ( op2->c.int_value & 0x0000ff00 ) == 0 ) return( TRUE );
        break;
    case V_OP2LOW_B_FF:
        if( op2->c.const_type == CONS_ABSOLUTE
         && ( ~op2->c.int_value & 0x000000ff ) == 0 ) return( TRUE );
        break;
    case V_OP2LOW_B_ZERO:
        if( op2->c.const_type == CONS_ABSOLUTE
         && ( op2->c.int_value & 0x000000ff ) == 0 ) return( TRUE );
        break;
    case V_OP2LOW_W_FFFF:
        if( op2->c.const_type == CONS_ABSOLUTE
         && ( ~op2->c.int_value & 0x0000ffff ) == 0 ) return( TRUE );
        break;
    case V_LSHONE:
        if( !_CPULevel( CPU_286 ) ) return( FALSE );
        if( ins->head.opcode != OP_LSHIFT ) return( FALSE );
        return( OtherVerify( V_OP2ONE, ins, op1, op2, result ) );
    case V_OP2TWO_SIZE:
        if( OptForSize <= 50 ) return( FALSE );
        return( OtherVerify( V_OP2TWO, ins, op1, op2, result ) );
    case V_BYTESHIFT:
        if( ins->head.opcode != OP_LSHIFT
         && ins->type_class != U4 ) return( FALSE );
        if( op2->c.const_type != CONS_ABSOLUTE ) return( FALSE );
        if( op2->c.int_value != 8 ) return( FALSE );
        if( op1->n.class == N_REGISTER
         && !HW_CEqual( op1->r.reg, HW_ABCD ) ) return( FALSE );
        if( result->n.class == N_REGISTER
         && !HW_CEqual( result->r.reg, HW_ABCD ) ) return( FALSE );
        return( TRUE );
    case V_CYP2SHIFT:
        return op2->c.const_type==CONS_ABSOLUTE && op2->c.int_value==8;
    case V_CYP4SHIFT:
        if( op2->c.const_type != CONS_ABSOLUTE ) return( FALSE );
        if( op2->c.int_value < 16 ) return( FALSE );
        return( TRUE );
    case V_OP2HIGH_B_FF:
        if( op2->c.const_type == CONS_ABSOLUTE
         && ( op2->c.int_value & 0x0000ff00 ) == 0x0000ff00 ) return( TRUE );
        break;
    case V_OP2HIGH_W_FFFF_REG:
        if( op1->n.class == N_REGISTER && HW_Ovlap( op1->r.reg, HW_BP ) ) break;
    case V_OP2HIGH_W_FFFF:
        if( op2->c.const_type == CONS_ABSOLUTE
         && ( op2->c.int_value & 0xffff0000 ) == 0xffff0000 ) return( TRUE );
        break;
    case V_OP2HIGH_W_ZERO_REG:
        if( op1->n.class == N_REGISTER && HW_Ovlap( op1->r.reg, HW_BP ) ) break;
    case V_OP2HIGH_W_ZERO:
        if( op2->c.const_type == CONS_ABSOLUTE
         && ( op2->c.int_value & 0xffff0000 ) == 0 ) return( TRUE );
        break;
    case V_OP2LOW_W_ZERO:
        if( op2->c.const_type == CONS_ABSOLUTE
         && ( op2->c.int_value & 0x0000ffff ) == 0 ) return( TRUE );
        break;
    case V_OP2PTR:
        if( op2->n.name_class == PT || op2->n.name_class == CP ) return( TRUE );
        break;
    case V_LEA_GOOD:
        if( OptForSize > 50 ) return( FALSE );
        /* fall through */
    case V_LEA:
        if( op2->c.const_type != CONS_ABSOLUTE ) return( FALSE );
        switch( ins->head.opcode ) {
        
            case OP_MUL:
                switch( op2->c.int_value ) {
                case 3:
                case 5:
                case 9:
                    return( TRUE );
                }
                break;
            case OP_LSHIFT:
                if( op1 == result && _CPULevel( CPU_586 ) ) return( FALSE );
                if( OptForSize >= 50 ) return( FALSE );
                switch( op2->c.int_value ) {
                case 1:
                case 2:
                case 3:
                    return( TRUE );
                }
                break;

        case OP_ADD:
        case OP_SUB:
            if( OptForSize < 50 && !_CPULevel( CPU_286 ) ) return( FALSE );
            return( TRUE );
        }
        break;
    case V_SIZE:
        if( OptForSize > 50 ) return( TRUE );
        break;
    case V_DIV_BUG: /* cant do idiv from mem on 80186 (or 8086 for compatibility)*/
        if( !_CPULevel( CPU_286 )
             && ins->type_class != U1 && ins->type_class != U2 ) return( TRUE );
        break;
    case V_OP1SP:
        if( HW_CEqual( op1->r.reg, HW_SP ) ) return( TRUE );
        break;
    case V_WORDREG_AVAIL:
        if( !HW_COvlap( ins->head.live.regs, HW_AX ) ) return( TRUE );
        if( !HW_COvlap( ins->head.live.regs, HW_BX ) ) return( TRUE );
        if( !HW_COvlap( ins->head.live.regs, HW_CX ) ) return( TRUE );
        if( !HW_COvlap( ins->head.live.regs, HW_DX ) ) return( TRUE );
        if( !HW_COvlap( ins->head.live.regs, HW_SI ) ) return( TRUE );
        if( !HW_COvlap( ins->head.live.regs, HW_DI ) ) return( TRUE );
        break;
    case V_AC_BETTER:
        if( op2->c.const_type != CONS_ABSOLUTE ) return( TRUE );
        if( op2->c.int_value > 127 ) return( TRUE );
        if( op2->c.int_value < -128 ) return( TRUE );
        break;
    case V_OP2POW2_286:
        if( !_CPULevel( CPU_286 ) ) return( FALSE );
        return( OtherVerify( V_OP2POW2, ins, op1, op2, result ) );
    case V_OP1ADDR:
    case V_OP1LOC:
        if( _IsTargetModel( INDEXED_GLOBALS ) ) return( FALSE );
        if( op1->n.class == N_MEMORY ) return( TRUE );
        break;
    case V_RESLOC:
        if( _IsTargetModel( INDEXED_GLOBALS ) ) return( FALSE );
        if( result->n.class == N_MEMORY ) return( TRUE );
        break;
    case V_GOOD_CLR:
        if( op1 == result ) return( TRUE );
        if( !_CPULevel( CPU_486 ) ) break;
        if( OptForSize > 50 ) break;
        if( result->n.class == N_REGISTER ) {
            if( HW_Ovlap( result->r.reg, HW_BP ) ) break;
        }
        return( TRUE );
    case V_P5_FXCH:
        if( !_CPULevel( CPU_586 ) ) return( FALSE );
        if( OptForSize <= 50 ) return( TRUE );
        break;
    case V_CYP_SEX:
        if( !_CPULevel( CPU_586 ) ) return( FALSE );
        if( OptForSize > 50 ) return( FALSE );
        op1 = ins->operands[0];
        {
            long backup;
            long new_off;

            backup = WORD_SIZE - op1->n.size;
            switch( op1->n.class ) {
            case N_TEMP:
                return( TRUE );
            case N_MEMORY:
                new_off = op1->v.offset - backup;
                if( new_off >= 0 ) return( TRUE );
                if( ins->num_operands > NumOperands( ins ) ) return( FALSE );
                return( TRUE );
            case N_INDEXED:
                new_off = op1->i.constant - backup;
                if( new_off >= 0 ) return( TRUE );
                if( ins->num_operands > NumOperands( ins ) ) return( FALSE );
                if( op1->i.base == NULL ) return( FALSE );
                return( TRUE );
            }
        }
        break;
    case V_NEGATE:
         if( ins->head.opcode == OP_NEGATE ) return( TRUE );
         break;
    case V_CONSTTEMP:
        {
            int                 i;

            for( i = ins->num_operands-1; i >= 0; --i ) {
                op1 = ins->operands[i];
                if( op1->n.class == N_TEMP
                    && (op1->t.temp_flags & CONST_TEMP ) ) return( TRUE );
            }
            return( FALSE );
        }
    case V_SWAP_GOOD_386:
        if( !_CPULevel( CPU_386 ) ) return( FALSE );
        return( OtherVerify( V_SWAP_GOOD, ins, op1, op2, result ) );
    case V_INTCOMP:
        if (result!=NULL && ins->type_class!=FS) return FALSE;
        return OtherVerify(V_CMPEQ,ins,op1,op2,result);
    default:
        return( OtherVerify( kind, ins, op1, op2, result ) );
    }
    return( FALSE );
}


#define N_CONSTANT 0
#define OP_CMP_EQUAL 0x30
#define OP_CMP_NOT_EQUAL 0x31
#define OP_CMP_LESS 0x34
#define OP_CMP_GREATER 0x32
#define OP_CMP_GREATER_EQUAL 0x35
#define OP_CMP_LESS_EQUAL 0x33
#define OP_BIT_TEST_TRUE 0x2e
#define OP_BIT_TEST_FALSE 0x2f
unsigned char NextCmp(instruction *ins);
unsigned char IsMin(name *op,unsigned type_class);
unsigned char IsMax(name *op,unsigned type_class);
unsigned char SameLocation(name *left,name *right);
unsigned char OtherVerify( unsigned kind, instruction *ins,
                             name *op1, name *op2, name *result ) {
/******************************************************************
    verify a if "kind" is true about instruction "ins" whose operands
    are "op1", "op2", "result".  This the target independant code.  We
    want to know these things for the 8086, 386 and 370 compilers.
*/

    switch( kind ) {
    case V_NO:
        /* This can get called if it's an optimization reduction that's
            no good for volatile instructions */
        return( TRUE );
    case V_CMPEQ:
        if( ins->head.opcode == OP_CMP_EQUAL ) return( TRUE );
        if( ins->head.opcode == OP_CMP_NOT_EQUAL ) return( TRUE );
        break;
    case V_CMPFALSE:
        if( ins->result != NULL ) return ( FALSE );
        if( NextCmp( ins ) ) return( FALSE );
        if( ins->head.opcode == OP_CMP_LESS ) {
            return( IsMin( op2, ins->type_class ) );
        } else if( ins->head.opcode == OP_CMP_GREATER ) {
            return( IsMax( op2, ins->type_class ) );
        } else if( ins->head.opcode == OP_BIT_TEST_TRUE ) {
            goto test_op2_zero;
        }
        break;
    case V_CMPTRUE:
        if( ins->result != NULL ) return ( FALSE );
        if( NextCmp( ins ) ) return( FALSE );
        if( ins->head.opcode == OP_CMP_GREATER_EQUAL ) {
            return( IsMin( op2, ins->type_class ) );
        } else if( ins->head.opcode == OP_CMP_LESS_EQUAL ) {
            return( IsMax( op2, ins->type_class ) );
        } else if( ins->head.opcode == OP_BIT_TEST_FALSE ) {
            goto test_op2_zero;
        }
        break;
    case V_OFFSETZERO:
        if( op1->n.class == N_INDEXED && op1->i.constant == 0
         && (op1->i.base==NULL || (op1->i.index_flags&0x10)) ) return( TRUE );
        break;
    case V_OP1ONE:
        if( op1->c.const_type == CONS_ABSOLUTE && op1->c.int_value == 1 )
            return( TRUE );
        break;
    case V_OP1RELOC:
        if( op1->n.class == N_CONSTANT
         && op1->c.const_type != CONS_ABSOLUTE ) return( TRUE );
        break;
    case V_OP1ZERO:
        if( op1->c.const_type == CONS_ABSOLUTE
         && CFIsI16( op1->c.value )
         && op1->c.int_value == 0 ) return( TRUE );
        break;
    case V_OP2NEG:
        if( ( op2->c.const_type == CONS_ABSOLUTE )
         && ( op2->c.int_value < 0 )
         && ( op2->c.int_value & 0xffff ) != 0x8000
         && ( op2->c.int_value != 0x80000000 ) ) return( TRUE );
        break;
    case V_OP2ONE:
        if( op2->c.const_type == CONS_ABSOLUTE
         && op2->c.int_value == 1 ) return( TRUE );
        break;
    case V_OP2POW2:
        if( !CFIsI32( op2->c.value ) ) return( FALSE );
        /* This older target tests GetLog2 directly; the released later
         * Op2Pow2 helper adds a signed high-bit restriction absent here. */
        return GetLog2(op2->c.int_value)!=-1;
        break;
    case V_OP2TWO:
        if( op2->c.const_type == CONS_ABSOLUTE
         && op2->c.int_value == 2 ) return( TRUE );
        break;
    case V_OP2ZERO:
    test_op2_zero:
        if( op2->c.const_type == CONS_ABSOLUTE
         &&  CFIsI16( op2->c.value )
         &&  op2->c.int_value == 0 ) return( TRUE );
        break;
    case V_OP2_FFFFFFFF:
        if( op2->c.int_value == 0xFFFFFFFF ) return( TRUE );
        break;
    case V_SAME_LOCN:
         if( op1->n.class != N_TEMP ) return( FALSE );
         if( result->n.class != N_TEMP ) return( FALSE );
         return( SameLocation( result, op1 ) );
    case V_SAME_TYPE:
        /* if we have a constant, the name_class of op1 is bogus */
        if( op1->n.class == N_CONSTANT ) {
            if( ins->type_class == result->n.name_class ) return( TRUE );
        } else {
            if( op1->n.name_class == result->n.name_class ) return( TRUE );
        }
        break;
    case V_SPEED:
        if( OptForSize <= 50 ) return( TRUE );
        break;
    case V_SWAP_GOOD:
        switch( op2->n.class ) {
        case N_REGISTER:
        case N_CONSTANT:
            return( FALSE );
        case N_TEMP:
            if( op2->t.temp_flags & CONST_TEMP ) return( FALSE );
            /* fall through */
        default:
            if( op1 != result ) return( TRUE );
            break;
        }
        break;
    case V_U_TEST:
        if( op2->c.const_type != CONS_ABSOLUTE ) return( FALSE );
        if( !CFIsI16( op2->c.value ) ) return( FALSE );
        if( op2->c.int_value != 0 ) return( FALSE );
        if( ins->head.opcode == OP_CMP_EQUAL
         || ins->head.opcode == OP_CMP_NOT_EQUAL
         || ins->type_class == U4 ) return( TRUE );
        break;
    default:
        Zoiks(53);
        return( FALSE );
    }
    return( FALSE );
}

extern opcode_entry DoNop[];
unsigned char NextCmp(instruction *ins)
{
    instruction *next;
    next=ins->head.next;
    if (next->head.opcode<0x2e || next->head.opcode>0x35) return 0;
    if (next->table==DoNop) return 1;
    if (next->u.gen_table==NULL) return 0;
    return next->u.gen_table->generate==0;
}
unsigned char CFIsI8(struct cfloat *value);
unsigned char IsMin(name *op,unsigned type_class)
{
    if (op->c.const_type==0) {
        switch (type_class) {
        case U1:case U2:case U4:
            if (CFIsI16(op->c.value) && op->c.int_value==0) return 1;
            break;
        case I1:
            if (CFIsI8(op->c.value) && op->c.int_value==0x80) return 1;
            break;
        case I2:
            if (CFIsI16(op->c.value) && op->c.int_value==0x8000) return 1;
            break;
        case I4:
            if (CFIsI32(op->c.value) && (unsigned long)op->c.int_value==0x80000000UL) return 1;
            break;
        }
    }
    return 0;
}
unsigned char CFIsU8(struct cfloat *value);
unsigned char IsMax(name *op,unsigned type_class)
{
    if (op->c.const_type==0) {
        switch (type_class) {
        case U1:
            if (CFIsU8(op->c.value) && op->c.int_value==0xff) return 1;
            break;
        case U2:
            if (CFIsU16(op->c.value) && op->c.int_value==0xffff) return 1;
            break;
        case U4:
            if (CFIsU32(op->c.value) && (unsigned long)op->c.int_value==0xffffffffUL) return 1;
            break;
        case I1:
            if (CFIsU8(op->c.value) && op->c.int_value==0x7f) return 1;
            break;
        case I2:
            if (CFIsU16(op->c.value) && op->c.int_value==0x7fff) return 1;
            break;
        case I4:
            if (CFIsU32(op->c.value) && op->c.int_value==0x7fffffffL) return 1;
            break;
        }
    }
    return 0;
}
/* The older body compares locations without the later floating-type guard. */
name *DeAlias(name *value);
unsigned char SameLocation(name *left,name *right)
{
    name *base;
    unsigned long left_location,right_location;
    if (left->t.location==0x7fffffffL || right->t.location==0x7fffffffL)
        return 0;
    base=DeAlias(left);
    left_location=(unsigned long)base->t.location+left->v.offset-base->v.offset;
    base=DeAlias(right);
    right_location=(unsigned long)base->t.location+right->v.offset-base->v.offset;
    return left_location==right_location;
}

