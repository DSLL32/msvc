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


/* Adapted to the actual1994 temporary-detection and registration-count
 * protocol; later PtdCompCtored state-table logic is not substituted. */
#include "compiler.h"
#include "ptree.h"
#include "symbol.h"
#include "cdopt.h"
#include "pending.h"
#include <recon.h>
int NodeIsBinaryOp(PTREE expr,unsigned char op);
int NodeIsUnaryOp(PTREE expr,unsigned char op);
int SymIsTemporary(SYMBOL sym);
void FunctionHasRegistration(void);
unsigned FunctionRegistrationFlag(void);
PTREE NodeComma(PTREE left,PTREE right);
PTREE PTreeIc(unsigned opcode,unsigned value);
PTREE NodeAddSideEffect(PTREE effect,PTREE expr);
static int has_temporary;
PTREE checkForTemporary(PTREE expr)
{
    if (NodeIsBinaryOp(expr,99) &&
        SymIsTemporary(expr->u.subtree[0]->u.subtree[0]->u.symcg.symbol))
        has_temporary=1;
    return expr;
}
PTREE addPtIcUnsigned(PTREE tree,unsigned opcode,unsigned value)
{
    PTREE node,*operand;
    if (!tree) {
        CgFrontCodePtr(opcode,(void *)value);
        CgFrontCode(26);
        return NULL;
    }
    if (NodeIsUnaryOp(tree,94)) operand=&tree->u.subtree[0];
    else operand=&tree;
    node=PTreeIc(opcode,value);
    *operand=NodeAddSideEffect(node,*operand);
    return tree;
}
PTREE DataDtorCompClass(PTREE expr,long offset)
{
    unsigned flag;
    has_temporary=0;
    PTreeTraversePostfix(expr,checkForTemporary);
    if (has_temporary) {
        flag=FunctionRegistrationFlag();
        expr=addPtIcUnsigned(expr,135,(unsigned)offset);
        expr=NodeComma(PTreeIc(134,flag),expr);
    } else {
        FunctionHasRegistration();
        expr=addPtIcUnsigned(expr,75,(unsigned)offset);
    }
    return expr;
}

TYPE ArrayType(TYPE type);
TYPE ArrayBaseType(TYPE type);
PTREE DataDtorObjPush(PTREE expr,TYPE type,SYMBOL sym,unsigned offset)
{
    TYPE dtor_type=type;
    if (ArrayType(dtor_type)) dtor_type=ArrayBaseType(dtor_type);
    CDoptDtorBuild(dtor_type);
    FunctionHasRegistration();
    /* The old protocol predates the released PtdObjPush helper. */
    expr=addPtIcUnsigned(expr,111,(unsigned)type);
    expr=addPtIcUnsigned(expr,113,(unsigned)sym);
    return addPtIcUnsigned(expr,114,offset);
}

PTREE addPtIc(PTREE expr,unsigned opcode)
{ return addPtIcUnsigned(expr,opcode,0); }
PTREE DataDtorObjPop(PTREE expr)
{ return addPtIc(expr,112); }
