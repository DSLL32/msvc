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

/* Reached pragma lookup from the actual 1994 compiler. Original source
 * reference: initial Open Watcom cpragma.c at6b9cb443. */
#include "cpragma.h"
#include "macro.h"
#include "pending.h"
#include <recon.h>
AUX_INFO *CurrInfo;
/* Address identity is encountered before the later pragma initialization.
 * These are genuine zero-initialized compiler objects, not host shims. */
static AUX_INFO DefaultInfo, CdeclInfo, PascalInfo;
static AUX_INFO FortranInfo, SyscallInfo, StdcallInfo;

void *PragmaLookup(char *name, unsigned index)
{
    AUX_ENTRY *entry;

    if (index && setAuxInfo(index, 0)) return CurrInfo;
    if (setAuxInfo(lookupMagicKeyword(name), 0)) return CurrInfo;
    entry = AuxLookup(name);
    if (!entry) return 0;
    return entry->info;
}

AUX_ENTRY *AuxLookup(char *name) { (void)name; STUB_TRAP(); return 0; }

int setAuxInfo(unsigned index, int create_new)
{
    switch (index) {
    case 1: CurrInfo = &DefaultInfo; break;
    case 2: CurrInfo = &CdeclInfo; break;
    case 3: CurrInfo = &PascalInfo; break;
    case 4: CurrInfo = &FortranInfo; break;
    case 5: CurrInfo = &SyscallInfo; break;
    case 6: CurrInfo = &StdcallInfo; break;
    default:
        if (!create_new) return 0;
        CreateAux(Buffer);
        break;
    }
    return 1;
}

void CreateAux(char *name) { (void)name; STUB_TRAP(); }
