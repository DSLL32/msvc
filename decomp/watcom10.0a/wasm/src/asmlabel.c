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


/* Adapted MakeLabel CF8d98..8ee9 from initial2002 asmlabel.c.
 *1994 has no structure-field branch, uses AsmError71 for redefinition,
 * and stores mem_type before GetSymInfo. */
#include <stdio.h>
#include <string.h>
#include "asmparse.h"
#include "direct.h"
#include <recon.h>

static unsigned AnonymousCounter; /* CF25f9c */

int MakeLabel(char *symbol_name, int mem_type)
{
    asm_sym *sym, *newsym;
    unsigned long addr;
    char buffer[20];
    if (strcmp(symbol_name,"@@") == 0) {
        sym = AsmGetSymbol("@B");
        if (sym != NULL) {
            sprintf(buffer,"L&_%d",AnonymousCounter++);
            AsmChangeName(sym->name,buffer);
        }
        sym = AsmLookup("@B");
        newsym = AsmGetSymbol("@F");
        if (newsym != NULL) {
            sym->fixup = newsym->fixup;
            newsym->fixup = NULL;
        }
        AsmTakeOut("@F");
        sym->state = SYM_INTERNAL;
        sym->mem_type = mem_type;
        GetSymInfo(sym);
        BackPatch(sym);
        sprintf(buffer,"L&_%d",AnonymousCounter + 1);
        sym = AsmGetSymbol(buffer);
        if (sym != NULL) AsmChangeName(sym->name,"@F");
        return NOT_ERROR;
    }
    sym = AsmLookup(symbol_name);
    if (sym == NULL) return ERROR;
    if (Parse_Pass == PASS_1) {
        if (sym->state != SYM_UNDEFINED) {
            AsmError(71);
            return ERROR;
        }
    } else {
        addr = sym->offset;
    }
    sym->state = SYM_INTERNAL;
    sym->mem_type = mem_type;
    GetSymInfo(sym);
    if (Parse_Pass != PASS_1 && sym->offset != addr) PhaseError = TRUE;
    BackPatch(sym);
    return NOT_ERROR;
}

/* CF8d40..8d95, reuse the actual AnonymousCounter owner from MakeLabel. */
void PrepAnonLabels(void)
{
    asm_sym *sym;
    char buffer[20];
    sprintf(buffer,"L&_%d",AnonymousCounter);
    AsmChangeName("@B",buffer);
    AnonymousCounter = 0;
    sym = AsmGetSymbol("L&_0");
    if (sym != NULL) AsmChangeName(sym->name,"@F");
}
