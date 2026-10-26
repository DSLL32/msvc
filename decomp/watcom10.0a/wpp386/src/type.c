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
#include "types.h"
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
static struct {
    TYPE ptr_diff_near, ptr_diff_far, ptr_diff_far16, ptr_diff_huge;
} CompInfoTypes;
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
static const struct scalar_type {
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
    clean_int = MakeType(TYP_SINT);
    clean_int->flag |= 1;
    diagnostic_error = MakeType(TYP_ERROR);
    CheckDupType(clean_int);
    type = MakeType(TYP_SINT);
    type->flag |= 2;
    default_int = type;
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
