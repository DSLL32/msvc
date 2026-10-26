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
* Description:  Code-generator tree initialization and arithmetic type rules.
*
****************************************************************************/


#include "codegen.h"
#include "procdef.h"
#include "cgtree.h"
#include "genquads.h"
#include "cfloat.h"
#include <recon.h>

/* treefold.c before 64-bit constant support: TYPE_FLOAT is the big-constant
 * condition and the integer folding cache/product is exactly one DWORD. */
struct tree_node *FoldPlus(struct tree_node *left,struct tree_node *right,type_def *type)
{
    tree_node *fold,*temp;
    struct cfloat *lv,*rv;
    unsigned long li,ri;
    if (left->class==18) { temp=left;left=right;right=temp; }
    fold=NULL;
    if (left->class==18) {
        lv=left->u.name->c.value;
        if (right->class==18) {
            rv=right->u.name->c.value;
            if (!(type->attr&1) && CFIs32(lv) && CFIs32(rv)) {
                li=(unsigned long)CFConvertByType(lv,type);
                ri=(unsigned long)CFConvertByType(rv,type);
                fold=IntToType((long)(li+ri),type);
            } else {
                fold=CFToType(CFAdd(lv,right->u.name->c.value),type);
            }
            BurnTree(right);
            BurnTree(left);
        } else if (CFTest(lv)==0) {
            fold=TGConvert(right,type);
            BurnTree(left);
        }
    } else if (right->class==18) {
        if (CFTest(right->u.name->c.value)==0) {
            fold=TGConvert(left,type);
            BurnTree(right);
        } else if (left->class==2 && left->op==O_PLUS &&
                type==left->tipe && !(type->attr&1)) {
            if (left->u.left->class==18) {
                type=FixAddType(left->u.left,right,type);
                if (left->u.left->tipe==type) {
                    left->u.left=FoldPlus(left->u.left,right,type);
                    fold=left;
                }
            } else if (left->rite->class==18) {
                type=FixAddType(left->rite,right,type);
                if (left->rite->tipe==type) {
                    left->rite=FoldPlus(left->rite,right,type);
                    fold=left;
                }
            }
        }
    } else if (right->class==1 && right->op==O_UMINUS) {
        fold=TGBinary(O_MINUS,left,right->u.left,type);
        right->u.left=NULL;
        BurnTree(right);
    } else if (left->class==1 && left->op==O_UMINUS) {
        fold=TGBinary(O_MINUS,right,left->u.left,type);
        left->u.left=NULL;
        BurnTree(left);
    }
    return fold;
}

static unsigned char RevOpcode[]={O_EQ,O_NE,O_LT,O_GE,O_GT,O_LE};

/* Old FoldCompare has the bit-test rewrite, before FindBase/BurnToBase. */
struct tree_node *FoldCompare(unsigned char op,struct tree_node *left,
    struct tree_node *right,type_def *type)
{
    tree_node *temp;
    struct cfloat *lv,*rv;
    int compare,result,true_value;
    if (left->class==18) {
        temp=left;left=right;right=temp;
        op=RevOpcode[op-O_EQ];
    }
    true_value=FETrue();
    if (left->class==18) {
        lv=CnvCFToType(left->u.name->c.value,type);
        rv=CnvCFToType(right->u.name->c.value,type);
        compare=CFCompare(lv,rv);
        CFFree(lv);
        CFFree(rv);
        result=0;
        switch (op) {
        case O_EQ:if (compare==0) result=true_value;break;
        case O_NE:if (compare!=0) result=true_value;break;
        case O_GT:if (compare>0) result=true_value;break;
        case O_LT:if (compare<0) result=true_value;break;
        case O_GE:if (compare>=0) result=true_value;break;
        case O_LE:if (compare<=0) result=true_value;break;
        }
        BurnTree(left);
        BurnTree(right);
        return IntToType(result,TypeInteger);
    }
    if (right->class==18 && left->class==2 && left->op==O_AND &&
        (op==O_EQ || op==O_NE)) {
        if (left->u.left->class==18) {
            temp=left->u.left;left->u.left=left->rite;left->rite=temp;
        }
        if (left->rite->class==18 && left->rite->u.name==right->u.name &&
            GetLog2((unsigned long)right->u.name->c.int_value)!=-1) {
            right->u.name=AllocIntConst(0);
            op=op==O_EQ?O_NE:O_EQ;
        }
        if (CFTest(right->u.name->c.value)!=0) return NULL;
        BurnTree(right);
        left->class=3; /* TN_COMPARE */
        left->tipe=TypeBoolean;
        left->op=op==O_EQ?47:46; /* BIT_TEST_FALSE / TRUE */
        return left;
    }
    return NULL;
}

struct tree_node *FoldBitCompare(unsigned char op,struct tree_node *left,
    struct tree_node *right)
{
    tree_node *fold,*temp;
    bit_tree_node *bits;
    unsigned long constant,mask;
    if (left->class==18) {
        temp=left;left=right;right=temp;
        op=RevOpcode[op-O_EQ];
    }
    fold=NULL;
    bits=(bit_tree_node *)left;
    if (right->class==18 && left->class==17 && !bits->is_signed) {
        constant=(unsigned long)right->u.name->c.int_value;
        constant<<=bits->start;
        mask=Mask(bits);
        if ((constant&~mask)==0) {
            fold=TGUnary(O_POINTS,bits->u.left,bits->tipe);
            fold=TGBinary(O_AND,fold,IntToType((long)mask,bits->tipe),bits->tipe);
            fold=TGCompare(op,fold,IntToType((long)constant,bits->tipe),bits->tipe);
            bits->u.left=NULL;
            BurnTree(left);
            BurnTree(right);
        }
    }
    return fold;
}

struct tree_node *FoldPostGetsCompare(unsigned char op,struct tree_node *left,
    struct tree_node *right,type_def *type)
{
    tree_node *temp;
    struct cfloat *lv,*rv;
    unsigned long li,ri,value;
    if (left->class==18) {
        temp=left;left=right;right=temp;
        op=RevOpcode[op-O_EQ];
    }
    if (op==O_GT && !(type->attr&2) && right->class==18 &&
        CFTest(right->u.name->c.value)==0) op=O_NE;
    if (op!=O_EQ && op!=O_NE) return NULL;
    if (right->class==18 && left->class==9 &&
        (left->op==O_MINUS || left->op==O_PLUS) &&
        left->rite->class==18 && SimpleLeaf(left->u.left) && type==left->tipe) {
        rv=right->u.name->c.value;
        lv=left->rite->u.name->c.value;
        if (!(type->attr&1) && CFIs32(lv) && CFIs32(rv)) {
            li=(unsigned long)CFConvertByType(lv,type);
            ri=(unsigned long)CFConvertByType(rv,type);
            value=left->op==O_MINUS?ri-li:ri+li;
            temp=IntToType((long)value,type);
            left->class=7; /* TN_PRE_GETS */
            left->optipe=left->tipe;
            temp=TGNode(3,op,left,temp,TypeBoolean);
            BurnTree(right);
            return temp;
        }
    }
    return NULL;
}

unsigned char FoldIfFalse(struct tree_node *value,struct code_lbl *label)
{
    unsigned char folded;
    folded=0;
    if (value->class==18) {
        if (CFTest(value->u.name->c.value)==0) BGGenCtrl(O_GOTO,NULL,label,1);
        folded=1;
        BurnTree(value);
    }
    return folded;
}

struct address_name *FoldConsCompare(unsigned char op,struct tree_node *left,
    struct tree_node *right,type_def *type)
{
    tree_node *temp;
    address_name *fold;
    struct cfloat *false_value,*true_value,*rv;
    int compare,compare_true;
    if (left->class==18) {
        temp=left;left=right;right=temp;
        op=RevOpcode[op-O_EQ];
    }
    fold=NULL;
    if (right->class==18 && left->class==13) { /* TN_FLOW_OUT */
        false_value=CFCnvI32F(0);
        true_value=CFCnvI32F(FETrue());
        rv=CnvCFToType(right->u.name->c.value,type);
        compare=CFCompare(false_value,rv);
        compare_true=CFCompare(true_value,rv);
        CFFree(rv);
        CFFree(false_value);
        CFFree(true_value);
        fold=TreeGen(left->u.left);
        switch (op) {
        case O_EQ:fold=Flip(fold,compare==0,compare_true==0);break;
        case O_NE:fold=Flip(fold,compare!=0,compare_true!=0);break;
        case O_GT:fold=Flip(fold,compare>0,compare_true>0);break;
        case O_LT:fold=Flip(fold,compare<0,compare_true<0);break;
        case O_GE:fold=Flip(fold,compare>=0,compare_true>=0);break;
        case O_LE:fold=Flip(fold,compare<=0,compare_true<=0);break;
        case 46:fold=Flip(fold,0,(right->u.name->c.int_value&FETrue())!=0);break;
        case 47:fold=Flip(fold,1,(right->u.name->c.int_value&FETrue())==0);break;
        }
        BurnTree(right);
        left->u.left=NULL;
        BurnTree(left);
    }
    return fold;
}
