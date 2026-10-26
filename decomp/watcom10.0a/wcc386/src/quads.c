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


#define WCC_CSUICIDE_ABORTS
#include <string.h>
#include "compiler.h"
#include "quads.h"
#include <recon.h>

unsigned LeafFreeList;
/* Provisional descriptive names for the older quad-chain state. */
unsigned FirstQuad,LastStatementQuad,LastQuad,DefaultLeaf;
struct seg_info QuadSegs[49],LeafSegs[98];
void QuadInit(void)
{
    unsigned i;
    LEAF leaf;
    QuadIndex=0;
    FirstQuad=0;
    LastStatementQuad=0;
    LastQuad=0;
    LeafIndex=0;
    LeafFreeList=0;
    InitIntermediateFiles();
    for (i=0;i<49;++i) QuadSegs[i].allocated=0;
    for (i=0;i<98;++i) LeafSegs[i].allocated=0;
    InitDataQuads();
    InitLeaf(&leaf);
    leaf.leaf_class=8;
    leaf.leaf_type=GetType(TYPE_INT);
    GenLeaf(&leaf);
    InitLeaf(&leaf);
    leaf.leaf_class=19;
    leaf.leaf_type=GetType(TYPE_INT);
    DefaultLeaf=GenLeaf(&leaf);
    CompFlags.quad_init_flag=0;
}

struct int_file_info QuadFileInfo,LeafFileInfo;
void InitIntermediateFiles(void)
{
    LeafFileInfo.seg_table=LeafSegs;
    InitFileInfo(&QuadFileInfo,QuadSegs,12);
    InitFileInfo(&LeafFileInfo,LeafSegs,sizeof(LEAF));
}

void InitFileInfo(struct int_file_info *info,struct seg_info *segments,unsigned item_size)
{
    info->seg_table=segments;
    info->size_of_item=item_size;
    info->items_per_buf=512/item_size;
    info->curr_buf_num=0;
    info->last_buf_num=0;
    info->curr_seg_num=0;
    info->curr_buf_seg=0;
    info->curr_buf_dirty=0;
    info->buffer=CPermAlloc(512);
}

void InitLeaf(LEAFPTR leaf)
{
    memset(leaf,0,sizeof(LEAF));
}

unsigned GenLeaf(LEAFPTR leaf)
{
    unsigned index=NewLeaf();
    LeafReplace(leaf,index);
    return index;
}

extern int ReclaimCount;
unsigned NewLeaf(void)
{
    unsigned index;
    LEAF leaf;
    if (LeafFreeList) {
        index=LeafFreeList;
        LeafGet(&leaf,index);
        ++ReclaimCount;
        LeafFreeList=leaf.payload[1];
    } else {
        if (LeafIndex>=0xfffb) {
            CErr1(0x479);
            CSuicide();
        }
        index=++LeafIndex;
    }
    return index;
}

void *AccessFileInfo(unsigned index,struct int_file_info *info)
{
    unsigned buf_num=index/info->items_per_buf;
    unsigned seg_num;
    struct seg_info *si;
    if (buf_num!=info->curr_buf_num) {
        if (info->curr_buf_dirty) {
            si=&info->seg_table[info->curr_seg_num];
            if (!si->allocated) info->curr_buf_seg=AllocSegment(si);
            if (info->curr_buf_seg) {
                memcpy((char *)info->curr_buf_seg+(info->curr_buf_num%32)*512,
                       info->buffer,512);
            } else {
                PageSeek(si->index*0x4000+(info->curr_buf_num%32)*512);
                PageWrite(info->buffer,512);
            }
            info->curr_buf_dirty=0;
        }
        seg_num=buf_num/32;
        si=&info->seg_table[seg_num];
        info->curr_buf_seg=AccessSegment(si);
        if (buf_num<=info->last_buf_num) {
            if (info->curr_buf_seg) {
                memcpy(info->buffer,(char *)info->curr_buf_seg+(buf_num%32)*512,512);
            } else {
                PageSeek(si->index*0x4000+(buf_num%32)*512);
                PageRead(info->buffer,512);
            }
        } else {
            info->last_buf_num=buf_num;
            memset(info->buffer,0,512);
        }
        info->curr_buf_num=buf_num;
        info->curr_seg_num=seg_num;
    }
    return info->buffer+(index%info->items_per_buf)*info->size_of_item;
}
