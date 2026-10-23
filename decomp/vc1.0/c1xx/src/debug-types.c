/* CodeView type references and deferred type records. */
#include <stdlib.h>
#include "debug-types.h"
#include "symbol-output.h"
#include "debug-info.h"
#include "cpp-context.h"
#include "class-finalization.h"
#include <recon.h>

DebugTypeRecord *debug_types; /* original 0x8ccb0 */
unsigned short next_debug_type_index = 0x1000; /* original 0x9f454 */
unsigned debug_type_count; /* original 0xa25c0 */
unsigned debug_type_capacity; /* original 0xa25bc */
unsigned short debug_field_properties; /* original 0xa25c4 */

/* Original 0x70320..0x70370. Short unsigned numeric leaves are their
   own value; larger ones carry an explicit unsigned-short or long tag. */
unsigned __stdcall encode_debug_unsigned(unsigned char *destination, unsigned value)
{
    if (value < 0x8000) {
        *(unsigned short *)destination = (unsigned short)value;
        return 2;
    }
    if (value < 0x10000) {
        *(unsigned short *)destination = 0x8002;
        *(unsigned short *)(destination + 2) = (unsigned short)value;
        return 4;
    }
    *(unsigned short *)destination = 0x8004;
    *(unsigned *)(destination + 2) = value;
    return 6;
}

/* Original 0x6e5e0..0x6e999. Write the class, union or enum leaf and
   enqueue field, enumerator and virtual-shape records as required. */
void __stdcall emit_debug_class(CppSymbol *tag, unsigned short field_count,
    unsigned short field_list)
{
    unsigned char *leaf = debug_record_buffer + 2, *name;
    unsigned short *words = (unsigned short *)leaf;
    unsigned short flags = tag->type->flags;
    unsigned record, length, numeric_length, i;
    HashIterator iterator;
    words[1] = field_count;
    if (flags != 0x402 && flags != 0x404 && flags != 0x408 && (flags & 0x200)) {
        words[0] = 7;
        words[4] = field_list;
        if (tag->flags & 4) {
            words[3] = next_debug_type_index;
            words[2] = primitive_debug_type((flags & ~0x200) | 0x64);
            record = reserve_debug_type(7);
            debug_types[record].attributes = field_count;
            debug_types[record].subject.members = malloc(field_count * sizeof(CppSymbol *));
            if (!debug_types[record].subject.members) fatal_error(60);
            debug_types[record].detail.first_member = 0;
            begin_hash_iteration(&iterator, tag->auxiliary.enumerators);
            if (next_hash_entry(&iterator)) {
                i = 0;
                do {
                    debug_types[record].subject.members[i++] = iterator.value;
                } while (next_hash_entry(&iterator));
            }
            qsort(debug_types[record].subject.members, field_count,
                sizeof(CppSymbol *), compare_debug_symbol_indices);
        } else {
            words[3] = 0;
            words[4] |= 0x80;
            words[2] = 0;
        }
        words[4] = (words[4] & ~8) | ((tag->detail.class_info->flags & 2) << 2);
        name = leaf + 10;
        length = copy_debug_name(name, tag->name) - name + 10;
    } else if (flags == 0x404) {
        words[0] = 6;
        words[3] = field_list;
        if (tag->flags & 4) {
            words[2] = next_debug_type_index;
            record = reserve_debug_type(2);
            debug_types[record].subject.symbol = tag;
            debug_types[record].attributes = 0;
            debug_types[record].related_type = 0;
        } else {
            words[2] = 0;
            words[3] |= 0x80;
        }
        words[3] = (words[3] & ~1) | (top_packing(packing_stack)->alignment == 1);
        words[3] = (words[3] & ~8) | ((tag->detail.class_info->flags & 2) << 2);
        numeric_length = encode_debug_unsigned(leaf + 8, tag->detail.class_info->size);
        name = leaf + 8 + numeric_length;
        length = numeric_length + (copy_debug_name(name, tag->name) - name) + 8;
    } else if (flags == 0x402 || flags == 0x408) {
        words[0] = flags == 0x402 ? 5 : 4;
        words[3] = field_list;
        if (tag->flags & 4) {
            words[2] = next_debug_type_index;
            record = reserve_debug_type(2);
            debug_types[record].subject.symbol = tag;
            debug_types[record].attributes = 0;
            if (tag->detail.class_info->alignment_flags & 0x40) {
                debug_types[record].related_type = next_debug_type_index;
                words[5] = debug_types[record].related_type;
                record = reserve_debug_type(6);
                debug_types[record].subject.symbol = tag;
            } else {
                debug_types[record].related_type = 0;
                words[5] = debug_types[record].related_type;
            }
            words[3] = (words[3] & ~1) | (top_packing(packing_stack)->alignment == 1);
        } else {
            words[2] = 0;
            words[3] |= 0x80;
            words[5] = 0;
        }
        words[4] = 0;
        words[3] = (words[3] & ~8) | ((tag->detail.class_info->flags & 2) << 2);
        numeric_length = encode_debug_unsigned(leaf + 12, tag->detail.class_info->size);
        name = leaf + 12 + numeric_length;
        length = numeric_length + (copy_debug_name(name, tag->name) - name) + 12;
    } else {
#ifdef RECON_NATIVE_TEST
        /* This descriptor domain leaves original length at entryESP-24
           unwritten. Public reachability and stale-stack behavior are unknown. */
        __asm { int 3 }
#endif
    }
    emit_debug_type_record(length);
}

/* Original 0x6dab0..0x6db1d. Intern a parameter-list debug record once,
   retaining the low-byte parameter count used by the original emitter. */
unsigned char __stdcall reference_parameter_debug_types(FunctionParameters *parameters)
{
    DeclarationParameterInfo *declarations = parameters->declarations;
    unsigned char count = 0;
    unsigned i, record;
    if (declarations->types[0] != type_0080)
        count = (unsigned char)declarations->count;
    if (!declarations->debug_index) {
        for (i = 0; i < count; ++i)
            if (declarations->types[i]) reference_debug_type(declarations->types[i]);
        declarations->debug_index = next_debug_type_index;
        record = reserve_debug_type(3);
        debug_types[record].subject.parameters = parameters;
    }
    return count;
}

/* Original 0x6dd80..0x6df7f. Register emitted method signatures and
   accumulate the class properties contributed by their overload set. */
unsigned __stdcall count_debug_methods(OverloadSet *overloads,
    unsigned short *field_properties)
{
    CppSymbol *function, *next, *owner;
    CType *type, description, *base;
    TypeShape *shape;
    FunctionParameters *parameters;
    unsigned count = 0, record;
    unsigned char argument_count;
#ifdef RECON_NATIVE_TEST
    /* NULL skips the original cursor's initialization, then reads that
       unwritten entryESP-20 slot. Its public reachability is unknown. */
    if (!overloads) { __asm { int 3 } }
#endif
    if (overloads) {
        function = overloads->functions;
        owner = cpp_context_stack->items[function->auxiliary.declaring_class];
        if ((overloads->name == name_ctor &&
             (owner->detail.class_info->trivial_special_members & 3) == 3) ||
            (overloads->name == name_assignment &&
             (owner->detail.class_info->trivial_special_members & 4)) ||
            (overloads->name == name_dtor &&
             (owner->detail.class_info->trivial_special_members & 8))) next = 0;
        else next = function;
    }
    /* The Win32 NULL-overloads branch deliberately retains its original
       uninitialized cursor; the native guard above stops before this read. */
    while (next) {
        function = next;
        next = function->next;
        if (function->name == name_ctor) {
            if (function->cpp.parts.member_flags & 0x20) {
                owner = cpp_context_stack->items[function->auxiliary.declaring_class];
                if (owner->detail.class_info->trivial_special_members & 2) continue;
            }
            if (function->cpp.parts.member_flags & 0x10) {
                owner = cpp_context_stack->items[function->auxiliary.declaring_class];
                if (owner->detail.class_info->trivial_special_members & 1) continue;
            }
        }
        type = intern_type((TypeDescription *)strip_based_shape(function->type));
        if (overloads->functions->cpp.parts.token != 1) *field_properties |= 4;
        if (overloads->name == name_ctor || overloads->name == name_dtor)
            *field_properties |= 2;
        if (!type->debug_index) {
            shape = type->related;
            description = *type;
            description.related = shape->related;
            base = intern_type((TypeDescription *)&description);
            reference_debug_type(base);
            parameters = shape->data.parameters;
            if (function->cpp.parts.member_flags & 1)
                reference_debug_type(parameters->associated_type);
            argument_count = reference_parameter_debug_types(parameters);
            type->debug_index = next_debug_type_index;
            record = reserve_debug_type(0);
            debug_types[record].subject.type = type;
            debug_types[record].attributes = argument_count;
            owner = cpp_context_stack->items[function->auxiliary.declaring_class];
            debug_types[record].related_type = owner->type->debug_index;
            debug_types[record].detail.displacement = function->this_displacement;
        }
        ++count;
    }
    return count;
}

/* Original 0x6df80..0x6e019. Allocate queue space without initializing
   fields owned by each producer; only the kind byte is common to all slots. */
unsigned __stdcall reserve_debug_type(unsigned char kind)
{
    unsigned index = debug_type_count++;
    ++next_debug_type_index;
    if (!debug_type_capacity) {
        debug_type_capacity = 128;
        debug_types = malloc(128 * sizeof(DebugTypeRecord));
        if (!debug_types) fatal_error(60);
    } else if (debug_type_count > debug_type_capacity) {
        debug_type_capacity += (debug_type_capacity >> 2) + 1;
        debug_types = realloc(debug_types, debug_type_capacity * sizeof(DebugTypeRecord));
        if (!debug_types) fatal_error(60);
    }
    debug_types[index].kind = kind;
    return index;
}

/* Original 0x6db20..0x6dd7e. Assign the tag index before descending into
   bases/members, then count the fields its deferred class record will emit. */
unsigned short __stdcall debug_tag_type(CppSymbol *tag)
{
    CppSymbol *member, *base;
    BaseClassLink *base_link;
    unsigned record, kind;
    unsigned short flags;
    int count = 0;
    if (options.configuration.line_records) {
        prepare_line_record_tag(tag);
        return line_record_tag_type(tag);
    }
    if (!tag) return 0;
    flags = tag->type->flags;
    if (tag->type->debug_index) return tag->type->debug_index;
    tag->type->debug_index = next_debug_type_index;
    record = reserve_debug_type(1);
    debug_types[record].subject.symbol = tag;
    debug_types[record].detail.field_list = debug_field_properties;
    if (tag->flags & 4) {
        if (flags & 0x200)
            count = hash_table_count(tag->auxiliary.enumerators);
        else {
            for (member = tag->auxiliary.scope_members; member; member = member->next) {
                if ((member->kind & 31) != 15 || (member->flags & 0x80)) continue;
                base_link = (BaseClassLink *)member;
                base = cpp_context_stack->items[base_link->context_index];
                base->type->debug_index = debug_tag_type(base);
                if (!(base_link->state & 1)) ++count;
            }
            count += class_virtual_table_group_count(tag);
            for (member = tag->auxiliary.scope_members; member; member = member->next) {
                kind = member->kind & 31;
                if (kind == 15 || kind == 5) continue;
                /* Anonymous enum declarations retain a debug field even
                   though the ordinary unnamed-declaration filter rejects it. */
                if (!(kind == 4 && member->name == name_unnamed &&
                      (member->type->flags & 0x200))) {
                    if (!member->name || member->name == name_unnamed ||
                        member->name == name_alignment) continue;
                    if (kind >= 1 && kind <= 13 && kind != 5 && !member->type) continue;
                }
                if (kind == 17) {
                    if (!((OverloadSet *)member)->functions) continue;
                    if (member->name == name_vbase_dtor || member->name == name_delete_dtor ||
                        member->name == name_vector_delete_dtor ||
                        member->name == name_default_ctor_closure || tag->name == name_unnamed)
                        continue;
                    count += count_debug_methods((OverloadSet *)member,
                        &debug_types[record].detail.field_list);
                } else if (kind == 4) {
                    member->type->debug_index = debug_tag_type(member);
                    ++count;
                } else if (member->name == name_vbptr) {
                    reference_debug_type(member->type);
                } else {
                    if (member->name != name_vfptr && kind != 2)
                        reference_debug_type(member->type);
                    ++count;
                }
            }
        }
    }
    debug_types[record].attributes = (unsigned short)count;
    tag->flags &= ~8;
    return tag->type->debug_index;
}

/* Original 0x6e080..0x6e267, switch data 0x6e268..0x6e288. Emission
   may append more records, so reread both the count and queue each turn. */
void flush_debug_types(void)
{
    unsigned index;
    DebugTypeRecord *record;
    CType *type, description, *base;
    CppSymbol *symbol;
    if (options.configuration.line_records || !debug_type_count) return;
    for (index = 0; index < debug_type_count; ++index) {
        record = &debug_types[index];
        switch (record->kind) {
        case 0:
            type = record->subject.type;
            if (!type->related) {
                emit_debug_qualified_type(type);
                break;
            }
            switch (type->related->flags & 0x77) {
            case 1: case 3: case 0x10: case 0x20: case 0x40:
                emit_debug_pointer_type(type, record->related_type);
                break;
            case 2:
                emit_debug_array_type(type, record->related_type);
                break;
            case 4:
                if (record->related_type) {
                    description = *type;
                    description.related = type->related->related;
                    base = intern_type((TypeDescription *)&description);
                    record = &debug_types[index];
                    emit_debug_member_function_type(type, base->debug_index,
                        record->attributes, record->related_type, record->detail.displacement);
                } else {
                    description = *type;
                    description.related = type->related->related;
                    base = intern_type((TypeDescription *)&description);
                    record = &debug_types[index];
                    emit_debug_function_type(type, base->debug_index, record->attributes);
                }
                break;
            }
            break;
        case 1:
            symbol = record->subject.symbol;
            switch (symbol->kind & 31) {
            case 2:
                emit_debug_bit_field(symbol);
                break;
            case 4:
                emit_debug_class(symbol, record->attributes,
                    record->detail.field_list);
                break;
            case 17:
                emit_debug_method_list((struct OverloadSet *)symbol);
                break;
            }
            break;
        case 2:
            emit_debug_field_list(record->subject.symbol, (unsigned)record->attributes,
                record->related_type);
            break;
        case 3:
            emit_debug_argument_list(record->subject.parameters);
            break;
        case 6:
            emit_debug_vshape(record->subject.symbol);
            break;
        case 7:
            emit_debug_field_list_chunk(record->subject.members,
                record->detail.first_member, (int)record->attributes);
            break;
        }
    }
    reset_debug_type_records();
}

/* CodeView predefined types indexed by the frontend's byte-width field.
   Keep the encountered logical rows, without neighbouring alignment bytes
   or the ordinary string following the final integral table. */
unsigned short cv_real_types[11] = {
    0, 0, 0, 0, 0x40, 0, 0, 0, 0x41, 0, 0x42
}; /* original 0xa25c8 */
unsigned short cv_unsigned_int_types[9] = {
    0, 0x70, 0x73, 0, 0x75, 0, 0, 0, 0x77
}; /* original 0xa25e0 */
unsigned short cv_signed_int_types[9] = {
    0, 0x70, 0x72, 0, 0x74, 0, 0, 0, 0x76
}; /* original 0xa25f8 */
unsigned short cv_unsigned_size_types[9] = {
    0, 0x20, 0x21, 0, 0x22, 0, 0, 0, 0x23
}; /* original 0xa2610 */
unsigned short cv_signed_size_types[9] = {
    0, 0x10, 0x11, 0, 0x12, 0, 0, 0, 0x13
}; /* original 0xa2628 */

/* Original 0x6d920..0x6d9b7. Integer spellings distinguish the int
   family from width-specific scalar types, including plain char. */
unsigned short __stdcall primitive_debug_type(unsigned short flags)
{
    unsigned width;
    if (flags & 0x80) return 3;
    if (!(flags & 0x140)) return 0;
    width = flags & 15;
#ifdef RECON_NATIVE_TEST
    /* Internal widths outside the recovered logical tables have no native
       support. This does not classify their public reachability or claim
       an original bug/equivalence for neighbouring out-of-domain reads. */
    if (width >= ((flags & 0x100) ? 11 : 9)) { __asm { int 3 } }
#endif
    if (flags & 0x100) return cv_real_types[width];
    if ((flags & 0x60) == 0x60 || (width == 1 && !(flags & 0xc000))) {
        if (flags & 0x4000) return cv_unsigned_int_types[width];
        return cv_signed_int_types[width];
    }
    if (flags & 0x4000) return cv_unsigned_size_types[width];
    return cv_signed_size_types[width];
}

/* Original 0x6d860..0x6d913. An ordinary unqualified pointer to a
   predefined type can encode its memory model in the predefined index. */
unsigned short __stdcall primitive_pointer_debug_type(CType *type)
{
    TypeShape *shape = type->related;
    unsigned short index, model;
    if (shape->related || (shape->flags & 0x77) != 1 ||
        (shape->attributes & 0x300c)) return 0;
    index = primitive_debug_type(type->flags);
    if (!index) return 0;
    switch (shape->attributes & 15) {
    case 1:
        return (index & 0xfcff) | 0x400;
    case 2:
        return (index & 0xfdff) | 0x500;
    case 3:
        return (index & 0xfbff) | 0x300;
    }
    model = (options.configuration.option_M & 2) ? 5 : 4;
    return (index & 0xf8ff) | (model << 8);
}

/* Original 0x6d9c0..0x6da4d. Tags retain their declaration identity;
   unshaped builtin types can use CodeView's predefined type indices. */
unsigned short __stdcall base_debug_type(CType *type)
{
    unsigned short flags = type->flags;
    if (flags & 0x200)
        return debug_tag_type(cpp_context_stack->items[type->field08]);
    if (flags & 0x400)
        return debug_tag_type(cpp_context_stack->items[type->field08]);
    if (flags & 0x80) return 3;
    if (flags & 0x800) return 2;
    if (!flags) return 0;
    if (flags == 0x802) return 2;
    return primitive_debug_type(flags);
}

/* Original 0x6d6e0..0x6d856. Reserve a type's index before queuing its
   record, recursively referring to the type beneath its first shape. */
void __stdcall reference_debug_type(CType *type)
{
    TypeShape *shape;
    CType description, *base;
    unsigned record;
    if (options.configuration.line_records) {
        reference_line_record_type(type);
        return;
    }
    /* The original reads related before its late null test. */
    shape = type->related;
    if (options.prototypes_only) return;
    if (!type) {
        type->debug_index = 0;
        return;
    }
    if (type->debug_index) return;
    if (shape) {
        type->debug_index = primitive_pointer_debug_type(type);
        if (type->debug_index) return;
        description = *type;
        description.related = shape->related;
        base = intern_type((TypeDescription *)&description);
        reference_debug_type(base);
        switch (shape->flags & 0x77) {
        case 0:
            type->debug_index = base->debug_index;
            return;
        case 1: case 2: case 3: case 0x10: case 0x20: case 0x40:
            type->debug_index = next_debug_type_index;
            record = reserve_debug_type(0);
            debug_types[record].subject.type = type;
            debug_types[record].related_type = base->debug_index;
            return;
        case 4:
            reference_function_debug_type(type);
            return;
        }
        return;
    }
    if (type->attributes & 0x3000) {
        type->debug_index = next_debug_type_index;
        record = reserve_debug_type(0);
        debug_types[record].subject.type = type;
        return;
    }
    type->debug_index = base_debug_type(type);
}
