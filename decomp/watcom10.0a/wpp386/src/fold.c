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


/* Reached cold CShift dependency: actual1994 CF3b7dd..3b7e7. */
#include "ptree.h"
#include "compiler.h"
#include "pending.h"
#include <recon.h>
long FoldSignedRShiftMax(long value)
{
    if (value<0) return -1;
    return 0;
}

int NodeIsBinaryOp(PTREE expr,unsigned char operation);
PTREE NodeRemoveCasts(PTREE expr);
int isCondDecor(PTREE expr)
{
    PTREE left;
    if(!NodeIsBinaryOp(expr,42)) return 0;
    left=expr->u.subtree[0];
    return left->op==11 &&
        (left->u.uint_constant==66 || left->u.uint_constant==67);
}
PTREE overCondDecor(PTREE expr)
{
    if(isCondDecor(expr)) expr=expr->u.subtree[1];
    return expr;
}
int zeroConstant(PTREE expr)
{
    PTREE original;
    for(;;) {
        switch(expr->op) {
        case 2: case 3: return expr->u.uint_constant==0;
        case 4:
            /* The old floating test checks just the high IEEE word, unlike
             * the later big-float backend. Preserve that original test. */
            return expr->u.floating_bits.high==0;
        case 9:
            original=expr;
            expr=NodeRemoveCasts(expr);
            if(expr!=original) continue;
            break;
        }
        return 0;
    }
}
int notFoldable(PTREE expr)
{
    if(expr->op>=2 && expr->op<=4) return 0;
    return !zeroConstant(expr);
}
PTREE castConstant(PTREE expr,TYPE type,int *happened);
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
void NodeFreeDupedExpr(PTREE expr);
PTREE foldInt(unsigned char operation,PTREE left,long right);
int nonZeroExpr(PTREE expr);
PTREE makeTrueFalse(PTREE expr,PTREE operand,int value);
PTREE FoldBinary(PTREE expr)
{
    PTREE original1=expr->u.subtree[0],original2=expr->u.subtree[1];
    PTREE op1=overCondDecor(original1),op2=overCondDecor(original2);
    int decorated=op1!=original1;
    (void)decorated;
    /* This discovery prefix is not accepted. The old constant/operator
     * cases below this non-foldable fast path remain to be reconstructed. */
    if(notFoldable(op1)) {
        if(notFoldable(op2)) return expr;
        if(expr->cgop==57) {
            int happened;
            PTREE converted=castConstant(op2,expr->type,&happened);
            if(happened) {
                converted=PTreeCopySrcLocation(converted,expr);
                NodeFreeDupedExpr(op1);
                PTreeFree(expr);
                return converted;
            }
            return expr;
        }
        switch(expr->cgop) {
        case 14:
            if(zeroConstant(op2)) {
                if(zeroConstant(op1)) return makeTrueFalse(expr,op1,1);
                if(nonZeroExpr(op1)) return makeTrueFalse(expr,op1,0);
            }
            return expr;
        case 15:
            if(zeroConstant(op2)) {
                if(zeroConstant(op1)) return makeTrueFalse(expr,op1,0);
                if(nonZeroExpr(op1)) return makeTrueFalse(expr,op1,1);
            }
            return expr;
        case 13: case 25: case 26:
        case 29: case 30: case 31: case 32:
            if(ArithType(expr->type)) {
                int happened;
                expr->u.subtree[1]=castConstant(op2,expr->type,&happened);
            }
            return expr;
        case 42:
            STUB_TRAP(); return NULL;
        default:
            return expr;
        }
    }
    /* Actual old driver promotes unlike integer constant kinds before
     * dispatch. Other constant domains remain pending discovery paths. */
    if(notFoldable(op2)) { STUB_TRAP(); return NULL; }
    if(op1->op!=op2->op) { STUB_TRAP(); return NULL; }
    if(op1->op==2) {
        int happened;
        PTREE result=foldInt(expr->cgop,op1,op2->u.int_constant);
        if(result==NULL) return expr;
        if(decorated) {
            original1->u.subtree[1]=NULL;
            original2->u.subtree[1]=NULL;
        } else {
            /* The1994 driver detaches BOTH operand owners here. */
            expr->u.subtree[0]=NULL;
            expr->u.subtree[1]=NULL;
        }
        result=castConstant(result,expr->type,&happened);
        result=PTreeCopySrcLocation(result,expr);
        NodeFreeDupedExpr(expr);
        return result;
    }
    STUB_TRAP(); return NULL;
}
void __cdecl CErr(unsigned number,...);
PTREE foldInt(unsigned char operation,PTREE left,long right)
{
    long value=left->u.int_constant;
    double test;
    switch(operation) {
    case 0: value+=right; break;
    case 1: value-=right; break;
    case 2:
        test=(double)value*(double)right;
        value*=right;
        if(test!=value) CErr(130);
        break;
    case 3:
        if(right==0) { CErr1(129); value=1; }
        else value/=right;
        break;
    case 4:
        if(right==0) { CErr1(129); value=1; }
        else value%=right;
        break;
    case 5: value&=right; break;
    case 6: value|=right; break;
    case 7: value^=right; break;
    case 8:
        if((unsigned long)right>=32) value=FoldSignedRShiftMax(value);
        else value>>=right;
        break;
    case 9:
        if((unsigned long)right>=32) value=0;
        else value<<=right;
        break;
    /* The old compiler makes these boolean results in place, rather than
     * calling the later released makeBooleanConst helper. */
    case 14: value=value==right; left->flags|=4; break;
    case 15: value=value!=right; left->flags|=4; break;
    case 16: value=value>right; left->flags|=4; break;
    case 17: value=value<=right; left->flags|=4; break;
    case 18: value=value<right; left->flags|=4; break;
    case 19: value=value>=right; left->flags|=4; break;
    case 20: value=value&&right; left->flags|=4; break;
    case 21: value=value||right; left->flags|=4; break;
    case 42: value=right; break;
    default: return NULL;
    }
    left->u.int_constant=value;
    left->op=2;
    return left;
}

int anachronismFound(PTREE expr)
{ (void)expr; STUB_TRAP(); return 0; }
int nonZeroExpr(PTREE expr)
{
    PTREE old;
    for(;;) {
        switch(expr->op) {
        case 2: case 3:
            if(expr->u.int_constant!=0) return 1;
            break;
        case 4:
            if(expr->u.floating_bits.high!=0) return 1;
            break;
        case 7: return 1;
        case 9:
            old=expr;
            expr=NodeRemoveCasts(expr);
            if(expr!=old) continue;
            break;
        }
        return (expr->flags&0x200)!=0;
    }
}
PTREE NodeOffset(unsigned offset);
PTREE makeTrueFalse(PTREE expr,PTREE operand,int value)
{
    PTREE result;
    if(operand->flags&0x10) return expr;
    if(anachronismFound(operand)) return expr;
    result=NodeOffset(value);
    result->flags|=4;
    PTreeCopySrcLocation(result,expr);
    NodeFreeDupedExpr(expr);
    return result;
}
PTREE FoldUnary(PTREE expr)
{
    PTREE operand=expr->u.subtree[0];
    TYPE result_type;
    int happened;
    if(notFoldable(operand)) {
        if(expr->cgop==22 && nonZeroExpr(operand))
            expr=makeTrueFalse(expr,operand,0);
        return expr;
    }
    result_type=expr->type;
    if(operand->op==4) {
        switch(expr->cgop) {
        case 22:
            /* Actual old zero test uses only the high IEEE word. */
            operand->u.int_constant=operand->u.floating_bits.high==0;
            operand->op=2;
            operand->flags|=4;
            break;
        case 10:
            operand->u.floating_constant=-operand->u.floating_constant;
            break;
        case 35: break;
        default: return expr;
        }
    } else {
        switch(expr->cgop) {
        case 11: operand->u.int_constant=~operand->u.int_constant; break;
        case 22:
            operand->op=2;
            operand->u.int_constant=operand->u.int_constant==0;
            operand->flags|=4;
            break;
        case 10: operand->u.int_constant=-operand->u.int_constant; break;
        case 35: break;
        default: return expr;
        }
    }
    operand=PTreeCopySrcLocation(operand,expr);
    PTreeFree(expr);
    return castConstant(operand,result_type,&happened);
}

TYPE TypedefModifierRemoveOnly(TYPE type);
TYPE TypedefModifierRemove(TYPE type);
PTREE PTreeUIntConstant(unsigned long value,type_id id);
PTREE PTreeFloatingConstant(double value,type_id id);
PTREE CastIntConstant(PTREE expr,TYPE type,int *happened)
{
    unsigned long value=expr->u.uint_constant;
    type_id id=TypedefModifierRemove(type)->id;
    PTREE result;
    float float_value;
    double double_value;
    switch(id) {
    case TYP_SCHAR:
        value=(signed char)value;
        /* Fall through to the old target-width signed conversions. */
    case TYP_SSHORT:
        value=(short)value;
    case TYP_SINT: case TYP_SLONG:
        result=PTreeIntConstant(value,id);
        break;
    case TYP_UCHAR:
        value=(unsigned char)value;
    case TYP_USHORT:
        value=(unsigned short)value;
    case TYP_UINT: case TYP_POINTER: case TYP_MEMBER_POINTER:
        result=PTreeUIntConstant(value,TYP_ULONG);
        result->type=type;
        break;
    case TYP_ULONG:
        result=PTreeUIntConstant(value,id);
        break;
    case TYP_WCHAR:
        value=(unsigned short)value;
        result=PTreeUIntConstant(value,id);
        break;
    case TYP_FLOAT:
        if(expr->op==2) float_value=(float)expr->u.int_constant;
        else float_value=(float)expr->u.uint_constant;
        result=PTreeFloatingConstant(float_value,id);
        break;
    case TYP_DOUBLE: case TYP_LONG_DOUBLE:
        if(expr->op==2) double_value=(double)expr->u.int_constant;
        else double_value=(double)expr->u.uint_constant;
        result=PTreeFloatingConstant(double_value,id);
        break;
    default:
        return expr;
    }
    *happened=1;
    result->flags=expr->flags;
    result=PTreeCopySrcLocation(result,expr);
    PTreeFree(expr);
    return result;
}
PTREE castFloatingConstant(PTREE expr,TYPE type,int *happened)
{ (void)expr; (void)type; (void)happened; STUB_TRAP(); return NULL; }
PTREE castConstant(PTREE expr,TYPE type,int *happened)
{
    TYPE final_type;
    *happened=0;
    if(notFoldable(expr)) return expr;
    final_type=TypedefModifierRemoveOnly(type);
    type=TypedefModifierRemove(final_type);
    switch(expr->op) {
    case 2: case 3:
        expr=CastIntConstant(expr,type,happened);
        expr->type=final_type;
        break;
    case 4:
        expr=castFloatingConstant(expr,type,happened);
        expr->type=final_type;
        break;
    }
    return expr;
}
