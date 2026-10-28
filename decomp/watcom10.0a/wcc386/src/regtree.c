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
* Description:  Register allocation trees for aliased temporary names.
*
****************************************************************************/


#include "codegen.h"
#include "block.h"
#include "stubs.h"
#include "regchoices.h"
#include <recon.h>
static void *RegFrl,*TreeFrl;

void RegTreeInit(void)
{
    InitFrl(&RegFrl);
    InitFrl(&TreeFrl);
}

unsigned char FrlFreeAll(void **head,int size);
unsigned char RegTreeFrlFree(void)
{
    unsigned char freed;
    freed=FrlFreeAll(&TreeFrl,sizeof(reg_tree));
    freed|=FrlFreeAll(&RegFrl,51*sizeof(hw_reg_set));
    return freed;
}

reg_tree *BuildTree(name *whole,name *temp,long offset,long size);
reg_tree *CheckTree(reg_tree *tree);
void TrimTree(reg_tree *tree);
void BuildNameTree(conflict_node *conf)
{
    name *temp;
    reg_tree *tree;
    temp=conf->name;
    if (temp->n.class==2 && (name *)temp->t.alias!=temp) {
        tree=BuildTree(temp,temp,temp->v.offset,temp->n.size);
        tree=CheckTree(tree);
        if (tree!=NULL) TrimTree(tree);
    } else tree=NULL;
    conf->tree=tree;
}
void FreeRegSet(hw_reg_set *regs);
void FreeRegTree(reg_tree *tree);
/* The older target has the recursive BurnRegTree implementation directly
 * at the BurnNameTree call endpoint; no separate forwarding body remains. */
void BurnNameTree(reg_tree *tree)
{
    if (tree!=NULL) {
        BurnNameTree(tree->lo);
        BurnNameTree(tree->hi);
        if (tree->regs!=NULL) FreeRegSet(tree->regs);
        FreeRegTree(tree);
    }
}

reg_tree *NewTree(void);
unsigned char RegIntersect(unsigned char left,unsigned char right);
reg_tree *BuildTree(name *alias,name *master,long offset,long size)
{
    reg_tree *tree;
    name *temp;
    unsigned char have_lo,have_hi;
    long losize,hisize,midpoint;
    tree=NewTree();
    tree->offset=offset;
    tree->size=size;
    if (alias!=NULL) {
        tree->temp=alias;
        tree->has_name=1;
        alias->t.temp_flags|=4; /* VISITED */
        for (temp=(name *)alias->t.alias;temp!=alias;temp=(name *)temp->t.alias) {
            if (temp->v.offset==offset && temp->n.size==size) {
                tree->alt=temp;
                temp->t.temp_flags|=4;
                break;
            }
        }
    }
    if (tree->alt==NULL) {
        if (tree->temp!=NULL) tree->idx=tree->temp->t.possible;
    } else tree->idx=RegIntersect(tree->temp->t.possible,tree->alt->t.possible);
    if (size==6) {
        losize=4;
        hisize=2;
    } else losize=hisize=size/2;
    midpoint=offset+losize;
    if (losize!=0) {
        have_lo=have_hi=0;
        for (temp=(name *)master->t.alias;temp!=master;temp=(name *)temp->t.alias) {
            if (!have_lo && temp->v.offset==offset && temp->n.size==losize) {
                tree->lo=BuildTree(temp,master,offset,losize);
                have_lo=1;
            } else if (!have_hi && temp->v.offset==midpoint && temp->n.size==hisize) {
                tree->hi=BuildTree(temp,master,midpoint,hisize);
                have_hi=1;
            }
        }
        if (!have_lo) tree->lo=BuildTree(NULL,master,offset,losize);
        if (!have_hi) tree->hi=BuildTree(NULL,master,midpoint,hisize);
        if (tree->hi->has_name) tree->has_name=1;
        if (tree->lo->has_name) tree->has_name=1;
    }
    return tree;
}
reg_tree *CheckTree(reg_tree *tree)
{
    name *alias,*temp;
    alias=tree->temp;
    temp=alias;
    for (;;) {
        temp=(name *)temp->t.alias;
        if (!(temp->t.temp_flags&4)) {
            BurnNameTree(tree);
            return NULL;
        }
        if (temp==alias) break;
    }
    return tree;
}
void TrimTree(reg_tree *tree)
{
    if (tree->lo!=NULL) {
        if (!tree->lo->has_name) {
            BurnNameTree(tree->lo);
            tree->lo=NULL;
        } else TrimTree(tree->lo);
    }
    if (tree->hi!=NULL) {
        if (!tree->hi->has_name) {
            BurnNameTree(tree->hi);
            tree->hi=NULL;
        } else TrimTree(tree->hi);
    }
}

void FreeRegSet(hw_reg_set *regs)
{
    FrlFreeSize(&RegFrl,regs,51*sizeof(hw_reg_set));
}
void FreeRegTree(reg_tree *tree)
{
    FrlFreeSize(&TreeFrl,tree,sizeof(reg_tree));
}

void BuildPossible(reg_tree *tree);
void CheckBigPointer(reg_tree *tree);
unsigned char Intersect(reg_tree *tree);
void CompressSets(reg_tree *tree);
void SimpleTree(conflict_node *conf);
void BuildRegTree(conflict_node *conf)
{
    name *temp;
    reg_tree *tree;
    temp=conf->name;
    if (temp->n.class==2 && (name *)temp->t.alias!=temp) {
        tree=BuildTree(temp,temp,temp->v.offset,temp->n.size);
        tree=CheckTree(tree);
        if (tree!=NULL) {
            BuildPossible(tree);
            TrimTree(tree);
            CheckBigPointer(tree);
            while (Intersect(tree)) {}
            CompressSets(tree);
        }
        conf->tree=tree;
    } else SimpleTree(conf);
}
hw_reg_set *AllocRegSet(void);
void BuildPossible(reg_tree *tree)
{
    hw_reg_set *src,*dst;
    if (tree!=NULL) {
        BuildPossible(tree->lo);
        BuildPossible(tree->hi);
        if (tree->idx!=RL_NUMBER_OF_SETS) {
            tree->regs=AllocRegSet();
            src=RegSets[tree->idx];
            dst=tree->regs;
            for (;;) {
                *dst=*src;
                if (*dst==0) break;
                ++src;
                ++dst;
            }
        }
    }
}
/* The old register-set cache has MAX_RG50 plus its empty terminator:
 * exactly51 four-byte hardware sets in the original 204-byte allocation. */
#define REG_SET_SIZE 51
hw_reg_set *AllocRegSet(void)
{
    hw_reg_set *regs,*curr;
    int i;
    regs=AllocFrl(&RegFrl,REG_SET_SIZE*sizeof(hw_reg_set));
    curr=regs;
    i=REG_SET_SIZE;
    while (--i>=0) *curr++=0;
    return regs;
}
unsigned char HasSegRegs(reg_tree *tree)
{
    hw_reg_set *regs;
    regs=tree->regs;
    if (regs==NULL) return 0;
    for (;;) {
        if (*regs==0) return 0;
        if (*regs&0xc000f000U) return 1; /* Actual HW_SEGS */
        ++regs;
    }
}
void CheckBigPointer(reg_tree *tree)
{
    if (tree->lo!=NULL && HasSegRegs(tree->lo)) {
        tree->lo->idx=RL_NUMBER_OF_SETS;
        FreeRegSet(tree->lo->regs);
        tree->lo->regs=NULL;
    }
    if (tree->hi!=NULL && tree->hi->idx==RL_WORD && HasSegRegs(tree)) {
        tree->hi->idx=RL_NUMBER_OF_SETS;
        FreeRegSet(tree->hi->regs);
        tree->hi->regs=NULL;
    }
}
hw_reg_set HighOffsetReg(hw_reg_set reg);
hw_reg_set LowOffsetReg(hw_reg_set reg);
unsigned char PartIntersect(reg_tree *part,reg_tree *whole,hw_reg_set (*rtn)(hw_reg_set))
{
    unsigned char change;
    int i,j;
    hw_reg_set *src,*dst,curr,tmp;
    change=0;
    if (whole->idx!=RL_NUMBER_OF_SETS) {
        if (part->idx==RL_NUMBER_OF_SETS) {
            part->regs=AllocRegSet();
            part->idx=RL_;
            src=whole->regs;
            dst=part->regs;
            i=REG_SET_SIZE;
            while (--i>=0) {
                *dst=rtn(*src);
                ++dst;
                ++src;
            }
        }
        src=whole->regs;
        i=REG_SET_SIZE;
        while (--i>=0) {
            if (*src!=0) {
                curr=rtn(*src);
                if (curr!=0) {
                    j=REG_SET_SIZE;
                    dst=part->regs;
                    while (--j>=0) {
                        if (*dst==curr) break;
                        ++dst;
                    }
                    if (j<0) {
                        *src=0;
                        change=1;
                    }
                } else {
                    *src=0;
                    change=1;
                }
            }
            ++src;
        }
        src=part->regs;
        i=REG_SET_SIZE;
        while (--i>=0) {
            curr=*src;
            if (curr!=0) {
                j=REG_SET_SIZE;
                dst=whole->regs;
                while (--j>=0) {
                    if (*dst!=0) {
                        tmp=rtn(*dst);
                        if (curr==tmp) break;
                    }
                    ++dst;
                }
                if (j<0) {
                    *src=0;
                    change=1;
                }
            }
            ++src;
        }
    }
    return change;
}
unsigned char Intersect(reg_tree *tree)
{
    unsigned char change;
    change=0;
    if (tree->hi!=NULL) {
        change|=PartIntersect(tree->hi,tree,HighOffsetReg);
        change|=Intersect(tree->hi);
    }
    if (tree->lo!=NULL) {
        change|=PartIntersect(tree->lo,tree,LowOffsetReg);
        change|=Intersect(tree->lo);
    }
    return change;
}
void CompressSets(reg_tree *tree)
{
    hw_reg_set *src,*dst;
    int i;
    if (tree!=NULL) {
        CompressSets(tree->lo);
        CompressSets(tree->hi);
        if (tree->regs!=NULL) {
            dst=tree->regs;
            src=dst;
            i=REG_SET_SIZE;
            while (--i>=0) {
                if (*src!=0) {
                    *dst=*src;
                    ++dst;
                }
                ++src;
            }
            while (dst!=src) *dst++=0;
        }
    }
}
reg_tree *NewTree(void);
void SimpleTree(conflict_node *conf)
{
    reg_tree *tree;
    name *temp;
    tree=NewTree();
    temp=conf->name;
    tree->temp=temp;
    tree->size=temp->n.size;
    tree->offset=temp->v.offset;
    tree->idx=conf->possible;
    BuildPossible(tree);
    conf->tree=tree;
}
reg_tree *AllocRegTree(void);
reg_tree *NewTree(void)
{
    reg_tree *tree;
    tree=AllocRegTree();
    tree->alt=NULL;
    tree->lo=NULL;
    tree->hi=NULL;
    tree->regs=NULL;
    tree->has_name=0;
    tree->idx=26; /* RL_NUMBER_OF_SETS */
    tree->temp=NULL;
    return tree;
}
reg_tree *AllocRegTree(void)
{
    return AllocFrl(&TreeFrl,sizeof(reg_tree));
}

