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

/* Adapted initial6b9cb443 analnode.c, with actual1994 effect bits0x11. */
#include <stdarg.h>
#include "compiler.h"
#include "ptree.h"
#include "symbol.h"
#include "pending.h"
#include "frontend.h"
#include "convdiag.h"
#include <recon.h>
PTREE PTreeErrorExpr(PTREE expr,unsigned message);
unsigned short PTreeEffFlags(PTREE tree);
PTREE PTreeBinary(unsigned char cgop,PTREE left,PTREE right);
PTREE set_meaningful_side_effects(PTREE node)
{
    if(node->op==8) node->flags|=PTreeEffFlags(node->u.subtree[0]);
    else if(node->op==9)
        node->flags|=PTreeEffFlags(node->u.subtree[1])|PTreeEffFlags(node->u.subtree[0]);
    return node;
}
PTREE NodeBinary(unsigned char cgop,PTREE left,PTREE right)
{
    return set_meaningful_side_effects(PTreeBinary(cgop,left,right));
}

int NodeIsUnaryOp(PTREE node,unsigned char operation)
{
    return node && node->op==8 && node->cgop==operation;
}

int NodeIsBinaryOp(PTREE node,unsigned char operation)
{
    return node && node->op==9 && node->cgop==operation;
}

PTREE PTreeOpLeft(PTREE tree);
PTREE PTreeOpRight(PTREE tree);
int IsActualOverloadedFunc(SYMBOL sym)
{ (void)sym; STUB_TRAP(); return 0; }
int SymIsFunction(SYMBOL sym);
int classifyFunctionSymbol(PTREE node)
{
    SYMBOL sym;
    if(node->op!=7) return 0;
    sym=(SYMBOL)node->u.subtree[0];
    if(!sym || !SymIsFunction(sym)) return 0;
    return IsActualOverloadedFunc(sym) ? 2:1;
}
/* Recovered1994 classification: address-of unique/overloaded function=1/2,
 * plain or member unique/overloaded function=3/4, nonfunction=0. */
int NodeFunctionOperand(PTREE node,PTREE *function)
{
    PTREE actual=node;
    int kind;
    if(!node) { *function=NULL; return 0; }
    if(NodeIsUnaryOp(node,12)) {
        actual=PTreeOpLeft(node);
        kind=classifyFunctionSymbol(actual);
        if(kind!=1 && kind!=2) actual=NULL;
        *function=actual;
        return kind;
    }
    if(NodeIsBinaryOp(node,23) || NodeIsBinaryOp(node,48))
        actual=PTreeOpRight(node);
    kind=classifyFunctionSymbol(actual);
    if(kind==1 || kind==2) {
        *function=actual;
        return kind+2;
    }
    *function=NULL;
    return 0;
}

TYPE IntegralType(TYPE type);
unsigned SymIsConstantInt(SYMBOL sym);
PTREE NodeGetConstantNode(PTREE node)
{
    for(;;) {
        if(NodeIsBinaryOp(node,57)) {
            if(!IntegralType(node->type)) { node=NULL; break; }
            node=node->u.subtree[1];
        } else if(NodeIsBinaryOp(node,42)) node=node->u.subtree[1];
        else break;
    }
    if(node) {
        switch(node->op) {
        case 2: case 3:
            if(!IntegralType(node->type)) node=NULL;
            break;
        case 7:
            if(SymIsConstantInt((SYMBOL)node->u.subtree[0]))
                node=PTreeOpRight(node);
            else node=NULL;
            break;
        default: node=NULL; break;
        }
    }
    return node;
}

PTREE PTreeOp(PTREE *node);
PTREE NodeRemoveCasts(PTREE node)
{
    while(NodeIsBinaryOp(node,57)) node=PTreeOpRight(node);
    return node;
}
int TypeIsConst(TYPE type);
/* Actual old constant-int test accepts the two integer constant node kinds
 * directly, without the later redundant IntegralType call. */
int NodeIsConstantInt(PTREE node)
{
    if(node) {
        node=NodeRemoveCasts(PTreeOp(&node));
        if(node->op==2 || node->op==3) return 1;
        if(node->op==7) return SymIsConstantInt((SYMBOL)node->u.subtree[0]);
    }
    return 0;
}
int NodeIsConstant(PTREE node)
{
    if(TypeIsConst(node->type)) return 1;
    return node->op>=1 && node->op<=4;
}
unsigned long NodeConstantValue(PTREE node);
int NodeIsZeroConstant(PTREE node)
{
    node=NodeRemoveCasts(node);
    return NodeIsConstantInt(node) && NodeConstantValue(node)==0;
}

TYPE ReferenceType(TYPE type);
TYPE ArrayType(TYPE type);
TYPE PointerTypeForArray(TYPE type);
TYPE MakeReferenceTo(TYPE type);
TYPE NodeType(PTREE node)
{
    TYPE type=node->type;
    if(node->flags&2) {
        if(!FunctionDeclarationType(type) && !ReferenceType(type)) {
            if(ArrayType(type)) type=PointerTypeForArray(type);
            else type=MakeReferenceTo(type);
        }
    }
    return type;
}

PTREE *nodePossibleTemp(PTREE *expr,PTREE *dtor)
{
    PTREE node;
    *dtor=NULL;
    node=*expr;
    for(;;) {
        if(NodeIsBinaryOp(node,57) || NodeIsBinaryOp(node,42)) {
            expr=&node->u.subtree[1];
            node=*expr;
            continue;
        }
        if(!NodeIsBinaryOp(node,99)) return expr;
        *dtor=node;
        expr=&node->u.subtree[1];
        node=*expr;
    }
}
PTREE NodeFuncForCall(PTREE expr)
{ (void)expr; STUB_TRAP(); return NULL; }
TYPE TypeFunctionCalled(TYPE type)
{ (void)type; STUB_TRAP(); return NULL; }
int SymIsCtor(SYMBOL sym);
int SymIsAssign(SYMBOL sym);
TYPE SymFuncReturnType(SYMBOL sym);
int nodeMakesTemporary(PTREE expr)
{
    SYMBOL sym;
    if(!(expr->flags&2)) return 0;
    if(NodeIsBinaryOp(expr,95)) {
        sym=NodeFuncForCall(expr)->u.symcg.symbol;
        if(SymIsCtor(sym)) return 1;
        if(ReferenceType(SymFuncReturnType(sym))) return 0;
        return !SymIsAssign(sym);
    }
    if(NodeIsBinaryOp(expr,97)) {
        TYPE called=TypeFunctionCalled(NodeFuncForCall(expr)->type);
        return ReferenceType(called->of)==NULL;
    }
    return 0;
}
void PTreeWarnExpr(PTREE expr,unsigned warning);
int isNonConstRef(TYPE type)
{
    TYPE ref=ReferenceType(type);
    return ref && !TypeIsConst(ref);
}
/* Old target warns and returns false; it has neither the later thunk test
 * nor the later diagnostic that can turn the node into PT_ERROR. */
int NodeNonConstRefToTemp(TYPE arg_type,PTREE node)
{
    PTREE dtor;
    PTREE *source;
    if(isNonConstRef(arg_type)) {
        source=nodePossibleTemp(&node,&dtor);
        if(nodeMakesTemporary(*source)) PTreeWarnExpr(node,665);
    }
    return 0;
}

SYMBOL SymMarkRefed(SYMBOL sym);
TYPE StructType(TYPE type);
void ScopeResultErrLocn(SEARCH_RESULT *result,CGIO_LOCATION *location)
{
    if(location && location->src_file) result->errlocn=*location;
}
PTREE NodeSymbolNoRef(PTREE expr,SYMBOL sym,SEARCH_RESULT *result)
{
    /* The old entry requires an allocated node; MakeNodeSymbol supplies it.
     * The later NULL-allocating form and LV_CHECKED bit are absent. */
    expr->op=7;
    expr->u.symcg.result=result;
    expr->u.symcg.symbol=sym;
    expr->flags|=2;
    expr->cgop=194;
    expr->type=sym->sym_type;
    if(result) {
        ScopeResultErrLocn(result,&expr->locn);
        if(result->sym) expr->cgop=199;
    }
    return expr;
}
PTREE MakeNodeSymbol(SYMBOL sym)
{
    sym=SymMarkRefed(sym);
    if(StructType(sym->sym_type)) sym->flag|=8;
    return NodeSymbolNoRef(PTreeAlloc(),sym,NULL);
}
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
PTREE NodeAssign(PTREE target,PTREE source)
{
    PTREE expr=NodeBinary(13,target,source);
    expr->type=target->type;
    expr->flags|=2;
    return PTreeCopySrcLocation(expr,source);
}

int TypesIdentical(TYPE first,TYPE second);
PTREE FoldBinary(PTREE expr);
PTREE nodeMakeConvert(TYPE type,PTREE expr)
{
    PTREE original=expr,cast;
    TYPE ref;
    cast=PTreeType(type);
    PTreeCopySrcLocation(cast,original);
    expr=NodeBinary(57,cast,original);
    PTreeCopySrcLocation(expr,original);
    ref=ReferenceType(type);
    if(ref) { type=ref; expr->flags|=2; }
    expr->type=type;
    return FoldBinary(expr);
}
PTREE NodeConvert(TYPE type,PTREE expr)
{
    if(!TypesIdentical(NodeType(expr),type) ||
       ((expr->flags&2) && !ReferenceType(type)))
        expr=nodeMakeConvert(type,expr);
    return expr;
}

PTREE PTreeUnary(unsigned char op,PTREE operand);
PTREE NodeUnary(unsigned char op,PTREE operand)
{
    return set_meaningful_side_effects(PTreeUnary(op,operand));
}
PTREE NodeUnaryCopy(unsigned char op,PTREE operand)
{
    PTREE expr=NodeUnary(op,operand);
    expr->type=operand->type;
    expr->flags=operand->flags;
    PTreeCopySrcLocation(expr,operand);
    return expr;
}
int nodeIsAssignment(PTREE expr)
{ (void)expr; STUB_TRAP(); return 0; }
void PTreeAssign(PTREE target,PTREE source);
void NodeUnduplicate(PTREE node)
{
    PTREE subtree=node->u.subtree[0],partner;
    if (subtree) {
        node->u.subtree[0]=NULL;
        partner=node->u.subtree[1];
        PTreeAssign(partner,subtree);
        subtree->op=0;
        PTreeFree(subtree);
    }
}
void ScopeFreeResult(SEARCH_RESULT *result);
void NodeFreeSearchResult(PTREE node)
{
    if (node->op==7) {
        ScopeFreeResult(node->u.symcg.result);
        node->u.symcg.result=NULL;
    }
}
PTREE fixupTraverse(PTREE node)
{
    if (node->op==10) NodeUnduplicate(node);
    else if (node->op==7) NodeFreeSearchResult(node);
    return node;
}
PTREE PTreeTraversePrefix(PTREE tree,PTREE (*callback)(PTREE));
void NodeFreeDupedExpr(PTREE expr)
{
    PTreeFreeSubtrees(PTreeTraversePrefix(expr,fixupTraverse));
}
PTREE nodeSimplifyComma(PTREE expr)
{
    PTREE left,right,inner,old;
    if(!NodeIsBinaryOp(expr,42)) return expr;
    right=expr->u.subtree[1];
    if(right->op==7) {
        left=PTreeOpLeft(expr);
        if(!nodeIsAssignment(left)) return expr;
        inner=PTreeOpLeft(left);
        if(inner->op!=7 || inner->u.symcg.symbol!=right->u.symcg.symbol)
            return expr;
    } else if(right->op==10 && right->u.subtree[0]==NULL) {
        left=PTreeOpLeft(expr);
        if(!nodeIsAssignment(left)) return expr;
        inner=PTreeOpLeft(left);
        if(inner->op!=10 || inner->u.subtree[1]!=right->u.subtree[1])
            return expr;
        left->u.subtree[0]=inner->u.subtree[0];
        PTreeFree(inner);
    } else return expr;
    old=expr;
    expr=expr->u.subtree[0];
    old->u.subtree[0]=NULL;
    NodeFreeDupedExpr(old);
    return expr;
}
PTREE NodeDone(PTREE expr)
{
    if(expr->op!=0) expr=NodeUnaryCopy(94,nodeSimplifyComma(expr));
    return expr;
}

PTREE NodeArg(PTREE expr);

TYPE TypeThis(void);
PTREE PTreeThis(void);
PTREE nodeSimplifyComma(PTREE expr);
PTREE PTreeOpRight(PTREE expr);
PTREE nodeRefedRvalue(PTREE *start);
PTREE NodeRvalueLeft(PTREE expr) { return nodeRefedRvalue(&expr->u.subtree[0]); }
PTREE NodeRvalueRight(PTREE expr) { return nodeRefedRvalue(&expr->u.subtree[1]); }
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
PTREE *PTreeRef(PTREE *target);
PTREE nodeDoFetch(PTREE current,unsigned char opcode)
{
    PTREE *ref,fetched,old;
    if (NodeIsBinaryOp(current,42)) {
        ref=PTreeRef(&current->u.subtree[1]);
        if (!NodeIsUnaryOp(*ref,101)) ref=&current;
    } else ref=&current;
    old=*ref;
    fetched=NodeUnary(opcode,old);
    *ref=fetched;
    fetched->flags|=old->flags&0x61b;
    fetched->type=old->type;
    set_meaningful_side_effects(fetched);
    PTreeCopySrcLocation(fetched,old);
    current->flags&=0x61b;
    return current;
}
PTREE nodeDoFetchRef(PTREE expr)
{
    expr->cgop=202;
    return nodeDoFetch(expr,111);
}
PTREE NodeStripPoints(PTREE expr)
{
    PTREE left=PTreeOpLeft(expr),original,original_left;
    if(NodeIsUnaryOp(left,12)) {
        original=expr;
        original_left=PTreeOpLeft(original);
        if(original_left==left) expr=left->u.subtree[0];
        else {
            expr=original_left;
            expr->u.subtree[1]=left->u.subtree[0];
        }
        PTreeFree(original);
        PTreeFree(left);
    }
    return expr;
}
PTREE NodeFetch(PTREE expr) { return NodeStripPoints(nodeDoFetch(expr,92)); }
TYPE TypeReferenced(TYPE type);
TYPE TypedefModifierRemoveOnly(TYPE type);
TYPE ReferenceType(TYPE type);
unsigned char ObjModelArgument(TYPE type);
PTREE nodeRvalueArray(PTREE expr);
PTREE nodeRvalueFetch(PTREE expr)
{
    TYPE unmod,type,ref;
    if(ArrayType(expr->type)) return nodeRvalueArray(expr);
    unmod=TypeReferenced(NodeType(expr));
    type=TypedefModifierRemoveOnly(unmod);
    switch(type->id) {
    case TYP_ARRAY: return nodeRvalueArray(expr);
    case TYP_CLASS:
        if(ObjModelArgument(type)!=3) return NodeConvert(unmod,expr);
        ref=ReferenceType(expr->type);
        if(ref) {
            expr=NodeConvert(ref,expr);
            expr->flags|=2;
        }
        break;
    case TYP_FUNCTION:
        if(expr->op==7) SymMarkRefed(expr->u.symcg.symbol);
        return expr;
    }
    expr=NodeFetch(expr);
    expr->flags&=~2;
    return expr;
}
SCOPE SymScope(SYMBOL sym);
PTREE nodeRvalueArray(PTREE expr)
{
    PTREE original=expr;
    expr=nodeMakeConvert(PointerTypeForArray(expr->type),expr);
    if(original->op==7 && ScopeType(SymScope(original->u.symcg.symbol),2))
        expr=NodeFetch(expr);
    return expr;
}
PTREE NodeFromConstSym(SYMBOL sym) { (void)sym; STUB_TRAP(); return NULL; }
unsigned SymIsConstantInt(SYMBOL sym);
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
TYPE TypeModFlags(TYPE type,unsigned short *flags);
TYPE ArrayType(TYPE type);
PTREE NodeRvalue(PTREE expr)
{
    PTREE current=nodeSimplifyComma(expr),result=current,left,right,colon;
    unsigned short flags;
    if (current->flags&2) {
        if (NodeIsBinaryOp(current,41)) {
            colon=PTreeOpRight(current);
            left=NodeRvalueLeft(colon);
            right=NodeRvalueRight(colon);
            colon->flags&=~2;
            flags=(left->flags|right->flags)&1;
            colon->flags|=flags;
            current->flags|=flags;
            current->flags&=~2;
        } else {
            TypeModFlags(current->type,&flags);
            if (current->op==7 && current->cgop==196) {
                if (CompFlags.extensions_enabled) result=nodeDoFetchRef(current);
                else result=NodeFetch(current);
                result->flags&=~2;
            } else if (current->op==7 && current->cgop==200) {
                result=NodeFetch(current);
                result->flags&=~2;
            } else if (current->op==7 && SymIsConstantInt(current->u.symcg.symbol)) {
                result=NodeFromConstSym(current->u.symcg.symbol);
                PTreeCopySrcLocation(result,current);
                PTreeFree(current);
            } else result=nodeRvalueFetch(current);
            if (flags&2) result->flags|=1;
        }
    } else if (ArrayType(current->type)) result=nodeRvalueArray(current);
    return result;
}
/*1994 has a separate lvalue constructor; the released NodeThis inlines it. */
PTREE NodeThisLvalue(void)
{
    TYPE type=TypeThis();
    PTREE node=NULL;
    if (type) {
        node=PTreeThis();
        node->op=7;
        node->u.symcg.symbol=NULL;
        node->u.symcg.result=NULL;
        node->type=type;
        node->flags|=0x202;
    }
    return node;
}
PTREE NodeThis(void)
{
    PTREE node=NodeThisLvalue();
    if (node) { node=NodeRvalue(node); node->flags|=0x200; }
    return node;
}
PTREE PTreeIntConstant(long value,type_id type);
PTREE NodeOffset(long offset) { return PTreeIntConstant(offset,TYP_UINT); }

PTREE NodeSymbol(PTREE expr,SYMBOL sym,SEARCH_RESULT *result)
{
    return NodeSymbolNoRef(expr,SymMarkRefed(sym),result);
}

TYPE ReferenceType(TYPE type);
unsigned char ObjModelArgument(TYPE type);
int SymIsArgument(SYMBOL sym);
void NodeSetMemoryExact(PTREE expr)
{
    if(StructType(expr->type)) expr->flags|=8;
}
PTREE NodeFetchReference(PTREE expr)
{
    TYPE type=expr->type,ref=ReferenceType(type);
    /* The1994 form predates the extra C++-linkage predicate in the released
     * implementation; its argument/reference test alone selects this fetch. */
    if(expr->op==7 && (ref || ObjModelArgument(type)==2) &&
       SymIsArgument(expr->u.symcg.symbol)) {
        expr=nodeDoFetchRef(expr);
        expr->type=ref?ref:type;
        expr->flags|=2;
    } else if(ref) {
        expr=NodeFetch(expr);
        expr->type=ref;
        expr->flags|=2;
    }
    return expr;
}

TYPE SignedIntType(TYPE type);
unsigned long NodeConstantValue(PTREE node)
{
    SYMBOL sym;
    node=NodeRemoveCasts(node);
    switch(node->op) {
    case 2: case 3:
        return node->u.uint_constant;
    case 7:
        sym=node->u.symcg.symbol;
        /* The old source distinguishes signed/unsigned32 constants here;
         * both retain the same32 payload bits in unsigned long. */
        if(SignedIntType(sym->sym_type)) return (unsigned long)sym->u.sval;
        return sym->u.uval;
    default:
        /* The original reads an uninitialized value for non-constants. */
        STUB_TRAP(); return 0;
    }
}

PTREE *PTreeRef(PTREE *expr);
PTREE nodeRefedRvalue(PTREE *start)
{
    PTREE *actual=PTreeRef(start);
    PTREE mod=NodeRvalue(*actual),node;
    *actual=mod;
    for(node=*start;node!=mod;node=node->u.subtree[1]) {
        node->flags=mod->flags;
        node->type=mod->type;
    }
    return mod;
}
int TypeIsBasedPtr(TYPE type);
TYPE TypeConvertFromPcPtr(TYPE type);
int NodeDerefPtr(PTREE *expr)
{
    PTREE node=NodeRvalue(*expr);
    static CNV_DIAG diagDeref={0,0,0,0,0};
    TYPE type;
    unsigned result;
    *expr=node;
    if(TypeIsBasedPtr(node->type)) {
        type=TypeConvertFromPcPtr(node->type);
        result=CastImplicit(expr,type,0x2010);
        return !ConversionDiagnose(result,node,&diagDeref);
    }
    return 1;
}

/* The old logical entry is the bottom test at1a16d; its loop body is
 * placed before it. Preserve that one function, not a second tail identity. */
int NodeBitField(PTREE node)
{
    for(;node!=NULL;node=PTreeOpLeft(node)) {
        if(!(node->flags&2)) return 0;
        if(node->op==8) {
            if(node->cgop==101) return 1;
        } else if(node->op==9) {
            switch(node->cgop) {
            case 48: case 45: case 23: case 46: case 55: return 0;
            }
        } else return 0;
    }
    return 0;
}

int SymIsVirtual(SYMBOL sym);
PTREE NodeMakeCallee(SYMBOL sym)
{
    if(!SymIsVirtual(sym)) sym=SymMarkRefed(sym);
    return NodeSymbolNoRef(PTreeAlloc(),sym,NULL);
}

TYPE TypeModExtract(TYPE type,unsigned short *flags,void **baser,unsigned char mask);
TYPE TypeGetActualFlags(TYPE type,unsigned short *flags);
TYPE MakeBasedModifierOf(TYPE type,unsigned short flags,void *base);
/* The 1994 member merge predates the later mutable-member adjustment. */
TYPE TypeMergeForMember(TYPE object,TYPE member)
{
    unsigned short owner_flags,member_flags;
    void *owner_base;
    TypeModExtract(object,&owner_flags,&owner_base,1);
    member=TypeGetActualFlags(member,&member_flags);
    owner_flags|=member_flags;
    if(owner_flags) member=MakeBasedModifierOf(member,owner_flags,owner_base);
    return member;
}
PTREE PTreeCopySrcLocation(PTREE source,PTREE target);
PTREE NodeReplace(PTREE old,PTREE replacement)
{
    PTREE *ref;
    if(old!=NULL) {
        old=PTreeCopySrcLocation(old,replacement);
        ref=PTreeRef(&old);
        NodeFreeDupedExpr(*ref);
        *ref=replacement;
        replacement=old;
    }
    return replacement;
}
PTREE pruneSubtree(PTREE expr,PTREE *subtree)
{
    PTREE *ref=PTreeRef(subtree);
    PTREE drop=*subtree;
    NodeFreeDupedExpr(*ref);
    *subtree=NULL;
    if (subtree!=ref) {
        *ref=expr;
        expr=drop;
    }
    return expr;
}
PTREE NodePruneRight(PTREE expr)
{
    return pruneSubtree(expr,&expr->u.subtree[1]);
}
PTREE nodeCommaPropogate(PTREE node);
PTREE NodePruneTop(PTREE curr)
{
    PTREE last=NULL,next,drop;
    if (!curr) return curr;
    while (NodeIsBinaryOp(curr,42)) {
        next=curr->u.subtree[1];
        curr->u.subtree[1]=last;
        last=curr;
        curr=next;
    }
    if (curr) {
        if (curr->op==10) drop=fixupTraverse(curr);
        else if (curr->op<8) { drop=curr; curr=NULL; }
        else if (curr->op==8) { drop=curr; curr=curr->u.subtree[0]; }
        else if (curr->op==9) {
            if (!curr->u.subtree[0]) { drop=curr; curr=curr->u.subtree[1]; }
            else if (!curr->u.subtree[1]) { drop=curr; curr=curr->u.subtree[0]; }
            else {
                curr->cgop=42;
                nodeCommaPropogate(curr);
                drop=NULL;
            }
        } else {
            /* The original leaves its deletion register undefined here. */
            STUB_TRAP(); return NULL;
        }
        PTreeFree(drop);
    }
    if (last) {
        if (!curr) {
            next=last->u.subtree[1];
            curr=last->u.subtree[0];
            PTreeFree(last);
            last=next;
        }
        while (last) {
            next=last->u.subtree[1];
            last->u.subtree[1]=curr;
            nodeCommaPropogate(last);
            curr=last;
            last=next;
        }
    }
    return curr;
}
void NodeConvertToBasePtr(PTREE *expr,TYPE type,SEARCH_RESULT *result,int positive);

unsigned short PTreeOpFlags(PTREE expr);
TYPE ClassTypeForType(TYPE type);
TYPE MemberPtrType(TYPE type);
PTREE ConvertBoolean(PTREE expr);
PTREE NodeCompareToZero(PTREE expr)
{
    TYPE type,zero_type;
    type_id zero_id;
    PTREE operand,zero;
    if((expr->flags&4) || (PTreeOpFlags(expr)&4)) return expr;
    type=TypedefModifierRemove(expr->type);
    if(type->id==TYP_VOID) {
        PTreeErrorExpr(expr,659);
        return expr;
    }
    if(!ClassTypeForType(type) && !MemberPtrType(type)) {
        expr=NodeRvalue(expr);
        zero_id=type->id;
        zero_type=expr->type;
    } else {
        zero_id=TYP_SINT;
        zero_type=GetBasicType(TYP_SINT);
    }
    operand=expr;
    zero=PTreeIntConstant(0,zero_id);
    zero->type=zero_type;
    expr=NodeBinary(15,operand,zero);
    PTreeCopySrcLocation(expr,operand);
    expr->type=GetBasicType(TYP_SINT);
    expr->flags|=4;
    return ConvertBoolean(expr);
}

PTREE PTreeCDtorExtra(void);
TYPE GetBasicUInt(void);
PTREE NodeCDtorExtra(void)
{
    PTREE node=PTreeCDtorExtra();
    node->op=7;
    node->type=GetBasicUInt();
    node->u.symcg.symbol=NULL;
    node->u.symcg.result=NULL;
    node->flags|=2;
    return NodeRvalue(node);
}

/* The encountered old promotion table covers the fourteen arithmetic ids.
 * The later BOOL and64-bit ids do not exist in this compiler. */
static const type_id intPromo[]={
    TYP_ERROR,TYP_SINT,TYP_SINT,TYP_SINT,TYP_SINT,TYP_SINT,TYP_SINT,
    TYP_SINT,TYP_UINT,TYP_SLONG,TYP_ULONG,TYP_FLOAT,TYP_DOUBLE,TYP_LONG_DOUBLE
};
type_id integralPromote(TYPE type)
{
    type=TypedefModifierRemove(type);
    if(type->id>=sizeof(intPromo)/sizeof(intPromo[0])) { STUB_TRAP(); return TYP_ERROR; }
    return intPromo[type->id];
}
TYPE TypeBinArithResult(TYPE left,TYPE right)
{
    type_id first=integralPromote(left),second=integralPromote(right),swap;
    if(first>second) { swap=first; first=second; second=swap; }
    if(first==TYP_UINT && second==TYP_SLONG) second=TYP_ULONG;
    return GetBasicType(second);
}

int IsBigCode(void);
int IsBigData(void);
int TypeTruncByMemModel(TYPE type)
{
    unsigned short flags;
    type=TypeGetActualFlags(type,&flags);
    flags&=0x73c;
    if(!flags) return 0;
    if(type->id==TYP_FUNCTION) return IsBigCode()?0:!(flags&4);
    return IsBigData()?0:!(flags&0x24);
}

PTREE NodeArgument(PTREE left,PTREE right)
{
    PTREE arg=NodeBinary(44,left,right);
    arg->type=right->type;
    arg->flags=right->flags;
    PTreeCopySrcLocation(arg,right);
    return arg;
}
PTREE NodeArguments(PTREE first,...)
{
    PTREE expr=NULL,argument=first;
    va_list args;
    va_start(args,first);
    while(argument) {
        expr=NodeArgument(expr,argument);
        argument=va_arg(args,PTREE);
    }
    va_end(args);
    return expr;
}

int TypeRequiresCtorParm(TYPE type);
TYPE GetBasicUInt(void);
PTREE PTreeUIntConstant(unsigned long value,type_id type);
PTREE NodeCDtorArg(unsigned code)
{
    PTREE node=PTreeUIntConstant(code,TYP_SINT);
    node->type=GetBasicUInt();
    return node;
}
PTREE NodeArg(PTREE expr)
{
    return NodeArgument(NULL,expr);
}
PTREE CallArgumentExactCtor(TYPE type,unsigned exact)
{
    if (!TypeRequiresCtorParm(type)) return NULL;
    return NodeArg(NodeCDtorArg(!exact));
}

PTREE nodeCommaPropogate(PTREE node)
{
    PTREE right=node->u.subtree[1];
    PTreeCopySrcLocation(node,right);
    node->type=right->type;
    node->flags=right->flags;
    return set_meaningful_side_effects(node);
}
PTREE NodeComma(PTREE left,PTREE right)
{
    if (!left) return right;
    if (!right) return left;
    return nodeCommaPropogate(NodeBinary(42,left,right));
}
TYPE TypeModFlagsEC(TYPE type,unsigned short *flags);
int nonVolatileSymbol(PTREE node)
{
    unsigned short flags;
    if (node->op!=7) return 0;
    TypeModFlagsEC(node->u.symcg.symbol->sym_type,&flags);
    return (flags&2)==0;
}
SYMBOL TemporaryAlloc(TYPE type);
PTREE *PTreeRef(PTREE *node);
int NodeIsConstantInt(PTREE node);
unsigned char TemporaryClass(unsigned char kind);
PTREE NodeFetch(PTREE expr);
TYPE ReferenceType(TYPE type);
TYPE TypeReferenced(TYPE type);
PTREE NodeAddSideEffect(PTREE effect,PTREE expr)
{
    PTREE dup,*ref,top,orig,fetched;
    TYPE type;
    SYMBOL temp;
    unsigned char old;
    if (!expr) return effect;
    dup=expr;
    ref=PTreeRef(&dup);
    top=*ref;
    if (NodeIsUnaryOp(top,92)) {
        fetched=PTreeOpLeft(top);
        if (nonVolatileSymbol(fetched)) top=fetched;
    }
    orig=expr;
    if (top->op==7 || NodeIsConstantInt(top)) {
        *ref=NodeComma(effect,*ref);
        expr=dup;
    } else {
        old=TemporaryClass(1);
        type=NodeType(expr);
        temp=TemporaryAlloc(type);
        expr=NodeAssign(MakeNodeSymbol(temp),expr);
        expr=NodeComma(expr,effect);
        fetched=NodeFetch(MakeNodeSymbol(temp));
        if (ReferenceType(type)) {
            fetched->flags|=2;
            fetched->type=TypeReferenced(type);
        }
        expr=NodeComma(expr,fetched);
        TemporaryClass(old);
    }
    expr->flags=orig->flags;
    expr->type=orig->type;
    return PTreeCopySrcLocation(expr,orig);
}

int NodePtrNonZero(PTREE expr)
{
    return (expr->flags&0x200)!=0 || (expr->op==7 && (expr->flags&2)!=0);
}

PTREE PTreeDupExpr(PTREE expr);
PTREE NodeDupExpr(PTREE *expr)
{
    PTREE old=*expr,node,partner;
    if ((old->op>=1 && old->op<=4) || old->op==7) {
        node=PTreeAlloc();
        PTreeAssign(node,old);
    } else {
        partner=PTreeDupExpr(old);
        *expr=partner;
        node=PTreeDupExpr(old);
        node->u.dup.partner=partner;
        partner->u.dup.partner=node;
    }
    return node;
}

PTREE NodeIc(unsigned opcode) { return PTreeIc(opcode,0); }

TYPE ReferenceType(TYPE type);
TYPE NodeSetReference(TYPE type,PTREE expr)
{
    if(type && ReferenceType(type)) expr->flags|=2;
    return type;
}

int NodeCallsCtor(PTREE node)
{
    if(NodeIsBinaryOp(node,95) || NodeIsBinaryOp(node,70))
        return SymIsCtor(node->u.subtree[0]->u.subtree[0]->u.symcg.symbol);
    return 0;
}

/* Descriptive inferred name: this distinct1994 initialization wrapper
 * calls NodeDtorExpr and extends temporary cleanup to a block lifetime.
 * It is absent from the released source. Exact body remains pending. */
int SymRequiresDtoring(SYMBOL sym);
int SymIsModuleDtorable(SYMBOL sym);
SCOPE SymScope(SYMBOL sym);
SYMBOL DtorFindLocn(TYPE type,CGIO_LOCATION *locn);
int TypeExactDtorable(TYPE type);
void CDtorScheduleArgRemap(SYMBOL sym);
void PTreeExtractLocn(PTREE expr,CGIO_LOCATION *locn);
PTREE PTreeErrorNode(PTREE expr);
PTREE PTreeErrorExpr(PTREE expr,unsigned message);
PTREE NodeDtorExpr(PTREE expr,SYMBOL sym)
{
    PTREE original,dtored;
    CGIO_LOCATION locn;
    SYMBOL dtor;
    if (expr->op!=0 && SymRequiresDtoring(sym)) {
        sym->flag|=8;
        original=expr;
        if (!SymIsModuleDtorable(sym)) SymScope(sym)->dtor_reqd=1;
        PTreeExtractLocn(expr,&locn);
        dtor=DtorFindLocn(sym->sym_type,&locn);
        if (!dtor) PTreeErrorNode(expr);
        else if (TypeTruncByMemModel(expr->type)) PTreeErrorExpr(expr,160);
        else if (TypeExactDtorable(sym->sym_type)) {
            /* The1994 body still schedules the dtor argument remap. */
            CDtorScheduleArgRemap(dtor);
            SymMarkRefed(dtor);
            dtored=MakeNodeSymbol(sym);
            dtored->cgop=201;
            expr=NodeBinary(99,dtored,expr);
            expr->locn=locn;
            expr->type=original->type;
            expr->flags=original->flags;
        }
    }
    return expr;
}
TYPE ArrayType(TYPE type);
unsigned FunctionRegistrationFlag(void);
PTREE NodeComma(PTREE left,PTREE right);
PTREE NodeDtorInitExpr(PTREE expr,SYMBOL sym)
{
    PTREE value;
    expr=NodeDtorExpr(expr,sym);
    if (NodeIsBinaryOp(expr,99)) {
        sym=expr->u.subtree[0]->u.symcg.symbol;
        if (sym->id==3 && !ArrayType(sym->sym_type)) {
            value=expr->u.subtree[1];
            expr->u.subtree[1]=NULL;
            NodeFreeDupedExpr(expr);
            value=NodeAddSideEffect(PTreeIc(95,(unsigned)sym),value);
            expr=NodeComma(PTreeIc(94,(unsigned)sym),value);
            FunctionRegistrationFlag();
        }
    }
    return expr;
}

PTREE NodeTestExpr(PTREE condition,PTREE yes,PTREE no)
{
    PTREE expr;
    TYPE type=yes->type;
    expr=NodeBinary(43,yes,no);
    expr->type=type;
    expr=NodeBinary(41,condition,expr);
    expr->type=type;
    return expr;
}

PTREE NodeSymbolCallee(PTREE expr,SYMBOL sym,SEARCH_RESULT *result)
{
    if(!SymIsVirtual(sym)) sym=SymMarkRefed(sym);
    return NodeSymbolNoRef(expr,sym,result);
}

PTREE NodePruneLeft(PTREE expr)
{ return pruneSubtree(expr,&expr->u.subtree[0]); }
PTREE NodePruneLeftTop(PTREE expr)
{ return NodePruneTop(NodePruneLeft(expr)); }
