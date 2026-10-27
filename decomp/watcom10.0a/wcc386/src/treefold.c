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

/* Old DWORD constants have no later _HasBigConst gate in this XOR rule. */
struct tree_node *FoldXor(struct tree_node *left,struct tree_node *right,type_def *type)
{
    tree_node *fold;
    struct cfloat *lv,*rv;
    unsigned long li,ri;
    if (left->class==18) {
        fold=left;
        left=right;
        right=fold;
    }
    fold=NULL;
    if (left->class==18) {
        lv=left->u.name->c.value;
        rv=right->u.name->c.value;
        if (CFIs32(lv) && CFIs32(rv)) {
            li=(unsigned long)CFConvertByType(lv,type);
            ri=(unsigned long)CFConvertByType(rv,type);
            fold=IntToType((long)(li^ri),type);
            BurnTree(left);
            BurnTree(right);
        }
    } else if (right->class==18) {
        rv=right->u.name->c.value;
        if (CFTest(rv)==0) {
            fold=TGConvert(left,type);
            BurnTree(right);
        } else if (right->u.name->c.int_value==-1) {
            fold=TGUnary(O_COMPLEMENT,left,type);
            BurnTree(right);
        }
    }
    return fold;
}

struct tree_node *FoldAnd(struct tree_node *left,struct tree_node *right,type_def *type)
{
    tree_node *fold;
    struct cfloat *lv,*rv;
    unsigned long value;
    if (left->class==18) {
        fold=left;left=right;right=fold;
    }
    fold=NULL;
    if (left->class==18) {
        lv=left->u.name->c.value;
        rv=right->u.name->c.value;
        if (CFIs32(lv) && CFIs32(rv)) {
            value=(unsigned long)CFConvertByType(rv,type);
            value&=(unsigned long)CFConvertByType(lv,type);
            fold=IntToType((long)value,type);
            BurnTree(left);
            BurnTree(right);
        }
    } else if (right->class==18) {
        rv=right->u.name->c.value;
        if (CFTest(rv)==0) {
            left=TGTrash(left);
            fold=TGBinary(O_COMMA,left,IntToType(0,type),type);
            BurnTree(right);
        } else if (right->u.name->c.int_value==-1) {
            fold=TGConvert(left,type);
            BurnTree(right);
        }
    }
    return fold;
}

struct tree_node *FoldLShift(struct tree_node *left,struct tree_node *right,type_def *type)
{
    tree_node *fold;
    struct cfloat *lv,*rv;
    long shift;
    unsigned long value;
    fold=NULL;
    if (right->class==18) {
        rv=right->u.name->c.value;
        shift=CFConvertByType(rv,type);
        if (left->class==18) {
            lv=left->u.name->c.value;
            if (CFIs32(lv) && CFIs32(rv)) {
                value=(unsigned long)CFConvertByType(lv,type);
                /* The original i386 SHL uses the low five count bits. */
                fold=IntToType((long)(value<<((unsigned)shift&31)),type);
                BurnTree(left);
                BurnTree(right);
            }
        } else if (shift==0) {
            fold=TGConvert(left,type);
            BurnTree(right);
        } else if (shift>=type->length*8) {
            fold=IntToType(0,type);
            left=TGTrash(left);
            fold=TGBinary(O_COMMA,left,fold,type);
            BurnTree(right);
        }
    }
    return fold;
}

/* This1994 body sign-extends byte/word values unconditionally; the later
 * released TYPE_SIGNED/unsigned branches are absent. */
long CFConvertByType(struct cfloat *value,type_def *type)
{
    long integer;
    integer=CFCnvF32(value);
    switch (type->length) {
    case 1:return (signed char)integer;
    case 2:return (short)integer;
    }
    return integer;
}

struct cfloat *CFMul(struct cfloat *left,struct cfloat *right);
struct tree_node *FoldTimes( struct tree_node * left, struct tree_node * right, type_def *type ) {
/* treefold.c, before TYPE_FLOAT was expanded to include 64-bit integers. */

    struct tree_node *          temp;
    struct tree_node *          fold;
    int         test;
    int         log;
    struct cfloat      *lv;
    struct cfloat      *rv;
    unsigned long li;
    unsigned long ri;

    if( left->class == 18 ) {
        temp = left;
        left = right;
        right = temp;
    }
    if( right->class != 18 ) return( NULL );
    if( left->class == 2 && left->op == O_TIMES &&
        type == left->tipe && !( type->attr & 1 ) ) {
        if( left->u.left->class == 18 ) {
            left->u.left = FoldTimes( left->u.left, right, type );
            return( left );
        }
        if( left->rite->class == 18 ) {
            left->rite = FoldTimes( left->rite, right, type );
            return( left );
        }
    }
    if( left->class==2 && left->op==O_LSHIFT && type==left->tipe ) {
        if( !( type->attr & 1 ) && left->rite->class == 18 ) {
            log = left->rite->u.name->c.int_value;
            li = 1;
            while( --log >= 0 ) {
                li <<= 1;
            }
            BurnTree( left->rite );
            left->rite = CFToType( CFCnvU32F( li ), type );
            left->op = O_TIMES;
            left->rite = FoldTimes( left->rite, right, type );
            return( left );
        }
    }
    rv = right->u.name->c.value;
    if( left->class == 18 ) {
        lv = left->u.name->c.value;
        if( !( type->attr & 1 ) && CFIs32( lv ) && CFIs32( rv ) ) {
            li = CFConvertByType( lv, type );
            ri = CFConvertByType( rv, type );
            ri = li * ri;
            fold = IntToType( ri, type );
        } else {
            fold = CFToType( CFMul( left->u.name->c.value, rv ), type );
        }
        BurnTree( left );
        BurnTree( right );
        return( fold );
    }
    /* This image tests the cached integer-one value without the later
     * additional big-constant guard. */
    if( right->u.name->c.int_value == 1 ) {
        BurnTree( right );
        return( left );
    }
    test = CFTest( rv );
    if( test == 0 ) {
        return( TGBinary( O_COMMA, TGTrash( left ), right, type ) );
    }
    if( ( type->attr & 1 ) ) return( NULL );
    if( test < 0 ) {
        CFNegate( rv );
    }
    fold = NULL;
    if( CFIsU32( rv ) ) {
        log = GetLog2( CFConvertByType( rv, type ) );
        if( log != -1 ) {
            fold = TGBinary( O_LSHIFT, left,
                          IntToType( log, TypeInteger ), type );
            if( test < 0 ) {
                fold = TGUnary( O_UMINUS, fold, type );
            }
            BurnTree( right );
        }
    }
    if( test < 0 ) {
        CFNegate( rv );
    }
    return( fold );
}


int GetLog2(unsigned long value)
{
    unsigned long count;
    int log;
    if (value!=0 && (value&(value-1))==0) {
        count=1;
        log=0;
        for (;;) {
            if (count==value) return log;
            count+=count;
            ++log;
        }
    }
    return -1;
}

struct cfloat *CFSub(struct cfloat *left,struct cfloat *right);
struct cfloat *OkToNegate(struct cfloat *value,type_def *type);
struct tree_node *FoldMinus( struct tree_node * left, struct tree_node * right, type_def *type ) {
/*************************************************************/

    struct tree_node *          fold;
    struct cfloat      *lv;
    struct cfloat      *rv;
    unsigned long li;
    unsigned long ri;

    fold = NULL;
    if( left->class == 18 ) {
        lv = left->u.name->c.value;
        if( right->class == 18 ) {
            rv = right->u.name->c.value;
            if( !( type->attr & 1 ) && CFIs32( lv ) && CFIs32( rv ) ) {
                li = CFConvertByType( lv, type );
                ri = CFConvertByType( rv, type );
                ri = li - ri;
                fold = IntToType( ri, type );
            } else {
                fold = CFToType( CFSub( lv, right->u.name->c.value ),
                                  type );
            }
            BurnTree( right );
            BurnTree( left );
        } else if( CFTest( lv ) == 0 ) {
            fold = TGUnary( O_UMINUS, right, type );
            BurnTree( left );
        }
    } else if( right->class == 18 ) {
        rv = OkToNegate( right->u.name->c.value, type );
        if( rv != NULL ) {
            fold = TGBinary( O_PLUS, left, TGConst( rv, right->tipe ),
                              type );
            BurnTree( right );
        }
    } else if( right->class == 1 && right->op == O_UMINUS ) {
        fold = TGBinary( O_PLUS, left, right->u.left, type );
        right->u.left = NULL;
        BurnTree( right );
    }
    return( fold );
}


struct cfloat *OkToNegate(struct cfloat *value,type_def *type)
{
    struct cfloat *negative;
    negative=CFCopy(value);
    CFNegate(negative);
    if ((type->attr&1) || (type->attr&2)) return negative;
    if (CFIsSize(negative,type->length)) return negative;
    CFFree(negative);
    return NULL;
}

/* Earlier symmetric constant fold. Unlike the released rewrite, this body
 * swaps a left constant to the right and has no TGNode comma construction. */
struct tree_node *FoldFlAnd(struct tree_node *left,struct tree_node *right)
{
    struct tree_node *temp,*fold;
    if (left->class==18) { temp=left;left=right;right=temp; }
    fold=NULL;
    if (left->class==18) {
        if (CFTest(left->u.name->c.value) && CFTest(right->u.name->c.value))
            fold=IntToType(FETrue(),TypeInteger);
        else fold=IntToType(0,TypeInteger);
        BurnTree(left);
        BurnTree(right);
    } else if (right->class==18) {
        if (CFTest(right->u.name->c.value)==0) {
            fold=IntToType(0,TypeInteger);
            BurnTree(left);
        } else fold=left;
        BurnTree(right);
    }
    return fold;
}

unsigned char FoldIfTrue(struct tree_node *value,struct code_lbl *label)
{
    unsigned char folded;
    folded=0;
    if (value->class==18) {
        if (CFTest(value->u.name->c.value)!=0) BGGenCtrl(O_GOTO,NULL,label,1);
        folded=1;
        BurnTree(value);
    }
    return folded;
}

type_def *FixAddType(struct tree_node *left,struct tree_node *right,type_def *type)
{
    if (type!=left->tipe && left->tipe==right->tipe) type=left->tipe;
    return type;
}

struct tree_node *IntToType(long value,type_def *type)
{
    return TGConst(IntToCF(value,type),type);
}

struct tree_node *FoldRShift(struct tree_node *left,struct tree_node *right,type_def *type)
{
    tree_node *fold;
    struct cfloat *lv,*rv;
    long shift,value;
    fold=NULL;
    if (right->class==18) {
        rv=right->u.name->c.value;
        shift=CFConvertByType(rv,type);
        if (left->class==18) {
            lv=left->u.name->c.value;
            if (CFIs32(lv) && CFIs32(rv)) {
                value=CFConvertByType(lv,type);
                /* The actual i386 SAR/SHR masks its count to five bits. */
                if (type->attr&2) value>>=(unsigned)shift&31;
                else value=(unsigned long)value>>((unsigned)shift&31);
                fold=IntToType(value,type);
                BurnTree(left);
                BurnTree(right);
            }
        } else if (shift==0) {
            fold=TGConvert(left,type);
            BurnTree(right);
        } else if (shift>=type->length*8 && !(type->attr&2)) {
            fold=IntToType(0,type);
            left=TGTrash(left);
            fold=TGBinary(O_COMMA,left,fold,type);
            BurnTree(right);
        }
    }
    return fold;
}

struct tree_node *FoldMod(struct tree_node *left,struct tree_node *rite,type_def *tipe) {
/***********************************************************/


    struct tree_node *fold;
    struct cfloat *rv;
    struct cfloat *lv;
    unsigned long ri;
    int         log;
    unsigned long li;

    fold = NULL;
    if( rite->class == 18 ) {
        rv = rite->u.name->c.value;
        if( CFTest( rv ) != 0
         && left->class == 18 && !(tipe->attr&1) ) {
            lv = left->u.name->c.value;
            if( tipe->attr & 2 ) {
                if( CFIsI32( lv ) && CFIsI32( rv ) ) {
                    li = CFConvertByType( lv, tipe );
                    ri = CFConvertByType( rv, tipe );
                    fold = IntToType( (long)li % (long)ri, tipe );
                    BurnTree( left );
                    BurnTree( rite );
                }
            } else {
                if( CFIsU32( lv ) && CFIsU32( rv ) ) {
                    li = CFConvertByType( lv, tipe );
                    ri = CFConvertByType( rv, tipe );
                    li = U32ModDiv( &li, ri );
                    fold = IntToType( li, tipe );
                    BurnTree( left );
                    BurnTree( rite );
                }
            }
        } else if( !(tipe->attr&1) && rite->u.name->c.int_value == 1 ) {
            fold = CFToType( CFCnvI32F( 0 ), tipe );
            fold = TGBinary( O_COMMA, left, fold, tipe );
            BurnTree( rite );
        } else if( !(tipe->attr&1)
              && ( left->tipe->attr & 2 ) == 0 ) {
            if( CFIsU32( rv ) ) {
                ri = CFConvertByType( rv, tipe );
                log = GetLog2( ri );
                if( log != -1 ) {
                    fold = TGBinary( O_AND, left,
                                  IntToType( ri-1, tipe ), tipe );
                    BurnTree( rite );
                }
            }
        }
    } else if( left->class == 18 ) {
        if( CFTest( left->u.name->c.value ) == 0 ) {
            rite = TGTrash( rite );
            fold = TGBinary( O_COMMA, rite, IntToType( 0, tipe ), tipe );
            BurnTree( left );
        }
    }
    return( fold );
}



struct cfloat *CFDiv(struct cfloat *left,struct cfloat *right);
struct cfloat *CFInverse(struct cfloat *value);
struct tree_node *FoldDiv(struct tree_node *left,struct tree_node *rite,type_def *tipe) {
/***********************************************************/


    struct tree_node *fold;
    struct cfloat *rv;
    struct cfloat *lv;
    struct cfloat *tmp;
    unsigned long ri;
    int         log;
    unsigned long li;

    fold = NULL;
    if( rite->class == 18 ) {
        rv = rite->u.name->c.value;
        if( (tipe->attr&1) ) {
            if( CFTest( rv ) != 0 && left->class == 18 ) {
                fold = CFToType( CFDiv( left->u.name->c.value, rv ), tipe );
                BurnTree( left );
                BurnTree( rite );
            } else if( ( (Model&0x80000) ||
                ( CFIsU32( rv ) && GetLog2( CFConvertByType( rv, tipe ) ) != -1 ) ) ) {
                if( CFTest( rv ) != NULL ) {
                    tmp = CFInverse( rv );
                    if( tmp != NULL ) {
                        fold = TGBinary( O_TIMES, left, TGConst( tmp, tipe ), tipe );
                        BurnTree( rite );
                    }
                }
            }
        } else if( CFTest( rv ) != 0 && left->class == 18 ) {
            lv = left->u.name->c.value;
            if( tipe->attr & 2 ) {
                if( CFIsI32( lv ) && CFIsI32( rv ) ) {
                    li = CFConvertByType( lv, tipe );
                    ri = CFConvertByType( rv, tipe );
                    fold = IntToType( (long)li / (long)ri, tipe );
                    BurnTree( left );
                    BurnTree( rite );
                }
            } else {
                if( CFIsU32( lv ) && CFIsU32( rv ) ) {
                    li = CFCnvF32( lv );
                    ri = CFCnvF32( rv );
                    U32ModDiv( &li, ri );
                    fold = IntToType( li, tipe );
                    BurnTree( left );
                    BurnTree( rite );
                }
            }
        } else if( rite->u.name->c.int_value == 1 ) {
            fold = TGConvert( left, tipe );
            BurnTree( rite );
        } else if( !(tipe->attr&1)
              && ( left->tipe->attr & 2 ) == 0 ) {
            if( CFIsU32( rv ) ) {
                log = GetLog2( rite->u.name->c.int_value );
                if( log != -1 ) {
                    fold = TGBinary( O_RSHIFT, left,
                                  IntToType( log, TypeInteger ), tipe );
                    BurnTree( rite );
                }
            }
        }
    } else if( left->class == 18 ) {
        if( CFTest( left->u.name->c.value ) == 0 ) {
            rite = TGTrash( rite );
            fold = TGBinary( O_COMMA, rite, TGConvert( left, tipe ), tipe );
        }
    }
    return( fold );
}


