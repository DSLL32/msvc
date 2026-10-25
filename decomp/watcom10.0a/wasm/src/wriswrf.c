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

/* Adapted from bld/sdk/rc/wres/c/read.x WResIsWResFile and wriswrf.c,
 * commit6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Actual CF21f70..21ff5
 * tests the older boolean magic directly; no newer three-format dispatcher. */
#include "wasm.h"
#include <recon.h>

int WResIsWResFile(int handle)
{
    long savepos;
    unsigned long magic[2];
    int error;
    savepos = (*WResRtns.seek)(handle, 0, SEEK_SET);
    if (savepos == -1L) {
        WresRecordError(9);
        return 0;
    }
    error = ResReadUint32(magic, handle);
    if (!error) {
        error = ResReadUint32(magic + 1, handle);
    }
    if ((*WResRtns.seek)(handle, savepos, SEEK_SET) == -1L) {
        WresRecordError(9);
    }
    return !error && magic[0] == 0xc3d4c1d7UL && magic[1] == 0xc3d2cdcfUL;
}
