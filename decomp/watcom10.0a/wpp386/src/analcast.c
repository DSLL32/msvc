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

/* The actual1994 implicit conversion is a single large routine. The later
 * analcast.c CONVCTL/dispatch implementation is a semantic reference only;
 * this discovery prefix remains unaccepted until its body is reconstructed. */
#include "convdiag.h"
#include "symbol.h"
#include "pending.h"
#include <recon.h>
int TypesIdentical(TYPE first,TYPE second);
TYPE NodeType(PTREE expr);
TYPE ClassTypeForType(TYPE type);
int ClassCorrupted(TYPE type);
TYPE StructType(TYPE type);
int TypeDefined(TYPE type);
int NodeIsUnaryOp(PTREE expr,unsigned char operation);
void PTreeErrorExpr(PTREE expr,unsigned error);
static TYPE cnv_type_src,cnv_type_tgt;
unsigned checkConversionClassDefined(TYPE type,PTREE expr,unsigned error)
{
    TYPE class_type=ClassTypeForType(type);
    if(class_type && !TypeDefined(class_type)) {
        PTreeErrorExpr(expr,error);
        return 2;
    }
    return 0;
}
TYPE MemberPtrType(TYPE type);
TYPE ReferenceType(TYPE type);
TYPE TypedefModifierRemove(TYPE type);
TYPE ArrayType(TYPE type);
TYPE FunctionDeclarationType(TYPE type);
TYPE PointerTypeForArray(TYPE type);
int TypeIsConst(TYPE type);
PTREE NodeConvert(TYPE type,PTREE expr);
PTREE NodeRvalue(PTREE expr);
PTREE convertRvalueOperand(PTREE expr)
{
    TYPE type=expr->type;
    TYPE base;
    expr=NodeRvalue(expr);
    base=TypedefModifierRemove(type);
    if(base->id>=TYP_CHAR && base->id<=TYP_ENUM) expr->type=type;
    return expr;
}
TYPE MakePointerTo(TYPE type);
unsigned convertReferenceOperand(TYPE type,PTREE *expr,unsigned control)
{ (void)type; (void)expr; (void)control; STUB_TRAP(); return 0; }
void NodeWarnPtrTrunc(PTREE expr)
{ (void)expr; STUB_TRAP(); }
PTREE PTreeOp(PTREE *expr);
int NodeFunctionOperand(PTREE expr,PTREE *function);
PTREE PTreeErrorNode(PTREE expr);
PTREE NodeUniqueFunction(PTREE expr)
{ (void)expr; STUB_TRAP(); return NULL; }
TYPE makeImplicitFunctionType(TYPE function,TYPE target)
{ (void)function; (void)target; STUB_TRAP(); return NULL; }
unsigned markImplicitFunctionAddress(PTREE function,PTREE expr)
{ (void)function; (void)expr; STUB_TRAP(); return 0; }
int convertOverloadedFunction(TYPE type,PTREE function)
{ (void)type; (void)function; STUB_TRAP(); return 0; }
TYPE PointerTypeEquivalent(TYPE type);
TYPE TypeModFlagsEC(TYPE type,unsigned short *flags);
unsigned getPointedCV(TYPE type)
{
    TYPE indirect=PointerTypeEquivalent(type);
    unsigned short flags;
    if(!indirect) indirect=MemberPtrType(type);
    if(!indirect) return 0;
    TypeModFlagsEC(indirect->of,&flags);
    return flags&3;
}
unsigned checkImplicitTypes(PTREE expr,TYPE target,TYPE source,unsigned control)
{
    unsigned tgt_flags,src_flags,error;
    TYPE ref;
    if(!ReferenceType(target)) {
        ref=ReferenceType(source);
        if(ref) source=ref;
    }
    tgt_flags=getPointedCV(target);
    src_flags=getPointedCV(source);
    error=0;
    if((src_flags&1)&~tgt_flags) {
        switch(control) {
        case 1: error=353; break;
        case 2: case 0x44: error=406; break;
        case 4: error=405; break;
        case 0x42: error=419; break;
        }
        if(error) { PTreeErrorExpr(expr,error); return 2; }
    }
    if((src_flags&2)&~tgt_flags) {
        switch(control) {
        case 1: error=566; break;
        case 2: case 0x44: error=567; break;
        case 4: error=568; break;
        case 0x42: error=467; break;
        }
        if(error) { PTreeErrorExpr(expr,error); return 2; }
    }
    return 0;
}
/* Valid old conversion results are0..12. The original fallthrough leaves
 * EDX as its result for invalid codes; those have no defined enum meaning. */
int conversionWorked(unsigned result)
{
    if(result>12) { STUB_TRAP(); return 0; }
    return result<2 || result>7;
}
int NodeNonConstRefToTemp(TYPE type,PTREE node);
unsigned checkReferenceTemporary(TYPE type,PTREE node,unsigned result)
{
    if(NodeNonConstRefToTemp(type,node)) result=2;
    return result;
}
SYMBOL SymFunctionReturn(void);
PTREE MakeNodeSymbol(SYMBOL sym);
PTREE NodeAssign(PTREE target,PTREE source);
int NodeIsBinaryOp(PTREE node,unsigned char operation);

/* Normalize unique or overloaded function operands before the old general
 * conversion. Member-pointer values retain their separate selected path. */
unsigned convertImplicitMemberPointer(PTREE *expr,TYPE type,TYPE *source,unsigned control)
{
    PTREE node=*expr,operand,function;
    TYPE result_type;
    unsigned result,mode;
    int kind;
    if(MemberPtrType(type)) goto unchanged;
    if(MemberPtrType(node->type)) {
        operand=PTreeOp(expr);
        kind=NodeFunctionOperand(operand,&function);
        if(kind==1) goto unique;
        if(kind!=2) goto unchanged;
        mode=control&0x2f;
        switch(mode) {
        case 1: case 2: case 4: case 8: case 32: break;
        default: goto bad_operand;
        }
        if(convertOverloadedFunction(type,function)) goto no_match;
        goto unique;
    }
    operand=PTreeOp(expr);
    kind=NodeFunctionOperand(operand,&function);
    switch(kind) {
    case 1: case 3:
        function=NodeUniqueFunction(function);
        goto unique;
    case 2: case 4:
        mode=control&0x2f;
        switch(mode) {
        case 1: case 2: case 4: case 8: case 32: break;
        default: goto bad_operand;
        }
        if(convertOverloadedFunction(type,function)) goto no_match;
        goto unique;
    default: goto unchanged;
    }
unique:
    result_type=makeImplicitFunctionType(function->type,type);
    operand->type=result_type;
    node->type=result_type;
    result=markImplicitFunctionAddress(function,node);
    *source=result_type;
    return result;
no_match:
    *source=NULL;
    return 5;
bad_operand:
    PTreeErrorExpr(function,486);
    PTreeErrorNode(node);
    *source=NULL;
    return 2;
unchanged:
    *source=NodeType(node);
    return 0;
}
/* Older rvalue/reference preparation predates the released CONVCTL helpers.
 * Preserve the actual node updates and recursive const-reference binding. */
PTREE convertToRvalue(PTREE node)
{
    TYPE ref=ReferenceType(NodeType(node));
    if(ref && !ClassTypeForType(node->type) &&
       !FunctionDeclarationType(node->type) && !MemberPtrType(node->type)) {
        if(FunctionDeclarationType(ref)) {
            if(node->op!=7) node=NodeConvert(ref,node);
            node->flags&=~2;
            return node;
        }
        return convertRvalueOperand(node);
    }
    if(ArrayType(node->type)) node=convertRvalueOperand(node);
    return node;
}
unsigned prepareImplicitOperand(PTREE *expr,TYPE type,unsigned control)
{
    PTREE node=*expr;
    TYPE ref=ReferenceType(type),source,unmod,function;
    unsigned result;
    if(!ref) {
        *expr=convertToRvalue(node);
        return 0;
    }
    if(node->flags&2) {
        source=NodeType(node);
        unmod=TypedefModifierRemove(source);
        if(ReferenceType(unmod)) return 0;
        if(unmod->id==TYP_POINTER) {
            function=TypedefModifierRemove(unmod->of);
            if(function->id==TYP_FUNCTION)
                return convertReferenceOperand(MakePointerTo(function),expr,control);
        } else if(unmod->id==TYP_ARRAY)
            return convertReferenceOperand(PointerTypeForArray(source),expr,control);
        return 0;
    }
    if(ClassTypeForType(type) || !TypeIsConst(ref)) return 0;
    result=CastImplicit(expr,ref,16);
    if(result==1) { NodeWarnPtrTrunc(*expr); result=0; }
    if(!result) return convertReferenceOperand((*expr)->type,expr,control);
    return result;
}
TYPE TypePointedAt(TYPE type,unsigned short *flags);
unsigned char TypePcPtr(unsigned short flags);
TYPE PointerTypeEquivalent(TYPE type);
unsigned PcPtrValidate(TYPE target,TYPE source,unsigned char target_class,
    unsigned char source_class,PTREE expr,unsigned conversion);
int TypeCompareExclude(TYPE source,TYPE target,unsigned char mask);
int NodeIsZeroConstant(PTREE expr);
int NodeIsConstantInt(PTREE expr);
unsigned char TypeDerived(TYPE derived,TYPE possible_base);
unsigned convertUserDefined(PTREE *expr,TYPE source,TYPE target);
unsigned NodeCheckPtrCastTrunc(TYPE target,TYPE source);
TYPE MakeReferenceTo(TYPE type);
/* Descriptive name for the old1578c helper: preserve a reference type or
 * create one. The later CONVCTL source has a target-specific equivalent. */
TYPE referenceTypeForConversion(TYPE type)
{
    return ReferenceType(type)?type:MakeReferenceTo(type);
}
TYPE conversionTypeModFlags(TYPE type,unsigned short *flags)
{
    if(type) return TypeModFlagsEC(type,flags);
    *flags=0;
    return NULL;
}
unsigned checkReferenceCV(PTREE node,unsigned short target_flags,
                          unsigned short source_flags,unsigned control)
{
    unsigned error;
    if(control==0x82 || control==8) return 0;
    /* Preserve the original cross-qualifier tests in15e41, including their
     * old error selection, rather than substituting modern cv rules. */
    if(target_flags&1) {
        if(!(source_flags&2)) return 0;
        error=496;
    } else if(target_flags&2) {
        if(!(source_flags&1)) return 0;
        error=495;
    } else {
        if(!(source_flags&3)) return 0;
        error=497;
    }
    PTreeErrorExpr(node,error);
    return 2;
}
PTREE markConversionAddress(PTREE node)
{
    if(node->op==7) node->u.symcg.symbol->flag|=8;
    return node;
}
unsigned NodeCheckPtrTrunc(TYPE target,TYPE source);
unsigned checkReferenceResult(unsigned result,TYPE target,PTREE *expr);
unsigned checkClassPtrTrunc(TYPE target,TYPE source,unsigned control)
{
    unsigned result;
    if(control==8) result=NodeCheckPtrCastTrunc(target,source);
    else result=NodeCheckPtrTrunc(target,source);
    if(result && (control==0x42 || control==0x82)) result=12;
    return result;
}
PTREE PTreeOpLeft(PTREE expr);
PTREE PTreeOpRight(PTREE expr);
PTREE NodeRemoveCasts(PTREE expr);
SYMBOL ScopeFunctionInProgress(void);
int SymIsGenedFunc(SYMBOL sym);
int SymIsAutomatic(SYMBOL sym);
int SymIsCtor(SYMBOL sym);
TYPE SymFuncReturnType(SYMBOL sym);
PTREE NodeFuncForCall(PTREE expr);
TYPE TypeFunctionCalled(TYPE type);
void PTreeWarnExpr(PTREE expr,unsigned warning);
unsigned checkReturnReference(PTREE value,PTREE expr,TYPE reference)
{
    SYMBOL sym;
    if(SymIsGenedFunc(ScopeFunctionInProgress())) return 0;
again:
    for(;;) {
        value=NodeRemoveCasts(value);
        if(value->op==7 && SymIsAutomatic(value->u.symcg.symbol)) {
            if(reference) goto temporary;
            PTreeWarnExpr(expr,8);
            return 0;
        }
        if(NodeIsBinaryOp(value,48) || NodeIsBinaryOp(value,45)) {
            value=PTreeOpLeft(value);
            continue;
        }
        break;
    }
    if(!reference) return 0;
    if(NodeIsBinaryOp(value,99)) {
        value=PTreeOpRight(value);
        /* The old helper peels the destructor wrapper back through its
         * ordinary cast/member loop before examining the underlying call. */
        goto again;
    }
    if(NodeIsBinaryOp(value,95)) {
        value=PTreeOpLeft(PTreeOpLeft(value));
        if(value->op==7) {
            sym=value->u.symcg.symbol;
            if(SymIsCtor(sym) || StructType(SymFuncReturnType(sym)))
                goto temporary;
        }
    } else if(NodeIsBinaryOp(value,97)) {
        TYPE called=TypeFunctionCalled(NodeFuncForCall(value)->type);
        if(StructType(called->of)) goto temporary;
    }
    return 0;
temporary:
    PTreeErrorExpr(expr,210);
    return 2;
}
unsigned NodeConvertPtr(unsigned conversion,PTREE *expr,TYPE source,TYPE target);
TYPE GetReferenceTo(TYPE type);
unsigned convertReferenceTypes(PTREE *expr,TYPE target,unsigned control)
{
    PTREE node=*expr;
    TYPE source,source_ref,target_ref,source_base,target_base;
    unsigned short source_flags,target_flags;
    unsigned result;
    if((node->flags&2) && ArrayType(node->type))
        source=MakeReferenceTo(node->type);
    else source=NodeType(node);
    source_ref=ReferenceType(source);
    target_ref=ReferenceType(target);
    if(ArrayType(target_ref)) { STUB_TRAP(); return 0; }
    if(!source_ref && !StructType(node->type)) { STUB_TRAP(); return 0; }
    source_base=conversionTypeModFlags(source_ref,&source_flags);
    target_base=conversionTypeModFlags(target_ref,&target_flags);
    if(!TypesIdentical(source_base,target_base)) {
        unsigned char derived=TypeDerived(source_ref,target_ref);
        if(derived>=3 && derived<=5) {
            result=derived+2;
        } else if(derived==1 || derived==2) {
            result=checkReferenceCV(node,target_flags,source_flags,control);
            if(!result) {
                checkImplicitTypes(node,target,source,control);
                result=NodeConvertPtr(0,expr,source,target);
                node=*expr;
                if(result==0 || result==1) {
                    node->type=GetReferenceTo(target);
                    node->flags|=2;
                }
            }
        } else { STUB_TRAP(); return 0; }
        *expr=node;
        return result;
    }
    result=checkReferenceCV(node,target_flags,source_flags,control);
    if(!result) {
        if(NodeIsUnaryOp(node,101)) { STUB_TRAP(); return 0; }
        node=NodeConvert(target,markConversionAddress(node));
        result=NodeCheckPtrTrunc(target,source);
    }
    *expr=node;
    return result;
}
unsigned NodeCheckCnvPtrVoid(PTREE *expr,TYPE source,TYPE target);
PTREE nodeBasedSelfExpr(PTREE expr)
{ (void)expr; STUB_TRAP(); return NULL; }
TYPE TypePointedAtModified(TYPE type);
TYPE TypeModExtract(TYPE type,unsigned short *flags,void **base,unsigned char mask);
TYPE MakeBasedModifierOf(TYPE type,unsigned short flags,void *base);
TYPE MakePointerTo(TYPE type);
unsigned classPtrConversion(PTREE *expr,TYPE target,unsigned control)
{
    TYPE pointed,new_target,source;
    unsigned short flags;
    void *base;
    unsigned mode;
    pointed=TypePointedAt(target,&flags);
    source=(*expr)->type;
    TypeModExtract(TypePointedAtModified(source),&flags,&base,1);
    new_target=MakePointerTo(MakeBasedModifierOf(pointed,flags,base));
    switch(control&0x2f) {
    case 8: mode=3; break;
    case 1: case 2: case 4: mode=4; break;
    default: mode=0; break;
    }
    return NodeConvertPtr(mode,expr,source,new_target);
}
PTREE convertToPcPtr(PTREE expr,TYPE target,unsigned char kind)
{ (void)expr; (void)target; (void)kind; STUB_TRAP(); return NULL; }
PTREE convertFromPcPtr(PTREE expr,unsigned char kind,PTREE based_self)
{ (void)expr; (void)kind; (void)based_self; STUB_TRAP(); return NULL; }
TYPE TypePointedAtModified(TYPE type);
TYPE TypeModExtract(TYPE type,unsigned short *flags,void **baser,unsigned char mask);
TYPE MakeBasedModifierOf(TYPE type,unsigned short flags,void *base);
TYPE MakePointerTo(TYPE type);
void CFatal(const char *message);
/* Released convctl.c matrix, byte-identical to the encountered1994 table.
 * Columns/rows: regular, string,self,void,fetch,add,far16,not-a-pointer.
 * Actions1..9 select the corresponding conversion arms;10 is no change,
 *99 denotes the impossible based-void to far16 conversion. */
static const unsigned char pcPtrCnv[8][8]={
    {1,2,2,2,2,8,3,10}, {4,1,1,1,1,5,5,10},
    {4,1,1,1,1,5,5,10}, {2,1,1,1,1,5,99,10},
    {4,1,1,1,1,5,5,10}, {4,9,9,9,9,7,5,10},
    {4,6,6,6,6,5,5,10}, {10,10,10,10,10,10,10,10}
};
unsigned pcPtrConvertSrcTgt(PTREE *a_expr,TYPE target,unsigned char target_class,
    unsigned char source_class,int class_conversion,unsigned control)
{
    PTREE expr,based_self;
    TYPE pointed;
    unsigned char action;
    unsigned short target_flags,source_flags;
    void *baser;
    unsigned result;
    if(source_class>=8 || target_class>=8) { STUB_TRAP(); return 0; }
    action=pcPtrCnv[source_class][target_class];
    if(action==10) return 0;
    based_self=NULL;
    if(source_class==2 && (action==4 || action==5))
        based_self=nodeBasedSelfExpr(*a_expr);
    if(class_conversion) {
        result=classPtrConversion(a_expr,target,control);
        expr=*a_expr;
    } else {
        expr=*a_expr;
        result=checkClassPtrTrunc(target,expr->type,control);
        if(result==0) result=8;
        else if(result==1) result=9;
        else if(result==11) result=10;
    }
    if(result<2 || result>7) {
        pointed=TypePointedAtModified(target);
        TypeModExtract(pointed,&target_flags,&baser,8);
        pointed=TypePointedAt(expr->type,&source_flags);
        target_flags=(target_flags&0x73c)|(source_flags&~0x73c);
        target=MakePointerTo(MakeBasedModifierOf(pointed,target_flags,baser));
        switch(action) {
        default:
            CFatal("pcPtrConvert -- impossible conversion");
            /* The original fatal boundary is nonlocal; retain its
             * syntactic fallthrough without assuming it ever returns. */
        case 1:
            if(class_conversion) break;
            /* fall through */
        case 2: expr=NodeConvert(target,expr); break;
        case 3: case 8: expr=convertToPcPtr(expr,target,target_class); break;
        case 4: expr=convertFromPcPtr(expr,source_class,based_self); break;
        case 5: case 7:
            expr=convertFromPcPtr(expr,source_class,based_self);
            expr=convertToPcPtr(expr,target,target_class);
            break;
        case 6: case 9:
            expr=NodeConvert(target,convertFromPcPtr(expr,source_class,based_self));
            break;
        }
    }
    *a_expr=expr;
    return result;
}
int TypeTruncByMemModel(TYPE type);
TYPE IntegralType(TYPE type);
void AnalyseIntTrunc(PTREE expr,TYPE type);
TYPE ArithType(TYPE type);
TYPE SegmentShortType(TYPE type);
PTREE markConversionAddress(PTREE expr);
unsigned CastImplicit(PTREE *expr,TYPE type,unsigned control)
{
    PTREE node=*expr;
    TYPE source=NodeType(node),ref,class_type;
    unsigned result,mode;
    SYMBOL retn;
    cnv_type_src=source;
    cnv_type_tgt=type;
    if((node->op==7 && StructType(node->type)) ||
       (NodeIsUnaryOp(node,111) && StructType(node->u.subtree[0]->type))) {
        if(checkConversionClassDefined(source,node,435)) return 2;
    }
    result=prepareImplicitOperand(expr,type,control);
    if(result) return result;
    ref=ReferenceType(type);
    class_type=ClassTypeForType(type);
    source=NodeType(*expr);
    cnv_type_src=source;
    result=convertImplicitMemberPointer(expr,type,&source,control);
    if(result) { STUB_TRAP(); return result; }
    node=*expr;
    if(MemberPtrType(type)) { STUB_TRAP(); return 0; }
    if(class_type && ((control&0x2f)==2 || (control&0x2f)==4)) {
        if(ref) {
            TYPE source_ref=ReferenceType(source);
            TYPE source_class=ClassTypeForType(source);
            (void)source_ref;
            if(ClassCorrupted(class_type) ||
               (source_class && ClassCorrupted(source_class))) {
                STUB_TRAP(); return 2;
            }
            if(source_class!=class_type) {
                if(control!=8 && checkConversionClassDefined(class_type,node,434)) {
                    STUB_TRAP(); return 2;
                }
                if(!source_class || !TypeDerived(source_class,class_type)) {
                    STUB_TRAP(); return 0;
                }
            }
            if(!source_ref) { STUB_TRAP(); return 0; }
            {
                TYPE original_source=source;
                TYPE target_reference;
                source=referenceTypeForConversion(source);
                if(source!=original_source) node=NodeConvert(source,node);
                target_reference=referenceTypeForConversion(type);
                result=checkImplicitTypes(node,type,original_source,control);
                if(result) { STUB_TRAP(); return result; }
                *expr=node;
                result=convertReferenceTypes(expr,target_reference,8);
                node=*expr;
                if(result) { STUB_TRAP(); return result; }
                result=checkClassPtrTrunc(target_reference,source,control);
                *expr=node;
                result=checkReferenceResult(result,type,expr);
                node=*expr;
                goto finalize;
            }
        }
        STUB_TRAP(); return 0;
    }
    if(TypesIdentical(TypedefModifierRemove(type),TypedefModifierRemove(source))) {
        if(!ref && (node->flags&2)) {
            node=NodeConvert(type,node);
            result=checkImplicitTypes(node,type,source,control);
            goto finalize;
        }
        if(NodeIsUnaryOp(node,101)) { STUB_TRAP(); return 0; }
        /* Actual16596..165a9 marks the address of an identical reference
         * operand before the common qualification/final-result checks. */
        if(ref) node=markConversionAddress(node);
        result=checkImplicitTypes(node,type,source,control);
        goto finalize;
    }
    if(ArithType(type)) {
        int segment=SegmentShortType(type)!=NULL;
        if(ArithType(source)) {
            if(control!=8 && TypeTruncByMemModel(type) &&
               !TypeTruncByMemModel(source)) {
                PTreeErrorExpr(node,139); result=2;
                goto finalize;
            }
            if(segment && !IntegralType(source)) { result=4; goto finalize; }
            if(control!=8) AnalyseIntTrunc(node,type);
            node=NodeConvert(type,node);
            result=0;
            goto finalize;
        }
        STUB_TRAP(); return 0;
    }
    /* The remaining conversion dispatcher is incomplete. Retain its real
     * pointed-type and memory-model classification prefix for pointer casts. */
    {
        TYPE target_ptr=type,source_ptr=source,pointed;
        unsigned short pointed_flags;
        unsigned char pointer_class;
        while(target_ptr->id==TYP_TYPEDEF || target_ptr->id==TYP_MODIFIER)
            target_ptr=target_ptr->of;
        while(source_ptr->id==TYP_TYPEDEF || source_ptr->id==TYP_MODIFIER)
            source_ptr=source_ptr->of;
        if(target_ptr->id==TYP_POINTER && source_ptr->id==TYP_POINTER) {
            pointed=TypePointedAt(type,&pointed_flags);
            pointer_class=TypePcPtr(pointed_flags);
            {
                TYPE source_pointed=TypePointedAt(source,&pointed_flags);
                unsigned char source_class=source_pointed ? TypePcPtr(pointed_flags) : 7;
                TYPE source_equivalent=PointerTypeEquivalent(source);
                result=PcPtrValidate(type,source,pointer_class,source_class,node,control);
                if(!result && source_equivalent && !(source_equivalent->flag&1)) {
                    if(TypeCompareExclude(source_pointed,pointed,5)) { STUB_TRAP(); return 0; }
                    if(NodeIsZeroConstant(node)) { STUB_TRAP(); return 0; }
                    if(NodeIsConstantInt(node)) { STUB_TRAP(); return 0; }
                    if(pointed->id==TYP_VOID) {
                        result=0;
                        if(control!=8) result=NodeCheckCnvPtrVoid(expr,source,type);
                        node=*expr;
                        if(result==2) goto finalize;
                        result=pcPtrConvertSrcTgt(expr,type,pointer_class,source_class,0,control);
                        node=*expr;
                        /* Actual168ca..1694f consumes the internal CV
                         * result before exposing the public conversion code. */
                        if(result>=8 && result<=10) {
                            unsigned cv=checkImplicitTypes(node,type,source,control);
                            if(cv==2) result=2;
                            else if(result==8) result=0;
                            else if(result==10) result=11;
                            else result=1;
                        }
                        goto finalize;
                    } else {
                        unsigned char derived=TypeDerived(source_pointed,pointed);
                        if(!derived && control==8)
                            derived=TypeDerived(pointed,source_pointed);
                        if(derived) {
                            result=pcPtrConvertSrcTgt(expr,type,pointer_class,
                                                     source_class,1,control);
                            node=*expr;
                            if(result>=8 && result<=10) {
                                unsigned cv=checkImplicitTypes(node,type,source,control);
                                if(cv==2) result=2;
                                else if(result==8) result=0;
                                else if(result==10) result=11;
                                else result=1;
                            }
                            goto finalize;
                        }
                        if(!derived) {
                            result=convertUserDefined(expr,source,type);
                            node=*expr;
                            if(result==4 && control==8)
                                result=NodeCheckPtrCastTrunc(type,source);
                            if(conversionWorked(result)) goto finalize;
                        }
                    }
                    STUB_TRAP(); return 0;
                }
                (void)source_equivalent;
            }
            (void)pointed;
        }
    }
    STUB_TRAP(); return 0;
finalize:
    mode=control&0x2f;
    if(conversionWorked(result) && (mode==1 || mode==2 || mode==4))
        result=checkReferenceTemporary(type,node,result);
    if(conversionWorked(result)) {
        if(mode==4 && (!class_type || ref)) {
            retn=SymFunctionReturn();
            retn->flag|=1;
            node=NodeAssign(MakeNodeSymbol(retn),node);
            if(MemberPtrType(type)) { STUB_TRAP(); return result; }
        } else if(mode==2 && MemberPtrType(type)) {
            STUB_TRAP(); return result;
        }
    }
    if(conversionWorked(result) && control==4 && NodeIsBinaryOp(node,13)) {
        PTREE value=NodeRemoveCasts(PTreeOpRight(node));
        if(ref) result=checkReturnReference(value,node,ref);
        else if(PointerTypeEquivalent(type) && NodeIsUnaryOp(value,12)) {
            value=NodeRemoveCasts(PTreeOpRight(value));
            result=checkReturnReference(value,node,ref);
        }
    }
    if(conversionWorked(result) && !TypesIdentical(NodeType(node),type))
        node=NodeConvert(type,node);
    *expr=node;
    return result;
}
void NodeWarnPtrTruncCast(PTREE expr)
{ (void)expr; STUB_TRAP(); }
void ConversionDiagnoseInf(void)
{ STUB_TRAP(); }
void ConversionInfDisable(void)
{
    cnv_type_src=NULL;
}
unsigned ConversionDiagnose(unsigned result,PTREE expr,CNV_DIAG *diagnosis)
{
    unsigned error;
    switch(result) {
    case 1: NodeWarnPtrTrunc(expr); return 0;
    case 11: NodeWarnPtrTruncCast(expr); return 0;
    case 2:
        PTreeErrorNode(expr);
        ConversionDiagnoseInf();
        return 2;
    case 3: error=diagnosis->msg_virt_der; break;
    case 4: error=diagnosis->msg_impossible; break;
    case 5: error=diagnosis->msg_ambiguous; break;
    case 6: error=diagnosis->msg_private; break;
    case 7: error=diagnosis->msg_protected; break;
    case 12:
        ConversionInfDisable();
        error=161;
        break;
    default: return result;
    }
    PTreeErrorExpr(expr,error);
    ConversionDiagnoseInf();
    return 2;
}

void ConversionTypesSet(TYPE first,TYPE second)
{ (void)first; (void)second; STUB_TRAP(); }

unsigned char TypePcPtr(unsigned short flags)
{
    return flags&16 ? 6 : (flags&0x700)>>8;
}


/* The reached old three-argument UDC boundary predates CONVCTL. Its body
 * and the unavailable1994 source-level spelling remain under reconstruction. */
unsigned short BaseTypeClassFlags(TYPE type);
unsigned char UdcLookup(TYPE source,unsigned short flags,TYPE *target,SYMBOL *symbol);
PTREE NodeAssignTemporary(TYPE type,PTREE expr);
unsigned NodeCheckPtrTrunc(TYPE target,TYPE source);
unsigned checkReferenceResult(unsigned result,TYPE target,PTREE *expr)
{
    PTREE node;
    TYPE target_ref,source_ref;
    if(conversionWorked(result)) {
        target_ref=ReferenceType(target);
        node=*expr;
        source_ref=ReferenceType(NodeType(node));
        if(!source_ref) {
            if(target_ref) {
                node=NodeAssignTemporary(target_ref,node);
                if(node->op==0) result=2;
                else node->flags|=0x20;
            }
        } else if(!target_ref) node=NodeRvalue(node);
        if(conversionWorked(result)) {
            if(target!=NodeType(node)) node=NodeConvert(target,node);
            if(target_ref && result==0)
                result=NodeCheckPtrTrunc(target,NodeType(node));
        }
        *expr=node;
    }
    return result;
}
unsigned convertUserDefined(PTREE *expr,TYPE source,TYPE target)
{
    TYPE target_class=ClassTypeForType(target),original_target=target,source_class;
    SYMBOL constructor=NULL,conversion;
    PTREE tree=*expr;
    unsigned result;
    unsigned short flags;
    unsigned char lookup;
    if(target_class) { STUB_TRAP(); return 0; }
    flags=BaseTypeClassFlags(source);
    source_class=ClassTypeForType(source);
    lookup=UdcLookup(source_class,flags,&target,&conversion);
    if(lookup==1) {
        if(!constructor) result=5;
        else { STUB_TRAP(); return 0; }
    } else if(lookup==2) {
        if(!constructor) result=4;
        else { STUB_TRAP(); return 0; }
    } else { STUB_TRAP(); return 0; }
    *expr=tree;
    return checkReferenceResult(result,original_target,expr);
}
