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

/* Adapted from bld/wasm/c/asmins.c AsmInit at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9; actual CF6e68..6ffa.
 * Token projection preserves original 939 row indices. CPU5 shares CPU4's
 * branch in the 1994 switch; newer Pentium/686 source branches are absent. */
#include <stdlib.h>
#include "asminit.h"
#include <recon.h>

static struct asm_tok tokens[100];
struct asm_tok *AsmBuffer[100];
static struct asm_code Code_Info;
struct asm_code *Code = &Code_Info;

void AsmInit(int cpu, int fpu, int use32)
{
    int pos = 0;
    unsigned short token_value = 1;
    int count;
    for (count = 0; count < 100; ++count) {
        AsmBuffer[count] = &tokens[count];
    }
    if (AsmOpcode[1].position == 0) {
        while (AsmOpcode[token_value].len != 0) {
            do {
                ++pos;
            } while (AsmOpTokens[pos] != token_value && pos <= 939);
            if (pos > 939) {
                AsmError(138);
                exit(-1);
            }
            AsmOpcode[token_value].position = pos;
            ++token_value;
        }
        switch (use32) {
        case 0: Code->use32 = 0; break;
        case 1: Code->use32 = 1; break;
        }
        if (fpu < 0) fpu = 0;
        switch (cpu) {
        case 0:
            if (fpu) Code->info.cpu |= 1;
            break;
        case 1:
            Code->info.cpu |= 0x10;
            if (fpu) Code->info.cpu |= 1;
            break;
        case 2:
            Code->info.cpu |= 0x28;
            if (fpu) Code->info.cpu |= 2;
            break;
        case 3:
            Code->info.cpu |= 0x38;
            if (fpu) Code->info.cpu |= 4;
            break;
        case 4:
        case 5:
            Code->info.cpu |= 0x48;
            if (fpu) Code->info.cpu |= 4;
            break;
        }
    }
    make_inst_hash_table();
}
