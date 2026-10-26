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

/* Adapted from bld/wasm/c/insthash.c, initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CFba18..ba37. */
#include <string.h>
#include "asminit.h"
#include <recon.h>

void make_inst_hash_table(void)
{
    unsigned short i;
    for (i = 0; i != T_NULL; ++i) {
        add(&AsmOpcode[i]);
    }
}

/* CFb98c..b9f6; the original file-local name remains visible for the MAP. */
struct AsmCodeName *add(struct AsmCodeName *inst)
{
    struct AsmCodeName **location;
    char buffer[20];
    strncpy(buffer, &AsmChars[inst->index], inst->len);
    buffer[inst->len] = '\0';
    location = find(buffer);
    if (*location != NULL) {
        AsmError(71); /* SYMBOL_ALREADY_DEFINED */
        return NULL;
    }
    inst->next = *location;
    *location = inst;
    return inst;
}

struct AsmCodeName *inst_table[211] = { NULL };

/* CFb91c..b98c: same-name lookup through the mnemonic chains. */
struct AsmCodeName **find(char *name)
{
    struct AsmCodeName **inst;
    inst = &inst_table[hashpjw(name)];
    for (; *inst; inst = &(*inst)->next) {
        if (strnicmp(name, &AsmChars[(*inst)->index], (*inst)->len) == 0
            && name[(*inst)->len] == '\0') {
            return inst;
        }
    }
    return inst;
}

/* CFb8d0..b919. The shipped body masks g with0xf000, rotates it left4
 * (D3C0 withCL4, 32-bit), XORs h and then masks h with0xfff. The rotated
 * term has no low12 bits. The later source's g>>12 fold differs, so retain
 * the actual rolling12-bit result rather than importing that newer fold. */
unsigned int hashpjw(char *name)
{
    unsigned h;
    for (h = 0; *name; ++name) {
        h = ((h << 4) + ((unsigned char)*name | ' ')) & 0x0fff;
    }
    return h % 211;
}

/* CFb9f8..ba15: the name table stores the instruction-row position. */
int get_instruction_position(char *string)
{
    struct AsmCodeName **inst = find(string);
    if (*inst != NULL) return (*inst)->position;
    return -2;
}
