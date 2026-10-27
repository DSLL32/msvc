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
* Description:  Sort a linked application list with its comparison callback.
*
****************************************************************************/


#include "cgmemory.h"
#include "stubs.h"
#include <recon.h>
void *DoSortList(void *list,unsigned next_offset,unsigned char (*before)(void *,void *),unsigned length);
void *SortList(void *list,unsigned next_offset,unsigned char (*before)(void *,void *))
{
    void *list2;
    unsigned length;
    unsigned char old_memout;
    list2=list;
    length=0;
    while (list2!=0) {
        ++length;
        list2=*(void **)((char *)list2+next_offset);
    }
    old_memout=SetMemOut(MO_OK);
    list=DoSortList(list,next_offset,before,length);
    SetMemOut(old_memout);
    return list;
}
void *MergeList(void *list,void *list2,unsigned next_offset,unsigned char (*before)(void *,void *));
void ShellSort(void **array,unsigned length,unsigned char (*before)(void *,void *));
void *BuildList(void **array,unsigned next_offset,unsigned length);
void *DoSortList(void *list,unsigned next_offset,unsigned char (*before)(void *,void *),unsigned length)
{
    void **array,**parray,*list2;
    unsigned mid;
    int i;
    if (list==0) return 0;
    array=CGAlloc(length*sizeof(void *));
    if (array==0) {
        mid=length/2;
        if (mid==0) return list;
        i=mid;
        list2=list;
        while (--i>=0) list2=*(void **)((char *)list2+next_offset);
        list=DoSortList(list,next_offset,before,mid);
        list2=DoSortList(list2,next_offset,before,length-mid);
        list=MergeList(list,list2,next_offset,before);
    } else {
        list2=list;
        parray=array;
        i=length;
        while (--i>=0) {
            *parray=list2;
            ++parray;
            list2=*(void **)((char *)list2+next_offset);
        }
        ShellSort(array,length,before);
        list=BuildList(array,next_offset,length);
        CGFreeSize(array,length*sizeof(void *));
    }
    return list;
}
void *MergeList(void *list,void *list2,unsigned next_offset,unsigned char (*before)(void *,void *)) { STUB_TRAP(); return 0; }
void ShellSort(void **array,unsigned length,unsigned char (*before)(void *,void *))
{
    unsigned i,gap,adjust;
    unsigned char swap;
    void *t;
    gap=length;
    adjust=1;
    do {
        adjust=!adjust;
        gap=gap/2+adjust;
        do {
            swap=0;
            for (i=0;i<length-gap;++i) {
                if (before(array[i+gap],array[i])) {
                    t=array[i];
                    array[i]=array[i+gap];
                    array[i+gap]=t;
                    swap=1;
                }
            }
        } while (swap);
    } while (gap!=1);
}
void *BuildList(void **array,unsigned next_offset,unsigned length)
{
    void *list,**owner;
    list=0;
    owner=&list;
    while (length!=0) {
        *owner=*array;
        *(void **)((char *)*owner+next_offset)=0;
        owner=(void **)((char *)*owner+next_offset);
        ++array;
        --length;
    }
    return list;
}
