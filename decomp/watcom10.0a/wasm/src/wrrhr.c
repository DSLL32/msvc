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

/* Adapted from bld/sdk/rc/wres/c/wrrhr.c, initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CF21730..217c1. */
#include "wasm.h"
#include <recon.h>

int WResReadHeaderRecord(WResHeader *header, int handle)
{
    long currpos;
    int error, numread;
    error = 1;
    currpos = (*WResRtns.seek)(handle, 0, SEEK_SET);
    if (currpos == -1L) {
        WresRecordError(9);
    } else {
        numread = (*WResRtns.read)(handle, header, sizeof(*header));
        if (numread != sizeof(*header)) {
            WresRecordError(numread == -1 ? 8 : 14);
        } else {
            currpos = (*WResRtns.seek)(handle, currpos, SEEK_SET);
            if (currpos == -1L) {
                WresRecordError(9);
            } else {
                error = 0;
            }
        }
    }
    return error;
}

/* CF217c4..21801: ten-byte extended header, preserving partial read effects. */
int WResReadExtHeader(WResExtHeader *header, int handle)
{
    int numread;
    numread = (*WResRtns.read)(handle, header, sizeof(*header));
    if (numread != sizeof(*header)) {
        WresRecordError(numread == -1 ? 8 : 14);
        return 1;
    }
    return 0;
}
