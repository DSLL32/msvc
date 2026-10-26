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

/* bld/wasm/c/fatal.c at6b9cb4438959d845a2b4dd4bf19f7908ecd608e9.
 * Actual CFf5a0..f614 source/ownership sequence. */
#include <stdio.h>
#include "wasm.h"
#include <recon.h>

void AsmShutDown(void)
{
    if (AsmFiles.file[ASM] != NULL) {
        if (fclose(AsmFiles.file[ASM]) != 0)
            Fatal(2,AsmFiles.fname[ASM]);
    }
    ObjWriteClose(pobjState.file_out);
    ObjRecFini();
    if (!write_to_file || Options.error_count > 0)
        remove(AsmFiles.fname[OBJ]);
    AsmFree(AsmFiles.fname[ASM]);
    AsmFree(AsmFiles.fname[ERR]);
    AsmFree(AsmFiles.fname[OBJ]);
}
