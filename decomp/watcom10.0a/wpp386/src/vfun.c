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


#include "ptree.h"
#include "symbol.h"
#include "scope.h"
#include "pending.h"
#include <recon.h>
/* Descriptive inferred name for the genuinely called empty entry35311.
 * The original name/formal parameters are unknown. Its sole RET consumes
 * no input or stack arguments, and this caller discards the return value.
 * It shares the predecessor's physical terminal byte, not that body. */
void noteVirtualReference(void) {}

PTREE NodeOffset(long offset);
PTREE NodeBinary(unsigned char op,PTREE left,PTREE right);
TYPE MakeVFTableFieldType(int add_consts);
PTREE NodeRvalue(PTREE expr);
TYPE TypePointedAtModified(TYPE type);
unsigned CgMemorySize(TYPE type);
PTREE AccessVirtualFnAddress(PTREE expr,SEARCH_RESULT *result,SYMBOL sym)
{
    TYPE vfptr_type,vfun_type;
    unsigned index;
    noteVirtualReference();
    expr=NodeBinary(48,expr,NodeOffset(result->vf_offset));
    vfptr_type=MakeVFTableFieldType(1);
    expr->type=vfptr_type;
    expr->flags|=2;
    expr=NodeRvalue(expr);
    vfun_type=TypePointedAtModified(vfptr_type);
    index=SymDefaultBase(sym)->u.uval-1;
    expr=NodeBinary(48,expr,NodeOffset(index*CgMemorySize(vfun_type)));
    expr->type=vfun_type;
    expr->flags|=2;
    expr=NodeRvalue(expr);
    expr->type=sym->sym_type;
    return expr;
}

PTREE NodeMakeCallee(SYMBOL sym);
PTREE VfnDecorateCall(PTREE expr,SYMBOL sym)
{
    PTREE callee=NodeMakeCallee(SymDefaultBase(sym));
    PTREE node;
    callee->cgop=205;
    node=NodeBinary(119,expr,callee);
    node->flags=expr->flags;
    node->type=expr->type;
    return node;
}
