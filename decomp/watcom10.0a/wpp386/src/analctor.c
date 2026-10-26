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
#include <recon.h>
unsigned CastImplicit(PTREE *expr,TYPE type,unsigned control);
void PTreeErrorExpr(PTREE expr,unsigned error);
TYPE TypedefModifierRemoveOnly(TYPE type);
TYPE ArrayBaseType(TYPE type);
unsigned analyseCtorClass(TYPE type,SCOPE access,SYMBOL *ctor,PTREE initial,PTREE *expr)
{ (void)type; (void)access; (void)ctor; (void)initial; (void)expr; STUB_TRAP(); return 0; }
unsigned AnalyseCtor(TYPE type,SYMBOL *ctor,PTREE *initial)
{ (void)type; (void)ctor; (void)initial; STUB_TRAP(); return 0; }
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
PTREE NodeDupExpr(PTREE *expr) { (void)expr; STUB_TRAP(); return NULL; }
PTREE NodeComma(PTREE left,PTREE right) { (void)left; (void)right; STUB_TRAP(); return NULL; }
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
PTREE PTreeSetLocn(PTREE expr,CGIO_LOCATION *location)
{ (void)expr; (void)location; STUB_TRAP(); return NULL; }
unsigned char AnalyseCallOpts(TYPE type,PTREE expr,PTREE *dtor,PTREE *bare)
{ (void)type; (void)expr; (void)dtor; (void)bare; STUB_TRAP(); return 0; }
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
PTREE NodeMakeCall(SYMBOL sym,TYPE type,PTREE arguments)
{ (void)sym; (void)type; (void)arguments; STUB_TRAP(); return NULL; }
void AddDefaultArgs(SYMBOL sym,PTREE call)
{ (void)sym; (void)call; STUB_TRAP(); }
PTREE CallArgumentExactCtor(TYPE type,unsigned exact)
{ (void)type; (void)exact; STUB_TRAP(); return NULL; }
PTREE NodeArg(PTREE expr);
PTREE CallArgsArrange(TYPE type,PTREE call,PTREE args,PTREE this_arg,PTREE exact_arg,PTREE extra)
{ (void)type; (void)call; (void)args; (void)this_arg; (void)exact_arg; (void)extra; STUB_TRAP(); return NULL; }
PTREE NodeDtorExpr(PTREE expr,SYMBOL sym)
{ (void)expr; (void)sym; STUB_TRAP(); return NULL; }
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
