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

/* Initial6b9cb443 analyse.c, reduced to the actual1994 return-expression
 * entry. The later FnRetnOpt flag propagation is absent from these bytes. */
#include "compiler.h"
#include "ptree.h"
#include "symbol.h"
#include "pending.h"
#include "convdiag.h"
#include <recon.h>
TYPE SymFuncReturnType(SYMBOL sym);
PTREE NodeBinary(unsigned char cgop,PTREE left,PTREE right);
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
PTREE PTreeErrorNode(PTREE expr)
{ (void)expr; STUB_TRAP(); return NULL; }
int PTreePropogateError(PTREE expr);
PTREE *PTreeRefLeft(PTREE expr);
int AnalyseLvalue(PTREE *expr);
TYPE StructType(TYPE type);
TYPE MakeReferenceTo(TYPE type);
unsigned char ObjModelArgument(TYPE type);
PTREE NodeConvert(TYPE type,PTREE expr);
PTREE NodeAssignTemporary(TYPE type,PTREE expr)
{ (void)type; (void)expr; STUB_TRAP(); return NULL; }
PTREE OverloadOperator(PTREE expr);
int NodeIsUnaryOp(PTREE expr,unsigned char operation);
int AnalyseLvalueAddrOf(PTREE *expr)
{ (void)expr; STUB_TRAP(); return 0; }
PTREE PTreeOp(PTREE *expr);
int NodeFunctionOperand(PTREE expr,PTREE *function);
int convertFunctionOperand(TYPE type,PTREE *operand,PTREE function)
{ (void)type; (void)operand; (void)function; STUB_TRAP(); return 0; }
int checkFunctionOperand(PTREE function)
{ (void)function; STUB_TRAP(); return 0; }
int SymIsThisFuncMember(SYMBOL sym);
TYPE SymClass(SYMBOL sym);
TYPE MakeMemberPointerTo(TYPE host,TYPE member);
TYPE MakePointerTo(TYPE type);
void PTreeErrorExpr(PTREE tree,unsigned error);
int prepare_operand(PTREE *expr,unsigned control)
{
    PTREE original=PTreeOp(expr),function;
    SYMBOL sym;
    TYPE type;
    int kind=NodeFunctionOperand(original,&function);
    /* The old classifier's four action codes are retained; the later source
     * folded this preparation into semantic actions. No other code adjusts
     * the expression. Control bits select conversion/diagnostic domains. */
    switch(kind) {
    case 1:
        if(!(control&4)) return convertFunctionOperand(NULL,expr,function);
        if(!checkFunctionOperand(function)) break;
        sym=(SYMBOL)function->u.subtree[0];
        if((function->flags&0x100) && SymIsThisFuncMember(sym)) {
            type=function->type;
            original->type=MakeMemberPointerTo(SymClass(sym),type);
            original->flags|=0x200;
            return 1;
        }
        return convertFunctionOperand(MakePointerTo(function->type),expr,function);
    case 2:
        if(control&1) return convertFunctionOperand(NULL,expr,function);
        PTreeErrorExpr(function,486);
        break;
    case 3:
        if(!checkFunctionOperand(function)) break;
        return convertFunctionOperand(function->type,expr,function);
    case 4:
        if(control&2) return convertFunctionOperand(NULL,expr,function);
        PTreeErrorExpr(function,486);
        break;
    default: return 1;
    }
    PTreeErrorNode(original);
    return 0;
}
PTREE PTreeOpLeft(PTREE expr);
PTREE PTreeOpRight(PTREE expr);
TYPE TypeReferenced(TYPE type);
TYPE TypedefModifierRemove(TYPE type);
TYPE PointerTypeEquivalent(TYPE type)
{
    type=TypedefModifierRemove(type);
    switch(type->id) {
    case TYP_POINTER: case TYP_ARRAY: case TYP_FUNCTION: return type;
    }
    return NULL;
}
TYPE ArithType(TYPE type);
/* The old operator classifier has five combined slots: ptr/arith pairs
 * and OTHER. Only the actual expression kinds carry classifiable values. */
unsigned classify_operand(PTREE operand)
{
    TYPE type;
    if(operand && operand->type) {
        type=TypeReferenced(operand->type);
        switch(operand->op) {
        case 1: case 2: case 3: case 4: case 5:
        case 7: case 8: case 9: case 10: case 11:
            type=TypedefModifierRemove(type);
            if(PointerTypeEquivalent(type)) return 0;
            if(ArithType(type)) return 1;
            break;
        }
    }
    return 4;
}
/* Encountered old semantic-driver data:121 operators by five combined
 * operand classes, with59 byte-action programs. These are the exact separate
 * typed tables at DATA b1dd9..b2036 and b1ced..b1dd9, not a section image.
 * Known1994 roles use the initial analyse.c names; unresolved action/routine
 * meanings retain OLD ordinals and trap in the driver until reconstructed.
 * Later action insertions and split RETURN classes are not imported. */
enum {
    OPR_BIN_ARITH=0,
    OPR_BIN_ARITH_I=1,
    OPR_MINUS_PP=2,
    OPR_QUESTMRK=3,
    OPR_COLON_PP=4,
    OPR_COLON_AA=5,
    OPR_COLON_AP=6,
    OPR_COLON_PA=7,
    OPR_COLON_OTHER=8,
    OPR_UN_PLUS_PTR=9,
    OPR_UN_ARITH=10,
    OPR_UN_ARITH_I=11,
    OPR_ADDR_OF=12,
    OPR_INDIRECT=13,
    OPR_LINCDEC_PTR=14,
    OPR_LINCDEC_ARITH=15,
    OPR_INCDEC_PTR=16,
    OPR_INCDEC_ARITH=17,
    OPR_SW_LV_EXPR=18,
    OPR_SHIFT_OP=19,
    OPR_LVALUE_BEXPR=20,
    OPR_EQUAL_PP=21,
    OPR_EQUAL_AA=22,
    OPR_EQUAL_PZ=23,
    OPR_EQUAL_OTHER=24,
    OPR_INIT_PP=25,
    OPR_INIT_AA=26,
    OPR_INIT_PZ=27,
    OPR_INIT_OTHER=28,
    OPR_CMP_PTR_ZERO=29,
    OPR_CMP_ZERO_PTR=30,
    OPR_CMP_PP=31,
    OPR_CMP_AA=32,
    OPR_CMP_MP=33,
    OPR_BIN_LOG=34,
    OPR_BIN_LOG_MP=35,
    OPR_UN_LOG=36,
    OPR_UN_LOG_P=37,
    OPR_UN_LOG_MP=38,
    OPR_EXPLICIT_CAST=39,
    OPR_OLD_40=40,
    OPR_OLD_41=41,
    OPR_OLD_42=42,
    OPR_OLD_43=43,
    OPR_OLD_44=44,
    OPR_OLD_45=45,
    OPR_OLD_46=46,
    OPR_OLD_47=47,
    OPR_OLD_48=48,
    OPR_OLD_49=49,
    OPR_OLD_50=50,
    OPR_OLD_51=51,
    OPR_RETURN=52,
    OPR_DOT_STAR=53,
    OPR_ARROW_STAR=54,
    OPR_CONVERT_INT=55,
    OPR_THROW=56,
    OPR_SEGOP=57,
    OPR_SEGNAME=58,
    OPR_IMPOSSIBLE=59,
    OPR_OLD_60=60,
    OPR_ER_BOTH_PTR=61,
    OPR_ER_SUB_AP=62,
    OPR_ER_ONLY_AR=63,
    OPR_ER_ONLY_AP=64,
    OPR_ER_ONLY_IN=65,
    OPR_ER_AP_ASSN=66,
    OPR_ER_NO_PTR=67,
    OPR_OLD_68=68,
    OPR_OLD_69=69,
    OPR_ER_INDEX=70,
    OPR_OLD_71=71,
    OPR_OLD_72=72
};
enum {
    OPAC_REQD_INT_LEFT=0,
    OPAC_REQD_INT_RIGHT=1,
    OPAC_REQD_LVALUE_LEFT=2,
    OPAC_REQD_NOT_ENUM_LEFT=3,
    OPAC_REQD_ENUM_SAME=4,
    OPAC_REQD_ZERO_LEFT=5,
    OPAC_REQD_ZERO_RIGHT=6,
    OPAC_REQD_PTR_SCALES_LEFT=7,
    OPAC_REQD_PTR_SCALES_RIGHT=8,
    OPAC_REQD_NOT_CONST_LEFT=9,
    OPAC_REQD_NOT_CONST_INIT=10,
    OPAC_REQD_NOT_ARRAY_LEFT=11,
    OPAC_REQD_NOT_FUNC_LEFT=12,
    OPAC_REQD_NOT_FUNC_RIGHT=13,
    OPAC_REQD_NOT_MFUN_LEFT=14,
    OPAC_REQD_NOT_MFUN_RIGHT=15,
    OPAC_REQD_DEFD_CLPTR_LEFT=16,
    OPAC_REQD_DEFD_CLPTR_RIGHT=17,
    OPAC_REQD_CLASS_LEFT=18,
    OPAC_REQD_CLASS_PTR_LEFT=19,
    OPAC_REQD_BOOL_LEFT=20,
    OPAC_REQD_BOOL_RIGHT=21,
    OPAC_WARN_ADJ_COMPARES=22,
    OPAC_WARN_INT_TRUNC=23,
    OPAC_WARN_BOOL_ASSIGN_RIGHT=24,
    OPAC_WARN_BOOL_ASSIGN_LEFT=25,
    OPAC_WARN_CONST_COND_LEFT=26,
    OPAC_WARN_CONST_COND_RIGHT=27,
    OPAC_WARN_POINTER_ZERO=28,
    OPAC_CONV_INIT_REF=29,
    OPAC_CONV_STAR_ADDR_OF=30,
    OPAC_CONV_SWITCH=31,
    OPAC_CONV_CMP_ZERO_LEFT=32,
    OPAC_CONV_CMP_ZERO_RIGHT=33,
    OPAC_CONV_TO_PTR_DIFF=34,
    OPAC_CONV_STRIP_TYPE=35,
    OPAC_CONV_RVALUE_LEFT=36,
    OPAC_CONV_RVALUE_RIGHT=37,
    OPAC_CONV_MEMBER=38,
    OPAC_CONV_INDEX=39,
    OPAC_CONV_TYPE_RIGHT=40,
    OPAC_CONV_TYPE_LEFT=41,
    OPAC_CONV_AA_COMMON=42,
    OPAC_CONV_PP_COMMON=43,
    OPAC_CONV_PP_ASSIGN=44,
    OPAC_CONV_CTOR=45,
    OPAC_CONV_REFERENCE=46,
    OPAC_CONV_MEANINGLESS=47,
    OPAC_CONV_LOCN_RIGHT=48,
    OPAC_STATIC_TEMP_SET=49,
    OPAC_STATIC_TEMP_RESET=50,
    OPAC_ASSIGN_OTHER=51,
    OPAC_INIT_CL_MP=52,
    OPAC_CONV_INIT_BARE=53,
    OPAC_RESULT_UN_ARITH=54,
    OPAC_RESULT_BIN_ARITH=55,
    OPAC_RESULT_CMP_ARITH=56,
    OPAC_RESULT_ADDR_OF=57,
    OPAC_RESULT_INCDEC_ARITH=58,
    OPAC_RESULT_PTR_SIZE=59,
    OPAC_RESULT_INCDEC_PTR=60,
    OPAC_RESULT_QUESTMRK=61,
    OPAC_RESULT_LVALUE=62,
    OPAC_RESULT_BOOLEAN=63,
    OPAC_RESULT_SIZEOF=64,
    OPAC_RESULT_OFFSETOF=65,
    OPAC_CONV_EXPLICIT_CAST=66,
    OPAC_RESULT_NEW=67,
    OPAC_RESULT_BARE=68,
    OPAC_RESULT_DLT=69,
    OPAC_RESULT_COLON_RVALUE=70,
    OPAC_RESULT_COLON_AA=71,
    OPAC_RESULT_COLON_PP=72,
    OPAC_RESULT_COLON_OTHER=73,
    OPAC_RESULT_COLON=74,
    OPAC_OLD_75=75,
    OPAC_RESULT_ASSIGN=76,
    OPAC_RESULT_RETURN=77,
    OPAC_OLD_78=78,
    OPAC_RESULT_INIT=79,
    OPAC_RESULT_MINUS_PP=80,
    OPAC_RESULT_BITQUEST=81,
    OPAC_RELOAD_EXPR_BINARY=82,
    OPAC_RELOAD_EXPR_UNARY=83,
    OPAC_RELOAD_EXPR_TYPE=84,
    OPAC_CONV_BASIC_TYPE=85,
    OPAC_RESULT_MEMPTR_QUAL=86,
    OPAC_RESULT_OTHER_CMP=87,
    OPAC_RESULT_THROW=88,
    OPAC_RESULT_SEGOP=89,
    OPAC_RESULT_SEGNAME=90,
    OPAC_RESULT_COND_EXPR=91,
    OPAC_RESULT_COND_OPRS=92,
    OPAC_RESULT_COND_OPR=93,
    OPAC_DREF_PTR_LEFT=94,
    OPAC_DREF_PTR_RIGHT=95,
    OPAC_END=96
};
static unsigned char opr_rtn_table[121][5]={
    { OPR_ER_BOTH_PTR, OPR_LVALUE_BEXPR, OPR_SW_LV_EXPR, OPR_BIN_ARITH, OPR_ER_ONLY_AP }, /* 0: PLUS */
    { OPR_MINUS_PP, OPR_LVALUE_BEXPR, OPR_ER_SUB_AP, OPR_BIN_ARITH, OPR_ER_ONLY_AP }, /* 1: MINUS */
    { OPR_ER_ONLY_AR, OPR_ER_ONLY_AR, OPR_ER_ONLY_AR, OPR_BIN_ARITH, OPR_ER_ONLY_AR }, /* 2: TIMES */
    { OPR_ER_ONLY_AR, OPR_ER_ONLY_AR, OPR_ER_ONLY_AR, OPR_BIN_ARITH, OPR_ER_ONLY_AR }, /* 3: DIVIDE */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_BIN_ARITH_I, OPR_ER_ONLY_IN }, /* 4: PERCENT */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_BIN_ARITH_I, OPR_ER_ONLY_IN }, /* 5: AND */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_BIN_ARITH_I, OPR_ER_ONLY_IN }, /* 6: OR */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_BIN_ARITH_I, OPR_ER_ONLY_IN }, /* 7: XOR */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_SHIFT_OP, OPR_ER_ONLY_IN }, /* 8: RSHIFT */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_SHIFT_OP, OPR_ER_ONLY_IN }, /* 9: LSHIFT */
    { OPR_ER_ONLY_AR, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_UN_ARITH, OPR_ER_ONLY_IN }, /* 10: NEGATE */
    { OPR_ER_ONLY_IN, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_UN_ARITH_I, OPR_ER_ONLY_IN }, /* 11: COMPLEMENT */
    { OPR_ADDR_OF, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_ADDR_OF, OPR_ADDR_OF }, /* 12: ADDR_OF */
    { OPR_EQUAL_PP, OPR_EQUAL_PZ, OPR_ER_AP_ASSN, OPR_EQUAL_AA, OPR_EQUAL_OTHER }, /* 13: EQUAL */
    { OPR_CMP_PP, OPR_CMP_PTR_ZERO, OPR_CMP_ZERO_PTR, OPR_CMP_AA, OPR_CMP_MP }, /* 14: EQ */
    { OPR_CMP_PP, OPR_CMP_PTR_ZERO, OPR_CMP_ZERO_PTR, OPR_CMP_AA, OPR_CMP_MP }, /* 15: NE */
    { OPR_CMP_PP, OPR_CMP_PTR_ZERO, OPR_CMP_ZERO_PTR, OPR_CMP_AA, OPR_ER_ONLY_AP }, /* 16: GT */
    { OPR_CMP_PP, OPR_CMP_PTR_ZERO, OPR_CMP_ZERO_PTR, OPR_CMP_AA, OPR_ER_ONLY_AP }, /* 17: LE */
    { OPR_CMP_PP, OPR_CMP_PTR_ZERO, OPR_CMP_ZERO_PTR, OPR_CMP_AA, OPR_ER_ONLY_AP }, /* 18: LT */
    { OPR_CMP_PP, OPR_CMP_PTR_ZERO, OPR_CMP_ZERO_PTR, OPR_CMP_AA, OPR_ER_ONLY_AP }, /* 19: GE */
    { OPR_BIN_LOG, OPR_BIN_LOG, OPR_BIN_LOG, OPR_BIN_LOG, OPR_BIN_LOG_MP }, /* 20: AND_AND */
    { OPR_BIN_LOG, OPR_BIN_LOG, OPR_BIN_LOG, OPR_BIN_LOG, OPR_BIN_LOG_MP }, /* 21: OR_OR */
    { OPR_UN_LOG, OPR_UN_LOG, OPR_UN_LOG, OPR_UN_LOG, OPR_UN_LOG_P }, /* 22: NOT */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 23: ARROW */
    { OPR_ER_INDEX, OPR_OLD_44, OPR_OLD_45, OPR_ER_INDEX, OPR_ER_INDEX }, /* 24: SUBSCRIPT */
    { OPR_ER_BOTH_PTR, OPR_OLD_40, OPR_ER_AP_ASSN, OPR_OLD_41, OPR_ER_ONLY_AP }, /* 25: EQUAL_PLUS */
    { OPR_ER_BOTH_PTR, OPR_OLD_40, OPR_ER_AP_ASSN, OPR_OLD_41, OPR_ER_ONLY_AP }, /* 26: EQUAL_MINUS */
    { OPR_ER_ONLY_AR, OPR_ER_ONLY_AR, OPR_ER_ONLY_AR, OPR_OLD_41, OPR_ER_ONLY_AR }, /* 27: EQUAL_TIMES */
    { OPR_ER_ONLY_AR, OPR_ER_ONLY_AR, OPR_ER_ONLY_AR, OPR_OLD_41, OPR_ER_ONLY_AR }, /* 28: EQUAL_DIVIDE */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_OLD_42, OPR_ER_ONLY_IN }, /* 29: EQUAL_PERCENT */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_OLD_42, OPR_ER_ONLY_IN }, /* 30: EQUAL_AND */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_OLD_42, OPR_ER_ONLY_IN }, /* 31: EQUAL_OR */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_OLD_42, OPR_ER_ONLY_IN }, /* 32: EQUAL_XOR */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_OLD_43, OPR_ER_ONLY_IN }, /* 33: EQUAL_RSHIFT */
    { OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_ER_ONLY_IN, OPR_OLD_43, OPR_ER_ONLY_IN }, /* 34: EQUAL_LSHIFT */
    { OPR_UN_PLUS_PTR, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_UN_ARITH, OPR_ER_ONLY_AR }, /* 35: UPLUS */
    { OPR_LINCDEC_PTR, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_LINCDEC_ARITH, OPR_ER_ONLY_AP }, /* 36: PRE_PLUS_PLUS */
    { OPR_LINCDEC_PTR, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_LINCDEC_ARITH, OPR_ER_ONLY_AP }, /* 37: PRE_MINUS_MINUS */
    { OPR_INCDEC_PTR, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_INCDEC_ARITH, OPR_ER_ONLY_AP }, /* 38: POST_PLUS_PLUS */
    { OPR_INCDEC_PTR, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_INCDEC_ARITH, OPR_ER_ONLY_AP }, /* 39: POST_MINUS_MINUS */
    { OPR_INDIRECT, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_ER_NO_PTR, OPR_ER_NO_PTR }, /* 40: INDIRECT */
    { OPR_QUESTMRK, OPR_QUESTMRK, OPR_QUESTMRK, OPR_QUESTMRK, OPR_QUESTMRK }, /* 41: old operator 41 */
    { OPR_OLD_46, OPR_OLD_46, OPR_OLD_46, OPR_OLD_46, OPR_OLD_46 }, /* 42: COMMA */
    { OPR_COLON_PP, OPR_COLON_PA, OPR_COLON_AP, OPR_COLON_AA, OPR_COLON_OTHER }, /* 43: QUESTION */
    { OPR_OLD_48, OPR_OLD_48, OPR_OLD_48, OPR_OLD_48, OPR_OLD_48 }, /* 44: COLON */
    { OPR_DOT_STAR, OPR_DOT_STAR, OPR_DOT_STAR, OPR_DOT_STAR, OPR_DOT_STAR }, /* 45: DOT_STAR */
    { OPR_ARROW_STAR, OPR_ARROW_STAR, OPR_ARROW_STAR, OPR_ARROW_STAR, OPR_ARROW_STAR }, /* 46: ARROW_STAR */
    { OPR_OLD_47, OPR_OLD_47, OPR_OLD_71, OPR_OLD_71, OPR_OLD_47 }, /* 47: FUNCTION */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 48: DOT */
    { OPR_ER_ONLY_IN, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_CONVERT_INT, OPR_CONVERT_INT }, /* 49: old operator 49 */
    { OPR_UN_LOG_MP, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_UN_LOG_MP, OPR_UN_LOG_MP }, /* 50: old operator 50 */
    { OPR_UN_LOG_MP, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_UN_LOG_MP, OPR_UN_LOG_MP }, /* 51: old operator 51 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 52: old operator 52 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_OLD_60 }, /* 53: STORAGE */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_OLD_60 }, /* 54: OPERATOR */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 55: COLON_COLON */
    { OPR_OLD_72, OPR_OLD_72, OPR_SEGOP, OPR_OLD_72, OPR_OLD_72 }, /* 56: old operator 56 */
    { OPR_EXPLICIT_CAST, OPR_EXPLICIT_CAST, OPR_EXPLICIT_CAST, OPR_EXPLICIT_CAST, OPR_EXPLICIT_CAST }, /* 57: CONVERT */
    { OPR_OLD_50, OPR_OLD_50, OPR_OLD_50, OPR_OLD_50, OPR_OLD_50 }, /* 58: DELETE */
    { OPR_OLD_50, OPR_OLD_50, OPR_OLD_50, OPR_OLD_50, OPR_OLD_50 }, /* 59: DELETE_G */
    { OPR_OLD_50, OPR_OLD_50, OPR_OLD_50, OPR_OLD_50, OPR_OLD_50 }, /* 60: DELETE_ARRAY */
    { OPR_OLD_50, OPR_OLD_50, OPR_OLD_50, OPR_OLD_50, OPR_OLD_50 }, /* 61: DELETE_ARRAY_G */
    { OPR_OLD_51, OPR_OLD_51, OPR_OLD_51, OPR_OLD_51, OPR_OLD_51 }, /* 62: CTOR */
    { OPR_OLD_49, OPR_OLD_49, OPR_OLD_49, OPR_OLD_49, OPR_OLD_49 }, /* 63: NEW */
    { OPR_OLD_49, OPR_OLD_49, OPR_OLD_49, OPR_OLD_49, OPR_OLD_49 }, /* 64: NEW_G */
    { OPR_OLD_49, OPR_OLD_49, OPR_OLD_49, OPR_OLD_49, OPR_OLD_49 }, /* 65: NEW_ARRAY */
    { OPR_OLD_60, OPR_OLD_60, OPR_OLD_60, OPR_OLD_60, OPR_OLD_60 }, /* 66: old operator 66 */
    { OPR_OLD_60, OPR_OLD_60, OPR_OLD_60, OPR_OLD_60, OPR_OLD_60 }, /* 67: old operator 67 */
    { OPR_SEGNAME, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 68: old operator 68 */
    { OPR_RETURN, OPR_RETURN, OPR_RETURN, OPR_RETURN, OPR_RETURN }, /* 69: RETURN */
    { OPR_OLD_47, OPR_OLD_47, OPR_OLD_71, OPR_OLD_71, OPR_OLD_47 }, /* 70: old operator 70 */
    { OPR_THROW, OPR_THROW, OPR_THROW, OPR_THROW, OPR_THROW }, /* 71: old operator 71 */
    { OPR_INIT_PP, OPR_INIT_PZ, OPR_ER_AP_ASSN, OPR_INIT_AA, OPR_INIT_OTHER }, /* 72: old operator 72 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 73: old operator 73 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 74: old operator 74 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 75: old operator 75 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 76: old operator 76 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 77: old operator 77 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 78: old operator 78 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 79: old operator 79 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 80: old operator 80 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 81: old operator 81 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 82: old operator 82 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 83: old operator 83 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 84: old operator 84 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 85: old operator 85 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 86: old operator 86 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 87: old operator 87 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 88: old operator 88 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 89: old operator 89 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 90: old operator 90 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 91: old operator 91 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 92: old operator 92 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 93: old operator 93 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 94: old operator 94 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 95: old operator 95 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 96: old operator 96 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 97: old operator 97 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 98: old operator 98 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 99: DTOR */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 100: old operator 100 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 101: old operator 101 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 102: old operator 102 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 103: old operator 103 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 104: old operator 104 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 105: old operator 105 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 106: old operator 106 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 107: old operator 107 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 108: old operator 108 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 109: old operator 109 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 110: old operator 110 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 111: old operator 111 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 112: old operator 112 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 113: old operator 113 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 114: old operator 114 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 115: old operator 115 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 116: old operator 116 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 117: old operator 117 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 118: old operator 118 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 119: old operator 119 */
    { OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE, OPR_IMPOSSIBLE }, /* 120: old operator 120 */
};
static unsigned char opac_BIN_ARITH[]={ OPAC_CONV_RVALUE_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_RESULT_BIN_ARITH, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_BIN_ARITH_I[]={ OPAC_REQD_INT_LEFT, OPAC_REQD_INT_RIGHT, OPAC_CONV_RVALUE_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_RESULT_BIN_ARITH, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_MINUS_PP[]={ OPAC_REQD_PTR_SCALES_LEFT, OPAC_REQD_PTR_SCALES_RIGHT, OPAC_DREF_PTR_LEFT, OPAC_DREF_PTR_RIGHT, OPAC_RESULT_MINUS_PP, OPAC_REQD_DEFD_CLPTR_LEFT, OPAC_CONV_TO_PTR_DIFF, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_QUESTMRK[]={ OPAC_CONV_CMP_ZERO_LEFT, OPAC_WARN_CONST_COND_LEFT, OPAC_RESULT_QUESTMRK, OPAC_RESULT_COND_EXPR, OPAC_END };
static unsigned char opac_COLON_PP[]={ OPAC_REQD_NOT_MFUN_LEFT, OPAC_REQD_NOT_MFUN_RIGHT, OPAC_RESULT_COLON_AA, OPAC_RESULT_COLON_OTHER, OPAC_RELOAD_EXPR_BINARY, OPAC_OLD_75, OPAC_RESULT_COND_OPRS, OPAC_END };
static unsigned char opac_COLON_AA[]={ OPAC_RESULT_COLON_AA, OPAC_RESULT_COLON_PP, OPAC_OLD_75, OPAC_RESULT_COND_OPRS, OPAC_END };
static unsigned char opac_COLON_AP[]={ OPAC_REQD_ZERO_LEFT, OPAC_RESULT_COLON_AA, OPAC_CONV_TYPE_RIGHT, OPAC_OLD_75, OPAC_RESULT_COND_OPRS, OPAC_END };
static unsigned char opac_COLON_PA[]={ OPAC_REQD_ZERO_RIGHT, OPAC_RESULT_COLON_AA, OPAC_OLD_75, OPAC_RESULT_COND_OPRS, OPAC_END };
static unsigned char opac_COLON_OTHER[]={ OPAC_RESULT_COLON_AA, OPAC_RESULT_COLON, OPAC_OLD_75, OPAC_RESULT_COND_OPRS, OPAC_END };
static unsigned char opac_UN_PLUS_PTR[]={ OPAC_CONV_RVALUE_LEFT, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_UN_ARITH[]={ OPAC_CONV_RVALUE_LEFT, OPAC_RESULT_UN_ARITH, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_UN_ARITH_I[]={ OPAC_REQD_INT_LEFT, OPAC_CONV_RVALUE_LEFT, OPAC_RESULT_UN_ARITH, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_ADDR_OF[]={ OPAC_REQD_LVALUE_LEFT, OPAC_RESULT_ADDR_OF, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_INDIRECT[]={ OPAC_DREF_PTR_LEFT, OPAC_CONV_STRIP_TYPE, OPAC_CONV_STAR_ADDR_OF, OPAC_RESULT_LVALUE, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_LINCDEC_PTR[]={ OPAC_REQD_PTR_SCALES_LEFT, OPAC_REQD_NOT_ARRAY_LEFT, OPAC_REQD_DEFD_CLPTR_LEFT, OPAC_REQD_LVALUE_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_RESULT_INCDEC_PTR, OPAC_CONV_TYPE_LEFT, OPAC_RESULT_ASSIGN, OPAC_END };
static unsigned char opac_LINCDEC_ARITH[]={ OPAC_REQD_LVALUE_LEFT, OPAC_REQD_NOT_ENUM_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_RESULT_INCDEC_ARITH, OPAC_RESULT_ASSIGN, OPAC_END };
static unsigned char opac_INCDEC_PTR[]={ OPAC_REQD_PTR_SCALES_LEFT, OPAC_REQD_NOT_ARRAY_LEFT, OPAC_REQD_DEFD_CLPTR_LEFT, OPAC_REQD_LVALUE_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_RESULT_INCDEC_PTR, OPAC_RESULT_BARE, OPAC_END };
static unsigned char opac_INCDEC_ARITH[]={ OPAC_REQD_LVALUE_LEFT, OPAC_REQD_NOT_ENUM_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_RESULT_INCDEC_ARITH, OPAC_RESULT_BARE, OPAC_END };
static unsigned char opac_SW_LV_EXPR[]={ OPAC_REQD_INT_LEFT, OPAC_REQD_PTR_SCALES_RIGHT, OPAC_REQD_DEFD_CLPTR_RIGHT, OPAC_CONV_SWITCH, OPAC_RELOAD_EXPR_BINARY, OPAC_CONV_TYPE_LEFT, OPAC_CONV_BASIC_TYPE, OPAC_CONV_RVALUE_LEFT, OPAC_DREF_PTR_RIGHT, OPAC_RESULT_PTR_SIZE, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_SHIFT_OP[]={ OPAC_REQD_INT_LEFT, OPAC_REQD_INT_RIGHT, OPAC_CONV_RVALUE_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_RESULT_BIN_ARITH, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_LVALUE_BEXPR[]={ OPAC_REQD_INT_RIGHT, OPAC_REQD_PTR_SCALES_LEFT, OPAC_REQD_DEFD_CLPTR_LEFT, OPAC_DREF_PTR_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_RESULT_PTR_SIZE, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_EQUAL_PP[]={ OPAC_REQD_LVALUE_LEFT, OPAC_REQD_NOT_FUNC_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_REQD_NOT_ARRAY_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_CONV_PP_ASSIGN, OPAC_CONV_TYPE_LEFT, OPAC_RESULT_ASSIGN, OPAC_END };
static unsigned char opac_EQUAL_AA[]={ OPAC_REQD_LVALUE_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_REQD_ENUM_SAME, OPAC_CONV_RVALUE_RIGHT, OPAC_WARN_INT_TRUNC, OPAC_CONV_TYPE_LEFT, OPAC_RESULT_ASSIGN, OPAC_RESULT_BITQUEST, OPAC_END };
static unsigned char opac_EQUAL_PZ[]={ OPAC_REQD_LVALUE_LEFT, OPAC_REQD_NOT_FUNC_LEFT, OPAC_REQD_ZERO_RIGHT, OPAC_REQD_NOT_ARRAY_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_CONV_TYPE_LEFT, OPAC_RESULT_ASSIGN, OPAC_END };
static unsigned char opac_EQUAL_OTHER[]={ OPAC_REQD_LVALUE_LEFT, OPAC_REQD_NOT_FUNC_LEFT, OPAC_ASSIGN_OTHER, OPAC_RELOAD_EXPR_TYPE, OPAC_RESULT_ASSIGN, OPAC_END };
static unsigned char opac_INIT_PP[]={ OPAC_CONV_INIT_REF, OPAC_REQD_NOT_FUNC_LEFT, OPAC_CONV_BASIC_TYPE, OPAC_CONV_INIT_BARE, OPAC_STATIC_TEMP_SET, OPAC_RESULT_INIT, OPAC_STATIC_TEMP_RESET, OPAC_RELOAD_EXPR_TYPE, OPAC_END };
static unsigned char opac_INIT_AA[]={ OPAC_CONV_INIT_REF, OPAC_CONV_BASIC_TYPE, OPAC_REQD_LVALUE_LEFT, OPAC_REQD_ENUM_SAME, OPAC_CONV_INIT_BARE, OPAC_STATIC_TEMP_SET, OPAC_RESULT_INIT, OPAC_STATIC_TEMP_RESET, OPAC_RELOAD_EXPR_TYPE, OPAC_END };
static unsigned char opac_INIT_PZ[]={ OPAC_CONV_INIT_REF, OPAC_REQD_NOT_FUNC_LEFT, OPAC_CONV_BASIC_TYPE, OPAC_CONV_INIT_BARE, OPAC_REQD_ZERO_RIGHT, OPAC_CONV_RVALUE_RIGHT, OPAC_STATIC_TEMP_SET, OPAC_RESULT_INIT, OPAC_STATIC_TEMP_RESET, OPAC_RELOAD_EXPR_TYPE, OPAC_END };
static unsigned char opac_INIT_OTHER[]={ OPAC_CONV_INIT_REF, OPAC_REQD_NOT_FUNC_LEFT, OPAC_CONV_BASIC_TYPE, OPAC_STATIC_TEMP_SET, OPAC_INIT_CL_MP, OPAC_STATIC_TEMP_RESET, OPAC_RELOAD_EXPR_TYPE, OPAC_RESULT_ASSIGN, OPAC_END };
static unsigned char opac_CMP_PTR_ZERO[]={ OPAC_REQD_ZERO_RIGHT, OPAC_CONV_RVALUE_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_WARN_ADJ_COMPARES, OPAC_WARN_POINTER_ZERO, OPAC_RESULT_BOOLEAN, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_CMP_ZERO_PTR[]={ OPAC_REQD_ZERO_LEFT, OPAC_CONV_RVALUE_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_WARN_ADJ_COMPARES, OPAC_WARN_POINTER_ZERO, OPAC_RESULT_BOOLEAN, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_CMP_PP[]={ OPAC_CONV_RVALUE_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_WARN_ADJ_COMPARES, OPAC_WARN_POINTER_ZERO, OPAC_CONV_PP_COMMON, OPAC_RESULT_BOOLEAN, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_CMP_AA[]={ OPAC_CONV_RVALUE_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_WARN_ADJ_COMPARES, OPAC_RESULT_CMP_ARITH, OPAC_CONV_AA_COMMON, OPAC_RESULT_BOOLEAN, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_CMP_MP[]={ OPAC_WARN_ADJ_COMPARES, OPAC_RESULT_OTHER_CMP, OPAC_RESULT_BOOLEAN, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_BIN_LOG[]={ OPAC_WARN_BOOL_ASSIGN_LEFT, OPAC_WARN_BOOL_ASSIGN_RIGHT, OPAC_WARN_CONST_COND_LEFT, OPAC_WARN_CONST_COND_RIGHT, OPAC_CONV_CMP_ZERO_RIGHT, OPAC_CONV_CMP_ZERO_LEFT, OPAC_RESULT_BOOLEAN, OPAC_CONV_MEANINGLESS, OPAC_RESULT_COND_OPR, OPAC_RESULT_COND_EXPR, OPAC_END };
static unsigned char opac_BIN_LOG_MP[]={ OPAC_WARN_BOOL_ASSIGN_LEFT, OPAC_WARN_BOOL_ASSIGN_RIGHT, OPAC_WARN_CONST_COND_LEFT, OPAC_WARN_CONST_COND_RIGHT, OPAC_REQD_BOOL_LEFT, OPAC_REQD_BOOL_RIGHT, OPAC_CONV_CMP_ZERO_RIGHT, OPAC_CONV_CMP_ZERO_LEFT, OPAC_RESULT_BOOLEAN, OPAC_CONV_MEANINGLESS, OPAC_RESULT_COND_OPR, OPAC_RESULT_COND_EXPR, OPAC_END };
static unsigned char opac_UN_LOG[]={ OPAC_WARN_BOOL_ASSIGN_LEFT, OPAC_WARN_CONST_COND_LEFT, OPAC_CONV_CMP_ZERO_LEFT, OPAC_RESULT_BOOLEAN, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_UN_LOG_P[]={ OPAC_WARN_BOOL_ASSIGN_LEFT, OPAC_WARN_CONST_COND_LEFT, OPAC_REQD_BOOL_LEFT, OPAC_CONV_CMP_ZERO_LEFT, OPAC_RESULT_BOOLEAN, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_UN_LOG_MP[]={ OPAC_RESULT_SIZEOF, OPAC_RELOAD_EXPR_TYPE, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_EXPLICIT_CAST[]={ OPAC_CONV_MEANINGLESS, OPAC_REQD_NOT_ARRAY_LEFT, OPAC_CONV_EXPLICIT_CAST, OPAC_CONV_REFERENCE, OPAC_END };
static unsigned char opac_OLD_40[]={ OPAC_REQD_INT_RIGHT, OPAC_REQD_NOT_FUNC_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_REQD_DEFD_CLPTR_LEFT, OPAC_REQD_PTR_SCALES_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_RESULT_PTR_SIZE, OPAC_CONV_TYPE_LEFT, OPAC_RESULT_ASSIGN, OPAC_END };
static unsigned char opac_OLD_41[]={ OPAC_REQD_LVALUE_LEFT, OPAC_REQD_NOT_ENUM_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_CONV_TYPE_LEFT, OPAC_RESULT_ASSIGN, OPAC_END };
static unsigned char opac_OLD_42[]={ OPAC_REQD_LVALUE_LEFT, OPAC_REQD_INT_LEFT, OPAC_REQD_INT_RIGHT, OPAC_WARN_INT_TRUNC, OPAC_REQD_NOT_ENUM_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_CONV_TYPE_LEFT, OPAC_RESULT_ASSIGN, OPAC_END };
static unsigned char opac_OLD_43[]={ OPAC_REQD_LVALUE_LEFT, OPAC_REQD_INT_LEFT, OPAC_REQD_INT_RIGHT, OPAC_REQD_NOT_ENUM_LEFT, OPAC_REQD_NOT_CONST_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_CONV_TYPE_LEFT, OPAC_RESULT_ASSIGN, OPAC_END };
static unsigned char opac_OLD_44[]={ OPAC_REQD_INT_RIGHT, OPAC_REQD_NOT_FUNC_LEFT, OPAC_REQD_PTR_SCALES_LEFT, OPAC_REQD_DEFD_CLPTR_LEFT, OPAC_DREF_PTR_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_RESULT_PTR_SIZE, OPAC_CONV_INDEX, OPAC_CONV_STRIP_TYPE, OPAC_RESULT_LVALUE, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_OLD_45[]={ OPAC_REQD_INT_LEFT, OPAC_REQD_NOT_FUNC_RIGHT, OPAC_REQD_PTR_SCALES_RIGHT, OPAC_REQD_DEFD_CLPTR_RIGHT, OPAC_CONV_SWITCH, OPAC_RELOAD_EXPR_BINARY, OPAC_CONV_TYPE_LEFT, OPAC_CONV_BASIC_TYPE, OPAC_CONV_RVALUE_LEFT, OPAC_DREF_PTR_RIGHT, OPAC_RESULT_PTR_SIZE, OPAC_CONV_INDEX, OPAC_CONV_STRIP_TYPE, OPAC_RESULT_LVALUE, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_OLD_46[]={ OPAC_CONV_TYPE_RIGHT, OPAC_OLD_78, OPAC_END };
static unsigned char opac_OLD_47[]={ OPAC_RESULT_NEW, OPAC_RELOAD_EXPR_TYPE, OPAC_CONV_REFERENCE, OPAC_RESULT_BARE, OPAC_END };
static unsigned char opac_OLD_48[]={ OPAC_CONV_TYPE_RIGHT, OPAC_CONV_LOCN_RIGHT, OPAC_END };
static unsigned char opac_OLD_49[]={ OPAC_RESULT_DLT, OPAC_RELOAD_EXPR_TYPE, OPAC_RESULT_BARE, OPAC_END };
static unsigned char opac_OLD_50[]={ OPAC_RESULT_COLON_RVALUE, OPAC_END };
static unsigned char opac_OLD_51[]={ OPAC_REQD_NOT_ARRAY_LEFT, OPAC_CONV_CTOR, OPAC_END };
static unsigned char opac_RETURN[]={ OPAC_WARN_INT_TRUNC, OPAC_RESULT_RETURN, OPAC_END };
static unsigned char opac_DOT_STAR[]={ OPAC_REQD_LVALUE_LEFT, OPAC_REQD_CLASS_LEFT, OPAC_RESULT_MEMPTR_QUAL, OPAC_RELOAD_EXPR_TYPE, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_ARROW_STAR[]={ OPAC_CONV_RVALUE_LEFT, OPAC_REQD_CLASS_PTR_LEFT, OPAC_REQD_DEFD_CLPTR_LEFT, OPAC_RESULT_MEMPTR_QUAL, OPAC_RELOAD_EXPR_TYPE, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_CONVERT_INT[]={ OPAC_REQD_INT_LEFT, OPAC_CONV_RVALUE_LEFT, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_THROW[]={ OPAC_RESULT_THROW, OPAC_END };
static unsigned char opac_SEGOP[]={ OPAC_CONV_RVALUE_LEFT, OPAC_CONV_RVALUE_RIGHT, OPAC_REQD_INT_LEFT, OPAC_CONV_SWITCH, OPAC_RELOAD_EXPR_BINARY, OPAC_RESULT_SEGOP, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char opac_SEGNAME[]={ OPAC_RESULT_SEGNAME, OPAC_CONV_MEANINGLESS, OPAC_END };
static unsigned char *opcomb_actions[59]={
    opac_BIN_ARITH,
    opac_BIN_ARITH_I,
    opac_MINUS_PP,
    opac_QUESTMRK,
    opac_COLON_PP,
    opac_COLON_AA,
    opac_COLON_AP,
    opac_COLON_PA,
    opac_COLON_OTHER,
    opac_UN_PLUS_PTR,
    opac_UN_ARITH,
    opac_UN_ARITH_I,
    opac_ADDR_OF,
    opac_INDIRECT,
    opac_LINCDEC_PTR,
    opac_LINCDEC_ARITH,
    opac_INCDEC_PTR,
    opac_INCDEC_ARITH,
    opac_SW_LV_EXPR,
    opac_SHIFT_OP,
    opac_LVALUE_BEXPR,
    opac_EQUAL_PP,
    opac_EQUAL_AA,
    opac_EQUAL_PZ,
    opac_EQUAL_OTHER,
    opac_INIT_PP,
    opac_INIT_AA,
    opac_INIT_PZ,
    opac_INIT_OTHER,
    opac_CMP_PTR_ZERO,
    opac_CMP_ZERO_PTR,
    opac_CMP_PP,
    opac_CMP_AA,
    opac_CMP_MP,
    opac_BIN_LOG,
    opac_BIN_LOG_MP,
    opac_UN_LOG,
    opac_UN_LOG_P,
    opac_UN_LOG_MP,
    opac_EXPLICIT_CAST,
    opac_OLD_40,
    opac_OLD_41,
    opac_OLD_42,
    opac_OLD_43,
    opac_OLD_44,
    opac_OLD_45,
    opac_OLD_46,
    opac_OLD_47,
    opac_OLD_48,
    opac_OLD_49,
    opac_OLD_50,
    opac_OLD_51,
    opac_RETURN,
    opac_DOT_STAR,
    opac_ARROW_STAR,
    opac_CONVERT_INT,
    opac_THROW,
    opac_SEGOP,
    opac_SEGNAME
};
PTREE NodeStripPoints(PTREE expr);
TYPE NodeType(PTREE expr);
int NodeBitField(PTREE expr);
TYPE TypePointedAt(TYPE type,unsigned short *flags);
TYPE CgStripType(TYPE type);
TYPE IntegralType(TYPE type);
TYPE EnumType(TYPE type);
unsigned short PTreeEffFlags(PTREE expr);

PTREE NodeGetConstantNode(PTREE expr);
unsigned SignedIntType(TYPE type);
void PTreeWarnExpr(PTREE expr,unsigned warning)
{ (void)expr; (void)warning; STUB_TRAP(); }
int NodeIsBinaryOp(PTREE expr,unsigned char operation);
int NodeIsConstant(PTREE expr);
unsigned CgMemorySize(TYPE type);
unsigned getConstBits(PTREE constant)
{
    unsigned value,bits;
    if(SignedIntType(constant->type) && constant->u.int_constant<0)
        value=0U-constant->u.uint_constant;
    else value=constant->u.uint_constant;
    for(bits=0;value!=0;++bits) value>>=1;
    return bits;
}
unsigned getConstBitsType(TYPE type)
{
    return CgMemorySize(type)*8;
}
/* Old two-argument truncation diagnostic, before64-bit constants and
 * promotion-suppression checks. Constants retain their precise bit counts. */
void warnIntTrunc(PTREE node,unsigned tgt_bits)
{
    unsigned bits,warning,right_bits;
    TYPE node_type;
    PTREE constant,colon;
    constant=NodeGetConstantNode(node);
    if(constant) {
        bits=getConstBits(constant);
        warning=388;
    } else if(NodeIsBinaryOp(node,41)) {
        colon=node->u.subtree[1];
        constant=NodeGetConstantNode(PTreeOpLeft(colon));
        if(!constant) {
            bits=getConstBitsType(node->type);
            warning=389;
        } else {
            bits=getConstBits(constant);
            constant=NodeGetConstantNode(PTreeOpRight(colon));
            if(!constant) {
                bits=getConstBitsType(node->type);
                warning=389;
            } else {
                right_bits=getConstBits(constant);
                if(right_bits>bits) bits=right_bits;
                warning=388;
            }
        }
    } else if(node->flags&4) {
        bits=1;
        warning=389;
    } else {
        node_type=node->type;
        if(node->op==9) {
            if(node->cgop>=5 && node->cgop<=7 &&
               node->u.subtree[0]->type==node->u.subtree[1]->type)
                node_type=node->u.subtree[0]->type;
        } else if(node->op==8 && (node->cgop==11 || node->cgop==35))
            node_type=node->u.subtree[0]->type;
        bits=getConstBitsType(node_type);
        warning=389;
    }
    if(tgt_bits<bits) PTreeWarnExpr(node,warning);
}
int DefargBeingCompiled(void);
int NodeIsZeroConstant(PTREE node);
PTREE *PTreeRefRight(PTREE node);
static CNV_DIAG diagReturn={324,323,0,343,347};
static CNV_DIAG diagDefarg={587,586,0,589,590};
PTREE PTreeExtractLocn(PTREE expr,CGIO_LOCATION *location);
PTREE FoldUnary(PTREE expr);
PTREE FoldBinary(PTREE expr);
TYPE ArrayType(TYPE type);
TYPE ReferenceType(TYPE type);
int isVoid(TYPE type);
int ptrCannotScale(PTREE expr);
int NodeDerefPtr(PTREE *expr);
PTREE NodeRvalueLeft(PTREE expr);
PTREE NodeRvalueRight(PTREE expr);
PTREE bld_ptr_adj(PTREE factor,PTREE pointer,unsigned char operation);

PTREE AnalyseOperator(PTREE expr)
{
    int operands_ok=1;
    PTREE left,right;
    TYPE type;
    unsigned index_left,index_right,index,control;
    unsigned char action_code,*ap;
    unsigned cnv_result;
    CNV_DIAG *diagnosis;
    /* Actual old operand preparation precedes operator-action selection.
     * This enclosing interpreter is not accepted: reconstruction currently
     * stops before its remaining operand classification/action interpreter. */
    if(expr->u.subtree[0]) {
        if(NodeIsUnaryOp(expr,12))
            operands_ok=AnalyseLvalueAddrOf(&expr->u.subtree[0]);
        else {
            operands_ok=AnalyseLvalue(&expr->u.subtree[0]);
            if(operands_ok && expr->cgop!=47)
                operands_ok=prepare_operand(&expr->u.subtree[0],2);
        }
    }
    if(expr->op==9 && expr->u.subtree[1])
        operands_ok &= AnalyseLvalue(&expr->u.subtree[1]);
    if(!operands_ok) return PTreeErrorNode(expr);
    expr=OverloadOperator(expr);
    if(expr->op==0) return expr;
    left=PTreeOpLeft(expr);
    index_left=classify_operand(left);
    if(expr->op==9) {
        control=4;
        switch(expr->cgop) {
        case 44: case 72: case 69: case 13: case 57: case 42:
            control|=3; break;
        }
        if(!prepare_operand(&expr->u.subtree[1],control))
            return PTreeErrorNode(expr);
        right=PTreeOpRight(expr);
        if(expr->cgop!=47) {
            if(left && (left->flags&0x80)) {
                PTreeErrorExpr(left,530); return PTreeErrorNode(expr);
            }
            if(right && (right->flags&0x80)) {
                PTreeErrorExpr(right,530); return PTreeErrorNode(expr);
            }
        }
        index_right=classify_operand(right);
        expr->flags|=(PTreeEffFlags(left)|PTreeEffFlags(right))&0x11;
    } else {
        right=NULL;
        expr->u.subtree[1]=NULL;
        if(left && (left->flags&0x80)) {
            PTreeErrorExpr(left,530); return PTreeErrorNode(expr);
        }
        index_right=index_left;
        expr->flags|=PTreeEffFlags(left)&0x11;
    }
    index=index_left*2+index_right;
    if(index>4) index=4;
    type=left?TypedefModifierRemove(left->type):NULL;
    action_code=opr_rtn_table[expr->cgop][index];
    /* Error-routine dispatch and the remaining action bodies are still
     * incomplete in this enclosing owner, and trap in both builds. */
    if(action_code>=59) { STUB_TRAP(); return NULL; }
    for(ap=opcomb_actions[action_code];*ap<OPAC_END;++ap) {
        switch(*ap) {
        case OPAC_REQD_LVALUE_LEFT:
            if(!(left->flags&2)) { STUB_TRAP(); return NULL; }
            continue;
        case OPAC_REQD_NOT_FUNC_LEFT:
            if(FunctionDeclarationType(type)) { STUB_TRAP(); return NULL; }
            continue;
        case OPAC_REQD_NOT_ENUM_LEFT:
            if(EnumType(left->type)) { STUB_TRAP(); return NULL; }
            continue;
        case OPAC_REQD_NOT_CONST_LEFT:
            /* The1994 driver delegates both value/type constness to this
             * existing node predicate, unlike the later TypeForLvalue pass. */
            if(NodeIsConstant(expr->u.subtree[0])) { STUB_TRAP(); return NULL; }
            continue;
        case OPAC_REQD_INT_LEFT:
            if(!IntegralType(type)) { STUB_TRAP(); return NULL; }
            continue;
        case OPAC_REQD_INT_RIGHT:
            if(!IntegralType(right->type)) { STUB_TRAP(); return NULL; }
            continue;
        case OPAC_REQD_DEFD_CLPTR_LEFT:
            {
                TYPE pointed=TypedefModifierRemove(type->of);
                if(!pointed || (pointed->id==TYP_CLASS && !pointed->u.c.info->defined)) {
                    STUB_TRAP(); return NULL;
                }
                continue;
            }
        case OPAC_DREF_PTR_LEFT:
            if(!NodeDerefPtr(&expr->u.subtree[0])) { STUB_TRAP(); return NULL; }
            left=PTreeOpLeft(expr);
            type=left->type;
            continue;
        case OPAC_CONV_STAR_ADDR_OF:
            expr=NodeStripPoints(expr);
            continue;
        case OPAC_RESULT_LVALUE:
            expr->flags|=2;
            continue;
        case OPAC_CONV_STRIP_TYPE:
            if(type->id==TYP_FUNCTION) expr->flags|=left->flags&0x200;
            else type=CgStripType(type);
            continue;
        case OPAC_CONV_RVALUE_LEFT:
            left=NodeRvalueLeft(expr);
            type=left->type;
            continue;
        case OPAC_CONV_RVALUE_RIGHT:
            right=NodeRvalueRight(expr);
            continue;
        case OPAC_RESULT_PTR_SIZE:
            *PTreeRefRight(expr)=bld_ptr_adj(right,left,2);
            right=PTreeOpRight(expr);
            continue;
        case OPAC_RESULT_INCDEC_ARITH:
            /* The old driver builds a typed one directly, before changing
             * the unary opcode into its binary update form. It predates
             * the later convertIncDec and bool-specific machinery. */
            *PTreeRefRight(expr)=PTreeIntConstant(1,type->id);
            expr->op=9;
            expr->cgop+=69;
            type=left->type;
            continue;
        case OPAC_REQD_PTR_SCALES_LEFT:
            if(ptrCannotScale(left)) { STUB_TRAP(); return NULL; }
            continue;
        case OPAC_REQD_PTR_SCALES_RIGHT:
            if(ptrCannotScale(right)) { STUB_TRAP(); return NULL; }
            continue;
        case OPAC_CONV_MEANINGLESS:
            expr->flags&=~0x10;
            continue;
        case OPAC_REQD_NOT_ARRAY_LEFT:
            if(ArrayType(type)) { STUB_TRAP(); return NULL; }
            continue;
        case OPAC_CONV_REFERENCE:
            {
                TYPE ref=ReferenceType(type);
                if(ref) { type=ref; expr->flags|=2; }
                continue;
            }
        case OPAC_CONV_EXPLICIT_CAST:
            {
                static CNV_DIAG diagCast={306,307,208,342,346};
                type=left->type;
                expr->type=type;
                cnv_result=CastImplicit(PTreeRefRight(expr),type,8);
                if(ConversionDiagnose(cnv_result,expr,&diagCast)) {
                    STUB_TRAP(); return NULL;
                }
                expr->flags|=expr->u.subtree[1]->flags;
                if(isVoid(type)) { STUB_TRAP(); return NULL; }
                continue;
            }
        case OPAC_CONV_PP_ASSIGN:
            {
                unsigned short left_flags,right_flags;
                static CNV_DIAG diagPtrConvAssign={166,608,208,459,460};
                TypePointedAt(right->type,&right_flags);
                TypePointedAt(left->type,&left_flags);
                if(right_flags&~left_flags&3) { STUB_TRAP(); return NULL; }
                cnv_result=CastImplicit(PTreeRefRight(expr),NodeType(left)->of,32);
                if(ConversionDiagnose(cnv_result,expr,&diagPtrConvAssign)) {
                    STUB_TRAP(); return NULL;
                }
                continue;
            }
        case OPAC_CONV_TYPE_LEFT:
            type=left->type;
            continue;
        case OPAC_RESULT_ASSIGN:
            expr->flags|=0x11;
            if(!NodeBitField(expr->u.subtree[0])) expr->flags|=2;
            continue;
        case OPAC_CONV_TYPE_RIGHT:
            if(right->flags&2) expr->flags|=2;
            type=right->type;
            continue;
        case OPAC_CONV_LOCN_RIGHT:
            PTreeExtractLocn(expr->u.subtree[1],&expr->locn);
            continue;
        case OPAC_WARN_INT_TRUNC:
            if(!IntegralType(type)) continue;
            if(!IntegralType(right->type)) continue;
            if(NodeIsUnaryOp(left,101)) {
                TYPE field=TypedefModifierRemove(PTreeOpLeft(left)->type);
                unsigned bits;
                if(NodeIsConstant(right)) bits=field->u.b.field_width;
                else bits=getConstBitsType(field->of);
                warnIntTrunc(right,bits);
            } else warnIntTrunc(right,getConstBitsType(type));
            continue;
        case OPAC_RESULT_RETURN:
            {
                TYPE ptr;
                TYPE enum_type=EnumType(left->type);
                if(enum_type && enum_type!=EnumType(right->type)) {
                    PTreeErrorExpr(expr,462);
                    STUB_TRAP(); return NULL;
                }
                ptr=PointerTypeEquivalent(type);
                if(ptr && !(ptr->flag&1) && ArithType(right->type) &&
                   !NodeIsZeroConstant(right)) {
                    PTreeErrorExpr(right,191);
                    STUB_TRAP(); return NULL;
                }
                if(DefargBeingCompiled()) { control=0x44; diagnosis=&diagDefarg; }
                else { control=4; diagnosis=&diagReturn; }
                type=left->type;
                expr->type=type;
                cnv_result=CastImplicit(PTreeRefRight(expr),left->type,control);
                if(ConversionDiagnose(cnv_result,expr,diagnosis)) {
                    STUB_TRAP(); return NULL;
                }
                continue;
            }
        default: STUB_TRAP(); return NULL;
        }
    }
    expr->type=type;
    if(expr->op==8 && expr->u.subtree[0]) expr=FoldUnary(expr);
    else if(expr->op==9 && expr->u.subtree[0] && expr->u.subtree[1])
        expr=FoldBinary(expr);
    return expr;
}
PTREE AnalyseNode(PTREE expr)
{
    PTREE left;
    TYPE class_type;
    PTreePropogateError(expr);
    switch(expr->op) {
    case 6: expr->flags|=2; break;
    case 9:
        switch(expr->cgop) {
        case 48:
            if(!AnalyseLvalue(PTreeRefLeft(expr))) {
                PTreeErrorNode(expr);
                return expr;
            }
            left=expr->u.subtree[0];
            if(!(left->flags&2)) {
                class_type=StructType(left->type);
                if(class_type) {
                    if(ObjModelArgument(class_type)==2) {
                        left=NodeConvert(MakeReferenceTo(left->type),left);
                        left->flags|=2;
                    } else {
                        left=NodeAssignTemporary(left->type,left);
                    }
                    expr->u.subtree[0]=left;
                }
            }
            break;
        case 23:
            if(!AnalyseLvalue(PTreeRefLeft(expr))) PTreeErrorNode(expr);
            else expr=OverloadOperator(expr);
            break;
        case 52: case 55: break;
        default: expr=AnalyseOperator(expr); break;
        }
        break;
    case 8: expr=AnalyseOperator(expr); break;
    }
    return expr;
}
int AnalyseLvalue(PTREE *expr);
void CgFrontExprBegin(void);
PTREE *PTreeRef(PTREE *target);
PTREE run_traversals(PTREE expr)
{
    if(!expr) return PTreeErrorNode(NULL);
    CgFrontExprBegin();
    expr=PTreeTraversePostfix(expr,AnalyseNode);
    if(expr->op!=0) AnalyseLvalue(PTreeRef(&expr));
    return expr;
}
PTREE NodeDone(PTREE expr);
PTREE AnalyseReturnExpr(SYMBOL func,PTREE expr)
{
    PTREE right;
    if(expr) {
        TYPE result=SymFuncReturnType(func);
        right=expr;
        expr=NodeBinary(0x45,PTreeType(result),right);
        expr=PTreeCopySrcLocation(expr,right);
        expr=run_traversals(expr);
        expr=NodeDone(expr);
    }
    return expr;
}

PTREE AnalyseRawExpr(PTREE expr) { return expr ? run_traversals(expr) : NULL; }

void warnMeaningfulSideEffect(PTREE expr)
{
    if(expr->op!=0) {
        if(!(expr->flags&0x10)) {
            if(!(expr->flags&1)) PTreeWarnExpr(expr,628);
            else PTreeWarnExpr(expr,387);
        } else if(!(expr->flags&1)) PTreeWarnExpr(expr,629);
    }
}
PTREE AnalyseStmtExpr(PTREE expr)
{
    if(expr) {
        expr=run_traversals(expr);
        warnMeaningfulSideEffect(expr);
        expr=NodeDone(expr);
    }
    return expr;
}
PTREE IcEmitExpr(PTREE expr);
void EmitStmtExpr(PTREE expr)
{
    expr=AnalyseStmtExpr(expr);
    if(expr) IcEmitExpr(expr);
}

/* The1994 predicate rejects void/function pointer scaling; it has the
 * opposite sense of the later ptr_scales helper and no incomplete-array test. */
TYPE TypePointedAt(TYPE type,unsigned short *flags);
int ptrCannotScale(PTREE expr)
{
    unsigned short flags;
    TYPE type=TypePointedAt(expr->type,&flags);
    return type->id==TYP_FUNCTION || type->id==TYP_VOID;
}

TYPE TypePointedAtModified(TYPE type);
TYPE TypePointerDiff(TYPE type);
unsigned CgTypeSize(TYPE type);
PTREE NodeOffset(long offset);
PTREE bld_pted_size(PTREE pointer)
{
    TYPE pointed=TypePointedAtModified(pointer->type);
    return NodeOffset(CgTypeSize(pointed));
}
/* The old three-argument helper always builds/folds the scale operation;
 * the later source omits multiplication when the pointed size is one. */
PTREE bld_ptr_adj(PTREE factor,PTREE pointer,unsigned char operation)
{
    PTREE node;
    factor=NodeConvert(TypePointerDiff(pointer->type),factor);
    node=NodeBinary(operation,factor,bld_pted_size(pointer));
    node->type=factor->type;
    return FoldBinary(node);
}
