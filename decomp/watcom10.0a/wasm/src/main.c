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

/* Adapted from bld/wasm/c/main.c, Open Watcom initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Actual Watcom 10.0a CF
 * cb28..cb5d determines the six edges, quiet+2 and unsigned error_count+8.
 * This is a static candidate; no fresh original/native acceptance yet. */
#include "wasm.h"
#include <recon.h>

global_options Options = { 0, 0, 0, 0, 0, 0, 0, 1, 0 };

int main(int argc, char **argv)
{
    main_init();
    do_init_stuff(argc, argv);
    WriteObjModule();
    if (!Options.quiet) {
        PrintStats();
    }
    MsgFini();
    main_fini();
    return (int)Options.error_count;
}
