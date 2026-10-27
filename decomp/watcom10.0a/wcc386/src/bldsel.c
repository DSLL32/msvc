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
* Description:  Construct the older switch-selection case list.
*
****************************************************************************/


#include "bldsel.h"
#include "cgmemory.h"
#include "codegen.h"
#include "address.h"
#include "stubs.h"
#include <recon.h>

select_node *BGSelInit(void)
{
    select_node *node;
    node=CGAlloc(sizeof(*node));
    node->num_cases=0;
    node->other_wise=NULL;
    node->list=NULL;
    return node;
}

void BGSelCase(select_node *node,struct code_lbl *label,long value)
{
    BGSelRange(node,value,value,label);
}

select_list *NewCase(long low,long high,struct code_lbl *label);
void BGSelRange(select_node *node,long low,long high,struct code_lbl *label)
{
    select_list *entry;
    if ((low^high)<0) Zoiks(89);
    entry=NewCase(low,high,label);
    entry->next=node->list;
    node->list=entry;
    node->num_cases+=(unsigned)high-(unsigned)low+1;
}
select_list *NewCase(long low,long high,struct code_lbl *label)
{
    select_list *entry;
    entry=CGAlloc(sizeof(*entry));
    entry->next=NULL;
    entry->low=low;
    entry->high=high;
    entry->label=label;
    entry->count=(unsigned)high-(unsigned)low+1;
    return entry;
}

void BGSelOther(select_node *node,struct code_lbl *label) { node->other_wise=label; }

static type_def *SortTipe;
void SortNodeList(address_name *value,select_node *node,unsigned char is_signed);
long ScanCost(select_node *node);
long JumpCost(select_node *node);
long DistinctIfCost(select_node *node);
void MergeListEntries(select_node *node);
address_name *GenScanTable(address_name *value,select_node *node,type_def *type);
address_name *GenSelTable(address_name *value,select_node *node,type_def *type);
address_name *GenIfStmts(address_name *value,select_node *node,type_def *type);
void FreeSelectNode(select_node *node);
/* The old selector always considers all three forms under both signednesses,
 * before the later CG_SWITCH_* restriction parameter. */
void BGSelect(select_node *node,address_name *value)
{
    long cost,best;
    unsigned char kind;
    value=Arithmetic(value,TypeInteger);
    if (node->num_cases!=0) {
        SortNodeList(value,node,1);
        best=ScanCost(node);
        kind=3; /* S_SCAN. */
        cost=JumpCost(node);
        if (cost<=best) { best=cost;kind=4; }
        cost=DistinctIfCost(node);
        if (cost<=best) { best=cost;kind=5; }
        SortNodeList(value,node,0);
        cost=ScanCost(node);
        if (cost<=best) { best=cost;kind=0; }
        cost=JumpCost(node);
        if (cost<=best) { best=cost;kind=1; }
        cost=DistinctIfCost(node);
        if (cost<=best) kind=2;
        if (kind>=3 && kind<=5) SortNodeList(value,node,1);
        value=BGConvert(value,SortTipe);
        /* A real add-zero temporary ensures volatile selectors are loaded
         * just once, before choosing/testing their dispatch representation. */
        value=BGBinary(1,value,BGInteger(0,SortTipe),SortTipe,1);
        MergeListEntries(node);
        switch (kind) {
        case 0:case 3: value=GenScanTable(value,node,SortTipe);break;
        case 1:case 4: value=GenSelTable(value,node,SortTipe);break;
        case 2:case 5: value=GenIfStmts(value,node,SortTipe);break;
        }
    } else if (node->other_wise!=NULL) BGControl(0x51,NULL,node->other_wise);
    BGDone(value);
    FreeSelectNode(node);
}
type_def *SelNodeType(address_name *value,unsigned char is_signed);
unsigned char NodeLess(void *left,void *right);
void *SortList(void *list,unsigned next_offset,unsigned char (*before)(void *,void *));
void SortNodeList(address_name *value,select_node *node,unsigned char is_signed)
{
    select_list *entry;
    SortTipe=SelNodeType(value,is_signed);
    entry=SortList(node->list,offsetof(select_list,next),NodeLess);
    node->list=entry;
    node->lower=entry->low;
    while (entry->next!=NULL) entry=entry->next;
    node->upper=entry->high;
}
type_def *SelNodeType(address_name *value,unsigned char is_signed)
{
    cg_type signed_type,unsigned_type;
    switch (value->tipe->length) {
    case 1: signed_type=T_INT_1;unsigned_type=T_UINT_1;break;
    case 4: signed_type=T_INT_4;unsigned_type=T_UINT_4;break;
    default:signed_type=T_INT_2;unsigned_type=T_UINT_2;break;
    }
    return TypeAddress(is_signed?signed_type:unsigned_type);
}
int SelCompare(long left,long right);
/* SortList's generic pointer transport retains the actual two case owners. */
unsigned char NodeLess(void *left,void *right)
{
    select_list *first,*second;
    first=left;
    second=right;
    return SelCompare(first->low,second->low)<0;
}
int SelCompare(long left,long right)
{
    if (left==right) return 0;
    if (SortTipe->attr&2) {
        if (left<right) return -1;
    } else if ((unsigned long)left<(unsigned long)right) return -1;
    return 1;
}


long IfCost(select_node *node,int entries);
long DistinctIfCost(select_node *node)
{
    select_list *entry,*next;
    int entries;
    entries=1;
    entry=node->list;
    next=entry->next;
    while (next!=NULL) {
        if ((unsigned long)entry->high+1!=(unsigned long)next->low ||
            entry->label!=next->label) {
            ++entries;
            entry=next;
        }
        next=next->next;
    }
    return IfCost(node,entries);
}
void MergeListEntries(select_node *node)
{
    select_list *entry,*next;
    entry=node->list;
    next=entry->next;
    while (next!=NULL) {
        if ((unsigned long)entry->high+1==(unsigned long)next->low &&
            entry->label==next->label) {
            entry->high=next->high;
            entry->next=next->next;
            CGFreeSize(next,sizeof(*next));
        } else entry=entry->next;
        next=entry->next;
    }
}
address_name *GenScanTable(address_name *value,select_node *node,type_def *type) { STUB_TRAP();return NULL; }
address_name *GenSelTable(address_name *value,select_node *node,type_def *type) { STUB_TRAP();return NULL; }
void DoBinarySearch(address_name *value,select_list *list,type_def *type,
    int low,int high,struct code_lbl *other,long lower,long upper,
    unsigned char have_lower,unsigned char have_upper);
address_name *GenIfStmts(address_name *value,select_node *node,type_def *type)
{
    select_list *entry;
    int count;
    count=0;
    for (entry=node->list;entry!=NULL;entry=entry->next) ++count;
    DoBinarySearch(value,node->list,type,0,count-1,node->other_wise,0,0,0,0);
    return value;
}
void DoBinarySearch(address_name *value,select_list *list,type_def *type,
    int low,int high,struct code_lbl *other,long lower,long upper,
    unsigned char have_lower,unsigned char have_upper)
{
    int mid,num;
    select_list *entry;
    bool_node *cmp;
    struct code_lbl *less;
    mid=low+(high-low)/2;
    entry=list;
    num=mid;
    while (--num>=0) entry=entry->next;
    if (low==high) {
        if (have_lower && lower==entry->low &&
            have_upper && upper==entry->high) {
            BGControl(0x51,NULL,entry->label);
            return;
        } else if (entry->low==entry->high) {
            cmp=BGCompare(0x30,BGDuplicate(value),
                          BGInteger(entry->low,type),NULL,type);
            BGControl(0x53,cmp,entry->label);
            BGControl(0x51,NULL,other);
            return;
        }
    }
    less=AskForNewLabel();
    if (!have_lower || SelCompare(lower,entry->low)<0) {
        if (have_upper && SelCompare(upper,entry->low)<0) {
            BGControl(0x51,NULL,less);
        } else {
            cmp=BGCompare(0x34,BGDuplicate(value),
                          BGInteger(entry->low,type),NULL,type);
            BGControl(0x53,cmp,less);
        }
    }
    if (!have_lower || SelCompare(lower,entry->high)<=0) {
        if (have_upper && SelCompare(upper,entry->high)<=0) {
            BGControl(0x51,NULL,entry->label);
        } else {
            cmp=BGCompare(0x33,BGDuplicate(value),
                          BGInteger(entry->high,type),NULL,type);
            BGControl(0x53,cmp,entry->label);
        }
    }
    if (mid<high) {
        DoBinarySearch(value,list,type,mid+1,high,other,
            (long)((unsigned long)entry->high+1),upper,1,have_upper);
    } else if (other!=NULL) BGControl(0x51,NULL,other);
    BGControl(0x56,NULL,less);
    if (low<mid) {
        DoBinarySearch(value,list,type,low,mid-1,other,lower,
            (long)((unsigned long)entry->low-1),have_lower,1);
    } else if (other!=NULL) BGControl(0x51,NULL,other);
}
void FreeSelectNode(select_node *node)
{
    select_list *entry,*next;
    entry=node->list;
    while (entry!=NULL) {
        next=entry->next;
        CGFreeSize(entry,sizeof(*entry));
        entry=next;
    }
    CGFreeSize(node,sizeof(*node));
}

cg_type SelType(unsigned long range)
{
    cg_type type;
    if (!(range&0xffff0000UL)) {
        if (!(range&0xff00UL)) type=T_UINT_1;
        else type=T_UINT_2;
    } else type=T_UINT_4;
    if (type>SortTipe->refno) {
        switch (SortTipe->refno) {
        case T_UINT_1:case T_INT_1:type=T_UINT_1;break;
        case T_UINT_2:case T_INT_2:type=T_UINT_2;break;
        case T_UINT_4:case T_INT_4:type=T_UINT_4;break;
        }
    }
    return type;
}
