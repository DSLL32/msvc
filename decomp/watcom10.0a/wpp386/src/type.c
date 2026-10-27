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

/* Initial Open Watcom type.c (6b9cb443), adapted to the actual 1994
 * typesInit body. The basic scalar table still carries the old keyword-mask
 * forms, and this version builds its caches directly in this initializer. */
#include <stddef.h>
#include "types.h"
#include "pending.h"
#include "cmdline.h"
#include "symbol.h"
#include "ptree.h"
#include "rewrite.h"
#include "linkage.h"
#include "symlocn.h"
#include "frontend.h"
#include "vstk.h"
#include <recon.h>

TYPE basicTypes[TYP_MAX];
static TYPE *typeHashTables[TYP_MAX];
static TYPE typeTable[TYP_MAX];
static TYPE fnHashTable[8][32], fnTable[8];
static TYPE pointerHashTable[32], bitfieldHashTable[32];
static TYPE arrayHashTable[32], modifierHashTable[32];
carve_t carveTYPE;
static carve_t carveDECL_SPEC, carveCLASSINFO, carveDECL_INFO;
TYPE TypeError;
/* Reached fields of the original compilation information object. */
COMP_INFO_TYPES CompInfoTypes;
static TYPE segment_short, clean_int, default_int, ptr_void;
static TYPE diagnostic_error, ptr_char, ptr_uint, ptr_const_uint;
static TYPE ptr_stdop_void, ptr_const_stdop_void;
static TYPE ptr_stdop_arith_void, ptr_const_stdop_arith_void;
static TYPE void_fun_of_void, void_fun_of_ptr_void;
static TYPE ptr_void_fun_of_void, ptr_void_fun_of_cdtor_arg;
static void *cdeclPragma;

enum {
    STM_CHAR=1, STM_INT=2, STM_LONG=4, STM_FLOAT=8, STM_DOUBLE=16,
    STM_SIGNED=32, STM_UNSIGNED=64, STM_SHORT=128, STM_VOID=256,
    STM_SEGMENT=512
};
#pragma pack(1)
const struct scalar_type {
    type_id id;
    unsigned short scalar;
} scalar_types[] = {
    {TYP_CHAR,STM_CHAR}, {TYP_SCHAR,STM_CHAR|STM_SIGNED},
    {TYP_UCHAR,STM_CHAR|STM_UNSIGNED},
    {TYP_SSHORT,STM_SHORT}, {TYP_SSHORT,STM_SHORT|STM_INT},
    {TYP_SSHORT,STM_SHORT|STM_SIGNED},
    {TYP_SSHORT,STM_SHORT|STM_SIGNED|STM_INT},
    {TYP_USHORT,STM_SHORT|STM_UNSIGNED},
    {TYP_USHORT,STM_SHORT|STM_UNSIGNED|STM_INT},
    {TYP_SINT,STM_SIGNED}, {TYP_SINT,STM_INT},
    {TYP_SINT,STM_SIGNED|STM_INT},
    {TYP_UINT,STM_INT|STM_UNSIGNED}, {TYP_UINT,STM_UNSIGNED},
    {TYP_SLONG,STM_LONG}, {TYP_SLONG,STM_LONG|STM_SIGNED},
    {TYP_ULONG,STM_LONG|STM_UNSIGNED}, {TYP_SLONG,STM_LONG|STM_INT},
    {TYP_SLONG,STM_LONG|STM_INT|STM_SIGNED},
    {TYP_ULONG,STM_LONG|STM_INT|STM_UNSIGNED},
    {TYP_FLOAT,STM_FLOAT}, {TYP_DOUBLE,STM_DOUBLE}, {TYP_VOID,STM_VOID},
    {TYP_LONG_DOUBLE,STM_LONG|STM_DOUBLE},
    {TYP_WCHAR,STM_LONG|STM_CHAR}, {TYP_USHORT,STM_SEGMENT}, {TYP_MAX,0}
};
#pragma pack()

void typesInit(INITFINI *defn)
{
    const struct scalar_type *p;
    TYPE type, canonical;

    (void)defn;
    typeHashTables[TYP_POINTER] = pointerHashTable;
    typeHashTables[TYP_BITFIELD] = bitfieldHashTable;
    typeHashTables[TYP_ARRAY] = arrayHashTable;
    typeHashTables[TYP_MODIFIER] = modifierHashTable;
    carveDECL_SPEC = CarveCreate(34, 8);
    carveTYPE = CarveCreate(sizeof(struct type), 64);
    carveCLASSINFO = CarveCreate(52, 16);
    carveDECL_INFO = CarveCreate(53, 4);
    for (p = scalar_types; p->scalar; ++p) {
        if (!basicTypes[p->id]) basicTypes[p->id] = MakeType(p->id);
    }
    basicTypes[TYP_CHAR]->of = basicTypes[TYP_UCHAR];
    basicTypes[TYP_ERROR] = MakeType(TYP_ERROR);
    basicTypes[TYP_DOT_DOT_DOT] = MakeType(TYP_DOT_DOT_DOT);
    TypeError = basicTypes[TYP_ERROR];
    default_int = MakeType(TYP_SINT);
    default_int->flag |= 1;
    diagnostic_error = MakeType(TYP_ERROR);
    CheckDupType(default_int);
    type = MakeType(TYP_SINT);
    type->flag |= 2;
    clean_int = type;
    CheckDupType(type);
    segment_short = MakeType(TYP_USHORT);
    segment_short->flag |= 1;
    CheckDupType(segment_short);
    CompInfoTypes.ptr_diff_near = basicTypes[TYP_SINT];
    CompInfoTypes.ptr_diff_far = basicTypes[TYP_SINT];
    CompInfoTypes.ptr_diff_far16 = basicTypes[TYP_SSHORT];
    CompInfoTypes.ptr_diff_huge = basicTypes[TYP_SLONG];
    ptr_void = MakePointerTo(GetBasicType(TYP_VOID));
    type = MakeType(TYP_VOID);
    type->flag |= 1;
    canonical = MakeConstTypeOf(type);
    ptr_stdop_void = MakePointerTo(type);
    ptr_const_stdop_void = MakePointerTo(canonical);
    type = MakeType(TYP_VOID);
    type->flag |= 2;
    canonical = MakeConstTypeOf(type);
    ptr_stdop_arith_void = MakePointerTo(type);
    ptr_const_stdop_arith_void = MakePointerTo(canonical);
    ptr_char = MakePointerTo(GetBasicType(TYP_CHAR));
    ptr_uint = MakePointerTo(GetBasicType(TYP_UINT));
    ptr_const_uint = MakePointerTo(MakeConstTypeOf(GetBasicType(TYP_UINT)));
    void_fun_of_void = MakeSimpleFunction(GetBasicType(TYP_VOID), (TYPE)0);
    void_fun_of_ptr_void = MakeSimpleFunction(GetBasicType(TYP_VOID), ptr_void, (TYPE)0);
    ptr_void_fun_of_void = MakeSimpleFunction(ptr_void, GetBasicType(TYP_VOID), (TYPE)0);
    ptr_void_fun_of_cdtor_arg = MakeSimpleFunction(ptr_void, GetBasicUInt(), (TYPE)0);
    ClassInit();
    cdeclPragma = PragmaLookup(0, 2);
}

static unsigned typeHashCtr;

TYPE MakeType(type_id id)
{
    TYPE type;

    type = (TYPE)CarveAlloc(carveTYPE);
    type->next = 0;
    type->of = 0;
    type->u.i.init1 = 0;
    type->u.i.init2 = 0;
    type->flag = 0;
    type->id = id;
    type->dbg_handle = typeHashCtr++;
    type->dbgflag = 8;
    return type;
}

TYPE CheckDupType(TYPE type)
{
    unsigned count;
    type_id id;
    TYPE *head;

    if (type->next) return type;
    id = type->id;
    head = typeHashTables[id];
    if (!head) {
        if (id == TYP_FUNCTION) {
            head = &typeTable[id];
            count = type->u.f.args->num_args;
            if (count < 8) head = &fnHashTable[count][type->of->dbg_handle & 31];
            else if (count < 16) head = &fnTable[count - 8];
            type = typeDuplicated(type, head);
        } else if (id == TYP_MEMBER_POINTER || id == TYP_GENERIC) {
            type = typeDuplicated(type, &typeTable[id]);
        } else {
            type = (TYPE)RingPush((void **)&typeTable[id], type);
        }
    } else {
        if (head == arrayHashTable && !type->u.a.array_size) return type;
        head += type->of->dbg_handle & 31;
        type = typeDuplicated(type, head);
    }
    return type;
}

TYPE GetBasicType(type_id id)
{
    return basicTypes[id];
}

TYPE MakePointerTo(TYPE base)
{
    while (base->id == TYP_TYPEDEF) base = base->of;
    return MakeTypeOf(MakeType(TYP_POINTER), base);
}

TYPE MakeTypeOf(TYPE type, TYPE base)
{
    type->of = base;
    return CheckDupType(type);
}

TYPE typeDuplicated(TYPE type, TYPE *head)
{
    TYPE check, last, previous;
    TYPE base;
    unsigned short flags;

    previous = 0;
    base = type->of;
    flags = type->flag;
    last = *head;
    if (last) {
        check = last;
        do {
            check = check->next;
            if (check->of == base && check->flag == flags && dupCompare(check, type)) {
                typeFree(type);
                return (TYPE)RingPromote((void **)head, check, previous);
            }
            previous = check;
        } while (check != last);
    }
    return (TYPE)RingPush((void **)head, type);
}

TYPE MakeConstTypeOf(TYPE type)
{
    return MakeTypeOf(MakeConstModifier(), type);
}

TYPE MakeConstModifier(void)
{
    return MakeFlagModifier(1);
}

TYPE MakeFlagModifier(unsigned short flag)
{
    return makeFullModifier(flag, 0, 0);
}

TYPE makeFullModifier(unsigned short flag, void *base, void *pragma)
{
    TYPE type;

    type = MakeType(TYP_MODIFIER);
    type->flag = flag;
    type->u.m.base = base;
    type->u.m.pragma = pragma;
    return type;
}

TYPE __cdecl MakeSimpleFunction(TYPE result, ...)
{
    TYPE function;
    va_list count_args, use_args;

    va_start(count_args, result);
    va_start(use_args, result);
    function = buildFnType(result, count_args, use_args);
    return CheckDupType(function);
}

TYPE buildFnType(TYPE result, va_list count_args, va_list use_args)
{
    TYPE argument, function;
    TYPE *current;
    arg_list *args;
    unsigned count;

    count = 0;
    while ((argument = va_arg(count_args, TYPE)) != 0) ++count;
    va_end(count_args);
    args = AllocArgList(count, 1);
    current = args->type_list;
    while ((argument = va_arg(use_args, TYPE)) != 0) *current++ = argument;
    va_end(use_args);
    function = MakeType(TYP_FUNCTION);
    function->of = result;
    function->u.f.args = args;
    function->flag |= 4;
    return function;
}

/* The original wrapper's source name is not recovered. */
TYPE GetBasicUInt(void)
{
    return GetBasicType(TYP_UINT);
}

int dupCompare(TYPE first, TYPE second)
{
    arg_list *args1, *args2;
    TYPE *p1, *p2;
    unsigned i;

    switch (first->id) {
    case TYP_BITFIELD:
        if (first->u.b.field_start != second->u.b.field_start) return 0;
        if (first->u.b.field_width != second->u.b.field_width) return 0;
        break;
    case TYP_FUNCTION:
        if (first->u.f.pragma != second->u.f.pragma) return 0;
        args1 = first->u.f.args;
        args2 = second->u.f.args;
        if (args1 == args2) return 1;
        if (args1->num_args != args2->num_args) return 0;
        if (args1->qualifier != args2->qualifier) return 0;
        p1 = args1->type_list;
        p2 = args2->type_list;
        for (i = args1->num_args; i; --i) {
            if (*p1++ != *p2++) return 0;
        }
        p1 = args1->except_spec;
        p2 = args2->except_spec;
        if (p1 != p2) {
            if (!p1 || !p2) return 0;
            /* Preserve the original test: distinct exception lists compare
             * unequal even when both reach their terminating NULL together. */
            for (;;) {
                if (!*p1 || !*p2 || *p1 != *p2) return 0;
                ++p1;
                ++p2;
            }
        }
        break;
    case TYP_ARRAY:
        if (first->u.a.array_size != second->u.a.array_size) return 0;
        break;
    case TYP_MODIFIER:
        if (first->u.m.base != second->u.m.base) return 0;
        if (first->u.m.pragma != second->u.m.pragma) return 0;
        break;
    case TYP_MEMBER_POINTER:
        if (first->u.mp.host != second->u.mp.host) return 0;
        break;
    case TYP_GENERIC:
        if (first->u.g.index != second->u.g.index) return 0;
        break;
    case TYP_POINTER:
        break;
    default:
        return 0;
    }
    return 1;
}

/* Reached1994 cached defaults retain their actual16-bit flag values. Later
 * source uses single TF1 flags and adds initCacheAfterOptions here. */
unsigned short defaultDataMemFlag, defaultFunctionMemFlag;

void PTypeCheckInit(void)
{
    unsigned index;
    for (index=0; index<TYP_MAX; ++index) {
        if (basicTypes[index]!=0)
            basicTypes[index]=CheckDupType(basicTypes[index]);
    }
    if (IsBigCode()) defaultFunctionMemFlag=8;
    else defaultFunctionMemFlag=4;
    if (IsHugeData()) defaultDataMemFlag=32;
    else if (IsBigData()) defaultDataMemFlag=8;
    else defaultDataMemFlag=4;
}

TYPE TypePtrToVoid(void) { return ptr_void; }

/* Actual1994 CF5384c..53873, before the later counters/debug checks. */
void typeFree(TYPE type)
{
    if (type->id==TYP_CLASS) CarveFree(carveCLASSINFO,type->u.i.init2);
    CarveFree(carveTYPE,type);
}

/* Old strip operations before later widened type sets. TypedefRemove owns
 * the preceding common RET5470d. FunctionDeclarationType retains the old
 * non-NULL type precondition (its original NULL path dereferences address20). */
TYPE TypedefRemove(TYPE type)
{
    while (type!=0 && type->id==TYP_TYPEDEF) type=type->of;
    return type;
}
TYPE TypedefModifierRemove(TYPE type)
{
    if (type!=0) {
        while (type->id==TYP_CHAR || type->id==TYP_ENUM
            || type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER) type=type->of;
    }
    return type;
}
TYPE FunctionDeclarationType(TYPE type)
{
    if (type!=0) {
        while (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER) type=type->of;
    }
    return type->id==TYP_FUNCTION ? type : 0;
}

arg_list *TypeArgList(TYPE type)
{
    type=FunctionDeclarationType(type);
    if (type!=0) return type->u.f.args;
    return 0;
}

TYPE TypeVoidFunOfPtrVoid(void) { return void_fun_of_ptr_void; }

TYPE TypePtrToVoidStdop(void) { return ptr_stdop_void; }
TYPE TypePtrToVoidStdopArith(void) { return ptr_stdop_arith_void; }
TYPE TypePtrToConstVoidStdop(void) { return ptr_const_stdop_void; }
TYPE TypePtrToConstVoidStdopArith(void) { return ptr_const_stdop_arith_void; }

/* Old pointer/reference constructors share the same MakeTypeOf tail. */
TYPE MakeReferenceTo(TYPE type)
{
    TYPE ref;
    while (type->id==TYP_TYPEDEF) type=type->of;
    ref=MakeType(TYP_POINTER);
    ref->flag|=1;
    return MakeTypeOf(ref,type);
}
static TYPE uniqueTypes;
void CheckUniqueType(TYPE type)
{ RingPush((void **)&uniqueTypes,type); }

/* Old1994 member-pointer setup has no later TypeModExtract pass. */
TYPE StructType(TYPE type)
{
    if (type) {
        while (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER) type=type->of;
        if (type->id==TYP_CLASS) return type;
    }
    return 0;
}
TYPE TypedefModifierRemoveOnly(TYPE type)
{
    if (type) while (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER)
        type=type->of;
    return type;
}
TYPE ReferenceType(TYPE type)
{
    if (type) {
        type=TypedefModifierRemoveOnly(type);
        if (type->id==TYP_POINTER && (type->flag&1)) return type->of;
    }
    return 0;
}
TYPE TypeReferenced(TYPE type)
{
    TYPE ref;
    ref=ReferenceType(type);
    return ref?ref:type;
}
unsigned short convertCVSpec(unsigned char spec)
{
    unsigned short flag;
    flag=0;
    if (spec&1) flag|=1;
    if (spec&2) flag|=2;
    return flag;
}
TYPE makeAnyPointer(type_id id,unsigned char qualifiers)
{
    TYPE type;
    unsigned short flags;
    type=MakeType(id);
    flags=convertCVSpec(qualifiers);
    if (flags) type->of=MakeFlagModifier(flags);
    return type;
}
TYPE makeMemberPointerType(TYPE host,unsigned char qualifiers)
{
    TYPE type;
    type=makeAnyPointer(TYP_MEMBER_POINTER,qualifiers);
    type->u.mp.host=TypedefRemove(host);
    return type;
}
TYPE MakeMemberPointerTo(TYPE host,TYPE base)
{
    TYPE type;
    host=StructType(host);
    base=TypeReferenced(base);
    type=makeMemberPointerType(host,0);
    return MakeTypeOf(type,base);
}
static TYPE void_member_ptr;
TYPE TypeVoidMemberPtr(void)
{
    TYPE host,base;
    if (void_member_ptr==0) {
        host=GetBasicType(TYP_VOID);
        base=GetBasicType(TYP_VOID);
        void_member_ptr=MakeMemberPointerTo(base,host);
    }
    return void_member_ptr;
}

TYPE TypeVoidFunOfVoid(void) { return void_fun_of_void; }

TYPE BasedType(TYPE type)
{
    for (;type;type=type->of) {
        if (type->id==TYP_MODIFIER && (type->flag&0x700)) return type;
        if (type->id!=TYP_MODIFIER && type->id!=TYP_TYPEDEF) break;
    }
    return 0;
}

DECL_SPEC *makeDeclSpec(void)
{
    DECL_SPEC *spec=(DECL_SPEC *)CarveAlloc(carveDECL_SPEC);
    spec->prev=NULL;
    spec->typedef_defined=NULL;
    spec->scope=NULL;
    spec->linkage=NULL;
    spec->partial=NULL;
    spec->name=NULL;
    spec->id=NULL;
    spec->stg_class=0;
    spec->scalar=0;
    spec->specifier=0;
    spec->type_defined=0;
    spec->type_declared=0;
    spec->ctor_name=0;
    spec->is_default=0;
    spec->diagnosed=0;
    spec->decl_checked=0;
    spec->type_elaborated=0;
    spec->nameless_allowed=0;
    spec->generic=0;
    spec->class_instantiation=0;
    spec->no_more_linkage=0;
    spec->arg_declspec=0;
    /* Original preserves four uninitialized allocator bits. */
    spec->reserved=0;
    return spec;
}
DECL_SPEC *PTypeScalar(unsigned short scalar)
{
    DECL_SPEC *spec=makeDeclSpec();
    spec->scalar=scalar;
    return spec;
}
DECL_SPEC *PTypeSpecifier(unsigned char specifier)
{
    DECL_SPEC *spec=makeDeclSpec();
    spec->specifier=specifier;
    return spec;
}
DECL_SPEC *PTypeStgClass(unsigned char storage_class)
{
    DECL_SPEC *spec=makeDeclSpec();
    spec->stg_class=storage_class;
    return spec;
}

DECL_SPEC *PTypeDefault(void)
{
    DECL_SPEC *spec=makeDeclSpec();
    spec->scalar=STM_INT;
    spec->is_default=1;
    return spec;
}

DECL_SPEC *PTypeActualType(TYPE type)
{
    DECL_SPEC *spec=makeDeclSpec();
    spec->partial=type;
    return spec;
}

void figureOutStgClass(DECL_SPEC *spec)
{
    if (spec->linkage && !spec->stg_class) spec->stg_class=8;
}
char *SimpleTypeName(TYPE type)
{
    SYMBOL sym;
    char *name=NULL;
    if (type) {
        switch (type->id) {
        case TYP_TYPEDEF: case TYP_ENUM:
            sym=type->u.t.sym;
            if (sym && !(type->flag&1)) name=sym->name->name;
            break;
        case TYP_CLASS: name=type->u.c.info->name;
        }
    }
    return name;
}
SYMBOL TemplateSymFromClass(TYPE type)
{ (void)type; STUB_TRAP(); return 0; }
SYMBOL checkPreviouslyDeclared(SYMBOL sym,char *name)
{ (void)sym; (void)name; STUB_TRAP(); return 0; }
void ScopeAddFriend(SCOPE scope,SYMBOL sym)
{ (void)scope; (void)sym; STUB_TRAP(); }
/* The old PTypeDone directly owns the friend checks, before their extraction
 * into checkForClassFriends in the released source. */
DECL_SPEC *PTypeDone(DECL_SPEC *spec,int declaration_done)
{
    SYMBOL sym=spec->typedef_defined;
    TYPE type,original_type;
    char *name;
    int bad_friend;
    if (!(spec->specifier&4) || !declaration_done) {
        if (sym && !spec->generic) {
            ScopeInsert(CurrScope,sym,spec->name);
            spec->typedef_defined=NULL;
        }
        return spec;
    }
    figureOutStgClass(spec);
    bad_friend=0;
    if (spec->stg_class || spec->scalar || spec->specifier!=4) bad_friend=1;
    if (bad_friend) CErr1(295);
    original_type=spec->partial;
    type=original_type;
    if (type) type=StructType(type);
    name=spec->name;
    if (spec->type_defined) {
        CErr1(298);
        bad_friend=1;
    } else if (!type || !spec->type_elaborated) {
        if (!type) {
            if (original_type!=TypeError) CErr1(485);
            bad_friend=1;
        } else if (CErr1(297)&2) name=SimpleTypeName(type);
        else bad_friend=1;
    }
    if (!ScopeType(CurrScope,1)) {
        CErr1(296);
        bad_friend=1;
    }
    spec->specifier&=~4;
    if (!bad_friend) {
        if (!sym && (type->flag&8)) sym=TemplateSymFromClass(type);
        else sym=checkPreviouslyDeclared(sym,name);
        if (sym) {
            ScopeAddFriend(CurrScope,sym);
            spec->nameless_allowed=1;
        }
    }
    return spec;
}
void LinkagePop(void);
LINKAGE LinkageMergePop(void);
void PTypeRelease(DECL_SPEC *spec);
unsigned char findTypeId(unsigned short scalar)
{
    switch (scalar) {
    case STM_CHAR: return TYP_CHAR;
    case STM_CHAR|STM_SIGNED: return TYP_SCHAR;
    case STM_CHAR|STM_UNSIGNED: return TYP_UCHAR;
    case STM_SHORT: case STM_SHORT|STM_INT:
    case STM_SHORT|STM_SIGNED: case STM_SHORT|STM_SIGNED|STM_INT:
        return TYP_SSHORT;
    case STM_SHORT|STM_UNSIGNED: case STM_SHORT|STM_UNSIGNED|STM_INT:
    case STM_SEGMENT: return TYP_USHORT;
    case STM_SIGNED: case STM_INT: case STM_INT|STM_SIGNED: return TYP_SINT;
    case STM_INT|STM_UNSIGNED: case STM_UNSIGNED: return TYP_UINT;
    case STM_LONG: case STM_LONG|STM_SIGNED: case STM_LONG|STM_INT:
    case STM_LONG|STM_INT|STM_SIGNED: return TYP_SLONG;
    case STM_LONG|STM_UNSIGNED: case STM_LONG|STM_INT|STM_UNSIGNED:
        return TYP_ULONG;
    case STM_FLOAT: return TYP_FLOAT;
    case STM_DOUBLE: return TYP_DOUBLE;
    case STM_VOID: return TYP_VOID;
    case STM_LONG|STM_DOUBLE: return TYP_LONG_DOUBLE;
    case STM_LONG|STM_CHAR: return TYP_WCHAR;
    default: return TYP_MAX;
    }
}
void checkStgClass(DECL_SPEC *first,DECL_SPEC *second)
{
    unsigned char repeats;
    if (second->stg_class) first->ctor_name=0;
    if (second->linkage) {
        if (first->no_more_linkage || !first->linkage) {
            CErr1(463);
            LinkagePop();
        } else first->linkage=LinkageMergePop();
        second->linkage=NULL;
    } else first->no_more_linkage=1;
    repeats=first->stg_class&second->stg_class;
    if (repeats) {
        CErr1(117);
        second->stg_class&=~repeats;
    }
    first->stg_class|=second->stg_class;
    switch (first->stg_class) {
    case 0: case 1: case 2: case 4: case 8: case 16: return;
    }
    first->stg_class=0;
    CErr1(117);
}
void checkScalar(DECL_SPEC *first,DECL_SPEC *second)
{
    unsigned short repeats=first->scalar&second->scalar;
    if (repeats) {
        CErr1(119);
        second->scalar&=~repeats;
    }
    first->scalar|=second->scalar;
    if (!first->scalar) return;
    first->ctor_name=0;
    if (findTypeId(first->scalar)!=TYP_MAX) return;
    first->scalar=0;
    CErr1(119);
}
void checkSpecifier(DECL_SPEC *first,DECL_SPEC *second)
{
    unsigned char repeats=first->specifier&second->specifier;
    if (repeats) {
        CErr1(120);
        second->specifier&=~repeats;
    }
    first->specifier|=second->specifier;
    if (second->specifier) first->ctor_name=0;
}
void checkType(DECL_SPEC *first,DECL_SPEC *second)
{
    if ((first->scalar|second->scalar) && (first->partial || second->partial)) {
        CErr1(62);
        first->scalar=0;
    }
    if (first->partial) {
        if (second->partial) CErr1(62);
        first->ctor_name=0;
    } else if (second->partial) {
        first->partial=second->partial;
        first->scope=second->scope;
        first->ctor_name=second->ctor_name;
        first->type_defined=second->type_defined;
        first->type_declared=second->type_declared;
        first->typedef_defined=second->typedef_defined;
        first->type_elaborated=second->type_elaborated;
        first->nameless_allowed=second->nameless_allowed;
        first->generic=second->generic;
        first->class_instantiation=second->class_instantiation;
        first->name=second->name;
        first->id=second->id;
        second->id=NULL;
    }
}
/*1994 has no later MS-declspec pass or long-long/bool extensions. */
DECL_SPEC *PTypeCombine(DECL_SPEC *first,DECL_SPEC *second)
{
    checkStgClass(first,second);
    checkScalar(first,second);
    checkSpecifier(first,second);
    checkType(first,second);
    PTypeRelease(second);
    return first;
}

DECL_SPEC *CheckArgDSpec(DECL_SPEC *spec)
{
    spec->arg_declspec=1;
    figureOutStgClass(spec);
    if (spec->stg_class&0x19) {
        CErr1(182);
        spec->stg_class&=~0x19;
    }
    if ((spec->type_declared || spec->type_defined) && !spec->generic)
        CErr1(414);
    return spec;
}

TYPE findScalarType(DECL_SPEC *spec)
{
    if (spec->is_default) return default_int;
    switch (spec->scalar) {
    case 0: return default_int;
    case STM_INT: return clean_int;
    case STM_SEGMENT: return segment_short;
    default: return GetBasicType(findTypeId(spec->scalar));
    }
}
void figureOutDSpec(DECL_SPEC *spec)
{
    unsigned short qualifiers;
    if (spec->decl_checked) return;
    figureOutStgClass(spec);
    if (!spec->partial) spec->partial=findScalarType(spec);
    qualifiers=spec->specifier&3;
    if (qualifiers) spec->partial=MakeTypeOf(MakeFlagModifier(qualifiers),spec->partial);
    spec->decl_checked=1;
}
TYPE declSpecType(DECL_SPEC *spec)
{
    if (spec->specifier&0x1c) {
        CErr1(135);
        return TypeError;
    }
    figureOutDSpec(spec);
    return spec->partial;
}
PTREE nameOfId(PTREE id)
{
    if (id && id->op!=6) id=id->u.subtree[1];
    return id;
}
PTREE CutAwayQualification(PTREE id)
{ (void)id; STUB_TRAP(); return 0; }
int ScopeEquivalent(SCOPE scope,unsigned char type);
TYPE ScopeClass(SCOPE scope);
TYPE ClassTypeForType(TYPE type);
SCOPE TypeScope(TYPE type)
{
    type=ClassTypeForType(type);
    return type ? type->u.c.scope : NULL;
}
void ScopeQualifyPush(SCOPE scope,SCOPE accessed);
char *CppDestructorName(void);
PTREE verifyQualifiedId(DECL_SPEC *spec,PTREE id,SCOPE *scope,unsigned *id_info)
{
    PTREE name_tree,scope_tree;
    TYPE scope_class_type,class_type;
    SCOPE qualifying_scope;
    int not_OK,strip_qualification=0;
    *scope=NULL;
    *id_info=0;
    name_tree=nameOfId(id);
    if (name_tree==id) return name_tree;
    scope_tree=id->u.subtree[0];
    if (!scope_tree) return CutAwayQualification(id);
    scope_class_type=scope_tree->type;
    if (scope_class_type->flag&4) {
        *id_info|=1;
        return id;
    }
    if (!ScopeEquivalent(CurrScope,0)) {
        not_OK=1;
        class_type=ScopeClass(CurrScope);
        if (class_type) {
            if (spec->specifier&4) not_OK=0;
            else if (class_type==scope_class_type) {
                not_OK=0;
                strip_qualification=1;
            }
        }
        if (not_OK) {
            CErr1(253);
            return CutAwayQualification(id);
        }
    }
    qualifying_scope=scope_class_type->u.c.scope;
    if (name_tree->u.id.name==CppDestructorName() &&
        TypeScope(name_tree->type)!=qualifying_scope) {
        CErr2p(427,name_tree->u.id.name);
        return CutAwayQualification(id);
    }
    if (strip_qualification) id=CutAwayQualification(id);
    else {
        ScopeQualifyPush(qualifying_scope,qualifying_scope);
        *scope=qualifying_scope;
    }
    return id;
}
DECL_INFO *makeDeclInfo(PTREE id)
{
    DECL_INFO *dinfo=CarveAlloc(carveDECL_INFO);
    dinfo->next=NULL;
    dinfo->scope=NULL;
    dinfo->friend_scope=NULL;
    dinfo->list=NULL;
    dinfo->parms=NULL;
    dinfo->sym=NULL;
    dinfo->generic_sym=NULL;
    dinfo->name=NULL;
    dinfo->type=NULL;
    dinfo->defarg_expr=NULL;
    dinfo->body=NULL;
    dinfo->mem_init=NULL;
    dinfo->id=id;
    /* Original preserves two uninitialized unused flag bits. */
    dinfo->flags=0;
    return dinfo;
}
DECL_INFO *MakeDeclarator(DECL_SPEC *spec,PTREE id)
{
    DECL_INFO *dinfo;
    SCOPE scope;
    unsigned id_info;
    id=verifyQualifiedId(spec,id,&scope,&id_info);
    dinfo=makeDeclInfo(id);
    dinfo->scope=scope;
    if (id_info&1) dinfo->flags|=8; /* class template member */
    return dinfo;
}
DECL_INFO *typeDeclInfo(TYPE type)
{
    DECL_INFO *dinfo=MakeDeclarator(NULL,NULL);
    dinfo->type=type;
    return dinfo;
}
struct decl_info *DeclSpecDeclarator(DECL_SPEC *spec)
{
    struct decl_info *dinfo=typeDeclInfo(declSpecType(spec));
    if (spec->generic) {
        dinfo->generic_sym=spec->typedef_defined;
        dinfo->name=spec->name;
    }
    return dinfo;
}

void LinkagePop(void);
void PTreeFreeSubtrees(PTREE tree);
void PTypeRelease(DECL_SPEC *spec)
{
    if (spec->id) PTreeFreeSubtrees(spec->id);
    CarveFree(carveDECL_SPEC,spec);
}
void PTypePop(DECL_SPEC *spec)
{
    if (spec->linkage) LinkagePop();
    PTypeRelease(spec);
}

char *NameDummy(void);
void noDuplicateNames(DECL_INFO *head,DECL_INFO *check)
{
    DECL_INFO *curr;
    char *name=check->name;
    if (!name || !head) return;
    curr=head;
    do {
        curr=curr->next;
        if (curr->name==name) {
            CErr2p(444,name);
            check->name=NameDummy();
            if (check->sym) check->sym->flag|=1;
            return;
        }
    } while (curr!=head);
}
TYPE MakePointerType(unsigned short ref_flag,unsigned char cv_flags)
{
    TYPE type=makeAnyPointer(TYP_POINTER,cv_flags);
    type->flag|=ref_flag;
    return type;
}
TYPE removeModifiers(TYPE type,TYPE stop);
TYPE replaceModifiers(TYPE modifiers,TYPE base);
TYPE changeToPointer(TYPE type,TYPE array_type)
{
    TYPE modifiers=removeModifiers(type,array_type);
    TYPE base=replaceModifiers(modifiers,array_type->of);
    return MakeTypeOf(MakePointerType(0,0),base);
}
TYPE adjustParmType(TYPE parm_type)
{
    TYPE parm=parm_type;
    while (parm->id==TYP_TYPEDEF || parm->id==TYP_MODIFIER) parm=parm->of;
    switch (parm->id) {
    case TYP_FUNCTION:
        if (parm->u.f.args->qualifier) CErr1(413);
        parm_type=MakeTypeOf(MakePointerType(0,0),parm_type);
        break;
    case TYP_ARRAY: parm_type=changeToPointer(parm_type,parm); break;
    }
    return parm_type;
}
void RingAppend(void **head,void *entry);
DECL_INFO *AddArgument(DECL_INFO *head,DECL_INFO *argument)
{
    TYPE type;
    SYMBOL sym;
    noDuplicateNames(head,argument);
    RingAppend((void **)&head,argument);
    type=adjustParmType(argument->type);
    argument->type=type;
    sym=head->sym;
    if (sym) sym->sym_type=type;
    return head;
}

TYPE IntegralType(TYPE type)
{
    while(type && (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER))
        type=type->of;
    if(!((1UL<<type->id)&((1UL<<TYP_ERROR)|(1UL<<TYP_CHAR)|
       (1UL<<TYP_SCHAR)|(1UL<<TYP_UCHAR)|(1UL<<TYP_WCHAR)|
       (1UL<<TYP_SSHORT)|(1UL<<TYP_USHORT)|(1UL<<TYP_SINT)|
       (1UL<<TYP_UINT)|(1UL<<TYP_SLONG)|(1UL<<TYP_ULONG)|
       (1UL<<TYP_ENUM)))) type=NULL;
    return type;
}
int validateTemplateArgType(TYPE type)
{
    TYPE trimmed=type;
    while (trimmed->id==TYP_TYPEDEF || trimmed->id==TYP_MODIFIER) trimmed=trimmed->of;
    switch (trimmed->id) {
    case TYP_GENERIC: return 1;
    case TYP_POINTER:
        /* This1994 compiler rejects references, unlike the released version;
         * ordinary pointer parameters are valid nongeneric arguments. */
        if (!(trimmed->flag&1)) return 0;
        break;
    default: if (IntegralType(trimmed)) return 0; break;
    }
    CErr2p(12,(const char *)type);
    return 0;
}
int ProcessTemplateArgs(DECL_INFO *args)
{
    DECL_INFO *curr;
    int all_generics=1,some_generics=0;
    if (!args) {
        CErr1(16);
        return 0;
    }
    curr=args;
    do {
        curr=curr->next;
        if (validateTemplateArgType(curr->type)) some_generics=1;
        else all_generics=0;
    } while (curr!=args);
    return all_generics && some_generics;
}

int ClassAnonymousUnion(DECL_SPEC *spec);
DECL_SPEC *CheckNoDeclarator(DECL_SPEC *spec)
{
    int check_stg=1;
    figureOutStgClass(spec);
    if (spec->type_declared || spec->type_defined || spec->nameless_allowed) {
        if (spec->specifier) {
            spec->specifier=0;
            CErr1(145);
        }
        if (spec->type_defined && ClassAnonymousUnion(spec)) check_stg=0;
        if (check_stg && spec->stg_class) {
            spec->stg_class=0;
            CErr1(146);
        }
    } else CErr1(118);
    return spec;
}

void CheckDeclarationDSpec(DECL_SPEC *spec,SCOPE scope)
{
    figureOutStgClass(spec);
    if ((spec->stg_class&6) && ScopeType(scope,0)) CErr1(177);
}

void FreeDeclInfo(DECL_INFO *info);
void freeDeclList(DECL_INFO **head)
{
    DECL_INFO *curr,*next,*last=*head;
    if (last) {
        curr=last->next;
        for (;;) {
            next=curr->next;
            FreeDeclInfo(curr);
            if (curr==last) break;
            curr=next;
        }
    }
    *head=0;
}
int cantHaveDefaultArgs(unsigned error,DECL_INFO *args)
{
    DECL_INFO *curr=args;
    if (curr) do {
        curr=curr->next;
        if (curr->defarg_expr) { CErr1(error); return 1; }
    } while (curr!=args);
    return 0;
}
int cantHaveDefaultArgGaps(DECL_INFO *args)
{
    DECL_INFO *curr=args;
    PTREE previous=0;
    int transition=0;
    if (curr) do {
        curr=curr->next;
        if (curr->defarg_expr && !previous) {
            if (transition) { CErr1(244); return 1; }
            transition=1;
        }
        previous=curr->defarg_expr;
    } while (curr!=args);
    return 0;
}
void FreeArgs(DECL_INFO *args)
{
    cantHaveDefaultArgGaps(args);
    cantHaveDefaultArgs(243,args);
    freeDeclList(&args);
}
void freeTypeList(TYPE type)
{
    TYPE next;
    while (type) {
        next=type->of;
        typeFree(type);
        type=next;
    }
}
void FreeDeclInfo(DECL_INFO *info)
{
    if (!info) return;
    if (info->id) PTreeFreeSubtrees(info->id);
    if (!info->type) freeTypeList(info->list);
    if (info->sym && !(info->flags&1)) FreeSymbol(info->sym);
    if (info->parms) freeDeclList(&info->parms);
    if (info->defarg_expr) PTreeFreeSubtrees(info->defarg_expr);
    if (info->body) RewriteFree(info->body);
    if (info->mem_init) RewriteFree(info->mem_init);
    CarveFree(carveDECL_INFO,info);
}

/* Initial type.c newINFO, restricted to the fields present in the 1994
 * 52-byte object. The original leaves bytes40..41, padding and six flag
 * bits uninitialized; deterministic reconstruction writes zero there. */
struct class_info *newINFO(void)
{
    struct class_info *info=CarveAlloc(carveCLASSINFO);
    info->bases=0; info->friends=0; info->name=0;
    info->size=0; info->vsize=0; info->cdopt_cache=0; info->refno=0;
    info->last_vfn=0; info->last_vbase=0;
    info->vb_offset=0; info->vf_offset=0; info->index=0;
    info->max_align=1;
    info->defined=0; info->opened=0; info->unnamed=0; info->corrupted=0;
    info->abstract=0; info->abstract_OK=0; info->anonymous=0; info->has_def_opeq=0;
    info->has_ctor=0; info->has_dtor=0; info->has_pure=0; info->has_vfptr=0;
    info->has_vbptr=0; info->has_data=0; info->has_vfn=0; info->has_vcdtor=0;
    info->ctor_defined=0; info->copy_defined=0; info->dtor_defined=0; info->assign_defined=0;
    info->ctor_gen=0; info->copy_gen=0; info->dtor_gen=0; info->assign_gen=0;
    info->ctor_user_code=0; info->copy_user_code=0; info->dtor_user_code=0; info->assign_user_code=0;
    info->ctor_user_code_checked=0; info->copy_user_code_checked=0;
    info->dtor_user_code_checked=0; info->assign_user_code_checked=0;
    info->needs_ctor=0; info->needs_dtor=0; info->needs_vdtor=0; info->needs_assign=0;
    info->const_copy=0; info->const_assign=0; info->const_ref=0; info->zero_array=0;
    info->flags50=0; info->derivation_copies=0; info->padding=0;
    return info;
}
TYPE MakeClassType(void)
{
    TYPE type=MakeType(TYP_CLASS);
    type->u.c.info=newINFO();
    return type;
}

DECL_SPEC *PTypeActualTypeName(TYPE type,PTREE id)
{
    DECL_SPEC *spec=makeDeclSpec();
    spec->partial=type;
    spec->id=id;
    spec->ctor_name=1;
    return spec;
}

/* Adapted from initial type.c FinishDeclarator, with the actual 1994
 * modifier masks, helper ABIs and diagnostics. No later mutable/class-mod,
 * interrupt-default-far or previous-symbol comparison logic is present. */
void ClassMakeUniqueName(TYPE type,char *name)
{ (void)type; (void)name; STUB_TRAP(); }
void checkForUnnamedStruct(DECL_SPEC *spec,char *name)
{
    TYPE type;
    if (!spec->type_defined) return;
    type=StructType(spec->partial);
    if (type && type->u.c.info->unnamed) ClassMakeUniqueName(type,name);
}
TYPE extractFunctionMods(TYPE curr,TYPE *ptail,int stop_at_nonmod)
{
    TYPE head=NULL,tail=NULL,prev=curr,next;
    for (curr=curr->of;curr;curr=next) {
        next=curr->of;
        if (curr->id!=TYP_MODIFIER) {
            if (stop_at_nonmod) break;
            prev=curr;
            continue;
        }
        if (curr->flag&0x800) {
            prev->of=next;
            if (!tail) tail=curr;
            curr->of=head;
            head=curr;
        } else prev=curr;
    }
    *ptail=tail;
    return head;
}
TYPE linkAfterPrev(TYPE list,TYPE prev,TYPE head,TYPE tail)
{
    if (prev) { tail->of=prev->of; prev->of=head; }
    else { tail->of=list; list=head; }
    return list;
}
TYPE adjustModifiers(TYPE list)
{
    TYPE last_fn_mod=NULL,last_fn_type=NULL,prev=NULL,last_prev=NULL;
    TYPE curr,head,tail;
    for (curr=list;curr;curr=curr->of) {
        if (curr->id==TYP_FUNCTION) {
            head=extractFunctionMods(curr,&tail,1);
            if (head) { list=linkAfterPrev(list,prev,head,tail); prev=tail; }
            last_fn_mod=NULL;
            last_fn_type=curr;
            last_prev=prev;
        } else if (curr->id==TYP_MODIFIER && (curr->flag&0x800)) last_fn_mod=curr;
        prev=curr;
    }
    if (last_fn_type && last_fn_mod) {
        head=extractFunctionMods(last_fn_type,&tail,0);
        if (head) list=linkAfterPrev(list,last_prev,head,tail);
    }
    return list;
}
TYPE dupModifier(TYPE type)
{
    return makeFullModifier(type->flag,type->u.m.base,type->u.m.pragma);
}
TYPE removeModifiers(TYPE type,TYPE stop)
{
    TYPE list=NULL,modifier;
    while (type!=stop) {
        if (type->id==TYP_MODIFIER) {
            modifier=dupModifier(type);
            modifier->of=list;
            list=modifier;
        }
        type=type->of;
    }
    return list;
}
TYPE dupFunction(TYPE type,unsigned control)
{
    arg_list *args=type->u.f.args,*old;
    TYPE result;
    unsigned i;
    if (!control) {
        old=args;
        args=AllocArgList(old->num_args,1);
        args->except_spec=old->except_spec;
        args->qualifier=old->qualifier;
        for (i=0;i<old->num_args;++i) args->type_list[i]=old->type_list[i];
    }
    result=MakeType(TYP_FUNCTION);
    result->flag=type->flag;
    result->of=type->of;
    result->u.f.args=args;
    result->u.f.pragma=type->u.f.pragma;
    return result;
}
TYPE massageFunctionTypeInDSpec(TYPE *base,TYPE list)
{
    TYPE fn=FunctionDeclarationType(*base),mods,mod,new_fn;
    if (!fn) return list;
    mods=removeModifiers(*base,fn);
    while (mods) {
        mod=mods; mods=mods->of; mod->of=list; list=mod;
    }
    new_fn=dupFunction(fn,1);
    new_fn->of=list;
    new_fn->flag|=0x8000;
    *base=fn->of;
    return new_fn;
}
unsigned scanDeclarator(TYPE list,unsigned *count)
{
    unsigned status=0;
    int cv_function_OK;
    arg_list *args;
    for (;list;list=list->of) {
        cv_function_OK=0;
        switch (list->id) {
        case TYP_FUNCTION:
            if (status&0x40) {
                status|=0x81; status&=~2;
                if (status&0x20) status|=4;
            } else if (status&0x80) status|=1;
            status&=~0x20;
            args=list->u.f.args;
            if (count) *count=args->num_args;
            if (args->qualifier) status|=0x20;
            status|=0x40;
            break;
        case TYP_MODIFIER: break;
        case TYP_MEMBER_POINTER: cv_function_OK=1;
        default:
            if ((status&0x20) && !cv_function_OK) status|=4;
            if (status&0x40) status|=2;
            status|=0x80;
            status&=~0x20;
        }
    }
    if (!(status&0x40)) status|=2;
    return status&0x1f;
}
void ClassAddFunctionMods(TYPE type);
void applyFnClassMods(TYPE list,DECL_SPEC *spec)
{
    TYPE type,fn=NULL;
    if (!spec->arg_declspec && ScopeType(CurrScope,1)) {
        for (type=list;type;type=type->of) if (type->id==TYP_FUNCTION) fn=type;
        if (!fn->u.f.pragma) ClassAddFunctionMods(fn);
    }
}
unsigned short convertFnSpec(unsigned char spec)
{
    unsigned short flags=0;
    if (spec&0x10) flags|=0x10;
    if (spec&8) flags|=8;
    return flags;
}
int applyFnSpec(TYPE list,DECL_SPEC *spec,unsigned status)
{
    TYPE type,fn=NULL;
    unsigned char fn_spec=spec->specifier;
    unsigned short flags=convertFnSpec(fn_spec);
    if (!flags && !(fn_spec&4)) return 0;
    for (type=list;type;type=type->of) if (type->id==TYP_FUNCTION) fn=type;
    if (!fn) CErr1(135);
    else {
        fn->flag|=flags;
        if (status&2) CErr1(301);
        else if (fn_spec&4) return 1;
    }
    return 0;
}
void removeDefaultArgs(DECL_INFO *args)
{
    DECL_INFO *curr=args;
    if (curr) do {
        curr=curr->next;
        if (curr->defarg_expr) {
            PTreeFreeSubtrees(curr->defarg_expr);
            curr->defarg_expr=NULL;
        }
    } while (curr!=args);
}
void checkUsefulParms(unsigned status,DECL_INFO *info)
{
    if (status&2) {
        FreeArgs(info->parms);
        info->parms=NULL;
        info->flags&=~0x20;
    } else if (cantHaveDefaultArgGaps(info->parms)) removeDefaultArgs(info->parms);
}
void checkUserConversion(TYPE type,unsigned status,unsigned count)
{ (void)type; (void)status; (void)count; STUB_TRAP(); }
void checkDestructor(TYPE type,unsigned status,unsigned count)
{
    if(status&2) CErr1(235);
    else if(type!=default_int || (status&1)) CErr1(234);
    if(status==0 && count!=0) CErr1(236);
}
void checkOperator(unsigned status,char *name)
{ if(status&2) CErr2p(237,name); }
void removeDefaultArgs(DECL_INFO *args);
void ForceNoDefaultArgs(DECL_INFO *info,unsigned error)
{
    DECL_INFO *parms=info->parms;
    cantHaveDefaultArgGaps(parms);
    if (cantHaveDefaultArgs(error,parms)) removeDefaultArgs(parms);
}
TYPE combineFunctionMods(TYPE first,TYPE second)
{ (void)first; (void)second; STUB_TRAP(); return 0; }
TYPE combineNormalMods(TYPE first,TYPE second)
{ (void)first; (void)second; STUB_TRAP(); return 0; }
TYPE extractMemoryModel(TYPE type,TYPE *leftover)
{ (void)type; (void)leftover; STUB_TRAP(); return 0; }
TYPE makeModifiedTypeOf(TYPE modifier,TYPE base)
{ (void)modifier; (void)base; STUB_TRAP(); return 0; }
unsigned short defaultMemoryFlag(TYPE type)
{
    return type->id==TYP_FUNCTION ? defaultFunctionMemFlag : defaultDataMemFlag;
}
TYPE TypeModFlags(TYPE type,unsigned short *flags)
{
    unsigned short accumulated=0;
    for (;type;type=type->of) {
        switch (type->id) {
        case TYP_MODIFIER: accumulated|=type->flag; continue;
        case TYP_TYPEDEF: continue;
        case TYP_CHAR: case TYP_ENUM: type=type->of;
        default:
            if (!(accumulated&0x73c)) accumulated|=defaultMemoryFlag(type);
            break;
        }
        break;
    }
    *flags=accumulated;
    return type;
}
int ScopeHasPureFunctions(SCOPE scope);
int verifyAbstractStatus(TYPE type)
{
    struct class_info *info=type->u.c.info;
    if(!info->abstract_OK) {
        if(!TypeDefined(type)) return 0;
        if(ScopeHasPureFunctions(type->u.c.scope)) info->abstract=1;
        info->abstract_OK=1;
    }
    return info->abstract;
}
int TypeAbstract(TYPE type)
{
    for (;;) {
        if (!type) return 0;
        while (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER) type=type->of;
        if (type->id==TYP_CLASS) return verifyAbstractStatus(type);
        if (type->id!=TYP_ARRAY) return 0;
        type=type->of;
    }
}
void abstractDiagnose(unsigned error,TYPE type)
{ (void)error; (void)type; STUB_TRAP(); }
int PragmaOKForVariables(void *pragma)
{ (void)pragma; STUB_TRAP(); return 0; }
void setStorageClass(SYMBOL sym,unsigned char storage)
{
    switch (storage) {
    case 1: sym->id=5; break;
    case 2: sym->id=4; break;
    case 4: sym->id=3; break;
    case 8: sym->id=1; break;
    case 16: sym->id=2; break;
    }
}
SCOPE ScopeQualifyPop(void);
TYPE TypeGetActualFlags(TYPE type,unsigned short *flags);
DECL_INFO *FinishDeclarator(DECL_SPEC *spec,DECL_INFO *info)
{
    PTREE id_tree=nameOfId(info->id);
    char *id=id_tree ? id_tree->u.id.name : NULL;
    TYPE curr,next,base,of_type,fnmod=NULL,mod=NULL,leftover,test_type;
    SYMBOL sym;
    unsigned status,arg_count,error=0;
    unsigned short flags,fn_flags;
    int move,add,diagnose,ok;
    void *pragma;
    figureOutDSpec(spec);
    checkForUnnamedStruct(spec,id);
    base=spec->partial;
    curr=adjustModifiers(info->list);
    curr=massageFunctionTypeInDSpec(&base,curr);
    status=scanDeclarator(curr,&arg_count);
    if (status&4) CErr1(413);
    if (!(status&2)) applyFnClassMods(curr,spec);
    if (applyFnSpec(curr,spec,status)) {
        info->flags|=2;
        if (spec->stg_class) { CErr1(295); spec->stg_class=0; }
    }
    if (info->parms) checkUsefulParms(status,info);
    if (id_tree) switch (id_tree->cgop) {
    case 199:
        checkUserConversion(base,status,arg_count);
        base=id_tree->type;
        break;
    case 197: checkDestructor(base,status,arg_count); break;
    case 198:
        checkOperator(status,id);
        switch (id_tree->u.id.operator_code) {
        case 58: case 60: case 63: case 65: break;
        default: ForceNoDefaultArgs(info,245); break;
        }
        break;
    }
    of_type=base;
    while (of_type && (of_type->id==TYP_TYPEDEF || of_type->id==TYP_MODIFIER))
        of_type=of_type->of;
    while (curr) {
        next=curr->of;
        move=0; add=1;
        switch (curr->id) {
        case TYP_MODIFIER:
            add=0;
            if (curr->flag&0x800) {
                curr->flag&=~0x800;
                fnmod=fnmod ? combineFunctionMods(fnmod,curr) : curr;
            } else mod=mod ? combineNormalMods(mod,curr) : curr;
            break;
        case TYP_FUNCTION:
            move=1;
            if (fnmod) {
                curr->flag|=fnmod->flag;
                curr->u.f.pragma=fnmod->u.m.pragma;
                typeFree(fnmod); fnmod=NULL;
            }
            fn_flags=curr->flag;
            if (fn_flags&0x8000) fn_flags&=~0x8000;
            else if (CurrLinkage==CppLinkage) fn_flags|=4;
            else fn_flags&=~4;
            curr->flag=fn_flags;
            switch (of_type->id) {
            case TYP_FUNCTION: CErr1(83); break;
            case TYP_ARRAY: CErr1(82); break;
            case TYP_POINTER:
                TypeModFlags(of_type->of,&flags);
                if ((flags&0x700)==0x200) CErr1(325);
                break;
            case TYP_CLASS:
                if (TypeAbstract(of_type)) abstractDiagnose(430,of_type);
                break;
            }
            break;
        case TYP_ARRAY:
            move=1;
            switch (of_type->id) {
            case TYP_FUNCTION: CErr1(81); break;
            case TYP_VOID: CErr1(218); break;
            case TYP_ARRAY: if (!of_type->u.a.array_size) CErr1(36); break;
            case TYP_POINTER: if (of_type->flag&1) CErr1(219); break;
            }
            break;
        case TYP_POINTER:
            if (curr->flag&1) {
                if (mod && (mod->flag&0x700)) CErr1(338);
                switch (of_type->id) {
                case TYP_POINTER: if (of_type->flag&1) CErr1(221); break;
                case TYP_VOID: CErr1(220); break;
                }
            } else switch (of_type->id) {
                case TYP_POINTER: if (of_type->flag&1) CErr1(222); break;
                case TYP_ARRAY: if (!of_type->u.a.array_size) CErr1(360); break;
            }
            break;
        case TYP_MEMBER_POINTER:
            if (mod && (mod->flag&0x73c)) CErr1(348);
            if (of_type->id==TYP_POINTER && (of_type->flag&1)) CErr1(225);
            break;
        }
        if (add) {
            if (mod) {
                if (move && !(mod->flag&0x73c)) move=0;
                if (move) {
                    mod=extractMemoryModel(mod,&leftover);
                    if (leftover) base=makeModifiedTypeOf(leftover,base);
                } else { base=makeModifiedTypeOf(mod,base); mod=NULL; }
            }
            base=MakeTypeOf(curr,base);
            of_type=base;
        }
        curr=next;
    }
    if (mod) base=makeModifiedTypeOf(mod,base);
    if (fnmod) {
        pragma=fnmod->u.m.pragma;
        ok=0;
        if (pragma && PragmaOKForVariables(pragma) && !fnmod->flag) ok=1;
        if (!FunctionDeclarationType(base) && !ok) CErr1(671);
        fnmod->flag=0;
        base=makeModifiedTypeOf(fnmod,base);
    }
    if (info->id) {
        diagnose=0;
        if (!(spec->stg_class&1)) {
            test_type=TypeGetActualFlags(base,&flags);
            if (test_type->id==TYP_VOID) { error=224; diagnose=1; }
            flags&=0x700;
            if (flags && flags!=0x100) { error=581; diagnose=1; }
            if (diagnose) base=TypeError;
        }
        sym=AllocSymbol();
        sym->sym_type=base;
        setStorageClass(sym,spec->stg_class);
        SymbolLocnDefine(&id_tree->locn,sym);
        if (!(PragToggle&2)) sym->flag|=0x10;
        info->sym=sym;
        info->name=id;
        if (info->scope) ScopeQualifyPop();
        if (diagnose) CErr2p(error,info->name);
    }
    info->type=base;
    if (FunctionDeclarationType(base)) {
        if (spec->type_defined) { CErr1(415); CErr1(707); }
        else if (spec->type_declared) CErr1(415);
    }
    return info;
}


TYPE TypeGetActualFlags(TYPE type,unsigned short *flags)
{
    unsigned short flag=0;
    for (;type;type=type->of) {
        if (type->id==TYP_MODIFIER) flag|=type->flag;
        else if (type->id!=TYP_TYPEDEF) break;
    }
    *flags=flag;
    return type;
}

int isVoid(TYPE type)
{
    return TypedefRemove(type)->id==TYP_VOID;
}
TYPE *makeExceptionList(PTREE tree)
{
    TYPE any=GetBasicType(TYP_DOT_DOT_DOT),*vector,*item;
    PTREE curr;
    unsigned count=0;
    for (curr=tree;curr;curr=curr->u.subtree[0]) {
        if (curr->type==any) return NULL;
        ++count;
    }
    vector=CPermAlloc((count+1)*sizeof(TYPE));
    item=vector;
    for (curr=tree;curr;curr=curr->u.subtree[0]) *item++=curr->type;
    *item=NULL;
    return vector;
}
TYPE MakeFnType(DECL_INFO **args,unsigned char qualifiers,PTREE exceptions)
{
    unsigned count=0,void_count=0;
    DECL_INFO *curr=*args;
    TYPE type,fn,*dest;
    arg_list *arg_types;
    PTREE next;
    if (curr) do {
        curr=curr->next;
        type=curr->type;
        if (isVoid(type)) ++void_count;
        else if (TypeAbstract(type)) abstractDiagnose(429,type);
        ++count;
    } while (curr!=*args);
    if (count && void_count) {
        if (count>1 || void_count>1) CErr1(212);
        freeDeclList(args);
        count=0;
    }
    arg_types=AllocArgList(count,1);
    arg_types->qualifier=convertCVSpec(qualifiers);
    arg_types->except_spec=makeExceptionList(exceptions);
    while (exceptions) {
        next=exceptions->u.subtree[0];
        PTreeFreeSubtrees(exceptions);
        exceptions=next;
    }
    dest=arg_types->type_list;
    curr=*args;
    if (curr) do {
        curr=curr->next;
        *dest++=curr->type;
    } while (curr!=*args);
    fn=MakeType(TYP_FUNCTION);
    fn->u.f.args=arg_types;
    return fn;
}
DECL_INFO *AddDeclarator(DECL_INFO *info,TYPE type)
{
    TYPE tail;
    if (type) {
        tail=type;
        while (tail->of) tail=tail->of;
        tail->of=info->list;
        info->list=type;
    }
    return info;
}
DECL_INFO *AddExplicitParms(DECL_INFO *info,DECL_INFO *args)
{
    if (info->flags&0x20) FreeArgs(args);
    else { info->parms=args; info->flags|=0x20; }
    return info;
}

void CheckFunctionDSpec(DECL_SPEC *spec)
{
    figureOutStgClass(spec);
    if (spec->stg_class&7) CErr1(37);
}
void PTypeForceLinkagePush(DECL_SPEC *spec)
{
    spec->linkage=CurrLinkage;
}
/* Actual1994 InsertDeclInfo: local externs promote directly to FileScope;
 * no later namespace-enclosure, block alias or symbol-location diagnostics. */
char *CppConstructorName(void);
/* Initial type.c special-function checks, adapted to the old cgop ids and
 * genuine helper ABIs. Explicit/namespace/derived-class extensions are later. */
SCOPE figureOutFriendScope(SCOPE scope,DECL_INFO *info)
{ (void)scope; (void)info; STUB_TRAP(); return 0; }
int memberCheck(SCOPE scope,DECL_INFO *info,TYPE *scope_type)
{
    TYPE type;
    *scope_type=NULL;
    if (info->scope) scope=info->scope;
    else if (info->flags&2) scope=figureOutFriendScope(scope,info);
    type=ScopeClass(scope);
    if (type) { *scope_type=type; return 1; }
    return 0;
}
unsigned howManyArgs(int member,TYPE fn)
{
    unsigned count=0;
    if (member) ++count;
    return count+fn->u.f.args->num_args;
}
int quietStaticMember(SYMBOL sym,PTREE id)
{
    if (id->cgop==198) {
        switch (id->u.id.operator_code) {
        case 58: case 60: case 63: case 65:
            sym->id=2;
            return 1;
        }
    }
    return 0;
}
TYPE classOrClassRef(TYPE type)
{ (void)type; STUB_TRAP(); return 0; }
void changeName(DECL_INFO *info,char *name)
{
    PTREE tree=nameOfId(info->id);
    if (tree) tree->u.id.name=name;
    info->name=name;
}
int SymIsThisFuncMember(SYMBOL sym);
TYPE adjustFunctionType(TYPE type,unsigned short flags,TYPE new_return);
unsigned short FunctionThisFlags(SYMBOL sym)
{
    return SymIsThisFuncMember(sym) ? TypeArgList(sym->sym_type)->qualifier : 0;
}
TYPE functionReturnsThis(SYMBOL sym,TYPE scope_type)
{
    TYPE this_type=MakeFlagModifier(FunctionThisFlags(sym));
    this_type=MakeTypeOf(this_type,scope_type);
    this_type=MakeReferenceTo(this_type);
    return adjustFunctionType(sym->sym_type,0,this_type);
}
TYPE functionReturnsPtrVoid(SYMBOL sym)
{ return adjustFunctionType(sym->sym_type,0,ptr_void); }
void extraInfoAboutArgs(unsigned count,int member,int non_static)
{ (void)count; (void)member; (void)non_static; STUB_TRAP(); }
void verifyIncDecSecondArg(TYPE fn,int member)
{ (void)fn; (void)member; STUB_TRAP(); }
int arrowReturnOK(TYPE type,char *name)
{ (void)type; (void)name; STUB_TRAP(); return 0; }
int isVoidPtr(TYPE type)
{ (void)type; STUB_TRAP(); return 0; }
void verifyNewFirstArg(char *name,TYPE fn)
{ (void)name; (void)fn; STUB_TRAP(); }
void verifyDelFirstArg(char *name,TYPE fn)
{ (void)name; (void)fn; STUB_TRAP(); }
void verifyDelSecondArg(char *name,TYPE fn)
{ (void)name; (void)fn; STUB_TRAP(); }
int checkOperatorArgs(TYPE fn)
{ (void)fn; STUB_TRAP(); return 0; }
TYPE DefaultIntType(TYPE type)
{
    return type->id==TYP_SINT && (type->flag&1) ? type : NULL;
}
void verifySpecialFunction(SCOPE scope,DECL_INFO *info)
{
    PTREE id=nameOfId(info->id);
    SYMBOL sym;
    char *name,*scope_name=NULL;
    TYPE type,fn,scope_type,udc;
    unsigned short cv;
    unsigned count;
    int member,non_static=0,check_args=1;
    if (!id) return;
    sym=info->sym;
    if (!SymIsFunction(sym)) return;
    name=id->u.id.name;
    member=memberCheck(scope,info,&scope_type);
    if (member) {
        scope_name=SimpleTypeName(scope_type);
        if (sym->id!=2 && !quietStaticMember(sym,id)) non_static=1;
    }
    type=sym->sym_type;
    fn=FunctionDeclarationType(type);
    cv=fn->u.f.args->qualifier;
    if (member) {
        if (!(fn->flag&4)) sym->sym_type=MakePlusPlusFunction(type);
        if (!non_static && (cv&3)) CErr1(411);
    } else {
        if (fn->flag&0x10) CErr1(357);
        if (cv&3) CErr1(412);
    }
    count=howManyArgs(non_static,fn);
    switch (id->cgop) {
    case 197: /* destructor */
        if (!member) CErr1(265);
        else {
            if (SimpleTypeName(id->type)!=scope_name) CErr1(418);
            if (!non_static) CErr1(265);
            if (cv&3) CErr1(410);
            sym->sym_type=functionReturnsPtrVoid(sym);
        }
        return;
    case 199: /* conversion function */
        if (!non_static) CErr1(264);
        else {
            udc=classOrClassRef(fn->of);
            if (udc && udc==scope_type) CErr1(446);
        }
        return;
    case 198: /* overloaded operator */ break;
    default:
        if (member && name==scope_name) {
            changeName(info,CppConstructorName());
            if (!DefaultIntType(fn->of)) CErr1(287);
            if (!non_static) CErr1(288);
            if (cv&3) CErr1(290);
            if (fn->flag&0x10) CErr1(291);
            if (member) sym->sym_type=functionReturnsThis(sym,scope_type);
        }
        return;
    }
    switch (id->u.id.operator_code) {
    case 36: case 37: /* preincrement/predecrement */
        if (count!=1 && count!=2) {
            CErr2p(269,name); extraInfoAboutArgs(count,member,non_static);
        }
        if (count>1) verifyIncDecSecondArg(fn,non_static);
        break;
    case 23: /* operator-> */
        if (count!=1) { CErr2p(267,name); extraInfoAboutArgs(count,member,non_static); }
        if (!non_static) { CErr2p(266,name); check_args=0; }
        if (!arrowReturnOK(fn->of,name)) CErr1(277);
        break;
    case 24: /* operator[] */
        if (count!=2) { CErr2p(268,name); extraInfoAboutArgs(count,member,non_static); }
        if (!non_static) { CErr2p(266,name); check_args=0; }
        break;
    case 47: /* operator() */
        if (!non_static) { CErr2p(266,name); check_args=0; }
        break;
    case 13: /* operator= */
        if (count!=2) { CErr2p(268,name); extraInfoAboutArgs(count,member,non_static); }
        if (!non_static) { CErr2p(266,name); check_args=0; }
        break;
    case 0: case 1: case 2: case 5: /* old unary/binary +,-,*,& */
        if (count!=1 && count!=2) {
            CErr2p(269,name); extraInfoAboutArgs(count,member,non_static);
        }
        break;
    case 11: case 22: /* ! and ~ */
        if (count!=1) { CErr2p(267,name); extraInfoAboutArgs(count,member,non_static); }
        break;
    case 63: case 65: /* new and new[] */
        check_args=0;
        if (!isVoidPtr(fn->of)) CErr2p(272,name);
        if (!count) { CErr2p(270,name); extraInfoAboutArgs(count,member,non_static); }
        else verifyNewFirstArg(name,fn);
        break;
    case 58: case 60: /* delete and delete[] */
        check_args=0;
        if (!isVoid(fn->of)) CErr2p(271,name);
        if (member) {
            if (count!=1 && count!=2) {
                CErr2p(269,name); extraInfoAboutArgs(count,member,non_static);
            }
        } else if (count!=1) {
            CErr2p(267,name); extraInfoAboutArgs(count,member,non_static);
        }
        if (count) {
            verifyDelFirstArg(name,fn);
            if (member && count>1) verifyDelSecondArg(name,fn);
        }
        break;
    default:
        if (count!=2) { CErr2p(268,name); extraInfoAboutArgs(count,member,non_static); }
        break;
    }
    if (check_args && !non_static && !checkOperatorArgs(fn)) CErr2p(278,name);
}

void complainAboutMemInit(DECL_INFO *info)
{
    if (!info->mem_init || info->name==CppConstructorName()) return;
    RewriteFree(info->mem_init);
    info->mem_init=NULL;
    CErr1(420);
}
int IsSrcFileLibrary(void);
void checkReservedName(DECL_INFO *info)
{
    unsigned char prev=0,c;
    char *name;
    if (CompFlags.extensions_enabled || IsSrcFileLibrary()) return;
    name=info->name;
    while ((c=*name++)!=0) {
        if (c=='_' && c==prev) { CErr2p(395,info->name); return; }
        prev=c;
    }
}
void TemplateFunctionCheck(SYMBOL sym,DECL_INFO *info);
unsigned short ExplicitModifierFlags(TYPE type)
{ (void)type; STUB_TRAP(); return 0; }
TYPE MakeModifiedType(TYPE type,unsigned short flags)
{
    if (flags) type=MakeTypeOf(MakeFlagModifier(flags),type);
    return type;
}
int ScopeLocalClass(SCOPE scope);
TYPE __cdecl MakeModifiableFunction(TYPE result,...)
{
    va_list count_args,use_args;
    va_start(count_args,result);
    va_start(use_args,result);
    return buildFnType(result,count_args,use_args);
}
int TypeIsConst(TYPE type)
{
    unsigned short flags;
    TypeModFlags(type,&flags);
    return flags&1;
}
TYPE replaceModifiers(TYPE modifiers,TYPE base);
TYPE functionReduce(TYPE type,unsigned count)
{
    TYPE fn=type,modifiers,new_fn;
    arg_list *old,*args;
    unsigned i;
    while (fn->id==TYP_TYPEDEF || fn->id==TYP_MODIFIER) fn=fn->of;
    modifiers=removeModifiers(type,fn);
    old=fn->u.f.args;
    args=AllocArgList(count,1);
    args->except_spec=old->except_spec;
    args->qualifier=old->qualifier;
    for (i=0;i<count;++i) args->type_list[i]=old->type_list[i];
    new_fn=MakeType(TYP_FUNCTION);
    new_fn->flag=fn->flag;
    new_fn->u.f.args=args;
    new_fn->u.f.pragma=fn->u.f.pragma;
    new_fn=MakeTypeOf(new_fn,fn->of);
    return replaceModifiers(modifiers,new_fn);
}
char *CppDefaultArgName(SYMBOL sym,unsigned index);
SYMBOL SymCreateAtLocn(TYPE type,unsigned char id,unsigned flags,char *name,
    SCOPE scope,CGIO_LOCATION *location);
/* Unlike the released version,1994 materializes an actual helper function
 * for each default expression and keeps that owner in the prototype's union. */
SYMBOL makeDefaultProto(DECL_INFO *info,unsigned index,DECL_INFO *parm)
{
    TYPE result=parm->type,ref,plain,helper_type;
    CGIO_LOCATION *location=NULL;
    SYMBOL base=info->sym,helper,prototype;
    ref=ReferenceType(result);
    if (ref && TypeIsConst(ref)) {
        plain=TypedefModifierRemoveOnly(ref);
        if (plain->id!=TYP_FUNCTION && plain->id!=TYP_ARRAY) result=ref;
    }
    if (parm->defarg_expr) location=&parm->defarg_expr->locn;
    helper_type=MakeModifiableFunction(result,(TYPE)NULL);
    helper_type->flag|=8;
    helper_type=CheckDupType(helper_type);
    helper=SymCreateAtLocn(helper_type,2,3,CppDefaultArgName(base,index),CurrScope,location);
    prototype=AllocSymbol();
    prototype->id=7;
    prototype->sym_type=functionReduce(base->sym_type,index);
    prototype->thread=base;
    prototype->u.sym=helper;
    SymbolLocnDefine(location,prototype);
    return prototype;
}
SYMBOL InsertSymbol(SCOPE scope,SYMBOL sym,char *name);
int defaultArgCantExist(SCOPE scope,DECL_INFO *info,SYMBOL sym)
{
    SYMBOL declaration=InsertSymbol(scope,sym,info->name);
    if(declaration!=sym) {
        if(declaration->id!=7) {
            CErr2p(248,(const char *)declaration);
            return 0;
        }
        if(SymDefaultBase(declaration)==info->sym)
            CErr2p(247,(const char *)declaration);
        else
            CErr2p(249,(const char *)declaration);
        return 0;
    }
    return 1;
}
SYMBOL defaultArgMustExist(SCOPE scope,DECL_INFO *info,SYMBOL sym)
{
    SYMBOL declaration=InsertSymbol(scope,sym,info->name),base;
    if(declaration!=sym) {
        if(declaration->id!=7) {
            CErr2p(248,(const char *)declaration);
            return NULL;
        }
        base=SymDefaultBase(declaration);
        if(base==info->sym) return declaration;
        CErr2p(249,(const char *)base);
    } else {
        CErr2p(250,(const char *)sym);
    }
    return NULL;
}
int SymIsTemporary(SYMBOL sym);
SCOPE SymScope(SYMBOL sym);
PTREE defaultArgSymError(unsigned error,PTREE tree,SYMBOL sym)
{ (void)error; (void)tree; (void)sym; STUB_TRAP(); return NULL; }
PTREE symCheck(PTREE tree)
{
    SYMBOL sym;
    SCOPE scope;
    if(tree->op==7) {
        sym=tree->u.symcg.symbol;
        if(!sym) tree=defaultArgSymError(606,tree,sym);
        else if(!SymIsTemporary(sym)) {
            scope=SymScope(sym);
            if(ScopeType(scope,2)) tree=defaultArgSymError(255,tree,sym);
            else if(ScopeType(scope,3)) tree=defaultArgSymError(256,tree,sym);
        }
    }
    return tree;
}
void defineDefaultValue(PTREE tree,SYMBOL sym,PTREE (*check)(PTREE));
void declareDefaultArgs(SCOPE scope,DECL_INFO *info)
{
    DECL_INFO *args=info->parms,*curr;
    SYMBOL def_sym,head=NULL,*prev=&head;
    unsigned index=0;
    int present=0;
    if (!args) return;
    curr=args;
    do {
        curr=curr->next;
        if (curr->type->id==TYP_DOT_DOT_DOT) break;
        def_sym=NULL;
        if (curr->defarg_expr) {
            def_sym=makeDefaultProto(info,index,curr);
            if (!defaultArgCantExist(scope,info,def_sym)) {
                PTreeFreeSubtrees(curr->defarg_expr);
                curr->defarg_expr=NULL;
                def_sym=NULL;
            }
            present=1;
        } else if (present) {
            def_sym=makeDefaultProto(info,index,curr);
            def_sym=defaultArgMustExist(scope,info,def_sym);
        }
        if (def_sym) { *prev=def_sym; prev=&def_sym->thread; }
        ++index;
    } while (curr!=args);
    *prev=info->sym;
    if (present) {
        curr=args;
        def_sym=head;
        do {
            curr=curr->next;
            if (curr->type->id==TYP_DOT_DOT_DOT) break;
            if (curr->defarg_expr) {
                defineDefaultValue(curr->defarg_expr,def_sym,symCheck);
                curr->defarg_expr=NULL;
                def_sym=def_sym->thread;
            }
        } while (curr!=args);
    }
}
void verifyNoDefaultArgs(DECL_INFO *info)
{
    if (info->parms) cantHaveDefaultArgs(243,info->parms);
}
DECL_INFO *InsertDeclInfo(SCOPE insert_scope,DECL_INFO *info)
{
    TYPE type;
    SCOPE scope;
    SYMBOL sym,check_sym,decl_sym;
    unsigned short flags;
    int is_function,is_redefined;
    verifySpecialFunction(insert_scope,info);
    complainAboutMemInit(info);
    checkReservedName(info);
    sym=info->sym;
    is_function=SymIsFunction(sym);
    scope=info->scope;
    if (scope) {
        check_sym=ScopeInsert(scope,sym,info->name);
        if (check_sym==sym && !(info->flags&1)) CErr2p(254,(char *)check_sym);
    } else {
        scope=insert_scope;
        if (info->flags&2) scope=info->friend_scope;
        else if (ScopeType(scope,4)) {
            TemplateFunctionCheck(sym,info);
            scope=FileScope;
        } else if (ScopeType(scope,3) || ScopeType(scope,2)) {
            if (sym->id==1) scope=FileScope;
            else if (is_function) {
                switch (sym->id) {
                case 5: break;
                case 0: scope=FileScope; break;
                default: CErr1(337); sym->id=1; scope=FileScope; break;
                }
            } else switch (sym->id) {
                case 5: case 2: break;
                default:
                    type=TypeGetActualFlags(sym->sym_type,&flags);
                    if (flags&0x73c) {
                        if (ExplicitModifierFlags(sym->sym_type)&0x73c) CErr1(373);
                        else {
                            flags&=~0x73c;
                            if (flags) type=MakeModifiedType(type,flags);
                        }
                        sym->sym_type=type;
                    }
                    if (TargetSwitches&0x200) sym->sym_type=MakeModifiedType(sym->sym_type,8);
            }
        } else if (is_function && ScopeLocalClass(scope)) {
            if (!(info->flags&4) && sym->id!=7) CErr1(361);
        }
        decl_sym=sym;
        is_redefined=ScopeCarefulInsert(scope,&decl_sym,info->name);
        check_sym=decl_sym;
        if (is_function) {
            if (check_sym==sym && (info->flags&2) && ScopeType(scope,0)
                && !CompFlags.extensions_enabled) check_sym->id=1;
        } else if (check_sym && check_sym!=sym && is_redefined)
            CErr2p(42,(char *)check_sym);
    }
    info->sym=check_sym;
    info->flags|=1;
    if (check_sym) {
        if (info->flags&2) {
            if (ScopeType(insert_scope,1)) {
                if (info->scope && (info->flags&4)) CErr2p(372,(char *)check_sym);
                ScopeAddFriend(insert_scope,check_sym);
            } else CErr1(296);
        }
        if (is_function) declareDefaultArgs(scope,info);
        else verifyNoDefaultArgs(info);
    }
    return info;
}

SYMBOL ScopeOrderedStart(SCOPE scope);
SYMBOL ScopeOrderedNext(SYMBOL stop,SYMBOL curr);
void *VstkPush(VSTK_CTL *stack);
void *VstkPop(VSTK_CTL *stack);
void VstkClose(VSTK_CTL *stack);
int markAllUnused(SCOPE scope,void (*diag)(SYMBOL))
{
    unsigned short all_flags=1,flags;
    SYMBOL curr=NULL,stop=ScopeOrderedStart(scope);
    TYPE generic;
    while ((curr=ScopeOrderedNext(stop,curr))!=NULL) {
        generic=TypedefRemove(curr->sym_type);
        flags=generic->flag;
        if (diag && !(flags&1)) diag(curr);
        all_flags&=flags;
        generic->flag=flags&~1;
    }
    return (all_flags&1)!=0;
}
void pushType(VSTK_CTL *stack,TYPE type)
{
    *(TYPE *)VstkPush(stack)=type;
}
void pushArguments(VSTK_CTL *stack,arg_list *args)
{
    unsigned i;
    TYPE type;
    for (i=0;i<args->num_args;++i) {
        type=args->type_list[i];
        while (type && (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER)) type=type->of;
        pushType(stack,type);
    }
}
void checkTemplateClass(VSTK_CTL *stack,TYPE type)
{ (void)stack; (void)type; STUB_TRAP(); }
void scanForGenerics(VSTK_CTL *stack,TYPE type)
{
    for (;;) switch (type->id) {
    case TYP_POINTER: case TYP_TYPEDEF: case TYP_ARRAY: case TYP_MODIFIER:
        type=type->of;
        break;
    case TYP_CLASS:
        if (type->flag&0xc) checkTemplateClass(stack,type);
        return;
    case TYP_FUNCTION:
        pushArguments(stack,type->u.f.args);
        type=type->of;
        break;
    case TYP_MEMBER_POINTER:
        pushType(stack,type->u.mp.host);
        type=type->of;
        break;
    case TYP_GENERIC:
        type->flag|=1;
        return;
    default: return;
    }
}
int FunctionUsesAllTypes(SYMBOL sym,SCOPE scope,void (*diag)(SYMBOL))
{
    TYPE fn=FunctionDeclarationType(sym->sym_type),*top;
    VSTK_CTL stack;
    if (!fn) return 0;
    markAllUnused(scope,NULL);
    VstkOpen(&stack,sizeof(TYPE),8);
    pushArguments(&stack,fn->u.f.args);
    while ((top=VstkPop(&stack))!=NULL) if (*top) scanForGenerics(&stack,*top);
    VstkClose(&stack);
    return markAllUnused(scope,diag);
}

TYPE replaceModifiers(TYPE modifiers,TYPE base)
{
    TYPE modifier;
    while (modifiers) {
        modifier=modifiers;
        modifiers=modifiers->of;
        base=MakeTypeOf(modifier,base);
    }
    return base;
}
TYPE adjustFullFunctionType(TYPE type,unsigned short on,unsigned short off,
    TYPE new_return,void *new_pragma)
{
    TYPE fn=FunctionDeclarationType(type),new_fn,modifiers;
    unsigned short old_flags=fn->flag,new_flags=(old_flags|on)&~off;
    if (new_flags==old_flags && !new_return && !new_pragma) return type;
    modifiers=removeModifiers(type,fn);
    new_fn=dupFunction(fn,1);
    new_fn->flag=new_flags;
    if (new_return) new_fn->of=new_return;
    if (new_pragma) new_fn->u.f.pragma=new_pragma;
    new_fn=CheckDupType(new_fn);
    return replaceModifiers(modifiers,new_fn);
}
TYPE adjustFunctionType(TYPE type,unsigned short flags,TYPE new_return)
{
    return adjustFullFunctionType(type,flags,0,new_return,NULL);
}
TYPE MakePlusPlusFunction(TYPE type)
{
    return adjustFunctionType(type,4,NULL);
}

/* Initial type.c InsertSymbol/tryInsertion share the old public body. */
SYMBOL InsertSymbol(SCOPE scope,SYMBOL sym,char *name)
{
    DECL_INFO *info=makeDeclInfo(NULL);
    SYMBOL declared;
    info->sym=sym;
    info->name=name;
    InsertDeclInfo(scope,info);
    declared=info->sym;
    FreeDeclInfo(info);
    return declared;
}

TYPE ArrayBaseType(TYPE type)
{
    for(;;) {
        while(type && (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER))
            type=type->of;
        if(type->id!=TYP_ARRAY) return type;
        type=type->of;
    }
}

TYPE AlignmentType(TYPE type)
{
    for (;;) {
        while (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER ||
               type->id==TYP_ENUM || type->id==TYP_CHAR) type=type->of;
        switch (type->id) {
        case TYP_CLASS:
            switch (type->u.c.info->max_align) {
            case 2: return GetBasicType(TYP_USHORT);
            case 4: return GetBasicType(TYP_ULONG);
            case 8: return GetBasicType(TYP_DOUBLE);
            default: return GetBasicType(TYP_UCHAR);
            }
        case TYP_ARRAY: type=type->of; break;
        case TYP_MEMBER_POINTER: return GetBasicType(TYP_UINT);
        case TYP_LONG_DOUBLE: return GetBasicType(TYP_DOUBLE);
        default: return type;
        }
    }
}

TYPE MakePureFunction(TYPE type)
{
    return adjustFunctionType(type,0x40,NULL);
}
int aZeroConstant(PTREE init)
{
    return init->op==2 && init->cgop==7 && init->u.int_constant==0;
}
void VerifyPureFunction(DECL_INFO *info,PTREE init)
{
    SYMBOL sym;
    TYPE type;
    /* The1994 body checks/frees the initializer before inspecting the
     * function type; the released later source orders these operations
     * differently. Preserve the actual diagnostic and lifetime order. */
    if (!aZeroConstant(init)) CErr1(137);
    PTreeFree(init);
    sym=info->sym;
    type=sym->sym_type;
    if (!FunctionDeclarationType(type)) CErr1(416);
    else sym->sym_type=MakePureFunction(type);
}

void VerifyMemberFunction(DECL_SPEC *spec,DECL_INFO *info)
{
    SYMBOL sym;
    if (spec->is_default && info) {
        sym=info->sym;
        if (sym && !FunctionDeclarationType(sym->sym_type)) CErr2p(336,sym->name->name);
    }
    FreeDeclInfo(info);
}

int TypeDefined(TYPE type)
{
    for (;;) {
        if (!type) return 0;
        while (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER) type=type->of;
        switch (type->id) {
        case TYP_CLASS: return type->u.c.info->defined!=0;
        case TYP_ARRAY:
            if (!type->u.a.array_size) return 0;
            type=type->of;
            break;
        default: return 1;
        }
    }
}

PTREE CheckScopedId(PTREE tree);
PTREE MakeConstructorId(DECL_SPEC *spec)
{
    PTREE tree=NULL;
    SCOPE scope;
    if (spec->ctor_name) {
        scope=spec->scope;
        if (!scope || ScopeType(scope,1)) {
            spec->scope=NULL;
            spec->partial=NULL;
            tree=spec->id;
            spec->id=NULL;
            tree=CheckScopedId(tree);
        }
    }
    return tree;
}

TYPE MakePureVirtualFunction(TYPE type) { (void)type; STUB_TRAP(); return NULL; }
TYPE MakeVirtualFunction(TYPE type) { return adjustFunctionType(type,0x10,NULL); }
int TypeHasEllipsisArg(TYPE type)
{
    arg_list *args;
    type=FunctionDeclarationType(type);
    if (type) {
        args=type->u.f.args;
        if (args->num_args && args->type_list[args->num_args-1]->id==TYP_DOT_DOT_DOT) return 1;
    }
    return 0;
}
int TypeHasNumArgs(TYPE type,unsigned count)
{
    arg_list *args=TypeArgList(type);
    if (!args) return 0;
    if (TypeHasEllipsisArg(type)) return count>=args->num_args-1;
    return count==args->num_args;
}
TYPE typeTested(TYPE type,unsigned char id)
{
    if (type) {
        type=TypeReferenced(type);
        type=TypedefModifierRemoveOnly(type);
        if (type->id!=id) type=NULL;
    }
    return type;
}
TYPE ClassTypeForType(TYPE type) { return typeTested(type,TYP_CLASS); }

/* Type modifier extraction: actual 1994 exclusions use bits1 and8. */
TYPE TypeModExtract(TYPE type,unsigned short *flags,void **baser,unsigned char mask)
{
    unsigned short flag=0;
    *baser=NULL;
    for(;type;type=type->of) {
        switch(type->id) {
        case TYP_MODIFIER:
            flag|=type->flag;
            if(type->flag&0x700) *baser=type->u.m.base;
            continue;
        case TYP_TYPEDEF: continue;
        case TYP_CHAR: case TYP_ENUM:
            if (!(mask&1)) type=type->of;
        default:
            if ((mask&8) && !(flag&0x73c)) flag|=defaultMemoryFlag(type);
            break;
        }
        break;
    }
    *flags=flag;
    return type;
}

TYPE RemoveFunctionFlag(TYPE type,unsigned short flags)
{
    return adjustFullFunctionType(type,0,flags,NULL,NULL);
}

/* The old mask includes ERROR and BITFIELD, before BOOL/INT64 were added. */
TYPE ArithType(TYPE type)
{
    while(type && (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER))
        type=type->of;
    if(!((1UL<<type->id)&((1UL<<TYP_ERROR)|(1UL<<TYP_CHAR)|
       (1UL<<TYP_SCHAR)|(1UL<<TYP_UCHAR)|(1UL<<TYP_WCHAR)|
       (1UL<<TYP_SSHORT)|(1UL<<TYP_USHORT)|(1UL<<TYP_SINT)|
       (1UL<<TYP_UINT)|(1UL<<TYP_SLONG)|(1UL<<TYP_ULONG)|
       (1UL<<TYP_FLOAT)|(1UL<<TYP_DOUBLE)|(1UL<<TYP_LONG_DOUBLE)|
       (1UL<<TYP_ENUM)|(1UL<<TYP_BITFIELD)))) type=NULL;
    return type;
}

unsigned SignedIntType(TYPE type)
{
    while(type && (type->id==TYP_CHAR || type->id==TYP_ENUM ||
          type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER)) type=type->of;
    return (1UL<<type->id)&((1UL<<TYP_SCHAR)|(1UL<<TYP_SSHORT)|
                           (1UL<<TYP_SINT)|(1UL<<TYP_SLONG));
}

TYPE EnumType(TYPE type)
{
    for(;;type=type->of) {
        if(type->id==TYP_ENUM) break;
        if(type->id!=TYP_TYPEDEF && type->id!=TYP_MODIFIER) {
            type=NULL; break;
        }
    }
    return type;
}

TYPE ArrayType(TYPE type)
{
    while(type && (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER))
        type=type->of;
    return type->id==TYP_ARRAY?type:NULL;
}
TYPE PointerTypeForArray(TYPE array)
{
    TYPE type=array;
    while(type && (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER))
        type=type->of;
    if(type->id==TYP_ARRAY) array=changeToPointer(array,type);
    return array;
}

TYPE MemberPtrType(TYPE type)
{
    if(type) {
        while(type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER) type=type->of;
        if(type->id!=TYP_MEMBER_POINTER) type=NULL;
    }
    return type;
}

TYPE makeVTableType(TYPE type,int add_consts)
{
    type=MakePointerTo(type);
    if(add_consts) type=MakeConstTypeOf(type);
    return type;
}
TYPE MakeVFTableFieldType(int add_consts)
{
    TYPE type=MakePointerTo(void_fun_of_void);
    type=MakeConstTypeOf(type);
    return makeVTableType(type,add_consts);
}

TYPE MakeInlineFunction(TYPE type)
{ return adjustFunctionType(type,8,NULL); }

unsigned char ObjModelArgument(TYPE type);
int TypeVAStartWontWork(TYPE type,unsigned *error)
{
    arg_list *args;
    unsigned count;
    TYPE arg;
    *error=0;
    if (!TypeHasEllipsisArg(type)) return 0;
    args=type->u.f.args;
    count=args->num_args;
    if (count) {
        --count;
        if (!count) { *error=667; return 1; }
        arg=args->type_list[--count];
        if (ReferenceType(arg)) { *error=668; return 1; }
        arg=StructType(arg);
        if (arg && ObjModelArgument(arg)==2) { *error=669; return 1; }
    }
    return 0;
}

SYMBOL SymMakeDummy(TYPE type,char **name);
void InsertArgs(DECL_INFO **args)
{
    DECL_INFO *head=*args,*current;
    char *name;
    if (head) {
        current=head;
        do {
            current=current->next;
            if (current->type->id==TYP_DOT_DOT_DOT) break;
            if (!current->sym) {
                current->sym=SymMakeDummy(current->type,&name);
                current->name=name;
            }
            InsertDeclInfo(CurrScope,current);
        } while (current!=head);
    }
    freeDeclList(args);
}
struct class_info *TypeClassInfo(TYPE type)
{
    type=ClassTypeForType(type);
    return type ? type->u.c.info : NULL;
}
struct class_info *getClassInfo(TYPE type)
{
    TYPE array;
    struct class_info *info;
    if (!type) return NULL;
    array=ArrayType(type);
    if (array) type=ArrayBaseType(array);
    type=StructType(type);
    if (!type) return NULL;
    info=TypeClassInfo(type);
    return info->corrupted ? NULL : info;
}
int TypeRequiresDtoring(TYPE type)
{
    struct class_info *info=getClassInfo(type);
    return info ? info->needs_dtor : 0;
}

PTREE DoDeclSpec(DECL_SPEC *spec)
{ return PTreeType(declSpecType(spec)); }



void *TypeHasPragma(TYPE type)
{
    void *pragma;
    for (;type;type=type->of) {
        if (type->id==TYP_MODIFIER) {
            pragma=type->u.m.pragma;
            if (pragma) return pragma;
            continue;
        }
        if (type->id!=TYP_TYPEDEF) break;
    }
    if (type->id==TYP_FUNCTION) return type->u.f.pragma;
    return NULL;
}

int TypeRequiresCtoring(TYPE type)
{
    struct class_info *info=getClassInfo(type);
    return info ? info->needs_ctor : 0;
}

SYMBOL ScopeFunctionInProgress(void);
TYPE TypeThisSymbol(SYMBOL sym,int reference)
{
    SCOPE scope;
    TYPE type=NULL;
    if (!sym) return NULL;
    scope=SymScope(sym);
    if (!scope) return NULL;
    type=ScopeClass(scope);
    if (!type || sym->id==2) return NULL;
    type=MakeModifiedType(type,FunctionThisFlags(sym));
    if (reference) type=MakeReferenceTo(type);
    else type=MakePointerTo(type);
    return MakeModifiedType(type,1);
}
TYPE TypeThis(void) { return TypeThisSymbol(ScopeFunctionInProgress(),0); }

TYPE TypeNeedsCtor(TYPE type)
{
    for(;;) {
        while(type && (type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER)) type=type->of;
        /* The original dereferences NULL here; it is not a valid type domain. */
        if(!type) { STUB_TRAP(); return NULL; }
        if(type->id==TYP_ARRAY) { type=type->of; continue; }
        if(type->id==TYP_CLASS && type->u.c.info->needs_ctor && !type->u.c.info->corrupted)
            return type;
        return NULL;
    }
}

TYPE TypePtrToChar(void) { return ptr_char; }
TYPE TypePtrToUInt(void) { return ptr_uint; }
TYPE TypePtrToConstUInt(void) { return ptr_const_uint; }

char *CppVFTableName(SCOPE scope,long offset);
SYMBOL ScopeAlreadyExists(SCOPE scope,char *name);
SYMBOL alreadyDefined(char *name) { return ScopeAlreadyExists(FileScope,name); }
TYPE MakeArrayType(unsigned long count)
{
    TYPE type=MakeType(TYP_ARRAY);
    type->u.a.array_size=count;
    return type;
}
TYPE MakeArrayOf(unsigned count,TYPE type)
{
    return MakeTypeOf(MakeArrayType(count),type);
}
TYPE makeCompilerData(TYPE type,unsigned short flags)
{
    if(defaultDataMemFlag&(8|32)) flags|=8;
    return MakeModifiedType(type,flags);
}
TYPE MakeCompilerConstCommonData(TYPE type)
{
    return makeCompilerData(type,0x801);
}
SYMBOL MakeVFTableSym(SCOPE scope,unsigned count,long offset)
{
    char *name=CppVFTableName(scope,offset);
    SYMBOL sym=alreadyDefined(name);
    TYPE type;
    if(!sym) {
        type=MakePointerTo(void_fun_of_void);
        type=MakeArrayOf(count+1,type);
        type=MakeCompilerConstCommonData(type);
        sym=AllocSymbol();
        sym->id=10;
        sym->sym_type=type;
        sym->flag|=1;
        sym=InsertSymbol(FileScope,sym,name);
        LinkageSet(sym,"C++");
    }
    return sym;
}

TYPE MakeVBTableFieldType(int add_consts)
{
    TYPE type=GetBasicType(TYP_UINT);
    type=MakeConstTypeOf(type);
    return makeVTableType(type,add_consts);
}

int typePoints(TYPE type)
{
    return type ? (1UL<<type->id)&((1UL<<TYP_POINTER)|(1UL<<TYP_FUNCTION)|(1UL<<TYP_ARRAY)) : 0;
}
TYPE TypePointedAtModified(TYPE type)
{
    TYPE original=type;
    if(type) {
        while(type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER) type=type->of;
        if(typePoints(type)) type=type->id==TYP_FUNCTION ? original : type->of;
        else type=NULL;
    }
    return type;
}
TYPE TypePointedAt(TYPE type,unsigned short *flags)
{
    return TypeGetActualFlags(TypePointedAtModified(type),flags);
}

unsigned char ScopeDerived(SCOPE derived,SCOPE possible_base);
unsigned char TypeDerived(TYPE derived,TYPE possible_base)
{
    SCOPE derived_scope=TypeScope(derived),base_scope;
    if(!derived_scope) return 0;
    base_scope=TypeScope(possible_base);
    if(!base_scope) return 0;
    return ScopeDerived(derived_scope,base_scope);
}

/* Old exclusion keeps char/enum nodes but supplies default memory flags. */
TYPE TypeModFlagsEC(TYPE type,unsigned short *flags)
{
    unsigned short accumulated=0;
    for(;type;type=type->of) {
        if(type->id==TYP_MODIFIER) accumulated|=type->flag;
        else if(type->id!=TYP_TYPEDEF) {
            if(!(accumulated&0x73c)) accumulated|=defaultMemoryFlag(type);
            break;
        }
    }
    *flags=accumulated;
    return type;
}
unsigned short BaseTypeClassFlags(TYPE type)
{
    unsigned short flags=0;
    if(type) {
        type=TypeModFlagsEC(type,&flags);
        if(type->id==TYP_POINTER) TypeModFlagsEC(type->of,&flags);
    }
    return flags&0x73f;
}

TYPE EnumTypeForType(TYPE type) { return typeTested(type,TYP_ENUM); }
TYPE UserTypeForType(TYPE type)
{
    TYPE user=ClassTypeForType(type);
    return user ? user : EnumTypeForType(type);
}

TYPE MakeBasedModifierOf(TYPE type,unsigned short flags,void *base)
{
    return MakeTypeOf(makeFullModifier(flags,base,NULL),type);
}

TYPE PointerType(TYPE type)
{
    while(type->id==TYP_TYPEDEF || type->id==TYP_MODIFIER) type=type->of;
    if(type->id!=TYP_POINTER || (type->flag&1)) return NULL;
    return type;
}

TYPE MakeUCharArray(unsigned count)
{ return MakeArrayOf(count,GetBasicType(TYP_UCHAR)); }

int ReverseParms(void *pragma);
int TypeHasReverseArgs(TYPE type)
{
    void *pragma;
    type=FunctionDeclarationType(type);
    if(!type || TypeHasEllipsisArg(type)) return 0;
    pragma=TypeHasPragma(type);
    return pragma?ReverseParms(pragma):0;
}

TYPE MakeCommonCodeData(TYPE type)
{ return MakeModifiedType(type,0x800); }
/* Actual1994 has separate copy-pragma and inline arguments, preceding the
 * released control bitmask. Its preserved function flag mask is0x0087. */
TYPE makeThunkType(TYPE type,int copy_pragma,int make_inline)
{
    TYPE old_fn=FunctionDeclarationType(type),new_fn;
    int reuse=TypeArgList(old_fn)->except_spec==NULL;
    new_fn=dupFunction(old_fn,reuse);
    new_fn->flag&=0x87;
    if(!reuse) TypeArgList(new_fn)->except_spec=NULL;
    if(!copy_pragma) new_fn->u.f.pragma=NULL;
    if(make_inline) new_fn->flag|=8;
    return MakeCommonCodeData(CheckDupType(new_fn));
}
TYPE MakeThunkFunction(TYPE type)
{ return makeThunkType(type,0,1); }

TYPE TypePtrVoidFunOfVoid(void) { return ptr_void_fun_of_void; }

int TypePartiallyDefined(TYPE type)
{ (void)type; STUB_TRAP(); return 0; }

int ScopeHasVirtualFunctions(SCOPE scope);
int TypeCtorDispRequired(TYPE derived,TYPE base)
{
    if (!ScopeHasVirtualFunctions(base->u.c.scope)) return 0;
    return derived->u.c.info->has_vcdtor!=0;
}

char *CppVBTableName(SCOPE scope,long offset);
SYMBOL MakeVBTableSym(SCOPE scope,unsigned count,long offset)
{
    char *name=CppVBTableName(scope,offset);
    SYMBOL sym=alreadyDefined(name);
    TYPE type;
    if (!sym) {
        type=MakeArrayOf(count+1,GetBasicType(TYP_UINT));
        type=MakeCompilerConstCommonData(type);
        sym=AllocSymbol();
        sym->id=10;
        sym->sym_type=type;
        sym->flag|=1;
        sym=InsertSymbol(FileScope,sym,name);
        LinkageSet(sym,"C++");
    }
    return sym;
}

int ScopeHasVirtualBases(SCOPE scope);
SCOPE TypeScope(TYPE type);
int TypeHasVirtualBases(TYPE type);
int TypeRequiresCtorParm(TYPE type)
{
    return TypeHasVirtualBases(type);
}
int TypeHasVirtualBases(TYPE type)
{
    type=StructType(type);
    if (type) return ScopeHasVirtualBases(TypeScope(type));
    return 0;
}

TYPE MakeThunkPragmaFunction(TYPE type)
{
    return makeThunkType(type,1,0);
}

TYPE StructOpened(TYPE type)
{
    type=StructType(type);
    return type && type->u.c.info->opened?type:NULL;
}

TYPE TypeModFlagsBaseEC(TYPE type,unsigned short *flags,void **base)
{
    unsigned short accumulated=0;
    *base=NULL;
    for(;type!=NULL;type=type->of) {
        if(type->id==TYP_MODIFIER) {
            accumulated|=type->flag;
            if(type->flag&0x700) *base=type->u.m.base;
        } else if(type->id!=TYP_TYPEDEF) {
            if(!(accumulated&0x73c)) accumulated|=defaultMemoryFlag(type);
            break;
        }
    }
    *flags=accumulated;
    return type;
}
TYPE DupArrayTypeForArray(TYPE type,unsigned size)
{ (void)type; (void)size; STUB_TRAP(); return NULL; }

TYPE TypeOverrideModifiers(TYPE previous,TYPE current)
{ (void)previous; (void)current; STUB_TRAP(); return NULL; }

int TypeIsCharString(TYPE type) { (void)type; STUB_TRAP(); return 0; }

TYPE MakeCombinedFunctionType(TYPE previous,TYPE current)
{
    unsigned short add_flags=FunctionDeclarationType(current)->flag&~4;
    if(add_flags!=0) previous=adjustFunctionType(previous,add_flags,NULL);
    return previous;
}

DECL_SPEC *CheckTypeSpecifier(DECL_SPEC *spec)
{
    figureOutStgClass(spec);
    if(spec->stg_class!=0) {
        spec->stg_class=0;
        CErr1(146);
    }
    if(spec->specifier&0x1c) {
        spec->specifier&=~0x1c;
        CErr1(145);
    }
    if(spec->type_declared || spec->type_defined) CErr1(197);
    return spec;
}

DECL_INFO *MakeNewDynamicArray(PTREE nelem)
{
    DECL_INFO *info=makeDeclInfo(NULL);
    info->defarg_expr=nelem;
    return info;
}

DECL_INFO *MakeNewDeclarator(DECL_SPEC *spec,DECL_INFO *ptrs,DECL_INFO *arrays)
{
    if(arrays==NULL) arrays=makeDeclInfo(NULL);
    if(ptrs!=NULL) {
        arrays=AddDeclarator(arrays,ptrs->list);
        ptrs->list=NULL;
        FreeDeclInfo(ptrs);
    }
    return FinishDeclarator(spec,arrays);
}

int newTypeIllegal(TYPE type)
{
    unsigned short flags;
    int error=0;
    TYPE test=TypeModFlags(type,&flags);
    switch(test->id) {
    case TYP_FUNCTION: CErr1(398); error=1; break;
    case TYP_VOID: CErr1(397); error=1; break;
    }
    if(flags&3) CErr1(399);
    return error;
}
PTREE PTreeErrorNode(PTREE expr);
PTREE PTreeBinary(unsigned char op,PTREE left,PTREE right);
PTREE MakeNewExpr(PTREE global,PTREE placement,DECL_INFO *info,PTREE init)
{
    PTREE nelem,new_type,parms1,parms2;
    TYPE type;
    unsigned char operation=63;
    if(global!=NULL) { PTreeFreeSubtrees(global); operation=64; }
    nelem=info->defarg_expr;
    if(nelem!=NULL) info->defarg_expr=NULL;
    type=info->type;
    FreeDeclInfo(info);
    if(newTypeIllegal(type)) {
        PTreeFreeSubtrees(init);
        return PTreeErrorNode(placement);
    }
    new_type=PTreeType(type);
    parms2=PTreeBinary(67,nelem,init);
    parms1=PTreeBinary(66,placement,parms2);
    return PTreeBinary(operation,new_type,parms1);
}

TYPE AbstractClassType(TYPE type)
{
    type=StructType(type);
    if(type!=NULL && verifyAbstractStatus(type)) return type;
    return NULL;
}

TYPE GetReferenceTo(TYPE type)
{
    if(!ReferenceType(type)) type=MakeReferenceTo(type);
    return type;
}

TYPE TypePointedAtReplace(TYPE type,TYPE pointed)
{ (void)type; (void)pointed; STUB_TRAP(); return NULL; }

TYPE SegmentShortType(TYPE type)
{ return type->id==TYP_USHORT && (type->flag&1)?type:NULL; }

TYPE PointerTypeEquivalent(TYPE type);
TYPE BasedPtrType(TYPE type)
{
    type=PointerTypeEquivalent(type);
    return type ? BasedType(type->of) : type;
}

TYPE MakeNonInlineFunction(TYPE type)
{
    return adjustFullFunctionType(type,0,8,NULL,NULL);
}

DECL_SPEC *PTypeLinkage(void)
{
    DECL_SPEC *spec=makeDeclSpec();
    spec->linkage=CurrLinkage;
    return spec;
}

DECL_SPEC *PTypeClassInstantiation(TYPE type,PTREE id)
{
    DECL_SPEC *spec=PTypeActualTypeName(type,id);
    spec->class_instantiation=1;
    return spec;
}

void CheckDimension(PTREE expr)
{
    if(expr->u.uint_constant==0) {
        CErr1(34);
        expr->u.uint_constant=1;
    } else if(expr->op==2 && expr->u.int_constant<0) {
        CErr1(35);
        expr->u.uint_constant=0UL-expr->u.uint_constant;
    }
}
DECL_INFO *AddArrayDeclarator(DECL_INFO *info,PTREE size)
{
    TYPE type;
    if(size!=NULL) {
        CheckDimension(size);
        type=MakeArrayType(size->u.uint_constant);
        PTreeFree(size);
    } else type=MakeArrayType(0);
    return AddDeclarator(info,type);
}

/* The old binder passes three16-byte stack controls separately, before
 * the later type_bind_info structure and derived-conversion class. */
void binderInit(VSTK_CTL *with_generic,VSTK_CTL *without_generic,VSTK_CTL *bindings)
{
    VstkOpen(with_generic,sizeof(TYPE),8);
    VstkOpen(without_generic,sizeof(TYPE),8);
    VstkOpen(bindings,sizeof(TYPE),8);
}
void binderFini(VSTK_CTL *with_generic,VSTK_CTL *without_generic,VSTK_CTL *bindings)
{
    VstkClose(with_generic);
    VstkClose(without_generic);
    VstkClose(bindings);
}
void pushPrototypeAndArguments(VSTK_CTL *with_generic,arg_list *prototype,
                     VSTK_CTL *without_generic,arg_list *actual,unsigned control)
{
    unsigned i;
    TYPE p_type,a_type,ref;
    for(i=0;i<prototype->num_args;++i) {
        p_type=prototype->type_list[i];
        while(p_type && (p_type->id==TYP_TYPEDEF || p_type->id==TYP_MODIFIER))
            p_type=p_type->of;
        a_type=actual->type_list[i];
        while(a_type && (a_type->id==TYP_TYPEDEF || a_type->id==TYP_MODIFIER))
            a_type=a_type->of;
        ref=ReferenceType(a_type);
        if(ref) {
            a_type=ref;
            ref=ReferenceType(p_type);
            if(ref) p_type=ref;
            else {
                while(a_type && (a_type->id==TYP_TYPEDEF || a_type->id==TYP_MODIFIER))
                    a_type=a_type->of;
            }
        } else {
            ref=ReferenceType(p_type);
            if(ref) p_type=ref;
        }
        a_type=adjustParmType(a_type);
        pushType(with_generic,p_type);
        pushType(without_generic,a_type);
        /* CFbe608 is the existing distinct error-type object allocated in
         * typesInit; the old binder uses its identity as a level marker. */
        if(control&1) pushType(with_generic,diagnostic_error);
    }
}
int TypeBasesEqual(unsigned short flags,void *first,void *second);
int modifiersMatch(unsigned short bound_flags,unsigned short unbound_flags,
                   void *bound_base,void *unbound_base)
{
    if(bound_flags!=unbound_flags) return 0;
    return !(bound_flags&0x700) || TypeBasesEqual(bound_flags,bound_base,unbound_base);
}
int compareClassTypes(TYPE bound,TYPE unbound,VSTK_CTL *bound_stack,VSTK_CTL *unbound_stack)
{ (void)bound; (void)unbound; (void)bound_stack; (void)unbound_stack; STUB_TRAP(); return 1; }
/* Adapted initial type.c typesBind. The1994 binder has only exact/trivial
 * outcomes, retains rejection of reference→generic bindings, and owns the
 * three separate stacks. It has no later derived-base or first-pointer helper. */
unsigned typesBind(VSTK_CTL *without_generic,VSTK_CTL *with_generic,VSTK_CTL *bindings)
{
    TYPE b_type,u_type,b_unmod,u_unmod,g_type,match,t_unmod;
    TYPE *b_top,*u_top;
    unsigned short b_flags,u_flags,g_flags,t_flags,d_flags;
    void *b_base,*u_base,*g_base,*t_base;
    arg_list *b_args,*u_args;
    unsigned i,status=1;
    int first_level;
    for(;;) {
        b_top=VstkPop(without_generic);
        u_top=VstkPop(with_generic);
        if(!b_top || !u_top) break;
        b_type=*b_top;
        if(!b_type) return 0;
        b_unmod=TypeModFlagsBaseEC(b_type,&b_flags,&b_base);
        u_type=*u_top;
        first_level=0;
        if(u_type==diagnostic_error) {
            first_level=1;
            u_top=VstkPop(with_generic);
            if(!u_top) break;
            u_type=*u_top;
        }
        if(!u_type) return 0;
        u_unmod=TypeModFlagsBaseEC(u_type,&u_flags,&u_base);
        if(b_unmod==u_unmod) {
            if(!modifiersMatch(b_flags,u_flags,b_base,u_base)) return 0;
            continue;
        }
        if(b_unmod->id!=u_unmod->id) {
            if(u_unmod->id!=TYP_GENERIC || ReferenceType(b_unmod)) return 0;
            b_unmod=TypeModExtract(b_type,&b_flags,&b_base,1);
            u_unmod=TypeModExtract(u_type,&u_flags,&u_base,1);
            d_flags=u_flags^(b_flags&u_flags);
            if(d_flags) {
                if(!first_level || (d_flags&~3)) return 0;
                status|=2;
                u_flags&=~d_flags;
            }
            if(u_flags&0x700) {
                if((u_flags&0x700)!=(b_flags&0x700)) return 0;
                if(!TypeBasesEqual(u_flags,b_base,u_base)) return 0;
            }
            g_flags=b_flags&~u_flags;
            g_type=b_unmod;
            if(CompFlags.xcmb_switch_used && CompFlags.extensions_enabled) {
                g_base=(g_flags&0x700)?b_base:NULL;
                if(g_flags) g_type=MakeBasedModifierOf(g_type,g_flags,g_base);
            } else {
                g_flags&=3;
                g_base=NULL;
                if(g_flags) g_type=MakeModifiedType(g_type,g_flags);
            }
            match=u_unmod->of;
            if(match) {
                t_unmod=TypeModExtract(match,&t_flags,&t_base,1);
                if(!TypesIdentical(b_unmod,t_unmod)) return 0;
                if((t_flags&~0x73c)!=(g_flags&~0x73c)) return 0;
                if(t_flags&0x73c) {
                    if((g_flags&0x73c) &&
                       !modifiersMatch(t_flags,g_flags,t_base,g_base)) return 0;
                } else if(g_flags&0x73c) u_unmod->of=g_type;
                continue;
            }
            u_unmod->of=g_type;
            pushType(bindings,u_unmod);
            continue;
        }
        if(first_level) {
            d_flags=u_flags^(b_flags&u_flags);
            if(d_flags && !(d_flags&~3)) {
                status|=2;
                u_flags&=~d_flags;
            }
        }
        if(!modifiersMatch(b_flags,u_flags,b_base,u_base)) return 0;
        switch(b_unmod->id) {
        case TYP_CLASS:
            if(compareClassTypes(b_unmod,u_unmod,without_generic,with_generic)) return 0;
            u_unmod->of=b_unmod;
            pushType(bindings,u_unmod);
            break;
        case TYP_MEMBER_POINTER:
            /* The old order puts hosts on top of pointed-to types. */
            pushType(without_generic,b_unmod->of);
            pushType(with_generic,u_unmod->of);
            pushType(without_generic,b_unmod->u.mp.host);
            pushType(with_generic,u_unmod->u.mp.host);
            break;
        case TYP_ARRAY:
            if(b_unmod->u.a.array_size!=u_unmod->u.a.array_size) return 0;
            pushType(without_generic,b_unmod->of);
            pushType(with_generic,u_unmod->of);
            break;
        case TYP_FUNCTION:
            if(b_unmod->u.f.pragma!=u_unmod->u.f.pragma) return 0;
            b_args=b_unmod->u.f.args;
            u_args=u_unmod->u.f.args;
            if(b_args!=u_args) {
                if(b_args->num_args!=u_args->num_args ||
                   b_args->qualifier!=u_args->qualifier) return 0;
                for(i=0;i<b_args->num_args;++i) {
                    pushType(without_generic,b_args->type_list[i]);
                    pushType(with_generic,u_args->type_list[i]);
                }
            }
            /* fall through */
        case TYP_POINTER:
            pushType(without_generic,b_unmod->of);
            pushType(with_generic,u_unmod->of);
            break;
        default:
            if(!TypesIdentical(b_unmod,u_unmod)) return 0;
        }
    }
    return status;
}

#pragma pack(4)
typedef struct type_bind_stack {
    TYPE old_type;
    TYPE *link_type;
    arg_list *type_args;
    unsigned char op;
} TYPE_BIND_STACK;
#pragma pack()
typedef char TypeBindStackSize[(sizeof(TYPE_BIND_STACK)==16)?1:-1];
enum { BIND_NULL=0,BIND_BIND_TYPE=1,BIND_DUP_TYPE=2,BIND_CLASS_TEMPLATE=3 };
void pushBinding(VSTK_CTL *stack,TYPE old_type,TYPE *link_type)
{
    TYPE_BIND_STACK *top;
    top=VstkPush(stack);
    top->old_type=NULL;
    top->type_args=NULL;
    top->op=BIND_DUP_TYPE;
    top->link_type=link_type;
    top=VstkPush(stack);
    top->type_args=NULL;
    top->op=BIND_BIND_TYPE;
    top->old_type=old_type;
    top->link_type=link_type;
}
TYPE *dupExceptSpec(TYPE *types)
{ (void)types; STUB_TRAP(); return NULL; }
int handleUnboundClass(VSTK_CTL *stack,TYPE old_type,TYPE *link)
{ (void)stack; (void)old_type; (void)link; STUB_TRAP(); return 1; }
int instantiateUnbound(TYPE old_type,TYPE *link,arg_list *args,CGIO_LOCATION *locn)
{ (void)old_type; (void)link; (void)args; (void)locn; STUB_TRAP(); return 1; }
enum { TF1_UNBOUND=4, DF_NULL=0 };
/* Initial performBinding source, with actual1994 direct CheckDupType.
 * Its16B stack records and original trailing epilogue remain distinct. */
int performBinding( VSTK_CTL *stk, CGIO_LOCATION *locn )
{
    int type_is_OK;
    unsigned i;
    arg_list *new_args;
    arg_list *old_args;
    TYPE old_type;
    TYPE *link_type;
    TYPE new_type;
    TYPE *old_arg;
    TYPE *new_arg;
    TYPE *old_except;
    TYPE *new_except;
    TYPE_BIND_STACK *top;

    type_is_OK = 1;
    for(;;) {
        top = VstkPop( stk );
        if( top == NULL ) break;
        /* fields must be extracted before any pushes */
        old_type = top->old_type;
        link_type = top->link_type;
        switch( top->op ) {
        case BIND_DUP_TYPE:
            *link_type = CheckDupType( *link_type );
            continue;
        case BIND_CLASS_TEMPLATE:
            if( instantiateUnbound( old_type, link_type, top->type_args, locn ) ) {
                type_is_OK = 0;
            }
            continue;
        }
        new_type = old_type;
        switch( old_type->id ) {
        case TYP_POINTER:
            new_type = MakeType( TYP_POINTER );
            new_type->flag = old_type->flag;
            pushBinding( stk, old_type->of, &(new_type->of) );
            break;
        case TYP_TYPEDEF:
            pushBinding( stk, old_type->of, link_type );
            break;
        case TYP_CLASS:
            if( old_type->flag & TF1_UNBOUND ) {
                if( old_type->of == NULL ) {
                    if( handleUnboundClass( stk, old_type, link_type ) ) {
                        type_is_OK = 0;
                    }
                } else {
                    new_type = old_type->of;
                }
            }
            break;
        case TYP_FUNCTION:
            new_type = dupFunction( old_type, DF_NULL );
            old_args = TypeArgList( old_type );
            old_arg = old_args->type_list;
            new_args = TypeArgList( new_type );
            new_arg = new_args->type_list;
            for( i = new_args->num_args; i != 0; --i ) {
                pushBinding( stk, *old_arg, new_arg );
                ++old_arg;
                ++new_arg;
            }
            new_except = new_args->except_spec;
            if( new_except != NULL ) {
                /* dupFunction doesn't duplicate the except-spec */
                old_except = old_args->except_spec;
                new_except = dupExceptSpec( old_except );
                new_args->except_spec = new_except;
                for(;;) {
                    if( *old_except == NULL || *new_except == NULL ) break;
                    pushBinding( stk, *old_except, new_except );
                    ++old_except;
                    ++new_except;
                }
            }
            pushBinding( stk, old_type->of, &(new_type->of) );
            break;
        case TYP_ARRAY:
            new_type = MakeArrayType( old_type->u.a.array_size );
            pushBinding( stk, old_type->of, &(new_type->of) );
            break;
        case TYP_MODIFIER:
            new_type = dupModifier( old_type );
            pushBinding( stk, old_type->of, &(new_type->of) );
            break;
        case TYP_MEMBER_POINTER:
            new_type = MakeType( TYP_MEMBER_POINTER );
            new_type->flag = old_type->flag;
            pushBinding( stk, old_type->of, &(new_type->of) );
            pushBinding( stk, old_type->u.mp.host, &(new_type->u.mp.host) );
            break;
        case TYP_GENERIC:
            new_type = old_type->of;
            break;
        }
        *link_type = new_type;
    }
    return( type_is_OK );
}

TYPE createBoundType(TYPE type,CGIO_LOCATION *locn)
{
    VSTK_CTL stack;
    TYPE bound=NULL;
    VstkOpen(&stack,sizeof(TYPE_BIND_STACK),8);
    pushBinding(&stack,type,&bound);
    if(!performBinding(&stack,locn)) bound=NULL;
    VstkClose(&stack);
    return bound;
}
void clearGenericBindings(VSTK_CTL *bindings)
{
    TYPE *top;
    for(;;) {
        top=VstkPop(bindings);
        if(!top) break;
        if(*top) (*top)->of=NULL;
    }
}
TYPE BindGenericTypes(arg_list *args,SYMBOL fn,CGIO_LOCATION *locn,unsigned *control)
{
    VSTK_CTL with_generic,without_generic,bindings;
    TYPE type,bound=NULL;
    unsigned flags,status;
    flags=(*control && CompFlags.extensions_enabled)?1:0;
    *control=0;
    binderInit(&with_generic,&without_generic,&bindings);
    type=fn->sym_type;
    pushPrototypeAndArguments(&with_generic,TypeArgList(type),&without_generic,args,flags);
    status=typesBind(&without_generic,&with_generic,&bindings);
    if(status) {
        if(status&2) *control=1;
        bound=createBoundType(type,locn);
    }
    clearGenericBindings(&bindings);
    binderFini(&with_generic,&without_generic,&bindings);
    return bound;
}

DECL_INFO *MakeNewTypeId(PTREE type_id)
{
    TYPE type=type_id->type;
    DECL_INFO *info;
    PTreeFree(type_id);
    info=makeDeclInfo(NULL);
    info->type=type;
    return info;
}

/* Expandable arrays deliberately bypass the canonical type chains. */
TYPE MakeExpandableType(type_id base)
{
    TYPE type=MakeArrayType(1);
    type->of=GetBasicType(base);
    return type;
}

TYPE MakeForceInMemory(TYPE type) { return MakeModifiedType(type,0x1000); }

unsigned char AreFunctionsDistinct(SYMBOL *old,SYMBOL current,char *name);
int sameBoundFunction(TYPE type,SYMBOL file_sym)
{
    SYMBOL test=AllocSymbol();
    unsigned char result;
    test->id=1;
    test->sym_type=type;
    result=AreFunctionsDistinct(&file_sym,test,file_sym->name->name);
    FreeSymbol(test);
    return result==3;
}
void TemplateFunctionInstantiate(SYMBOL sym,SYMBOL model,void *bindings);
int BindFunction(SYMBOL sym,SYMBOL model)
{
    TYPE bound;
    unsigned status;
    arg_list *sym_args=SymFuncArgList(sym),*model_args=SymFuncArgList(model);
    VSTK_CTL with_generic,without_generic,bindings;
    if(sym_args->num_args!=model_args->num_args) return 0;
    binderInit(&with_generic,&without_generic,&bindings);
    pushArguments(&with_generic,model_args);
    pushArguments(&without_generic,sym_args);
    status=typesBind(&without_generic,&with_generic,&bindings);
    if(status) {
        bound=createBoundType(model->sym_type,(CGIO_LOCATION *)sym->locn);
        if(bound && sameBoundFunction(bound,sym)) {
            TemplateFunctionInstantiate(sym,model,&bindings);
            /* Actual1994 success returns directly, without the later
             * binderFini call present in the released source. */
            return 1;
        }
    }
    clearGenericBindings(&bindings);
    binderFini(&with_generic,&without_generic,&bindings);
    return 0;
}

TYPE PTypeListOfTypes(type_id id)
{
    return typeTable[id];
}
