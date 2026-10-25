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

/* Adapted from bld/sdk/rc/wres/c/read.x and resrdu32.c, commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9; CF22660..2269d. */
#include "wasm.h"
#include <recon.h>

int ResReadUint32(unsigned long *value, int handle)
{
    int numread;
    numread = (*WResRtns.read)(handle, value, sizeof(*value));
    if (numread == sizeof(*value)) {
        return 0;
    }
    if (numread == -1) {
        WresRecordError(8); /* WRS_READ_FAILED */
    } else {
        WresRecordError(14); /* WRS_READ_INCOMPLETE */
    }
    return 1;
}

/* CF216f0..21728: the pre-v3 fixed type record is five bytes. */
int WResReadFixedTypeRecord(WResTypeInfo *info, int handle)
{
    int numread;
    numread = (*WResRtns.read)(handle, info, sizeof(*info));
    if (numread == sizeof(*info)) {
        return 0;
    }
    WresRecordError(numread == -1 ? 8 : 14);
    return 1;
}

/* CF21630..2166d: version-0/1 resource record, including byte-sized name. */
int WResReadFixedResRecord1(WResResInfo1 *info, int handle)
{
    int numread;
    numread = (*WResRtns.read)(handle, info, sizeof(*info));
    if (numread != sizeof(*info)) {
        WresRecordError(numread == -1 ? 8 : 14);
        return 1;
    }
    return 0;
}
