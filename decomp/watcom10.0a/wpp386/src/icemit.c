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


/* Initial6b9cb443 icemit.c, preserving the actual1994 expression path.
 * NodePromoteDups still receives the current emitted source position, and
 * LabelExprEnd receives the old dead-code result. */
#include "ptree.h"
#include "symbol.h"
#include "frontend.h"
#include "pending.h"
#include <recon.h>
static CGIO_LOCATION posn_gened;
void CgFrontCode(int opcode);
void LabelExprBegin(void);
void LabelExprEnd(int dead);
PTREE NodePromoteDups(PTREE expr,CGIO_LOCATION *position,CGIO_LOCATION *emitted);
void CgFrontDbgLine(CGIO_LOCATION *position);
void SrcPosnEmit(CGIO_LOCATION *position)
{
    if(position->src_file) {
        CgFrontDbgLine(position);
        posn_gened=*position;
    }
}
void SrcFileGetTokenLocn(CGIO_LOCATION *position);
CGIO_LOCATION SrcPosnEmitCurrent(void)
{
    CGIO_LOCATION position;
    SrcFileGetTokenLocn(&position);
    SrcPosnEmit(&position);
    return position;
}
void InitRefFixup(PTREE expr)
{ (void)expr; STUB_TRAP(); }
void CgSetType(TYPE type);
void CgSetTypeExact(TYPE type);
TYPE NodeType(PTREE expr);
void CgFrontCodeInt(int opcode,int value);
void generate_type(PTREE expr)
{
    if(expr->flags&0x400) CgSetTypeExact(expr->type);
    else CgSetType(expr->type);
}
TYPE generate_node_type(PTREE expr)
{
    TYPE type;
    if(expr->flags&0x400) {
        type=expr->type;
        CgSetTypeExact(type);
    } else {
        type=NodeType(expr);
        CgSetType(type);
    }
    return type;
}
void generate_type_instr(TYPE type,unsigned value,unsigned char opcode)
{
    CgSetType(type);
    CgFrontCodeInt(opcode,value);
}
void generate_expr_instr(PTREE expr,unsigned value,unsigned char opcode)
{
    generate_node_type(expr);
    CgFrontCodeInt(opcode,value);
}
/* The leaf emission path is recovered first. Other reached operator
 * families remain explicit pending semantics in both builds. This enclosing
 * callback is not accepted until its complete switch has been recovered. */
SYMBOL SymDeAlias(SYMBOL sym);
SYMBOL getPtreeSymbol(PTREE expr)
{
    SYMBOL sym=expr->u.symcg.symbol;
    return sym ? SymDeAlias(sym) : sym;
}
PTREE PTreeOpLeft(PTREE expr);
SYMBOL getLeftPtreeSymbol(PTREE expr)
{
    return getPtreeSymbol(PTreeOpLeft(expr));
}
void CgFrontSymbol(SYMBOL sym);
void CgFrontCodePtr(int opcode,void *value);
TYPE TypeModFlags(TYPE type,unsigned short *flags);
int handleSpecialLValue(PTREE expr)
{
    unsigned short flags;
    TYPE type=TypeModFlags(expr->type,&flags);
    if(flags&0x1002) return 1;
    return CompFlags.op_switch_used && type->id>=11 && type->id<=13;
}
void LabelExprScope(void);
PTREE PTreeOpRight(PTREE expr);
void CgSetTypeEmitted(TYPE type);
TYPE TypePointedAtModified(TYPE type);
TYPE FunctionDeclarationType(TYPE type);
unsigned char SpecialFunction(SYMBOL sym);
void LabelExprSetjmp(void);
int NodeIsUnaryOp(PTREE expr,unsigned char operation);
int NodeIsBinaryOp(PTREE expr,unsigned char operation);
TYPE ReferenceType(TYPE type);
PTREE NodeRemoveCasts(PTREE expr);
unsigned long NodeConstantValue(PTREE expr);
void CgFrontCodeUint(int opcode,unsigned value);
/* Actual1994 only tests the reference type, and recognizes the direct,
 * fetched and fixed-offset forms; no later vbase/ARG_THIS extension. */
void generateCallRefICs(PTREE expr)
{
    PTREE arg,right;
    unsigned index;
    for(expr=expr->u.subtree[1],index=0;expr;expr=expr->u.subtree[0],++index) {
        if(!ReferenceType(NodeType(expr))) continue;
        arg=NodeRemoveCasts(PTreeOpRight(expr));
        if(arg->op==7) {
            CgFrontCodePtr(98,getPtreeSymbol(arg));
        } else if(NodeIsUnaryOp(arg,111)) {
            arg=NodeRemoveCasts(PTreeOpLeft(arg));
            CgFrontCodePtr(97,getPtreeSymbol(arg));
        } else if(NodeIsBinaryOp(arg,48)) {
            right=PTreeOpRight(arg);
            arg=NodeRemoveCasts(PTreeOpLeft(arg));
            if(arg->op==7) {
                CgFrontCodePtr(98,getPtreeSymbol(arg));
                CgFrontCodeUint(100,NodeConstantValue(right));
            } else if(NodeIsUnaryOp(arg,111)) {
                arg=NodeRemoveCasts(PTreeOpLeft(arg));
                CgFrontCodePtr(97,getPtreeSymbol(arg));
                CgFrontCodeUint(100,NodeConstantValue(right));
            } else continue;
        } else continue;
        CgFrontCodeUint(99,index);
    }
}
void *ConPoolFloatAdd(PTREE expr);
TYPE generate_call_type(PTREE expr)
{
    TYPE type=expr->type,pointed=TypePointedAtModified(type),function;
    if(pointed) type=pointed;
    function=FunctionDeclarationType(type);
    if(function->flag&4) CgSetType(function->of);
    else CgSetTypeExact(function->of);
    return type;
}
void LabelCondTrue(void);
void LabelCondFalse(void);
void LabelCondEnd(void);
TYPE MakeNonInlineFunction(TYPE type);
void exprCondEnd(PTREE expr)
{
    if(expr->flags&0x800) LabelCondEnd();
}
PTREE emitNode(PTREE expr)
{
    switch(expr->op) {
    case 2:
    case 3:
        if(expr->cgop!=205)
            generate_expr_instr(expr,expr->u.uint_constant,19);
        break;
    case 7:
        if(expr->u.symcg.result) {
            ScopeFreeResult(expr->u.symcg.result);
            expr->u.symcg.result=NULL;
        }
        switch(expr->cgop) {
        case 196: generate_type(expr); CgFrontCode(23); break;
        case 201: case 202: case 205: break;
        case 200: generate_type(expr); CgFrontCode(24); break;
        default: CgFrontSymbol(getPtreeSymbol(expr)); break;
        }
        break;
    case 9:
        switch(expr->cgop) {
        case 13: case 25: case 26: case 27: case 28: case 29:
        case 30: case 31: case 32: case 33: case 34:
        case 72: case 105: case 106:
        {
            TYPE type=NodeType(expr);
            CgFrontCodePtr(29,type);
            if(handleSpecialLValue(expr)) CgFrontCodeInt(32,116);
            generate_type_instr(type,expr->cgop,31);
            break;
        }
        case 48:
            generate_type_instr(NodeType(expr),expr->cgop,31);
            break;
        case 23:
            generate_type_instr(expr->u.subtree[0]->type,expr->cgop,31);
            break;
        case 120:
            CgFrontCodeInt(78,PTreeOpRight(expr)->u.uint_constant);
            break;
        case 57:
        {
            PTREE operand=expr;
            /* Actual old bytes compare op, not cgop, to CO_CONVERT57. */
            do { operand=PTreeOpRight(operand); } while(operand->op==57);
            if(!(operand->flags&2))
                generate_expr_instr(expr,expr->cgop,32);
            break;
        }
        case 69:
            generate_type(expr);
            break;
        case 43: case 55:
            break;
        /* Named non-default1994 handlers not yet reconstructed. */
        case 14: case 15: case 16: case 17: case 18: case 19:
            generate_expr_instr(expr,expr->cgop,32);
            CgSetTypeEmitted(NodeType(expr));
            break;
        case 44:
            generate_node_type(expr);
            CgFrontCode(40);
            break;
        case 95:
            generateCallRefICs(expr);
            generate_node_type(expr);
            CgFrontCode(42);
            break;
        case 97:
            generate_node_type(expr);
            CgFrontCode(44);
            break;
        case 20: case 21:
            generate_expr_instr(expr,expr->cgop,31);
            exprCondEnd(expr);
            break;
        case 119:
            CgFrontCodePtr(45,getPtreeSymbol(expr->u.subtree[1]));
            break;
        case 41:
            generate_type_instr(NodeType(expr),expr->cgop,30);
            exprCondEnd(expr);
            break;
        case 24:
        case 99: case 100: case 107: case 108:
            STUB_TRAP();
            break;
        default:
            generate_expr_instr(expr,expr->cgop,31);
            break;
        }
        break;
    case 8:
        if(expr->cgop==92 || expr->cgop==112) {
            generate_type(expr->u.subtree[0]);
            if(handleSpecialLValue(expr)) CgFrontCodeInt(32,115);
            CgFrontCodeInt(32,expr->cgop);
        } else if(expr->cgop==111) {
            generate_type(expr);
            CgFrontCodePtr(96,getLeftPtreeSymbol(expr));
        } else if(expr->cgop==40) {
            unsigned short flags;
            TypeModFlags(expr->type,&flags);
            if(flags&0x10) CgFrontCodeInt(32,113);
        } else if(expr->cgop==94) {
            LabelExprScope();
            CgFrontCode(27);
        } else if(expr->cgop==96) {
            SYMBOL sym;
            generate_call_type(expr);
            sym=getLeftPtreeSymbol(expr);
            CgFrontCodePtr(41,sym);
            if(SpecialFunction(sym)==5) LabelExprSetjmp();
        } else if(expr->cgop==98) {
            TYPE type=generate_call_type(expr);
            TYPE pointed=TypePointedAtModified(type);
            if(pointed) type=pointed;
            CgFrontCodePtr(43,MakeNonInlineFunction(type));
        } else if(expr->cgop==10) generate_expr_instr(expr,expr->cgop,32);
        else if(expr->cgop==117) CgFrontCodePtr(107,expr->type);
        else if(expr->cgop==93) CgFrontCode(26);
        else if(expr->cgop!=12 && expr->cgop!=49) STUB_TRAP();
        break;
    case 11:
        if(expr->u.ic.opcode==66) LabelCondTrue();
        else if(expr->u.ic.opcode==67) LabelCondFalse();
        else {
            generate_type(expr);
            CgFrontCodePtr(expr->u.ic.opcode,(void *)expr->u.ic.value);
        }
        break;
    case 4:
        generate_type(expr);
        CgFrontCodePtr(20,ConPoolFloatAdd(expr));
        break;
    case 1:
        STUB_TRAP();
        break;
    default:
        break;
    }
    return expr;
}
int NodeIsUnaryOp(PTREE expr,unsigned char operation);
int NodeIsBinaryOp(PTREE expr,unsigned char operation);
PTREE PTreeOp(PTREE *expr);
int IsPragmaAborts(SYMBOL sym);
int RunTimeIsThrow(SYMBOL sym);
void FunctionBodyDeadCode(void);
int checkDeadCode(PTREE expr)
{
    SYMBOL sym;
    if(NodeIsUnaryOp(expr,94)) expr=expr->u.subtree[0];
    expr=PTreeOp(&expr);
    if(NodeIsBinaryOp(expr,95)) {
        expr=PTreeOp(&expr->u.subtree[0]->u.subtree[0]);
        if(expr->op==7) {
            sym=getPtreeSymbol(expr);
            if(IsPragmaAborts(sym) || RunTimeIsThrow(sym)) {
                FunctionBodyDeadCode();
                CgFrontCode(108);
                return 1;
            }
        }
    }
    return 0;
}
int NodeIsUnaryOp(PTREE expr,unsigned char operation);
int NodeIsBinaryOp(PTREE expr,unsigned char operation);
TYPE ReferenceType(TYPE type);
PTREE IcEmitExpr(PTREE expr)
{
    CGIO_LOCATION posn;
    if(expr) {
        LabelExprBegin();
        expr=NodePromoteDups(expr,&posn,&posn_gened);
        SrcPosnEmit(&posn);
        if(NodeIsUnaryOp(expr,94) && NodeIsBinaryOp(expr->u.subtree[0],72) &&
           ReferenceType(expr->u.subtree[0]->u.subtree[0]->type))
            InitRefFixup(expr->u.subtree[0]);
        expr=PTreeTraversePostfix(expr,emitNode);
        LabelExprEnd(checkDeadCode(expr));
        PTreeFreeSubtrees(expr);
    }
    return expr;
}

PTREE NodeDone(PTREE tree);
void EmitAnalysedStmt(PTREE tree)
{
    if (tree) IcEmitExpr(NodeDone(tree));
}

PTREE NodeDtorExpr(PTREE expr,SYMBOL sym);
PTREE NodeUnaryCopy(unsigned char operation,PTREE expr);
/* Old single symbol marker predates the released auto/static split. */
void IcEmitDtorSym(SYMBOL sym)
{
    PTREE expr;
    expr=NodeDtorExpr(PTreeIntConstant(12345,TYP_SINT),sym);
    expr=NodeUnaryCopy(93,expr);
    IcEmitExpr(expr);
}
