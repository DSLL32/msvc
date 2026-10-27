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
* Description:  Estimate and emit the older i386 switch dispatch forms.
*
****************************************************************************/


#include "bldsel.h"
#include "codegen.h"
#include "stubs.h"
#include <recon.h>

int SelCompare(long left,long right);
cg_type SelType(unsigned long range);
long Balance(long size,long time);
long ScanCost(select_node *node)
{
    select_list *entry;
    long high,low,values;
    cg_type type;
    high=node->upper;
    low=node->lower;
    values=0;
    for (entry=node->list;entry!=NULL;entry=entry->next) {
        if (SelCompare(entry->low,high)>0) break;
        values=(long)((unsigned long)values+(unsigned long)entry->high-
                      (unsigned long)entry->low+1);
    }
    type=SelType((unsigned long)high-(unsigned long)low);
    if ((type==T_UINT_4 && values<5) || (type!=T_UINT_4 && values<7))
        return 0x7fffffffL;
    return Balance((long)((unsigned long)values*(4+TypeAddress(type)->length)),
                   values/2);
}
extern unsigned char OptForSize;
long Balance(long size,long time)
{
    unsigned char weight;
    long cost_size,cost_time;
    weight=OptForSize;
    if (weight<25) weight=25;
    /* Preserve the original low-DWORD products and signed overflow tests. */
    cost_size=(long)((unsigned long)size*weight);
    if (cost_size<0) return 0x7fffffffL;
    cost_time=(long)((unsigned long)time*10U*(100U-weight));
    if (cost_time<0) return 0x7fffffffL;
    return (long)((unsigned long)cost_time+(unsigned long)cost_size)/100;
}

long JumpCost(select_node *node)
{
    long range;
    range=(long)((unsigned long)node->upper-(unsigned long)node->lower+1);
    if (range>0x20c49bL || range<0) range=0x20c49bL;
    if (node->num_cases<4 || range<4) return 0x7fffffffL;
    return Balance((long)((unsigned long)range*4),1);
}

/* Actual i386 compare encodings by operand width, matching released i86sel.c. */
static unsigned char CmpSize[]={0,3,5,0,6};
long IfCost(select_node *node,int entries)
{
    long cost;
    int log_entries;
    unsigned type_length;
    type_length=TypeAddress(SelType((unsigned long)node->upper-
                                   (unsigned long)node->lower))->length;
    cost=entries>20?6:2;
    cost+=CmpSize[type_length];
    cost=(long)((unsigned long)cost*(unsigned)entries);
    log_entries=0;
    while (entries!=0) {
        ++log_entries;
        entries=(unsigned)entries>>2;
    }
    cost=Balance(cost,log_entries);
    if (cost>=0x7fffffffL) cost=0x7ffffffeL;
    return cost;
}
