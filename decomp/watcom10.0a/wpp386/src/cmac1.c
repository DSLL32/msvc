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

/* bld/plusplus/c/cmac1.c initial commit6b9cb443. In1994 the special
 * entries are built here, without the later MacroSpecialAdd helper or
 * nested-macro carver. Names and order match the encountered six-row table. */
#include <stddef.h>
#include <string.h>
#include "compiler.h"
#include "initstate.h"
#include "macro.h"
#include <recon.h>

static struct special_macro_name {
    char *name;
    int value;
} SpcMacros[] = {
    { "__LINE__", 0 },
    { "__FILE__", 1 },
    { "__DATE__", 2 },
    { "__TIME__", 3 },
    { "__STDC__", 4 },
    { "__cplusplus", 5 },
    { 0, 0 }
};
static void *macro_expansion_state;
static void *nestedMacros, *scannerTokenList;
unsigned char InitialMacroFlag;
/* This path passes only the scanner buffer's address with length0.
 * Its full extent and contents are pending the scanner reconstruction. */
char Buffer[1];

void macroInit(INITFINI *entry)
{
    struct special_macro_name *mac;
    MEPTR macptr;
    unsigned required;

    (void)entry;
    DirectiveInit();
    macro_expansion_state = 0;
    nestedMacros = 0;
    scannerTokenList = 0;
    InitialMacroFlag = 1;
    MacroStorageInit();
    for (mac = SpcMacros; mac->name; ++mac) {
        required = strlen(mac->name) + offsetof(MEDEFN, macro_name) + 1;
        macptr = (MEPTR)CMemAlloc(required);
        macptr->macro_defn = 0;
        macptr->macro_len = required;
        macptr->parm_count = mac->value;
        strcpy(macptr->macro_name, mac->name);
        MacroDefine(macptr, Buffer, 0);
    }
    TimeInit();
}
