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

/* Adapted from bld/wasm/c/expand.c, initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9; CF19a1c..19a41.
 * createconstant's fifth bool argument is the original callee-cleaned stack
 * argument after EAX/EDX/EBX/ECX. Native uses the authorized GCC ABI. */
#include <string.h>
#include "asminit.h"
#include <recon.h>

#ifdef __WATCOMC__
/* Shipped bodies expand these string operations in place. */
#pragma intrinsic(strlen,strcpy)
#endif

int StoreConstant(char *name, char *value, signed char redefine)
{
    AsmScan(value, StringBuf);
    return createconstant(name, 0, 0, redefine, 0);
}

/* CF19ae4..19e2f. No later '$' location-label expansion or extra_value field. */
int createconstant(char *name, int value, int start, signed char redefine,
                   unsigned char expand_early)
{
    struct asm_tok *new;
    dir_node *dir;
    asm_sym *sym;
    int i, count;
    sym = AsmGetSymbol(name);
    if (sym == NULL) {
        dir = dir_insert(name, TAB_CONST);
        if (dir == NULL) return -1;
        dir->e.constinfo->redefine = redefine;
        dir->e.constinfo->expand_early = expand_early;
        dir->e.constinfo->data = NULL;
        dir->e.constinfo->count = 0;
        dir->sym.state = SYM_UNDEFINED;
    } else {
        dir = (dir_node *)sym;
        if (sym->state == SYM_UNDEFINED) {
            dir->e.constinfo = AsmAlloc(sizeof(const_info));
            dir->e.constinfo->redefine = redefine;
            dir->e.constinfo->expand_early = expand_early;
            sym->grpidx = sym->segidx = sym->offset = 0;
            dir->e.constinfo->data = NULL;
        } else if (sym->state != SYM_CONST
                   || (!dir->e.constinfo->redefine && Parse_Pass == PASS_1)) {
            AsmError(1049); /* LABEL_ALREADY_DEFINED */
            return -1;
        }
    }
    if (value) {
        new = AsmAlloc(sizeof(*new));
        new[0].token = T_NUM;
        new[0].value = 1;
        new[0].string_ptr = NULL;
        if (dir->e.constinfo->data != NULL) {
            FreeInfo(dir);
            dir->e.constinfo = AsmAlloc(sizeof(const_info));
            dir->e.constinfo->redefine = redefine;
            dir->e.constinfo->expand_early = expand_early;
        }
        dir->e.constinfo->count = 1;
        dir->e.constinfo->data = new;
        dir->sym.state = SYM_CONST;
        return 1;
    }
    if (ExpandTheWorld(start, 0) == -1) return -1;
    for (i = start; AsmBuffer[i]->token != T_FINAL; ++i) ;
    count = i - start;
    new = AsmAlloc(count * sizeof(*new));
    for (i = 0; i < count; ++i) {
        if (AsmBuffer[start+i]->token == T_STRING
            && AsmBuffer[start+i]->value == 0) {
            --i;
            --count;
            continue;
        }
        new[i].token = AsmBuffer[start+i]->token;
        new[i].value = AsmBuffer[start+i]->value;
        if (AsmBuffer[start+i]->string_ptr == NULL) {
            new[i].string_ptr = NULL;
        } else {
            new[i].string_ptr = AsmAlloc(strlen(AsmBuffer[start+i]->string_ptr) + 1);
            strcpy(new[i].string_ptr, AsmBuffer[start+i]->string_ptr);
        }
    }
    if (dir->e.constinfo->data != NULL) {
        FreeInfo(dir);
        dir->e.constinfo = AsmAlloc(sizeof(const_info));
        dir->e.constinfo->redefine = redefine;
        dir->e.constinfo->expand_early = expand_early;
    }
    dir->e.constinfo->count = count;
    dir->e.constinfo->data = new;
    dir->sym.state = SYM_CONST;
    return 1;
}

/* CF19e30..19e6d. Token_Count is the actual shared expression-line count;
 * StoreConstant does not assign the separate AsmScan return value to it. */
int Token_Count;
int ExpandTheWorld(int start_pos, unsigned char early_only)
{
    int val;
    if (ExpandAllConsts(start_pos, early_only) == -1) return -1;
    if (!early_only) {
        val = EvalExpr(Token_Count, start_pos, Token_Count);
        if (val == -1) val = 0;
        Token_Count = val;
    }
    return 1;
}

/* CF19e70..19ee0. No later Globals.expand_count field exists in this body. */
int ExpandAllConsts(int start_pos, unsigned char early_only)
{
    int i;
    if (AsmBuffer[start_pos + 1]->token == T_DIRECTIVE) {
        switch (AsmBuffer[start_pos + 1]->value) {
        case T_EQU:
        case T_EQU2:
        case T_TEXTEQU:
            start_pos += 2;
        }
    }
    for (i = start_pos; AsmBuffer[i]->token != T_FINAL; ++i) {
        if (AsmBuffer[i]->token != T_ID) continue;
        switch (ExpandSymbol(i, early_only)) {
        case -1: return -1;
        case 2: --i; break;
        }
    }
    return 1;
}
