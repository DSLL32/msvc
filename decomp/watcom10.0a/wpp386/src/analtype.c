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

/* Common class-derivation classification from initial analtype.c; the
 * returned byte values are the actual1994 CTD ordering. */
#include "types.h"
#include "ptree.h"
#include "symbol.h"
#include "pending.h"
#include <recon.h>
TYPE StructType(TYPE type);
SCOPE TypeScope(TYPE type);
unsigned char ScopeDerived(SCOPE derived,SCOPE base);
unsigned char TypeCommonDerivation(TYPE source,TYPE target)
{
    SCOPE src=TypeScope(StructType(source)),tgt;
    unsigned char derived;
    if(!src) return 0;
    tgt=TypeScope(StructType(target));
    if(src==tgt) return 1;
    if(!tgt) return 0;
    derived=ScopeDerived(src,tgt);
    switch(derived) {
    case 1: return 1;
    case 2: return 2;
    case 3: return 3;
    case 4: return 4;
    case 5: return 5;
    case 0:
        derived=ScopeDerived(tgt,src);
        switch(derived) {
        case 0: return 0;
        case 1: return 6;
        case 2: return 7;
        case 3: return 8;
        case 4: return 9;
        case 5: return 10;
        }
    }
    STUB_TRAP(); return 0;
}

TYPE TypeGetActualFlags(TYPE type,unsigned short *flags);
TYPE MakeModifiedType(TYPE type,unsigned flags);
TYPE augmentWithNear(TYPE type)
{
    unsigned short flags;
    TypeGetActualFlags(type,&flags);
    if(!(flags&0x73c)) type=MakeModifiedType(type,4);
    return type;
}
TYPE TypeAutoDefault(TYPE type,PTREE expr)
{
    if(expr->op==7) {
        SYMBOL sym=expr->u.symcg.symbol;
        if(sym->id==3) {
            unsigned short flags;
            TypeGetActualFlags(sym->sym_type,&flags);
            if(!(flags&0x73c)) type=augmentWithNear(type);
        }
    }
    return type;
}

TYPE TypeCommonBase(TYPE first,TYPE second)
{ (void)first; (void)second; STUB_TRAP(); return NULL; }

TYPE SymClass(SYMBOL sym);
TYPE NodeType(PTREE expr);
TYPE ReferenceType(TYPE type);
TYPE MakeReferenceTo(TYPE type);
TYPE MakePointerTo(TYPE type);
TYPE TypeThisForCall(PTREE expr,SYMBOL sym)
{
    TYPE type=SymClass(sym);
    if(ReferenceType(NodeType(expr))) return MakeReferenceTo(type);
    return MakePointerTo(type);
}

TYPE TypeUnArithResult(TYPE type)
{ return GetBasicType(integralPromote(type)); }
