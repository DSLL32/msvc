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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Adapted from bld/sdk/rc/wres/c/ralliae.c, initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CF215c0..215e5. */
#include "wasm.h"
#include <recon.h>

typedef struct ss { struct ss *next, *prev; } ss;

void ResAddLLItemAtEnd(void **head, void **tail, void *newnode)
{
    ss **headptr, **tailptr, *itemptr;
    headptr = (ss **)head;
    tailptr = (ss **)tail;
    itemptr = (ss *)newnode;
    if (*headptr == NULL) {
        *headptr = *tailptr = itemptr;
        itemptr->next = NULL;
        itemptr->prev = NULL;
    } else {
        itemptr->prev = *tailptr;
        itemptr->next = NULL;
        (*tailptr)->next = itemptr;
        *tailptr = itemptr;
    }
}
