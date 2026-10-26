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

/* bld/plusplus/c/cgio.c at initial commit6b9cb443. The1994 body
 * clears only the file ring; later thunk/removed rings are absent. */
#include "compiler.h"
#include "initstate.h"
#include "carve.h"
#include "cgio.h"
#include <recon.h>

static void *cg_file_ring;
static carve_t carveCGFILE, carveCGFILE_GEN;

void cgioInit(INITFINI *entry)
{
    (void)entry;
    cg_file_ring = 0;
    CgioBuffInit();
    /* Actual packed1994 element sizes, before those fields are encountered. */
    carveCGFILE = CarveCreate(46, 64);
    carveCGFILE_GEN = CarveCreate(156, 32);
}
