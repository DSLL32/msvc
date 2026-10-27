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


/* Initial6b9cb443 opovload.c/ppopscnv.h, adapted to1994's54
 * named prototypes: no INT64 or later RPI rows; old SCALAR_ASSIGN bit
 * retained. All54 type pairs and masks match actualb74f4/b75cc data.
 * The old initializer has no extension-extra-prototypes/reporting pass. */
#include "compiler.h"
#include "types.h"
#include "symbol.h"
#include "ptree.h"
#include "pending.h"
#include "fnovload.h"
#include "analcall.h"
#include <recon.h>
enum {
    OPM_AA=0x1,
    OPM_II=0x2,
    OPM_PP=0x4,
    OPM_A=0x8,
    OPM_RA=0x10,
    OPM_P=0x20,
    OPM_RP=0x40,
    OPM_I=0x80,
    OPM_RI=0x100,
    OPM_IP=0x200,
    OPM_PI=0x400,
    OPM_MM=0x800,
    OPM_PM=0x1000,
    OPM_FUN=0x2000,
    OPM_ASSIGN=0x4000,
    OPM_QUEST=0x8000,
    OPM_SCALAR_ASSIGN=0x10000,
    OPM_POST=0x20000,
    OPM_SUB=0x40000,
    OPM_SH=0x80000,
    OPM_RR=0x100000,
    OPM_LT=0x200000,
    OPM_NV=0x400000
};
struct operator_proto { type_id first,second; unsigned mask; };
/* Genuine const data public bounds adjacent period code envelopes. */
const struct operator_proto operator_prototypes[] = {
    { TYP_SINT,TYP_MAX,OPM_I  | OPM_A },
    { TYP_UINT,TYP_MAX,OPM_I  | OPM_A },
    { TYP_SLONG,TYP_MAX,OPM_I  | OPM_A },
    { TYP_ULONG,TYP_MAX,OPM_I  | OPM_A },
    { TYP_FLOAT,TYP_MAX,OPM_A },
    { TYP_DOUBLE,TYP_MAX,OPM_A },
    { TYP_LONG_DOUBLE,TYP_MAX,OPM_A },
    { TYP_POINTER,TYP_MAX,OPM_P  | OPM_A },
    { TYP_CHAR,TYP_CHAR,OPM_RA | OPM_RI | OPM_ASSIGN },
    { TYP_SCHAR,TYP_SCHAR,OPM_RA | OPM_RI | OPM_ASSIGN },
    { TYP_UCHAR,TYP_UCHAR,OPM_RA | OPM_RI | OPM_ASSIGN },
    { TYP_WCHAR,TYP_WCHAR,OPM_RA | OPM_RI | OPM_ASSIGN },
    { TYP_SSHORT,TYP_SSHORT,OPM_RA | OPM_RI | OPM_ASSIGN },
    { TYP_USHORT,TYP_USHORT,OPM_RA | OPM_RI | OPM_ASSIGN },
    { TYP_SINT,TYP_SINT,OPM_RA | OPM_RI | OPM_ASSIGN },
    { TYP_SINT,TYP_SINT,OPM_II | OPM_AA | OPM_SH },
    { TYP_SINT,TYP_UINT,OPM_SH },
    { TYP_SINT,TYP_SLONG,OPM_SH },
    { TYP_SINT,TYP_ULONG,OPM_SH },
    { TYP_SINT,TYP_POINTER,OPM_IP },
    { TYP_UINT,TYP_SINT,OPM_SH },
    { TYP_UINT,TYP_UINT,OPM_RA | OPM_RI | OPM_ASSIGN },
    { TYP_UINT,TYP_UINT,OPM_II | OPM_AA | OPM_SH },
    { TYP_UINT,TYP_SLONG,OPM_SH },
    { TYP_UINT,TYP_ULONG,OPM_SH },
    { TYP_UINT,TYP_POINTER,OPM_IP },
    { TYP_SLONG,TYP_SINT,OPM_SH },
    { TYP_SLONG,TYP_UINT,OPM_SH },
    { TYP_SLONG,TYP_SLONG,OPM_RA | OPM_RI | OPM_ASSIGN },
    { TYP_SLONG,TYP_SLONG,OPM_II | OPM_AA | OPM_SH },
    { TYP_SLONG,TYP_ULONG,OPM_SH },
    { TYP_SLONG,TYP_POINTER,OPM_IP },
    { TYP_ULONG,TYP_SINT,OPM_SH },
    { TYP_ULONG,TYP_UINT,OPM_SH },
    { TYP_ULONG,TYP_SLONG,OPM_SH },
    { TYP_ULONG,TYP_ULONG,OPM_RA | OPM_RI | OPM_ASSIGN },
    { TYP_ULONG,TYP_ULONG,OPM_II | OPM_AA | OPM_SH },
    { TYP_ULONG,TYP_POINTER,OPM_IP },
    { TYP_FLOAT,TYP_FLOAT,OPM_RA          | OPM_ASSIGN },
    { TYP_FLOAT,TYP_FLOAT,OPM_AA },
    { TYP_DOUBLE,TYP_DOUBLE,OPM_RA          | OPM_ASSIGN },
    { TYP_DOUBLE,TYP_DOUBLE,OPM_AA },
    { TYP_LONG_DOUBLE,TYP_LONG_DOUBLE,OPM_RA          | OPM_ASSIGN },
    { TYP_LONG_DOUBLE,TYP_LONG_DOUBLE,OPM_AA },
    { TYP_POINTER,TYP_SINT,OPM_PI },
    { TYP_POINTER,TYP_UINT,OPM_PI },
    { TYP_POINTER,TYP_SLONG,OPM_PI },
    { TYP_POINTER,TYP_ULONG,OPM_PI },
    { TYP_POINTER,TYP_POINTER,OPM_RP |          OPM_ASSIGN },
    { TYP_POINTER,TYP_POINTER,OPM_PP },
    { TYP_POINTER,TYP_MEMBER_POINTER,OPM_PM },
    { TYP_MEMBER_POINTER,TYP_MEMBER_POINTER,OPM_MM },
    { TYP_MAX,TYP_MAX,OPM_RR },
    { TYP_MAX,TYP_MAX,0 },
};
static SYMBOL ovfuns[54];
static PTREE nullArgument;
TYPE MakeReferenceTo(TYPE type);
void CheckUniqueType(TYPE type);
TYPE TypePtrToVoidStdop(void);
TYPE TypePtrToVoidStdopArith(void);
TYPE TypePtrToConstVoidStdop(void);
TYPE TypePtrToConstVoidStdopArith(void);
TYPE TypeVoidMemberPtr(void);
TYPE getOperandType(type_id id,unsigned control)
{
    if (id==TYP_POINTER) {
        if (control&1) {
            if (control&2) return TypePtrToConstVoidStdopArith();
            return TypePtrToConstVoidStdop();
        }
        if (control&2) return TypePtrToVoidStdopArith();
        return TypePtrToVoidStdop();
    }
    if (id==TYP_MEMBER_POINTER) return TypeVoidMemberPtr();
    return GetBasicType(id);
}
void overloadOperatorInit(INITFINI *defn)
{
    unsigned i,mask,control;
    TYPE first,result,second,function;
    SYMBOL fun;
    arg_list *args;
    (void)defn;
    for (i=0;i<54;++i) {
        mask=operator_prototypes[i].mask;
        if (mask==0 || (mask&OPM_RR)) {
            first=MakeReferenceTo(GetBasicType(TYP_CHAR));
            args=AllocArgList(2,1);
            args->type_list[0]=first;
            args->type_list[1]=first;
            function=MakeType(TYP_FUNCTION);
            function->u.f.args=args;
            function->of=first;
            function->flag|=4;
            CheckUniqueType(function);
        } else {
            result=getOperandType(operator_prototypes[i].first,1);
            control=(mask&OPM_ASSIGN)?1:0;
            if (mask&OPM_NV) control|=2;
            first=getOperandType(operator_prototypes[i].first,control);
            if (mask&OPM_ASSIGN) {
                first=MakeReferenceTo(first);
                result=MakeReferenceTo(result);
            }
            if (operator_prototypes[i].second==TYP_MAX)
                function=MakeSimpleFunction(result,first,(TYPE)0);
            else {
                control=(mask&OPM_NV)?2:0;
                second=getOperandType(operator_prototypes[i].second,control);
                function=MakeSimpleFunction(result,first,second,(TYPE)0);
            }
        }
        fun=AllocSymbol();
        fun->sym_type=function;
        fun->u.uval=i;
        ovfuns[i]=fun;
    }
    nullArgument=PTreeIntConstant(0,TYP_SINT);
}

/* The1994 overload entry precedes the later OLINF consolidation. It keeps
 * the actual scalar-symbol ring, operator mask and repeat flag separately. */
static int repeat_overload;
unsigned short PTreeOpFlags(PTREE expr);
/* The encountered 24 conversion classes at b7638..b7698 follow the
 * original ppopscnv.h order. Index zero is no conversion, not a table row.
 * The older COLON class and SCALAR_ASSIGN mask retain their 1994 meanings. */
static unsigned conversion_masks[24]={
    OPM_NV|OPM_PI|OPM_IP|OPM_AA,             /* PLUS */
    OPM_NV|OPM_PI|OPM_AA,                    /* MINUS */
    OPM_A,                                  /* ONE_AR */
    OPM_NV|OPM_ASSIGN|OPM_RP|OPM_RA,          /* ONE_RAP */
    OPM_NV|OPM_POST|OPM_RP|OPM_RA,            /* POST_OP */
    OPM_AA,                                 /* TWO_AR */
    OPM_I,                                  /* ONE_INT */
    OPM_II,                                 /* TWO_INT */
    OPM_SH,                                 /* SHIFT */
    OPM_MM|OPM_PP|OPM_AA,                    /* RELEQ */
    OPM_RR|OPM_MM|OPM_PP|OPM_AA,             /* COLON */
    OPM_PP|OPM_AA,                           /* RELN */
    OPM_P|OPM_A,                            /* LOG1 */
    OPM_PI|OPM_IP|OPM_PP|OPM_AA,             /* LOG2 */
    OPM_NV|OPM_SUB|OPM_PI|OPM_IP,             /* SUBSCR */
    OPM_P,                                  /* PTR */
    OPM_LT|OPM_FUN,                          /* FUNCTION */
    OPM_ASSIGN|OPM_RP|OPM_RA,                /* EQAP */
    OPM_ASSIGN|OPM_RA,                       /* EQAR */
    OPM_ASSIGN|OPM_RI,                       /* EQINT */
    OPM_LT|OPM_QUEST|OPM_P|OPM_A,             /* QUESTION */
    OPM_LT,                                 /* ARROW */
    OPM_LT|OPM_ASSIGN,                       /* EQUALS */
    OPM_PM                                  /* ARROW_STAR */
};
unsigned getOperatorMask(unsigned short flags,unsigned *mask)
{
    unsigned index=(flags&0x1f0)>>4;
    if(index==0) *mask=0;
    else *mask=conversion_masks[index-1];
    return index;
}
TYPE ClassTypeForType(TYPE type);
TYPE UserTypeForType(TYPE type);
TYPE operandUserType(PTREE expr,unsigned mask)
{
    if(!expr) return NULL;
    return mask&OPM_RR ? ClassTypeForType(expr->type) : UserTypeForType(expr->type);
}
void overloadColon(unsigned current,PTREE expr,SYMBOL *list)
{ (void)current; (void)expr; (void)list; STUB_TRAP(); }
SYMBOL scalarOperators(PTREE expr,unsigned short flags,unsigned *mask)
{
    SYMBOL list=NULL;
    TYPE left,right;
    unsigned i,m;
    if(getOperatorMask(flags,mask)) {
        left=operandUserType(expr->u.subtree[0],*mask);
        right=((*mask&OPM_LT) || expr->op!=9)?NULL:
            operandUserType(expr->u.subtree[1],*mask);
        if(left || right) {
            for(i=0;i<54;++i) {
                m=operator_prototypes[i].mask;
                if(*mask&m) {
                    if(m&OPM_RR) overloadColon(i,expr,&list);
                    else RingAppend((void **)&list,ovfuns[i]);
                }
            }
        }
    }
    return list;
}
TYPE ClassTypeForType(TYPE type);
TYPE EnumTypeForType(TYPE type);
SCOPE TypeScope(TYPE type);
char *CppOperatorName(unsigned oper);
int NodeIsBinaryOp(PTREE expr,unsigned op);
SEARCH_RESULT *ScopeContainsMember(SCOPE scope,char *name);
SEARCH_RESULT *ScopeFindMember(SCOPE scope,char *name);
PTREE PTreeIdSym(SYMBOL sym);
PTREE NodeSymbolNoRef(PTREE expr,SYMBOL sym,SEARCH_RESULT *result);
TYPE NodeType(PTREE expr);
unsigned char OpOverloaded(SYMBOL *fun,SEARCH_RESULT *member,
                          SEARCH_RESULT *nonmember,SYMBOL scalars,
                          arg_list *args,PTREE *plist);
SYMBOL FnovGetAmbiguousEntry(FNOV_LIST **ptr)
{ (void)ptr; STUB_TRAP(); return NULL; }
SYMBOL FnovNextAmbiguousEntry(void)
{ STUB_TRAP(); return NULL; }
void InfSymbolAmbiguous(SYMBOL sym)
{ (void)sym; STUB_TRAP(); }
PTREE NodeBinary(unsigned op,PTREE left,PTREE right);
PTREE NodeArg(PTREE expr);
PTREE build_fun_name(SEARCH_RESULT *result)
{
    SYMBOL_NAME name=result->sym_name;
    return NodeSymbolNoRef(PTreeId(name->name),name->name_syms,result);
}
PTREE transform_to_call(PTREE expr,PTREE caller,PTREE args)
{
    expr->cgop=70;
    expr->op=9;
    expr->flags=0;
    expr->u.subtree[0]=caller;
    expr->u.subtree[1]=args;
    return expr;
}
PTREE transform_member(PTREE expr,SEARCH_RESULT *member)
{
    PTREE fun,caller,param;
    if(expr->cgop==23) {
        /* Old arrow transformation has no later ArrowMemberOK helper. */
        repeat_overload=1;
        fun=build_fun_name(member);
        caller=NodeBinary(23,expr->u.subtree[0],fun);
        caller=NodeBinary(47,caller,NULL);
        expr->u.subtree[0]=AnalyseCall(caller,NULL);
    } else {
        if(expr->op==9) {
            param=expr->u.subtree[1];
            if(expr->cgop!=47) param=NodeArg(param);
        } else if(expr->cgop==38 || expr->cgop==39)
            param=NodeArg(PTreeIntConstant(0,TYP_SINT));
        else param=NULL;
        fun=build_fun_name(member);
        caller=NodeBinary(48,expr->u.subtree[0],fun);
        caller->type=fun->type;
        expr=transform_to_call(expr,caller,param);
    }
    return expr;
}
PTREE transform_naked(PTREE expr,SEARCH_RESULT *nonmember)
{ (void)expr; (void)nonmember; STUB_TRAP(); return NULL; }
PTREE transform_conversions(PTREE expr,SYMBOL fun,unsigned mask)
{ (void)expr; (void)fun; (void)mask; STUB_TRAP(); return NULL; }
PTREE NodeBinary(unsigned op,PTREE left,PTREE right);
void PTreeErrorExpr(PTREE tree,unsigned error);
/* Actual old explicit argument form: no later OLINF, FNOV_DIAG object,
 * namespace result or extension retry. Ambiguity lists belong to fnovload. */
PTREE resolve_symbols(PTREE expr,SEARCH_RESULT *member,
                     SEARCH_RESULT *nonmember,SYMBOL scalars,unsigned mask)
{
    PTREE plist[2],oper;
    struct { arg_list base; TYPE second; } alist;
    SYMBOL fun,next;
    FNOV_LIST *amb_list;
    unsigned count,i;
    unsigned char result;
    if(!member && !nonmember && !scalars) return expr;
    if(mask&OPM_FUN) {
        fun=member->sym_name->name_syms;
        oper=NodeSymbolNoRef(PTreeIdSym(fun),fun,member);
        oper->cgop=195;             /* CO_NAME_DOT */
        oper=NodeBinary(48,expr->u.subtree[0],oper);
        oper->type=fun->sym_type;
        oper->flags|=2;
        expr->u.subtree[0]=oper;
        return expr;
    }
    if(expr->op==9) {
        if(mask&OPM_ASSIGN) {
            plist[1]=expr->u.subtree[1];
            count=2;
        } else if(mask&OPM_LT) count=1;
        else {
            if((mask&OPM_SUB) && member) scalars=NULL;
            plist[1]=expr->u.subtree[1];
            count=2;
        }
    } else if(mask&OPM_POST) {
        plist[1]=nullArgument;
        count=2;
    } else count=1;
    plist[0]=expr->u.subtree[0];
    InitArgList(&alist.base);
    alist.base.num_args=count;
    for(i=count;i!=0;--i) alist.base.type_list[i-1]=NodeType(plist[i-1]);
    result=OpOverloaded(&fun,member,nonmember,scalars,&alist.base,plist);
    switch(result) {
    case 1:                        /* FNOV_AMBIGUOUS */
        fun=NULL;
        amb_list=NULL;
        for(;;) {
            next=FnovGetAmbiguousEntry(&amb_list);
            if(!next) break;
            if(next->name==NULL && next->id!=9 && next->u.uval<52) {
                if(!fun || fun->u.uval<next->u.uval) fun=next;
            } else { fun=NULL; break; }
        }
        if(!fun) {
            PTreeErrorExpr(expr,293);
            for(;;) {
                next=FnovNextAmbiguousEntry();
                if(!next) break;
                if(next->name==NULL) CErr2p(646,SymFuncArgList(next)->type_list[0]);
                else InfSymbolAmbiguous(next);
            }
            ScopeFreeResult(member);
            ScopeFreeResult(nonmember);
            break;
        }
        /* A selected built-in candidate uses the normal transform below. */
    case 0:                        /* FNOV_NONAMBIGUOUS */
        if(fun->id==9) {
            expr=transform_member(expr,member);
            ScopeFreeResult(nonmember);
        } else if(fun->name==NULL) {
            expr=transform_conversions(expr,fun,mask);
            ScopeFreeResult(member);
            ScopeFreeResult(nonmember);
        } else {
            expr=transform_naked(expr,nonmember);
            ScopeFreeResult(member);
        }
        break;
    case 2:                        /* FNOV_NO_MATCH */
        ScopeFreeResult(member);
        ScopeFreeResult(nonmember);
        break;
    }
    return expr;
}
PTREE resolveOperator(PTREE expr,unsigned short flags,SYMBOL scalars,unsigned mask)
{
    SEARCH_RESULT *member=NULL,*nonmember=NULL;
    TYPE type;
    SCOPE scope;
    char *name;
    int lookup;
    if(flags&8) {
        type=ClassTypeForType(expr->u.subtree[0]->type);
        if(type) {
            scope=TypeScope(type);
            name=CppOperatorName(expr->cgop);
            if(NodeIsBinaryOp(expr,13))
                member=ScopeContainsMember(scope,name);
            else {
                member=ScopeFindMember(scope,name);
                nonmember=ScopeFindNaked(FileScope,name);
            }
        } else if(!(mask&(OPM_SUB|OPM_FUN|OPM_QUEST)) &&
                  !NodeIsBinaryOp(expr,13)) {
            lookup=0;
            if(expr->op==9 && expr->u.subtree[1] &&
               ClassTypeForType(expr->u.subtree[1]->type)) lookup=1;
            if(!lookup && EnumTypeForType(expr->u.subtree[0]->type)) lookup=1;
            if(!lookup && expr->op==9 && expr->u.subtree[1] &&
               EnumTypeForType(expr->u.subtree[1]->type)) lookup=1;
            if(lookup)
                nonmember=ScopeContainsMember(FileScope,CppOperatorName(expr->cgop));
        }
    }
    return resolve_symbols(expr,member,nonmember,scalars,mask);
}
PTREE OverloadOperator(PTREE expr)
{
    unsigned short flags;
    unsigned mask;
    SYMBOL scalars;
    do {
        repeat_overload=0;
        flags=PTreeOpFlags(expr);
        scalars=scalarOperators(expr,flags,&mask);
        expr=resolveOperator(expr,flags,scalars,mask);
    } while(repeat_overload && expr->op!=0);
    return expr;
}

PTREE PTreeOp(PTREE *expr);
PTREE PTreeOpLeft(PTREE expr);
PTREE PTreeErrorNode(PTREE expr);
int SymIsThisFuncMember(SYMBOL sym);
PTREE FoldBinary(PTREE expr);
PTREE FoldUnary(PTREE expr);
TYPE MemberPtrType(TYPE type);
PTREE MembPtrCompare(PTREE expr) { (void)expr; STUB_TRAP(); return NULL; }
/* Old isBadFun diagnoses only the actual member-function symbol case;
 * the later NodeAddrOfFun/overloaded-function diagnostics are absent. */
int isBadFun(PTREE expr,PTREE operand)
{
    PTREE node=PTreeOp(&operand);
    if(node->op==7 && SymIsThisFuncMember(node->u.symcg.symbol)) {
        PTreeErrorExpr(node,562);
        PTreeErrorNode(expr);
        return 1;
    }
    return 0;
}
PTREE ConvertBoolean(PTREE expr)
{
    PTREE left=expr->u.subtree[0];
    SYMBOL scalars;
    unsigned mask;
    if(ClassTypeForType(left->type)) {
        scalars=scalarOperators(expr,PTreeOpFlags(expr),&mask);
        expr=resolve_symbols(expr,NULL,NULL,scalars,mask);
        if(expr->op!=0 && ClassTypeForType(expr->u.subtree[0]->type)) {
            PTreeErrorExpr(PTreeOpLeft(expr),554);
            PTreeErrorNode(expr);
        }
    } else if(MemberPtrType(left->type)) expr=MembPtrCompare(expr);
    else if(expr->op==9) {
        if(!isBadFun(expr,expr->u.subtree[0]) &&
           !isBadFun(expr,expr->u.subtree[1])) expr=FoldBinary(expr);
    } else if(expr->op==8) {
        if(!isBadFun(expr,expr->u.subtree[0])) expr=FoldUnary(expr);
    }
    return expr;
}

void overloadOperatorFini(INITFINI *defn)
{
    unsigned arg;
    (void)defn;
    for (arg=0;arg<54;++arg) if (ovfuns[arg]) FreeSymbol(ovfuns[arg]);
    /* This old table also owns the shared null-argument parse node. */
    PTreeFree(nullArgument);
}
