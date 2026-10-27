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

/* Typed public truncation boundaries from initial analpcnv.c. The old
 * cast boundary maps ordinary truncation1 to cast-specific warning11. */
#include "types.h"
#include "ptree.h"
#include "symbol.h"
#include "pending.h"
#include <recon.h>
unsigned CgTypeTruncation(TYPE target,TYPE source);
unsigned NodeCheckPtrTrunc(TYPE target,TYPE source)
{
    return CgTypeTruncation(target,source);
}
unsigned NodeCheckPtrCastTrunc(TYPE target,TYPE source)
{
    unsigned result=CgTypeTruncation(target,source);
    return result==1 ? 11 : result;
}

TYPE TypePointedAt(TYPE type,unsigned short *flags);
int TypeIsBasedPtr(TYPE type)
{
    unsigned short flags;
    return TypePointedAt(type,&flags) ? flags&0x700 : 0;
}

void ConversionTypesSet(TYPE source,TYPE target);
void PTreeErrorExpr(PTREE expr,unsigned error);
unsigned emitConvMsg(PTREE expr,TYPE source,TYPE target,unsigned message)
{
    ConversionTypesSet(source,target);
    PTreeErrorExpr(expr,message);
    return 2;
}
unsigned NodeCheckCnvPtrVoid(PTREE *expr,TYPE source,TYPE target)
{
    TYPE pointed;
    unsigned short src_flags,tgt_flags;
    unsigned result,message;
    pointed=TypePointedAt(source,&src_flags);
    if(!pointed) return 0;
    if(pointed->id==TYP_FUNCTION) {
        result=CgTypeTruncation(target,source);
        if(result==1) result=emitConvMsg(*expr,source,target,470);
        return result;
    }
    if(CgTypeTruncation(target,source)==1)
        return emitConvMsg(*expr,source,target,471);
    TypePointedAt(target,&tgt_flags);
    src_flags&=3;
    tgt_flags&=src_flags&3;
    if(src_flags==tgt_flags) return 0;
    switch(tgt_flags) {
    case 0: message=468; break;
    case 1: message=564; break;
    case 2: message=469; break;
    default: message=565; break;
    }
    return emitConvMsg(*expr,source,target,message);
}

/* Actual1994 base-pointer conversion predates inline-bound parameter
 * decoration. Its virtual path preserves the original0x061b flag mask. */
TYPE NodeType(PTREE expr);
TYPE ReferenceType(TYPE type);
PTREE NodeConvert(TYPE type,PTREE expr);
PTREE NodeOffset(long offset);
PTREE NodeDupExpr(PTREE *expr);
PTREE NodeBinary(unsigned char op,PTREE left,PTREE right);
PTREE NodeCompareToZero(PTREE expr);
PTREE NodeTestExpr(PTREE condition,PTREE yes,PTREE no);
int NodePtrNonZero(PTREE expr);
PTREE NodeConvertVirtualPtr(PTREE expr,TYPE type,long offset,long index);
PTREE adjust_base_ptr(PTREE expr,PTREE offset,TYPE type)
{
    expr=NodeBinary((expr->flags&0x200)?48:102,expr,offset);
    expr->type=type;
    return expr;
}
void adjust_by_delta(PTREE *expr,TYPE base,unsigned delta,int positive)
{
    PTREE offset;
    if (!delta) *expr=NodeConvert(base,*expr);
    else {
        offset=NodeOffset(delta);
        if (!positive) offset->u.int_constant=-(long)delta;
        *expr=adjust_base_ptr(*expr,offset,base);
    }
}
void NodeConvertToBasePtr(PTREE *expr,TYPE base,SEARCH_RESULT *result,int positive)
{
    PTREE node=*expr,dup=NULL,orig;
    unsigned flags;
    if (ReferenceType(NodeType(node))) node->flags|=0x200;
    if (node->flags&8) adjust_by_delta(expr,base,result->exact_delta,positive);
    else if (result->flags&2) adjust_by_delta(expr,base,result->delta,positive);
    else {
        flags=(node->flags&0x61b)|2;
        if (!NodePtrNonZero(node)) dup=NodeDupExpr(&node);
        node=NodeConvertVirtualPtr(node,base,result->vb_offset,result->vb_index);
        if (result->delta) {
            node=NodeBinary(48,node,NodeOffset(result->delta));
            node->type=base;
            node->flags|=flags;
        }
        if (dup) {
            orig=NodeDupExpr(&dup);
            node=NodeTestExpr(NodeCompareToZero(orig),node,dup);
        }
        *expr=node;
    }
}

TYPE MakeVBTableFieldType(int is_const);
PTREE NodeFetch(PTREE expr);
TYPE TypePointedAtModified(TYPE type);
unsigned CgMemorySize(TYPE type);
PTREE NodeConvertVirtualPtr(PTREE expr,TYPE final_type,long vb_offset,long vb_index)
{
    TYPE vbptr=MakeVBTableFieldType(1),adjust;
    PTREE offset,dup;
    offset=NodeOffset(vb_offset);
    expr=NodeBinary(48,expr,offset);
    expr->flags|=0x202;
    expr->type=vbptr;
    dup=NodeDupExpr(&expr);
    expr->flags|=0x202;
    expr=NodeFetch(expr);
    expr->type=vbptr;
    expr->flags&=~0x202;
    expr->flags|=0x200;
    adjust=TypePointedAtModified(vbptr);
    offset=NodeOffset(vb_index*CgMemorySize(adjust));
    expr=NodeBinary(48,expr,offset);
    expr->type=adjust;
    expr->flags|=0x202;
    expr=NodeFetch(expr);
    expr->type=adjust;
    expr->flags|=0x200;
    expr=NodeBinary(48,dup,expr);
    expr->type=final_type;
    expr->flags|=0x202;
    return expr;
}

int TypeCompareExclude(TYPE first,TYPE second,unsigned flags);
TYPE type_pointed_to(TYPE type)
{
    unsigned short flags;
    return TypePointedAt(type,&flags);
}
int same_ptr_types(TYPE source,TYPE target)
{
    TYPE pointed_target=type_pointed_to(target);
    return TypeCompareExclude(type_pointed_to(source),pointed_target,4);
}
SCOPE TypeScope(TYPE type);
SCOPE scope_for_ptr(TYPE type)
{
    return TypeScope(type_pointed_to(type));
}
int ptr_to_void(TYPE type)
{
    return type_pointed_to(type)->id==TYP_VOID;
}
unsigned char TypeCommonDerivation(TYPE source,TYPE target);
SEARCH_RESULT *ScopeBaseResult(SCOPE derived,SCOPE base);
void ScopeFreeResult(SEARCH_RESULT *result);
TYPE convert_base_ptr(PTREE *expr,TYPE target,SCOPE derived,SCOPE base,int positive)
{
    SEARCH_RESULT *result=ScopeBaseResult(derived,base);
    NodeConvertToBasePtr(expr,target,result,positive);
    ScopeFreeResult(result);
    return target;
}
TYPE NodeConvertDerivedToBase(PTREE *expr,TYPE target,SCOPE derived,SCOPE base)
{
    return convert_base_ptr(expr,target,derived,base,1);
}
TYPE NodeConvertBaseToDerived(PTREE *expr,TYPE target,SCOPE derived,SCOPE base)
{ (void)expr; (void)target; (void)derived; (void)base; STUB_TRAP(); return NULL; }
TYPE NodeSetReference(TYPE type,PTREE expr);
unsigned check_result(PTREE *expr,TYPE source,TYPE target)
{
    PTREE node=*expr;
    if(node->op==0) return 2;
    node->type=target;
    NodeSetReference(target,node);
    return CgTypeTruncation(target,source);
}
unsigned check_result_cv(PTREE *expr,TYPE source,TYPE target,unsigned conversion)
{
    unsigned result=(conversion&8)?0:check_result(expr,source,target);
    if(conversion&4) {
        if(!result) result=8;
        else if(result==1) result=(conversion&2)?10:9;
    }
    return result;
}
unsigned NodeConvertPtr(unsigned conversion,PTREE *expr,TYPE source,TYPE target)
{
    SCOPE src_scope,tgt_scope;
    if(same_ptr_types(source,target)) {
        (*expr)->type=source;
        return 0;
    }
    src_scope=scope_for_ptr(source);
    tgt_scope=scope_for_ptr(target);
    /* Old CTD ordering: no relationship, left, left virtual, left
     * ambiguous/private/protected, then the five corresponding right cases. */
    switch(TypeCommonDerivation(TypePointedAtModified(source),TypePointedAtModified(target))) {
    case 0:
        if(!ptr_to_void(target)) return 4;
        *expr=NodeConvert(target,*expr);
        break;
    case 10:
        if(!(conversion&2)) return (conversion&1)?7:4;
        /* fall through */
    case 9:
        if(!(conversion&2)) return (conversion&1)?6:4;
        /* fall through */
    case 6:
        if(!(conversion&1)) return 4;
        NodeConvertBaseToDerived(expr,target,tgt_scope,src_scope);
        break;
    case 7: return (conversion&1)?3:4;
    case 8: return (conversion&1)?5:4;
    case 4:
        if(!(conversion&2)) return 6;
        /* fall through */
    case 5:
        if(!(conversion&2)) return 7;
        /* fall through */
    case 1: case 2:
        /* Both old virtual/nonvirtual entries share the actual helper. */
        NodeConvertDerivedToBase(expr,target,src_scope,tgt_scope);
        break;
    case 3: return 5;
    default: STUB_TRAP(); return 0;
    }
    return check_result_cv(expr,source,target,conversion);
}

int NodeIsBinaryOp(PTREE node,unsigned char operation);
PTREE PTreeOpLeft(PTREE expr);
PTREE PTreeOpRight(PTREE expr);
unsigned propogateNonZero(unsigned result,PTREE expr)
{
    if(NodeIsBinaryOp(expr,43) &&
       (PTreeOpRight(expr)->flags&0x200) &&
       (PTreeOpLeft(expr)->flags&0x200)) expr->flags|=0x200;
    return result;
}
unsigned check_common(unsigned result,PTREE expr,TYPE type)
{
    if(result==0 || result==1) {
        expr->type=type;
        result=propogateNonZero(result,expr);
    }
    return result;
}
TYPE TypeCommonBase(TYPE first,TYPE second);
TYPE TypePointedAtReplace(TYPE type,TYPE pointed);
unsigned convertUserDefined(PTREE *expr,TYPE source,TYPE target);
unsigned CgTypeSize(TYPE type);
unsigned PtrConvertCommon(PTREE expr)
{
    PTREE *left=&expr->u.subtree[0],*right=&expr->u.subtree[1];
    TYPE ltype=NodeType(*left),rtype=NodeType(*right);
    TYPE lbtype,rbtype,common,lptype,rptype;
    unsigned result;
    if(same_ptr_types(ltype,rtype)) {
        expr->type=CgTypeSize(ltype)>CgTypeSize(rtype)?ltype:rtype;
        return propogateNonZero(0,expr);
    }
    if(ptr_to_void(ltype)) {
        result=NodeCheckCnvPtrVoid(&expr,rtype,ltype);
        return check_common(result,expr,ltype);
    }
    if(ptr_to_void(rtype)) {
        result=NodeCheckCnvPtrVoid(&expr,ltype,rtype);
        return check_common(result,expr,rtype);
    }
    lbtype=TypePointedAtModified(ltype);
    rbtype=TypePointedAtModified(rtype);
    switch(TypeCommonDerivation(lbtype,rbtype)) {
    case 1: case 2:
        result=NodeConvertPtr(0,left,ltype,rtype);
        return check_common(result,expr,rtype);
    case 6: case 7:
        result=NodeConvertPtr(0,right,rtype,ltype);
        return check_common(result,expr,ltype);
    case 0:
        common=TypeCommonBase(rbtype,lbtype);
        if(common) {
            lptype=TypePointedAtReplace(ltype,common);
            rptype=TypePointedAtReplace(rtype,common);
            NodeConvertPtr(0,left,ltype,lptype);
            NodeConvertPtr(0,right,rtype,rptype);
            expr->type=expr->u.subtree[0]->type;
            return propogateNonZero(0,expr);
        }
        result=convertUserDefined(left,ltype,rtype);
        if(result==4) {
            result=convertUserDefined(right,rtype,ltype);
            return check_common(result,expr,ltype);
        }
        return check_common(result,expr,rtype);
    case 8: ConversionTypesSet(rtype,ltype); /* fall through */
    case 3: return 5;
    case 9: ConversionTypesSet(rtype,ltype); /* fall through */
    case 4: return 6;
    case 10: ConversionTypesSet(rtype,ltype); /* fall through */
    case 5: return 7;
    }
    STUB_TRAP(); return 0;
}
