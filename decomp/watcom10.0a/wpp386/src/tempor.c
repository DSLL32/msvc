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

/* Reached1994 tempor.c scope bracket at52c5a/52c73, retaining the
 * actual single saved scope and the TypeScope call boundary. */
#include "types.h"
#include "scope.h"
#include "symbol.h"
#include "pending.h"
#include <recon.h>
static SCOPE tempScope;
SCOPE TypeScope(TYPE type);
void ScopeGenAccessSet(TYPE type)
{
    tempScope=CurrScope;
    CurrScope=TypeScope(type);
}
void ScopeGenAccessReset(void)
{
    SCOPE old=tempScope;
    tempScope=NULL;
    CurrScope=old;
}

/* TEMP_TYPE_EXPR is the actual initialized DATA byte at CF0xb7d18. */
static unsigned char tempClass=1;
unsigned char TemporaryClass(unsigned char kind)
{
    unsigned char old=tempClass;
    tempClass=kind;
    return old;
}

SYMBOL SymMakeDummy(TYPE type,char **name);
unsigned short CgSegIdData(SYMBOL sym,unsigned char control);
void DgSymbol(SYMBOL sym);
SCOPE ScopeForTemps(void)
{
    return tempScope?tempScope:CurrScope;
}
SYMBOL makeTemporary(TYPE type,int storage)
{
    SYMBOL sym;
    char *name;
    unsigned char id;
    sym=SymMakeDummy(type,&name);
    if (tempClass==1 || tempClass==2) id=3;
    else if (tempClass==3) id=2;
    else { STUB_TRAP(); return NULL; }
    sym->id=id;
    if (id==2) {
        ScopeInsert(FileScope,sym,name);
        if (storage) {
            CgSegIdData(sym,1);
            DgSymbol(sym);
        }
    } else ScopeInsert(ScopeForTemps(),sym,name);
    return sym;
}
SYMBOL TemporaryAlloc(TYPE type)
{
    return makeTemporary(type,1);
}
