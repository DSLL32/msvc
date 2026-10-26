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


/* Adapted from initial6b9cb443 bld/plusplus/c/gstack.c.
 * Actual1994 CF3c679..3c690 creates108-byte GLOBAL_STACK records,
 * sixteen per block. The discriminated stack payload is recovered when used. */
#include "compiler.h"
#include "carve.h"
#include "gstack.h"
#include "pending.h"
#include <recon.h>
static carve_t carveGLOBAL_STACK;
void gstackInit(INITFINI *defn)
{
    (void)defn;
    carveGLOBAL_STACK = CarveCreate(108, 16);
}

void StackPush(void **head,void *entry);
GLOBAL_STACK *GStackPush(GLOBAL_STACK **head,unsigned char id)
{
    GLOBAL_STACK *entry=(GLOBAL_STACK *)CarveAlloc(carveGLOBAL_STACK);
    entry->id=id;
    StackPush((void **)head,entry);
    return entry;
}

void *StackPop(void **head);
void PTypePop(DECL_SPEC *spec);
void DataInitFinish(void *data)
{ (void)data; STUB_TRAP(); }
void TemplateDeclFini(void);
void GStackPop(GLOBAL_STACK **head)
{
    GLOBAL_STACK *top=StackPop((void **)head);
    switch (top->id) {
    case 1: PTypePop(top->u.dspec); break;
    case 4: ClassPop(&top->u.classdata); break;
    case 5: DataInitFinish(&top->u); break;
    case 6: TemplateDeclFini(); break;
    }
    CarveFree(carveGLOBAL_STACK,top);
}
