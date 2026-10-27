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
* Description:  Compiler free-list allocation and accounting.
*
****************************************************************************/


#include "codegen.h"
#include "cgmemory.h"
#include "stubs.h"
#include <recon.h>

/* freelist.c before debug fill/alignment instrumentation. */
void *AllocFrl(void **head,int size)
{
    void *entry;
    if (*head==NULL) return CGAlloc(size);
    entry=*head;
    *head=*(void **)entry;
    FrlSize-=size;
    return entry;
}

void FrlFreeSize(void **head,void *entry,int size)
{
    FrlSize+=size;
    *(void **)entry=*head;
    *head=entry;
}

unsigned char FrlFreeAll(void **head,int size)
{
    void *junk;
    junk=NULL;
    while (*head!=NULL) {
        junk=*head;
        *head=*(void **)*head;
        CGFreeSize(junk,size);
        FrlSize-=size;
    }
    return junk!=NULL;
}
