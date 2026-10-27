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
int commonCheck(SYMBOL previous,SYMBOL current,unsigned *control);
int checkFnStorageClass(SYMBOL previous,SYMBOL current,unsigned *control)
{
    if(commonCheck(previous,current,control)) return 1;
    /* The1994 default-argument entries are accepted directly; later
     * SymDefaultBase comparisons and alias checks are absent. */
    if(current->id==7) return 1;
    if(previous->id==7) { CErr2p(618,(const char *)previous); return 0; }
    switch(current->id) {
    case 0: break;
    case 1:
        if(previous->id==0) previous->id=1;
        else if(previous->id!=1) {
            CErr2p(328,(const char *)previous); return 0;
        }
        break;
    case 2:
        if(previous->id==0) previous->id=2;
        else if(previous->id!=2) {
            CErr2p(328,(const char *)previous); return 0;
        }
        break;
    default:
        if(previous->id!=7 && previous->id!=current->id) {
            CErr2p(328,(const char *)previous); return 0;
        }
    }
    return 1;
}
TYPE TypeModFlagsBaseEC(TYPE type,unsigned short *flags,void **base);
int TypeBasesEqual(unsigned short flags,void *previous,void *current);
int LinkageSpecified(void);
void *TypeHasPragma(TYPE type);
TYPE MakeCombinedFunctionType(TYPE previous,TYPE current);
int SymIsInitialized(SYMBOL sym);
void SymLocnCopy(SYMBOL destination,SYMBOL source);
SYMBOL combineFunctions(SYMBOL previous,SYMBOL current)
{
    void *prev_base,*curr_base,*prev_pragma,*curr_pragma;
    unsigned short prev_flags,curr_flags,prev_fn_flags,curr_fn_flags;
    TYPE prev_type,curr_type,unmod_previous,unmod_current;
    if(previous->id==7 || current->id==7) return previous;
    prev_type=previous->sym_type;
    curr_type=current->sym_type;
    unmod_previous=TypeModFlagsBaseEC(prev_type,&prev_flags,&prev_base);
    unmod_current=TypeModFlagsBaseEC(curr_type,&curr_flags,&curr_base);
    if(((prev_flags^curr_flags)&~0x1800)!=0 ||
       ((prev_flags&0x700) && !TypeBasesEqual(prev_flags,prev_base,curr_base)))
        CErr2p(329,(const char *)previous);
    prev_fn_flags=unmod_previous->flag;
    curr_fn_flags=unmod_current->flag;
    if((prev_fn_flags^curr_fn_flags)&4) {
        if(LinkageSpecified()) CErr2p(619,(const char *)previous);
    } else if((~prev_fn_flags)&curr_fn_flags&0x50)
        CErr2p(632,(const char *)previous);
    curr_pragma=TypeHasPragma(unmod_current);
    if(curr_pragma!=NULL) {
        prev_pragma=TypeHasPragma(unmod_previous);
        if(prev_pragma==NULL || prev_pragma!=curr_pragma)
            CErr2p(670,(const char *)previous);
    }
    /* Older combined types take two arguments, with no later modifier delta
     * or argument-qualifier diagnostic in this owner. */
    previous->sym_type=MakeCombinedFunctionType(prev_type,curr_type);
    if(!SymIsInitialized(previous)) SymLocnCopy(previous,current);
    return previous;
}
SCOPE SymScope(SYMBOL sym);
int LinkageSpecified(void);
int LinkageIsCurr(SYMBOL sym);
TYPE TypeModFlagsBaseEC(TYPE type,unsigned short *flags,void **base);
int TypeBasesEqual(unsigned short flags,void *previous,void *current);
TYPE DupArrayTypeForArray(TYPE type,unsigned size);
int SymIsStaticMember(SYMBOL sym);
int memModelModifiersOK(unsigned short previous,unsigned short current,unsigned char *adjust)
{
    previous&=~0x1800;
    current&=~0x1800;
    if((previous^current)&~0x73c) return 0;
    previous&=0x73c;
    current&=0x73c;
    if(previous==8 && current==0x100) { *adjust|=1; return 1; }
    return previous==0x100 && current==8;
}
int combinableSyms(SYMBOL previous,SYMBOL current,unsigned char *adjust)
{
    TYPE prev_type=previous->sym_type,curr_type=current->sym_type;
    TYPE prev_unmod,curr_unmod;
    unsigned short prev_flags,curr_flags;
    void *prev_base,*curr_base;
    unsigned size;
    int ret=1,file_scope=0;
    if(ScopeType(SymScope(current),0)) {
        file_scope=1;
        if(LinkageSpecified() && !LinkageIsCurr(previous)) {
            CErr2p(619,(const char *)previous);
            ret=0;
        }
    }
    if(TypesIdentical(prev_type,curr_type)) return ret;
    prev_unmod=TypeModFlagsBaseEC(prev_type,&prev_flags,&prev_base);
    curr_unmod=TypeModFlagsBaseEC(curr_type,&curr_flags,&curr_base);
    if((prev_flags^curr_flags)&~0x1800) {
        if(!memModelModifiersOK(prev_flags,curr_flags,adjust)) {
            CErr2p(329,(const char *)previous); ret=0;
        }
    } else if((prev_flags&0x700) && !TypeBasesEqual(prev_flags,prev_base,curr_base)) {
        CErr2p(329,(const char *)previous); ret=0;
    }
    if(TypesIdentical(prev_unmod,curr_unmod)) return ret;
    if(prev_unmod->id==TYP_ARRAY && curr_unmod->id==TYP_ARRAY) {
        size=curr_unmod->u.a.array_size;
        if(prev_unmod->u.a.array_size==0) {
            if(size && TypesIdentical(prev_unmod->of,curr_unmod->of)) {
                previous->sym_type=DupArrayTypeForArray(prev_type,size);
                return ret;
            }
        } else if((SymIsStaticMember(previous)||file_scope) && size==0 &&
                  TypesIdentical(prev_unmod->of,curr_unmod->of)) return ret;
    }
    return 0;
}
int SymIsInitialized(SYMBOL sym);
int commonCheck(SYMBOL previous,SYMBOL current,unsigned *control)
{
    if(SymIsStaticMember(previous) && (current->id==0 || current->id==2)) {
        *control=1;
        return 1;
    }
    if(ScopeType(SymScope(previous),0)) {
        switch(previous->id) {
        case 1:
            if(current->id==0) {
                if(SymIsInitialized(previous)) *control=1;
            } else if(current->id!=1) return 0;
            previous->id=current->id;
            return 1;
        case 10:
            if(current->id==0) { *control=1; return 1; }
            return current->id==1;
        case 2: return current->id==1;
        }
    }
    return 0;
}
int checkVarStorageClass(SYMBOL previous,SYMBOL current,unsigned *control)
{
    if(commonCheck(previous,current,control)) return 1;
    *control=1;
    if(current->id!=0 && current->id!=8 && previous->id!=8 && previous->id!=current->id)
        CErr2p(328,(const char *)previous);
    return 0;
}
void SymLocnCopy(SYMBOL previous,SYMBOL current);
TYPE TypeOverrideModifiers(TYPE previous,TYPE current);
SYMBOL combineVariables(SYMBOL previous,SYMBOL current,unsigned char adjust)
{
    if(!SymIsInitialized(previous)) SymLocnCopy(previous,current);
    if(adjust&1) previous->sym_type=TypeOverrideModifiers(previous->sym_type,current->sym_type);
    return previous;
}
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

typedef unsigned char BASE_INFO;
enum { IS_REF=1,IS_CONST=2,IS_NULL=0 };
TYPE baseTypeInfo(TYPE type,BASE_INFO *info);
void handleInlineFunction(SYMBOL sym);
/* Actual1994 CF30f6c..30fe7 has only the old CONST default-storage
 * rule, before the later DLLIMPORT condition. */
void DeclDefaultStorageClass(SCOPE scope,SYMBOL sym)
{
    TYPE fn_type;
    BASE_INFO info;
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
TYPE TypeGetActualFlags(TYPE type,unsigned short *flags);
TYPE baseTypeInfo(TYPE type,BASE_INFO *info)
{
    unsigned short flags;
    *info=IS_NULL;
    for(;;) {
        type=TypeGetActualFlags(type,&flags);
        if(flags&1) *info|=IS_CONST;
        switch(type->id) {
        case TYP_POINTER:
            if(type->flag&1) *info|=IS_REF;
            break;
        case TYP_ARRAY:
            type=type->of;
            continue;
        }
        return type;
    }
}
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
int TypeAbstract(TYPE type);
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
    BASE_INFO extra;
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

int ScopeEquivalent(SCOPE scope,unsigned char id);
int DeclWithInit(DECL_INFO *info)
{
    SYMBOL sym=info->sym;
    TYPE type;
    if(sym==NULL) return 0;
    if(SymIsTypedef(sym)) { CErr1(327); return 0; }
    if(SymIsFunction(sym)) { CErr1(330); return 0; }
    if(SymIsInitialized(sym)) {
        if(SymIsStaticMember(sym) && TemplateMemberCanBeIgnored()) return 0;
        CErr2p(489,(const char *)sym); return 0;
    }
    if(sym->id==10) { CErr2p(42,(const char *)sym); return 0; }
    if(ScopeEquivalent(SymScope(sym),0)) {
        if(ScopeType(CurrScope,5) && TemplateVerifyDecl(sym)) return 0;
        if(sym->id!=2) {
            CompFlags.external_defn_found=1;
            if(LinkageIsC(sym)) CompFlags.extern_C_defn_found=1;
        }
        if(!ScopeType(CurrScope,0)) CErr2p(87,(const char *)sym);
    } else if(SymIsStaticMember(sym) && !ScopeType(CurrScope,1)) {
        CompFlags.external_defn_found=1;
    }
    type=sym->sym_type;
    if(TypeDefined(type)) {
        if(TypeAbstract(type)) return diagnoseAbstractDefn(sym,type);
        return 1;
    }
    type=TypedefModifierRemove(type);
    if(type->id==TYP_ARRAY && type->u.a.array_size==0 && TypeDefined(type->of)) {
        if(TypeAbstract(type->of)) return diagnoseAbstractDefn(sym,type->of);
        return 1;
    }
    CErr2p(326,(const char *)sym);
    return 0;
}
