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


/* Actual1994 call construction, before VfunSetupCall replaced the old
 * indirect CO_CALL_SETUP_IND unary form. */
#include "compiler.h"
#include "ptree.h"
#include "symbol.h"
#include "pending.h"
#include "convdiag.h"
#include "analcall.h"
#include "scope.h"
#include <recon.h>
PTREE NodeMakeCallee(SYMBOL sym);
PTREE NodeUnaryCopy(unsigned char op,PTREE operand);
PTREE NodeBinary(unsigned char op,PTREE left,PTREE right);
PTREE makeCall(PTREE proc,TYPE type,PTREE args,int direct)
{
    PTREE node=NodeUnaryCopy(direct?96:98,proc);
    node=NodeBinary(direct?95:97,node,args);
    node->type=type;
    node->flags|=0x11;
    return node;
}
PTREE NodeMakeCall(SYMBOL sym,TYPE type,PTREE args)
{ return makeCall(NodeMakeCallee(sym),type,args,1); }

PTREE NodeReverseArgs(unsigned *count,PTREE initial)
{
    PTREE last=NULL,next;
    unsigned total=0;
    while (initial) {
        ++total;
        next=initial->u.subtree[0];
        initial->u.subtree[0]=last;
        last=initial;
        initial=next;
    }
    *count=total;
    return last;
}
TYPE NodeType(PTREE expr);
void NodeBuildArgList(arg_list *args,PTREE *nodes,PTREE node,unsigned count)
{
    TYPE *types=args->type_list;
    args->num_args=count;
    while (count) {
        *types++=NodeType(node);
        --count;
        *nodes++=node->u.subtree[1];
        node=node->u.subtree[0];
    }
}
unsigned char TemporaryClass(unsigned char kind);
arg_list *TypeArgList(TYPE type);
TYPE FunctionDeclarationType(TYPE type);
int TypeHasEllipsisArg(TYPE type);
PTREE PTreeOp(PTREE *node);
TYPE StructType(TYPE type);
unsigned char ObjModelArgument(TYPE type);
PTREE PTreeErrorNode(PTREE node);
PTREE *PTreeRefRight(PTREE arg);
PTREE PTreeOpRight(PTREE arg);
TYPE ArrayType(TYPE type);
TYPE PointerTypeForArray(TYPE type);
TYPE MemberPtrType(TYPE type);
void MembPtrPassArgument(PTREE *expr)
{ (void)expr; STUB_TRAP(); }
static CNV_DIAG diagArgConv={333,334,0,378,377};
int NodeConvertArgument(PTREE *expr,TYPE proto)
{
    unsigned result;
    if (ArrayType(proto)) proto=PointerTypeForArray(proto);
    result=CastImplicit(expr,proto,2);
    result=ConversionDiagnose(result,*expr,&diagArgConv);
    if (result) return 0;
    if (MemberPtrType(proto)) MembPtrPassArgument(expr);
    return 1;
}
int arg_convert(PTREE arg,TYPE type)
{
    PTREE right;
    if (NodeConvertArgument(PTreeRefRight(arg),type)) {
        right=PTreeOpRight(arg);
        arg->type=type;
        arg->flags=(arg->flags&~2)|(right->flags&2);
        return 1;
    }
    PTreeErrorNode(arg);
    return 0;
}
int convertEllipsisArg(PTREE arg)
{ (void)arg; STUB_TRAP(); return 0; }
int passStructOnStack(PTREE arg,unsigned message)
{ (void)arg; (void)message; STUB_TRAP(); return 0; }
/* The old interface receives the caller's argument-list descriptor, rather
 * than the later unsigned count. Empty calls still bracket TemporaryClass. */
PTREE NodeConvertCallArgList(PTREE node,arg_list *args,TYPE fn_type,PTREE *node_addr)
{
    unsigned char old;
    unsigned count,pcount,acount;
    TYPE *pptr,cl_type;
    int ellipsis,extern_c;
    PTREE arg;
    if (node && node->op!=0) {
        old=TemporaryClass(1);
        acount=args->num_args;
        if (acount) {
            args=TypeArgList(fn_type);
            pptr=args->type_list;
            pcount=args->num_args;
            fn_type=FunctionDeclarationType(fn_type);
            ellipsis=TypeHasEllipsisArg(fn_type);
            extern_c=(fn_type->flag&0x400)==0;
            for (count=1;count<=acount;++count,node_addr=&arg->u.subtree[0]) {
                arg=PTreeOp(node_addr);
                if (count<pcount || (count==pcount && !ellipsis)) {
                    if (!arg_convert(arg,*pptr++)) {
                        PTreeErrorNode(node);
                        break;
                    }
                }
                if (ellipsis) {
                    if (count>=pcount && !convertEllipsisArg(arg)) {
                        PTreeErrorNode(node);
                        break;
                    }
                } else {
                    cl_type=StructType(arg->type);
                    if (cl_type) {
                        if (extern_c) {
                            if (!passStructOnStack(arg,613)) {
                                PTreeErrorNode(node);
                                break;
                            }
                        } else if (ObjModelArgument(cl_type)==3) passStructOnStack(arg,0);
                    }
                }
            }
        }
        TemporaryClass(old);
    }
    return node;
}
void CallDiagAmbiguous(PTREE expr,unsigned message)
{ (void)expr; (void)message; STUB_TRAP(); }

unsigned char ObjModelFunctionReturn(TYPE type);
TYPE TypedefModifierRemove(TYPE type);
int TypeHasReverseArgs(TYPE type);
unsigned char PcCallImpl(TYPE type)
{
    type=TypedefModifierRemove(type);
    if (TypeHasReverseArgs(type)) return (type->flag&0x400)?2:3;
    if (TypeHasEllipsisArg(type)) return (type->flag&0x400)?4:5;
    return (type->flag&0x400)?0:1;
}
PTREE insertArg(PTREE seq,PTREE arg)
{
    if (!seq) return arg;
    if (arg) { arg->u.subtree[0]=seq; return arg; }
    return seq;
}
PTREE appendArg(PTREE seq,PTREE arg)
{
    PTREE next;
    if (!seq) return arg;
    if (arg) {
        next=seq;
        while (next->u.subtree[0]) next=next->u.subtree[0];
        next->u.subtree[0]=arg;
    }
    return seq;
}
PTREE NodeArg(PTREE expr);
PTREE insertCppRetnArg(PTREE seq,PTREE arg,unsigned char kind)
{
    if (kind==2) {
        arg->flags|=0x2000;
        seq=insertArg(seq,NodeArg(arg));
    }
    return seq;
}
PTREE PTreeOpLeft(PTREE expr);
PTREE NodeCopyClassObject(PTREE target,PTREE source)
{ (void)target; (void)source; STUB_TRAP(); return NULL; }
/* In the1994 image the stack-copy block is internal to this function;
 * it has no separate called insertRetnCopy entry. */
PTREE insertCppRetnCopy(PTREE expr,PTREE arg,unsigned char kind)
{
    if (kind==3 && arg) {
        PTreeOpLeft(expr)->flags|=0x400;
        expr->flags|=0x400;
        expr=NodeCopyClassObject(arg,expr);
    }
    return expr;
}
PTREE CallArgsArrange(TYPE type,PTREE call,PTREE args,PTREE this_arg,PTREE exact_arg,PTREE extra)
{
    PTREE arglist;
    unsigned char kind,impl;
    if (this_arg) this_arg->flags|=0x1000;
    kind=ObjModelFunctionReturn(type);
    impl=PcCallImpl(type);
    switch (impl) {
    case 3: kind=3;
    case 2:
        arglist=insertArg(this_arg,exact_arg);
        arglist=insertCppRetnArg(arglist,extra,kind);
        arglist=appendArg(args,arglist);
        break;
    case 1: case 5: kind=3;
    case 0: case 4:
        arglist=insertCppRetnArg(args,extra,kind);
        arglist=insertArg(arglist,exact_arg);
        arglist=insertArg(arglist,this_arg);
        break;
    default: STUB_TRAP(); return NULL;
    }
    call->u.subtree[1]=arglist;
    return insertCppRetnCopy(call,extra,kind);
}

void CtorDiagNoMatch(PTREE expr,unsigned message)
{ (void)expr; (void)message; STUB_TRAP(); }

/* Genuine1994 three-argument wrapper at12134, folded into its callers in
 * the released source. It locates the right argument slot for conversion. */
PTREE *PTreeRefRight(PTREE expr);
PTREE NodeConvertCallArgs(PTREE node,arg_list *args,TYPE type)
{
    if(args->num_args>0)
        node=NodeConvertCallArgList(node,args,type,PTreeRefRight(node));
    return node;
}
void CallDiagNoMatch(PTREE expr,unsigned one,unsigned many,PTREE this_node,SYMBOL sym)
{ (void)expr; (void)one; (void)many; (void)this_node; (void)sym; STUB_TRAP(); }

PTREE *PTreeRefLeft(PTREE expr);
PTREE NodePruneTop(PTREE expr);
arg_list *AllocArgList(int count,int permanent);
TYPE TypeThis(void);
unsigned short BaseTypeClassFlags(TYPE type);
unsigned char FuncOverloaded(SYMBOL *symbol,SYMBOL candidates,arg_list *args,PTREE *nodes);
int SymIsThisFuncMember(SYMBOL sym);
int SymIsStaticFuncMember(SYMBOL sym);
int SymIsCtor(SYMBOL sym);
int SymIsDtor(SYMBOL sym);
PTREE NodeThis(void);
typedef struct symbol_diag { int msg_no_this,msg_extra_this,msg_enclosing; } SYMBOL_DIAG;
int AnalyseSymbolAccess(PTREE expr,PTREE symbol,PTREE this_expr,SYMBOL_DIAG *diag);
TYPE TypeGetActualFlags(TYPE type,unsigned short *flags);
TYPE TypeThisSymbol(SYMBOL sym,int reference);
unsigned checkImplicitTypes(PTREE expr,TYPE target,TYPE source,unsigned control);
TYPE TypePointedAtModified(TYPE type);
int TypeTruncByMemModel(TYPE type);
SYMBOL ScopeFunctionInProgress(void);
TYPE TypeThisForCall(PTREE expr,SYMBOL sym);
void NodeConvertToBasePtr(PTREE *expr,TYPE type,SEARCH_RESULT *result,int positive);
SYMBOL SymDefaultBase(SYMBOL sym);
int SymIsVirtual(SYMBOL sym);
PTREE NodeDupExpr(PTREE *expr);
PTREE AccessVirtualFnAddress(PTREE expr,SEARCH_RESULT *result,SYMBOL sym);
TYPE MakePointerTo(TYPE type);
PTREE NodeReplace(PTREE old,PTREE replacement);
void NodeFreeSearchResult(PTREE expr);
PTREE VfnDecorateCall(PTREE expr,SYMBOL sym);
void SymMarkRefed(SYMBOL sym);
PTREE NodeCDtorArg(unsigned code);
PTREE NodeTemporary(TYPE type);
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
void AddDefaultArgs(SYMBOL sym,PTREE expr);
TYPE ReferenceType(TYPE type);
int TypeDefined(TYPE type);
PTREE NodeDtorExpr(PTREE expr,SYMBOL sym);

int adjustForVirtualCall(PTREE *this_node,PTREE *function,SEARCH_RESULT *result)
{
    SYMBOL sym=(*function)->u.symcg.symbol;
    TYPE type=TypeThisForCall(*this_node,sym);
    int exact;
    PTREE expr;
    (*this_node)->flags|=0x200;
    exact=(*this_node)->flags&8;
    NodeConvertToBasePtr(this_node,type,result,1);
    sym=SymDefaultBase(sym);
    if(SymIsVirtual(sym) && !((*function)->flags&0x100) && !exact) {
        expr=AccessVirtualFnAddress(NodeDupExpr(this_node),result,sym);
        expr->type=MakePointerTo(expr->type);
        *function=NodeReplace(*function,expr);
        return 1;
    }
    NodeFreeSearchResult(*function);
    return 0;
}
/* Reached old call pipeline. The remaining indirect/setup/return paths are
 * explicit pending domains; this enclosing owner is not yet accepted. */
PTREE AnalyseCall(PTREE expr,CALL_DIAG *diagnostic)
{
    PTREE *r_args=PTreeRefRight(expr),*r_func,*nodes;
    PTREE right,left,this_node=NULL;
    unsigned count;
    unsigned char status;
    unsigned short fn_mod;
    arg_list *args;
    SYMBOL sym;
    TYPE type,fn_type,pointed;
    SEARCH_RESULT *result;
    static SYMBOL_DIAG diagAccess={499,500,502};
    *r_args=right=NodeReverseArgs(&count,*r_args);
    r_func=PTreeRefLeft(expr);
    left=*r_func;
    switch(left->cgop) {
    case 48: case 23:
        this_node=left->u.subtree[0];
        left->u.subtree[0]=NULL;
        *r_func=NodePruneTop(left);
        r_func=PTreeRefLeft(expr);
        left=*r_func;
        if(left->op==6 && left->cgop==197) { STUB_TRAP(); return NULL; }
        break;
    case 97:
        if(left->flags&0x80) { STUB_TRAP(); return NULL; }
        break;
    }
    args=AllocArgList(count,0);
    nodes=CMemAlloc(count*sizeof(*nodes));
    NodeBuildArgList(args,nodes,right,count);
    type=this_node ? NodeType(this_node) : TypeThis();
    args->qualifier=BaseTypeClassFlags(type);
    if(left->op!=7) { STUB_TRAP(); return NULL; }
    sym=left->u.symcg.symbol;
    if(left->cgop==199) { STUB_TRAP(); return NULL; }
    status=FuncOverloaded(&sym,sym,args,nodes);
    if(status==1) { CallDiagAmbiguous(expr,diagnostic->msg_ambiguous); STUB_TRAP(); return NULL; }
    if(status==2) {
        CallDiagNoMatch(expr,diagnostic->msg_no_match,diagnostic->msg_no_overload,
                        this_node,left->u.symcg.symbol);
        STUB_TRAP(); return NULL;
    }
    left->u.symcg.symbol=sym;
    result=left->u.symcg.result;
    if(!this_node && SymIsThisFuncMember(sym)) {
        if(!(result->flags&4)) { STUB_TRAP(); return NULL; }
        this_node=NodeThis();
        if(!this_node) { STUB_TRAP(); return NULL; }
    }
    if(!AnalyseSymbolAccess(expr,left,this_node,&diagAccess)) { STUB_TRAP(); return NULL; }
    type=sym->sym_type;
    fn_type=TypeGetActualFlags(type,&fn_mod);
    if(fn_type->flag&0x100) { STUB_TRAP(); return NULL; }
    if(fn_mod&0x10) ScopeFunctionInProgress()->flag|=0x20;
    left->type=type;
    if(!this_node) {
        if(SymIsThisFuncMember(sym)) this_node=NodeThis();
    } else if(SymIsStaticFuncMember(sym)) {
        NodeFreeDupedExpr(this_node);
        this_node=NULL;
    }
    if(this_node) {
        if(!SymIsCtor(sym) && !SymIsDtor(sym) &&
           checkImplicitTypes(this_node,TypeThisSymbol(sym,this_node->flags&2),
                        NodeType(this_node),66)) { STUB_TRAP(); return NULL; }
        pointed=TypePointedAtModified(this_node->type);
        if(!pointed) pointed=this_node->type;
        if(TypeTruncByMemModel(pointed)) { STUB_TRAP(); return NULL; }
        if(adjustForVirtualCall(&this_node,r_func,result)) {
            expr->cgop=97;
            left=NodeUnaryCopy(98,expr->u.subtree[0]);
            left=VfnDecorateCall(left,sym);
        } else {
            expr->cgop=95;
            left=NodeUnaryCopy(96,expr->u.subtree[0]);
            SymMarkRefed(sym);
        }
    } else {
        NodeFreeSearchResult(left);
        expr->cgop=95;
        left=NodeUnaryCopy(96,expr->u.subtree[0]);
        SymMarkRefed(sym);
    }
    expr->u.subtree[0]=left;
    expr=NodeConvertCallArgs(expr,args,type);
    if(expr->op!=0) {
        PTREE call=expr,cdtor=NULL,retnnode=NULL;
        TYPE ftype=type;
        if(this_node) {
            this_node=NodeArg(this_node);
            if(sym && SymIsDtor(sym)) cdtor=NodeArg(NodeCDtorArg(0));
        }
        fn_type=TypedefModifierRemove(type);
        TypeHasEllipsisArg(fn_type);
        type=fn_type->of;
        expr->type=type;
        if(sym) AddDefaultArgs(sym,expr);
        if(ReferenceType(type)) expr->flags|=2;
        if(ObjModelArgument(type)==2) {
            retnnode=NodeTemporary(type);
            PTreeCopySrcLocation(retnnode,expr);
        }
        expr=CallArgsArrange(ftype,call,call->u.subtree[1],
                             this_node,cdtor,retnnode);
        if(retnnode) expr=NodeDtorExpr(expr,retnnode->u.symcg.symbol);
        pointed=StructType(type);
        if(pointed && !TypeDefined(pointed)) { STUB_TRAP(); return NULL; }
        /* Actual128ba only sets these flags on its error-node branch. */
        if(expr->op==0) expr->flags|=0x11;
    }
    CMemFree(args);
    CMemFree(nodes);
    return expr;
}

TYPE SymFuncReturnType(SYMBOL sym);
SEARCH_RESULT *DtorFindResult(TYPE type);
PTREE NodeSymbolCallee(PTREE expr,SYMBOL sym,SEARCH_RESULT *result);
PTREE AnalyseDtorCall(TYPE class_type,PTREE this_node,unsigned extra)
{
    SEARCH_RESULT *result;
    SYMBOL dtor;
    PTREE dtor_id,expr;
    CGIO_LOCATION locn;
    int virtual_call;
    result=DtorFindResult(class_type);
    PTreeExtractLocn(this_node,&locn);
    ScopeResultErrLocn(result,&locn);
    dtor=result->sym_name->name_syms;
    ScopeCheckSymbol(result,dtor);
    dtor_id=NodeSymbolCallee(PTreeAlloc(),dtor,result);
    virtual_call=adjustForVirtualCall(&this_node,&dtor_id,result);
    expr=makeCall(dtor_id,SymFuncReturnType(dtor),NULL,!virtual_call);
    expr=CallArgsArrange(dtor->sym_type,expr,NULL,NodeArg(this_node),
                         NodeArg(NodeCDtorArg(extra)),NULL);
    if(virtual_call)
        expr->u.subtree[0]=VfnDecorateCall(expr->u.subtree[0],dtor);
    expr->flags|=0x11;
    return expr;
}
