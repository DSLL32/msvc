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

/* The 1994 ClassInit has one16-byte base-class pool, before the later
 * separate VF_HIDE pool. The live class index is a16-bit value. */
#include <string.h>
#include "types.h"
#include "scan.h"
#include "macro.h"
#include "rewrite.h"
#include "class.h"
#include "cmdline.h"
#include "frontend.h"
#include "scope.h"
#include "symbol.h"
#include "ptree.h"
#include "symlocn.h"
#include "template.h"
#include "pending.h"
#include <recon.h>
TYPE StructType(TYPE type);
int ClassCorrupted(TYPE type)
{
    type=StructType(type);
    return type==NULL || type->u.c.info->corrupted;
}
static carve_t carveBASE_CLASS;
static unsigned short classIndex;

void ClassInit(void)
{
    carveBASE_CLASS = CarveCreate(16, 16);
    classIndex = 0;
}

static CLASS_DATA *classData;
void ClassPush(CLASS_DATA *state)
{
    CLASS_DATA *old=classData;
    classData=state;
    state->next=old;
}
void CErrCheckpoint(unsigned *state);
void ProcessClassModifiers(TYPE modifiers,unsigned short *flags,void **pragma)
{ (void)modifiers; (void)flags; (void)pragma; STUB_TRAP(); }
void ClassInitState(unsigned short flags,unsigned char extra,TYPE modifiers)
{
    CLASS_DATA *data=classData;
    unsigned packing=PackAmount;
    if (extra&1) packing=1;
    data->sym=0;
    data->type=0;
    data->scope=0;
    data->info=0;
    data->bases=0;
    data->base_vbptr=0;
    data->base_vfptr=0;
    data->name=0;
    data->inlines=0;
    data->offset=0;
    data->vb_offset=0;
    data->vf_offset=0;
    data->vf_index=0;
    data->vb_index=0;
    data->max_align=0;
    data->pack_amount=packing;
    data->tflag=flags;
    /* Original preserves nine unused uninitialized flag bits. */
    data->flags=CD_IS_EXPLICIT;
    if (modifiers) {
        ProcessClassModifiers(modifiers,&data->fn_flags,&data->fn_pragma);
        data->flags|=CD_MEMBER_MOD_ADJUST;
    }
    if (extra&2) data->flags|=CD_CLASS_TEMPLATE;
    data->inline_data=data->next ? data->next->inline_data : 0;
    switch (CurrScope->id) {
    case 3: data->flags|=CD_LOCAL_CLASS; break;
    case 1: data->flags|=CD_NESTED_CLASS; break;
    case 5: data->tflag|=8; break;
    }
    if (flags&3) {
        data->perm=0;
        if (flags&1) data->flags|=CD_IS_UNION;
    } else data->perm=0x40000000UL;
    CErrCheckpoint(&data->errors);
}

/* Recovered 1994 helper ABIs predate the later declaration/id arguments. */
void setScopeOwner(TYPE type,SCOPE scope)
{
    type->u.c.scope=scope;
    ScopeSetClassOwner(scope,type);
}
TYPE createClassType(char *name,unsigned short flags)
{
    TYPE type=MakeClassType();
    SCOPE scope=ScopeCreate(1);
    struct class_info *info;
    type->flag=flags;
    setScopeOwner(type,scope);
    ScopeEstablishEnclosing(scope,CurrScope);
    info=type->u.c.info;
    if (name) info->name=name; else info->unnamed=1;
    info->const_copy=1; info->const_assign=1;
    info->ctor_user_code=1; info->copy_user_code=1;
    info->dtor_user_code=1; info->assign_user_code=1;
    return type;
}
void newClassType(CLASS_DATA *data)
{
    TYPE class_type;
    if (data->flags&0x200) data->type=MakeType(TYP_GENERIC);
    else {
        class_type=createClassType(data->name,data->tflag);
        data->type=class_type;
        data->info=class_type->u.c.info;
        data->scope=class_type->u.c.scope;
    }
}
SYMBOL getClassSym(CLASS_DATA *data)
{
    SYMBOL sym=AllocSymbol();
    sym->id=5;
    sym->sym_type=data->type;
    return sym;
}
SYMBOL InsertSymbol(SCOPE scope,SYMBOL sym,char *name);
void newClassSym(CLASS_DATA *data,unsigned char declaration,PTREE id)
{
    SYMBOL sym=getClassSym(data);
    SymbolLocnDefine(&id->locn,sym);
    if (declaration==1) InsertSymbol(CurrScope,sym,data->name);
    else data->sym=sym;
}
TYPE ClassTagDefinition(TYPE type,char *name)
{ (void)type; (void)name; STUB_TRAP(); return 0; }
unsigned char processClassTemplate(CLASS_DATA *data,unsigned char declaration)
{
    data->flags|=0x100;
    if (declaration==2) {
        TemplateClassDeclaration(data->name);
        return 0;
    }
    return TemplateClassDefinition(data->name) ? 2 : 1;
}
void setClassType(CLASS_DATA *data,TYPE type)
{ (void)data; (void)type; STUB_TRAP(); }
int TypeDefined(TYPE type);
extern TYPE TypeError;

unsigned char ClassName(PTREE id,unsigned char declaration)
{
    CLASS_DATA *data=classData,*enclosing_data;
    char *name;
    SEARCH_RESULT *result;
    SYMBOL sym;
    TYPE type,class_type;
    if (!id) {
        if (data->flags&CD_CLASS_TEMPLATE) CErr1(407);
        newClassType(data);
        return 0;
    }
    name=id->u.id.name;
    data->name=name;
    if (declaration==3) {
        data->flags|=0x200; /* generic template parameter */
        newClassType(data);
        newClassSym(data,declaration,id);
        PTreeFree(id);
        return 0;
    }
    if (declaration && (data->flags&CD_CLASS_TEMPLATE)) {
        PTreeFree(id);
        return processClassTemplate(data,declaration);
    }
    result=ScopeFindLexicalClassType(CurrScope,name);
    if (result) {
        sym=result->sym_name->name_type;
        type=sym->sym_type;
        class_type=ClassTagDefinition(type,name);
        if (!class_type) {
            ScopeFreeResult(result);
            result=0;
        } else if (data->flags&CD_IS_UNION) {
            if (!(class_type->flag&1)) CErr2p(358,(const char *)class_type);
        } else if (class_type->flag&1) CErr2p(359,(const char *)class_type);
    }
    if (result) {
        if (declaration) {
            if (result->scope==CurrScope) {
                if (declaration==1 && TypeDefined(type)) {
                    CErr2p(126,(const char *)type);
                    newClassType(data);
                } else {
                    if (declaration==1) {
                        if (data->flags&CD_NESTED_CLASS) {
                            enclosing_data=data->next;
                            if (enclosing_data &&
                                ((enclosing_data->perm^sym->flag)&0xc0000000UL))
                                CErr2p(331,(const char *)type);
                        }
                        SymbolLocnDefine(&id->locn,sym);
                    }
                    setClassType(data,type);
                    if (declaration==2) data->flags|=0x100; /* nameless_OK */
                }
            } else {
                newClassType(data);
                newClassSym(data,declaration,id);
            }
        } else {
            data->type=ScopeCheckSymbol(result,sym) ? TypeError : type;
        }
        ScopeFreeResult(result);
    } else {
        newClassType(data);
        newClassSym(data,declaration,id);
    }
    PTreeFree(id);
    return 0;
}

DECL_SPEC *ClassRefDef(void)
{
    CLASS_DATA *data=classData;
    DECL_SPEC *spec=PTypeActualType(data->type);
    if (data->flags&2) spec->type_defined=1;
    else spec->type_elaborated=1;
    if (data->flags&0x100) spec->nameless_allowed=1;
    if (data->sym) {
        if (data->flags&0x200) spec->generic=1;
        spec->typedef_defined=data->sym;
        spec->type_declared=1;
    }
    spec->name=data->name;
    return spec;
}

void ClassPop(CLASS_DATA *data)
{
    classData=data->next;
}

int ClassAnonymousUnion(DECL_SPEC *spec)
{ (void)spec; STUB_TRAP(); return 0; }

TYPE ClassUnboundTemplate(char *name)
{
    return createClassType(name,6);
}

void ClassStoreInlineFunc(DECL_INFO *info)
{
    CLASS_DATA *data=classData->inline_data;
    if(info->scope==NULL) info->scope=CurrScope;
    RingAppend((void **)&data->inlines,info);
}

unsigned short nextClassIndex(void)
{
    unsigned short old=classIndex,next=old+1;
    if (next<old) return old;
    classIndex=next;
    return next;
}
void ClassStart(void)
{
    CLASS_DATA *data=classData;
    struct class_info *info=data->type->u.c.info;
    data->start=data->offset;
    if (!data->inline_data) data->inline_data=data;
    info->index=nextClassIndex();
    info->opened=1;
    ScopeOpen(data->scope);
}

TYPE ScopeClass(SCOPE scope);
TYPE TypeGetActualFlags(TYPE type,unsigned short *flags);
TYPE MakeModifiedType(TYPE type,unsigned short flags);
SYMBOL SymDefaultBase(SYMBOL sym);
unsigned short ExplicitModifierFlags(TYPE type);
TYPE ArrayBaseType(TYPE type);
char *CppConstructorName(void);
char *CppDestructorName(void);
char *CppOperatorName(unsigned char op);
int ScopeFindVirtual(SCOPE scope,SYMBOL *sym,char *name,unsigned *return_thunk);
TYPE FunctionDeclarationType(TYPE type);
TYPE MakePlusPlusFunction(TYPE type);
TYPE MakePureVirtualFunction(TYPE type);
TYPE MakeVirtualFunction(TYPE type);
int TypeHasEllipsisArg(TYPE type);
SYMBOL SymDefaultBase(SYMBOL sym);
int isDefaultCopy(SYMBOL sym,TYPE type,unsigned *arg_info);
int hasNonRefArg(SYMBOL sym,TYPE type)
{ (void)sym; (void)type; STUB_TRAP(); return 0; }
void verifyCtor(CLASS_DATA *data,SYMBOL sym)
{
    unsigned args;
    if (isDefaultCopy(sym,data->type,&args)) {
        if (args&1) data->flags|=CD_HAS_CONST_COPY;
        else data->flags|=CD_HAS_NONCONST_COPY;
        if (!(args&2) || (sym->id==7 && hasNonRefArg(SymDefaultBase(sym),data->type))) {
            CErr1(289);
            data->info->corrupted=1;
        }
    }
}
void clearInheritedVFIndices(struct base_class *bases)
{ (void)bases; STUB_TRAP(); }
TYPE MakeVFTableFieldType(int add_consts);
long addTypeField(CLASS_DATA *data,TYPE type);
void createVFPtrField(CLASS_DATA *data,unsigned char creation)
{
    if(creation) data->vf_offset=addTypeField(data,MakeVFTableFieldType(1));
    data->info->has_vfptr=1;
    data->flags|=CD_OWN_VFPTR;
}
int ScopeHasVirtualFunctions(SCOPE scope);
void checkAllBasesForVDtor(struct class_info *info)
{
    struct base_class *last=info->bases,*curr;
    TYPE type;
    struct class_info *base;
    if(last!=NULL) {
        curr=last->next;
        do {
            type=curr->type;
            base=type->u.c.info;
            if(base->needs_dtor && !base->needs_vdtor) {
                if(ScopeHasVirtualFunctions(type->u.c.scope) || (curr->flag&7)!=1)
                    CErr2p(4,(char *)type);
            }
            if(curr==last) break;
            curr=curr->next;
        } while(1);
    }
}
int TypeHasNumArgs(TYPE type,unsigned count);
TYPE ClassTypeForType(TYPE type);
TYPE ReferenceType(TYPE type);
TYPE TypeModFlags(TYPE type,unsigned short *flags);
/* Same actual1994 body tests assignment and copy-constructor arguments;
 * the later source's isDefaultAssign is only a forwarding wrapper. */
int isDefaultCopy(SYMBOL sym,TYPE type,unsigned *arg_info)
{
    arg_list *args;
    TYPE arg,test;
    unsigned short flags;
    *arg_info=0;
    if (!TypeHasNumArgs(sym->sym_type,1)) return 0;
    args=SymFuncArgList(sym);
    arg=args->type_list[0];
    test=ClassTypeForType(arg);
    if (!test || test!=type) return 0;
    test=ReferenceType(arg);
    if (!test) test=arg;
    else *arg_info|=2;
    TypeModFlags(test,&flags);
    if (flags&1) *arg_info|=1;
    return 1;
}
/*1994 has the four-argument boolean virtual search, without later VF-hide,
 * calling-convention adjustment or member-modifier propagation. */
void handleFunctionMember(CLASS_DATA *data,SYMBOL sym,char *name)
{
    TYPE type=sym->sym_type,fn=FunctionDeclarationType(type);
    struct class_info *info;
    unsigned vf_index=0,return_thunk,arg_info;
    int is_virtual,is_pure,virtual_above=0;
    SYMBOL search_sym[2];
    if (!(fn->flag&4)) {
        type=MakePlusPlusFunction(type);
        sym->sym_type=type;
    }
    if (sym->id==7) {
        if (name==CppConstructorName()) verifyCtor(data,sym);
        return;
    }
    is_pure=(fn->flag&0x40)!=0;
    is_virtual=(fn->flag&0x10)!=0;
    if (is_virtual) {
        if (sym->id==2) { is_virtual=0; CErr1(376); }
        if (data->flags&CD_IS_UNION) { is_virtual=0; CErr1(109); }
    }
    info=data->info;
    if (name==CppConstructorName()) {
        if (data->flags&CD_IS_EXPLICIT) {
            info->has_ctor=1;
            info->needs_ctor=1;
        }
        verifyCtor(data,sym);
        return;
    }
    if (sym->id==2) return;
    if (data->bases) {
        search_sym[0]=sym;
        virtual_above=ScopeFindVirtual(CurrScope,search_sym,name,&return_thunk);
        if (virtual_above && search_sym[0]) {
            vf_index=search_sym[0]->u.uval;
            if (return_thunk && !(data->flags&CD_OWN_VFPTR)) {
                clearInheritedVFIndices(data->bases);
                createVFPtrField(data,0);
            }
        }
    }
    if (virtual_above) {
        if (!is_virtual) {
            type=is_pure ? MakePureVirtualFunction(type) : MakeVirtualFunction(type);
            sym->sym_type=type;
            is_virtual=1;
        }
    } else if (is_pure && !is_virtual) {
        is_virtual=1;
        CErr1(417);
    }
    if (is_pure) {
        info->has_pure=1;
        info->abstract=1;
        info->abstract_OK=1;
    }
    if (is_virtual) {
        if (!vf_index) {
            if (!data->vf_index) createVFPtrField(data,0);
            vf_index=++data->vf_index;
        }
        sym->u.uval=vf_index;
        info->needs_ctor=1;
        info->has_vfn=1;
        if (TypeHasEllipsisArg(fn)) CErr1(505);
    }
    if (name==CppDestructorName()) {
        if (data->flags&CD_IS_EXPLICIT) {
            info->has_dtor=1;
            info->needs_dtor=1;
        }
        if (is_virtual) {
            info->needs_vdtor=1;
            checkAllBasesForVDtor(info);
        }
        if (is_pure) CErr1(621);
    } else if (name==CppOperatorName(13)) {
        if (data->flags&CD_IS_EXPLICIT) info->needs_assign=1;
        if (isDefaultCopy(sym,data->type,&arg_info)) {
            if (arg_info&1) data->flags|=CD_HAS_CONST_OPEQ;
            else data->flags|=CD_HAS_NONCONST_OPEQ;
        }
    }
}
void verifyClassMember(CLASS_DATA *data,TYPE type,unsigned char is_static)
{ (void)data; (void)type; (void)is_static; STUB_TRAP(); }
unsigned CgMemorySize(TYPE type);
TYPE AlignmentType(TYPE type);
void doAlignment(CLASS_DATA *data,unsigned adjustment)
{
    unsigned offset=(data->offset+adjustment-1)&~(adjustment-1);
    if (offset<data->offset) CErr1(85);
    data->offset=offset;
}
void abandonBitfield(TYPE type)
{
    CLASS_DATA *data=classData;
    if ((data->flags&1) || !type) data->flags&=~1;
    else doAlignment(data,CgMemorySize(type));
}
unsigned PackAlignment(unsigned packing,unsigned size)
{
    if (packing==1) return 1;
    switch (size) {
    case 1: return 1;
    case 2: if (packing>2) return 2; break;
    case 4: if (packing>4) return 4; break;
    }
    return packing;
}
unsigned packSize(CLASS_DATA *data,TYPE type)
{
    unsigned size=CgMemorySize(AlignmentType(type));
    return PackAlignment(data->pack_amount,size);
}
unsigned addField(CLASS_DATA *data,unsigned size,unsigned packing)
{
    unsigned old_end=data->offset,start,end;
    if (packing>data->max_align) data->max_align=packing;
    if (data->flags&CD_IS_UNION) data->offset=data->start;
    doAlignment(data,packing);
    start=data->offset;
    end=start+size;
    if (end<start) CErr1(85);
    data->offset=end;
    if ((data->flags&CD_IS_UNION) && old_end>end) data->offset=old_end;
    return start;
}
long addTypeField(CLASS_DATA *data,TYPE type)
{
    unsigned packing=packSize(data,type);
    unsigned size=CgMemorySize(type);
    return addField(data,size,packing);
}
long addZeroSizedField(CLASS_DATA *data,TYPE type)
{
    return addField(data,0,packSize(data,type));
}
CLASS_DATA *findClassData(SCOPE scope)
{
    CLASS_DATA *data;
    for (data=classData;data;data=data->next) {
        if (data->scope==scope) break;
    }
    return data;
}
void setHasData(CLASS_DATA *data)
{
    if (data->flags&CD_ZERO_ARRAY_DEFD) CErr1(681);
    data->info->has_data=1;
}
/* Old1994 member layout predates mutable, separate modifier propagation and
 * ArrayModifiedBaseType; preserve its explicit memory-model mask and offsets. */
void ClassMember(SCOPE scope,SYMBOL sym)
{
    CLASS_DATA *data=findClassData(scope);
    TYPE type,no_mem_model,base_type;
    char *name,*scope_name;
    unsigned short modifiers;
    unsigned char is_static=0,zero_array=0;
    if (!data) {
        if (sym->id==7) CErr2p(88,(const char *)SymDefaultBase(sym));
        return;
    }
    sym->flag|=data->perm;
    switch (data->perm) {
    case 0x40000000UL: data->flags|=CD_A_PRIVATE; break;
    case 0x80000000UL:
        if (data->flags&CD_IS_UNION) CErr1(311);
        data->flags|=CD_A_PROTECTED;
        break;
    default: data->flags|=CD_A_PUBLIC;
    }
    name=sym->name->name;
    scope_name=ScopeClass(scope)->u.c.info->name;
    switch (sym->id) {
    case 5:
        if (!(data->flags&CD_ALLOW_TYPEDEF) && name==scope_name) CErr2p(284,name);
        return;
    case 8:
        if (name==scope_name) CErr2p(285,name);
        return;
    case 6: return;
    case 2:
        is_static=1;
        if (name==scope_name) CErr2p(286,name);
        if ((data->flags&CD_IS_UNION) && !SymIsFunction(sym)) CErr1(168);
        break;
    case 1: case 3: case 4:
        CErr1(169);
        sym->id=0;
        break;
    }
    if (SymIsFunction(sym)) {
        handleFunctionMember(data,sym,name);
        return;
    }
    type=TypeGetActualFlags(sym->sym_type,&modifiers);
    if ((modifiers&1) && !is_static) data->flags|=CD_A_CONST;
    if (type->id==TYP_BITFIELD) {
        sym->u.uval=data->boffset;
        setHasData(data);
        return;
    }
    switch (type->id) {
    case TYP_CLASS: verifyClassMember(data,type,is_static); break;
    case TYP_POINTER:
        if ((type->flag&1) && !is_static) data->flags|=CD_A_REFERENCE;
        break;
    case TYP_ARRAY:
        if (CompFlags.extensions_enabled && !type->u.a.array_size && !is_static) {
            if (data->bases) CErr2p(683,(const char *)sym);
            type->u.a.array_size=1;
            data->info->zero_array=1;
            zero_array=1;
        }
        base_type=ArrayBaseType(type);
        if (base_type->id==TYP_CLASS) verifyClassMember(data,base_type,is_static);
        break;
    }
    if (!data->bases) abandonBitfield(NULL);
    if (!is_static) {
        if (modifiers&0x73c) {
            no_mem_model=type;
            if (ExplicitModifierFlags(sym->sym_type)&0x73c) CErr1(582);
            else {
                modifiers&=~0x73c;
                if (modifiers) no_mem_model=MakeModifiedType(type,modifiers);
            }
            sym->sym_type=no_mem_model;
        }
        setHasData(data);
        if (zero_array) {
            data->flags|=CD_ZERO_ARRAY_DEFD;
            sym->u.uval=addZeroSizedField(data,type);
        } else sym->u.uval=addTypeField(data,type);
    } else if (data->flags&CD_LOCAL_CLASS) CErr1(322);
}

void ClassAddFunctionMods(TYPE type)
{
    CLASS_DATA *data=classData;
    if (data && (data->flags&CD_MEMBER_MOD_ADJUST)) {
        type->flag|=data->fn_flags;
        type->u.f.pragma=data->fn_pragma;
    }
}

void ClassPermission(unsigned perm) { classData->perm=perm; }
SYMBOL ClassAddDefaultCtor(SCOPE scope)
{ (void)scope; STUB_TRAP(); return NULL; }
char *CppConstructorName(void);
TYPE MakeConstTypeOf(TYPE type);
TYPE MakeReferenceTo(TYPE type);
TYPE __cdecl MakeModifiableFunction(TYPE result,...);
SEARCH_RESULT *ScopeContainsMember(SCOPE scope,char *name);
SYMBOL checkPresence(SCOPE scope,char *name)
{
    SEARCH_RESULT *result=ScopeContainsMember(scope,name);
    SYMBOL syms=NULL;
    if(result!=NULL) {
        syms=result->sym_name->name_syms;
        ScopeFreeResult(result);
    }
    return syms;
}
/* Disambiguated from scope.c's unrelated findMember lookup implementation. */
SYMBOL classFindMember(SYMBOL syms,TYPE type,int (*check)(SYMBOL,TYPE))
{
    SYMBOL sym;
    if(syms!=NULL) {
        sym=syms->next;
        do {
            if(check(sym,type)) return sym;
            if(sym==syms) break;
            sym=sym->next;
        } while(1);
    }
    return NULL;
}
int ClassIsDefaultCopy(SYMBOL sym,TYPE type)
{
    unsigned arg_info;
    return isDefaultCopy(sym,type,&arg_info);
}
TYPE addClassFnMods(SCOPE scope,TYPE type)
{ (void)scope; (void)type; STUB_TRAP(); return NULL; }
SYMBOL insertDefaultFunc(SCOPE scope,TYPE type,char *name)
{ (void)scope; (void)type; (void)name; STUB_TRAP(); return NULL; }
TYPE buildCopyAssign(SCOPE scope,unsigned short flags,TYPE arg_type,TYPE class_type)
{
    TYPE fn_type,ret_type;
    arg_type=MakeReferenceTo(arg_type);
    ret_type=MakeReferenceTo(class_type);
    fn_type=MakeModifiableFunction(ret_type,arg_type,(TYPE)NULL);
    fn_type->flag|=flags;
    fn_type=addClassFnMods(scope,fn_type);
    return CheckDupType(fn_type);
}
SYMBOL ClassAddDefaultCopy(SCOPE scope)
{
    TYPE class_type=ScopeClass(scope),arg_type,fn_type;
    char *name=CppConstructorName();
    SYMBOL syms=checkPresence(scope,name),sym;
    struct class_info *info;
    unsigned short flags=0;
    if(syms!=NULL && classFindMember(syms,class_type,ClassIsDefaultCopy)!=NULL) return NULL;
    info=class_type->u.c.info;
    arg_type=class_type;
    if(info->const_copy) arg_type=MakeConstTypeOf(class_type);
    if((int)OptSize<=50) flags=8;
    fn_type=buildCopyAssign(scope,flags,arg_type,class_type);
    sym=insertDefaultFunc(scope,fn_type,name);
    info->copy_defined=1;
    info->copy_user_code=0;
    return sym;
}
SYMBOL ClassAddDefaultDtor(SCOPE scope)
{ (void)scope; STUB_TRAP(); return NULL; }
SYMBOL ClassAddDefaultAssign(SCOPE scope)
{ (void)scope; STUB_TRAP(); return NULL; }
int addDefaultCtor(CLASS_DATA *data,SCOPE scope,TYPE type)
{
    struct class_info *info=type->u.c.info;
    if(info->needs_ctor || (data->flags&CD_MEMBER_MOD_ADJUST)) {
        if(!info->has_ctor) { ClassAddDefaultCtor(scope); return 1; }
    } else info->ctor_user_code=0;
    return 0;
}
int addDefaultCopy(CLASS_DATA *data,SCOPE scope,TYPE type)
{
    struct class_info *info=type->u.c.info;
    if(info->needs_ctor || (data->flags&CD_MEMBER_MOD_ADJUST)) {
        if(ClassAddDefaultCopy(scope)!=NULL) return 1;
    } else info->copy_user_code=0;
    return 0;
}
int addDefaultDtor(CLASS_DATA *data,SCOPE scope,TYPE type)
{
    struct class_info *info=type->u.c.info;
    if(info->needs_dtor || (data->flags&CD_MEMBER_MOD_ADJUST)) {
        if(!info->has_dtor) { ClassAddDefaultDtor(scope); return 1; }
    } else info->dtor_user_code=0;
    return 0;
}
int addDefaultOpEq(CLASS_DATA *data,SCOPE scope,TYPE type)
{
    struct class_info *info=type->u.c.info;
    if(info->needs_assign || (data->flags&CD_MEMBER_MOD_ADJUST)) {
        if(!info->const_ref && ClassAddDefaultAssign(scope)!=NULL) return 1;
    } else info->assign_user_code=0;
    return 0;
}
void setAbstractStatus(CLASS_DATA *data,struct class_info *info)
{
    struct base_class *last=data->bases,*base;
    struct class_info *base_info;
    info->abstract_OK=1;
    if(last!=NULL) {
        base=last->next;
        do {
            base_info=StructType(base->type)->u.c.info;
            if(!base_info->abstract_OK || base_info->abstract) {
                info->abstract_OK=0;
                break;
            }
            if(base==last) break;
            base=base->next;
        } while(1);
    }
}
int TypeCtorDispRequired(TYPE derived,TYPE base)
{ (void)derived; (void)base; STUB_TRAP(); return 0; }
unsigned nonVirtualField(TYPE type)
{ return addField(classData,type->u.c.info->vsize,type->u.c.info->max_align); }
struct friend_entry *ScopeFriends(SCOPE scope);
void checkClassStatus(CLASS_DATA *data)
{
    struct class_info *info;
    if((data->flags&CD_A_PRIVATE) && !(data->flags&CD_A_PUBLIC) && !(data->flags&CD_A_PROTECTED)) {
        if(data->bases==NULL && ScopeFriends(data->scope)==NULL) CErr1(11);
    }
    info=data->info;
    if(!info->has_ctor) {
        if(data->flags&CD_A_CONST) CErr1(370);
        if(data->flags&CD_A_REFERENCE) CErr1(371);
    }
}
void unfinishedClass(SYMBOL_NAME name)
{
    SYMBOL sym=name->name_type;
    TYPE type;
    char *type_name;
    if(sym==NULL) return;
    type=sym->sym_type;
    if(type->id!=TYP_TYPEDEF) return;
    type_name=type->u.t.sym->name->name;
    type=type->of;
    if(type->id!=TYP_CLASS || type->u.c.info->name!=type_name || type->u.c.info->defined) return;
    CErr2p(263,name->name);
}
void ScopeWalkNames(SCOPE scope,void (*walker)(SYMBOL_NAME));
void checkForUnfinishedNestedClasses(CLASS_DATA *data)
{ ScopeWalkNames(data->scope,unfinishedClass); }
void insertCtorTypedef(CLASS_DATA *data)
{
    SEARCH_RESULT *result;
    SYMBOL sym;
    if(data->name==NULL || !data->info->needs_ctor) return;
    result=ScopeContainsMember(data->scope,data->name);
    if(result!=NULL) { ScopeFreeResult(result); return; }
    sym=getClassSym(data);
    SymbolLocnDefine(NULL,sym);
    data->flags|=CD_ALLOW_TYPEDEF;
    InsertSymbol(data->scope,sym,data->name);
    data->flags&=~CD_ALLOW_TYPEDEF;
}
void SrcFileGetTokenLocn(CGIO_LOCATION *location);
void SrcFileResetTokenLocn(CGIO_LOCATION *location);
void ParseFlush(void);
void FunctionBody(DECL_INFO *info);
TYPE MakeInlineFunction(TYPE type);
void changeToInlineFunction(DECL_INFO *info)
{
    SYMBOL sym=info->sym;
    if(sym!=NULL && FunctionDeclarationType(sym->sym_type)!=NULL)
        sym->sym_type=MakeInlineFunction(sym->sym_type);
}
/* The1994 state has one deferred inline ring; the later separate class
 * default-argument rewrite list is absent. */
void defineInlineFuncsAndDefArgExprs(CLASS_DATA *data)
{
    DECL_INFO *curr;
    CGIO_LOCATION location;
    TOKEN_SOURCE last_source;
    REWRITE *last_rewrite;
    if(CurToken!=12) Expecting(Tokens[12]);
    SrcFileGetTokenLocn(&location);
    while((curr=RingPop((void **)&data->inlines))!=NULL) {
        ParseFlush();
        last_source=SetTokenSource(RewriteToken);
        last_rewrite=RewriteRewind(curr->body);
        changeToInlineFunction(curr);
        FunctionBody(curr);
        RewriteClose(last_rewrite);
        ResetTokenSource(last_source);
        FreeDeclInfo(curr);
    }
    SrcFileResetTokenLocn(&location);
    CurToken=12;
    strcpy(Buffer,Tokens[12]);
}
void TemplateSpecificDefnEnd(void);
int CErrOccurred(unsigned *state);
DECL_SPEC *ClassEnd(void)
{
    CLASS_DATA *data=classData;
    TYPE type;
    SCOPE scope;
    struct class_info *info;
    struct base_class *base,*last;
    unsigned size_of_vbases;
    long offset;
    data->flags&=~CD_IS_EXPLICIT;
    ClassPermission(0);
    type=data->type;
    scope=data->scope;
    info=data->info;
    if(data->flags&(CD_A_CONST|CD_A_REFERENCE)) info->const_ref=1;
    if(data->flags&CD_HAS_CONST_COPY) info->const_copy=1;
    else if(data->flags&CD_HAS_NONCONST_COPY) info->const_copy=0;
    if(data->flags&CD_HAS_CONST_OPEQ) info->const_assign=1;
    else if(data->flags&CD_HAS_NONCONST_OPEQ) info->const_assign=0;
    addDefaultCtor(data,scope,type);
    addDefaultCopy(data,scope,type);
    addDefaultDtor(data,scope,type);
    addDefaultOpEq(data,scope,type);
    if(data->flags&CD_OWN_VFPTR) createVFPtrField(data,1);
    info=data->info;
    data->flags|=CD_DEFINED;
    offset=data->vb_offset;
    if(data->base_vbptr!=NULL) offset+=data->base_vbptr->delta;
    info->vb_offset=offset;
    offset=data->vf_offset;
    if(data->base_vfptr!=NULL) offset+=data->base_vfptr->delta;
    info->vf_offset=offset;
    info->last_vfn=data->vf_index;
    info->last_vbase=data->vb_index;
    if((info->has_ctor || info->has_dtor) && info->has_vfn) info->has_vcdtor=1;
    if(!info->abstract_OK) setAbstractStatus(data,info);
    doAlignment(data,data->max_align);
    size_of_vbases=data->offset;
    last=data->bases;
    if(last!=NULL) {
        base=last->next;
        do {
            if(base->flag&0x18) {
                if(TypeCtorDispRequired(type,base->type)) {
                    base->flag|=0x100;
                    addField(data,4,1);
                }
                base->delta=nonVirtualField(base->type);
            }
            if(base==last) break;
            base=base->next;
        } while(1);
    }
    size_of_vbases=data->offset-size_of_vbases;
    if(data->offset==0) addField(data,1,1);
    doAlignment(data,data->max_align);
    info->size=data->offset;
    info->vsize=info->size-size_of_vbases;
    info->max_align=data->max_align;
    info->defined=1;
    checkClassStatus(data);
    checkForUnfinishedNestedClasses(data);
    insertCtorTypedef(data);
    ScopeEnd(1);
    if(data->flags&CD_SPECIFIC_DEFN) TemplateSpecificDefnEnd();
    CheckDupType(data->type);
    if(CErrOccurred(&data->errors)) info->corrupted=1;
    defineInlineFuncsAndDefArgExprs(data);
    return ClassRefDef();
}

void ClassDtorNullBody(SYMBOL sym) { (void)sym; STUB_TRAP(); }
void ClassCtorNullBody(SYMBOL sym) { (void)sym; STUB_TRAP(); }
void ClassAssignNullBody(SYMBOL sym) { (void)sym; STUB_TRAP(); }
int GeneratedDefaultFunction(SYMBOL sym) { (void)sym; STUB_TRAP(); return 0; }

REWRITE *RewritePackageToken(void);
void RewriteRestoreToken(REWRITE *rewrite);
void ParseFlush(void);
PTREE ParseMemInit(void);
SYMBOL ScopeFunctionInProgress(void);
SCOPE SymScope(SYMBOL sym);
int duplicateMemInit(PTREE first,PTREE second)
{ (void)first; (void)second; STUB_TRAP(); return 0; }
int SymIsFunction(SYMBOL sym);
int SymIsStaticMember(SYMBOL sym);
int verifyMemberInit(PTREE member,SCOPE scope)
{
    char *name=member->u.id.name;
    SEARCH_RESULT *result=ScopeContainsMember(scope,name);
    SYMBOL sym;
    if (!result) { CErr2p(424,name); return 1; }
    sym=result->sym_name->name_syms;
    ScopeFreeResult(result);
    if (!sym) { CErr2p(424,name); return 1; }
    if (SymIsFunction(sym)) { CErr2p(425,name); return 1; }
    if (SymIsStaticMember(sym)) { CErr2p(426,name); return 1; }
    return 0;
}
int verifyBaseClassInit(PTREE member,SCOPE scope)
{ (void)member; (void)scope; STUB_TRAP(); return 0; }
int symInitialized(PTREE init,SYMBOL sym)
{
    PTREE current,member;
    char *name=sym->name->name;
    for (current=init;current;current=current->u.subtree[0]) {
        member=current->u.subtree[1]->u.subtree[0];
        if (member->op==6 && member->u.id.name==name) return 1;
    }
    return 0;
}
SYMBOL ScopeOrderedStart(SCOPE scope);
SYMBOL ScopeOrderedNext(SYMBOL stop,SYMBOL current);
int SymIsData(SYMBOL sym);
int verifyAllConstRefsInit(SCOPE scope,PTREE init)
{
    SYMBOL stop=ScopeOrderedStart(scope),current=NULL;
    TYPE type;
    unsigned short flags;
    int error=0;
    for (;;) {
        current=ScopeOrderedNext(stop,current);
        if (!current) break;
        if (!SymIsData(current) || SymIsStaticMember(current)) continue;
        type=TypeModFlags(current->sym_type,&flags);
        if ((flags&1) || (type->id==TYP_POINTER && (type->flag&1))) {
            if (!symInitialized(init,current)) {
                CErr2p(428,(char *)current);
                error=1;
            }
        }
    }
    return error;
}
PTREE verifyMemInit(PTREE tree)
{
    PTREE current,test,member,test_member;
    SCOPE scope=SymScope(ScopeFunctionInProgress());
    int error=0;
    for (current=tree;current;current=current->u.subtree[0]) {
        member=current->u.subtree[1]->u.subtree[0];
        for (test=tree;test!=current;test=test->u.subtree[0]) {
            test_member=test->u.subtree[1]->u.subtree[0];
            if (duplicateMemInit(member,test_member)) { error=1; break; }
        }
        if (member->op==6) {
            if (verifyMemberInit(member,scope)) error=1;
        } else if (verifyBaseClassInit(member,scope)) error=1;
    }
    if (verifyAllConstRefsInit(scope,tree)) error=1;
    if (error) { PTreeFreeSubtrees(tree); tree=NULL; }
    return tree;
}
TYPE SymClass(SYMBOL sym);
PTREE ClassMemInit(SYMBOL ctor,void *mem_initializer)
{
    PTREE mem_init=NULL;
    REWRITE *save_token,*last_rewrite;
    TOKEN_SOURCE last_source;
    if (mem_initializer) {
        save_token=RewritePackageToken();
        last_source=SetTokenSource(RewriteToken);
        last_rewrite=RewriteRewind(mem_initializer);
        ParseFlush();
        mem_init=ParseMemInit();
        ParseFlush();
        RewriteClose(last_rewrite);
        ResetTokenSource(last_source);
        RewriteFree(mem_initializer);
        RewriteRestoreToken(save_token);
        mem_init=verifyMemInit(mem_init);
    } else if (!ClassIsDefaultCopy(ctor,SymClass(ctor)))
        mem_init=verifyMemInit(mem_init);
    return mem_init;
}
