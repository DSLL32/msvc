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
#include "dbinfo.h"
#include "stubs.h"
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

unsigned char FrlFreeAll(void **head,int size);
unsigned char TreeFrlFree(void)
{
    return FrlFreeAll(&TreeFrl,sizeof(tree_node));
}

/* Later TFini calls TreeFrlFree; this old body contains its direct pool
 * release, with a distinct entry and no intervening helper vertex. */
void TFini(void)
{
    FrlFreeAll(&TreeFrl,sizeof(tree_node));
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
    } else if (!CFIsSize(right->u.name->c.value,left->tipe->length)) {
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
void *DoTreeGen(void *value)
{
    tree_node *node=value;
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

/* Old AddrGen saves only flags/base; later alignment propagation is absent. */
struct address_name *AddrGen(struct tree_node *node)
{
    address_name *result;
    name *base;
    unsigned char old_address,flags;
    old_address=IsAddress;
    IsAddress=1;
    base=TNFindBase(node);
    flags=node->flags;
    result=SafeRecurse(DoTreeGen,node);
    if (result->format!=3) { /* NF_BOOL has no address flags/base. */
        if (flags&1) result->flags|=4; /* TF_VOLATILE / VOLATILE */
        result->base=base;
    }
    IsAddress=old_address;
    return result;
}

/* SafeRecurse transports either an address or a name pointer. Keep its actual
 * generic callback ABI, and recover the typed tree/name semantics here. */
void *TNFindBase(void *value)
{
    tree_node *node=value;
    name *op;
    if (node==NULL) return NULL;
    if (node->base!=NULL) return node->base;
    switch (node->class) {
    case 0: /* TN_LEAF */
        return TNGetLeafName(node);
    case 12:case 24: /* TN_COMMA / TN_SIDE_EFFECT */
        if (!(node->tipe->attr&4)) return NULL;
        op=SafeRecurse(TNFindBase,node->class==12?node->u.left:node->rite);
        if (op!=NULL) return op;
        /* fall through */
    case 2: /* TN_BINARY: a based-pointer conversion has no fake base. */
        if (node->class==2 && node->op==36) return NULL;
        /* fall through */
    case 5:case 8: /* TN_LV_ASSIGN / TN_LV_PRE_GETS */
        if (!(node->tipe->attr&4)) return NULL;
        op=SafeRecurse(TNFindBase,node->rite);
        if (op!=NULL) return op;
        if (node->op==36 || node->class!=2) return NULL;
        return SafeRecurse(TNFindBase,node->u.left);
    case 1: /* TN_UNARY */
        if (!(node->tipe->attr&4)) return NULL;
        switch (node->op) {
        case 33:case 34:case 36:
            return SafeRecurse(TNFindBase,node->u.left);
        }
        if (!(Model&0x20000000UL)) return NULL; /* FORTRAN_ALIASING */
        if (node->op!=O_POINTS) return NULL;
        switch (node->u.left->class) {
        case 24:return SafeRecurse(TNFindBase,node->u.left->rite);
        case 12:return SafeRecurse(TNFindBase,node->u.left->u.left);
        case 5:case 8:return SafeRecurse(TNFindBase,node->u.left->rite);
        }
        node=node->u.left;
        if (node->class!=0) return NULL;
        op=TNGetLeafName(node);
        if (op==NULL) return NULL;
        if (op->n.class==2) { /* N_TEMP */
            if (op->v.symbol==NULL) return NULL;
            op=SAllocUserTemp(FEAuxInfo(op->v.symbol,9),
                              op->n.name_class,op->n.size);
        }
        return op;
    }
    return NULL;
}

/* The old generator retries folding after generating each operand. Its
 * TreeGen and SetAddress helpers are inlined at these actual callsites. */
struct address_name *TNBinary(struct tree_node *node)
{
    address_name *left,*right,*result;
    tree_node *l,*r,*folded;
    unsigned char old_address;
    if (node->u.left->kids>=node->rite->kids) {
        left=SafeRecurse(DoTreeGen,node->u.left);
        right=SafeRecurse(DoTreeGen,node->rite);
    } else {
        right=SafeRecurse(DoTreeGen,node->rite);
        left=SafeRecurse(DoTreeGen,node->u.left);
    }
    l=TGLeaf(AddrCopy(left));
    r=TGLeaf(AddrCopy(right));
    folded=BinFold(node->op,l,r,node->tipe);
    if (folded!=NULL) {
        BGDone(left);
        BGDone(right);
        return SafeRecurse(DoTreeGen,folded);
    }
    BurnTree(l);
    BurnTree(r);
    if (node->op==36) {
        result=MakeBased(left,right,node->tipe);
    } else if (node->flags&4) {
        old_address=IsAddress;
        IsAddress=0;
        result=BGBinary(node->op,left,right,node->tipe,0);
        AddrDemote(result);
        IsAddress=old_address;
    } else {
        result=BGBinary(node->op,left,right,node->tipe,1);
    }
    result->flags|=2;
    return result;
}

name *TNGetLeafName(struct tree_node *node)
{
    address_name *addr;
    addr=node->u.addr;
    if (addr->format!=0) return NULL;
    switch (addr->class) {
    case 0:case 1:return addr->u.name; /* CL_ADDR_GLOBAL / CL_ADDR_TEMP */
    case 2:
        if (Model&0x20000000UL) return addr->u.name;
        break;
    }
    return NULL;
}

unsigned char TGIsAddress(void)
{
    return IsAddress;
}

struct tree_node *TGAssign(struct tree_node *dest,struct tree_node *source,type_def *type)
{
    return DoTGAssign(dest,source,type,4); /* TN_ASSIGN */
}

struct tree_node *DoTGAssign(struct tree_node *dest,struct tree_node *source,
                             type_def *type,unsigned char class)
{
    tree_node *node;
    type_def *node_type;
    if (type->refno==14) type=source->tipe; /* T_DEFAULT */
    source=TGConvert(source,type);
    node_type=type;
    if (class==5) node_type=dest->tipe; /* TN_LV_ASSIGN */
    node=TGNode(class,0,dest,source,node_type);
    node->optipe=type;
    return node;
}

struct tree_node *TGTrash(struct tree_node *node)
{
    node->flags&=~2; /* TF_USED */
    if (node->class==1 && node->op==O_POINTS) node->u.left->flags&=~2;
    return node;
}

struct address_name *TGen(struct tree_node *node,type_def *type)
{
    address_name *result;
    name *base;
    DGBlip(); /* TGBlip shares the existing physical progress-hook entry. */
    base=TNFindBase(node);
    node=TGConvert(node,type);
    result=SafeRecurse(DoTreeGen,node); /* TreeGen is inlined here. */
    if (result->format!=3) {
        result->flags&=~2;
        result->base=base;
    }
    return result;
}

struct address_name *TNAssign(struct tree_node *node)
{
    address_name *left,*right,*result;
    unsigned char old_address;
    result=NULL;
    if (node->u.left->class==16) {
        result=TNBitAssign(node);
    } else {
        old_address=SetAddress(0); /* NotAddrGen is inlined here. */
        right=TreeGen(node->rite);
        SetAddress(old_address);
        left=AddrGen(node->u.left);
        if (node->class==5) result=BGCopy(left);
        left=BGAssign(left,right,node->optipe);
        if (node->class==4) result=BGDuplicate(left);
        BGDone(left);
    }
    /* The actual dispatcher supplies TN_ASSIGN or TN_LV_ASSIGN. An invalid
     * other class leaves the original result register uninitialized. */
    if (result==NULL) STUB_TRAP();
    result->flags|=2;
    return result;
}

/* TGen is inlined here, while this old entry still calls TreeGen itself. */
struct address_name *TGReturn(struct tree_node *node,type_def *type)
{
    address_name *result;
    name *base;
    DGBlip();
    base=TNFindBase(node);
    node=TGConvert(node,type);
    result=TreeGen(node);
    if (result->format!=3) {
        result->flags&=~2;
        result->base=base;
    }
    result->flags|=2;
    return result;
}

name *TGetName(struct tree_node *node)
{
    name *operand;
    operand=NULL;
    if (node->class==18) operand=node->u.name;
    else if (node->class==0) operand=AddrToName(node->u.addr);
    BurnTree(node);
    return operand;
}

struct tree_node *TName(name *op,type_def *type)
{
    return TGLeaf(AddrName(op,type));
}

unsigned char TGCanDuplicate(struct tree_node *node)
{
    switch (node->class) {
    case 0:case 18: /* TN_LEAF, TN_CONS */
        return 1;
    case 1:case 16:case 17: /* unary, bit lvalue/rvalue */
        return TGCanDuplicate(node->u.left);
    case 2:
        if (!TGCanDuplicate(node->u.left)) return 0;
        if (!TGCanDuplicate(node->rite)) return 0;
        return 1;
    default:return 0;
    }
}
struct tree_node *NewTreeNode(void)
{
    tree_node *node;
    node=AllocFrl(&TreeFrl,sizeof(*node));
    node->base=NULL;
    return node;
}
struct tree_node *TGDuplicate(struct tree_node *node)
{
    tree_node *copy,*left,*right;
    bit_tree_node *bit_copy,*bit;
    if (node==NULL) return NULL;
    switch (node->class) {
    case 18: /* TGConst is inlined here. */
        copy=TGLeaf(MakeConst(CFCopy(node->u.name->c.value),node->tipe));
        break;
    case 0:
        copy=TGLeaf(AddrCopy(node->u.addr));
        break;
    case 16:case 17: /* Inlined TGBitMask, preserving bit coordinates. */
        bit=(bit_tree_node *)node;
        left=TGDuplicate(bit->u.left);
        bit_copy=(bit_tree_node *)NewTreeNode();
        bit_copy->flags=2;
        bit_copy->is_signed=0;
        bit_copy->u.left=left;
        bit_copy->tipe=bit->tipe;
        bit_copy->start=bit->start;
        bit_copy->len=bit->len;
        bit_copy->kids=left->kids+1;
        bit_copy->class=16;
        copy=(tree_node *)bit_copy;
        break;
    default:
        right=TGDuplicate(node->rite);
        left=TGDuplicate(node->u.left);
        copy=TGNode(node->class,node->op,left,right,node->tipe);
        break;
    }
    copy->class=node->class;
    return copy;
}
/* Older DoTGPreGets predates the DIV/MOD/RSHIFT demotion exclusions. */
struct tree_node *DoTGPreGets(unsigned char op,struct tree_node *left,
    struct tree_node *right,type_def *type,unsigned char class,unsigned char assign_class)
{
    tree_node *leftp,*result,*dupleft,*l,*r;
    type_def *optipe;
    TGDemote(right,type);
    dupleft=TGCanDuplicate(left) ? TGDuplicate(left):NULL;
    leftp=TGUnary(O_POINTS,left,type);
    l=leftp;
    r=right;
    optipe=BinResult(op,&l,&r,TypeNone,0);
    right=r;
    leftp=l;
    result=dupleft!=NULL ? BinFold(op,leftp,right,optipe):NULL;
    if (result==NULL) {
        result=TGNode(class,op,left,right,type);
        result->optipe=optipe;
        if (type!=optipe) {
            if (leftp!=left) {
                if (leftp->u.left!=left)
                    FrlFreeSize(&TreeFrl,leftp->u.left,sizeof(*leftp));
                FrlFreeSize(&TreeFrl,leftp,sizeof(*leftp));
            }
        } else if (leftp!=left) FrlFreeSize(&TreeFrl,leftp,sizeof(*leftp));
        if (left->class==17) left->class=16;
        if (dupleft!=NULL) BurnTree(dupleft);
    } else if (result->class==2 && result->u.left==leftp &&
        leftp->class==1 && leftp->op==O_POINTS && leftp->u.left==left) {
        result->class=class;
        result->u.left=left;
        result->optipe=optipe;
        if (left->class==17) left->class=16;
        else FrlFreeSize(&TreeFrl,leftp,sizeof(*leftp));
        BurnTree(dupleft);
    } else {
        result=DoTGAssign(dupleft,result,type,assign_class);
        if (leftp==left) result->u.left->class=16;
    }
    return result;
}
struct tree_node *TGPreGets(unsigned char op,struct tree_node *left,
    struct tree_node *right,type_def *type)
{
    return DoTGPreGets(op,left,right,type,7,4);
}

address_name *BGOpGets(unsigned char op,address_name *left,address_name *right,
    type_def *type,type_def *optype);
address_name *Int(unsigned long value)
{
    return BGInteger((long)value,TypeInteger);
}
/* TNBitRVal, Mask, DoAnd and FreeTreeNode are inlined at these sites. */
address_name *TNBitOpGets(tree_node *node,type_def *type,unsigned char post)
{
    address_name *left,*right,*before_value,*after_value,*result,*free_result;
    bit_tree_node *lhs;
    unsigned shift,i;
    unsigned long mask,shiftmask,bit;
    unsigned char old_address;
    lhs=(bit_tree_node *)node->u.left;
    left=AddrGen(lhs->u.left);
    before_value=TNBitShift(BGUnary(O_POINTS,AddrCopy(left),lhs->tipe),lhs,0);
    old_address=SetAddress(0);
    right=TreeGen(node->rite);
    SetAddress(old_address);
    if (node->op!=O_NOP) {
        right=BGConvert(right,type);
        after_value=BGBinary(node->op,BGConvert(BGDuplicate(before_value),type),
                             right,type,1);
    } else after_value=right;
    after_value=BGConvert(after_value,node->tipe);
    shift=lhs->start;
    if (lhs->len==0 || shift>=32 || lhs->len>32-shift) STUB_TRAP();
    mask=0;
    bit=1UL<<shift;
    for (i=lhs->len;i!=0;--i) { mask|=bit;bit+=bit; }
    FrlFreeSize(&TreeFrl,lhs,sizeof(tree_node));
    shiftmask=mask>>shift;
    if (after_value->format==2 && after_value->class==5) {
        result=BGInteger((long)(shiftmask&after_value->u.name->c.int_value),TypeInteger);
        if ((unsigned long)result->u.name->c.int_value!=shiftmask) {
            BGDone(BGOpGets(O_AND,AddrCopy(left),Int(~mask),node->tipe,node->tipe));
        }
        if (result->u.name->c.int_value!=0) {
            free_result=result;
            result=BGInteger((long)((unsigned long)result->u.name->c.int_value<<shift),TypeInteger);
            BGDone(free_result);
            result=BGOpGets(O_OR,left,result,node->tipe,node->tipe);
        } else BGDone(left);
        BGDone(result);
    } else {
        result=BGBinary(O_AND,AddrCopy(after_value),Int(shiftmask),node->tipe,1);
        BGDone(BGOpGets(O_AND,AddrCopy(left),Int(~mask),node->tipe,node->tipe));
        result=BGBinary(O_LSHIFT,result,BGInteger(shift,TypeInteger),node->tipe,1);
        result=BGOpGets(O_OR,left,result,node->tipe,node->tipe);
        BGDone(result);
    }
    if (post) {
        BGDone(after_value);
        result=before_value;
    } else {
        BGDone(before_value);
        result=BGBinary(O_AND,after_value,BGInteger(shiftmask,TypeInteger),node->tipe,1);
    }
    return result;
}
address_name *TNPreGets(tree_node *node)
{
    address_name *left,*right,*result;
    unsigned char old_address;
    if (node->u.left->class==16) {
        result=TNBitOpGets(node,node->optipe,0);
    } else {
        left=AddrGen(node->u.left);
        old_address=SetAddress(0); /* NotAddrGen is inlined. */
        right=TreeGen(node->rite);
        SetAddress(old_address);
        if (node->class==8) result=BGCopy(left);
        left=BGOpGets(node->op,left,right,node->tipe,node->optipe);
        if (node->class==8) BGDone(left);
        else result=left;
    }
    result->flags|=2; /* STACKABLE */
    return result;
}

/* NewTreeNode is inlined in this old public bit-mask constructor. */
bit_tree_node *TGBitMask(tree_node *value,unsigned char start,unsigned char width,type_def *type)
{
    bit_tree_node *node;
    node=AllocFrl(&TreeFrl,sizeof(tree_node));
    ((tree_node *)node)->base=NULL;
    node->flags=2;
    node->is_signed=0;
    node->u.left=value;
    node->tipe=type;
    node->start=start;
    node->len=width;
    node->kids=value->kids+1;
    node->class=16;
    return node;
}

/* The 386 JUST_USE_SHIFTS implementation; Mask is inlined in this build. */
address_name *TNBitShift(address_name *value,bit_tree_node *node,unsigned char masked)
{
    type_def *unsigned_type,*signed_type;
    unsigned long mask,bit;
    unsigned i;
    switch (node->tipe->length) {
    case 1:unsigned_type=TypeAddress(T_UINT_1);signed_type=TypeAddress(T_INT_1);break;
    case 2:unsigned_type=TypeAddress(T_UINT_2);signed_type=TypeAddress(T_INT_2);break;
    case 4:unsigned_type=TypeAddress(T_UINT_4);signed_type=TypeAddress(T_INT_4);break;
    default:STUB_TRAP();return NULL;
    }
    if (node->len==0 || node->start>=32 || node->len>32-node->start) STUB_TRAP();
    mask=0;
    bit=1UL<<node->start;
    for (i=node->len;i!=0;--i) { mask|=bit;bit+=bit; }
    if (!node->is_signed && node->start==0) {
        if (!masked) value=BGBinary(O_AND,value,BGInteger((long)mask,TypeInteger),unsigned_type,1);
    } else {
        value=BGBinary(O_LSHIFT,value,
            BGInteger(8*unsigned_type->length-node->len-node->start,TypeInteger),unsigned_type,1);
        value=BGBinary(O_RSHIFT,value,
            BGInteger(8*unsigned_type->length-node->len,TypeInteger),
            node->is_signed ? signed_type:unsigned_type,1);
    }
    return value;
}

struct tree_node *TGInitCall(struct tree_node *function,type_def *type,void *symbol)
{
    struct tree_node *node;
    unsigned *call_class;
    /* TN_PARM's rite slot stores the opaque front-end symbol, not a child. */
    node=TGNode(10,O_NOP,function,(struct tree_node *)symbol,NULL);
    node=TGNode(11,O_NOP,node,NULL,type); /* TN_CALL. */
    call_class=FEAuxInfo(symbol,2); /* CALL_CLASS. */
    if (*call_class&1) node->flags|=8; /* REVERSE_PARMS / TF_REVERSE. */
    return node;
}

struct tree_node *TGCall(struct tree_node *call);
struct tree_node *TGAddParm(struct tree_node *call,struct tree_node *value,type_def *type)
{
    struct tree_node *new_parm,*scan;
    if (type->refno==T_DEFAULT) {
        type=value->tipe;
        if (type==TypeBoolean) type=TypeInteger;
    }
    value=TGConvert(value,type);
    new_parm=TGNode(10,O_NOP,value,NULL,type);
    if (call->flags&8) {
        new_parm->rite=call->rite;
        call->rite=new_parm;
    } else {
        scan=call;
        while (scan->rite!=NULL) scan=scan->rite;
        scan->rite=new_parm;
    }
    /* The actual1994 routine falls through to TGCall's separately callable
     * identity-return entry after restoring its registers. Keep that vertex
     * and its pointer result rather than masking over a pending entry. */
    return TGCall(call);
}

struct tree_node *TGCall(struct tree_node *call)
{
    return call;
}

struct tree_node *FoldFlAnd(struct tree_node *left,struct tree_node *right);
struct tree_node *FoldFlOr(struct tree_node *left,struct tree_node *right);
struct tree_node *FoldFlNot(struct tree_node *left);
struct tree_node *TGFlow(unsigned char op,struct tree_node *left,struct tree_node *right)
{
    struct tree_node *result;
    switch (op) {
    case O_FLOW_AND: result=FoldFlAnd(left,right); break;
    case O_FLOW_OR: result=FoldFlOr(left,right); break;
    case 79: result=FoldFlNot(left); break; /* O_FLOW_NOT. */
    default: Zoiks(56); STUB_TRAP();
    }
    if (result!=NULL) return result;
    left=TGConvert(left,TypeBoolean);
    if (right!=NULL) right=TGConvert(right,TypeBoolean);
    return TGNode(6,op,left,right,TypeBoolean); /* TN_FLOW. */
}

struct address_name *NotAddrGen(struct tree_node *node)
{
    struct address_name *result;
    unsigned char old_address;
    /* This1994 body inlines SetAddress/TreeGen around the real stack callback. */
    old_address=IsAddress;
    IsAddress=0;
    result=SafeRecurse(DoTreeGen,node);
    IsAddress=old_address;
    return result;
}

struct call_node;
static struct tree_node *NodesToZap;
void BGStartInline(void *symbol);
struct call_node *BGInitCall(struct address_name *value,type_def *type,void *symbol);
struct address_name *MakeTempAddr(name *value,type_def *type);
struct address_name *PassProcParm(struct address_name *value);
type_def *PassParmType(void *symbol,type_def *type,unsigned call_class);
void BGAddInlineParm(struct address_name *value);
void BGAddParm(struct call_node *call,struct address_name *value);
struct address_name *BGStopInline(struct tree_node *call,type_def *type);
void BGZapBase(name *value,type_def *type);
void FreeTreeNode(struct tree_node *value);
struct address_name *BGCall(struct call_node *call,unsigned char use_result,unsigned char in_line);
struct address_name *TNCall(struct tree_node *what,unsigned char ignore_return)
{
    struct tree_node *addr,*scan,*parm_tree,*junk;
    struct address_name *temp,*parameter,*result;
    struct call_node *call;
    type_def *type;
    unsigned call_class;
    unsigned char in_line,old_address;
    (void)ignore_return; /* Actual result use comes from TF_USED. */
    addr=what->u.left;
    in_line=FEAuxInfo(addr->rite,1)!=NULL; /* CALL_BYTES. */
    call_class=*(unsigned *)FEAuxInfo(addr->rite,2); /* CALL_CLASS. */
    /* Sentinel for the unestablished incoming ECX owner, not an observed
     * original initialization or value. Every real argument path below
     * replaces this with its produced parameter; later by-address reuse
     * remains unqualified along with the entire PARMS_BY_ADDRESS domain. */
    parameter=NULL;
    if (call_class&8) { /* MAKE_CALL_INLINE. */
        BGDone(SafeRecurse(DoTreeGen,addr->u.left));
        BGStartInline(addr->rite);
    } else {
        call=BGInitCall(SafeRecurse(DoTreeGen,addr->u.left),what->tipe,addr->rite);
    }
    /* This older body predates MakeSPSafe. */
    for (scan=what->rite;scan!=NULL;scan=scan->rite) {
        {
            name *base;
            base=TNFindBase(scan->u.left);
            parm_tree=scan->u.left;
            scan->u.name=base;
        }
        if (call_class&4) { /* PARMS_BY_ADDRESS. */
            if (parm_tree->class==1 && parm_tree->op==O_POINTS) {
                parameter=AddrGen(parm_tree->u.left);
                parm_tree->u.left=NULL;
                parm_tree->rite=NULL;
                BurnTree(parm_tree);
            } else {
                /* At42862 the type comes from the previous parameter owner
                 * in ECX, not parm_tree. A first missing owner traps in both
                 * builds. This is not a claim that original ECX was NULL or
                 * a proof of an original stale-register/read defect. */
                if (parameter==NULL) STUB_TRAP();
                type=parameter->tipe;
                temp=MakeTempAddr(BGNewTemp(type),type);
                parameter=AddrGen(parm_tree);
                BGDone(BGAssign(BGDuplicate(temp),parameter,parameter->tipe));
                parameter=temp;
            }
            type=TypePtr;
        } else {
            /* NotAddrGen is inlined at this particular1994 callsite. */
            old_address=SetAddress(0);
            parameter=TreeGen(parm_tree);
            SetAddress(old_address);
            type=scan->tipe;
        }
        if (type->refno==T_DEFAULT) type=parameter->tipe;
        if (type==TypeProcParm) {
            if (parameter->tipe!=TypeProcParm) {
                result=PassProcParm(parameter);
                BGDone(parameter);
                parameter=result;
            }
        } else {
            if (!in_line && !(call_class&8))
                type=PassParmType(addr->rite,type,call_class);
            parameter=BGConvert(parameter,type);
        }
        parameter->flags|=2; /* STACKABLE. */
        if (call_class&8) BGAddInlineParm(parameter);
        else BGAddParm(call,parameter);
    }
    FrlFreeSize(&TreeFrl,addr,sizeof(*addr));
    if (call_class&8) {
        result=BGStopInline(what,what->tipe);
        NodesToZap=what->rite;
        /* TNZapParms is inlined here, but FreeTreeNode remains an entry. */
        scan=NodesToZap;
        while (scan!=NULL) {
            BGZapBase(scan->u.name,scan->tipe);
            junk=scan;
            scan=scan->rite;
            FreeTreeNode(junk);
        }
    } else {
        NodesToZap=what->rite;
        result=BGCall(call,(what->flags&2)!=0,in_line);
        result->flags|=2;
    }
    return result;
}

void TNZapParms(void)
{
    struct tree_node *scan,*junk;
    scan=NodesToZap;
    while (scan!=NULL) {
        BGZapBase(scan->u.name,scan->tipe);
        junk=scan;
        scan=scan->rite;
        /* FreeTreeNode is inlined at this older callsite. */
        FrlFreeSize(&TreeFrl,junk,sizeof(*junk));
    }
}

struct tree_node *TGQuestion(struct tree_node *condition,struct tree_node *first,
                             struct tree_node *second,type_def *type)
{
    tree_node *result;
    condition=TGConvert(condition,TypeBoolean);
    first=TGConvert(first,type);
    second=TGConvert(second,type);
    result=TGNode(15,0,first,second,type); /* TN_COLON */
    return TGNode(14,0,condition,result,type); /* TN_QUESTION */
}

void BGControl(unsigned char op,bool_node *condition,struct code_lbl *label);
/* Complete the question before BGGenCtrl is allowed to flush blocks. */
address_name *TNQuestion(tree_node *node)
{
    struct code_lbl *otherwise,*around;
    address_name *temp,*value;
    name *variable;
    otherwise=AskForNewLabel();
    around=AskForNewLabel();
    Control(O_IF_FALSE,node->u.left,otherwise,0);
    variable=BGNewTemp(node->tipe);
    variable->v.usage|=2; /* USE_IN_ANOTHER_BLOCK */
    temp=MakeTempAddr(variable,node->tipe);
    value=SafeRecurse(DoTreeGen,node->rite->u.left);
    BGDone(BGAssign(AddrCopy(temp),value,node->tipe));
    BGControl(O_GOTO,NULL,around);
    BGControl(O_LABEL,NULL,otherwise);
    BGFiniLabel(otherwise);
    value=SafeRecurse(DoTreeGen,node->rite->rite);
    BGDone(BGAssign(AddrCopy(temp),value,node->tipe));
    BGControl(O_LABEL,NULL,around);
    BGFiniLabel(around);
    temp=BGUnary(O_POINTS,temp,node->tipe);
    FrlFreeSize(&TreeFrl,node->rite,sizeof(*node));
    temp->flags|=2; /* STACKABLE */
    return temp;
}

tree_node *TGVolatile(tree_node *value)
{
    value->flags|=1; /* TF_VOLATILE */
    return value;
}
