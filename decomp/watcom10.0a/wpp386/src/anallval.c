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

/* Adapted initial6b9cb443 anallval.c to the actual1994 lvalue dispatcher.
 * The old body also owns the member/qualification checks moved elsewhere
 * later, and its default success path does not set a later LV_CHECKED bit. */
#include "compiler.h"
#include "symbol.h"
#include "scope.h"
#include "ptree.h"
#include "pending.h"
#include <recon.h>
PTREE NodeThis(void);
PTREE PTreeOpLeft(PTREE expr);
PTREE NodeCDtorExtra(void);
SEARCH_RESULT *ScopeFindNakedConversion(SCOPE scope,TYPE type,unsigned short flags)
{ (void)scope; (void)type; (void)flags; STUB_TRAP(); return NULL; }
int checkConversionLookup(SEARCH_RESULT *result,PTREE name,PTREE expr)
{ (void)result; (void)name; (void)expr; STUB_TRAP(); return 0; }
void ScopeInsertErrorSym(SCOPE scope,PTREE id)
{ (void)scope; (void)id; STUB_TRAP(); }
PTREE PTreeErrorExprSym(PTREE expr,unsigned error,SYMBOL sym);
PTREE PTreeErrorNode(PTREE expr);
PTREE NodeSymbolNoRef(PTREE expr,SYMBOL sym,SEARCH_RESULT *result);
PTREE NodeSymbol(PTREE expr,SYMBOL sym,SEARCH_RESULT *result);
int SymIsError(SYMBOL sym);
int SymIsFunction(SYMBOL sym);
void PTreeErrorExpr(PTREE expr,unsigned error);
int checkIdLookup(SEARCH_RESULT *result,SCOPE start,PTREE id,PTREE expr)
{
    SYMBOL sym;
    if(!result) {
        ScopeInsertErrorSym(start,id);
        PTreeErrorExprSym(id,29,(SYMBOL)id->u.id.name);
        PTreeErrorNode(expr);
        return 0;
    }
    sym=result->sym_name->name_syms;
    if(!sym) {
        PTreeErrorExpr(id,356);
        PTreeErrorNode(expr);
        ScopeFreeResult(result);
        return 0;
    }
    if(SymIsError(sym)) {
        PTreeErrorNode(expr);
        ScopeFreeResult(result);
        return 0;
    }
    if(SymIsFunction(sym)) NodeSymbolNoRef(id,sym,result);
    else NodeSymbol(id,sym,result);
    return 1;
}
PTREE NodeBinary(unsigned char op,PTREE left,PTREE right);
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
PTREE thisPointsNode(PTREE expr)
{
    PTREE left=NodeThis();
    TYPE type;
    if (!left) PTreeErrorExpr(expr,374);
    else {
        PTreeCopySrcLocation(left,expr);
        type=expr->type;
        expr=NodeBinary(23,left,expr);
        expr->type=type;
        expr->flags|=2;
        PTreeCopySrcLocation(expr,left);
    }
    return expr;
}
int analyseFunction(PTREE expr,PTREE func);
int SymIsStaticDataMember(SYMBOL sym);
int SymIsEnumeration(SYMBOL sym);
PTREE reduceToRight(PTREE *expr);
int analyseBareSymbol(PTREE *expr);
void checkVolatileVar(PTREE expr);
int AnalyseThisDataItem(PTREE *expr);
int analyseMemberExpr(PTREE *a_expr)
{
    PTREE expr=*a_expr;
    SYMBOL sym;
    int result;
    if(expr->op==0) return 0;
    sym=expr->u.subtree[1]->u.symcg.symbol;
    expr->type=sym->sym_type;
    if(SymIsFunction(sym)) return analyseFunction(expr,expr->u.subtree[1]);
    if(SymIsStaticDataMember(sym) || SymIsEnumeration(sym)) {
        reduceToRight(a_expr);
        return analyseBareSymbol(a_expr);
    }
    result=AnalyseThisDataItem(a_expr);
    if(result) { checkVolatileVar(*a_expr); result=1; }
    return result;
}
int analyseBareSymbol(PTREE *expr);
int SymIsThisDataMember(SYMBOL sym);
int analyseSymbol(PTREE *a_expr)
{
    PTREE expr=*a_expr;
    if(SymIsThisDataMember(expr->u.symcg.symbol)) {
        if(expr->u.symcg.result->flags&4) {
            expr=thisPointsNode(expr);
            *a_expr=expr;
            return expr->op ? analyseMemberExpr(a_expr) : 0;
        }
        PTreeErrorExpr(expr,374);
        /* The old error branch retains its incoming result register.  This
         * unsupported erroneous-access domain is not a valid C result. */
        STUB_TRAP();
        return 0;
    }
    return analyseBareSymbol(a_expr);
}
TYPE analyseClPtrLeft(PTREE *expr) { (void)expr; STUB_TRAP(); return NULL; }
TYPE ClassTypeForType(TYPE type);
int TypeDefined(TYPE type);
int simpleTypeDtor(TYPE type,PTREE expr)
{ (void)type; (void)expr; STUB_TRAP(); return 0; }
TYPE LvalueErrLeft(PTREE left,PTREE expr)
{ (void)left; (void)expr; STUB_TRAP(); return NULL; }
void InfClassDecl(TYPE type) { (void)type; STUB_TRAP(); }
TYPE analyseClassLeft(PTREE *a_expr)
{
    PTREE expr=*a_expr,left=PTreeOpLeft(expr);
    TYPE type;
    if(left->flags&2) {
        type=left->type;
        if(!ClassTypeForType(type) && !simpleTypeDtor(type,expr)) {
            PTreeErrorExpr(left,40);
            PTreeErrorNode(expr);
            type=NULL;
        } else if(!TypeDefined(type)) {
            PTreeErrorExpr(left,516);
            InfClassDecl(type);
            PTreeErrorNode(expr);
            type=NULL;
        }
    } else type=LvalueErrLeft(left,expr);
    return type;
}
SCOPE TypeScope(TYPE type);
TYPE TypeVoidFunOfVoid(void);
PTREE NodePruneLeftTop(PTREE expr);
PTREE reduceToRight(PTREE *expr)
{
    PTREE right=NodePruneLeftTop(*expr);
    *expr=right;
    return right;
}
int analyseDtor(PTREE *expr,SCOPE scope,SCOPE disamb)
{ (void)expr; (void)scope; (void)disamb; STUB_TRAP(); return 0; }
SEARCH_RESULT *ScopeFindScopedMember(SCOPE start,SCOPE disamb,char *name);
SEARCH_RESULT *ScopeFindScopedMemberConversion(SCOPE start,SCOPE disamb,
                                               TYPE type,unsigned short flags);
PTREE PTreeOpRight(PTREE expr);
int analyseMember(PTREE *expr,SCOPE scope,SCOPE disamb)
{
    PTREE node=*expr,member=PTreeOpRight(node);
    SEARCH_RESULT *result;
    int ok;
    if(member->cgop==199) {
        result=ScopeFindScopedMemberConversion(scope,disamb,member->type,0);
        ok=checkConversionLookup(result,member,node);
    } else {
        result=ScopeFindScopedMember(scope,disamb,member->u.id.name);
        ok=checkIdLookup(result,scope,member,node);
    }
    return ok?analyseMemberExpr(expr):ok;
}
int NodeIsBinaryOp(PTREE tree,unsigned char operation);
int analyseMembRight(PTREE *a_expr,TYPE type)
{
    PTREE expr,right;
    SCOPE scope,disamb=NULL;
    if(!type) return 0;
    scope=TypeScope(type);
    expr=*a_expr;
    if(!scope) { expr->type=TypeVoidFunOfVoid(); return 1; }
    right=expr->u.subtree[1];
    if(right->op!=6) {
        if(!NodeIsBinaryOp(right,55)) {
            /* The original leaves its incoming result register unchanged
             * for this non-identifier domain. */
            STUB_TRAP(); return 0;
        }
        if(!right->u.subtree[0]) { PTreeErrorNode(expr); return 0; }
        disamb=TypeScope(right->u.subtree[0]->type);
        right=reduceToRight(&expr->u.subtree[1]);
        right->flags|=0x100;
    }
    if(right->cgop==197) return analyseDtor(a_expr,scope,disamb);
    return analyseMember(a_expr,scope,disamb);
}
SEARCH_RESULT *ScopeFindScopedNakedConversion(SCOPE start,SCOPE disamb,
                                             TYPE type,unsigned short flags);
SEARCH_RESULT *ScopeFindScopedNaked(SCOPE start,SCOPE disamb,char *name);
int analyseClQual(PTREE *a_expr)
{
    PTREE expr=*a_expr,right=expr->u.subtree[1];
    SCOPE start,disamb;
    SEARCH_RESULT *result;
    int ok;
    if(!expr->u.subtree[0]) { start=FileScope; disamb=NULL; }
    else {
        start=CurrScope;
        disamb=TypeScope(expr->u.subtree[0]->type);
    }
    if(right->cgop==199) {
        result=ScopeFindScopedNakedConversion(start,disamb,right->type,0);
        ok=checkConversionLookup(result,right,expr);
    } else {
        result=ScopeFindScopedNaked(start,disamb,right->u.id.name);
        ok=checkIdLookup(result,start,right,expr);
    }
    if(ok) reduceToRight(a_expr)->flags|=0x100;
    return ok;
}
int NodeIsBinaryOp(PTREE tree,unsigned char operation);
int NodeIsConstantInt(PTREE tree);
void diagnoseQualifiedName(PTREE tree,PTREE right)
{ (void)tree; (void)right; STUB_TRAP(); }
TYPE EnumType(TYPE type);
void PTreeErrorExpr(PTREE tree,unsigned error);

int AnalyseLvalue(PTREE *a_expr)
{
    PTREE expr=*a_expr,right;
    TYPE type;
    SEARCH_RESULT *result;
    int ok;
    switch(expr->op) {
    case 6:
        switch(expr->cgop) {
        case 196:
            right=NodeThis();
            if(!right) { PTreeErrorExpr(expr,204); return 0; }
            *a_expr=right;
            PTreeFree(expr);
            return 1;
        case 200:
            *a_expr=NodeCDtorExtra();
            PTreeFree(expr);
            return 1;
        case 197:
            PTreeErrorExpr(expr,538);
            return 0;
        case 199:
            result=ScopeFindNakedConversion(CurrScope,expr->type,0);
            ok=checkConversionLookup(result,expr,expr);
            break;
        default:
            result=ScopeFindNaked(CurrScope,expr->u.id.name);
            ok=checkIdLookup(result,CurrScope,expr,expr);
            break;
        }
        return ok ? analyseSymbol(a_expr):0;
    case 9:
        switch(expr->cgop) {
        case 23:
            if(!expr->type) return analyseMembRight(a_expr,analyseClPtrLeft(a_expr));
            break;
        case 48:
            if(!expr->type) return analyseMembRight(a_expr,analyseClassLeft(a_expr));
            break;
        case 55:
            if(!analyseClQual(a_expr)) return 0;
            return analyseSymbol(a_expr);
        case 52:
            type=expr->u.subtree[0]->type;
            if(!TypeDefined(type)) { PTreeErrorExpr(expr,383); return 0; }
            type=StructType(type);
            if(!type) { PTreeErrorExpr(expr,164); return 0; }
            if(!analyseMembRight(a_expr,type)) return 0;
            expr=*a_expr;
            if(NodeIsBinaryOp(expr,52)) {
                right=expr->u.subtree[1];
                if(NodeIsConstantInt(right)) {
                    *a_expr=right;
                    expr->u.subtree[1]=NULL;
                    PTreeFreeSubtrees(expr);
                    return 1;
                }
                diagnoseQualifiedName(expr,right);
                return 0;
            }
            if(EnumType(expr->type)) { PTreeErrorExpr(expr,381); return 0; }
            diagnoseQualifiedName(expr,expr);
            return 0;
        }
        break;
    }
    return 1;
}

typedef struct symbol_diag {
    int msg_no_this,msg_extra_this,msg_enclosing;
} SYMBOL_DIAG;
/* The encountered member-access diagnosis at original b1b28. */
static SYMBOL_DIAG diagMemb={374,501,503};
PTREE PTreeExtractLocn(PTREE expr,CGIO_LOCATION *location);
void ScopeResultErrLocn(SEARCH_RESULT *result,CGIO_LOCATION *location);
void SymSetNvReferenced(SYMBOL sym);
int SymIsClassMember(SYMBOL sym);
int SymIsStaticMember(SYMBOL sym);
int SymIsEnumeration(SYMBOL sym);
TYPE TypeThis(void);
SCOPE ScopeFunctionScopeInProgress(void);
SCOPE SymScope(SYMBOL sym);
unsigned char ScopeDerived(SCOPE derived,SCOPE base);
int requiresThis(SYMBOL sym)
{
    SCOPE class_scope;
    if(!SymIsClassMember(sym)) return 0;
    if(SymIsStaticMember(sym) || SymIsEnumeration(sym)) return 0;
    if(TypeThis()==NULL) return 1;
    class_scope=ScopeFunctionScopeInProgress()->enclosing;
    if(class_scope->id==1 && ScopeDerived(class_scope,SymScope(sym))) return 0;
    return 1;
}
int AnalyseSymbolAccess(PTREE expr,PTREE symbol,PTREE this_expr,SYMBOL_DIAG *diag)
{
    SEARCH_RESULT *result;
    CGIO_LOCATION location;
    PTreeExtractLocn(expr,&location);
    result=symbol->u.symcg.result;
    ScopeResultErrLocn(result,&location);
    SymSetNvReferenced(symbol->u.symcg.symbol);
    if(ScopeCheckSymbol(result,symbol->u.symcg.symbol)) {
        PTreeErrorNode(expr);
        return 0;
    }
    if(!this_expr) {
        if(result->flags&1) return 1;
        if(result->flags&8) return 1;
        if(requiresThis(symbol->u.symcg.symbol)) {
            PTreeErrorExpr(expr,diag->msg_no_this);
            return 0;
        }
        return 1;
    }
    return SymIsClassMember(symbol->u.symcg.symbol);
}
int ScopeImmediateCheck(SEARCH_RESULT *result);
int analyseFunction(PTREE expr,PTREE func)
{
    if(ScopeImmediateCheck(func->u.symcg.result)) {
        PTreeErrorNode(expr);
        return 0;
    }
    expr->flags|=2;
    return 1;
}
TYPE TypeModFlags(TYPE type,unsigned *flags);
void checkVolatileVar(PTREE expr)
{
    unsigned flags;
    TypeModFlags(expr->type,&flags);
    if(flags&2) expr->flags|=1;
}
int MainProcedure(SYMBOL sym);
int SymIsEnumeration(SYMBOL sym);
void NodeSetMemoryExact(PTREE expr);
PTREE NodeFetchReference(PTREE expr);
PTREE NodeFromConstSym(SYMBOL sym);
PTREE NodeConvert(TYPE type,PTREE expr);
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
void NodeFreeSearchResult(PTREE expr);
int analyseBareSymbol(PTREE *a_expr)
{
    PTREE expr=*a_expr,alias;
    SYMBOL sym=expr->u.symcg.symbol;
    int ok;
    if(sym->id==15) {
        alias=PTreeCopySrcLocation(PTreeAlloc(),expr);
        alias=NodeSymbol(alias,sym->u.sym,expr->u.symcg.result);
        alias=NodeConvert(sym->sym_type,alias);
        PTreeFree(expr);
        expr=alias;
        ok=1;
    } else if(SymIsFunction(sym)) {
        if(MainProcedure(sym)) {
            PTreeErrorExpr(expr,466);
            ok=0;
        } else ok=analyseFunction(expr,expr);
    } else {
        ok=AnalyseSymbolAccess(expr,expr,NULL,&diagMemb);
        NodeFreeSearchResult(expr);
        if(ok) {
            if(SymIsEnumeration(sym)) {
                sym=expr->u.symcg.symbol;
                PTreeFree(expr);
                expr=NodeFromConstSym(sym);
                expr->type=sym->sym_type;
            } else {
                NodeSetMemoryExact(expr);
                expr=NodeFetchReference(expr);
                checkVolatileVar(expr);
            }
            ok=1;
        }
    }
    *a_expr=expr;
    return ok;
}

TYPE TypeMergeForMember(TYPE object,TYPE member);
TYPE NodeType(PTREE expr);
PTREE *PTreeRefRight(PTREE expr);
PTREE NodeReplace(PTREE old,PTREE replacement);
PTREE NodePruneRight(PTREE expr);
PTREE NodePruneTop(PTREE expr);
void NodeConvertToBasePtr(PTREE *expr,TYPE type,SEARCH_RESULT *result,int exact);
PTREE NodeOffset(long offset);
PTREE NodeBinary(unsigned char op,PTREE left,PTREE right);
PTREE NodeUnaryCopy(unsigned char op,PTREE expr);
PTREE NodeFetchReference(PTREE expr);
int AnalyseThisDataItem(PTREE *a_expr)
{
    PTREE expr=*a_expr,*r_right=PTreeRefRight(expr),right=*r_right;
    SEARCH_RESULT *result;
    TYPE type,base;
    if(!AnalyseSymbolAccess(expr,right,expr->u.subtree[0],&diagMemb)) return 0;
    type=TypedefModifierRemove(NodeType(expr->u.subtree[0]))->of;
    type=TypeMergeForMember(type,expr->type);
    result=right->u.symcg.result;
    right->u.symcg.result=NULL;
    if(expr->u.subtree[0]->flags&8)
        *r_right=NodeReplace(right,NodeOffset(result->exact_delta+result->offset));
    else if(result->flags&2)
        *r_right=NodeReplace(right,NodeOffset(result->delta+result->offset));
    else {
        expr=NodePruneRight(expr);
        expr=NodePruneTop(expr);
        expr->flags|=0x200;
        NodeConvertToBasePtr(&expr,type,result,1);
        expr=NodeBinary(48,expr,NodeOffset(result->offset));
    }
    expr->type=type;
    expr->flags|=2;
    ScopeFreeResult(result);
    base=TypedefModifierRemove(type);
    if(base->id==TYP_BITFIELD) {
        expr=NodeUnaryCopy(101,expr);
        expr->type=base->of;
        expr->flags|=2;
    }
    NodeSetMemoryExact(expr);
    expr=NodeFetchReference(expr);
    *a_expr=expr;
    return 1;
}

int SymIsCtor(SYMBOL sym);
int SymIsDtor(SYMBOL sym);
int AnalyseLvalueAddrOf(PTREE *expr)
{
    SYMBOL sym;
    int result;
    if(!NodeIsBinaryOp(*expr,55)) return AnalyseLvalue(expr);
    result=analyseClQual(expr);
    if(result) {
        sym=(*expr)->u.symcg.symbol;
        if(SymIsFunction(sym)) {
            if(SymIsCtor(sym)) {
                PTreeErrorExpr(*expr,662);
                return 0;
            }
            if(SymIsDtor(sym)) {
                PTreeErrorExpr(*expr,663);
                return 0;
            }
            (*expr)->flags|=2;
            return 1;
        }
        if(SymIsThisDataMember(sym)) {
            (*expr)->flags|=2;
            return 1;
        }
        return analyseSymbol(expr);
    }
    return result;
}
