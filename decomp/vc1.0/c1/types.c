/* Interned frontend types and related hash tables. */
#include "types.h"
#include "lexer.h"
#include "builtins.h"

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
TypeDescription type_description_stack[10];

/* Original 0x40530..0x40560. */
TypeDescription *__stdcall copy_type_description(CType *type)
{
    TypeDescription *description = &type_description_stack[++type_description_depth];
    *description = *type;
    return description;
}

/* Original 0x1a970..0x1ac86. Combine C type-specifier keywords while
   retaining the spelling distinctions needed for duplicate diagnostics. */
CType *__stdcall merge_type_specifier(CType *type, unsigned short *flags, unsigned short added)
{
    unsigned short existing = *flags, sign = existing & 0xc000;
    unsigned short next_sign = added & 0xc000, base;
    int invalid = 0;
    if (sign && next_sign) {
        if (sign & next_sign) compiler_warning(1, 114);
        else {
            added &= 0x3fff;
            report_syntax_error(42);
        }
        *flags |= added;
        return type;
    }
    if (next_sign || (added & 0x4000)) {
        *flags |= added;
        return type;
    }
    if ((existing & 0xefff) == sign) {
        *flags = existing | added;
        return type;
    }
    existing &= 0x2fff;
    base = added & 0x2fff;
    if (existing & 0x400) {
        report_syntax_error(628, last_lookup_symbol->name, type_specifier_name(base));
        last_lookup_symbol = 0;
        *flags = added;
        return 0;
    }
    switch (existing) {
    case 0x42:
        if (base != 0x64) invalid = 1;
        existing = 0x2042;
        break;
    case 0x44:
        switch (base) {
        case 0x64: existing = 0x2044; break;
        case 0x104:
            compiler_warning(1, 215);
            existing = 0x108;
            break;
        case 0x108:
            existing = 0x128;
            compiler_warning(3, 69);
            break;
        default: invalid = 1; break;
        }
        break;
    case 0x64:
        if (base == 0x42) existing = 0x2042;
        else if (base == 0x44) existing = 0x2044;
        else invalid = 1;
        break;
    case 0x80:
        report_syntax_error(120);
        break;
    case 0x104:
        if (base == 0x44) {
            compiler_warning(1, 216);
            existing = 0x108;
        } else invalid = 1;
        break;
    case 0x108:
        if (base == 0x44) existing = 0x128;
        else invalid = 1;
        break;
    default:
        invalid = 1;
        break;
    }
    if (invalid) {
        report_syntax_error(632, type_specifier_name(existing), type_specifier_name(base));
        type = 0;
    }
    *flags = existing | sign;
    return type;
}

/* Original 0x3fca0..0x3fd57. Resolve omitted int and char signedness,
   remove temporary spelling bits, and intern the completed description. */
CType *__stdcall finalize_type_description(TypeDescription *description)
{
    unsigned short flags, base;
    CType *type;
    if (!description) return type_0064;
    flags = description->flags;
    base = flags & 0x2fdf;
    if (base == 0x41) {
        if (options.z_a && options.option_J && !(flags & 0x8000)) flags |= 0x4000;
    } else if (!base) flags |= 0x64;
    flags &= 0xcfff;
    if (flags & 0xc000) {
        if (!(flags & 0x840)) {
            compiler_warning(1, 76, (flags & 0x4000) ? "unsigned" : "signed");
            flags &= 0x3fff;
        } else if (options.z_a || (flags & 0x2fdf) != 0x41)
            flags &= 0x7fff;
    }
    description->flags = flags;
    type = intern_type(description);
    --type_description_depth;
    return type;
}

/* Original 0x40560..0x405b1. Prepend a mutable shape to a declarator. */
void __stdcall attach_type_shape(FrontendSymbol *symbol, TypeShape *shape)
{
    FrontendSymbol *based;
    shape->attributes &= ~0xa00;
    if ((shape->attributes & 7) == 5) {
        based = (FrontendSymbol *)shape->field08;
        if ((based->kind & 0x1f) == 13 && (based->field24 & 0xff) == 8)
            based->field24 = (based->field24 & ~0xffU) | 1;
    }
    shape->related = symbol->type->related;
    symbol->type->related = shape;
}

/* Original 0x3fb90..0x3fc75. Merge qualifiers, pointer models, and calling
   conventions, diagnosing conflicting explicit spellings. */
unsigned short __stdcall merge_type_attributes(unsigned short existing, unsigned short added)
{
    if (existing & added & 0x3000) {
        if (options.z_a) compiler_warning(1, 114);
        else report_syntax_error(289);
    }
    if ((existing ^ added) & 15) {
        if ((existing & 4) != (added & 4)) {
            if (existing & 4) existing |= (unsigned short)(added << 14);
            else {
                existing |= (unsigned short)(existing << 14);
                existing &= ~7;
                existing |= added & 15;
            }
        } else if ((existing & 15) && (added & 15)) {
            report_syntax_error(131);
            existing &= ~7;
        }
    }
    if (((existing ^ added) & 0x70) && (existing & 0x70) && (added & 0x70)) {
        report_syntax_error(151);
        existing &= ~0x70;
    }
    return existing | added;
}

/* Original 0x40000..0x4001d. */
CType *__stdcall remove_type_prefix(CType *type)
{
    if (type && type->related && !(type->related->flags & 7))
        type = drop_first_type_shape(type);
    return type;
}

/* Original 0x401c0..0x40211. A transient copy leaves the interned source
   descriptor untouched when removing its first shape. */
CType *__stdcall drop_first_type_shape(CType *type)
{
    CType *copy;
    ArenaPool *pool = &compiler_arena.pools[2];
    if (pool->remaining < (int)sizeof(CType)) refill_arena(&compiler_arena, pool);
    copy = (CType *)pool->next;
    pool->remaining -= sizeof(CType);
    pool->next += sizeof(CType);
    *copy = *type;
    copy->related = type->related->related;
    return copy;
}

/* Original 0x32d50..0x32dcf. Prefix shapes carry qualifiers, not size. */
int __stdcall type_size(CType *type)
{
    TypeShape *shape = type->related;
    int element, size;
    unsigned count;
    if (shape && !(shape->flags & 7)) shape = shape->related;
    if (!shape) return base_type_size(type->flags, (void *)type->field08);
    if ((shape->flags & 7) != 2) return pointer_type_size(shape->attributes);
    element = array_element_size(type->flags, (void *)type->field08, shape);
    count = shape->field08;
    size = (int)((unsigned)element * count);
    if (element > size && count) {
        report_syntax_error(148);
        shape->field08 = 1;
    }
    return size;
}

/* Original 0x32e30..0x32e76. Primitive flags encode their byte size in
   the low nibble; record tags keep their computed size in the tag payload. */
int __stdcall base_type_size(unsigned short flags, void *information)
{
    FrontendSymbol *tag = information;
    if (flags & 0x400) {
        if ((tag->kind & 0x1f) != 4) tag = (FrontendSymbol *)tag->type->field08;
        return (int)tag->detail.value;
    }
    if (flags & 0x200) return 4;
    return flags & 15;
}

/* Original prefix-shape workspace at 0x66360, referenced by 0x6d9e0. */
static TypeShape replacement_prefix;

/* Original 0x40040..0x40159. Replace the first substantive shape, preserving
   a declaration prefix and propagating qualifiers through an array shape. */
CType *__stdcall replace_outer_type_shape(CType *type, unsigned short kind, unsigned short attributes, void *parameter)
{
    TypeDescription *description = &type_description_stack[++type_description_depth];
    TypeShape *replacement, *shape, *remaining, *next;
    CType *result;
    int prefix = 0;
    *description = *type;
    replacement = new_type_shape(kind, attributes, parameter);
    shape = type->related;
    remaining = 0;
    if (shape) {
        if (!(shape->flags & 7)) {
            shape = shape->related;
            prefix = 1;
        }
        if ((shape->flags & 7) == 2) {
            next = shape->related;
            if (!next) description->attributes |= shape->attributes & 0x3000;
            else {
                remaining = new_type_shape(next->flags,
                    (unsigned short)(next->attributes | (shape->attributes & 0x3000)), &next->field08);
                remaining->related = next->related;
                remaining = intern_type_shape(remaining);
            }
        } else remaining = shape->related;
    }
    replacement->related = remaining;
    replacement = intern_type_shape(replacement);
    if (prefix) {
        replacement_prefix = *type->related;
        replacement_prefix.related = replacement;
        replacement = intern_type_shape(&replacement_prefix);
    }
    description->related = replacement;
    result = intern_type(description);
    --type_description_depth;
    return result;
}
