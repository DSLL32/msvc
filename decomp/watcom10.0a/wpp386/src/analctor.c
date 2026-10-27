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


#include "compiler.h"
#include "types.h"
#include "symbol.h"
#include "ptree.h"
#include "cgio.h"
#include "pending.h"
#include "convdiag.h"
#include "fnovload.h"
#include <recon.h>
unsigned CastImplicit(PTREE *expr,TYPE type,unsigned control);
void PTreeErrorExpr(PTREE expr,unsigned error);
TYPE TypedefModifierRemoveOnly(TYPE type);
TYPE ArrayBaseType(TYPE type);
PTREE NodeReverseArgs(unsigned *count,PTREE initial);
void NodeBuildArgList(arg_list *args,PTREE *nodes,PTREE node,unsigned count);
PTREE NodeConvertCallArgList(PTREE node,arg_list *args,TYPE fn_type,PTREE *node_addr);
unsigned CtorFind(SCOPE access,TYPE type,arg_list *args,PTREE *nodes,
    CGIO_LOCATION *locn,SYMBOL *ctor);
CGIO_LOCATION *PTreeExtractLocn(PTREE expr,CGIO_LOCATION *locn);
unsigned char ObjModelArgument(TYPE type);
void CallDiagAmbiguous(PTREE expr,unsigned message);
TYPE NodeType(PTREE expr);
void ConversionTypesSet(TYPE first,TYPE second);
unsigned analyseCtorClass(TYPE type,SCOPE access,SYMBOL *ctor,PTREE initial,PTREE *expr)
{
    unsigned count,result;
    PTREE node=NodeReverseArgs(&count,initial),*nodes;
    arg_list *args=AllocArgList(count,0);
    CGIO_LOCATION location;
    nodes=CMemAlloc(count*sizeof(PTREE));
    NodeBuildArgList(args,nodes,node,count);
    PTreeExtractLocn(initial,&location);
    result=CtorFind(access,type,args,nodes,&location,ctor);
    switch (result) {
    case 0:
        node=NodeConvertCallArgList(node,args,(*ctor)->sym_type,&node);
        if (node && node->op==0) result=2;
        break;
    case 4:
        if (count==1 && StructType(initial->type)==type && ObjModelArgument(type)==3)
            result=0;
        break;
    case 5:
        result=2;
        CallDiagAmbiguous(node,303);
        break;
    }
    if (result && initial) ConversionTypesSet(NodeType(initial),type);
    CMemFree(args);
    CMemFree(nodes);
    *expr=node;
    return result;
}
TYPE AbstractClassType(TYPE type);
int TypeDefined(TYPE type);
void PTreeErrorExprSym(PTREE expr,unsigned message,SYMBOL payload);
void ScopeNotePureFunctions(TYPE type);
unsigned AnalyseTypeCtor(SCOPE access,TYPE type,unsigned control,SYMBOL *ctor,PTREE *initial);
unsigned AnalyseCtor(TYPE type,SYMBOL *ctor,PTREE *initial)
{
    TYPE class_type=StructType(type);
    PTREE expr;
    if(class_type!=NULL) {
        *ctor=NULL;
        expr=*initial;
        if(AbstractClassType(class_type)!=NULL) {
            /* This diagnostic's generic pointer payload is the class type. */
            PTreeErrorExprSym(expr,436,(SYMBOL)class_type);
            ScopeNotePureFunctions(class_type);
            return 2;
        }
        if(!TypeDefined(class_type)) {
            PTreeErrorExprSym(expr,437,(SYMBOL)class_type);
            return 2;
        }
    }
    return AnalyseTypeCtor(NULL,type,8,ctor,initial);
}
unsigned single_arg(PTREE *initial)
{
    PTREE expr=*initial;
    if(!expr) return 0;
    if(expr->u.subtree[0]) {
        PTreeErrorExpr(expr,242);
        return 2;
    }
    *initial=expr->u.subtree[1];
    PTreeFree(expr);
    return 0;
}
/* Actual five-message scalar-construction diagnosis at b1b14. */
static CNV_DIAG diagCtor={340,303,0,0,0};
unsigned AnalyseTypeCtor(SCOPE access,TYPE type,unsigned control,SYMBOL *ctor,PTREE *initial)
{
    TYPE base_type;
    PTREE expr=*initial;
    unsigned result;
    *ctor=NULL;
    base_type=TypedefModifierRemoveOnly(type);
    switch(base_type->id) {
    case TYP_ERROR: return 2;
    case TYP_FUNCTION: case TYP_VOID:
        if(!*initial) return 4;
        /* fall through */
    case TYP_CHAR: case TYP_SCHAR: case TYP_UCHAR: case TYP_WCHAR:
    case TYP_SSHORT: case TYP_USHORT: case TYP_SINT: case TYP_UINT:
    case TYP_SLONG: case TYP_ULONG: case TYP_FLOAT: case TYP_DOUBLE:
    case TYP_LONG_DOUBLE: case TYP_ENUM: case TYP_POINTER:
    case TYP_BITFIELD: case TYP_MEMBER_POINTER:
        result=single_arg(initial);
        if(result==2 || !*initial) return result;
        result=CastImplicit(initial,base_type,control);
        return ConversionDiagnose(result,*initial,&diagCtor);
    case TYP_CLASS:
        return analyseCtorClass(base_type,access,ctor,expr,initial);
    case TYP_ARRAY:
        if(expr) {
            PTreeErrorExpr(expr,223);
            return 4;
        }
        return AnalyseCtor(ArrayBaseType(type),ctor,initial);
    }
    return 4;
}
PTREE NodeBinary(unsigned char op,PTREE left,PTREE right);
PTREE NodeUnaryCopy(unsigned char op,PTREE expr);
PTREE NodeUnary(unsigned char op,PTREE expr);
PTREE NodeConvert(TYPE type,PTREE expr);
PTREE NodeDupExpr(PTREE *expr);
PTREE NodeComma(PTREE left,PTREE right);
unsigned MembPtrAssign(PTREE *expr) { (void)expr; STUB_TRAP(); return 0; }
PTREE bareArg(PTREE expr)
{
    PTREE result=expr->u.subtree[1];
    PTreeFree(expr);
    return result;
}
PTREE ClassDefaultCopy(PTREE target,PTREE source)
{ (void)target; (void)source; STUB_TRAP(); return NULL; }
PTREE NodeTemporary(TYPE type) { (void)type; STUB_TRAP(); return NULL; }
PTREE PTreeSetLocn(PTREE expr,CGIO_LOCATION *location);
unsigned char AnalyseCallOpts(TYPE type,PTREE expr,PTREE *dtor,PTREE *bare);
/* The1994 optimizer has two genuine entries; their names are descriptive
 * until a contemporary symbol source identifies them. The later source
 * folded these CALL_OPT_CTOR/FUN_CALL paths into CopyOptimize. */
PTREE copyEliminateCtor(PTREE bare,PTREE source,PTREE target,PTREE dtor)
{ (void)bare; (void)source; (void)target; (void)dtor; STUB_TRAP(); return NULL; }
PTREE copyEliminateCall(PTREE bare,PTREE source,PTREE target,PTREE dtor)
{ (void)bare; (void)source; (void)target; (void)dtor; STUB_TRAP(); return NULL; }
TYPE ClassTypeForType(TYPE type);
TYPE PointerTypeEquivalent(TYPE type);
TYPE MakePointerTo(TYPE type);
TYPE MakeReferenceTo(TYPE type);
TYPE MakeModifiedType(TYPE type,unsigned flags);
TYPE SymFuncReturnType(SYMBOL sym);
PTREE NodeMakeCall(SYMBOL sym,TYPE type,PTREE arguments);
void AddDefaultArgs(SYMBOL sym,PTREE call);
PTREE CallArgumentExactCtor(TYPE type,unsigned exact);
PTREE NodeArg(PTREE expr);
PTREE CallArgsArrange(TYPE type,PTREE call,PTREE args,PTREE this_arg,PTREE exact_arg,PTREE extra);
PTREE NodeDtorExpr(PTREE expr,SYMBOL sym);
void NodeFreeDupedExpr(PTREE expr);
PTREE PTreeErrorNode(PTREE expr);
PTREE EffectCtor(PTREE initial,SYMBOL ctor,TYPE type,PTREE target,CGIO_LOCATION *location,unsigned control)
{
    PTREE node,dup,bare,dtor;
    TYPE this_type;
    arg_list *args;
    unsigned result;
    unsigned char opt;
    int check_dtoring;
    /* The original void-type branch consumes an uninitialized node. Keep
     * that body faithful below, but deny its invalid domain before effects. */
    if(type->id==TYP_VOID) STUB_TRAP();
    switch(type->id) {
    case TYP_CHAR: case TYP_SCHAR: case TYP_UCHAR: case TYP_WCHAR:
    case TYP_SSHORT: case TYP_USHORT: case TYP_SINT: case TYP_UINT:
    case TYP_SLONG: case TYP_ULONG: case TYP_FLOAT: case TYP_DOUBLE:
    case TYP_LONG_DOUBLE: case TYP_ENUM: case TYP_POINTER:
    case TYP_BITFIELD: case TYP_FUNCTION: case TYP_MEMBER_POINTER:
        if(!initial) initial=PTreeIntConstant(0,TYP_SINT);
        if(!target) return NodeConvert(type,initial);
        if(type->id==TYP_MEMBER_POINTER) {
            node=NodeBinary(72,target,initial);
            node->type=target->type;
            result=MembPtrAssign(&node);
            ConversionDiagnose(result,node,&diagCtor);
            return node;
        }
        dup=NULL;
        if(control&2) dup=NodeDupExpr(&target);
        if(type->id==TYP_BITFIELD) {
            target=NodeUnaryCopy(101,target);
            target->type=type->of;
        }
        /* Actual1994 always emits CO_INIT here, including references. */
        node=NodeBinary(72,target,initial);
        node->type=target->type;
        if(dup) node=NodeComma(node,dup);
        return node;
    case TYP_CLASS:
        if(!ctor) return ClassDefaultCopy(target,bareArg(initial));
        if(!target) {
            target=PTreeSetLocn(NodeTemporary(type),location);
            check_dtoring=1;
        } else check_dtoring=0;
        opt=0;
        if(initial && !initial->u.subtree[0] &&
           type==ClassTypeForType(initial->type)) {
            opt=AnalyseCallOpts(type,initial->u.subtree[1],&dtor,&bare);
            if(opt) initial=bareArg(initial);
        }
        if(opt==2) node=copyEliminateCtor(bare,initial,target,dtor);
        else if(opt==3) node=copyEliminateCall(bare,initial,target,dtor);
        else if(opt==0) {
            if(control&1) target->flags|=8;
            args=SymFuncArgList(ctor);
            this_type=type;
            if(args->qualifier) this_type=MakeModifiedType(type,args->qualifier);
            this_type=PointerTypeEquivalent(target->type)?
                MakePointerTo(this_type):MakeReferenceTo(this_type);
            result=CastImplicit(&target,this_type,0x82);
            if(ConversionDiagnose(result,target,&diagCtor)==2) {
                NodeFreeDupedExpr(initial);
                node=PTreeErrorNode(target);
            } else {
                node=NodeMakeCall(ctor,SymFuncReturnType(ctor),initial);
                node->flags|=2;
                node=PTreeSetLocn(node,location);
                AddDefaultArgs(ctor,node);
                node=CallArgsArrange(ctor->sym_type,node,node->u.subtree[1],
                    NodeArg(target),CallArgumentExactCtor(type,(control&1)!=0),NULL);
            }
        } else {
            /* Remaining old optimization codes do not establish node. */
            STUB_TRAP(); return NULL;
        }
        if(node->op==0) return node;
        if(check_dtoring) {
            node=NodeDtorExpr(node,target->u.symcg.symbol);
            if(node->op==0) return node;
        }
        node->flags|=8;
        return node;
    case TYP_VOID:
        /* Original CO_TRASH_EXPR reads an uninitialized local node here.
         * This invalid domain remains unsupported in both builds. */
        node=NodeUnary(93,node);
        node->type=type;
        return node;
    }
    return target;
}

SCOPE TypeScope(TYPE type);
char *CppConstructorName(void);
SEARCH_RESULT *ScopeFindBaseMember(SCOPE scope,char *name);
SEARCH_RESULT *ScopeContainsMember(SCOPE scope,char *name);
void ScopeResultErrLocn(SEARCH_RESULT *result,CGIO_LOCATION *locn);
void ScopeFreeResult(SEARCH_RESULT *result);
int ScopeCheckSymbol(SEARCH_RESULT *result,SYMBOL sym);
int TypeDefined(TYPE type);
unsigned ClassDefaultCtorDefine(TYPE type,SYMBOL *ctor);
SYMBOL ClassAddDefaultCopy(SCOPE scope);
unsigned char FuncOverloaded(SYMBOL *ctor,SYMBOL candidates,arg_list *args,PTREE *nodes);
SEARCH_RESULT *ctorResult(SCOPE access,CGIO_LOCATION *locn,TYPE type)
{
    SCOPE scope=TypeScope(type);
    char *name=CppConstructorName();
    SEARCH_RESULT *result;
    if(access) result=ScopeFindBaseMember(scope,name);
    else result=ScopeContainsMember(scope,name);
    if(locn && result) ScopeResultErrLocn(result,locn);
    return result;
}
int ctorDefineDefault(TYPE type,arg_list *args)
{
    SYMBOL ctor;
    if(!TypeDefined(type)) return 0;
    if(args->num_args==0) return ClassDefaultCtorDefine(type,&ctor)==0;
    if(args->num_args==1) return ClassAddDefaultCopy(type->u.c.scope)!=NULL;
    return 0;
}
unsigned char ctorExplicit(SCOPE access,TYPE type,arg_list *args,PTREE *nodes,
                           CGIO_LOCATION *locn,SYMBOL *ctor)
{
    SEARCH_RESULT *result;
    unsigned char status;
    type=ClassTypeForType(type);
    result=ctorResult(access,locn,type);
    if(!result) return 2;
    status=FuncOverloaded(ctor,result->sym_name->name_syms,args,nodes);
    if(status==0 && ScopeCheckSymbol(result,*ctor)) status=7;
    ScopeFreeResult(result);
    return status;
}
unsigned char ctorFindOverload(SCOPE access,TYPE type,arg_list *args,PTREE *nodes,
                               CGIO_LOCATION *locn,SYMBOL *ctor)
{
    unsigned char status=ctorExplicit(access,type,args,nodes,locn,ctor);
    if(status==2 && ctorDefineDefault(type,args))
        status=ctorExplicit(access,type,args,nodes,locn,ctor);
    return status;
}
unsigned CtorFind(SCOPE access,TYPE type,arg_list *args,PTREE *nodes,
                  CGIO_LOCATION *locn,SYMBOL *ctor)
{
    unsigned char status;
    *ctor=NULL;
    status=ctorFindOverload(access,type,args,nodes,locn,ctor);
    switch(status) {
    case 0: return 0;
    case 1: return 5;
    case 2: return 4;
    case 7: return 2;
    default:
        /* The original leaves its result uninitialized for statuses outside
         * this old overload-result domain; none is qualified here. */
        STUB_TRAP(); return 0;
    }
}

/* These1994 list-producing conversion searches predate the released
 * CtorFindList interface: they rank a candidate list using control2 and
 * retain the original default-constructor/copy-declaration retry. */
unsigned char resolveFunctionOverload(unsigned short control,FNOV_LIST **match,
    SYMBOL syms,arg_list *args,void *nodes);
unsigned char ctorExplicitList(TYPE type,arg_list *args,PTREE *nodes,
    FNOV_LIST **match,CGIO_LOCATION *locn)
{
    SEARCH_RESULT *result;
    unsigned char status;
    *match=NULL;
    type=ClassTypeForType(type);
    result=ctorResult(NULL,locn,type);
    if (!result) return 2;
    status=resolveFunctionOverload(2,match,result->sym_name->name_syms,args,nodes);
    if (status==0 && locn && ScopeCheckSymbol(result,(*match)->sym)) status=7;
    ScopeFreeResult(result);
    return status;
}
int ctorDefineDefaultCnv(TYPE type,arg_list *args)
{
    if (!TypeDefined(type)) return 0;
    if (args->num_args==0) return ctorDefineDefault(type,args);
    if (args->num_args==1)
        return ClassAddDefaultCopy(ClassTypeForType(type)->u.c.scope)!=NULL;
    return 0;
}
unsigned char ctorFindConversion(TYPE type,arg_list *args,PTREE *nodes,
    FNOV_LIST **match,CGIO_LOCATION *locn)
{
    unsigned char status=ctorExplicitList(type,args,nodes,match,locn);
    if (status==2 && ctorDefineDefaultCnv(type,args))
        status=ctorExplicitList(type,args,nodes,match,locn);
    return status;
}
