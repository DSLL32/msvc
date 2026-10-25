/* CodeView type references and deferred type records. */
#include <stdlib.h>
#include "debug-types.h"
#include "symbol-output.h"
#include "debug-info.h"
#include "c1xx.h"
#include "cpp-context.h"
#include "class-finalization.h"
#include "call-expressions.h"
#include "global-ir.h"
#include <recon.h>

DebugTypeRecord *debug_types; /* original 0x8ccb0 */
unsigned short next_debug_type_index = 0x1000; /* original 0x9f454 */
unsigned debug_type_count; /* original 0xa25c0 */
unsigned debug_type_capacity; /* original 0xa25bc */
unsigned short debug_field_properties; /* original 0xa25c4 */
DebugTypeFixup *debug_type_fixups; /* original 0xa25b0 */

/* Original 0x6e020..0x6e077. Keep a small reusable queue after
   flushing, shrinking larger allocations to the initial capacity. */
void reset_debug_type_records(void)
{
    if (debug_type_capacity > 128) {
        debug_type_capacity = 128;
        debug_types = realloc(debug_types, 128 * sizeof(DebugTypeRecord));
        if (!debug_types) fatal_error(60);
    }
    debug_type_count = 0;
}

/* Original 0x6fab0..0x6fc6d. Omit trivial implicit special members
   and retain per-overload access, virtual, pure and static attributes. */
void __stdcall emit_debug_method_list(OverloadSet *overloads)
{
    unsigned char *leaf = debug_record_buffer + 2, *field;
    CppSymbol *function, *next, *owner;
    unsigned length = 0, pad, left;
    unsigned short properties;
    *(unsigned short *)leaf = 0x207;
#ifdef RECON_NATIVE_TEST
    /* NULL skips every writer of the original entryESP-8 cursor. */
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
        pad = (-length) & 3;
        field = leaf + length + 2;
        for (left = pad; left; --left) *field++ = (unsigned char)(0xf0 + left);
        length += pad;
        properties = (debug_field_properties & ~3) | (function->flags & 3);
        *(unsigned short *)(field + 2) = function->type->debug_index;
        length += 4;
        if (function->storage[1] & 2) {
            if (function->cpp.parts.member_flags & 2)
                properties = (properties & ~0x18) | 4;
            else {
                properties = (properties & ~0xc) | 0x10;
                *(unsigned *)(field + 4) = function->detail.virtual_table_offset;
                length += 4;
            }
        } else if ((function->storage[0] & 7) == 3)
            properties = (properties & ~0x14) | 8;
        else properties &= ~0x1c;
        *(unsigned short *)field = properties;
    }
    emit_debug_type_record(length + 2);
}

/* Original 0x6fc70..0x6fcf5. Two four-bit near32 method descriptors
   share each byte; an odd last entry keeps a zero low nibble. */
void __stdcall emit_debug_vshape(CppSymbol *tag)
{
    unsigned char *leaf = debug_record_buffer + 2;
    int count = 0;
    VirtualEntryIterator iterator;
    *(unsigned short *)leaf = 10;
    begin_virtual_entries(&iterator,
        first_vector_item(tag->detail.class_info->virtual_tables));
    while (next_virtual_entry(&iterator)) {
        if (count & 1) leaf[4 + count / 2] |= 5;
        else leaf[4 + count / 2] = 0x50;
        ++count;
    }
    *(unsigned short *)(leaf + 2) = (unsigned short)count;
    emit_debug_type_record((count + 1) / 2 + 4);
}

/* Original 0x6fd00..0x6fd86. Pair near/far forms of the compiler's
   calling conventions, retaining the dedicated syscall code. */
unsigned debug_calling_convention(unsigned short attributes)
{
    unsigned far_pointer = (attributes & 7) != 1;
    switch (attributes & 0x70) {
    case 0: case 0x10: return far_pointer;
    case 0x20: case 0x30: return 2 + far_pointer;
    case 0x40: return 4 + far_pointer;
    case 0x50: case 0x60: return 7 + far_pointer;
    case 0x70: return 11;
    }
    return 15;
}

/* Original 0x6f8f0..0x6f94b. Enum bit fields use the underlying
   integral primitive encoding while retaining their original width and offset. */
void __stdcall emit_debug_bit_field(CppSymbol *field)
{
    unsigned char *leaf = debug_record_buffer + 2;
    unsigned short flags = field->type->flags;
    *(unsigned short *)leaf = 0x206;
    leaf[2] = field->auxiliary.bit_field.width;
    leaf[3] = field->auxiliary.bit_field.offset;
    if (flags & 0x200) flags = (flags & ~0x200) | 0x64;
    *(unsigned short *)(leaf + 4) = primitive_debug_type(flags);
    emit_debug_type_record(6);
}

/* Original 0x6e590..0x6e5d3. Array leaves carry the element type,
   the fixed index type and a numeric byte size followed by an empty name. */
void __stdcall emit_debug_array_type(CType *type, unsigned short base_index)
{
    unsigned char *leaf = debug_record_buffer + 2;
    unsigned length;
    *(unsigned short *)leaf = 3;
    *(unsigned short *)(leaf + 2) = base_index;
    *(unsigned short *)(leaf + 4) = 0x11;
    length = encode_debug_unsigned(leaf + 6, type_size(type));
    leaf[length + 6] = 0;
    emit_debug_type_record(length + 7);
}

/* Original 0x6fd90..0x6fde9, stdcall12. A nonmember function leaf
   contains the return type, calling convention and argument-list reference. */
void __stdcall emit_debug_function_type(CType *type,
    unsigned short return_index, unsigned short argument_count)
{
    unsigned char *leaf = debug_record_buffer + 2;
    unsigned short *words = (unsigned short *)leaf;
    words[0] = 8;
    words[1] = return_index;
    leaf[4] = (unsigned char)debug_calling_convention(type->related->attributes);
    leaf[5] = 0;
    words[3] = argument_count;
    words[4] = type->related->data.parameters->declarations->debug_index;
    emit_debug_type_record(10);
}

/* Original 0x6fdf0..0x6fe78. Member-function leaves carry the owner,
   optional this type, argument-list index and signed this adjustment. */
void __stdcall emit_debug_member_function_type(CType *type,
    unsigned short return_index, unsigned short argument_count,
    unsigned short class_index, int this_displacement)
{
    unsigned char *leaf = debug_record_buffer + 2;
    unsigned short *words = (unsigned short *)leaf;
    FunctionParameters *parameters = type->related->data.parameters;
    words[0] = 9;
    words[1] = return_index;
    words[2] = class_index;
    words[3] = (parameters->flags & 1) ? 0 : parameters->associated_type->debug_index;
    leaf[8] = (unsigned char)debug_calling_convention(type->related->attributes);
    leaf[9] = 0;
    words[5] = argument_count;
    words[6] = parameters->declarations->debug_index;
    *(int *)(leaf + 14) = this_displacement;
    emit_debug_type_record(18);
}

/* Original 0x6fe80..0x6ff00. Emit the low-byte argument count and
   the registered type index of each parameter, preserving null varargs. */
void __stdcall emit_debug_argument_list(FunctionParameters *parameters)
{
    DeclarationParameterInfo *declarations = parameters->declarations;
    unsigned short *words = (unsigned short *)(debug_record_buffer + 2);
    unsigned count = 0, i;
    words[0] = 0x201;
    if (declarations->types[0] != type_0080)
        count = (unsigned char)declarations->count;
    words[1] = (unsigned short)count;
    if (!count) {
        emit_debug_type_record(4);
        return;
    }
    for (i = 0; i < count; ++i)
        words[i + 2] = declarations->types[i] ? declarations->types[i]->debug_index : 0;
    emit_debug_type_record(count * 2 + 4);
}

/* Original 0x6ff00..0x6ffcf. A modifier leaf names the corresponding
   unqualified type and retains only the const/volatile qualifier bits. */
void __stdcall emit_debug_qualified_type(CType *type)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    CType *description, *base;
    unsigned short *words = (unsigned short *)(debug_record_buffer + 2);
    if (pool->remaining < 12) refill_arena(&compiler_arena, pool);
    description = (CType *)pool->next;
    pool->remaining -= 12;
    pool->next += 12;
    *description = *type;
    description->attributes &= 0xcfff;
    description->debug_index = 0;
    base = intern_type((TypeDescription *)description);
    if (!base->debug_index) reference_debug_type(base);
    words[0] = 1;
    words[1] = (debug_field_properties & ~3) |
        ((type->attributes & 0x1000) ? 1 : 0) |
        ((type->attributes & 0x2000) ? 2 : 0);
    words[2] = base->debug_index;
    emit_debug_type_record(6);
}

/* Explicit pointer distances: default, near32, far32 and far16. Only
   indices0..3 reach this table: based attributes4/8 take another branch. */
unsigned char cv_pointer_models[4] = { 0, 10, 11, 2 }; /* original0xa25b8 */

/* Original 0x6e340..0x6e582. Pointer leaves include references,
   member pointers, qualifiers and the segmented based-pointer variants. */
void __stdcall emit_debug_pointer_type(CType *type, unsigned short base_index)
{
    TypeShape *shape = type->related;
    unsigned short flags = shape->flags, attributes = shape->attributes;
    unsigned char *leaf = debug_record_buffer + 2;
    CppSymbol *symbol;
    unsigned extra = 0, model, category;
    *(unsigned short *)leaf = 2;
    *(unsigned short *)(leaf + 2) = 0;
    *(unsigned short *)(leaf + 4) = base_index;
    if (flags & 0x40) leaf[2] = (leaf[2] & 0x1f) | 0x20;
    else if (flags & 0x30) {
        leaf[2] = (leaf[2] & 0x1f) | ((flags & 0x10) ? 0x40 : 0x60);
        *(unsigned short *)(leaf + 6) = debug_tag_type(shape->data.tag_symbol);
        *(unsigned short *)(leaf + 8) = 0;
        extra = 4;
    } else leaf[2] &= 0x1f;
    if (attributes & 0x1000) leaf[3] |= 4;
    if (attributes & 0x2000) leaf[3] |= 2;
    if (attributes & 12) {
        if (attributes & 4) {
            symbol = shape->data.based_symbol;
            switch (attributes & 7) {
            case 4:
                leaf[2] = (leaf[2] & 0xe8) | 8;
                *(unsigned short *)(leaf + 6) = 3;
                leaf[8] = 0;
                extra = 3;
                break;
            case 5:
                if ((symbol->kind & 31) == 13) {
                    if (symbol->index == 6) {
                        leaf[2] = (leaf[2] & 0xe5) | 5;
                        *(unsigned short *)(leaf + 6) = 7;
                        *(unsigned short *)(leaf + 8) = 2;
                        *(unsigned short *)(leaf + 10) = 0;
                        *(unsigned short *)(leaf + 12) = 0x1b;
                        leaf[14] = 0;
                        extra = 9;
                    } else {
                        leaf[2] = (leaf[2] & 0xe3) | 3;
                        extra = 4;
                        queue_debug_type_fixup(symbol, leaf + 6, extra);
                    }
                } else {
                    leaf[2] = (leaf[2] & 0xe4) | 4;
                    extra = encode_debug_based_symbol(leaf + 6, symbol);
                }
                break;
            case 6:
                leaf[2] = (leaf[2] & 0xe7) | 7;
                extra = encode_debug_based_symbol(leaf + 6, symbol);
                break;
            case 7:
                leaf[2] = (leaf[2] & 0xe5) | 5;
                extra = encode_debug_based_symbol(leaf + 6, symbol);
                break;
            }
        } else if (attributes & 8) leaf[2] = (leaf[2] & 0xe9) | 9;
    } else {
        model = cv_pointer_models[attributes & 15];
        leaf[2] = (leaf[2] & ~31) | model;
        if (!(attributes & 15)) {
            category = flags & 0x77;
            switch (category) {
            case 1: case 2: case 0x10: case 0x40:
                model = (options.configuration.option_M & 2) ? 11 : 10;
                leaf[2] = (leaf[2] & ~31) | model;
                break;
            case 3: case 4: case 0x20:
                model = (options.configuration.option_M & 1) ? 11 : 10;
                leaf[2] = (leaf[2] & ~31) | model;
                break;
            }
        }
    }
    emit_debug_type_record(extra + 6);
}

/* Original 0x6ffd0..0x700e3. Align the leaf and serialize it, splitting
   literal runs around any symbol fixups queued by the leaf writer. */
void __stdcall emit_debug_type_record(int length)
{
    unsigned char *cursor = debug_record_buffer;
    unsigned padding, i;
    int run;
    if (length > 4096) fatal_error(1, fatal_error_label, 0xa42);
    padding = (-(length + 2)) & 3;
    for (i = 0; i < padding; ++i)
        debug_record_buffer[length + 2 + i] = (unsigned char)(0xf0 + padding - i);
    length += padding;
    *(unsigned short *)debug_record_buffer = (unsigned short)length;
    if (pch_separate_debug)
        pragma_record_number = update_checksum(debug_record_buffer, 3, pragma_record_number);
    if (debug_type_fixups) {
        do {
            run = debug_type_fixups->offset - (cursor - debug_record_buffer);
            if (run) write_debug_record(3, run, cursor);
            write_debug_record(2, debug_type_fixups->symbol, debug_type_fixups->bytes);
            cursor += run + debug_type_fixups->bytes;
            debug_type_fixups = debug_type_fixups->next;
        } while (debug_type_fixups);
        run = debug_record_buffer - cursor + length + 2;
        if (run) write_debug_record(3, run, cursor);
    } else write_debug_record(11, cursor);
    debug_stream_offset += length + 2;
}

/* Original 0x74500..0x74543. CodeView names carry a byte length and
   at most255 bytes, without a trailing NUL. Return the next output byte. */
unsigned char *__stdcall copy_debug_name(unsigned char *destination, char *name)
{
    unsigned length, i;
    for (length = 0; name[length]; ++length) ;
    if (length > 255) length = 255;
    destination[0] = (unsigned char)length;
    for (i = 0; i < length; ++i) destination[i + 1] = name[i];
    return destination + length + 1;
}

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
    FunctionParameters *parameters;
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
            symbol = record->subject.symbol;
            emit_debug_field_list(symbol, (unsigned)record->attributes,
                record->related_type);
            break;
        case 3:
            parameters = record->subject.parameters;
            emit_debug_argument_list(parameters);
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

/* Original 0x6da50..0x6daa6, stdcall4. Parameter records are reserved
   before the function type so its index follows its argument-list indices. */
void __stdcall reference_function_debug_type(CType *type)
{
    unsigned char count = reference_parameter_debug_types(
        type->related->data.parameters);
    unsigned record;
    type->debug_index = next_debug_type_index;
    record = reserve_debug_type(0);
    debug_types[record].subject.type = type;
    debug_types[record].attributes = count;
    debug_types[record].related_type = 0;
}

/* Original0x706f0..0x70703. qsort receives addresses of symbol pointers;
   retain the original target-word subtraction rather than a tri-state order. */
int compare_debug_symbol_indices(const void *left, const void *right)
{
    const CppSymbol *first = *(CppSymbol * const *)left;
    const CppSymbol *second = *(CppSymbol * const *)right;
    return (int)(first->index - second->index);
}
