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


/* Adapted from initial6b9cb443 symbol.c. Actual1994 SymCreate
 * returns ScopeInsert directly; the later check!=sym NULL rewrite is absent.
 * The file-scope wrapper owns its real shared argument/call tail4e892..4e89b. */
#include "symbol.h"
#include "pending.h"
#include <recon.h>
SYMBOL symAllocate(TYPE type,unsigned char id,unsigned flags)
{
    SYMBOL sym;
    sym=AllocSymbol();
    sym->id=id;
    sym->sym_type=type;
    sym->flag=flags;
    return sym;
}
SYMBOL SymCreate(TYPE type,unsigned char id,unsigned flags,char *name,SCOPE scope)
{
    SYMBOL sym;
    sym=symAllocate(type,id,flags);
    return ScopeInsert(scope,sym,name);
}
SYMBOL SymCreateFileScope(TYPE type,unsigned char id,unsigned flags,char *name)
{
    return SymCreate(type,id,flags,name,FileScope);
}

/* The old private predicate bodies are genuine graph entries even though
 * the later released symbol.c expressed them as macros. */
TYPE SymType(SYMBOL sym) { return TypedefModifierRemove(sym->sym_type); }
int SymIsError(SYMBOL sym) { return SymType(sym)->id==TYP_ERROR; }
int symIsDataFunction(SYMBOL sym)
{
    if (sym->id==5 || sym->id==6 || SymIsError(sym)) return 0;
    return 1;
}
int symIsFuncSym(SYMBOL sym) { return FunctionDeclarationType(sym->sym_type)!=0; }
int SymIsFunction(SYMBOL sym) { return symIsDataFunction(sym) && symIsFuncSym(sym); }

arg_list *SymFuncArgList(SYMBOL sym) { return TypeArgList(sym->sym_type); }

int SymIsInline(SYMBOL sym)
{
    TYPE type;
    type=FunctionDeclarationType(sym->sym_type);
    return type!=0 && (type->flag&0x800)!=0;
}
int SymIsData(SYMBOL sym)
{ return symIsDataFunction(sym) && !symIsFuncSym(sym); }
int SymIsTemporary(SYMBOL sym)
{
    if (!SymIsData(sym)) return 0;
    if (SymIsError(sym)) return 1;
    return sym->name->name[0]=='.';
}

char *CppDestructorName(void);
int symHasFuncName(SYMBOL sym,char *name)
{
    if (SymIsFunction(sym)) return name==sym->name->name;
    return 0;
}
int SymIsDtor(SYMBOL sym)
{ return symHasFuncName(sym,CppDestructorName()); }
int SymIsCtor(SYMBOL sym) { (void)sym; STUB_TRAP(); return 0; }
int SymIsAssign(SYMBOL sym) { (void)sym; STUB_TRAP(); return 0; }
int SymIsGenedFunc(SYMBOL sym)
{
    if(!SymIsFunction(sym)) return 0;
    if(SymIsError(sym)) return 1;
    return sym->name->name[0]=='.';
}

int SymIsInitialized(SYMBOL sym) { return sym->flag&2; }

int SymIsClassTemplateModel(SYMBOL sym)
{ (void)sym; STUB_TRAP(); return 0; }

SCOPE SymScope(SYMBOL sym)
{
    return sym->name ? sym->name->containing : NULL;
}

SYMBOL SymDefaultBase(SYMBOL sym)
{
    while (sym->id==7) sym=sym->thread;
    return sym;
}
TYPE ScopeClass(SCOPE scope);
TYPE SymClass(SYMBOL sym)
{
    SCOPE scope=SymScope(sym);
    return scope ? ScopeClass(scope) : NULL;
}
int SymIsClassMember(SYMBOL sym)
{
    return symIsDataFunction(sym) && SymClass(sym)!=NULL;
}
int SymIsTypedef(SYMBOL sym) { return sym->id==5; }
int SymIsStatic(SYMBOL sym) { return SymDefaultBase(sym)->id==2; }
int SymIsStaticMember(SYMBOL sym)
{
    return SymIsClassMember(sym) && SymIsStatic(sym);
}

int SymIsThisMember(SYMBOL sym)
{
    return SymIsClassMember(sym) && !SymIsStatic(sym) && sym->id!=8;
}
int SymIsThisFuncMember(SYMBOL sym)
{
    return SymIsThisMember(sym) && symIsFuncSym(sym);
}

void SymbolLocnDefine(CGIO_LOCATION *location,SYMBOL sym);
SYMBOL SymCreateAtLocn(TYPE type,unsigned char id,unsigned flags,char *name,
    SCOPE scope,CGIO_LOCATION *location)
{
    SYMBOL sym=symAllocate(type,id,flags);
    SymbolLocnDefine(location,sym);
    return ScopeInsert(scope,sym,name);
}

char *CppSpecialName(unsigned index);
SCOPE ScopeFunctionScopeInProgress(void);
SYMBOL SymAllocReturn(SCOPE scope,TYPE type)
{
    return SymCreate(type,3,1,CppSpecialName(1),scope);
}
SYMBOL SymFunctionReturn(void)
{
    SEARCH_RESULT *result=ScopeFindNaked(CurrScope,CppSpecialName(1));
    SYMBOL retn=NULL;
    SCOPE function_scope;
    if(result) {
        function_scope=ScopeFunctionScopeInProgress();
        if(result->scope==function_scope || result->scope->enclosing==function_scope) {
            retn=result->sym_name->name_syms;
            retn->flag|=1;
        }
        ScopeFreeResult(result);
    }
    return retn;
}

TYPE SymFuncReturnType(SYMBOL sym) { return SymType(sym)->of; }

unsigned SymIsConstantInt(SYMBOL sym)
{
    return sym->flag&4;
}

char *CppSpecialName(unsigned name);
int SymIsThunk(SYMBOL sym)
{
    char *name;
    if (!sym || !sym->name || !sym->name->name) return 0;
    name=sym->name->name;
    return name==CppSpecialName(8) || name==CppSpecialName(6) ||
        name==CppSpecialName(9) || name==CppSpecialName(10) ||
        name==CppSpecialName(7);
}
SYMBOL SymMarkRefed(SYMBOL sym)
{
    /*1994 marks this symbol directly, before the later default-base walk. */
    sym->flag|=1;
    if(SymIsFunction(sym) && SymIsThunk(sym)) sym->u.sym->flag|=1;
    return sym;
}

/* The old entry is the loop test; its alias dereference precedes it. */
SYMBOL SymDeAlias(SYMBOL sym)
{
    while(sym->id==11) sym=sym->u.sym;
    return sym;
}

int SymIsAnonymous(SYMBOL sym) { (void)sym; STUB_TRAP(); return 0; }

int SymIsFunctionTemplateModel(SYMBOL sym)
{ return sym->id==13; }

SYMBOL SymMakeDummy(TYPE type,char **name)
{ (void)type; (void)name; STUB_TRAP(); return NULL; }

int TypeDefined(TYPE type);
int SymClassCorrupted(SYMBOL sym)
{
    TYPE type=SymClass(sym);
    return !type || !TypeDefined(type) || type->u.c.info->corrupted;
}

int SymIsVirtual(SYMBOL sym)
{
    TYPE type=FunctionDeclarationType(sym->sym_type);
    return type && (type->flag&0x10);
}

int SymIsThisDataMember(SYMBOL sym)
{ return SymIsThisMember(sym) && !symIsFuncSym(sym); }
int TypeRequiresDtoring(TYPE type);
int SymRequiresDtoring(SYMBOL sym)
{ return SymIsData(sym) && TypeRequiresDtoring(sym->sym_type); }

int TypeRequiresCtoring(TYPE type);
int SymRequiresCtoring(SYMBOL sym)
{ return SymIsData(sym) && TypeRequiresCtoring(sym->sym_type); }

int SymIsAnonymousMember(SYMBOL sym) { (void)sym; STUB_TRAP(); return 0; }

void SymSetNvReferenced(SYMBOL sym)
{
    if(!SymIsVirtual(sym)) SymMarkRefed(sym);
}

int SymIsEnumeration(SYMBOL sym) { return sym->id==8; }
int SymIsArgument(SYMBOL sym) { (void)sym; STUB_TRAP(); return 0; }

int SymIsAutomatic(SYMBOL sym)
{
    sym=SymDeAlias(sym);
    return sym->id==3 || sym->id==4;
}
int SymIsStaticDataMember(SYMBOL sym)
{ return SymIsStaticMember(sym) && !symIsFuncSym(sym); }

int SymIsExtern(SYMBOL sym)
{
    if(SymIsInitialized(sym)) return 0;
    return sym->id==1 || SymIsStaticDataMember(sym);
}
