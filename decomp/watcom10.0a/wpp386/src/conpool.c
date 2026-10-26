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


/* Adapted from initial6b9cb443 bld/plusplus/c/conpool.c.
 * Actual1994 CF2ec6f..2ec90 has only the floating constant pool:
 *20-byte records,32 per block; no later int64 pool or64-record block. */
#include "compiler.h"
#include "carve.h"
#include "ptree.h"
#include "pending.h"
#include <recon.h>
typedef struct pool_con POOL_CON;
static POOL_CON *pool_float;
static carve_t carvePOOL_CON;
void conPoolsInit(INITFINI *defn)
{
    (void)defn;
    pool_float = 0;
    carvePOOL_CON = CarveCreate(20, 32);
}

void *ConPoolFloatAdd(PTREE expr)
{ (void)expr; STUB_TRAP(); return 0; }
