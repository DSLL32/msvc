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

/* Adapted from bld/wasm/c/asmsym.c, initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CF8b04..8b0c. */
#include <string.h>
/* The released hash.h was included privately in two translation units. Keep
 * its known source name while qualifying this separate physical linker body;
 * the original two hashpjw bodies must remain distinct graph vertices. */
#define hashpjw asmsym_hashpjw
#include "asminit.h"
#include "asmparse.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(strlen,strcpy)
#endif

asm_sym *AsmGetSymbol(char *name)
{
    asm_sym **sym_ptr;
    sym_ptr = AsmFind(name);
    return *sym_ptr;
}

asm_sym *sym_table[211] = { NULL };

/* CF883c..8872. */
asm_sym **AsmFind(char *name)
{
    asm_sym **sym;
    sym = &sym_table[hashpjw(name)];
    for (; *sym; sym = &(*sym)->next) {
        if (stricmp(name, (*sym)->name) == 0) return sym;
    }
    return sym;
}

/* Distinct private hash.h copy, CF87f0..8839, byte-identical to CFb8d0 body.
 * The actual rotate/mask leaves the low12-bit recurrence used below. */
unsigned int hashpjw(char *name)
{
    unsigned h;
    for (h = 0; *name; ++name) {
        h = ((h << 4) + ((unsigned char)*name | ' ')) & 0x0fff;
    }
    return h % 211;
}

/* CF8a64..8ad5; CvtTable values are the shipped 1994 token IDs. */
unsigned short CvtTable[] = {
    T_BYTE, T_WORD, T_DWORD, T_DWORD, T_DWORD, T_QWORD,
    T_TBYTE, T_NEAR, T_NEAR, T_FAR, T_FAR
};
asm_sym *AsmAdd(asm_sym *sym)
{
    asm_sym **location;
    location = AsmFind(sym->name);
    if (*location != NULL) {
        AsmError(71);
        return NULL;
    }
    sym->offset = 0;
    sym->public = 0;
    sym->fixup = NULL;
    sym->mangler = NULL;
    sym->next = *location;
    *location = sym;
    sym->state = AsmQueryExternal(sym->name);
    if (sym->state != SYM_UNDEFINED) {
        sym->mem_type = CvtTable[AsmQueryType(sym->name)];
    } else {
        sym->mem_type = 0;
    }
    return sym;
}

/* CF8874..89e8; the encountered allocation is a full52-byte dir_node.
 * The '$' location symbol retains its special state and current address. */
asm_sym *AsmLookup(char *name)
{
    asm_sym **location;
    asm_sym *sym;
    char is_current_loc = 0;
    if (strlen(name) > 247) { AsmError(136); return NULL; }
    location = AsmFind(name);
    if (strcmp(name, "$") == 0) {
        is_current_loc = 1;
        if (*location != NULL) GetSymInfo(*location);
    }
    if (*location != NULL) return *location;
    sym = AsmAlloc(sizeof(dir_node));
    if (sym != NULL) {
        sym->grpidx = sym->segidx = 0;
        sym->public = 0;
        if (is_current_loc) {
            GetSymInfo(sym);
            sym->state = 1; /* SYM_INTERNAL */
        } else sym->offset = 0;
        sym->first_size = sym->first_length = 0;
        sym->total_size = sym->total_length = 0;
        sym->mangler = NULL;
        sym->fixup = NULL;
        sym->next = *location;
        *location = sym;
        sym->name = AsmAlloc(strlen(name) + 1);
        strcpy(sym->name, name);
        if (!is_current_loc) {
            sym->state = AsmQueryExternal(name);
            if (sym->state != SYM_UNDEFINED)
                sym->mem_type = CvtTable[AsmQueryType(name)];
            else sym->mem_type = 0;
        }
    } else AsmError(37);
    return sym;
}

/* CF89e8..8a61, remove old hash-chain identity before allocating its new name. */
int AsmChangeName(char *old_name, char *new_name)
{
    asm_sym **sym_ptr;
    asm_sym *sym;
    sym_ptr = AsmFind(old_name);
    if (*sym_ptr != NULL) {
        sym = *sym_ptr;
        *sym_ptr = sym->next;
        AsmFree(sym->name);
        sym->name = AsmAlloc(strlen(new_name) + 1);
        strcpy(sym->name,new_name);
        sym_ptr = AsmFind(new_name);
        if (*sym_ptr != NULL) return ERROR;
        sym->next = *sym_ptr;
        *sym_ptr = sym;
    }
    return NOT_ERROR;
}

/* CF8b0c..8b65; FreeASym is inlined in the original rather than a
 * separate graph vertex. Preserve its name/fixup/symbol ownership order. */
void AsmSymFini(void)
{
    unsigned i;
    asm_sym *sym, *next;
    struct asmfixup *fix;
    FreePubQueue();
    FreeAliasQueue();
    FreeLnameQueue();
    for (i = 0; i < 211; ++i) {
        next = sym_table[i];
        while (next != NULL) {
            sym = next;
            next = sym->next;
            FreeInfo((dir_node *)sym);
            AsmFree(sym->name);
            while (sym->fixup != NULL) {
                fix = sym->fixup;
                sym->fixup = fix->next;
                AsmFree(fix);
            }
            AsmFree(sym);
        }
    }
}

/* CF8ad8..8b02. FreeASym's name/object frees are inline in1994. */
void AsmTakeOut( char *name )
{
    asm_sym *sym;
    asm_sym **sym_ptr;
    sym_ptr = AsmFind( name );
    if( *sym_ptr != NULL ) {
        sym = *sym_ptr;
        *sym_ptr = sym->next;
        FreeInfo( (dir_node *)sym );
        AsmFree( sym->name );
        AsmFree( sym );
    }
}
