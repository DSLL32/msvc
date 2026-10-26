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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


/* AsmInit from released asmins.c, with the older inline-assembler layouts.
 * In this target both cpu4 and cpu5 select P_486p, as the actual jump table
 * maps both entries to the same older implementation. */
#include <stdlib.h>
#include "asm.h"
#include <recon.h>

static struct asm_tok tokens[100];
struct asm_tok *AsmBuffer[100];
static struct asm_code Code_Info;
struct asm_code *Code=&Code_Info;

void AsmInit(int cpu,int fpu,int use32)
{
    int pos,size,count;
    enum asm_token token_value;
    pos=0;
    token_value=1;
    size=sizeof(AsmOpTable)/sizeof(AsmOpTable[0]);
    for (count=0;count<100;++count) AsmBuffer[count]=&tokens[count];
    if (AsmOpcode[1].position==0) {
        while (AsmOpcode[token_value].len!=0) {
            do {
                ++pos;
            } while (AsmOpTable[pos].token!=token_value && pos<=size);
            if (pos>size) {
                AsmError(88);
                exit(-1);
            }
            AsmOpcode[token_value].position=pos;
            ++token_value;
        }
        switch (use32) {
        case 0: Code->use32=0; break;
        case 1: Code->use32=1; break;
        }
        if (fpu<0) fpu=0;
        switch (cpu) {
        case 0: Code->info.cpu |= P_86; if (fpu) Code->info.cpu |= P_87; break;
        case 1: Code->info.cpu |= P_186; if (fpu) Code->info.cpu |= P_87; break;
        case 2: Code->info.cpu |= P_286p; if (fpu) Code->info.cpu |= P_287; break;
        case 3: Code->info.cpu |= P_386p; if (fpu) Code->info.cpu |= P_387; break;
        case 4:
        case 5: Code->info.cpu |= P_486p; if (fpu) Code->info.cpu |= P_387; break;
        }
    }
    make_inst_hash_table();
}
