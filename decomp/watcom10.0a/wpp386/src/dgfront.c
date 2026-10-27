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


/* Initial dgfront.c adapted to the actual1994 storage-class ids and
 * separate value/data emitters; no later static-member extension. */
#include "symbol.h"
#include "ptree.h"
#include "pending.h"
#include <recon.h>
void DgSymbol(SYMBOL sym) { (void)sym; STUB_TRAP(); }
void setICValue(unsigned value);
void CgFrontData(int opcode);
void DgSymbolLabel(SYMBOL sym)
{
    if(sym->id==0 || sym->id==1) sym->id=10;
    setICValue((unsigned)sym);
    CgFrontData(46);
}

TYPE TypedefModifierRemove(TYPE type);
void addICValue(unsigned value);
void *ConPoolFloatAdd(PTREE expr);
void CFatal(const char *message);
int DgStoreScalarValue(TYPE type,PTREE expr,unsigned offset)
{
    setICValue((unsigned)type);
    CgFrontData(25);
    switch(expr->op) {
    case 4:
        setICValue((unsigned)ConPoolFloatAdd(expr));
        CgFrontData(51);
        /* The old double representation again tests only its high word. */
        return expr->u.floating_bits.high==0;
    case 2: case 3:
        addICValue(expr->u.uint_constant);
        CgFrontData(50);
        return expr->u.int_constant==0;
    case 1:
        addICValue(offset);
        CgFrontData(47);
        setICValue(expr->u.uint_constant);
        CgFrontData(49);
        return 0;
    case 7:
        addICValue(offset);
        CgFrontData(47);
        setICValue((unsigned)expr->u.symcg.symbol);
        CgFrontData(48);
        return 0;
    default:
        CFatal("dgfront.c unexpected pointer data in DgStorePointer");
        return 0;
    }
}
int DgStoreMemberPointer(TYPE type,PTREE expr)
{ (void)type; (void)expr; STUB_TRAP(); return 0; }
int DgStoreScalar(PTREE expr,unsigned offset,TYPE type)
{
    type=TypedefModifierRemove(type);
    if(type->id==TYP_MEMBER_POINTER) return DgStoreMemberPointer(type,expr);
    return DgStoreScalarValue(type,expr,offset);
}



void DgPadBytes(unsigned size) { (void)size; STUB_TRAP(); }
void DgStoreConstScalar(PTREE expr,TYPE type,SYMBOL sym)
{ (void)expr; (void)type; (void)sym; STUB_TRAP(); }

int DgStoreBitfield(TYPE type,unsigned mask)
{ (void)type; (void)mask; STUB_TRAP(); return 0; }


