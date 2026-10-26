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
#include "class-lookup.h"
#include "member-expressions.h"
#include "member-access.h"
#include "enumerators.h"
#include <recon.h>

DebugTypeRecord *debug_types; /* original 0x8ccb0 */
unsigned short next_debug_type_index = 0x1000; /* original 0x9f454 */
unsigned debug_type_count; /* original 0xa25c0 */
unsigned debug_type_capacity; /* original 0xa25bc */
unsigned short debug_field_properties; /* original 0xa25c4 */
DebugTypeFixup *debug_type_fixups; /* original 0xa25b0 */
static DebugTypeFixup *last_debug_type_fixup; /* original0xa25b4 */

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

/* Original0x6e300..0x6e338. End the separate PCH type stream with its
   checksum leaf; line-record mode does not write the CodeView terminator. */
void finish_pch_debug_types(void)
{
    unsigned char *leaf;
    pch_separate_debug = 0;
    if (!options.configuration.line_records) {
        leaf = debug_record_buffer + 2;
        *(unsigned short *)leaf = 0x14;
        *(unsigned *)(leaf + 2) = pragma_record_number;
        emit_debug_type_record(6);
        ++next_debug_type_index;
    }
}

/* Original0x6e290..0x6e300, stdcall4. Emit a precompiled-type reference
   using the restored type count/checksum and the saved object filename. */
void __stdcall restore_pch_debug_state(char *filename)
{
    unsigned char *leaf, *name, *end;
    if (options.configuration.line_records) {
        begin_line_record_debug_info();
        return;
    }
    debug_stream_offset = 0;
    begin_debug_info();
    leaf = debug_record_buffer + 2;
    *(unsigned short *)leaf = 0x13;
    *(unsigned short *)(leaf + 2) = 0x1000;
    --next_debug_type_index;
    *(unsigned short *)(leaf + 4) = (unsigned short)(next_debug_type_index - 0x1000);
    *(unsigned *)(leaf + 6) = pragma_record_number;
    name = leaf + 10;
    end = copy_debug_name(name, filename);
    emit_debug_type_record((end - name) + 10);
}

/* Original0x70370..0x70420, stdcall8. Embedded based-pointer symbols use
   a stack-relative record for automatic storage and a data record otherwise. */
unsigned __stdcall encode_debug_based_symbol(unsigned char *destination, CppSymbol *symbol)
{
    unsigned storage;
    unsigned short length;
    unsigned char *name, *end;
    reference_debug_type(symbol->type);
    storage = symbol->storage[0] & 7;
    if (storage == 4 || storage == 5) {
        *(unsigned short *)(destination + 2) = 0x200;
        *(unsigned *)(destination + 4) = 0;
        *(unsigned short *)(destination + 8) = symbol->type->debug_index;
        name = destination + 10;
        end = copy_debug_name(name, symbol->name);
        length = (unsigned short)(end - name + 8);
    } else {
        *(unsigned short *)(destination + 2) = storage == 3 ? 0x201 : 0x202;
        *(unsigned *)(destination + 4) = 0;
        *(unsigned short *)(destination + 8) = 0;
        *(unsigned short *)(destination + 10) = symbol->type->debug_index;
        name = destination + 12;
        end = copy_debug_name(name, symbol->name);
        length = (unsigned short)(end - name + 10);
    }
    *(unsigned short *)destination = length;
    return (unsigned)length + 2;
}

/* Original0x70a50..0x70a6e, cdecl1/RET0. Prototype-only compilation
   suppresses this separate type database in the same way as ordinary debug. */
unsigned short __cdecl reference_line_record_type(CType *type)
{
    if (options.prototypes_only) return 0;
    return ensure_line_record_type(type);
}
/* Original0x70a70..0x70ab0, cdecl1/RET0. Cache the actual separate
   database index in the same descriptor field used by ordinary CodeView. */
unsigned short ensure_line_record_type(CType *type)
{
    if (!type->debug_index) {
        if (type->related) return bind_line_record_shape(type, type->related);
        if (type->attributes & 0x3000) return qualify_line_record_type(type);
        type->debug_index = line_record_base_type(type);
    }
    return type->debug_index;
}
/* Original0x70ab0..0x70b5a, cdecl2/RET0. The passed first shape
   remains distinct from the interned descriptor made after removing it. */
unsigned short bind_line_record_shape(CType *type, TypeShape *shape)
{
    unsigned short base = line_record_primitive_pointer(type, shape);
    if (base) {
        type->debug_index = base;
        return base;
    }
    base = ensure_line_record_type(line_record_drop_shape(type));
    switch (shape->flags & 0x77) {
    case 0:
        type->debug_index = base;
        break;
    case 1: case 3: case 0x10: case 0x20: case 0x40:
        type->debug_index = line_record_pointer_type(type, base);
        break;
    case 2:
        type->debug_index = line_record_array_type(type, base);
        break;
    case 4:
        type->debug_index = line_record_function_type(type, base);
        break;
    default:
        return 0;
    }
    return type->debug_index;
}
/* Original0x71130..0x711b2, cdecl1/RET0. Reference the unqualified
   descriptor first, then record this const/volatile combination. */
unsigned short qualify_line_record_type(CType *type)
{
    unsigned short *words = (unsigned short *)line_record_leaf;
    unsigned short base = ensure_line_record_type(line_record_unqualified_type(type));
    words[0] = 1;
    words[1] = (line_record_properties & ~3) |
        ((type->attributes & 0x1000) ? 1 : 0) |
        ((type->attributes & 0x2000) ? 2 : 0);
    words[2] = base;
    type->debug_index = intern_line_record(6);
    return type->debug_index;
}
/* Original0x72880..0x728b5, cdecl1/RET0. */
CType *line_record_unqualified_type(CType *type)
{
    CType description = *type;
    description.attributes &= 0xcfff;
    description.debug_index = 0;
    return intern_type((TypeDescription *)&description);
}
/* Original0x711c0..0x7124b, cdecl1/RET0. Tags use actual context
   entries; builtin leaves use the separate module's predefined conversion. */
unsigned short line_record_base_type(CType *type)
{
    unsigned short flags = type->flags;
    if (flags & 0x200) {
        type->debug_index = line_record_enum_type(cpp_context_stack->items[type->field08]);
        return type->debug_index;
    }
    if (flags & 0x400) {
        type->debug_index = line_record_tag_type(cpp_context_stack->items[type->field08]);
        return type->debug_index;
    }
    if (flags & 0x80) return 3;
    if (flags & 0x800) return 2;
    if (!flags) return 0;
    if (flags == 0x802) return 2;
    return line_record_primitive_type(flags);
}
unsigned short prepare_line_record_enum(CppSymbol *tag);

/* Original 0x71250..0x7130f, cdecl4/defined AX. Named enums get
   a forward leaf before their enumerator field list is completed. */
unsigned short line_record_enum_type(CppSymbol *tag)
{
    unsigned short *words = (unsigned short *)line_record_leaf;
    if (tag->type->debug_index) return tag->type->debug_index;
    if (tag->name == name_unnamed) {
        tag->type->debug_index = prepare_line_record_enum(tag);
        return tag->type->debug_index;
    }
    tag->flags &= ~8;
    words[0] = 7;
    words[1] = words[2] = words[3] = 0;
    words[4] = line_record_properties | 0x80;
    words[4] = (words[4] & ~8) | ((tag->detail.class_info->flags & 2) << 2);
    tag->type->debug_index = intern_line_record((unsigned short)
        (copy_line_record_name(line_record_leaf + 10, tag->name) + 10));
    return tag->type->debug_index;
}
/* Original0x70c00..0x70c85, cdecl1/RET0. This separate module has
   identical encountered scalar rows at a26a0/a26b8/a26d0/a26e8/a2700;
   their11/9 logical entries were independently checked against the named
   CodeView constants above. Only values are observed, so share those constants
   without absorbing neighbouring alignment or inventing wider table domains. */
unsigned short line_record_primitive_type(unsigned short flags)
{
    unsigned width;
    if (flags & 0x80) return 3;
    if (!(flags & 0x140)) return 0;
    width = flags & 15;
#ifdef RECON_NATIVE_TEST
    /* Exact unsupported data-domain guard before any unproved table row.
       This is not a claim of original UB or public reachability. */
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

/* Original0x70b60..0x70bff, cdecl2/RET0. Only an unqualified,
   single ordinary pointer can fold into a predefined scalar index. */
unsigned short line_record_primitive_pointer(CType *type, TypeShape *shape)
{
    unsigned short index, model;
    if (shape->related || (shape->flags & 0x77) != 1 ||
        (shape->attributes & 0x300c)) return 0;
    index = line_record_primitive_type(type->flags);
    if (!index) return 0;
    switch (shape->attributes & 15) {
    case 1: return (index & 0xfcff) | 0x400;
    case 2: return (index & 0xfdff) | 0x500;
    case 3: return (index & 0xfbff) | 0x300;
    }
    model = (options.configuration.option_M & 2) ? 5 : 4;
    return (index & 0xf8ff) | (model << 8);
}

/* Original0x72840..0x72878, cdecl1/RET0. Intern the actual descriptor
   with one shape removed and no inherited cached database index. */
CType * line_record_drop_shape(CType *type)
{
    CType description = *type;
    description.debug_index = 0;
    description.related = type->related->related;
    return intern_type((TypeDescription *)&description);
}

/* Original0x70c90..0x70f66, cdecl2/RET0. Serialize the pointer's
   distance, member owner, qualifiers and actual segmented based-symbol leaf. */
unsigned short line_record_pointer_type(CType *type, unsigned short base)
{
    TypeShape *shape = type->related;
    unsigned short flags = shape->flags, attributes = shape->attributes;
    unsigned short extra = 0, owner_index, kind, mode;
    unsigned char *leaf = line_record_leaf;
    CppSymbol *symbol, *debug_symbol;
    if (flags & 0x30) owner_index = line_record_tag_type(shape->data.tag_symbol);
    if ((attributes & 12) && (attributes & 4)) {
        symbol = shape->data.based_symbol;
        mode = attributes & 7;
        if (mode == 5) {
            if ((symbol->kind & 31) == 13) {
                if (symbol->index != 6) {
                    ensure_debug_constant_symbol(symbol);
                    debug_symbol = (CppSymbol *)symbol->field28;
                    ensure_line_record_type(debug_symbol->type);
                }
            } else ensure_line_record_type(symbol->type);
        } else if (mode == 6 || mode == 7) ensure_line_record_type(symbol->type);
    }
    *(unsigned short *)leaf = 2;
    *(unsigned short *)(leaf + 2) = line_record_properties;
    *(unsigned short *)(leaf + 4) = base;
    if (flags & 0x40) leaf[2] = (leaf[2] & 31) | 0x20;
    else if (flags & 0x30) {
        leaf[2] = (leaf[2] & 31) | ((flags & 0x10) ? 0x40 : 0x60);
        *(unsigned short *)(leaf + 6) = owner_index;
        *(unsigned short *)(leaf + 8) = 0;
        extra = 4;
    } else leaf[2] &= 31;
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
                if ((symbol->kind & 31) == 13 && symbol->index == 6) {
                    leaf[2] = (leaf[2] & 0xe5) | 5;
                    *(unsigned short *)(leaf + 6) = 7;
                    *(unsigned short *)(leaf + 8) = 2;
                    *(unsigned short *)(leaf + 10) = 0;
                    *(unsigned short *)(leaf + 12) = 0x1b;
                    leaf[14] = 0;
                    extra = 9;
                } else {
                    leaf[2] = (leaf[2] & 0xe4) | 4;
                    debug_symbol = (symbol->kind & 31) == 13 ? (CppSymbol *)symbol->field28 : symbol;
                    extra = encode_line_record_based_symbol(leaf + 6, debug_symbol);
                }
                break;
            case 6:
                leaf[2] = (leaf[2] & 0xe7) | 7;
                extra = encode_line_record_based_symbol(leaf + 6, symbol);
                break;
            case 7:
                leaf[2] = (leaf[2] & 0xe5) | 5;
                extra = encode_line_record_based_symbol(leaf + 6, symbol);
                break;
            }
        } else if (attributes & 8) leaf[2] = (leaf[2] & 0xe9) | 9;
    } else {
        /* The preceding !(attributes&12) proves the low-nibble domain0..3.
           Original a2714..a2718 independently matches cv_pointer_models. */
        leaf[2] = (leaf[2] & ~31) | cv_pointer_models[attributes & 15];
        if (!(attributes & 15)) {
            kind = flags & 0x77;
            switch (kind) {
            case 1: case 2: case 3: case 4: case 0x10: case 0x20: case 0x40:
                /* This module always chooses near32 here, unlike the older
                   ordinary CodeView module's option_M-sensitive default. */
                leaf[2] = (leaf[2] & ~31) | 10;
                break;
            }
        }
    }
    return intern_line_record((unsigned short)(extra + 6));
}
/* Original0x55330..0x55397, stdcall4. The500-byte local contains
   only the encountered segment prefix and the actual interned source name. */
char *__stdcall make_segment_debug_name(CppSymbol *symbol)
{
    char name[500], *destination = name;
    const char *source = "__seg__";
    do { *destination = *source++; } while (*destination++);
    --destination;
    source = symbol->name;
    do { *destination = *source++; } while (*destination++);
    return intern_identifier(name);
}

/* Original0x55270..0x55330, stdcall4. Permanent allocation supplies
   the original zeroed symbol owner before these individual flag updates. */
void __stdcall ensure_debug_constant_symbol(CppSymbol *symbol)
{
    CppSymbol *debug_symbol;
    if (!options.configuration.debug_info || symbol->field28) return;
    debug_symbol = allocate_permanent(sizeof(CppSymbol));
    symbol->field28 = (unsigned)debug_symbol;
    debug_symbol->kind = 9;
    if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    debug_symbol->index = next_symbol_index++;
    debug_symbol->name = make_segment_debug_name(symbol);
    debug_symbol->type = symbol->type;
    debug_symbol->declaration_flags = (debug_symbol->declaration_flags & ~0x100) | 0x200;
    debug_symbol->storage[0] = (debug_symbol->storage[0] & ~5) | 2;
    debug_symbol->storage[1] |= 0x80;
    debug_symbol->storage[0] |= 0x40;
    debug_symbol->flags |= 0x80;
    debug_symbol->flags |= 4;
    debug_symbol->attributes |= 0x8004;
    ++debug_symbol->references;
    insert_scope_symbol(global_scope, debug_symbol);
}

/* Original0x70420..0x70495, stdcall12. A16-byte work-arena owner
   records each symbol substitution in order, relative to the current leaf. */
void __stdcall queue_debug_type_fixup(CppSymbol *symbol, unsigned char *location, unsigned bytes)
{
    ArenaPool *pool = &compiler_arena.pools[1];
    DebugTypeFixup *fixup;
    if (pool->remaining < (int)sizeof(DebugTypeFixup)) refill_arena(&compiler_arena, pool);
    fixup = (DebugTypeFixup *)pool->next;
    pool->remaining -= sizeof(DebugTypeFixup);
    pool->next += sizeof(DebugTypeFixup);
    if (!debug_type_fixups) {
        last_debug_type_fixup = fixup;
        debug_type_fixups = fixup;
    } else {
        last_debug_type_fixup->next = fixup;
        last_debug_type_fixup = fixup;
    }
    fixup->next = 0;
    fixup->symbol = symbol;
    fixup->offset = (unsigned)location - (unsigned)debug_record_buffer;
    fixup->bytes = bytes;
}
/* Original0x72c40..0x72ccb, cdecl2 with defined AX result. Based
   pointer leaves embed an actual stack/global symbol record and counted name. */
unsigned short encode_line_record_based_symbol(unsigned char *destination, CppSymbol *symbol)
{
    unsigned storage = symbol->storage[0] & 7;
    unsigned short length;
    if (storage == 4 || storage == 5) {
        *(unsigned short *)(destination + 2) = 0x200;
        *(unsigned *)(destination + 4) = 0;
        *(unsigned short *)(destination + 8) = symbol->type->debug_index;
        length = (unsigned short)(copy_line_record_name(destination + 10, symbol->name) + 10);
    } else {
        *(unsigned short *)(destination + 2) = storage == 3 ? 0x201 : 0x202;
        *(unsigned *)(destination + 4) = 0;
        *(unsigned short *)(destination + 8) = 0;
        *(unsigned short *)(destination + 10) = symbol->type->debug_index;
        length = (unsigned short)(copy_line_record_name(destination + 12, symbol->name) + 12);
    }
    *(unsigned short *)destination = length;
    return length;
}

/* Original0x70f70..0x70fbe, cdecl2. Separate-debug arrays retain
   their byte size and an empty counted name after the element/index types. */
unsigned short line_record_array_type(CType *type, unsigned short base)
{
    unsigned char *leaf = line_record_leaf;
    unsigned short length;
    *(unsigned short *)leaf = 3;
    *(unsigned short *)(leaf + 2) = base;
    *(unsigned short *)(leaf + 4) = 0x11;
    length = encode_line_record_unsigned(leaf + 6, type_size(type));
    leaf[length + 6] = 0;
    return intern_line_record((unsigned short)(length + 7));
}

/* Original0x71030..0x7110c and0x71110..0x71128: separate-debug
   argument-list ownership and count helpers, pending genuine entry. */
unsigned line_record_argument_count(FunctionParameters *parameters);

/* Original cursor a2718 starts at BSS workspace991b8. DeviceRoot's
   real vfptr and eight retained overload groups require this nine-WORD
   encountered prefix. The ctor's two methods and at-most-two parameters use
   temporary slots within that bound; full historical capacity is unclaimed. */
unsigned short line_record_scratch[9];
unsigned char *line_record_cursor = (unsigned char *)line_record_scratch;

/* Original0x72cd0..0x72cd6, cdecl0. */
unsigned char *line_record_cursor_position(void)
{
    return line_record_cursor;
}
/* Original0x72ce0..0x72cea, cdecl1. Callers supply the mark
   previously returned by line_record_cursor_position, preserving nesting. */
void line_record_restore_cursor(unsigned char *position)
{
    line_record_cursor = position;
}
/* Original0x72cf0..0x72d06, cdecl1. The actual71083 caller has
   two arguments at initial991b8; Root method queues extend the proved prefix. */
void line_record_push_type(unsigned short index)
{
#ifdef RECON_NATIVE_TEST
    if ((unsigned)line_record_cursor < (unsigned)line_record_scratch ||
        (unsigned)line_record_cursor > (unsigned)line_record_scratch +
            sizeof(line_record_scratch) - sizeof(unsigned short)) {
        __asm { int 3 }
    }
#endif
    *(unsigned short *)line_record_cursor = index;
    line_record_cursor += sizeof(unsigned short);
}

/* Original0x71030..0x7110c, cdecl1. Nested type emission uses a
   separate WORD scratch cursor; save and restore the same caller-owned mark. */
unsigned short line_record_argument_list(FunctionParameters *parameters)
{
    unsigned char *mark = line_record_cursor_position(), *leaf;
    DeclarationParameterInfo *declarations = parameters->declarations;
    CType **type = declarations->types;
    unsigned count, remaining;
    if (declarations->debug_index) return declarations->debug_index;
    count = line_record_argument_count(parameters);
    for (remaining = count; remaining; --remaining, ++type)
        line_record_push_type(*type ? ensure_line_record_type(*type) : 0);
    check_line_record_size(count * 2 + 4);
    leaf = line_record_leaf;
    *(unsigned short *)leaf = 0x201;
    *(unsigned short *)(leaf + 2) = (unsigned short)count;
    line_record_restore_cursor(mark);
    /* Original REP MOVS copies precisely count WORDs, without a CRT call. */
    for (remaining = 0; remaining < count * 2; ++remaining)
        leaf[remaining + 4] = mark[remaining];
    line_record_restore_cursor(mark);
    declarations->debug_index = intern_line_record((unsigned short)(count * 2 + 4));
    return declarations->debug_index;
}
/* Original0x71110..0x71128, cdecl1. The canonical sole void
   parameter denotes an empty argument list; otherwise retain the real count. */
unsigned line_record_argument_count(FunctionParameters *parameters)
{
    DeclarationParameterInfo *declarations = parameters->declarations;
    if (declarations->types[0] == type_0080) return 0;
    return declarations->count;
}

/* Original0x70fc0..0x71029, cdecl2. Resolve the argument list before
   taking the reusable leaf, since that lookup can emit its own records. */
unsigned short line_record_function_type(CType *type, unsigned short base)
{
    unsigned short arguments = line_record_argument_list(type->related->data.parameters);
    unsigned char *leaf = line_record_leaf;
    unsigned short *words = (unsigned short *)leaf;
    words[0] = 8;
    words[1] = base;
    leaf[4] = (unsigned char)debug_calling_convention(type->related->attributes);
    leaf[5] = 0;
    words[3] = (unsigned short)line_record_argument_count(type->related->data.parameters);
    words[4] = arguments;
    return intern_line_record(10);
}

unsigned short line_record_properties; /* original0xa2698, shared leaf properties */

/* Original0x715c0..0x71775, cdecl1/RET0. A named incomplete tag gets
   a forward leaf; anonymous declarations need their genuine complete form. */
unsigned short __cdecl line_record_tag_type(CppSymbol *tag)
{
    unsigned char *leaf = line_record_leaf;
    unsigned short *words = (unsigned short *)leaf;
    CType *type = tag->type;
    unsigned short flags = type->flags, numeric, length;
    if (type->debug_index) return type->debug_index;
    if (tag->name == name_unnamed) {
        type->debug_index = prepare_line_record_tag(tag);
        return tag->type->debug_index;
    }
    tag->flags &= ~8;
    words[1] = 0;
    if (flags == 0x402 || flags == 0x408) {
        words[0] = flags == 0x402 ? 5 : 4;
        words[3] = line_record_properties | 0x80;
        words[3] = (words[3] & ~8) | ((tag->detail.class_info->flags & 2) << 2);
        words[2] = words[5] = words[4] = 0;
        numeric = encode_line_record_unsigned(leaf + 12, 0);
        length = copy_line_record_name(leaf + 12 + numeric, tag->name) + numeric + 12;
    } else if (flags == 0x404) {
        words[0] = 6;
        words[3] = line_record_properties | 0x80;
        words[3] = (words[3] & ~8) | ((tag->detail.class_info->flags & 2) << 2);
        words[2] = 0;
        numeric = encode_line_record_unsigned(leaf + 8, 0);
        length = copy_line_record_name(leaf + 8 + numeric, tag->name) + numeric + 8;
    } else if (flags & 0x200) return line_record_enum_type(tag);
    else {
#ifdef RECON_NATIVE_TEST
        /* Original0x71751 reads the never-written WORD at entryESP-2.
           Keep the original Win32 defect and trap exactly before that use. */
        __asm { int 3 }
#endif
    }
    tag->type->debug_index = intern_line_record(length);
    return tag->type->debug_index;
}
/* Original0x72be0..0x72c1f, cdecl2/RET0, defined AX count. */
unsigned short encode_line_record_unsigned(unsigned char *destination, unsigned value)
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
/* Original0x709c0..0x70a47, cdecl1/RET0. Intern the padded record,
   then clear exactly the bytes consumed from the reusable serialization owner. */
unsigned short intern_line_record(unsigned short length)
{
    unsigned short index;
    unsigned i;
    if (!debug_database) return 0;
    length += pad_line_record(line_record_buffer + length + 2, (unsigned short)(length + 2));
    *(unsigned short *)line_record_buffer = length;
    check_line_record_size(length);
    index = debug_database_intern(debug_database, line_record_buffer);
    for (i = 0; i < (unsigned)length + 2; ++i) line_record_buffer[i] = 0;
    return index;
}
/* Original0x709a0..0x709b5, cdecl1/RET0. */
void check_line_record_size(unsigned length)
{
    if (length >= 0xfafe) fatal_error(67);
}

unsigned short build_line_record_enum_fields(CppSymbol *tag);

/* Original0x71310..0x71425, cdecl1/defined AX. Complete the enum
   field list before emitting the definition, retaining an earlier forward leaf. */
unsigned short prepare_line_record_enum(CppSymbol *tag)
{
    unsigned char *leaf = line_record_leaf;
    unsigned short *words = (unsigned short *)leaf;
    unsigned short properties = line_record_properties;
    unsigned short primitive = (tag->type->flags & ~0x200) | 0x64;
    unsigned short fields, length;
    if (tag->detail.class_info->debug_index)
        return tag->detail.class_info->debug_index;
    if (!tag->type->debug_index && tag->name != name_unnamed)
        line_record_enum_type(tag);
    if (!(tag->flags & 4)) return 0;
    tag->flags &= ~8;
    fields = build_line_record_enum_fields(tag);
    primitive = line_record_primitive_type(primitive);
    words[0] = 7;
    words[1] = (unsigned short)hash_table_count(tag->auxiliary.enumerators);
    words[2] = primitive;
    words[3] = fields;
    words[4] = properties;
    words[4] = (words[4] & ~8) | ((tag->detail.class_info->flags & 2) << 2);
    length = copy_line_record_name(leaf + 10, tag->name) + 10;
    tag->detail.class_info->debug_index = intern_line_record(length);
    if (tag->name == name_unnamed)
        tag->type->debug_index = tag->detail.class_info->debug_index;
    return tag->detail.class_info->debug_index;
}

int compare_line_record_enum_indices(const void *left, const void *right);
unsigned short encode_line_record_number(unsigned char *destination, int value);

/* Original0x71430..0x71593, cdecl1/defined AX. Emit enum members
   in symbol-index order, retaining numeric widths and per-field alignment. */
unsigned short build_line_record_enum_fields(CppSymbol *tag)
{
    unsigned count = hash_table_count(tag->auxiliary.enumerators), i;
    CppSymbol **members = malloc(count * sizeof(CppSymbol *)), *member;
    HashIterator iterator;
    unsigned char *leaf, *field;
    unsigned short length = 0, numeric;
    if (!members) fatal_error(60);
    begin_hash_iteration(&iterator, tag->auxiliary.enumerators);
    for (i = 0; i < count; ++i) {
        next_hash_entry(&iterator);
        members[i] = iterator.value;
    }
    qsort(members, count, sizeof(CppSymbol *), compare_line_record_enum_indices);
    leaf = line_record_leaf;
    *(unsigned short *)leaf = 0x204;
    for (i = 0; i < count; ++i) {
        member = members[i];
        field = leaf + (unsigned)length + 2;
        member->flags &= ~8;
        *(unsigned short *)field = 0x403;
        *(unsigned short *)(field + 2) = line_record_properties | 3;
        numeric = encode_line_record_number(field + 4, (int)((EnumeratorSymbol *)member)->value);
        length += (unsigned short)(numeric +
            copy_line_record_name(field + numeric + 4, member->name) + 4);
        length += pad_line_record(leaf + (unsigned)length + 2, length);
        check_line_record_size((unsigned)length + 2);
    }
    free(members);
    return intern_line_record((unsigned short)(length + 2));
}

/* Original 0x715a0..0x715b3, cdecl2. This distinct qsort callback
   subtracts target-width symbol indices, like the separate CodeView one. */
int compare_line_record_enum_indices(const void *left, const void *right)
{
    const CppSymbol *first = *(CppSymbol * const *)left;
    const CppSymbol *second = *(CppSymbol * const *)right;
    return (int)(first->index - second->index);
}



/* Original0x72020..0x72050, cdecl1. Replace only the access bits
   in the shared class-field properties with this direct base's access. */
unsigned short line_record_base_properties(BaseClassLink *base)
{
    return (line_record_properties & ~3) | (base->flags & 3);
}

/* Original0x71e70..0x71ed0, cdecl4. Nonvirtual base records carry
   access, the already resolved tag index and the signed object displacement. */
unsigned short line_record_nonvirtual_base(int pass, BaseClassLink *base, CppSymbol *tag, unsigned char *destination)
{
    if (pass == 1) {
        ensure_line_record_type(tag->type);
        return 0;
    }
    *(unsigned short *)destination = 0x400;
    *(unsigned short *)(destination + 4) = line_record_base_properties(base);
    *(unsigned short *)(destination + 2) = tag->type->debug_index;
    return (unsigned short)(encode_line_record_number(destination + 6, (int)base->displacement) + 6);
}
/* Original0x72630..0x7264f, cdecl3. Only the serialization pass
   writes the existing record-padding encoding; the discovery pass is empty. */
unsigned short line_record_field_padding(int pass, unsigned char *destination, unsigned short bytes)
{
    if (pass == 2) return pad_line_record(destination, bytes);
    return 0;
}
/* Original0x71f60..0x72014, cdecl4. Virtual-base records retain
   the vbptr displacement and the table offset in pointer-size units. */
unsigned short line_record_virtual_base_data(int pass, unsigned char *destination,
    CppSymbol *tag, CppSymbol *owner)
{
    unsigned pointer_offset, table_offset;
    CType *pointer_type;
    unsigned short numeric;
#ifdef RECON_NATIVE_TEST
    int found = query_virtual_base_pointer(tag, owner,
        &pointer_offset, &table_offset, &pointer_type);
#else
    query_virtual_base_pointer(tag, owner, &pointer_offset, &table_offset, &pointer_type);
#endif
    if (pass == 1) {
        ensure_line_record_type(tag->type);
#ifdef RECON_NATIVE_TEST
        /* Original ignores failure and then reads the unwritten type at
           entryESP-12. Preserve the preceding genuine tag reference. */
        if (!found) { __asm { int 3 } }
#endif
        ensure_line_record_type(pointer_type);
        return 0;
    }
#ifdef RECON_NATIVE_TEST
    /* Pass2 first consumes the unwritten offset at entryESP-4. */
    if (!found) { __asm { int 3 } }
#endif
    numeric = encode_line_record_number(destination + 8, (int)pointer_offset);
    *(unsigned short *)(destination + 2) = tag->type->debug_index;
    *(unsigned short *)(destination + 4) = pointer_type->debug_index;
    return (unsigned short)(numeric + encode_line_record_unsigned(destination + 8 + numeric,
        (unsigned)((int)table_offset / (int)(pointer_type->flags & 15))) + 8);
}

/* Original0x71ed0..0x71f0c, cdecl5. Direct virtual bases preserve
   their access properties before writing the shared virtual-base payload. */
unsigned short line_record_virtual_base(int pass, BaseClassLink *base, CppSymbol *tag, CppSymbol *owner, unsigned char *destination)
{
    if (pass == 2) {
        *(unsigned short *)destination = 0x401;
        *(unsigned short *)(destination + 6) = line_record_base_properties(base);
    }
    return line_record_virtual_base_data(pass, destination, tag, owner);
}
/* Original0x72050..0x72088, cdecl2. Keep the incoming field
   properties while recovering access through the actual virtual-base path. */
unsigned short line_record_virtual_base_properties(CppSymbol *tag, CppSymbol *owner)
{
    unsigned short properties = line_record_properties;
    unsigned char access = virtual_base_access_level(tag, owner);
    return (properties & ~3) | (access & 3);
}

/* Original0x71f10..0x71f5d, cdecl4. Indirect virtual bases use
   the recovered path access with the same virtual-base payload. */
unsigned short line_record_indirect_base(int pass, CppSymbol *tag, CppSymbol *owner, unsigned char *destination)
{
    if (pass == 2) {
        *(unsigned short *)destination = 0x402;
        *(unsigned short *)(destination + 6) = line_record_virtual_base_properties(tag, owner);
    }
    return line_record_virtual_base_data(pass, destination, tag, owner);
}
unsigned short line_record_read_type(void);

/* Original0x72410..0x7251f, cdecl1. Member-function leaves bind
   the actual class, optional this type, arguments and this adjustment. */
unsigned short line_record_method_type(CppSymbol *function)
{
    CType *type = intern_type((TypeDescription *)strip_based_shape(function->type));
    TypeShape *shape;
    CType *this_type;
    unsigned char *leaf;
    unsigned short *words;
    unsigned short result, owner, arguments, this_index;
    if (!type->debug_index) {
        leaf = line_record_leaf;
        shape = type->related;
        result = ensure_line_record_type(line_record_drop_shape(type));
        this_type = shape->data.parameters->associated_type;
        owner = line_record_tag_type(cpp_context_stack->items[function->auxiliary.declaring_class]);
        arguments = line_record_argument_list(type->related->data.parameters);
        if (function->cpp.parts.member_flags & 1) this_index = ensure_line_record_type(this_type);
        else this_index = 0;
        words = (unsigned short *)leaf;
        words[0] = 9;
        words[1] = result;
        words[2] = owner;
        words[3] = this_index;
        leaf[8] = (unsigned char)debug_calling_convention(type->related->attributes);
        leaf[9] = 0;
        words[5] = (unsigned short)line_record_argument_count(type->related->data.parameters);
        words[6] = arguments;
        *(int *)(leaf + 14) = function->this_displacement;
        type->debug_index = intern_line_record(18);
    }
    function->type->debug_index = type->debug_index;
    return function->type->debug_index;
}

/* Original0x72140..0x72402, cdecl1. Resolve retained method types
   into the WORD scratch area, rewind it, then emit the method descriptors. */
unsigned short line_record_method_list(OverloadSet *methods)
{
    unsigned char *leaf = line_record_leaf, *mark = line_record_cursor_position(), *field;
    CppSymbol *function, *next, *owner;
    unsigned short length = 0, properties, index;
    if (methods) {
        function = methods->functions;
        owner = cpp_context_stack->items[function->auxiliary.declaring_class];
        if ((methods->name == name_ctor &&
             (owner->detail.class_info->trivial_special_members & 3) == 3) ||
            (methods->name == name_assignment &&
             (owner->detail.class_info->trivial_special_members & 4)) ||
            (methods->name == name_dtor &&
             (owner->detail.class_info->trivial_special_members & 8))) next = 0;
        else next = function;
    } else {
#ifdef RECON_NATIVE_TEST
        /* Original721d0 consumes the unwritten entryESP-8 next pointer
           only when the overload-set input skipped its initialization. */
        __asm { int 3 }
#endif
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
        index = line_record_method_type(function);
        line_record_push_type(index);
    }
    line_record_restore_cursor(mark);
    /* At7224a the restore argument is still on the stack: ESP+14
       selects the saved leaf (oldESP+10), before caller cleanup. */
    *(unsigned short *)leaf = 0x207;
    if (methods) {
        function = methods->functions;
        owner = cpp_context_stack->items[function->auxiliary.declaring_class];
        if ((methods->name == name_ctor &&
             (owner->detail.class_info->trivial_special_members & 3) == 3) ||
            (methods->name == name_assignment &&
             (owner->detail.class_info->trivial_special_members & 4)) ||
            (methods->name == name_dtor &&
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
        length += pad_line_record(leaf + length + 2, length);
        field = leaf + length + 2;
        properties = (line_record_properties & ~3) | (function->flags & 3);
        *(unsigned short *)field = properties;
        length += 4;
        *(unsigned short *)(field + 2) = line_record_read_type();
        if (function->storage[1] & 2) {
            if (function->cpp.parts.member_flags & 2)
                properties = (properties & ~0x18) | 4;
            else {
                properties = (properties & ~0xc) | 0x10;
                length += 4;
                *(unsigned *)(field + 4) = function->detail.virtual_table_offset;
            }
        } else if ((function->storage[0] & 7) == 3)
            properties = (properties & ~0x14) | 8;
        else properties &= ~0x1c;
        *(unsigned short *)field = properties;
    }
    line_record_restore_cursor(mark);
    return intern_line_record((unsigned short)(length + 2));
}
void __stdcall format_diagnostic_symbol(short text, CppSymbol *symbol);
void __stdcall release_type_diagnostic(short text);

/* Original0x728c0..0x72b5c, cdecl3. Counted browser/debug names
   retain operator spellings, conversion types and constructor/destructor names.
   Byte copies express the original inline string operations without CRT calls. */
unsigned short copy_line_record_method_name(unsigned char *destination,
    CppSymbol *function, int qualified)
{
    char *name = (char *)destination + 1, *cursor = name, *source, *text, *end;
    short handle;
    CppSymbol *owner;
    unsigned char length;
#define APPEND_METHOD_TEXT(text_) do { \
    source = (text_); \
    while ((*cursor = *source++) != 0) ++cursor; \
} while (0)
    if (!(function->declaration_flags & 0x400)) {
        APPEND_METHOD_TEXT(function->name);
    } else {
        if (qualified) {
            begin_type_diagnostic();
            handle = reserve_type_diagnostic();
            owner = cpp_context_stack->items[function->auxiliary.declaring_class];
            format_diagnostic_symbol(handle, owner);
            text = type_diagnostic_text(handle) + 2;
            APPEND_METHOD_TEXT(text);
            APPEND_METHOD_TEXT("::");
            end_type_diagnostic();
        } else *cursor = 0;
        if (function->cpp.parts.token != 1) {
            APPEND_METHOD_TEXT("operator");
            if (function->declaration_flags & 0x4000) {
                handle = format_declarator(0, line_record_drop_shape(function->type));
                text = type_diagnostic_text(handle);
                end = text;
                while (*end) ++end;
                if (end[-1] == ' ') end[-1] = 0;
                APPEND_METHOD_TEXT(" ");
                APPEND_METHOD_TEXT(text);
                release_type_diagnostic(handle);
            } else {
                if (function->cpp.parts.token == 0x97 || function->cpp.parts.token == 0x98) {
                    APPEND_METHOD_TEXT(" ");
                }
                APPEND_METHOD_TEXT(function->name);
            }
        } else if (function->declaration_flags & 0x1000) {
            owner = cpp_context_stack->items[function->auxiliary.declaring_class];
            APPEND_METHOD_TEXT(owner->name);
        } else if (function->declaration_flags & 0x2000) {
            APPEND_METHOD_TEXT("~");
            owner = cpp_context_stack->items[function->auxiliary.declaring_class];
            APPEND_METHOD_TEXT(owner->name);
        } else {
            APPEND_METHOD_TEXT(function->name);
        }
    }
    length = (unsigned char)(cursor - name);
    destination[0] = length;
#undef APPEND_METHOD_TEXT
    /* Original AX sign-extends the low length byte, rather than returning
       the full string size; callers retain that historical count ABI. */
    return (unsigned short)(short)(signed char)length;
}

/* Original0x72090..0x7213c, cdecl5. Emit the overload-set name and
   method-list reference, counting only the functions retained by the compiler. */
unsigned short line_record_methods(int pass, OverloadSet *methods, unsigned char *destination, unsigned *count, unsigned short *properties)
{
    CppSymbol *function = methods->functions;
    unsigned emitted = count_emitted_overloads(methods);
    unsigned short index;
    if (!emitted) return 0;
    if (pass == 1) {
        index = line_record_method_list(methods);
        line_record_push_type(index);
        return 0;
    }
    if (function->cpp.parts.token != 1) *properties |= 4;
    if (methods->name == name_ctor || methods->name == name_dtor) *properties |= 2;
    *(unsigned short *)destination = 0x408;
    *(unsigned short *)(destination + 2) = (unsigned short)emitted;
    *(unsigned short *)(destination + 4) = line_record_read_type();
    *count += emitted;
    return (unsigned short)(copy_line_record_method_name(destination + 6, function, 0) + 7);
}
unsigned short line_record_virtual_shape(CppSymbol *tag);

/* Original0x72570..0x725bc, cdecl4. Bind the vftable shape through
   the pointer leaf in pass1; pass2 reads that same queued type reference. */
unsigned short line_record_vftable_pointer(int pass, CppSymbol *symbol, CppSymbol *owner, unsigned char *destination)
{
    unsigned short shape, pointer;
    if (pass == 1) {
        shape = line_record_virtual_shape(owner);
        pointer = line_record_pointer_type(symbol->type, shape);
        line_record_push_type(pointer);
        return 0;
    }
    *(unsigned short *)destination = 0x40a;
    *(unsigned short *)(destination + 2) = line_record_read_type();
    return 4;
}
/* Original0x725c0..0x7262d, cdecl3. Static members name their
   type and access properties without an object-relative byte displacement. */
unsigned short line_record_static_member(int pass, CppSymbol *symbol, unsigned char *destination)
{
    if (pass == 1) {
        ensure_line_record_type(symbol->type);
        return 0;
    }
    *(unsigned short *)destination = 0x407;
    *(unsigned short *)(destination + 2) = symbol->type->debug_index;
    *(unsigned short *)(destination + 4) = (line_record_properties & ~3) | (symbol->flags & 3);
    return (unsigned short)(copy_line_record_name(destination + 6, symbol->name) + 6);
}
/* Original0x72760..0x727a5, cdecl1. Enum bit-fields use the
   canonical integer type before their width and offset are emitted. */
unsigned short line_record_bit_field_type(CppSymbol *symbol)
{
    unsigned char *leaf = line_record_leaf;
    CType *type = symbol->type;
    unsigned short index;
    if (type->flags & 0x200) type = type_0064;
    index = ensure_line_record_type(type);
    *(unsigned short *)leaf = 0x206;
    leaf[2] = symbol->auxiliary.bit_field.width;
    leaf[3] = symbol->auxiliary.bit_field.offset;
    *(unsigned short *)(leaf + 4) = index;
    return intern_line_record(6);
}
/* Original0x72d10..0x72d20, cdecl0. Advance the common cursor
   before loading the current WORD, retaining the original side-effect order. */
unsigned short line_record_read_type(void)
{
    unsigned char *position = line_record_cursor;
#ifdef RECON_NATIVE_TEST
    /* Keep the original address increment observable even at an unsupported
       prefix boundary, without forming a native out-of-array C pointer sum. */
    line_record_cursor = (unsigned char *)((unsigned)position + sizeof(unsigned short));
    if ((unsigned)position < (unsigned)line_record_scratch ||
        (unsigned)position > (unsigned)line_record_scratch +
            sizeof(line_record_scratch) - sizeof(unsigned short)) {
        __asm { int 3 }
    }
#else
    line_record_cursor += sizeof(unsigned short);
#endif
    return *(unsigned short *)position;
}
/* Original0x72b80..0x72bd3, cdecl2/defined AX. Signed numeric
   leaves carry the smallest signed byte/WORD representation that fits. */
unsigned short encode_line_record_signed(unsigned char *destination, int value)
{
    if (value >= -128 && value <= 127) {
        *(unsigned short *)destination = 0x8000;
        destination[2] = (unsigned char)value;
        return 3;
    }
    if (value >= -32768 && value <= 32767) {
        *(unsigned short *)destination = 0x8001;
        *(short *)(destination + 2) = (short)value;
        return 4;
    }
    *(unsigned short *)destination = 0x8003;
    *(int *)(destination + 2) = value;
    return 6;
}

/* Original0x72b60..0x72b80, cdecl2. Nonnegative values use the
   existing unsigned encoding; negative values retain the signed leaf form. */
unsigned short encode_line_record_number(unsigned char *destination, int value)
{
    if (value < 0) return encode_line_record_signed(destination, value);
    return encode_line_record_unsigned(destination, (unsigned)value);
}

/* Original0x72690..0x72752, cdecl3. Bit-field type indices pass
   through the shared WORD cursor; ordinary members use their type's index. */
unsigned short line_record_data_member(int pass, CppSymbol *symbol, unsigned char *destination)
{
    unsigned short numeric, index, properties;
    if (pass == 1) {
        if ((symbol->kind & 31) == 2) {
            index = line_record_bit_field_type(symbol);
            line_record_push_type(index);
        } else ensure_line_record_type(symbol->type);
        return 0;
    }
    *(unsigned short *)destination = 0x406;
    properties = line_record_properties;
    *(unsigned short *)(destination + 4) = (properties & ~3) | (symbol->flags & 3);
    if ((symbol->kind & 31) == 2) index = line_record_read_type();
    else index = symbol->type->debug_index;
    *(unsigned short *)(destination + 2) = index;
    numeric = encode_line_record_number(destination + 6, (int)symbol->detail.value);
    return (unsigned short)(numeric + copy_line_record_name(destination + 6 + numeric, symbol->name) + 6);
}
/* Original0x72520..0x7256d, cdecl3. Nested tags and typedefs
   name their separately resolved type without a data-member displacement. */
unsigned short line_record_nested_type(int pass, CppSymbol *symbol, unsigned char *destination)
{
    if (pass == 1) {
        ensure_line_record_type(symbol->type);
        return 0;
    }
    *(unsigned short *)destination = 0x409;
    *(unsigned short *)(destination + 2) = symbol->type->debug_index;
    return (unsigned short)(copy_line_record_name(destination + 4, symbol->name) + 4);
}
/* Original0x70710..0x70748, stdcall4. Compiler-generated deleting
   destructors and constructor closures are omitted from the method list. */
int __stdcall line_record_skip_method(OverloadSet *methods)
{
    char *name = methods->name;
    return name == name_vbase_dtor || name == name_delete_dtor ||
        name == name_vector_delete_dtor || name == name_default_ctor_closure;
}
/* Original0x70780..0x707f2, stdcall8. Indirect virtual groups
   omit a base already represented by an active direct-base declaration. */
int __stdcall line_record_has_direct_base(CppSymbol *owner, CppSymbol *base)
{
    CppSymbol *member;
    BaseClassLink *link;
    for (member = owner->auxiliary.scope_members; member; member = member->next) {
        if ((member->kind & 31) != 15 || (member->flags & 0x80)) continue;
        link = (BaseClassLink *)member;
        if (cpp_context_stack->items[link->context_index] == base) return 1;
    }
    return 0;
}

/* Original0x71a60..0x71e6b, cdecl4. Process nonvirtual bases,
   virtual bases, indirect virtual groups, then the ordinary declaration chain.
   Pass1 resolves types; pass2 writes the same fields in that order. */
unsigned short build_line_record_class_fields(int pass, CppSymbol *tag,
    unsigned *count, unsigned short *properties)
{
    unsigned char *leaf = line_record_leaf;
    unsigned short length = 0;
    unsigned kind, storage;
    CppSymbol *member, *base;
    BaseClassLink *link;
    VirtualBaseSearch search;
    *count = 0;
    if (pass == 2) *(unsigned short *)leaf = 0x204;
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        if ((member->kind & 31) != 15 || (member->flags & 0x80)) continue;
        link = (BaseClassLink *)member;
        base = cpp_context_stack->items[link->context_index];
        if (link->state & 1) continue;
        length += line_record_nonvirtual_base(pass, link, base, line_record_leaf + length + 2);
        length += line_record_field_padding(pass, leaf + length + 2, length);
        ++*count;
        check_line_record_size((unsigned)length + 2);
    }
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        if ((member->kind & 31) != 15 || (member->flags & 0x80)) continue;
        link = (BaseClassLink *)member;
        base = cpp_context_stack->items[link->context_index];
        if (!(link->state & 1)) continue;
        length += line_record_virtual_base(pass, link, base, tag, line_record_leaf + length + 2);
        length += line_record_field_padding(pass, leaf + length + 2, length);
        ++*count;
        check_line_record_size((unsigned)length + 2);
    }
    begin_virtual_base_search(&search, tag);
    while (next_virtual_base_search(&search)) {
        if (line_record_has_direct_base(tag, search.tag)) continue;
        length += line_record_indirect_base(pass, search.tag, tag, line_record_leaf + length + 2);
        length += line_record_field_padding(pass, leaf + length + 2, length);
        ++*count;
        check_line_record_size((unsigned)length + 2);
    }
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        kind = member->kind & 31;
        if (kind == 15) continue;
        if (kind == 17 && line_record_skip_method((OverloadSet *)member)) continue;
        if (!member->type || member->name == name_vbptr || kind == 5) continue;
        if (!(kind == 4 && member->name == name_unnamed && (member->type->flags & 0x200))) {
            if (!member->name || member->name == name_unnamed || member->name == name_alignment)
                continue;
            if (kind >= 1 && kind <= 13 && kind != 5 && !member->type) continue;
        }
        if (kind == 17) {
            if (tag->name == name_unnamed) continue;
            length += line_record_methods(pass, (OverloadSet *)member,
                line_record_leaf + length + 2, count, properties);
            --*count;
        } else {
            storage = member->storage[0] & 7;
            if (kind == 4 || storage == 7)
                length += line_record_nested_type(pass, member, line_record_leaf + length + 2);
            else if (member->name == name_vfptr)
                length += line_record_vftable_pointer(pass, member, tag,
                    line_record_leaf + length + 2);
            else if (storage == 3)
                length += line_record_static_member(pass, member, line_record_leaf + length + 2);
            else length += line_record_data_member(pass, member, line_record_leaf + length + 2);
        }
        length += line_record_field_padding(pass, leaf + length + 2, length);
        ++*count;
        check_line_record_size((unsigned)length + 2);
    }
    if (pass == 2) return intern_line_record((unsigned short)(length + 2));
    return 0;
}

/* Original0x71a10..0x71a5e, cdecl3. Two passes share the same
   scratch mark while counting/resolving then serializing the field list. */
unsigned short line_record_class_fields(CppSymbol *tag, unsigned *count,
    unsigned short *properties)
{
    unsigned char *mark = line_record_cursor_position();
    unsigned short fields;
    build_line_record_class_fields(1, tag, count, properties);
    line_record_restore_cursor(mark);
    fields = build_line_record_class_fields(2, tag, count, properties);
    line_record_restore_cursor(mark);
    return fields;
}
/* Original0x727b0..0x72840, cdecl1. Two near32 virtual-method
   descriptors share each byte, with the odd tail's low nibble kept zero. */
unsigned short line_record_virtual_shape(CppSymbol *tag)
{
    unsigned char *leaf = line_record_leaf;
    int count = 0, index;
    VirtualEntryIterator iterator;
    *(unsigned short *)leaf = 10;
    begin_virtual_entries(&iterator,
        first_vector_item(tag->detail.class_info->virtual_tables));
    while (next_virtual_entry(&iterator)) {
        index = count / 2;
        if (count & 1) leaf[index + 4] |= 5;
        else leaf[index + 4] = 0x50;
        check_line_record_size(index + 4);
        ++count;
    }
    *(unsigned short *)(leaf + 2) = (unsigned short)count;
    return intern_line_record((unsigned short)((count + 1) / 2 + 4));
}

/* Original0x71780..0x71a0b, cdecl1. Complete class leaves have
   independent cached indices; named types first retain a forward reference
   so recursive member types can refer back to the same declaration. */
unsigned short __cdecl prepare_line_record_tag(CppSymbol *tag)
{
    unsigned char *leaf = line_record_leaf;
    unsigned short *words = (unsigned short *)leaf;
    unsigned short properties = line_record_properties, vshape = 0;
    unsigned short flags = tag->type->flags, fields, numeric, length;
    unsigned count = 0;
    ClassInfo *info = tag->detail.class_info;
    if (info->debug_index) return info->debug_index;
    if (flags & 0x200) return prepare_line_record_enum(tag);
    if (!tag->type->debug_index && tag->name != name_unnamed)
        line_record_tag_type(tag);
    if (!(tag->flags & 4)) return 0;
    tag->flags &= ~8;
    fields = line_record_class_fields(tag, &count, &properties);
    if ((info->alignment_flags & 0x40) && (flags == 0x402 || flags == 0x408))
        vshape = line_record_virtual_shape(tag);
    words[1] = (unsigned short)count;
    if (flags == 0x402 || flags == 0x408) {
        words[0] = flags == 0x402 ? 5 : 4;
        words[3] = properties;
        words[2] = fields;
        words[5] = vshape;
        words[3] = (words[3] & ~1) | (top_packing(packing_stack)->alignment == 1);
        words[4] = 0;
        words[3] = (words[3] & ~8) | ((info->flags & 2) << 2);
        numeric = encode_line_record_unsigned(leaf + 12, info->size);
        length = numeric + copy_line_record_name(leaf + 12 + numeric, tag->name) + 12;
    } else if (flags == 0x404) {
        words[0] = 6;
        words[3] = properties;
        words[2] = fields;
        words[3] = (words[3] & ~1) | (top_packing(packing_stack)->alignment == 1);
        words[3] = (words[3] & ~8) | ((info->flags & 2) << 2);
        numeric = encode_line_record_unsigned(leaf + 8, info->size);
        length = numeric + copy_line_record_name(leaf + 8 + numeric, tag->name) + 8;
    } else {
#ifdef RECON_NATIVE_TEST
        /* Original7187a consumes the never-written length WORD at
           entryESP-6 only on this nonenum, completed unknown-kind route. */
        __asm { int 3 }
#endif
    }
    info->debug_index = intern_line_record(length);
    if (tag->name == name_unnamed) tag->type->debug_index = info->debug_index;
    return info->debug_index;
}

/* Original0x702c0..0x7031b, stdcall8. CodeView numeric leaves retain
   a signed byte, short or long after the corresponding numeric type tag. */
unsigned __stdcall encode_debug_signed(unsigned char *destination, int value)
{
    if (value >= -128 && value <= 127) {
        *(unsigned short *)destination = 0x8000;
        destination[2] = (unsigned char)value;
        return 3;
    }
    if (value >= -32768 && value <= 32767) {
        *(unsigned short *)destination = 0x8001;
        *(short *)(destination + 2) = (short)value;
        return 4;
    }
    *(unsigned short *)destination = 0x8003;
    *(int *)(destination + 2) = value;
    return 6;
}
