/* CodeView field-list construction and continuation records. */
#include <string.h>
#include "debug-types.h"
#include "debug-info.h"
#include "type-diagnostics-internal.h"
#include "class-finalization.h"
#include "cpp-context.h"
#include "class-lookup.h"
#include "member-expressions.h"
#include "member-access.h"
#include "inline-capture.h"
#include <recon.h>

unsigned debug_field_bytes; /* original0x8ccac */

/* Original 0x70750..0x70776. Padding bytes descend from f0+pad
   through f1, bringing the field stream to a four-byte boundary. */
unsigned pad_debug_field(unsigned char *destination, unsigned length)
{
    unsigned pad = (-length) & 3, left;
    for (left = pad; left; --left)
        *destination++ = (unsigned char)(0xf0 + left);
    return pad;
}

/* Original0x6ea40..0x6ebda. Measure the debug spelling of a member,
   optionally qualifying its owner. Only conversions write the text handle. */
unsigned __stdcall debug_member_name_length(CppSymbol *symbol,
    short *conversion_text, int qualified)
{
    unsigned count = 0, length;
    short text;
    char *name;
    CType description;
    if (!(symbol->declaration_flags & 0x400)) {
        for (length = 0; symbol->name[length]; ++length) ;
        return length;
    }
    if (qualified) {
        begin_type_diagnostic();
        text = reserve_type_diagnostic();
        format_diagnostic_symbol(text,
            cpp_context_stack->items[symbol->auxiliary.declaring_class]);
        name = type_diagnostic_text(text);
        for (count = 0; name[count]; ++count) ;
        end_type_diagnostic();
    }
    if (symbol->cpp.parts.token != 1) {
        count += 8;
        if (symbol->cpp.parts.token == 0x97 || symbol->cpp.parts.token == 0x98) ++count;
        if (symbol->declaration_flags & 0x4000) {
            description = *symbol->type;
            description.related = symbol->type->related->related;
            ++count;
            *conversion_text = format_declarator(0, intern_type((TypeDescription *)&description));
            name = type_diagnostic_text(*conversion_text);
            for (length = 0; name[length]; ++length) ;
            count += length;
            if (name[length - 1] == ' ') --count;
            return count;
        }
        name = symbol->name;
    } else if (symbol->declaration_flags & 0x1000)
        name = ((CppSymbol *)cpp_context_stack->items[symbol->auxiliary.declaring_class])->name;
    else if (symbol->declaration_flags & 0x2000) {
        name = ((CppSymbol *)cpp_context_stack->items[symbol->auxiliary.declaring_class])->name;
        ++count;
    } else name = symbol->name;
    for (length = 0; name[length]; ++length) ;
    return count + length;
}

/* Original 0x6ebe0..0x6ee4e. Write the measured Pascal-style name.
   Ordinary members do not consume the conversion-only diagnostic handle. */
void __stdcall write_debug_member_name(unsigned char *destination,
    CppSymbol *symbol, unsigned length, short conversion_text, int qualified)
{
    char *name, *out = (char *)destination + 1;
    unsigned prefix_length;
    short text;
    destination[0] = (unsigned char)length;
    if (!(symbol->declaration_flags & 0x400)) {
        name = symbol->name;
        do { *out++ = *name; } while (*name++);
        return;
    }
    if (qualified) {
        begin_type_diagnostic();
        text = reserve_type_diagnostic();
        format_diagnostic_symbol(text,
            cpp_context_stack->items[symbol->auxiliary.declaring_class]);
        name = type_diagnostic_text(text) + 2;
        while (*name) *out++ = *name++;
        *out++ = ':';
        *out++ = ':';
        *out = 0;
        end_type_diagnostic();
    } else *out = 0;
    if (symbol->cpp.parts.token != 1) {
        name = "operator";
        while (*name) *out++ = *name++;
        *out = 0;
        prefix_length = 8;
        if (symbol->cpp.parts.token == 0x97 || symbol->cpp.parts.token == 0x98) {
            *out++ = ' ';
            *out = 0;
            prefix_length = 9;
        }
        if (symbol->declaration_flags & 0x4000) {
            *out++ = ' ';
            *out = 0;
            strncat((char *)destination + 1, type_diagnostic_text(conversion_text),
                length - prefix_length - 1);
            release_type_diagnostic(conversion_text);
            return;
        }
        name = symbol->name;
    } else if (symbol->declaration_flags & 0x1000)
        name = ((CppSymbol *)cpp_context_stack->items[symbol->auxiliary.declaring_class])->name;
    else if (symbol->declaration_flags & 0x2000) {
        *out++ = '~';
        name = ((CppSymbol *)cpp_context_stack->items[symbol->auxiliary.declaring_class])->name;
    } else name = symbol->name;
    do { *out++ = *name; } while (*name++);
}

/* Original0x6f480..0x6f8ec. Direct nonvirtual bases precede direct
   virtual bases, followed by inherited virtual bases not already emitted. */
int __stdcall emit_debug_base_fields(CppSymbol *tag, unsigned *member_index,
    unsigned first_member)
{
    BaseClassLink *link;
    CppSymbol *base;
    VirtualBaseSearch search;
    unsigned char *field;
    unsigned length, second_length, pointer_offset, table_offset;
    CType *pointer_type;
    int offset;
    for (link = (BaseClassLink *)tag->auxiliary.scope_members; link;
         link = (BaseClassLink *)link->next) {
        if ((link->kind & 31) != 15 || (link->flags & 0x80)) continue;
        base = cpp_context_stack->items[link->context_index];
        if (link->state & 1) continue;
        if (*member_index < first_member) { ++*member_index; continue; }
        debug_field_bytes += pad_debug_field(debug_record_buffer + debug_field_bytes + 4,
            debug_field_bytes);
        if (debug_field_bytes + 14 >= 0x109) return 0;
        ++*member_index;
        field = debug_record_buffer + debug_field_bytes + 4;
        *(unsigned short *)field = 0x400;
        *(unsigned short *)(field + 2) = base->type->debug_index;
        *(unsigned short *)(field + 4) = (debug_field_properties & ~3) | (link->flags & 3);
        offset = (int)link->displacement;
        if (offset >= 0) length = encode_debug_unsigned(field + 6, (unsigned)offset);
        else length = encode_debug_signed(field + 6, offset);
        debug_field_bytes += length + 6;
    }
    for (link = (BaseClassLink *)tag->auxiliary.scope_members; link;
         link = (BaseClassLink *)link->next) {
        if ((link->kind & 31) != 15 || (link->flags & 0x80)) continue;
        base = cpp_context_stack->items[link->context_index];
        if (!(link->state & 1)) continue;
        if (*member_index < first_member) { ++*member_index; continue; }
        debug_field_bytes += pad_debug_field(debug_record_buffer + debug_field_bytes + 4,
            debug_field_bytes);
        if (debug_field_bytes + 20 >= 0x109) return 0;
        ++*member_index;
        field = debug_record_buffer + debug_field_bytes + 4;
        *(unsigned short *)field = 0x401;
        *(unsigned short *)(field + 2) = base->type->debug_index;
        *(unsigned short *)(field + 6) = (debug_field_properties & ~3) | (link->flags & 3);
#ifdef RECON_NATIVE_TEST
        /* Failure leaves all three original output locals unwritten. */
        if (!query_virtual_base_pointer(base, tag, &pointer_offset, &table_offset,
            &pointer_type)) { __asm { int 3 } }
#else
        query_virtual_base_pointer(base, tag, &pointer_offset, &table_offset, &pointer_type);
#endif
        if ((int)pointer_offset >= 0) length = encode_debug_unsigned(field + 8, pointer_offset);
        else length = encode_debug_signed(field + 8, (int)pointer_offset);
        *(unsigned short *)(field + 4) = pointer_type->debug_index;
        second_length = encode_debug_unsigned(field + 8 + length,
            (int)table_offset / (pointer_type->flags & 15));
        debug_field_bytes += length + second_length + 8;
    }
    begin_virtual_base_search(&search, tag);
    while (next_virtual_base_search(&search)) {
        base = search.tag;
        for (link = (BaseClassLink *)tag->auxiliary.scope_members; link;
             link = (BaseClassLink *)link->next) {
            if ((link->kind & 31) == 15 && !(link->flags & 0x80) &&
                cpp_context_stack->items[link->context_index] == base) break;
        }
        if (link) continue;
        if (*member_index < first_member) { ++*member_index; continue; }
        debug_field_bytes += pad_debug_field(debug_record_buffer + debug_field_bytes + 4,
            debug_field_bytes);
        if (debug_field_bytes + 20 >= 0x109) return 0;
        ++*member_index;
        field = debug_record_buffer + debug_field_bytes + 4;
        *(unsigned short *)field = 0x402;
        *(unsigned short *)(field + 6) =
            (debug_field_properties & ~3) | (virtual_base_access_level(base, tag) & 3);
        *(unsigned short *)(field + 2) = base->type->debug_index;
#ifdef RECON_NATIVE_TEST
        if (!query_virtual_base_pointer(base, tag, &pointer_offset, &table_offset,
            &pointer_type)) { __asm { int 3 } }
#else
        query_virtual_base_pointer(base, tag, &pointer_offset, &table_offset, &pointer_type);
#endif
        if ((int)pointer_offset >= 0) length = encode_debug_unsigned(field + 8, pointer_offset);
        else length = encode_debug_signed(field + 8, (int)pointer_offset);
        *(unsigned short *)(field + 4) = pointer_type->debug_index;
        second_length = encode_debug_unsigned(field + 8 + length,
            (int)table_offset / (pointer_type->flags & 15));
        debug_field_bytes += length + second_length + 8;
    }
    return 1;
}

/* These operations were expanded inline in the original emitter. Keep
   padding and room checks at each field, without adding helper vertices. */
#define FIELD_SPACE(required_) do { \
    padding = (-debug_field_bytes) & 3; \
    for (i = 0; i < padding; ++i) \
        debug_record_buffer[debug_field_bytes + 4 + i] = \
            (unsigned char)(0xf0 + padding - i); \
    debug_field_bytes += padding; \
    if (debug_field_bytes + (required_) + 4 >= 0x109) { \
        split_debug_field_record(2, tag, member_index, vshape_index); \
        return; \
    } \
} while (0)

#define NAME_BYTES(name_, length_) do { \
    for ((length_) = 0; (name_)[length_]; ++(length_)) ; \
    ++(length_); \
} while (0)

/* Original0x6ee50..0x6f480. Continue past already-emitted members,
   filtering declarations without fields and reserving dependent type records. */
void __stdcall emit_debug_field_list(CppSymbol *tag, unsigned first_member,
    unsigned short vshape_index)
{
    unsigned char *leaf = debug_record_buffer + 2, *field, *name;
    CppSymbol *member, *functions;
    unsigned member_index = 0, kind, length, name_length, count, record;
    unsigned padding, i;
    short conversion_text;
    int offset;
    debug_field_bytes = 0;
    *(unsigned short *)leaf = 0x204;
    if (!tag->auxiliary.scope_members) {
        emit_debug_type_record(2);
        return;
    }
    if (!emit_debug_base_fields(tag, &member_index, first_member)) {
        split_debug_field_record(2, tag, member_index, vshape_index);
        return;
    }
    for (member = tag->auxiliary.scope_members; member; member = member->next) {
        kind = member->kind & 31;
        if (kind == 15 || kind == 5) continue;
        if (!(kind == 4 && member->name == name_unnamed &&
              (member->type->flags & 0x200))) {
            if (!member->name || member->name == name_unnamed ||
                member->name == name_alignment) continue;
            if (kind >= 1 && kind <= 13 && kind != 5 && !member->type) continue;
        }
        if (kind == 17) {
            if (member->name == name_vbase_dtor || member->name == name_delete_dtor ||
                member->name == name_vector_delete_dtor ||
                member->name == name_default_ctor_closure || tag->name == name_unnamed)
                continue;
        }
        if (member->name == name_vbptr) continue;
        if (first_member > member_index) {
            ++member_index;
            continue;
        }
        if (kind == 17) {
            functions = ((OverloadSet *)member)->functions;
            count = count_emitted_overloads((OverloadSet *)member);
            if (functions && count) {
                name_length = debug_member_name_length(functions, &conversion_text, 0);
                FIELD_SPACE(name_length + 7);
                field = leaf + debug_field_bytes + 2;
                *(unsigned short *)field = 0x408;
                *(unsigned short *)(field + 2) = (unsigned short)count;
                *(unsigned short *)(field + 4) = next_debug_type_index;
                record = reserve_debug_type(1);
                debug_types[record].subject.symbol = member;
                write_debug_member_name(field + 6, functions, name_length, conversion_text, 0);
                debug_field_bytes += name_length + 7;
            }
        } else if (kind == 4 || (member->storage[0] & 7) == 7) {
            NAME_BYTES(member->name, length);
            FIELD_SPACE(length + 4);
            field = leaf + debug_field_bytes + 2;
            *(unsigned short *)field = 0x409;
            *(unsigned short *)(field + 2) = member->type->debug_index;
            name = field + 4;
            debug_field_bytes += copy_debug_name(name, member->name) - name + 4;
        } else if (member->name == name_vfptr) {
            FIELD_SPACE(4);
            field = leaf + debug_field_bytes + 2;
            *(unsigned short *)field = 0x40a;
            *(unsigned short *)(field + 2) = next_debug_type_index;
            record = reserve_debug_type(0);
            debug_types[record].subject.type = member->type;
            debug_types[record].related_type = vshape_index;
            debug_field_bytes += 4;
        } else if ((member->storage[0] & 7) == 3) {
            NAME_BYTES(member->name, length);
            FIELD_SPACE(length + 6);
            field = leaf + debug_field_bytes + 2;
            *(unsigned short *)field = 0x407;
            *(unsigned short *)(field + 2) = member->type->debug_index;
            *(unsigned short *)(field + 4) =
                (debug_field_properties & ~3) | (member->flags & 3);
            name = field + 6;
            debug_field_bytes += copy_debug_name(name, member->name) - name + 6;
        } else {
            NAME_BYTES(member->name, length);
            FIELD_SPACE(length + 12);
            field = leaf + debug_field_bytes + 2;
            *(unsigned short *)field = 0x406;
            *(unsigned short *)(field + 4) =
                (debug_field_properties & ~3) | (member->flags & 3);
            if (kind == 2) {
                *(unsigned short *)(field + 2) = next_debug_type_index;
                record = reserve_debug_type(1);
                debug_types[record].subject.symbol = member;
            } else *(unsigned short *)(field + 2) = member->type->debug_index;
            offset = (int)member->detail.value;
            if (offset >= 0) length = encode_debug_unsigned(field + 6, (unsigned)offset);
            else length = encode_debug_signed(field + 6, offset);
            name = field + 6 + length;
            debug_field_bytes += copy_debug_name(name, member->name) - name + length + 6;
        }
        ++member_index;
    }
    emit_debug_type_record(debug_field_bytes + 2);
}
#undef NAME_BYTES
#undef FIELD_SPACE
