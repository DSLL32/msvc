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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


#include <stdio.h>
#include <string.h>
#include "quads.h"
#include "codegen.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif

int LeafRepCount;
void LeafReplace(LEAFPTR leaf,unsigned index)
{
    LEAFPTR dst;
    ++LeafRepCount;
    if (index>=0xfffb) printf("***** LeafReplace: leaf %x being updated\n",index);
    dst=AccessFileInfo(index,&LeafFileInfo);
    memcpy(dst,leaf,sizeof(LEAF));
    LeafFileInfo.curr_buf_dirty=1;
}

int LeafGetCount;
void LeafGet(LEAFPTR leaf,unsigned index)
{
    LEAFPTR source;
    ++LeafGetCount;
    if (index>=0xfffb) {
        InitLeaf(leaf);
        leaf->leaf_class=0x0a;
        leaf->leaf_flags=0x40;
        leaf->leaf_type=GetType(TYPE_INT);
        switch (index-0xfffb) {
        case 0:
            leaf->leaf_class=0x0c;
            leaf->leaf_type=GetType(TYPE_LONG);
            leaf->u.constant.value.integer=0;
            break;
        case 1: leaf->u.constant.value.integer=4; break;
        case 2: leaf->u.constant.value.integer=2; break;
        case 3: leaf->u.constant.value.integer=1; break;
        case 4: leaf->u.constant.value.integer=0; break;
        }
    } else {
        source=AccessFileInfo(index,&LeafFileInfo);
        memcpy(leaf,source,sizeof(LEAF));
    }
}

void FreeLeaf(unsigned index)
{
    LEAF leaf;
    LeafGet(&leaf,index);
    if (leaf.leaf_class!=0x10 && leaf.leaf_class!=0x13) {
        leaf.u.words[1]=LeafFreeList;
        LeafReplace(&leaf,index);
        LeafFreeList=index;
    }
}

/* Leaf predecessor of the later symbol-expression tree constructor. */
unsigned MakeSymLeaf(SYMPTR sym,SYM_HANDLE handle)
{
    LEAF leaf;
    TYPEPTR typ;
    InitLeaf(&leaf);
    leaf.leaf_class=0;
    leaf.leaf_flags=0x20;
    leaf.u.symbol.handle=handle;
    typ=sym->sym_type;
    for (;;) {
        leaf.leaf_flags|=typ->decl_flags&0x1f;
        if (typ->decl_type!=TYPE_TYPEDEF) break;
        typ=typ->object;
    }
    leaf.leaf_type=typ;
    leaf.u.symbol.offset=0;
    return GenLeaf(&leaf);
}

unsigned NextLabel(void)
{
    LEAF leaf;
    InitLeaf(&leaf);
    leaf.leaf_class=0x11;
    leaf.u.words[1]=0xffffffffU;
    return GenLeaf(&leaf);
}

void AddLabelRef(unsigned label,unsigned quad)
{
    LEAF leaf;
    LeafGet(&leaf,label);
    leaf.u.words[1]=quad;
    LeafReplace(&leaf,label);
}

unsigned NextCaseLabel(unsigned value)
{
    LEAF leaf;
    InitLeaf(&leaf);
    leaf.leaf_class=0x11;
    leaf.u.words[1]=0xffffffffU;
    leaf.u.words[3]=value;
    return GenLeaf(&leaf);
}

/* Strings are materialized lazily when a code-generation operand is read. */
void LeafGetString(LEAFPTR leaf,unsigned index)
{
    LeafGet(leaf,index);
    if (leaf->leaf_class==0x10) Emit1String(leaf,index);
}
