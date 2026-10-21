/* Interned frontend types and related hash tables. */
#include "types.h"

TypeEntry **type_descriptor_buckets; /* original 0x6b7d0 */
TypeShapeEntry **type_shape_buckets;  /* original 0x6b658 */
ParameterEntry **parameter_buckets;  /* original 0x6b64c */

/* Original 0x3fb20..0x3fb4e. */
void initialize_type_table(void)
{
    type_descriptor_buckets = allocate_permanent(4096);
    type_shape_buckets = allocate_permanent(4096);
    parameter_buckets = allocate_permanent(2048);
}

/* Original 0x3fd60..0x3fe33. */
CType *__stdcall intern_type(TypeDescription *description)
{
    unsigned hash, flags, attributes;
    TypeEntry **bucket, *entry;
    flags = description->flags;
    attributes = description->attributes;
    hash = ((attributes >> 4) & 0xfffc) ^ (attributes << 2);
    hash += ((flags >> 4) & 0xfffc) ^ (flags << 2);
    hash += ((unsigned)description->related & 0xffc) * 4;
    hash += description->field08 & 0xffc;
    bucket = &type_descriptor_buckets[(hash & 0xffc) >> 2];
    for (entry = *bucket; entry; entry = entry->next) {
        if (entry->type.related == description->related &&
            entry->type.flags == flags &&
            entry->type.attributes == attributes &&
            entry->type.field08 == description->field08)
            return &entry->type;
    }
    entry = allocate_permanent(sizeof(TypeEntry));
    entry->next = *bucket;
    *bucket = entry;
    entry->type = *description;
    if (options.debug_info)
        entry->type.field0c &= 0xffff0000;
    return &entry->type;
}

/* Pointer shape cache and reusable descriptors, original 0x6b73c/0x66340/0x66350. */
TypeShape *function_shapes[37];
CType function_type_scratch;
TypeShape function_shape_scratch;

/* Original 0x3ff30..0x3fffd. */
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

int active_plmn_pragma, active_plmf_pragma; /* original 0x6b7ec/0x6b7f0 */

/* Original 0x2cb30..0x2cde8. */
unsigned short __stdcall normalize_type_attributes(unsigned short kind, unsigned short attributes)
{
    unsigned category = kind & 7;
    int function = 0;
    char *conflict;
    if (options.syntax_only)
        return attributes;
    if (category == 1 || category == 2) {
        if (!(attributes & 15))
            attributes |= (unsigned short)code_pointer_kind;
        return attributes;
    }
    if (category != 3 && category != 4)
        return attributes;
    function = category == 4;
    if (option_Au)
        attributes |= 0x800;
    if (!(attributes & 7))
        attributes |= (unsigned short)data_pointer_kind;
    if (!(attributes & 0x70)) {
        if (options.option_Gr || options.option_Gc) {
            if (attributes & 0x100) {
                compiler_warning(2, 96, "cdecl", "interrupt");
                goto default_cdecl;
            }
            if (kind & 0x8000) {
                compiler_warning(2, 96, "cdecl", "...");
                goto default_cdecl;
            }
        }
        if (options.option_Gr)
            attributes |= 0x40;
        else if (options.option_Gz)
            attributes |= (kind & 0x8000) ? 0x10 : 0x60;
        else if (options.option_Gc)
            attributes |= 0x20;
        else
            attributes |= 0x10;
    } else if (((attributes & 0x70) == 0x60 || (attributes & 0x70) == 0x40) &&
               (kind & 0x8000)) {
        attributes &= 0xff8f;
    default_cdecl:
        attributes |= 0x10;
    }
    if (!function)
        return attributes;
    if (attributes & 0x100) {
        if ((attributes & 0x70) != 0x50 && (attributes & 0x70) != 0x10)
            report_syntax_error(217, "interrupt", "cdecl");
        if (attributes & 0x200)
            report_syntax_error(216, "saveregs", "interrupt");
    }
    if ((attributes & 7) == 3 && (attributes & 0x70) != 0x20) {
        attributes &= 0xff8f;
        report_syntax_error(217, "_far16", "pascal");
        attributes |= 0x20;
    }
    if ((kind & 0x8000) && (attributes & 0x70) != 0x60 &&
        (attributes & 0x70) != 0x50 && (attributes & 0x70) != 0x10)
        report_syntax_error(217, "cdecl", "...");
    if ((attributes & 0x70) == 0x40) {
        conflict = 0;
        if (attributes & 0x200)
            conflict = "saveregs";
        else if (attributes & 0x100)
            conflict = "interrupt";
        else if (attributes & 0x400)
            conflict = "export";
        if (conflict) {
            report_syntax_error(216, "_fastcall", conflict);
            attributes &= 0xffbf;
            attributes |= 0x10;
        }
        if (active_plmf_pragma || active_plmn_pragma) {
            report_syntax_error(216, "_fastcall", "#pragma plmn/plmf()");
            active_plmn_pragma = active_plmf_pragma = 0;
        }
    }
    return attributes;
}

/* Original 0x405c0..0x4073b. */
TypeShape *__stdcall new_type_shape(unsigned short kind, unsigned short attributes, void *parameter)
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
    switch (kind & 7) {
    case 0:
        if (parameter) {
            if ((attributes & 12) && attributes != 5) {
                report_syntax_error(493);
                shape->attributes = 0;
                shape->field08 = 0;
            } else
                shape->field08 = *(unsigned *)parameter;
        }
        break;
    case 1:
        if ((attributes & 4) && parameter)
            shape->field08 = *(unsigned *)parameter;
        break;
    case 2:
        if (kind & 0x1000)
            shape->field08 = *(unsigned *)parameter;
        else if (parameter) {
            if (*(int *)parameter < 0) {
                report_syntax_error(118);
                *(int *)parameter = 1;
            }
            shape->field08 = *(unsigned *)parameter;
        } else
            shape->field08 = 0;
        break;
    case 4:
        if (kind & 0x400) {
            shape->flags &= 0xfbff;
            shape->field08 = *(unsigned *)parameter;
        } else {
            adjusted_kind = kind;
            shape->field08 = (unsigned)build_parameter_info(parameter, &adjusted_kind);
            shape->flags |= adjusted_kind;
        }
        break;
    }
    return shape;
}

/* Original 0x3fe40..0x3ff29. */
TypeShape *__stdcall intern_type_shape(TypeShape *shape)
{
    unsigned hash, category, attributes;
    TypeShapeEntry **bucket, *entry, *previous = 0;
    if (!shape)
        return 0;
    category = shape->flags & 7;
    attributes = shape->attributes & 7;
    if ((category == 1 || category == 3) && (attributes < 5 || attributes > 7))
        shape->field08 = 0;
    hash = (shape->flags >> 6) ^ shape->flags;
    hash += (shape->attributes >> 6) + shape->attributes;
    hash += (unsigned)shape->related & 0xffc;
    hash += (shape->field08 & 0xffc) >> 2;
    bucket = &type_shape_buckets[hash & 0x3ff];
    for (entry = *bucket; entry; entry = entry->next) {
        if (&entry->shape == shape ||
            (entry->shape.flags == shape->flags &&
             entry->shape.attributes == shape->attributes &&
             entry->shape.related == shape->related &&
             entry->shape.field08 == shape->field08))
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

/* Original 0x40b60..0x40c0a. */
CType *__stdcall make_tagged_type(unsigned short flags)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    TypeDescription *description;
    if (pool->remaining < (int)sizeof(TypeDescription))
        refill_arena(&compiler_arena, pool);
    description = (TypeDescription *)pool->next;
    pool->remaining -= sizeof(TypeDescription);
    pool->next += sizeof(TypeDescription);
    if (flags & 0x80) {
        description->attributes = 0;
        description->field08 = 0;
        description->flags = flags;
        description->related = intern_type_shape(new_type_shape(1,
            normalize_type_attributes(1, 0), 0));
    } else
        description->flags = flags;
    return intern_type(description);
}

int type_description_depth; /* original 0x5ff18 */

/* Original 0x40360..0x403a2. */
CType *__stdcall make_qualified_type(CType *base, unsigned short kind, unsigned short attributes, void *parameter)
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

/* Original description stack at 0x5ff1c, followed by a separate global
   at 0x5ffbc. Entry zero is reserved by the pre-increment convention. */
static TypeDescription type_description_stack[10];

/* Original 0x40530..0x40560. */
TypeDescription *__stdcall copy_type_description(CType *type)
{
    TypeDescription *description = &type_description_stack[++type_description_depth];
    *description = *type;
    return description;
}
