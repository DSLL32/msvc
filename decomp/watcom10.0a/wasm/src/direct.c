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

/* Adapted from bld/wasm/c/direct.c at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9, CFfa3c..fce6.
 * The shipped compiler inlines dir_add; retain those operations in this body.
 * Allocation layouts and switch IDs are the 1994 layouts, not newer additions. */
#include <string.h>
#include "asminit.h"
#include <recon.h>

#ifdef __WATCOMC__
/* Shipped bodies expand these string operations in place. */
#pragma intrinsic(strlen,strcpy)
#endif

typedef struct symbol_queue { dir_node *head, *tail; } symbol_queue;
symbol_queue Tables[TAB_LAST];
unsigned LnamesIdx;
static unsigned grpdefidx;
unsigned LineNumber;

dir_node *dir_insert(char *name, int tab)
{
    dir_node *new;
    asm_sym *sym;
    new = AsmAlloc(sizeof(dir_node));
    new->next = NULL;
    new->line = LineNumber;
    new->sym.name = AsmAlloc(strlen(name) + 1);
    strcpy(new->sym.name, name);
    new->next = new->prev = NULL;
    if (tab != TAB_CLASS_LNAME) sym = AsmAdd((asm_sym *)new);
    else sym = (asm_sym *)new;
    if (name == NULL) InternalError("..\\c\\direct.c", 412);
    switch (tab) {
    case TAB_SEG:
        new->e.seginfo = AsmAlloc(sizeof(seg_info));
        new->e.seginfo->lname_idx = 0;
        break;
    case TAB_GRP:
        new->e.grpinfo = AsmAlloc(sizeof(grp_info));
        new->e.grpinfo->idx = ++grpdefidx;
        new->e.grpinfo->seglist = NULL;
        new->e.grpinfo->numseg = 0;
        new->e.grpinfo->lname_idx = 0;
        break;
    case TAB_EXT:
        new->e.extinfo = AsmAlloc(sizeof(ext_info));
        break;
    case TAB_COMM:
        new->e.comminfo = AsmAlloc(sizeof(comm_info));
        new->e.comminfo->size = 1;
        break;
    case TAB_CONST:
        sym->state = SYM_CONST;
        sym->grpidx = sym->segidx = sym->offset = 0;
        new->e.constinfo = AsmAlloc(sizeof(const_info));
        return new;
    case TAB_PROC:
        new->e.procinfo = AsmAlloc(sizeof(proc_info));
        new->e.procinfo->regslist = NULL;
        new->e.procinfo->paralist = NULL;
        new->e.procinfo->locallist = NULL;
        break;
    case TAB_MACRO:
        new->e.macroinfo = AsmAlloc(sizeof(macro_info));
        new->e.macroinfo->parmlist = NULL;
        new->e.macroinfo->data = NULL;
        new->e.macroinfo->filename = NULL;
        new->e.macroinfo->start_line = LineNumber;
        break;
    case TAB_CLASS_LNAME:
    case TAB_LNAME:
        sym->state = tab == TAB_LNAME ? SYM_LNAME : SYM_CLASS_LNAME;
        new->e.lnameinfo = AsmAlloc(sizeof(lname_info));
        new->e.lnameinfo->idx = LnamesIdx;
        return new;
    case TAB_PUB:
        sym->state = SYM_UNDEFINED;
        sym->public = 1;
        return new;
    case TAB_GLOBAL:
        sym->state = SYM_UNDEFINED;
        return new;
    case TAB_STRUCT:
        sym->state = SYM_STRUCT;
        new->e.structinfo = AsmAlloc(sizeof(struct_info));
        new->e.structinfo->size = 0;
        new->e.structinfo->alignment = 0;
        new->e.structinfo->head = NULL;
        new->e.structinfo->tail = NULL;
        sym->total_size = sym->total_length = 0;
        sym->first_size = sym->first_length = 0;
        return new;
    }
    if (Tables[tab].head == NULL) {
        Tables[tab].head = Tables[tab].tail = new;
        new->prev = new->next = NULL;
    } else {
        new->prev = Tables[tab].tail;
        Tables[tab].tail->next = new;
        Tables[tab].tail = new;
        new->next = NULL;
    }
    return new;
}
