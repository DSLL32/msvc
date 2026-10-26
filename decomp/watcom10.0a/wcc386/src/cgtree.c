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
* Description:  Code-generator tree construction and generation.
*
****************************************************************************/


#include "codegen.h"
#include "procdef.h"
#include "cgtree.h"
#include "genquads.h"
#include <recon.h>

/* tree.c's subtraction matrix before 64-bit integer/long-double classes. */
static unsigned char SubMat[XX][XX]={
/* U1 I1 U2 I2 U4 I4 CP PT FS FD */
 {U1,U1,U2,I2,U4,I4,CP,PT,FS,FD},
 {U1,I1,U2,I2,U4,I4,CP,PT,FS,FD},
 {U2,U2,U2,U2,U4,I4,CP,PT,FS,FD},
 {I2,I2,U2,I2,U4,I4,CP,PT,FS,FD},
 {U4,U4,U4,U4,U4,U4,CP,PT,FS,FD},
 {I4,I4,I4,I4,U4,I4,CP,PT,FS,FD},
 {CP,CP,CP,CP,CP,CP,XX,PT,ER,ER},
 {PT,PT,PT,PT,PT,PT,PT,XX,ER,ER},
 {FS,FS,FS,FS,FS,FS,ER,ER,FS,FD},
 {FD,FD,FD,FD,FD,FD,ER,ER,FD,FD}
};
static void *TreeFrl;
static unsigned char IsAddress;

void TInit(void)
{
    IsAddress=0;
    SubMat[CP][CP]=TypeClass(TypeLongInteger);
    SubMat[PT][PT]=TypeClass(TypeHugeInteger);
    InitFrl(&TreeFrl);
}

void InitFrl(void **list)
{
    *list=NULL;
}

/* tree.c public control wrapper folds the two conditional forms separately. */
void TGControl(unsigned char op,struct tree_node *node,struct code_lbl *label)
{
    switch (op) {
    case 83: /* O_IF_TRUE */
        if (!FoldIfTrue(node,label)) Control(op,node,label,1);
        break;
    case 84: /* O_IF_FALSE */
        if (!FoldIfFalse(node,label)) Control(op,node,label,1);
        break;
    default:
        BGGenCtrl(op,NULL,label,1);
        break;
    }
}

/* tree.c NewTreeNode is inlined into TGLeaf in this binary. */
struct tree_node *TGLeaf(struct address_name *addr)
{
    tree_node *node;
    node=AllocFrl(&TreeFrl,sizeof(*node));
    node->base=NULL;
    node->op=0; /* O_NOP */
    node->rite=NULL;
    node->kids=0;
    node->flags=2; /* TF_USED */
    node->u.addr=addr;
    node->class=0; /* TN_LEAF */
    node->tipe=addr->tipe;
    if (addr->format==3) node->tipe=TypeBoolean;
    else if (addr->format==2) {
        node->class=18; /* TN_CONS */
        node->u.name=addr->u.name;
        BGDone(addr);
    }
    return node;
}

struct tree_node *TGUnary(unsigned char op,struct tree_node *left,type_def *type)
{
    tree_node *node;
    node=NULL;
    if (op!=O_POINTS && type==TypeNone) type=left->tipe;
    switch (op) {
    case O_UMINUS:
        left=TGConvert(left,type);
        node=FoldUMinus(left,type);
        break;
    case O_COMPLEMENT:
        left=TGConvert(left,type);
        node=Fold1sComp(left,type);
        break;
    case O_POINTS:
        if (left->class==16) { /* TN_BIT_LVALUE */
            node=left;
            node->class=17; /* TN_BIT_RVALUE */
            if (type->attr&2) ((bit_tree_node *)node)->is_signed=1;
        }
        break;
    case 37: /* O_ROUND */
        node=FoldCnvRnd(op,left,type);
        break;
    case 36: /* O_CONVERT */
        node=FoldCnvRnd(op,left,type);
        if (node==NULL) node=TGConvert(left,type);
        break;
    case O_SQRT:
        left=TGConvert(left,type);
        node=FoldSqrt(left,type);
        break;
    case O_LOG:case O_LOG10:
        left=TGConvert(left,type);
        node=FoldLog(op,left,type);
        break;
    case O_COS:case O_SIN:case O_TAN:case O_FABS:
    case O_ACOS:case O_ASIN:case O_ATAN:case O_COSH:
    case O_SINH:case O_TANH:case O_EXP:
        left=TGConvert(left,type);
        break;
    case 35: /* O_PARENTHESIS, an optimization-control no-op here. */
        node=left;
        break;
    case 33:case 34: /* O_PTR_TO_NATIVE / O_PTR_TO_FOREIGN */
        break;
    default:
        Zoiks(55);
        break;
    }
    if (node==NULL) node=TGNode(1,op,left,NULL,type); /* TN_UNARY */
    return node;
}

/* Old TGNode inlines allocation. Unlike the released later version, its
 * left child is required and TF_HAS_LEFT is not a separate flag. */
struct tree_node *TGNode(unsigned char class,unsigned char op,
    struct tree_node *left,struct tree_node *right,type_def *type)
{
    tree_node *node;
    node=AllocFrl(&TreeFrl,sizeof(*node));
    node->base=NULL;
    node->flags=2; /* TF_USED */
    node->u.left=left;
    node->rite=right;
    node->class=class;
    node->op=op;
    node->tipe=type;
    node->kids=left->kids+1;
    if (right!=NULL && class!=10) /* TN_PARM */
        node->kids+=right->kids+1;
    return node;
}

struct tree_node *TGBinary(unsigned char op,struct tree_node *left,
    struct tree_node *right,type_def *type)
{
    tree_node *node;
    type=BinResult(op,&left,&right,type,1);
    node=BinFold(op,left,right,type);
    if (node==NULL) node=TGNode(2,op,left,right,type); /* TN_BINARY */
    return node;
}

/* tree.c's pre-64-bit arithmetic promotion table. */
static unsigned char BinMat[XX][XX]={
 {U1,U1,U2,I2,U4,I4,CP,PT,FS,FD},
 {U1,I1,U2,I2,U4,I4,CP,PT,FS,FD},
 {U2,U2,U2,U2,U4,I4,CP,PT,FS,FD},
 {I2,I2,U2,I2,U4,I4,CP,PT,FS,FD},
 {U4,U4,U4,U4,U4,U4,CP,PT,FS,FD},
 {I4,I4,I4,I4,U4,I4,CP,PT,FS,FD},
 {CP,CP,CP,CP,CP,CP,CP,PT,ER,ER},
 {PT,PT,PT,PT,PT,PT,PT,PT,ER,ER},
 {FS,FS,FS,FS,FS,FS,ER,ER,FS,FD},
 {FD,FD,FD,FD,FD,FD,ER,ER,FD,FD}
};

type_def *BinResult(unsigned char op,struct tree_node **l,
    struct tree_node **r,type_def *type,unsigned char commute)
{
    tree_node *left,*right,*temp;
    type_def *old_type;
    (void)commute;
    left=*l;
    right=*r;
    switch (op) {
    case O_LSHIFT:case O_RSHIFT:
        left=TGConvert(left,type);
        type=left->tipe;
        right=TGConvert(right,TypeUnsigned);
        break;
    case O_DIV:case O_MOD:
        old_type=type;
        type=ResultType(left,right,type,&BinMat[0][0],0);
        if (old_type==TypeNone && (type->refno==T_UINT_1 || type->refno==T_INT_1))
            type=TypeInteger;
        left=TGConvert(left,type);
        right=TGConvert(right,type);
        break;
    case 36: /* O_CONVERT: compose a based pointer. */
        left=TGConvert(left,TypeAddress(T_NEAR_POINTER));
        if (!RHSLongPointer(right)) right=TGConvert(right,TypeAddress(T_UINT_2));
        switch (type->refno) {
        case T_DEFAULT:case T_NEAR_POINTER:
            type=TypeAddress(T_LONG_POINTER);
            break;
        case T_LONG_POINTER:case T_HUGE_POINTER:
            break;
        default:
            Zoiks(82);
            break;
        }
        break;
    case O_PLUS:
        if (left->class==11) { /* TN_CALL: return-value address. */
            type=TypePtr;
            if (type->refno==T_HUGE_POINTER) right=TGConvert(right,TypeHugeInteger);
            else if (type->refno==T_LONG_POINTER) right=TGConvert(right,TypeLongInteger);
            else right=TGConvert(right,TypeNearInteger);
        } else {
            type=ResultType(left,right,type,&BinMat[0][0],0);
            if (type->attr&4) {
                if (!(left->tipe->attr&4)) { temp=right;right=left;left=temp; }
                if (type->refno==T_HUGE_POINTER) right=TGConvert(right,TypeHugeInteger);
                else if (type->refno==T_LONG_POINTER) right=TGConvert(right,TypeLongInteger);
                else right=TGConvert(right,TypeNearInteger);
                if (left->class!=0 || NeedPtrConvert(left->u.addr,type))
                    left=TGConvert(left,type);
            } else {
                right=TGConvert(right,type);
                if (left->class!=0 || NeedPtrConvert(left->u.addr,type))
                    left=TGConvert(left,type);
            }
        }
        break;
    case O_MINUS:
        type=ResultType(left,right,type,&SubMat[0][0],0);
        if ((type->refno==TypeHugeInteger->refno &&
                left->tipe->refno==T_HUGE_POINTER && right->tipe->refno==T_HUGE_POINTER) ||
            (type->refno==TypeLongInteger->refno &&
                left->tipe->refno==T_LONG_POINTER && right->tipe->refno==T_LONG_POINTER) ||
            (type->refno==TypeNearInteger->refno &&
                left->tipe->refno==T_NEAR_POINTER && right->tipe->refno==T_NEAR_POINTER)) {
            /* A pointer difference retains the two pointer operands. */
        } else if (type->refno==T_HUGE_POINTER) right=TGConvert(right,TypeHugeInteger);
        else if (type->refno==T_LONG_POINTER) right=TGConvert(right,TypeLongInteger);
        else if (type->refno==T_NEAR_POINTER) right=TGConvert(right,TypeNearInteger);
        else right=TGConvert(right,type);
        left=TGConvert(left,type);
        break;
    case O_TIMES:case O_POW:case O_ATAN2:case O_FMOD:
        type=ResultType(left,right,type,&BinMat[0][0],0);
        left=TGConvert(left,type);
        right=TGConvert(right,type);
        break;
    case O_AND:case O_OR:case O_XOR:
        type=ResultType(left,right,type,&BinMat[0][0],1);
        left=TGConvert(left,type);
        right=TGConvert(right,type);
        break;
    case O_COMMA:case 98: /* Old O_SIDE_EFFECT uses the same ordering. */
        type=ResultType(left,right,type,&BinMat[0][0],1);
        break;
    default:
        Zoiks(54);
        break;
    }
    *l=left;
    *r=right;
    return type;
}

/* Old ResultType uses the ten-class matrix and a one-DWORD constant cache. */
type_def *ResultType(struct tree_node *left,struct tree_node *right,
    type_def *type,unsigned char *matrix,unsigned char demote_constant)
{
    tree_node *temp;
    type_def *left_type,*right_type;
    if (!demote_constant && type->refno!=T_DEFAULT) return type;
    if (left->class==18) { temp=left;left=right;right=temp; } /* TN_CONS */
    left_type=left->tipe;
    if (left_type==TypeBoolean) left_type=TypeInteger;
    right_type=right->tipe;
    if (right_type==TypeBoolean) right_type=TypeInteger;
    if (type->refno==T_DEFAULT)
        return ClassType(matrix[TypeClass(left_type)*XX+TypeClass(right_type)]);
    if (left->tipe->length>=type->length) return type;
    if (right->class!=18) return type;
    if (left->tipe->attr&1) return type;
    if (right->tipe->attr&1) return type;
    if (type->length>TypeClassSize[U4]) return type;
    if (left->tipe->attr&2) {
        if (!CFSignedSize(right->u.name->c.value,left->tipe->length)) return type;
        if (right->u.name->c.int_value<0) return type;
        if (!(type->attr&2)) return type;
    } else if (!CFUnSignedSize(right->u.name->c.value,left->tipe->length)) {
        return type;
    }
    return left->tipe;
}

struct tree_node *TGConvert(struct tree_node *value,type_def *type)
{
    tree_node *node;
    type_def *node_type;
    node=value;
    node_type=value->tipe;
    if (type->refno!=T_DEFAULT && type!=node_type) {
        if (type==TypeBoolean) {
            if (node_type!=TypeBoolean)
                node=TGCompare(O_NE,value,TGLeaf(BGInteger(0,TypeInteger)),node_type);
        } else if (node_type==TypeBoolean) {
            node=TGNode(13,O_NOP,value,NULL,type); /* TN_FLOW_OUT */
        } else {
            node=FoldCnvRnd(36,value,type); /* O_CONVERT */
            if (node==NULL) node=TGNode(1,36,value,NULL,type);
        }
    }
    return node;
}

struct tree_node *TGConst(struct cfloat *value,type_def *type)
{
    return TGLeaf(MakeConst(value,type));
}

/* FreeTreeNode is inlined at both sites in this old BurnTree body. */
void BurnTree(struct tree_node *node)
{
    switch (node->class) {
    case 0: /* TN_LEAF */
        BGDone(node->u.addr);
        break;
    case 18: /* TN_CONS: the constant name owns its decimal value. */
        break;
    case 16:case 17: /* TN_BIT_LVALUE / TN_BIT_RVALUE */
        if (node->u.left!=NULL) BurnTree(node->u.left);
        break;
    case 11: /* TN_CALL */
        BurnTree(node->u.left->u.left);
        FrlFreeSize(&TreeFrl,node->u.left,sizeof(*node));
        if (node->rite!=NULL) BurnTree(node->rite);
        break;
    default:
        if (node->rite!=NULL) BurnTree(node->rite);
        if (node->u.left!=NULL) BurnTree(node->u.left);
        break;
    }
    FrlFreeSize(&TreeFrl,node,sizeof(*node));
}

struct tree_node *BinFold(unsigned char op,struct tree_node *left,
    struct tree_node *right,type_def *type)
{
    switch (op) {
    case O_PLUS:return FoldPlus(left,right,type);
    case O_MINUS:return FoldMinus(left,right,type);
    case O_TIMES:return FoldTimes(left,right,type);
    case O_DIV:return FoldDiv(left,right,type);
    case O_MOD:return FoldMod(left,right,type);
    case O_LSHIFT:return FoldLShift(left,right,type);
    case O_RSHIFT:return FoldRShift(left,right,type);
    case O_AND:return FoldAnd(left,right,type);
    case O_OR:return FoldOr(left,right,type);
    case O_XOR:return FoldXor(left,right,type);
    case O_COMMA:return TGNode(12,O_NOP,left,right,right->tipe);
    case 98:return TGNode(24,O_NOP,left,right,right->tipe); /* Old side effect type. */
    case O_POW:return FoldPow(left,right,type);
    }
    return NULL;
}

/* Old TGCompare always obtains ResultType without constant demotion. */
struct tree_node *TGCompare(unsigned char op,struct tree_node *left,
    struct tree_node *right,type_def *type)
{
    tree_node *node;
    type=ResultType(left,right,type,&BinMat[0][0],0);
    left=TGConvert(left,type);
    right=TGConvert(right,type);
    node=FoldCompare(op,left,right,type);
    if (node==NULL) node=FoldBitCompare(op,left,right);
    if (node==NULL) node=FoldPostGetsCompare(op,left,right,type);
    if (node==NULL) node=TGNode(3,op,left,right,TypeBoolean); /* TN_COMPARE */
    return node;
}

/* NotAddrGen's SetAddress/TreeGen/restore sequence is inlined here. */
void Control(unsigned char op,struct tree_node *node,struct code_lbl *label,
    unsigned char generate)
{
    address_name *addr;
    unsigned char was_address;
    node=TGConvert(node,TypeBoolean);
    was_address=SetAddress(0);
    addr=TreeGen(node);
    SetAddress(was_address);
    if (addr->format==2) {
        if (CFTest(addr->u.name->c.value)!=0) {
            if (op==O_IF_TRUE) BGGenCtrl(O_GOTO,NULL,label,generate);
        } else if (op==O_IF_FALSE) BGGenCtrl(O_GOTO,NULL,label,generate);
        BGDone(addr);
    } else {
        BGGenCtrl(op,(bool_node *)addr,label,generate);
    }
}

unsigned char SetAddress(unsigned char value)
{
    unsigned char old;
    old=IsAddress;
    IsAddress=value;
    return old;
}

struct address_name *TreeGen(struct tree_node *value)
{
    return SafeRecurse(DoTreeGen,value);
}

/* Old DoTreeGen inlines NotAddrGen, TNFlow, TNBitRVal and FreeTreeNode at
 * the actual callsites below; their genuine remaining helpers stay distinct. */
struct address_name *DoTreeGen(struct tree_node *node)
{
    address_name *result,*left,*right;
    unsigned char old_address;
    result=NULL; /* Original default result is uninitialized after Zoiks. */
    switch (node->class) {
    case 0:result=node->u.addr;break;
    case 18:result=AddrName(node->u.name,node->tipe);break;
    case 12: /* TN_COMMA */
        old_address=SetAddress(0);
        left=TreeGen(node->u.left);
        SetAddress(old_address);
        BGDone(left);
        result=SafeRecurse(DoTreeGen,node->rite);
        break;
    case 24: /* TN_SIDE_EFFECT */
        result=SafeRecurse(DoTreeGen,node->u.left);
        old_address=SetAddress(0);
        right=TreeGen(node->rite);
        SetAddress(old_address);
        BGDone(right);
        break;
    case 2:result=TNBinary(node);break;
    case 1:result=TNUnary(node);break;
    case 3:result=TNCompare(node);break;
    case 4:case 5:result=TNAssign(node);break;
    case 7:case 8:result=TNPreGets(node);break;
    case 9:result=TNPostGets(node);break;
    case 6: /* TN_FLOW */
        left=NotAddrGen(node->u.left);
        /* The old missing-RHS register is uninitialized for unary flow;
         * BGFlow does not consume it in that role. Supply a defined NULL. */
        right=NULL;
        if (node->rite!=NULL) right=NotAddrGen(node->rite);
        result=(address_name *)BGFlow(node->op,(bool_node *)left,(bool_node *)right);
        break;
    case 11:result=TNCall(node,0);break;
    case 13: /* TN_FLOW_OUT */
        old_address=SetAddress(0);
        left=TreeGen(node->u.left);
        SetAddress(old_address);
        result=Arithmetic(left,node->tipe);
        break;
    case 14:result=TNQuestion(node);break;
    case 17: /* TN_BIT_RVALUE */
        left=AddrGen(node->u.left);
        left=BGUnary(O_POINTS,left,node->tipe);
        result=TNBitShift(left,(bit_tree_node *)node,0);
        break;
    case 19:result=TNWarp(node);break;
    case 20:result=TNCallback(node);break;
    case 22:
        result=TNPatch(node);
        /* This old switch really falls through to Zoiks after TNPatch. */
    default:Zoiks(57);break;
    }
    FrlFreeSize(&TreeFrl,node,sizeof(*node));
    return result;
}

/* Comparison evaluation orders larger trees first, with NotAddrGen inlined. */
struct address_name *TNCompare(struct tree_node *node)
{
    address_name *result,*left,*right;
    struct code_lbl *entry;
    unsigned char old_address;
    entry=BGGetEntry();
    result=FoldConsCompare(node->op,node->u.left,node->rite,node->u.left->tipe);
    if (result==NULL) {
        if (node->u.left->kids>=node->rite->kids) {
            old_address=SetAddress(0);
            left=TreeGen(node->u.left);
            SetAddress(old_address);
            old_address=SetAddress(0);
            right=TreeGen(node->rite);
            SetAddress(old_address);
        } else {
            old_address=SetAddress(0);
            right=TreeGen(node->rite);
            SetAddress(old_address);
            old_address=SetAddress(0);
            left=TreeGen(node->u.left);
            SetAddress(old_address);
        }
        result=(address_name *)BGCompare(node->op,left,right,entry,left->tipe);
    } else {
        result=(address_name *)Boolean(Arithmetic(result,TypeInteger),entry);
    }
    return result;
}

struct address_name *TNUnary(struct tree_node *node)
{
    address_name *left,*result;
    result=NULL;
    if (node->op==O_POINTS) {
        left=AddrGen(node->u.left);
    } else if (node->op==36) {
        left=SafeRecurse(DoTreeGen,node->u.left);
        result=BGConvert(left,node->tipe);
    } else {
        left=SafeRecurse(DoTreeGen,node->u.left);
    }
    if (result==NULL) result=BGUnary(node->op,left,node->tipe);
    result->flags|=2; /* STACKABLE */
    if (node->flags&4) AddrDemote(result); /* TF_DEMOTED */
    return result;
}
