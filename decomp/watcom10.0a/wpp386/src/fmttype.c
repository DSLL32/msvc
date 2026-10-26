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


/* Adapted from initial6b9cb443 bld/plusplus/c/fmttype.c.
 * Actual1994 CF357a7..357be uses a byte-sized bitfield, making FMT_INFO
 * nine bytes before the allocator rounds it. */
#include "compiler.h"
#include "carve.h"
#include "types.h"
#include <recon.h>
#pragma pack(1)
typedef struct fmt_info {
    struct fmt_info *stack;
    TYPE type;
    unsigned char main_function:1;
} FMT_INFO;
#pragma pack()
typedef char FmtInfoSize[(sizeof(FMT_INFO)==9)?1:-1];
static carve_t carveFMT;
void formatTypeInit(INITFINI *defn)
{
    (void)defn;
    carveFMT = CarveCreate(sizeof(FMT_INFO), 16);
}
