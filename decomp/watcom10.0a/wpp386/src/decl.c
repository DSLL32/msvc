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


/* Adapted from initial6b9cb443 decl.c, actual1994 CF31555..317e6.
 * Old classes: TYPEDEF5 / CLASS_TEMPLATE12; no namespace case, browser
 * callbacks, FNC_NO_DEALIAS fourth argument, KEEP_SYMBOL or new-flags output.
 * Actual old overload statuses3/4/5 and integer diagnostics are retained. */
#include "symbol.h"
#include "linkage.h"
#include "frontend.h"
#include "pending.h"
#include <recon.h>
TYPE TypedefRemove(TYPE type);
TYPE ElaboratableType(TYPE type);
int TypesIdentical(TYPE left,TYPE right);
TYPE MakeTypedefOf(TYPE type,SCOPE scope,SYMBOL sym);
int SymIsFunction(SYMBOL sym);
int MainProcedure(SYMBOL sym);
void verifyMainFunction(SYMBOL sym);
int userWantsTypeSymOverload(SYMBOL type_sym,SYMBOL other);
unsigned char IsOverloadedFuncDistinct(SYMBOL *previous,SYMBOL current,char *name);
int checkFnStorageClass(SYMBOL previous,SYMBOL current,unsigned *control);
SYMBOL combineFunctions(SYMBOL previous,SYMBOL current);
int combinableSyms(SYMBOL previous,SYMBOL current,unsigned char *adjust);
int checkVarStorageClass(SYMBOL previous,SYMBOL current,unsigned *control);
SYMBOL combineVariables(SYMBOL previous,SYMBOL current,unsigned char adjust);
void verifyCanBeOverloaded(SYMBOL_NAME sym_name,SYMBOL sym,char *name);

SYMBOL DeclCheck(SYMBOL_NAME sym_name,SYMBOL sym,unsigned *control)
{
    TYPE sym_type,chk_type;
    SYMBOL chk_sym;
    int new_sym_is_function;
    unsigned char check,adjust;
    char *name;
    *control=0;
    name=sym_name->name;
    sym_type=TypedefRemove(sym->sym_type);
    switch (sym->id) {
    case 12:
        chk_sym=sym_name->name_type;
        if (chk_sym!=0) {
            if (chk_sym->id==12) CErr2p(174,(const char *)chk_sym);
            else CErr2p(176,(const char *)chk_sym);
            return 0;
        }
        chk_sym=sym_name->name_syms;
        if (chk_sym!=0) { CErr2p(176,(const char *)chk_sym); return 0; }
        RingAppend((void **)&sym_name->name_type,sym);
        break;
    case 5:
        chk_sym=sym_name->name_type;
        if (chk_sym!=0) {
            chk_type=TypedefRemove(chk_sym->sym_type);
            if (sym_type!=chk_type) {
                if (chk_sym->id==12) CErr2p(179,(const char *)chk_sym);
                else if (TypesIdentical(sym_type,chk_type)) CErr2p(583,(const char *)chk_sym);
                else CErr2p(125,(const char *)chk_sym);
                return 0;
            }
            return chk_sym;
        }
        chk_sym=sym_name->name_syms;
        if (chk_sym!=0) {
            if (ElaboratableType(sym_type)==0) {
                CErr2p(458,(const char *)chk_sym); return 0;
            }
            if (!userWantsTypeSymOverload(sym,chk_sym))
                CErr2p(481,(const char *)chk_sym);
        }
        sym->sym_type=MakeTypedefOf(sym_type,sym_name->containing,sym);
        RingAppend((void **)&sym_name->name_type,sym);
        break;
    default:
        new_sym_is_function=SymIsFunction(sym);
        chk_sym=sym_name->name_syms;
        if (chk_sym==0) {
            chk_sym=sym_name->name_type;
            if (chk_sym!=0) {
                switch (chk_sym->id) {
                case 12: CErr2p(179,(const char *)chk_sym); return 0;
                case 5:
                    if (ElaboratableType(chk_sym->sym_type)==0) {
                        CErr2p(457,(const char *)chk_sym); return 0;
                    }
                    if (!userWantsTypeSymOverload(chk_sym,sym))
                        CErr2p(480,(const char *)chk_sym);
                    break;
                }
            }
            RingAppend((void **)&sym_name->name_syms,sym);
            if (new_sym_is_function && MainProcedure(sym)) verifyMainFunction(sym);
        } else if (SymIsFunction(chk_sym)) {
            if (!new_sym_is_function) { CErr2p(258,name); return 0; }
            check=IsOverloadedFuncDistinct(&chk_sym,sym,name);
            switch (check) {
            case 3:
                if (checkFnStorageClass(chk_sym,sym,control))
                    return combineFunctions(chk_sym,sym);
                return 0;
            case 4: CErr2p(226,(const char *)chk_sym); break;
            case 5: CErr2p(385,(const char *)chk_sym); break;
            }
            verifyCanBeOverloaded(sym_name,sym,name);
            RingAppend((void **)&sym_name->name_syms,sym);
        } else {
            if (new_sym_is_function) { CErr2p(258,name); return 0; }
            adjust=0;
            if (combinableSyms(chk_sym,sym,&adjust)
                && checkVarStorageClass(chk_sym,sym,control))
                return combineVariables(chk_sym,sym,adjust);
            CErr2p(42,(const char *)chk_sym);
            return 0;
        }
    }
    return sym;
}
TYPE ElaboratableType(TYPE type) { (void)type; STUB_TRAP(); return 0; }
int TypesIdentical(TYPE left,TYPE right);
TYPE MakeTypedefOf(TYPE type,SCOPE scope,SYMBOL sym)
{
    TYPE new_type=MakeType(TYP_TYPEDEF);
    new_type->u.t.sym=sym;
    new_type->u.t.scope=scope;
    return MakeTypeOf(new_type,type);
}
void verifyMainFunction(SYMBOL sym) { (void)sym; STUB_TRAP(); }
int userWantsTypeSymOverload(SYMBOL type_sym,SYMBOL other)
{ (void)type_sym; (void)other; STUB_TRAP(); return 0; }
int checkFnStorageClass(SYMBOL previous,SYMBOL current,unsigned *control)
{ (void)previous; (void)current; (void)control; STUB_TRAP(); return 0; }
SYMBOL combineFunctions(SYMBOL previous,SYMBOL current)
{ (void)previous; (void)current; STUB_TRAP(); return 0; }
int combinableSyms(SYMBOL previous,SYMBOL current,unsigned char *adjust)
{ (void)previous; (void)current; (void)adjust; STUB_TRAP(); return 0; }
int checkVarStorageClass(SYMBOL previous,SYMBOL current,unsigned *control)
{ (void)previous; (void)current; (void)control; STUB_TRAP(); return 0; }
SYMBOL combineVariables(SYMBOL previous,SYMBOL current,unsigned char adjust)
{ (void)previous; (void)current; (void)adjust; STUB_TRAP(); return 0; }
char *CppOperatorName(unsigned oper);
/* Actual1994 CF31485..31519 performs the old C-linkage ring check
 * locally; the later DeclVerifyNoOtherCLinkage helper is not an App edge. */
void verifyCanBeOverloaded(SYMBOL_NAME sym_name,SYMBOL sym,char *name)
{
    SYMBOL first,ptr;
    if (MainProcedure(sym)) CErr1(396);
    else if (name==CppOperatorName(58)) CErr2p(488,name);
    else if (name==CppOperatorName(60)) CErr2p(488,name);
    else if (sym->id!=7 && ScopeType(sym_name->containing,0)
        && CurrLinkage==CLinkage) {
        first=sym_name->name_syms;
        if (first!=0) {
            ptr=first->next;
            do {
                if (LinkageIsC(ptr)) { CErr2p(617,(const char *)ptr); break; }
                if (ptr==first) break;
                ptr=ptr->next;
            } while (1);
        }
    }
}

TYPE baseTypeInfo(TYPE type,unsigned *info);
void handleInlineFunction(SYMBOL sym);
/* Actual1994 CF30f6c..30fe7 has only the old CONST default-storage
 * rule, before the later DLLIMPORT condition. */
void DeclDefaultStorageClass(SCOPE scope,SYMBOL sym)
{
    TYPE fn_type;
    unsigned info;
    if (sym->id==7) return;
    if (scope->id==0) {
        fn_type=FunctionDeclarationType(sym->sym_type);
        if (fn_type!=0 && (fn_type->flag&8)) handleInlineFunction(sym);
    }
    if (sym->id!=0) return;
    switch (scope->id) {
    case 1: sym->id=9; break;
    case 0:
        baseTypeInfo(sym->sym_type,&info);
        if (info&2) sym->id=2;
        break;
    case 2: case 3: sym->id=3; break;
    }
}
TYPE baseTypeInfo(TYPE type,unsigned *info)
{ (void)type; (void)info; STUB_TRAP(); return 0; }
void handleInlineFunction(SYMBOL sym) { (void)sym; STUB_TRAP(); }

void CheckFunctionDSpec(DECL_SPEC *spec);
DECL_INFO *InsertDeclInfo(SCOPE scope,DECL_INFO *info);
SCOPE SymScope(SYMBOL sym);
int LinkageIsCurr(SYMBOL sym);
void LinkagePushC(void);
void LinkagePushCpp(void);
void PTypeForceLinkagePush(DECL_SPEC *spec);
DECL_INFO *DeclFunction(DECL_SPEC *spec,DECL_INFO *info)
{
    SYMBOL sym;
    if (spec) { CheckFunctionDSpec(spec); info->flags|=0x10; }
    sym=info->sym;
    if (!SymIsFunction(sym)) {
        CErr2p(spec ? 238 : 336,info->name);
        return info;
    }
    info->flags|=4;
    info=InsertDeclInfo(CurrScope,info);
    sym=info->sym;
    if (sym && spec && !spec->linkage && ScopeType(SymScope(sym),0)) {
        if (!LinkageIsCurr(sym)) {
            if (LinkageIsC(sym)) LinkagePushC(); else LinkagePushCpp();
            PTypeForceLinkagePush(spec);
        }
    }
    return info;
}

int SymIsTypedef(SYMBOL sym);
int SymIsStaticMember(SYMBOL sym);
int SymIsInitialized(SYMBOL sym);
TYPE TypeAbstract(TYPE type);
int TypeDefined(TYPE type);
int TemplateMemberCanBeIgnored(void);
int TemplateVerifyDecl(SYMBOL sym);
int diagnoseAbstractDefn(SYMBOL sym,TYPE type)
{ (void)sym; (void)type; STUB_TRAP(); return 0; }
int ConstNeedsExplicitInitializer(TYPE type)
{ (void)type; STUB_TRAP(); return 0; }
int DeclNoInit(DECL_INFO *info)
{
    SYMBOL sym=info->sym;
    SCOPE scope;
    TYPE type;
    unsigned extra;
    if (!sym) return 0;
    if (SymIsTypedef(sym) || SymIsFunction(sym) || sym->id==1) return 0;
    type=sym->sym_type;
    if (SymIsStaticMember(sym)) {
        if (ScopeType(CurrScope,1)) {
            if (TypeAbstract(type)) return diagnoseAbstractDefn(sym,type);
            return 0;
        }
        CompFlags.external_defn_found=1;
        if (SymIsInitialized(sym)) {
            if (TemplateMemberCanBeIgnored()) return 0;
            CErr2p(489,(const char *)sym);
            return 0;
        }
        if (!TypeDefined(type)) {
            CErr2p(326,(const char *)sym);
            return 0;
        }
        if (TypeAbstract(type)) return diagnoseAbstractDefn(sym,type);
    } else {
        if (!TypeDefined(type)) {
            CErr2p(326,(const char *)sym);
            return 0;
        }
        if (TypeAbstract(type)) return diagnoseAbstractDefn(sym,type);
        scope=SymScope(sym);
        if (ScopeType(scope,5) && TemplateVerifyDecl(sym)) return 0;
        if (sym->id!=2 && ScopeType(scope,0)) {
            CompFlags.external_defn_found=1;
            if (LinkageIsC(sym)) CompFlags.extern_C_defn_found=1;
        }
        if (!ScopeType(scope,0) && !ScopeType(scope,3)) return 0;
        if (SymIsInitialized(sym)) return 0;
    }
    type=baseTypeInfo(type,&extra);
    switch (extra) {
    case 1: CErr1(394); return 0;
    case 2:
        if (ConstNeedsExplicitInitializer(type)) { CErr1(178); return 0; }
    }
    return 1;
}
