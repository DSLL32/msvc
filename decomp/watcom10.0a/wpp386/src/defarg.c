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


/* Original 1994 default arguments compile hidden helper functions. The later
 * defarg.c instead stores expression/scope nodes; its ownership model must not
 * be substituted for this reached old function-body lifecycle. */
#include "compiler.h"
#include "carve.h"
#include "symbol.h"
#include "scope.h"
#include "ptree.h"
#include "fnbody.h"
#include <recon.h>
SYMBOL CgFrontSaveFile(void);
void CgFrontRestoreFile(SYMBOL sym);
SCOPE SymScope(SYMBOL sym);
void ScopeBeginFunction(SYMBOL sym);
PTREE AnalyseReturnExpr(SYMBOL func,PTREE expr);
PTREE IcEmitExpr(PTREE expr);
SYMBOL SymFunctionReturn(void);
void CgFrontReturnSymbol(SYMBOL sym);
void FunctionBodyShutdown(SYMBOL sym,FUNCTION_DATA *data);

void defineDefaultValue(PTREE tree,SYMBOL sym,PTREE (*check)(PTREE))
{
    SYMBOL saved_file=CgFrontSaveFile();
    SCOPE saved_scope=CurrScope;
    SYMBOL func=sym->u.sym;
    FUNCTION_DATA data;
    CurrScope=SymScope(sym);
    ScopeBeginFunction(func);
    FunctionBodyStartup(func,&data,1);
    data.is_defarg=1;
    tree=AnalyseReturnExpr(func,tree);
    tree=PTreeTraversePostfix(tree,check);
    IcEmitExpr(tree);
    CgFrontReturnSymbol(SymFunctionReturn());
    FunctionBodyShutdown(func,&data);
    ScopeEnd(2);
    CurrScope=saved_scope;
    CgFrontRestoreFile(saved_file);
}
