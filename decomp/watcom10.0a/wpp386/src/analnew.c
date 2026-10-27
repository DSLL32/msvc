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


/* Initial6b9cb443 analnew.c adapted to the1994 deletion path.  This
 * enclosing body is still under reconstruction; unsupported paths trap. */
#include "compiler.h"
#include "types.h"
#include "symbol.h"
#include "ptree.h"
#include "scope.h"
#include "pending.h"
#include "typesig.h"
#include <recon.h>
PTREE PTreeFree(PTREE expr);
PTREE NodeRvalue(PTREE expr);
PTREE PTreeExtractLocn(PTREE expr,CGIO_LOCATION *location);
void PTreeErrorExpr(PTREE expr,unsigned error);
TYPE PointerType(TYPE type);
int TypeIsConst(TYPE type);
int TypeTruncByMemModel(TYPE type);
TYPE ArrayBaseType(TYPE type);
SCOPE TypeScope(TYPE type);
int TypeDefined(TYPE type);
int ScopeHasVirtualFunctions(SCOPE scope);
void PTreeWarnExpr(PTREE expr,unsigned warning);
char *CppOperatorName(unsigned operation);
SEARCH_RESULT *ScopeFindMember(SCOPE scope,char *name);
void ScopeFreeResult(SEARCH_RESULT *result);
void ScopeResultErrLocn(SEARCH_RESULT *result,CGIO_LOCATION *locn);
SEARCH_RESULT *findNewDelOp(SCOPE scope,char *name)
{
    SEARCH_RESULT *result=NULL;
    if(!ScopeType(scope,0)) result=ScopeFindMember(scope,name);
    if(!result) result=ScopeFindNaked(FileScope,name);
    return result;
}
SYMBOL accessDelete(unsigned operation,SCOPE scope,unsigned *count,SEARCH_RESULT **presult)
{
    SEARCH_RESULT *result=findNewDelOp(scope,CppOperatorName(operation));
    SYMBOL sym=result->sym_name->name_syms;
    if(!presult) ScopeFreeResult(result);
    else *presult=result;
    *count=SymFuncArgList(sym)->num_args;
    return sym;
}
SYMBOL checkDeleteResult(SYMBOL sym,SEARCH_RESULT *result,CGIO_LOCATION *locn,int inside)
{
    if(result) {
        ScopeResultErrLocn(result,locn);
        if(!inside && ScopeCheckSymbol(result,sym)) sym=NULL;
        ScopeFreeResult(result);
    }
    return sym;
}
SYMBOL checkDeleteAccess(unsigned operation,SCOPE scope,unsigned *count,
                         CGIO_LOCATION *locn,int inside)
{
    SEARCH_RESULT *result;
    SYMBOL sym=accessDelete(operation,scope,count,&result);
    return checkDeleteResult(sym,result,locn,inside);
}
PTREE MakeNodeSymbol(SYMBOL sym);
PTREE NodeArguments(PTREE first,...);
PTREE RunTimeCall(PTREE args,TYPE type,unsigned code);
PTREE NodeMakeCall(SYMBOL sym,TYPE type,PTREE args);
PTREE NodeArgument(PTREE left,PTREE right);
PTREE NodeConvert(TYPE type,PTREE expr);
PTREE PTreeSetLocn(PTREE expr,CGIO_LOCATION *locn);
PTREE setDeleteType(PTREE expr,CGIO_LOCATION *locn)
{
    expr=NodeConvert(GetBasicType(TYP_VOID),expr);
    expr->flags|=0x11;
    PTreeSetLocn(expr,locn);
    return expr;
}
PTREE AnalyseDtorCall(TYPE type,PTREE this_node,unsigned extra);
PTREE NodeDupExpr(PTREE *expr);
PTREE NodeCompareToZero(PTREE expr);
PTREE NodeTestExpr(PTREE condition,PTREE yes,PTREE no);
PTREE NodeOffset(long offset);
PTREE deleteCheckForNULL(PTREE value,PTREE expr)
{
    PTREE condition=NodeCompareToZero(value);
    return NodeTestExpr(condition,expr,NodeOffset(0));
}
int TypeReallyDtorable(TYPE type);
PTREE PTreeErrorNode(PTREE expr);
PTREE AnalyseDelete(PTREE expr)
{
    PTREE data=expr->u.subtree[0];
    TYPE ptr_type,pted,cltype,type;
    SCOPE opdel_scope;
    SYMBOL sym;
    struct class_info *info;
    unsigned count,num_in_alloc=0;
    unsigned char oper=expr->cgop;
    int array_delete=oper==60 || oper==61;
    int inside=(expr->flags&0x400)!=0;
    CGIO_LOCATION err_locn;
    PTreeExtractLocn(expr,&err_locn);
    PTreeFree(expr);
    data=NodeRvalue(data);
    type=data->type;
    ptr_type=PointerType(type);
    if(!ptr_type) { PTreeErrorExpr(data,492); return data; }
    pted=ptr_type->of;
    if(TypeIsConst(pted)) { PTreeErrorExpr(data,491); return data; }
    if(TypeTruncByMemModel(pted)) { PTreeErrorExpr(data,163); return data; }
    pted=ArrayBaseType(pted);
    cltype=StructType(pted);
    if(!cltype) {
        if(FunctionDeclarationType(pted)) { PTreeErrorExpr(data,490); return data; }
        opdel_scope=FileScope;
    } else {
        if(!TypeDefined(cltype)) { PTreeErrorExpr(data,546); return data; }
        info=cltype->u.c.info;
        opdel_scope=TypeScope(cltype);
        if(info->needs_dtor && !inside && ScopeHasVirtualFunctions(opdel_scope) &&
           !info->needs_vdtor) PTreeWarnExpr(data,379);
        if(oper==61) {
            accessDelete(array_delete?60:58,opdel_scope,&count,NULL);
            if(count==2) num_in_alloc=1;
        }
        if(oper==59 || oper==61) opdel_scope=FileScope;
    }
    sym=checkDeleteAccess(array_delete?60:58,opdel_scope,&count,&err_locn,inside);
    if(!sym) { PTreeErrorNode(data); return data; }
    if(cltype) {
        if(!inside && info->needs_vdtor) {
            PTREE dup=data;
            data=NodeDupExpr(&dup);
            expr=AnalyseDtorCall(cltype,data,array_delete?4:2);
            expr=deleteCheckForNULL(dup,expr);
            return setDeleteType(expr,&err_locn);
        }
        if((!inside || oper!=58) && info->needs_dtor) {
            int really_dtorable=TypeReallyDtorable(cltype);
            if(!inside && !really_dtorable) { STUB_TRAP(); return NULL; }
            if(array_delete) {
                int errors;
                TYPE_SIG *sig=TypeSigFind(16,cltype,&err_locn,&errors);
                if(!errors) {
                    PTREE args;
                    TypeSigReferenced(sig);
                    /* NodeTypeSig is a source macro, not another entry. */
                    args=NodeArguments(MakeNodeSymbol(sig->sym),data,(PTREE)NULL);
                    expr=RunTimeCall(args,type,48);
                    if(count==2) { STUB_TRAP(); return NULL; }
                    args=NodeArgument(NULL,expr);
                    expr=NodeMakeCall(sym,GetBasicType(TYP_VOID),args);
                } else {
                    if(count==2) { STUB_TRAP(); return NULL; }
                    expr=NodeMakeCall(sym,GetBasicType(TYP_VOID),NodeArgument(NULL,data));
                }
                return setDeleteType(expr,&err_locn);
            }
        }
    }
    /* With no class destructor and the one-argument deallocator, the old
     * array path leaves the pointer unchanged and emits the ordinary call. */
    if(!cltype && count==1 && !num_in_alloc) {
        expr=NodeMakeCall(sym,GetBasicType(TYP_VOID),NodeArgument(NULL,data));
        return setDeleteType(expr,&err_locn);
    }
    if(inside && !array_delete && oper==58 && count!=2) {
        expr=NodeMakeCall(sym,GetBasicType(TYP_VOID),NodeArgument(NULL,data));
        return setDeleteType(expr,&err_locn);
    }
    (void)num_in_alloc;
    STUB_TRAP();
    return NULL;
}

void NodeFreeDupedExpr(PTREE expr);
PTREE NodeReverseArgs(unsigned *count,PTREE args);
TYPE ArrayType(TYPE type);
TYPE MakePointerTo(TYPE type);
TYPE PointerTypeForArray(TYPE type);
unsigned CgTypeSize(TYPE type);
PTREE NodeOffset(long offset);
PTREE NodeBinary(unsigned char operation,PTREE left,PTREE right);
PTREE FoldBinary(PTREE expr);
TYPE figureOutNewType(PTREE *number,TYPE *of_type)
{
    PTREE array_number=*number,extra;
    TYPE type=*of_type,test,base,expression_type;
    unsigned size,count;
    if(array_number==NULL && (test=ArrayType(type))!=NULL) {
        array_number=NodeOffset(test->u.a.array_size);
        expression_type=PointerTypeForArray(type);
        type=test->of;
    } else expression_type=MakePointerTo(type);
    test=ArrayType(type);
    if(test!=NULL) {
        size=CgTypeSize(type);
        base=ArrayBaseType(type);
        count=size/CgTypeSize(base);
        extra=NodeOffset(count);
        if(array_number!=NULL) {
            array_number=NodeRvalue(array_number);
            array_number=NodeBinary(2,array_number,extra);
            array_number->type=extra->type;
            array_number=FoldBinary(array_number);
        } else array_number=extra;
        type=base;
    }
    *of_type=type;
    *number=array_number;
    return expression_type;
}
PTREE NodeDupExpr(PTREE *expr);
PTREE NodeCompareToZero(PTREE expr);
PTREE NodeTestExpr(PTREE condition,PTREE if_true,PTREE if_false);
PTREE newCheckForNULL(PTREE value,PTREE expr)
{
    PTREE if_false=value;
    value=NodeDupExpr(&if_false);
    return NodeTestExpr(NodeCompareToZero(value),expr,if_false);
}
PTREE sizeOfUInt(void) { STUB_TRAP(); return NULL; }
PTREE setupArrayStorage(PTREE expr,TYPE type,PTREE number)
{ (void)expr; (void)type; (void)number; STUB_TRAP(); return NULL; }
unsigned AnalyseCtor(TYPE type,SYMBOL *ctor,PTREE *initial);
TYPE TypeNeedsCtor(TYPE type);
void SetErrLoc(CGIO_LOCATION *location);
void CallDiagAmbiguous(PTREE expr,unsigned message);
TYPE NodeType(PTREE expr);
void ConversionTypesSet(TYPE first,TYPE second);
PTREE PTreeCopySrcLocation(PTREE target,PTREE source);
void CtorDiagNoMatch(PTREE expr,unsigned message);
void ConversionDiagnoseInf(void);
unsigned checkNewCtor(TYPE class_type,TYPE base_type,SYMBOL *ctor,PTREE *a_initial,
                      CGIO_LOCATION *location)
{
    unsigned result=AnalyseCtor(base_type,ctor,a_initial);
    PTREE initial=*a_initial;
    switch(result) {
    case 0: case 2: break;
    case 5: CallDiagAmbiguous(initial,303); break;
    case 4:
        if(initial==NULL) {
            if(class_type==NULL || !TypeNeedsCtor(class_type)) result=0;
            else { SetErrLoc(location); CErr1(580); }
        } else {
            ConversionTypesSet(NodeType(initial),base_type);
            initial=NodeBinary(62,NULL,initial);
            *a_initial=initial;
            initial=PTreeCopySrcLocation(initial,initial->u.subtree[1]);
            CtorDiagNoMatch(initial,580);
            ConversionDiagnoseInf();
        }
        break;
    default:
        if(initial==NULL) { SetErrLoc(location); CErr1(580); }
        else {
            ConversionTypesSet(NodeType(initial),base_type);
            PTreeErrorExpr(initial,580);
            ConversionDiagnoseInf();
        }
    }
    return result;
}
SCOPE scopeLookup(TYPE type)
{
    type=StructType(type);
    return type==NULL?FileScope:type->u.c.scope;
}
SCOPE opNewSearchScope(TYPE type,unsigned char operation)
{
    SCOPE scope=FileScope;
    if(operation!=64) scope=scopeLookup(type);
    return scope;
}
void NodeBuildArgList(arg_list *args,PTREE *nodes,PTREE node,unsigned count);
unsigned char FuncOverloaded(SYMBOL *sym,SYMBOL candidates,arg_list *args,PTREE *nodes);
PTREE NodeConvertCallArgs(PTREE node,arg_list *args,TYPE type);
void AddDefaultArgs(SYMBOL sym,PTREE expr);
void CallDiagNoMatch(PTREE expr,unsigned one,unsigned many,PTREE this_node,SYMBOL sym);
PTREE buildNewCall(PTREE node,SCOPE scope,TYPE type,unsigned count,
                  CGIO_LOCATION *location,unsigned operation)
{
    SEARCH_RESULT *result=findNewDelOp(scope,CppOperatorName(operation));
    SYMBOL sym;
    arg_list *args;
    PTREE *nodes;
    unsigned char status;
    ScopeResultErrLocn(result,location);
    args=AllocArgList(count,0);
    nodes=CMemAlloc(count*sizeof(PTREE));
    NodeBuildArgList(args,nodes,node,count);
    sym=result->sym_name->name_syms;
    status=FuncOverloaded(&sym,sym,args,nodes);
    if(status==0) {
        if(ScopeCheckSymbol(result,sym)) PTreeErrorNode(node);
        else {
            node=NodeMakeCall(sym,type,node);
            node=NodeConvertCallArgs(node,args,sym->sym_type);
            if(node->op!=0) AddDefaultArgs(sym,node);
        }
    } else {
        sym=result->sym_name->name_syms;
        node=NodeMakeCall(sym,type,node);
        switch(status) {
        case 1: CallDiagAmbiguous(node,228); break;
        case 2: CallDiagNoMatch(node,473,215,NULL,sym); break;
        }
    }
    ScopeFreeResult(result);
    CMemFree(args);
    CMemFree(nodes);
    return node;
}
int TypeReallyDefCtorable(TYPE type);
unsigned CgTypeSize(TYPE type);
PTREE NodeOffset(long offset);
PTREE NodeDupExpr(PTREE *expr);
PTREE NodeBinary(unsigned char operation,PTREE left,PTREE right);
PTREE FoldBinary(PTREE expr);
TYPE IntegralType(TYPE type);
SYMBOL ClassFunMakeAddressable(SYMBOL sym);
int TypeRequiresDtoring(TYPE type);
int TypeHasVirtualBases(TYPE type);
PTREE NodeArg(PTREE expr);
PTREE PTreeIc(unsigned opcode,unsigned value);
PTREE NodeComma(PTREE left,PTREE right);
PTREE NodeAddSideEffect(PTREE effect,PTREE expr);
PTREE NodeIc(unsigned opcode);
void FunctionHasRegistration(void);
PTREE EffectCtor(PTREE expr,SYMBOL ctor,TYPE type,PTREE target,
                 CGIO_LOCATION *location,unsigned control);
PTREE AnalyseNew(PTREE expr,TYPE type)
{
    unsigned count_placement,num_args,ctor_overload,rt_code;
    PTREE parms1,parms2,placement,initial,array_number,elem_size,node,dup,args;
    TYPE base_type,class_type,expression_type,offset_type,integral_type;
    SYMBOL ctor,op_delete,address_delete;
    SEARCH_RESULT *delete_result;
    CGIO_LOCATION location;
    unsigned char operation;
    int errors,really_dtorable;
    TYPE_SIG *sig;
    if(!TypeDefined(type)) { PTreeErrorExpr(expr,545); return expr; }
    PTreeExtractLocn(expr,&location);
    parms1=expr->u.subtree[1];
    placement=NodeReverseArgs(&count_placement,parms1->u.subtree[0]);
    parms1->u.subtree[0]=NULL;
    parms2=parms1->u.subtree[1];
    initial=parms2->u.subtree[1]; parms2->u.subtree[1]=NULL;
    array_number=parms2->u.subtree[0]; parms2->u.subtree[0]=NULL;
    operation=expr->cgop;
    NodeFreeDupedExpr(expr);
    expression_type=figureOutNewType(&array_number,&type);
    elem_size=NodeOffset(CgTypeSize(type));
    offset_type=elem_size->type;
    base_type=ArrayBaseType(type);
    class_type=StructType(type);
    ctor_overload=checkNewCtor(class_type,base_type,&ctor,&initial,&location);
    if(ctor_overload!=0) {
        NodeFreeDupedExpr(placement); NodeFreeDupedExpr(elem_size);
        NodeFreeDupedExpr(array_number); NodeFreeDupedExpr(initial);
        return PTreeErrorNode(NULL);
    }
    if(ctor!=NULL && initial==NULL && !TypeReallyDefCtorable(class_type)) ctor=NULL;
    really_dtorable=TypeReallyDtorable(class_type);
    op_delete=accessDelete(array_number==NULL?58:60,scopeLookup(class_type),
                           &num_args,&delete_result);
    address_delete=ClassFunMakeAddressable(op_delete);
    if(array_number!=NULL) {
        array_number=NodeRvalue(array_number);
        integral_type=IntegralType(array_number->type);
        if(integral_type==NULL || initial!=NULL) {
            if(integral_type==NULL) PTreeErrorExpr(array_number,529);
            else PTreeErrorExpr(initial,223);
            ScopeFreeResult(delete_result);
            NodeFreeDupedExpr(placement); NodeFreeDupedExpr(elem_size);
            NodeFreeDupedExpr(array_number); NodeFreeDupedExpr(initial);
            return PTreeErrorNode(NULL);
        }
        dup=array_number;
        if(ctor!=NULL || num_args==2 || really_dtorable)
            array_number=NodeDupExpr(&dup);
        node=NodeBinary(2,dup,elem_size); node->type=offset_type;
        node=FoldBinary(node);
        if(really_dtorable || num_args==2) {
            node=NodeBinary(0,node,sizeOfUInt()); node->type=offset_type;
            node=FoldBinary(node);
        }
    } else node=elem_size;
    node->locn=location;
    node=NodeArgument(placement,node);
    ++count_placement;
    node=buildNewCall(node,opNewSearchScope(class_type,operation),expression_type,
                      count_placement,&location,array_number==NULL?63:65);
    if(node->op==0) {
        ScopeFreeResult(delete_result);
        NodeFreeDupedExpr(node); NodeFreeDupedExpr(array_number);
        NodeFreeDupedExpr(initial);
        return PTreeErrorNode(NULL);
    }
    if(array_number!=NULL) {
        if(ctor!=NULL) {
            sig=TypeSigFind(TypeRequiresDtoring(class_type)?24:8,class_type,
                            &location,&errors);
            if(errors) {
                ScopeFreeResult(delete_result);
                NodeFreeDupedExpr(node); NodeFreeDupedExpr(array_number);
                NodeFreeDupedExpr(initial);
                return PTreeErrorNode(NULL);
            }
            TypeSigReferenced(sig);
            op_delete=checkDeleteResult(op_delete,delete_result,&location,0);
            if(op_delete==NULL) {
                NodeFreeDupedExpr(node); NodeFreeDupedExpr(array_number);
                NodeFreeDupedExpr(initial);
                return PTreeErrorNode(NULL);
            }
            if(num_args==1) {
                if(!really_dtorable) {
                    /*1994 has the two simple array-ctor runtime variants,
                     * selected by whether the class has virtual bases. */
                    rt_code=TypeHasVirtualBases(class_type)?9:8;
                    args=NULL;
                } else if(SymScope(op_delete)==FileScope && address_delete==op_delete) {
                    rt_code=49; args=NULL;
                } else { rt_code=50; args=NodeArg(MakeNodeSymbol(address_delete)); }
            } else { rt_code=51; args=NodeArg(MakeNodeSymbol(address_delete)); }
            args=NodeArgument(args,MakeNodeSymbol(sig->sym));
            args=NodeArgument(args,array_number);
            args=NodeArgument(args,node);
            node=RunTimeCall(args,expression_type,rt_code);
            if(placement!=NULL) {
                dup=node; node=NodeDupExpr(&dup);
                node=newCheckForNULL(dup,node);
            }
        } else {
            ScopeFreeResult(delete_result);
            if(really_dtorable || num_args==2) {
                dup=node; node=NodeDupExpr(&dup);
                expr=setupArrayStorage(dup,expression_type,array_number);
                node=newCheckForNULL(node,expr);
            }
        }
    } else {
        ScopeFreeResult(delete_result);
        if(initial!=NULL || ctor!=NULL) {
            dup=node; node=NodeDupExpr(&dup);
            if(ctor!=NULL && placement==NULL) {
                FunctionHasRegistration();
                dup=NodeComma(dup,PTreeIc(122,(unsigned)class_type));
            }
            dup->type=type; dup->flags|=2;
            expr=EffectCtor(initial,ctor,base_type,dup,&location,3);
            if(expr->op==0) { NodeFreeDupedExpr(node); node=expr; }
            else {
                if(ctor!=NULL && placement==NULL)
                    expr=NodeAddSideEffect(NodeIc(123),expr);
                expr=NodeConvert(expression_type,expr);
                node=newCheckForNULL(node,expr);
            }
        }
    }
    PTreeSetLocn(node,&location);
    return node;
}

