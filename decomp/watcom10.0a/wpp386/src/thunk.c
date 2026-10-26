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


#include "compiler.h"
#include "symbol.h"
#include "pending.h"
#include <recon.h>
void *TypeHasPragma(TYPE type);
unsigned classifyThunk(SYMBOL sym) { (void)sym; STUB_TRAP(); return 0; }
SYMBOL addrThunkSymbol(unsigned kind,SYMBOL sym)
{ (void)kind; (void)sym; STUB_TRAP(); return 0; }
void RtnGenAddSymbol(unsigned char callback,SYMBOL sym);
SYMBOL ClassFunMakeAddressable(SYMBOL sym)
{
    SYMBOL thunk;
    unsigned kind;
    if (!sym) return sym;
    if (sym->id!=7 && !TypeHasPragma(sym->sym_type)) return sym;
    kind=classifyThunk(sym);
    thunk=addrThunkSymbol(kind,sym);
    if (!thunk->u.thunk_calls) {
        thunk->u.thunk_calls=sym;
        RtnGenAddSymbol(1,thunk);
    }
    return thunk;
}
