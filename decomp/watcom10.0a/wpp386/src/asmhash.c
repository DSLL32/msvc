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
* Description:  Inline assembler instruction-name hash table.
*
****************************************************************************/


/* insthash.c's instruction-name hash table. */
#include <string.h>
#include <stdlib.h>
#include "asm.h"
#include "pending.h"
#include <recon.h>

void make_inst_hash_table(void)
{
    unsigned short i;
    for (i=0;i!=T_NULL;++i) add(&AsmOpcode[i]);
}

struct AsmCodeName *add(struct AsmCodeName *instruction)
{
    struct AsmCodeName **location;
    char buffer[20];
    strncpy(buffer,AsmChars+instruction->index,instruction->len);
    buffer[instruction->len]='\0';
    location=find(buffer);
    if (*location!=NULL) {
        AsmError(45);
        return NULL;
    }
    instruction->next=*location;
    *location=instruction;
    return instruction;
}

struct AsmCodeName *inst_table[211];
struct AsmCodeName **find(char *name)
{
    struct AsmCodeName **instruction;
    instruction=&inst_table[AsmHashPJW(name)];
    for (;*instruction;instruction=&(*instruction)->next) {
        if (strnicmp(name,AsmChars+(*instruction)->index,(*instruction)->len)==0 &&
            name[(*instruction)->len]=='\0') return instruction;
    }
    return instruction;
}

/* This earlier PJW variant rotates the high nibble left before masking.
 * Keep its genuine library call; the later source's g>>12 is different. */
#ifdef __WATCOMC__
#pragma function(_rotl)
#endif
unsigned AsmHashPJW(char *name)
{
    unsigned h;
    for (h=0;*name;++name) {
        h=(h<<4)+(unsigned char)(*name|' ');
        h^=_rotl(h&0xf000U,4);
        h&=0x0fffU;
    }
    return h%211;
}

/* The original reports frontend assembler diagnostics through this pending App entry. */
void AsmError(int code) { (void)code; STUB_TRAP(); }
