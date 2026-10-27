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
* Description:  Initialize register costs and weight savings by loop depth.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
/* Locally adapted from the sibling's recovered source. ActualWPP
 * InitWeights71bfd and85290/852c4/85314 helpers independently bind the
 * same old11-class costs and292B savings record; no sibling build input. */
#include "codegen.h"
#include "regsave.h"
#include <recon.h>
struct savings Save;
#define COST(size_cost,time_cost) (((size_cost)*size+(time_cost)*time)>>8)

void InitWeights(unsigned size)
{
    unsigned time;
    AdjTimeSize(&time,&size);
    SetLoopCost(time);
    if ((CGProcessorVersion&15)>=4) {
        SetCost(Save.load_cost,COST(3,1));
        SetCost(Save.store_cost,COST(3,1));
        SetCost(Save.use_save,COST(1,1));
        SetCost(Save.def_save,COST(1,1));
        SetCost(Save.push_cost,COST(1,1));
        SetCost(Save.pop_cost,COST(1,1));
    } else if ((CGProcessorVersion&15)>=3) {
        SetCost(Save.load_cost,COST(3,4));
        SetCost(Save.store_cost,COST(3,2));
        SetCost(Save.use_save,COST(1,2));
        SetCost(Save.def_save,COST(1,2));
        SetCost(Save.push_cost,COST(1,2));
        SetCost(Save.pop_cost,COST(1,4));
    } else {
        SetCost(Save.load_cost,COST(3,17));
        SetCost(Save.store_cost,COST(3,18));
        SetCost(Save.use_save,COST(1,15));
        SetCost(Save.def_save,COST(1,18));
        SetCost(Save.push_cost,COST(1,11));
        SetCost(Save.pop_cost,COST(1,8));
    }
    Save.index_save=Save.load_cost[4]; /* WD in the old type-class order. */
}

void AdjTimeSize(unsigned *time,unsigned *size)
{
    if (*size>100) *size=256;
    else *size=(*size<<8)/100;
    *time=256-*size;
}

void SetLoopCost(unsigned time)
{
    unsigned weight;
    int i;
    weight=(20*time)>>8;
    if (weight==0) weight=1;
    Save.loop_weight[0]=1;
    for (i=1;i<=5;++i) Save.loop_weight[i]=Save.loop_weight[i-1]*weight;
}

void SetCost(save_def *cost,save_def value)
{
    int i;
    for (i=0;i<SAVE_CLASS_COUNT;++i) cost[i]=value;
}

