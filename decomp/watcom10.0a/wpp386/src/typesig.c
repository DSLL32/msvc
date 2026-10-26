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


/* Recovered actual1994 type-signature pool initialization5a6b7..5a6d8.
 * initial6b9cb443 typesig.h supplies the semantic names; its typesig.c
 * no longer contains the implementation. The old28-byte TYPE_SIG lacks
 * the later base link. PCH relocation visits next/type/four symbol slots,
 * and the actual free flag is bit2 at24. typeSigInit is a descriptive
 * recovered name, not a symbol imported from the stripped binary. */
#include "compiler.h"
#include "carve.h"
#include "types.h"
#include <recon.h>
typedef struct type_sig TYPE_SIG;
struct type_sig {
    TYPE_SIG *next;
    TYPE type;
    void *sym, *dtor, *default_ctor, *copy_ctor;
    unsigned cgref:1, cggen:1, free:1;
};
typedef char TypeSigSize[(sizeof(TYPE_SIG)==28)?1:-1];
static TYPE_SIG *type_sigs;
static carve_t carveTYPE_SIG;
void typeSigInit(INITFINI *defn)
{
    (void)defn;
    type_sigs = 0;
    carveTYPE_SIG = CarveCreate(sizeof(TYPE_SIG), 16);
}
