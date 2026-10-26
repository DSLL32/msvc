/* Nested scratch strings used by type and declaration diagnostics. */
#include <stdlib.h>
#include "declarations.h"
#include "cpp-context.h"
#include "type-diagnostics-internal.h"
#include "function-definitions.h"
#include "lexer.h"
#include "inline-capture.h"
#include "mangling.h"
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

/* Only these encountered attribute/spelling tables are needed here;
   other diagnostic vocabularies between them remain unreconstructed. */
typedef struct DeclarationSpelling {
    unsigned short attributes, reserved;
    char *text;
} DeclarationSpelling;
typedef char DeclarationSpellingSizeMustBe8[(sizeof(DeclarationSpelling) == 8) ? 1 : -1];
DeclarationSpelling declaration_calling_conventions[6] = {
    {0x20,0,"__pascal "}, {0x40,0,"__fastcall "}, {0x10,0,"__cdecl "},
    {0x60,0,"__stdcall "}, {0x50,0,"__syscall "}, {0,0,""}
}; /* original 0xa1ea8..0xa1ed8 */
DeclarationSpelling declaration_pointer_models[4] = {
    {1,0,""}, {2,0,"__far "}, {3,0,"__far16 "}, {0,0,""}
}; /* original 0xa1ef8..0xa1f18 */

/* Original 0x67e10..0x67f4d. Emit calling convention and extension
   attributes through the supplied append/prepend writer, retaining the
   original ordering and restrictions for each declarator shape. */
short __stdcall format_shape_attributes(short text, TypeDiagnosticWriter writer,
    unsigned short attributes, void *base, TypeShape *shape)
{
    unsigned convention = attributes & 0x70, index, kind;
    short based;
    for (index = 0; index < 6; ++index) {
        if (declaration_calling_conventions[index].attributes == convention) {
            writer(text, declaration_calling_conventions[index].text);
            break;
        }
    }
    if (!options.configuration.extensions_enabled) return text;
    if (attributes & 12) {
        based = format_based_declaration(attributes, base);
        writer(text, type_diagnostic_text(based));
        release_type_diagnostic(based);
    } else {
        for (index = 0; index < 4; ++index) {
            if (declaration_pointer_models[index].attributes == (attributes & 7)) {
                writer(text, declaration_pointer_models[index].text);
                break;
            }
        }
    }
    if (attributes & 0x100) writer(text, "__interrupt ");
    if (shape) {
        kind = shape->flags & 0x77;
        if (!(shape->flags & 0x40) && kind != 3 && kind != 1 && !(shape->flags & 0x30)) {
            if (attributes & 0x400) writer(text, "__export ");
            if (attributes & 0x800) writer(text, "__loadds ");
            if (attributes & 0x200) writer(text, "__saveregs ");
            if (convention == 0x40) writer(text, "__fastcall ");
        }
    }
    return text;
}

/* Original 0x67c50..0x67d47. Print the active directory's class-scope
   run, optionally with a leading global scope separator. Omit hidden tags
   except when the parser requests their explicit spelling. */
int __stdcall format_enclosing_declarator_scope(short text, int mode)
{
    unsigned char scope_kind, index;
    int count = 0;
    CppSymbol *tag;
    scope_kind = (unsigned char)scope_directory[scope_directory_index]->flags;
    if (scope_kind < 0xfa) return 0;
    index = scope_directory_index;
    do { --index; }
    while ((unsigned char)scope_directory[index]->flags == scope_kind);
    if ((unsigned char)scope_directory[index]->flags == 0xfc && scope_kind == 0xfe)
        --index;
    if (mode) append_type_diagnostic(text, "::");
    ++index;
    while (index <= scope_directory_index) {
        tag = scope_directory[index]->buckets[0];
        if ((token_allocation_flags & 0x200) || !(tag->flags & 0x80)) {
            ++count;
            append_type_diagnostic(text, tag->name);
            if (index != scope_directory_index) append_type_diagnostic(text, "::");
        }
        ++index;
    }
    return count != 0;
}

/* Original 0x688a0..0x68939. Prepending places the class owner
   before a member pointer, and qualifiers after its pointer/reference mark. */
short __stdcall format_pointer_declarator(short text, TypeShape *shape)
{
    unsigned short attributes = shape->attributes;
    if (attributes & 0x1000) prepend_type_diagnostic(text, "const ");
    if (attributes & 0x2000) prepend_type_diagnostic(text, "volatile ");
    prepend_type_diagnostic_char(text, (shape->flags & 0x40) ? '&' : '*');
    if (shape->flags & 0x30) {
        prepend_type_diagnostic(text, "::");
        prepend_type_diagnostic(text, shape->data.tag_symbol->name);
    }
    if (options.configuration.extensions_enabled)
        return format_shape_attributes(text, prepend_type_diagnostic,
            attributes, shape->data.based_symbol, shape);
    return text;
}

/* Original 0x68140..0x6825f. Add the pointer, array and function
   portions of a declarator, inserting parentheses when a following shape
   requires them. An unhandled shape retains the original non-advancing loop. */
short __stdcall format_declarator_shapes(short text, TypeShape *shape)
{
    char number[80];
    while (shape) {
        switch (shape->flags & 0x77) {
        case 1: case 3: case 0x10: case 0x20: case 0x44:
            format_pointer_declarator(text, shape);
            shape = shape->related;
            if (!shape) break;
            if (shape->flags & 0x31) append_type_diagnostic_char(text, ' ');
            else {
                append_type_diagnostic_char(text, ')');
                prepend_type_diagnostic_char(text, '(');
            }
            continue;
        case 2:
            append_type_diagnostic_char(text, '[');
            if (shape->data.array_bound) {
                format_integer(shape->data.array_bound, number, 10);
                append_type_diagnostic(text, number);
            }
            append_type_diagnostic_char(text, ']');
            shape = shape->related;
            break;
        case 4:
            append_type_diagnostic_char(text, '(');
            format_function_parameters(text, shape->data.parameters, shape->flags);
            append_type_diagnostic_char(text, ')');
            if (!(shape->data.parameters->flags & 1))
                format_this_qualifiers(text, shape->data.parameters->associated_type);
            shape = shape->related;
            break;
        case 0x40:
            format_pointer_declarator(text, shape);
            shape = shape->related;
            break;
        default:
            continue;
        }
    }
    return text;
}

/* Original 0x678a0..0x678de. Reserve before the spelling and copy
   backward from the former boundary; an empty prefix leaves it untouched. */
void __stdcall prepend_type_diagnostic(short text, char *value)
{
    unsigned length = 0;
    char *cursor, *source;
    while (value[length]) ++length;
    if (length) {
        cursor = reserve_type_diagnostic_bytes(text, 0, length);
        source = value + length - 1;
        do { *cursor-- = *source--; } while (--length);
    }
}

/* Original 0x67860..0x67878. Reserve one byte before the current
   diagnostic spelling, then publish only the supplied character's low byte. */
void __stdcall prepend_type_diagnostic_char(short text, int character)
{
    char *cursor = reserve_type_diagnostic_bytes(text, 0, 1);
    *cursor = (char)character;
}

/* Original 0x67880..0x67898. Reserve one byte at the string's
   append boundary; the centered buffer retains its existing NUL terminator. */
void __stdcall append_type_diagnostic_char(short text, int character)
{
    char *cursor = reserve_type_diagnostic_bytes(text, 1, 1);
    *cursor = (char)character;
}

/* Original 0x68610..0x6889f. Print explicit parameter declarations
   when available, otherwise their types; omit compiler-generated leading
   parameters and append textual defaults and prototype ellipses in order. */
short __stdcall format_function_parameters(short text, FunctionParameters *parameters,
    unsigned flags)
{
    CppSymbol *source = printing_function_declaration ? parsed_parameter_symbols : 0;
    CppSymbol *first, *current;
    CType **types;
    DefaultArgument *defaults;
    IrToken *expression;
    int remaining, default_count;
    short parameter_text;
    printing_function_declaration = 0;
    remaining = parameters->declarations->count;
    types = parameters->declarations->types;
    if (options.prototypes_only) {
        if (flags & 0x4000) {
            append_type_diagnostic(text, "void");
            return text;
        }
        if ((flags & 0x8000) && !--remaining) {
            append_type_diagnostic(text, "...");
            return text;
        }
    }
    default_count = options.prototypes_only ? 0 : parameters->default_count;
    if (default_count && (flags & 0x8000)) ++default_count;
    defaults = options.prototypes_only ? 0 : parameters->defaults;
    if (defaults) defaults = defaults->following;
    while (source && (source->kind & 31) != 10 && (source->kind & 31) != 11)
        source = source->next;
    if (source) {
        first = current = source->detail.parameter_next;
        while (current->flags & 0x80) {
            --remaining;
            ++types;
            if (!current) break;
            current = current->detail.parameter_next;
            if (current == first) {
                current = 0;
                break;
            }
        }
    } else first = current = 0;
    for (;;) {
        if (!options.prototypes_only && !*types) {
            append_type_diagnostic(text, remaining ? "..." : "void");
            return text;
        }
        if (current) {
            parameter_text = format_declarator(current, 0);
            append_type_diagnostic(text, type_diagnostic_text(parameter_text));
            release_type_diagnostic(parameter_text);
            if (current) {
                current = current->detail.parameter_next;
                if (current == first) current = 0;
            }
        } else {
            parameter_text = format_declarator(0, *types);
            append_type_diagnostic(text, type_diagnostic_text(parameter_text));
            release_type_diagnostic(parameter_text);
        }
        if (default_count >= remaining && defaults) {
            expression = defaults->expression;
            if (expression && expression->kind == 0xc3) {
                append_type_diagnostic(text, " =");
                append_type_diagnostic(text, expression->value.text);
            }
            defaults = defaults->following;
        }
        ++types;
        if (--remaining <= 0) break;
        append_type_diagnostic_char(text, ',');
    }
    if (options.prototypes_only && (flags & 0x8000)) append_type_diagnostic(text, ",...");
    return text;
}

/* Original 0x682a0..0x683b7. Compose base qualifiers, primitive or
   aggregate kind, and its qualified tag name. Hidden tag declarations record
   the unsupported kind for the declaration printer's comment diagnostic. */
short __stdcall format_declaration_base_type(short text, CType *type)
{
    unsigned short flags, attributes;
    CppSymbol *tag;
    if (!text) text = reserve_type_diagnostic();
    if (!type) return text;
    flags = type->flags;
    attributes = type->attributes;
    if (flags & 0x4000) prepend_type_diagnostic(text, "unsigned ");
    if (flags & 0x8000) prepend_type_diagnostic(text, "signed ");
    if (attributes & 0x1000) prepend_type_diagnostic(text, "const ");
    if (attributes & 0x2000) prepend_type_diagnostic(text, "volatile ");
    tag = type->field08 ? cpp_context_stack->items[type->field08] : 0;
    if (!type->field08 || !(tag->detail.class_info->flags2 & 0x20)) {
        append_type_diagnostic(text, type_kind_name(flags));
        append_type_diagnostic_char(text, ' ');
    }
    if (type->field08) {
        tag = cpp_context_stack->items[type->field08];
        if (tag->flags & 0x80) {
            declaration_print_error = type_kind_name(tag->type->flags);
            return text;
        }
        format_diagnostic_symbol(text, tag);
        append_type_diagnostic_char(text, ' ');
    }
    return text;
}

/* Original 0x678e0..0x67a01. Select the source spelling of a primitive
   or tagged base kind. Keep exact floating-kind comparisons and the original
   precedence of class/struct/union bits rather than normalizing invalid flags. */
char *__stdcall type_kind_name(unsigned short kind)
{
    if (kind & 0x40) {
        if (kind & 0x20) return "int";
        switch (kind & 15) {
        case 1: return "char";
        case 2: return "short";
        case 4: return "long";
        }
    } else if (kind & 0x100) {
        switch (kind) {
        case 0x104: return "float";
        case 0x108: return "double";
        case 0x128: return "long double";
        }
    } else if (kind & 0x400) {
        if ((kind & 0x408) == 0x408) return "class";
        if ((kind & 0x402) == 0x402) return "struct";
        if ((kind & 0x404) == 0x404) return "union";
    } else if (kind & 0x200) return "enum";
    else if (kind & 0x80) return "void";
    else if (kind & 0x800) return "__segment";
    return "";
}

/* Original 0x68a10..0x68a23. Release the handle while retaining
   its allocated buffer and cursor state for the next reservation. */
void __stdcall release_type_diagnostic(short text)
{
    find_type_diagnostic_slot(text)->handle = 0;
}

/* Original 0x68c50..0x68cd1. Method qualifiers follow the implicit
   object's pointer model, then its const and volatile type attributes. */
short __stdcall format_this_qualifiers(short text, CType *type)
{
    unsigned kind;
    if (type) {
        kind = type->related->attributes & 7;
        if (kind == 2) append_type_diagnostic(text, "__far ");
        else if (kind == 3) append_type_diagnostic(text, "__huge ");
        if (type->attributes & 0x1000) append_type_diagnostic(text, "const ");
        if (type->attributes & 0x2000) append_type_diagnostic(text, "volatile ");
    }
    return text;
}

/* Original 0x68470..0x684e4. Format storage in a temporary diagnostic
   nesting level and copy its text to the supplied file or compiler text
   stack before releasing that level. Only the three storage bytes are used. */
void __stdcall write_storage_specification(StorageSpec storage, FILE *stream)
{
    char *text;
    unsigned length;
    begin_type_diagnostic();
    text = type_diagnostic_text(format_storage_spec(0, storage));
    for (length = 0; text[length]; ++length) {}
    if (!stream) write_work_bytes(text, length, 1, compiler_stack);
    else fwrite(text, length, 1, stream);
    end_type_diagnostic();
}

/* Original 0x684f0..0x685eb and its 0x685ec..0x6860c switch table.
   Retain inline/virtual/friend spelling order, storage-class comments, and
   the separate extended-storage formatter. */
short __stdcall format_storage_spec(short text, StorageSpec storage)
{
    char *spelling;
    if (!text) text = reserve_type_diagnostic();
    if (storage.flags & 0x100) {
        append_type_diagnostic(text, token_spellings[17]);
        append_type_diagnostic_char(text, ' ');
    }
    if (storage.flags & 0x200) {
        append_type_diagnostic(text, token_spellings[18]);
        append_type_diagnostic_char(text, ' ');
    }
    if (!options.prototypes_only && (storage.flags & 0x400) &&
        !(token_allocation_flags & 0x200)) {
        append_type_diagnostic(text, token_spellings[16]);
        append_type_diagnostic_char(text, ' ');
    }
    if (!(storage.flags & 0x20)) {
        switch (storage.flags & 7) {
        case 0: spelling = ""; break;
        case 1: spelling = token_spellings[13]; break;
        case 2: spelling = "/*global*/"; break;
        case 3: spelling = token_spellings[12]; break;
        case 4: spelling = token_spellings[11]; break;
        case 5: spelling = token_spellings[14]; break;
        case 6: spelling = "/* really */ register"; break;
        case 7: spelling = token_spellings[15]; break;
        default: spelling = "/*CLASS?*/"; break;
        }
        append_type_diagnostic(text, spelling);
        append_type_diagnostic_char(text, ' ');
    }
    return format_storage_extensions(text, storage);
}

/* Original 0x683c0..0x6846d. Build the extended storage keywords in
   a separate centered string and emit __declspec(...) only when nonempty.
   The temporary handle remains owned by the enclosing diagnostic level. */
short __stdcall format_storage_extensions(short text, StorageSpec storage)
{
    short extensions = reserve_type_diagnostic();
    char *spelling;
    if ((storage.extended & 4) && !(storage.extended & 8))
        append_type_diagnostic(extensions, "dllimport ");
    if ((storage.extended & 4) && (storage.extended & 8))
        append_type_diagnostic(extensions, "dllexport ");
    if (storage.extended & 1) append_type_diagnostic(extensions, "naked ");
    if (storage.extended & 2) append_type_diagnostic(extensions, "thread ");
    spelling = type_diagnostic_text(extensions);
    if (*spelling) {
        append_type_diagnostic(text, token_spellings[205]);
        append_type_diagnostic_char(text, '(');
        append_type_diagnostic(text, spelling);
        append_type_diagnostic(text, ") ");
    }
    return text;
}

/* Original0x67d50..0x67e02, stdcall8. Spell the actual Microsoft based
   declaration in a normal nested diagnostic string handle. */
short __stdcall format_based_declaration(unsigned short attributes, void *base)
{
    short text = reserve_type_diagnostic();
    unsigned kind = attributes & 7;
    CppSymbol *symbol;
    append_type_diagnostic(text, "__based(");
    if (kind == 4) append_type_diagnostic(text, "void");
    else if (attributes & 8) append_type_diagnostic(text, "(__segment)__self");
    else {
        symbol = base;
        if ((symbol->kind & 31) == 13) append_type_diagnostic(text, "__segname(\"");
        if (kind == 7 || kind == 6) {
            append_type_diagnostic(text, "(__segment)");
            if (kind == 6) append_type_diagnostic(text, "&");
        }
        append_type_diagnostic(text, symbol->name);
        if ((symbol->kind & 31) == 13) append_type_diagnostic(text, "\")");
    }
    append_type_diagnostic(text, ") ");
    return text;
}

/* Original 0x5cf30..0x5cfc4. Both type spellings share one diagnostic
   lifetime; a missing type uses the existing overloaded-function vocabulary. */
void __stdcall report_type_conversion(unsigned code, int mode,
    unsigned char operation, CType *source, CType *destination)
{
    char *source_text, *destination_text;
    begin_type_diagnostic();
    if (source)
        source_text = type_diagnostic_text(format_declarator(0, source));
    else
        source_text = message_text(345);
    if (destination)
        destination_text = type_diagnostic_text(format_declarator(0, destination));
    else
        destination_text = message_text(345);
    if (mode)
        compiler_warning(mode, code, token_spellings[operation], source_text,
            destination_text);
    else
        report_syntax_error(code, token_spellings[operation], source_text,
            destination_text);
    end_type_diagnostic();
}

/* Original 0x5ce40..0x5cf25. Prefer the real function declaration,
   then a function type, then the caller's spelling for an overload error. */
void __stdcall report_argument_conversion(unsigned code, CppSymbol *function,
    CType *function_type, char *name, unsigned argument, CType *source, CType *target)
{
    short source_handle, target_handle, function_handle;
    char *source_text, *target_text;
    begin_type_diagnostic();
    source_handle = format_declarator(0, source);
    target_handle = format_declarator(0, target);
    if (function) {
        function_handle = format_declarator(function, 0);
        target_text = type_diagnostic_text(target_handle);
        source_text = type_diagnostic_text(source_handle);
        name = type_diagnostic_text(function_handle);
    } else if (function_type) {
        function_handle = format_declarator(0, function_type);
        target_text = type_diagnostic_text(target_handle);
        source_text = type_diagnostic_text(source_handle);
        name = type_diagnostic_text(function_handle);
    } else {
        target_text = type_diagnostic_text(target_handle);
        source_text = type_diagnostic_text(source_handle);
    }
    report_syntax_error(code, name, argument, source_text, target_text);
    end_type_diagnostic();
}

/* Original 0x5cda0..0x5ce31. The context declaration supplies its full
   spelling when present; otherwise retain the caller-owned name. */
void __stdcall report_named_type_difference(unsigned code, CppSymbol *context,
    char *name, CType *source, CType *destination)
{
    short source_handle, destination_handle, context_handle;
    char *source_text, *destination_text;
    begin_type_diagnostic();
    source_handle = format_declarator(0, source);
    destination_handle = format_declarator(0, destination);
    if (context) {
        context_handle = format_declarator(context, 0);
        destination_text = type_diagnostic_text(destination_handle);
        source_text = type_diagnostic_text(source_handle);
        name = type_diagnostic_text(context_handle);
    } else {
        destination_text = type_diagnostic_text(destination_handle);
        source_text = type_diagnostic_text(source_handle);
    }
    report_syntax_error(code, name, source_text, destination_text);
    end_type_diagnostic();
}

/* Original 0x637f0..0x63962; eleven-way switch data0x63964..0x63990.
   Rank9 describes the unavailable class operator directly; other ranks use
   the ordinary source-to-target conversion diagnostic. */
void __stdcall report_conversion_rank_failure(unsigned char operation,
    unsigned rank, CType *target, CType *source)
{
    unsigned code;
    CType *type;
    char *text;
    if (rank & 0x4000)
        return;
    switch (rank & 0x1fff) {
    case 1: code = 750; break;
    case 2: code = 751; break;
    case 3: code = 752; break;
    case 4: code = 753; break;
    case 5: code = 754; break;
    case 6: code = 755; break;
    case 7: code = 756; break;
    case 8: code = 757; break;
    case 9:
        type = target;
        if (!source) {
            if (operation == 0x3b)
                return;
            code = 675;
        } else if (is_class_name_type(target)) {
            code = 676;
        } else if (is_class_name_type(source)) {
            code = 677;
            type = source;
        } else
            return;
        begin_type_diagnostic();
        text = type_diagnostic_text(format_declarator(0, type));
        report_syntax_error(code, token_spellings[operation], text);
        end_type_diagnostic();
        return;
    case 10: code = 674; break;
    case 0x1000: code = 594; break;
    default: code = 446; break;
    }
    report_type_conversion(code, 0, operation, source, target);
}

/* Original0x5cd30..0x5cd5d, stdcall8. Keep formatted type storage
   alive through the variadic diagnostic, then release its text context. */
void __stdcall report_type_error(unsigned code, CType *type)
{
    short text;
    begin_type_diagnostic();
    text = format_declarator(0, type);
    report_syntax_error(code, type_diagnostic_text(text));
    end_type_diagnostic();
}

/* Original 0x3ada0..0x3add3, stdcall12. Format the index before
   the symbol name, preserving the diagnostic's two scratch lifetimes. */
void __stdcall report_call_argument_error(unsigned code, CppSymbol *function,
    unsigned index)
{
    char number[16];
    char *text;
    text = _itoa((int)index, number, 10);
    report_syntax_error(code, format_symbol_name(function), text);
}

/* Original0x33940..0x3396d, stdcall8. The formatted declaration
   remains live through the diagnostic and is released by the matching end. */
void __stdcall report_declaration_error(unsigned code, CppSymbol *symbol)
{
    char *text;
    begin_type_diagnostic();
    text = type_diagnostic_text(format_declarator(symbol, 0));
    report_syntax_error(code, text);
    end_type_diagnostic();
}

/* Original0x5cd60..0x5cd92, stdcall12. Type-only warning text
   shares the same bounded diagnostic-string lifetime as declarations. */
void __stdcall type_warning(unsigned level, unsigned code, CType *type)
{
    char *text;
    begin_type_diagnostic();
    text = type_diagnostic_text(format_declarator(0, type));
    compiler_warning(level, code, text);
    end_type_diagnostic();
}

/* Original0x61500..0x61534, stdcall8. Localize left/right before
   reading the operator spelling, retaining the three diagnostic arguments. */
void __stdcall report_invalid_operand(unsigned char operation, int position)
{
    char *side = message_text(position == 1 ? 301 : 302);
    report_syntax_error(121, token_spellings[operation], side);
}

IrToken *__stdcall strip_function_designator(IrToken *expression);

/* Original0x3b190..0x3b1c5, stdcall8. Diagnose the actual unwrapped
   overload owner and return that same designator for parser recovery. */
IrToken *__stdcall report_expression_error(unsigned char operation, IrToken *expression)
{
    expression = strip_function_designator(expression);
    report_syntax_error(568, token_spellings[operation],
        ((CppSymbol *)expression->value.pointer)->name);
    return expression;
}

/* Original0x3b1d0..0x3b247, stdcall8. Missing declarations retain
   either their actual type or the three defined storage-specification bytes. */
void __stdcall report_missing_declarator(unsigned code, DeclarationType *type)
{
    StorageSpec storage;
    char *text;
    if (!(type->storage[0] & 0x10)) {
        report_type_error(code, type->type);
        return;
    }
    begin_type_diagnostic();
    storage.flags = *(unsigned short *)type->storage;
    storage.extended = type->storage[2];
    text = type_diagnostic_text(format_storage_spec(0, storage));
    report_syntax_error(code, *text ? text : token_spellings[35]);
    end_type_diagnostic();
}
