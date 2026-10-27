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


/* Initial6b9cb443 module.c; actual1994 ModuleInitInit41fb0..42020
 * has the older SC_STATIC(2) symbol,
 * no later return-optimization store. */
#include "compiler.h"
#include "frontend.h"
#include "fnbody.h"
#include "types.h"
#include "scope.h"
#include "pending.h"
#include <recon.h>
static SYMBOL module_init_func;
static SCOPE module_init_scope;
static FUNCTION_DATA module_fd;
char *CppSpecialName(unsigned index);
TYPE TypeVoidFunOfVoid(void);
void ScopeBeginFunction(SYMBOL sym);
SCOPE moduleInitSave(void)
{
    SCOPE scope;
    scope=CurrScope;
    LabelSwitchFunc(&module_fd.label_mem);
    return scope;
}
void moduleInitRestore(SCOPE scope)
{
    CurrScope=scope;
    LabelSwitchFunc(&module_fd.label_mem);
}
void ModuleInitInit(void)
{
    SCOPE save;
    SYMBOL sym;
    TYPE type;
    char *name;
    /* Old41fb9 clears CompFlags byte8 bits6/7, preserving bit5.
     * Their individual source names remain unknown. */
    CompFlags.reserved8b&=1;
    save=moduleInitSave();
    name=CppSpecialName(4);
    type=TypeVoidFunOfVoid();
    sym=SymCreateFileScope(type,2,0,name);
    module_init_func=sym;
    CurrScope=FileScope;
    ScopeBeginFunction(sym);
    FunctionBodyStartup(sym,&module_fd,0);
    module_init_scope=CurrScope;
    ScopeKeep(CurrScope);
    moduleInitRestore(save);
}

void ScopeMemberModuleFunction(SCOPE scope,SCOPE function);
void ScopeRestoreModuleFunction(SCOPE function);
void ModuleInitResumeScoped(SCOPE scope)
{ ScopeMemberModuleFunction(scope,module_init_scope); }
SYMBOL ModuleInitFuncSym(void) { return module_init_func; }
void ModuleInitUsed(void)
{
    module_init_func->id=2;
    module_init_func->flag|=3;
}
SCOPE ModuleInitResume(void)
{
    SCOPE saved=moduleInitSave();
    CurrScope=module_init_scope;
    return saved;
}
void ModuleInitRestore(SCOPE scope)
{
    ScopeRestoreModuleFunction(module_init_scope);
    moduleInitRestore(scope);
}

CGFILE *CgFrontSwitchFile(SYMBOL sym);
void FunctionBodyShutdown(SYMBOL sym,FUNCTION_DATA *data);
void CgFrontModInitFini(void);
void ModuleInitFini(void)
{
    SCOPE save_scope;
    CgFrontSwitchFile(module_init_func);
    save_scope=ModuleInitResume();
    FunctionBodyShutdown(module_init_func,&module_fd);
    ScopeEnd(2);
    ModuleInitRestore(save_scope);
    CgFrontModInitFini();
}
