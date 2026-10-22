/* Interned frontend types and the associated parameter tables. */
#include "types.h"
#include "operators.h"
#include <recon.h>

TypeEntry **type_descriptor_buckets; /* original 0x9f410 */
TypeShapeEntry **type_shape_buckets; /* original 0x9f1e8 */
ParameterEntry **parameter_buckets; /* original 0x9f1dc */
DeclarationParameterEntry **declaration_parameter_buckets; /* original 0x9f3d8 */

/* Original 0x61e60..0x61e9d. */
void initialize_type_table(void)
{
    type_descriptor_buckets = allocate_permanent(4096);
    type_shape_buckets = allocate_permanent(4096);
    parameter_buckets = allocate_permanent(2048);
    declaration_parameter_buckets = allocate_permanent(2048);
}

/* Original 0x62080..0x62149. Debug indices do not participate in identity. */
CType *__stdcall intern_type(TypeDescription *description)
{
    unsigned flags = description->flags;
    unsigned attributes = description->attributes;
    unsigned hash;
    TypeEntry **bucket, *entry;
    hash = ((attributes >> 4) & 0xfffc) ^ (attributes << 2);
    hash += ((flags >> 4) & 0xfffc) ^ (flags << 2);
    hash += ((unsigned)description->related & 0xffc) * 4;
    hash += (short)description->field08 * 4;
    bucket = &type_descriptor_buckets[(hash & 0xffc) >> 2];
    for (entry = *bucket; entry; entry = entry->next) {
        if (entry->type.related == description->related &&
            entry->type.flags == flags &&
            entry->type.attributes == attributes &&
            entry->type.field08 == (short)description->field08)
            return &entry->type;
    }
    entry = allocate_permanent(sizeof(TypeEntry));
    entry->next = *bucket;
    *bucket = entry;
    entry->type.flags = description->flags;
    entry->type.attributes = description->attributes;
    entry->type.related = description->related;
    entry->type.field08 = description->field08;
    entry->type.debug_index = options.configuration.debug_info ? 0 : description->field0a;
    return &entry->type;
}

/* Calling-convention/pointer-model cache, original 0x9f2d4..0x9f328.
   Valid combinations use conventions 0..6 and near/far models 1..2. */
TypeShape *function_shapes[21];
typedef char FunctionShapeCacheBytesMustBe84[(sizeof(function_shapes) == 84) ? 1 : -1];
TypeDescription function_type_scratch; /* original 0x98150, pointer 0xa1e2c */
TypeShape function_shape_scratch;      /* original 0x98160, pointer 0xa1e30 */

/* Original 0x62250..0x6231d. */
CType *__stdcall make_function_type(unsigned short attributes, CType *base)
{
    unsigned index = ((attributes & 0x70) >> 4) * 3 + (attributes & 15);
    TypeShape *shape;
    if (!function_shapes[index])
        function_shapes[index] = intern_type_shape(new_type_shape(4,
            normalize_type_attributes(4, attributes), 0));
    if (base->related) {
        function_shape_scratch = *function_shapes[index];
        function_shape_scratch.related = intern_type_shape(base->related);
        shape = intern_type_shape(&function_shape_scratch);
    } else
        shape = function_shapes[index];
    function_type_scratch.related = shape;
    function_type_scratch.flags = base->flags;
    function_type_scratch.attributes = shape->attributes;
    return intern_type(&function_type_scratch);
}

/* Original 0x62c30..0x62de3. Parameter data has a different meaning for
   based pointers, arrays, functions, and the C++ class/member shapes. */
TypeShape *__stdcall new_type_shape(unsigned short kind, unsigned short attributes,
                                    void *parameter)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    TypeShape *shape;
    unsigned short adjusted_kind;
    if (pool->remaining < (int)sizeof(TypeShape))
        refill_arena(&compiler_arena, pool);
    shape = (TypeShape *)pool->next;
    pool->remaining -= sizeof(TypeShape);
    pool->next += sizeof(TypeShape);
    shape->flags = kind;
    shape->attributes = attributes;
    switch (kind & 0x77) {
    case 0:
        if (parameter) {
            if ((attributes & 12) && attributes != 5) {
                report_syntax_error(493);
                shape->attributes = 0;
                shape->data.word = 0;
            } else
                shape->data.word = *(unsigned *)parameter;
        }
        break;
    case 1:
    case 0x40:
        if ((attributes & 4) && parameter)
            shape->data.word = *(unsigned *)parameter;
        break;
    case 2:
        if (kind & 0x1000)
            shape->data.array_bound = *(int *)parameter;
        else if (parameter) {
            if (*(int *)parameter < 0) {
                report_syntax_error(118);
                *(int *)parameter = 1;
            }
            shape->data.array_bound = *(int *)parameter;
        } else
            shape->data.array_bound = 0;
        break;
    case 4:
        if (kind & 0x400) {
            shape->flags &= 0xfbff;
            shape->data.parameters = *(FunctionParameters **)parameter;
        } else {
            adjusted_kind = kind;
            shape->data.parameters = build_parameter_info(parameter, &adjusted_kind);
            shape->flags |= adjusted_kind;
        }
        break;
    case 0x10:
    case 0x20:
        shape->data.word = *(unsigned *)parameter;
        break;
    }
    return shape;
}

/* Original 0x62150..0x62242. Canonical shapes are appended to their buckets. */
TypeShape *__stdcall intern_type_shape(TypeShape *shape)
{
    unsigned hash, category, attributes;
    TypeShapeEntry **bucket, *entry, *previous = 0;
    if (!shape)
        return 0;
    category = shape->flags & 0x77;
    attributes = shape->attributes & 7;
    if ((category == 1 || category == 3 || category == 0x40 || category == 0x44) &&
        (attributes < 5 || attributes > 7))
        shape->data.word = 0;
    hash = (shape->flags >> 6) ^ shape->flags;
    hash += (shape->attributes >> 6) + shape->attributes;
    hash += (unsigned)shape->related & 0xffc;
    hash += (shape->data.word & 0xffc) >> 2;
    bucket = &type_shape_buckets[hash & 0x3ff];
    for (entry = *bucket; entry; entry = entry->next) {
        if (&entry->shape == shape ||
            (entry->shape.flags == shape->flags &&
             entry->shape.attributes == shape->attributes &&
             entry->shape.related == shape->related &&
             entry->shape.data.word == shape->data.word))
            return &entry->shape;
        previous = entry;
    }
    entry = allocate_permanent(sizeof(TypeShapeEntry));
    if (previous)
        previous->next = entry;
    else
        *bucket = entry;
    entry->shape = *shape;
    return &entry->shape;
}

/* Original 0x63430..0x634eb. */
CType *__stdcall make_tagged_type(unsigned short flags)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    TypeDescription *description;
    if (flags & 0x80) {
        if (pool->remaining < (int)sizeof(TypeDescription))
            refill_arena(&compiler_arena, pool);
        description = (TypeDescription *)pool->next;
        pool->remaining -= sizeof(TypeDescription);
        pool->next += sizeof(TypeDescription);
        description->attributes = 0;
        description->field08 = 0;
        description->flags = flags;
        description->related = intern_type_shape(new_type_shape(1,
            normalize_type_attributes(1, 0), 0));
    } else {
        if (token_allocation_flags & 0x20)
            description = allocate_permanent(sizeof(TypeDescription));
        else {
            if (pool->remaining < (int)sizeof(TypeDescription))
                refill_arena(&compiler_arena, pool);
            description = (TypeDescription *)pool->next;
            pool->remaining -= sizeof(TypeDescription);
            pool->next += sizeof(TypeDescription);
        }
        description->flags = flags;
    }
    return intern_type(description);
}

int type_description_depth; /* original 0x9116c */
TypeDescription type_description_stack[10]; /* original 0x91170..0x911e8 */

/* Original 0x628b0..0x628dc. Entry zero is reserved by pre-increment. */
TypeDescription *__stdcall copy_type_description(CType *type)
{
    TypeDescription *description = &type_description_stack[++type_description_depth];
    description->flags = type->flags;
    description->attributes = type->attributes;
    description->related = type->related;
    description->field08 = type->field08;
    description->field0a = type->debug_index;
    return description;
}

/* Original [0x50e70,0x50e95). Intern an unqualified description and
   release exactly the temporary description-stack entry used for the copy. */
CType *__stdcall remove_type_qualifiers(CType *type, unsigned short qualifiers)
{
    TypeDescription *description = copy_type_description(type);
    CType *result;
    description->attributes &= ~qualifiers;
    result = intern_type(description);
    --type_description_depth;
    return result;
}

/* Original 0x626e0..0x62722. Prepend and intern a qualified shape. */
CType *__stdcall make_qualified_type(CType *base, unsigned short kind,
                                     unsigned short attributes, void *parameter)
{
    TypeDescription *description = copy_type_description(base);
    TypeShape *shape = new_type_shape(kind, attributes, parameter);
    CType *result;
    shape->related = description->related;
    description->related = intern_type_shape(shape);
    result = intern_type(description);
    --type_description_depth;
    return result;
}

/* Original 0x3b840..0x3b895. A class/enum operand may have a qualifier
   prefix and a single reference shape, but no pointer or array suffix. */
int __stdcall is_class_name_type(CType *type)
{
    TypeShape *shape;
    if (!type || !(type->flags & 0x600)) return 0;
    shape = type->related;
    if (shape) {
        if (!(shape->flags & 0x77)) shape = shape->related;
        if (shape) return (shape->flags & 0x40) && !shape->related;
    }
    return 1;
}

/* Original 0x61ff0..0x6207b. Complete a mutable descriptor, then consume
   its stack slot. A null descriptor requests the canonical int type. */
CType *__stdcall finish_type(CType *type)
{
    unsigned short flags;
    CType *result;
    if (!type) return type_0064;
    flags = type->flags;
    if (!(flags & 0x2fdf)) flags |= 100;
    flags &= 0xcfff;
    if (flags & 0xc000) {
        if (!(flags & 0x840)) {
            compiler_warning(1, 76, token_spellings[(flags & 0x4000) ? 29 : 28]);
            flags &= 0x3fff;
        } else if ((flags & 0x2fdf) != 0x41) flags &= 0x7fff;
    }
    type->flags = flags;
    result = intern_type((TypeDescription *)type);
    --type_description_depth;
    return result;
}

/* Original mutable prefix at 0x98170, reached through the stable pointer
   0xa1e34. Keep one workspace for the modifier reapplied after replacement. */
TypeShape replacement_shape_prefix;

/* Original 0x62380..0x6248f. Replace the first effective shape, moving array
   cv bits onto the remaining element type and retaining a leading modifier. */
CType *__stdcall replace_first_type_shape(CType *type, unsigned short kind,
    unsigned short attributes, void *parameter)
{
    TypeDescription *description = &type_description_stack[++type_description_depth];
    TypeShape *replacement, *remaining, *following, *element;
    CType *result;
    int prefix = 0;
    description->flags = type->flags;
    description->attributes = type->attributes;
    description->related = type->related;
    description->field08 = type->field08;
    description->field0a = type->debug_index;
    replacement = new_type_shape(kind, attributes, parameter);
    remaining = type->related;
    if (remaining) {
        if (!(remaining->flags & 0x77)) {
            remaining = remaining->related;
            prefix = 1;
        }
        if ((remaining->flags & 0x77) == 2) {
            following = remaining->related;
            if (!following) {
                description->attributes |= remaining->attributes & 0x3000;
                remaining = 0;
            } else {
                element = new_type_shape(following->flags,
                    (unsigned short)(following->attributes | (remaining->attributes & 0x3000)),
                    &following->data);
                element->related = following->related;
                remaining = intern_type_shape(element);
            }
        } else remaining = remaining->related;
    }
    replacement->related = remaining;
    remaining = intern_type_shape(replacement);
    if (prefix) {
        replacement_shape_prefix = *type->related;
        replacement_shape_prefix.related = remaining;
        remaining = intern_type_shape(&replacement_shape_prefix);
    }
    description->related = remaining;
    result = intern_type(description);
    --type_description_depth;
    return result;
}

/* Original 0x62340..0x62353. Strip at most one outer reference shape. */
TypeShape *__stdcall skip_reference_shape(TypeShape *shape)
{
    if (shape && (shape->flags & 0x40)) shape = shape->related;
    return shape;
}

/* Original 0x62320..0x6233c. Strip only a leading based/segment modifier,
   retaining the incoming pointer when no such shape is present. */
CType *__stdcall strip_based_shape(CType *type)
{
    if (type && type->related && !(type->related->flags & 0x77))
        return drop_first_type_shape(type);
    return type;
}

/* Original 0x624f0..0x6254c. Produce a mutable twelve-byte descriptor with
   one outer shape removed, preserving the other type fields verbatim. */
CType *__stdcall drop_first_type_shape(CType *type)
{
    CType *result;
    ArenaPool *pool;
    if (token_allocation_flags & 0x20) result = allocate_permanent(sizeof(CType));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(CType)) refill_arena(&compiler_arena, pool);
        result = (CType *)pool->next;
        pool->remaining -= sizeof(CType);
        pool->next += sizeof(CType);
    }
    *result = *type;
    result->related = type->related->related;
    return result;
}

CppSymbol *pointer_base_symbol; /* original 0x911e8, based-pointer context */

/* Original 0x62490..0x624e6. Drop indirection while carrying its pointer
   model and based-symbol state into the interned result descriptor. */
CType *__stdcall dereference_pointer_type(CType *type)
{
    unsigned short attributes = type->related->attributes;
    if ((attributes & 7) == 4) report_syntax_error(0xd6);
    pointer_base_symbol = type->related->data.based_symbol;
    type = drop_first_type_shape(type);
    type->attributes = (type->attributes & 0xfff8) | (attributes & 15);
    return intern_type((TypeDescription *)type);
}

/* Original 0x62760..0x62803. Convert the outer pointer model into a shape,
   retaining the requested descriptor bits separately from default resolution. */
CType *__stdcall make_pointer_to_type(unsigned short attributes, CType *type)
{
    unsigned short original_attributes, requested, resolved;
    TypeDescription *description;
    int based = 0;
    if (type->related && !(type->related->flags & 0x77))
        type = drop_first_type_shape(type);
    original_attributes = type->attributes;
    requested = attributes & 7;
    resolved = requested;
    if (!resolved) resolved = normalize_type_attributes(1, 0);
    if ((type->attributes ^ resolved) & 7) {
        description = copy_type_description(type);
        description->attributes = (description->attributes & 0xfff8) | requested;
        type = intern_type(description);
        --type_description_depth;
    }
    resolved = normalize_type_attributes(1, original_attributes & 7);
    return make_qualified_type(type, 1, resolved, &based);
}
