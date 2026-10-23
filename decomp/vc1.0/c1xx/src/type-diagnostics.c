/* Nested scratch strings used by type and declaration diagnostics. */
#include <stdlib.h>
#include "declarations.h"
#include "cpp-context.h"
#include "type-diagnostics-internal.h"
#include "function-definitions.h"
#include "lexer.h"
#include "inline-capture.h"
#include <recon.h>

TypeDiagnosticStorage type_diagnostic_storage; /* original 0x90f54, slots start 0x90f64 */
unsigned next_type_diagnostic_handle = 100; /* original 0xa1f18 */
unsigned type_diagnostic_depth; /* original 0x9fcc0 */

/* Original 0x68a30..0x68a3a. Each nesting level reserves its first
   text handle through the same allocator used for additional strings. */
unsigned short begin_type_diagnostic(void)
{
    ++type_diagnostic_depth;
    return (unsigned short)reserve_type_diagnostic();
}

/* Original 0x68980..0x68a02. Reserve a free centered string buffer.
   On exhaustion retain the original -1 slot, including its forced-input alias.
   No new capacity guard or synthetic fallback handle is introduced. */
short reserve_type_diagnostic(void)
{
    TypeDiagnosticSlot *slot;
    unsigned char *cursor;
    unsigned bytes, words;
    int index = -1, i;
    for (i = 0; i < 32; ++i) {
        if (!type_diagnostic_slots[i].handle) {
            index = i;
            break;
        }
    }
    slot = &type_diagnostic_slots[index];
    if (!slot->buffer) {
        slot->capacity = 128;
        slot->buffer = malloc(128);
    }
    slot->handle = (short)next_type_diagnostic_handle++;
    slot->left = 64;
    slot->right = 65;
    bytes = slot->capacity;
    cursor = (unsigned char *)slot->buffer;
    for (words = bytes / 4; words; --words) {
        *(unsigned *)cursor = 0;
        cursor += 4;
    }
    for (bytes &= 3; bytes; --bytes) *cursor++ = 0;
    return slot->handle;
}

/* Original 0x68260..0x68295. Emit every enclosing class, then its scope
   separator and name. The first component retains its global :: prefix. */
void __stdcall format_diagnostic_symbol(short text, CppSymbol *symbol)
{
    if (symbol) {
        format_diagnostic_symbol(text, next_member_context(symbol));
        append_type_diagnostic(text, "::");
        append_type_diagnostic(text, symbol->name);
    }
}

/* Original 0x67820..0x6785a. Reserve only the string's nonzero bytes,
   leaving the centered diagnostic buffer's existing NUL boundary intact. */
void __stdcall append_type_diagnostic(short text, char *value)
{
    unsigned length = 0, words, bytes;
    char *cursor;
    while (value[length]) ++length;
    cursor = reserve_type_diagnostic_bytes(text, 1, length);
    for (words = length / 4; words; --words) {
        *(unsigned *)cursor = *(unsigned *)value;
        cursor += 4;
        value += 4;
    }
    for (bytes = length & 3; bytes; --bytes) *cursor++ = *value++;
}

/* Original 0x68aa0..0x68c48. Grow a centered text buffer on either side,
   retaining 16-bit capacity/index wrap. Prepend returns the former left
   boundary; its caller writes backward from that byte. Append writes forward.
   The original prepend growth adds bytes+32 to the prior right boundary;
   retain that arithmetic rather than recomputing a compacted string length. */
char *__stdcall reserve_type_diagnostic_bytes(short text, int append, unsigned bytes)
{
    TypeDiagnosticSlot *slot = find_type_diagnostic_slot(text);
    char *buffer, *cursor, *source, *target;
    unsigned capacity, words, count, offset;
    if (!slot) return 0;
    if (append != 0 && append != 1) return 0;
    if ((!append && slot->left < bytes) ||
        (append && (unsigned)slot->right + bytes >= slot->capacity)) {
        capacity = (unsigned short)(slot->capacity + bytes + 32);
        buffer = malloc(capacity);
        cursor = buffer;
        for (words = capacity / 4; words; --words) {
            *(unsigned *)cursor = 0;
            cursor += 4;
        }
        for (count = capacity & 3; count; --count) *cursor++ = 0;
        if (!append) {
            source = slot->buffer + slot->left + 1;
            target = buffer + bytes + 33;
            count = (unsigned)slot->right - slot->left - 1;
        } else {
            source = slot->buffer;
            target = buffer;
            count = slot->right;
        }
        for (words = count / 4; words; --words) {
            *(unsigned *)target = *(unsigned *)source;
            target += 4;
            source += 4;
        }
        for (count &= 3; count; --count) *target++ = *source++;
        free(slot->buffer);
        slot->buffer = buffer;
        if (!append) {
            slot->right = (unsigned short)(slot->right + bytes + 32);
            slot->left = (unsigned short)(bytes + 32);
        }
        slot->capacity = (unsigned short)capacity;
    }
    if (!append) {
        offset = slot->left;
        slot->left = (unsigned short)(offset - bytes);
    } else {
        offset = slot->right;
        slot->right = (unsigned short)(offset + bytes);
    }
    return slot->buffer + offset;
}

/* Original 0x68940..0x6897a. Match all 32 slots by their 16-bit handle,
   including handle zero's first unused slot; the boundary prefix is excluded. */
TypeDiagnosticSlot *__stdcall find_type_diagnostic_slot(short text)
{
    unsigned short index;
    for (index = 0; index < 32; ++index)
        if (type_diagnostic_slots[index].handle == text)
            return &type_diagnostic_slots[index];
    return 0;
}

/* Original 0x68a70..0x68a93. Text starts one byte beyond the current
   left boundary; an unknown handle has no associated string. */
char *__stdcall type_diagnostic_text(short text)
{
    TypeDiagnosticSlot *slot = find_type_diagnostic_slot(text);
    if (!slot) return 0;
    return slot->buffer + slot->left + 1;
}

/* Original 0x68a40..0x68a61. The outermost diagnostic scope releases
   handles without freeing their reusable buffers. An unmatched end retains
   the original unsigned depth wrap and therefore does not clear the pool. */
void end_type_diagnostic(void)
{
    unsigned index;
    if (--type_diagnostic_depth) return;
    for (index = 0; index < 32; ++index)
        type_diagnostic_slots[index].handle = 0;
}

char *declaration_print_error; /* original 0xa1e58 */
int printing_function_declaration; /* original 0xa1e60 */

/* Original 0x67a40..0x67bd2. A null stream writes reconstructed
   declaration text to the pending compiler text stack. A file receives the
   same spelling, with a final semicolon/newline only for stdout. Unsupported
   spellings are emitted as comments and diagnosed by their recorded kind. */
void __stdcall print_declaration(CppSymbol *symbol, FILE *stream)
{
    char *text;
    unsigned length, kind;
    StorageSpec storage;
    if (!symbol) return;
    declaration_print_error = 0;
    printing_function_declaration = (symbol->kind & 31) == 7;
    text = type_diagnostic_text(format_declarator(symbol, 0));
    if (declaration_print_error) {
        if (!stream) write_work_bytes("/* ", 3, 1, compiler_stack);
        else fwrite("/* ", 3, 1, stream);
        for (length = 0; text[length]; ++length) {}
        if (!stream) write_work_bytes(text, length, 1, compiler_stack);
        else fwrite(text, length, 1, stream);
        if (!stream) write_work_bytes("; */\n", 5, 1, compiler_stack);
        else fwrite("; */\n", 5, 1, stream);
        compiler_warning(1, 36, declaration_print_error);
        return;
    }
    kind = symbol->kind & 31;
    if ((kind != 6 && kind != 7) || !(symbol->declaration_flags & 0x7000)) {
        storage.flags = symbol->storage[0] | ((unsigned short)symbol->storage[1] << 8);
        storage.extended = symbol->storage[2];
        write_storage_specification(storage, stream);
    }
    for (length = 0; text[length]; ++length) {}
    if (!stream) write_work_bytes(text, length, 1, compiler_stack);
    else fwrite(text, length, 1, stream);
    if (stream == stdout) fwrite(";\n", 2, 1, stream);
}

/* Original 0x67be0..0x67c47. Join a declaration's base type and its
   name/shape text. Special function spellings suppress the base type; the
   intermediate shape string is released after being appended. */
short __stdcall format_declarator(CppSymbol *symbol, CType *type)
{
    short text = reserve_type_diagnostic(), shape;
    if (!type) {
        if (!symbol) return text;
        type = symbol->type;
    }
    if (!symbol || (symbol->kind & 31) != 7 ||
        !(symbol->declaration_flags & 0x7000))
        format_declaration_base_type(text, type);
    shape = format_declarator_shape(symbol, type);
    append_type_diagnostic(text, type_diagnostic_text(shape));
    release_type_diagnostic(shape);
    return text;
}

/* Original 0x67f50..0x68137. Format a declaration's name, scope,
   constructor/destructor or conversion-operator spelling, followed by the
   pointer/array/function shapes supplied by the requested type. */
short __stdcall format_declarator_shape(CppSymbol *symbol, CType *type)
{
    short text = reserve_type_diagnostic();
    TypeShape *shape;
    CType *conversion_type;
    CppSymbol *owner;
    unsigned kind;
    if (symbol) {
        kind = symbol->kind & 31;
        if (kind == 7) {
            shape = symbol->type->related;
            if (!(shape->flags & 0x77))
                text = format_shape_attributes(text, append_type_diagnostic,
                    shape->attributes, shape->data.based_symbol, 0);
            if (shape && !(shape->flags & 0x77)) shape = shape->related;
            text = format_shape_attributes(text, append_type_diagnostic,
                shape->attributes, based_symbol, shape);
        } else
            text = format_shape_attributes(text, append_type_diagnostic,
                symbol->type->attributes, based_symbol, 0);
        if (symbol->name != name_formal) {
            if ((symbol->declaration_flags & 0x400) || kind == 3) {
                if (format_enclosing_declarator_scope(text, 0))
                    append_type_diagnostic(text, "::");
            }
            if (kind != 6 && kind != 7)
                append_type_diagnostic(text, symbol->name);
            else {
                if (symbol->declaration_flags & 0x2000)
                    append_type_diagnostic_char(text, '~');
                if (symbol->declaration_flags & 0x3000) {
                    owner = cpp_context_stack->items[symbol->auxiliary.declaring_class];
                    if (owner) append_type_diagnostic(text, owner->name);
                } else if (symbol->cpp.parts.token == 1)
                    append_type_diagnostic(text, symbol->name);
                else if (symbol->declaration_flags & 0x4000) {
                    conversion_type = strip_based_shape(symbol->type);
                    if (printing_inline_declaration) {
                        append_type_diagnostic(text, "operator\"");
                        append_type_diagnostic(text, symbol->name);
                        append_type_diagnostic(text, "\"(void)");
                    } else {
                        append_type_diagnostic(text, "operator`");
                        append_type_diagnostic(text, type_diagnostic_text(
                            format_declarator(0, drop_first_type_shape(conversion_type))));
                        append_type_diagnostic(text, "'(void)");
                    }
                    shape = symbol->type->related;
                    if (shape && !(shape->flags & 0x77)) shape = shape->related;
                    format_this_qualifiers(text, shape->data.parameters->associated_type);
                    return text;
                } else {
                    append_type_diagnostic(text, "operator ");
                    append_type_diagnostic(text, symbol->name);
                }
            }
        }
    }
    if (!type) return text;
    shape = type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    return format_declarator_shapes(text, shape);
}
