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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

/* Initial Open Watcom rtngen.c, commit6b9cb443, adapted to the 1994
 * initializer: it does not clear the two pending-work heads here. */
#include "compiler.h"
#include "carve.h"
#include "symbol.h"
#include "scope.h"
#include "pending.h"
#include <recon.h>

typedef struct routine_gen {
    struct routine_gen *next;
    unsigned index;
    void *parm;
} RTN_GEN;
static carve_t carveRTN_GEN;

void rtnGenInit(INITFINI *defn)
{
    (void)defn;
    carveRTN_GEN = CarveCreate(sizeof(RTN_GEN), 16);
}

static RTN_GEN *useSYMBOL,*useTYPE;
RTN_GEN *findGenRoutine(RTN_GEN **head,unsigned index,void *parm)
{
    RTN_GEN *entry;
    for (entry=*head;entry;entry=entry->next)
        if (entry->index==index && entry->parm==parm) return entry;
    return 0;
}
void addGenRoutine(RTN_GEN **head,unsigned index,void *parm)
{
    RTN_GEN *entry=CarveAlloc(carveRTN_GEN);
    entry->index=index;
    entry->parm=parm;
    entry->next=*head;
    *head=entry;
}
void RtnGenAddSymbol(unsigned char index,SYMBOL parm)
{
    if (!findGenRoutine(&useSYMBOL,index,parm)) addGenRoutine(&useSYMBOL,index,parm);
}
void RtnGenAddType(unsigned char index,TYPE parm)
{
    if (!findGenRoutine(&useTYPE,index,parm)) addGenRoutine(&useTYPE,index,parm);
}

/* The encountered 1994 dispatch tables contain two SYMBOL and four TYPE
 * callbacks, in the order retained by the released rtnglist.h. */
void ClassDefineRefdDefaults(void);
void ScopeEmitIndexMappings(void);
void RtnGenCallBackRunTimeTableThunk(SYMBOL);
void RtnGenCallBackGenThunk(SYMBOL);
void RtnGenCallBackArrayDtor(TYPE);
void RtnGenCallBackCdoptAccCtor(TYPE);
void RtnGenCallBackCdoptAccDtor(TYPE);
void RtnGenCallBackCdoptAccOpEq(TYPE);
static void (*execSYMBOL[])(SYMBOL) = {
    RtnGenCallBackRunTimeTableThunk, RtnGenCallBackGenThunk
};
static void (*execTYPE[])(TYPE) = {
    RtnGenCallBackArrayDtor, RtnGenCallBackCdoptAccCtor,
    RtnGenCallBackCdoptAccDtor, RtnGenCallBackCdoptAccOpEq
};
void RtnGenerate(void)
{
    RTN_GEN *entry;
    ClassDefineRefdDefaults();
    ScopeEmitIndexMappings();
    while (useSYMBOL) {
        entry=useSYMBOL;
        useSYMBOL=entry->next;
        (*execSYMBOL[entry->index])(entry->parm);
        CarveFree(carveRTN_GEN,entry);
    }
    while (useTYPE) {
        entry=useTYPE;
        useTYPE=entry->next;
        (*execTYPE[entry->index])(entry->parm);
        CarveFree(carveRTN_GEN,entry);
    }
    CurrScope=FileScope;
}
