/* Declarator text construction for prototype output and diagnostics. */
#include <stdlib.h>
#include "declaration-text.h"
#include "global-ir.h"
#include <recon.h>

int declaration_text_depth; /* original0x6c064 */

/* Original0x42d10..0x42d1a. Open a nested formatting scope and return
   its first text handle, even when the caller ignores that handle. */
short begin_declaration_text(void)
{
    ++declaration_text_depth;
    return new_declaration_text();
}

DeclarationTextBuffer declaration_text_pool[32]; /* original0x5fd10..0x5ff0f */
unsigned next_declaration_text_handle = 100; /* original0x6daa8 */

/* On exhaustion the original retains slot-1 and addresses the preceding
   sixteen bytes: expression_storage's last twelve bytes and option_FAT.
   These contextual lvalues retain that alias and each write's timing without
   relying on neighboring rebuilt globals or adding helper graph vertices. */
#define TEXT_HANDLE(slot_) (*((slot_) < 0 ? (short *)(expression_storage + 2040) : &declaration_text_pool[slot_].handle))
#define TEXT_BYTES(slot_) (*((slot_) < 0 ? (char **)(expression_storage + 2044) : &declaration_text_pool[slot_].bytes))
#define TEXT_BEFORE(slot_) (*((slot_) < 0 ? (unsigned short *)(expression_storage + 2048) : &declaration_text_pool[slot_].before))
#define TEXT_AFTER(slot_) (*((slot_) < 0 ? (unsigned short *)(expression_storage + 2050) : &declaration_text_pool[slot_].after))
#define TEXT_CAPACITY(slot_) (*((slot_) < 0 ? (unsigned short *)&options.option_FAT : &declaration_text_pool[slot_].capacity))

/* Original0x42c60..0x42ce3. Reuse one of32 centered text buffers;
   allocated capacity is retained when a formatting scope releases handles. */
short new_declaration_text(void)
{
    int slot = -1, index;
    unsigned bytes;
    char *cursor;
    for (index = 0; index < 32; ++index) {
        if (!declaration_text_pool[index].handle) { slot = index; break; }
    }
    if (!TEXT_BYTES(slot)) {
        TEXT_CAPACITY(slot) = 128;
        TEXT_BYTES(slot) = malloc(128);
    }
    TEXT_HANDLE(slot) = (short)next_declaration_text_handle++;
    TEXT_BEFORE(slot) = 64;
    TEXT_AFTER(slot) = 65;
    bytes = TEXT_CAPACITY(slot);
    cursor = TEXT_BYTES(slot);
    while (bytes--) *cursor++ = 0;
    return TEXT_HANDLE(slot);
}

char *unprintable_declaration_type; /* original0x6d9e8 */
int declaration_text_is_function; /* original0x6d9f0 */

/* Original0x422e0..0x423c4. Print storage and a complete declarator;
   unsupported types retain a commented declaration and their diagnostic. */
void __stdcall print_function_declaration(FrontendSymbol *symbol, FILE *stream)
{
    short handle;
    char *text, *cursor;
    unsigned bytes, storage;
    if (!symbol) return;
    unprintable_declaration_type = 0;
    declaration_text_is_function = (symbol->kind & 31) == 7;
    handle = build_declaration_text(symbol,0);
    text = declaration_text_value(handle);
    if (unprintable_declaration_type) {
        fwrite("/* ",3,1,stream);
        cursor = text;
        bytes = 0;
        while (*cursor++) ++bytes;
        fwrite(text,bytes,1,stream);
        fwrite("; */\n",5,1,stream);
        compiler_warning(1,36,unprintable_declaration_type);
        return;
    }
    storage = symbol->storage[0] | ((unsigned)symbol->storage[1] << 8) |
        ((unsigned)symbol->storage[2] << 16);
    write_declaration_storage(storage,stream);
    cursor = text;
    bytes = 0;
    while (*cursor++) ++bytes;
    fwrite(text,bytes,1,stream);
    fwrite(";\n",2,1,stream);
}

/* Named unvisited declarator builders and storage formatter. */
/* Original0x423d0. */
/* Original0x423d0..0x42427. Combine the base type and declarator in a
   new handle, releasing the temporary declarator after its text is copied. */
short __stdcall build_declaration_text(FrontendSymbol *symbol, CType *type)
{
    short result = new_declaration_text(), declarator;
    char *text;
    if (!type) {
        if (!symbol) return result;
        type = symbol->type;
    }
    if (type) append_type_text(result,type);
    declarator = build_declarator_text(symbol,type);
    text = declaration_text_value(declarator);
    append_declaration_text(result,text);
    release_declaration_text(declarator);
    return result;
}
/* Original0x42d50. */
char *__stdcall declaration_text_value(short handle)
{
    DeclarationTextBuffer *buffer = find_declaration_text(handle);
    return buffer ? buffer->bytes + buffer->before + 1 : 0;
}
/* Original0x42930. */
void __stdcall write_declaration_storage(unsigned storage, FILE *stream)
{
    short handle;
    char *text, *cursor;
    unsigned bytes = 0;
    begin_declaration_text();
    handle = build_storage_text(0,storage);
    text = declaration_text_value(handle);
    cursor = text;
    while (*cursor++) ++bytes;
    fwrite(text,bytes,1,stream);
    end_declaration_text();
}

/* Named unvisited type/declarator text helpers. */
/* Original0x420c0. */
/* Original0x420c0..0x420fa. Copy without a NUL; the centered buffer's
   cleared spare byte remains the text terminator. */
void __stdcall append_declaration_text(short handle, char *text)
{
    unsigned bytes = 0, index;
    char *cursor = text, *destination;
    while (*cursor++) ++bytes;
    destination = reserve_declaration_text(handle,1,bytes);
    for (index = 0; index < bytes; ++index) destination[index] = text[index];
}
/* Original0x427b0. */
/* Original0x427b0..0x4287e. Base qualifiers precede the primitive/tag
   spelling; anonymous tags retain their unprintable-type diagnostic name. */
short __stdcall append_type_text(short handle, CType *type)
{
    unsigned short flags, attributes;
    FrontendSymbol *tag;
    if (!handle) handle = new_declaration_text();
    if (!type) return handle;
    flags = type->flags;
    attributes = type->attributes;
    if (flags & 0x4000) prepend_declaration_text(handle,"unsigned ");
    if (flags & 0x8000) prepend_declaration_text(handle,"signed ");
    if (attributes & 0x1000) prepend_declaration_text(handle,"const ");
    if (attributes & 0x2000) prepend_declaration_text(handle,"volatile ");
    append_declaration_text(handle,type_specifier_name(flags));
    append_declaration_character(handle,' ');
    tag = (FrontendSymbol *)type->field08;
    if (tag) {
        if (tag->name == name_unnamed) {
            unprintable_declaration_type = type_specifier_name(tag->type->flags);
            return handle;
        }
        append_declaration_text(handle,tag->name);
        append_declaration_character(handle,' ');
    }
    return handle;
}
/* Original0x42620. */
/* Original0x42620..0x426d2. Name text combines a function's outer
   attributes, any based prefix, and the remaining pointer/array/function
   shapes. Anonymous formal placeholders contribute no identifier text. */
short __stdcall build_declarator_text(FrontendSymbol *symbol, CType *type)
{
    short handle = new_declaration_text();
    unsigned short attributes;
    TypeShape *shape;
    if (symbol) {
        if ((symbol->kind & 31) == 7) {
            shape = symbol->type->related;
            if (!(shape->flags & 7))
                handle = write_declarator_attributes(handle,append_declaration_text,
                    shape->attributes,(FrontendSymbol *)shape->field08,0);
            if (shape && !(shape->flags & 7)) shape = shape->related;
            attributes = shape->attributes;
        } else {
            shape = 0;
            attributes = symbol->type->attributes;
        }
        handle = write_declarator_attributes(handle,append_declaration_text,
            attributes,(FrontendSymbol *)based_type,shape);
        if (symbol->name != name_formal) append_declaration_text(handle,symbol->name);
    }
    if (!type) return handle;
    shape = type->related;
    if (shape && !(shape->flags & 7)) shape = shape->related;
    return append_type_shapes(handle,shape);
}
/* Original0x42cf0. */
void __stdcall release_declaration_text(short handle)
{
    DeclarationTextBuffer *buffer = find_declaration_text(handle);
    buffer->handle = 0;
}

/* Original0x42140. */
void __stdcall prepend_declaration_text(short handle, char *text)
{
    char *cursor = text, *destination;
    unsigned count = 0;
    while (*cursor++) ++count;
    if (!count) return;
    destination = reserve_declaration_text(handle,0,count);
    cursor = text + count - 1;
    while (count--) *destination-- = *cursor--;
}
/* Original0x42120. */
/* Original0x42120..0x42137. */
void __stdcall append_declaration_character(short handle, int character)
{
    char *destination = reserve_declaration_text(handle,1,1);
    *destination = (unsigned char)character;
}

/* Original0x42d80. */
/* Original0x42d80..0x42f28. Reserve suffix space forwards or prefix
   space backwards; grow by the requested count plus32 spare bytes while
   retaining the original sixteen-bit cursors and capacity. */
char *__stdcall reserve_declaration_text(short handle, int side, unsigned bytes)
{
    DeclarationTextBuffer *buffer = find_declaration_text(handle);
    unsigned capacity, count, index;
    unsigned short old;
    char *allocation, *source, *destination;
    if (!buffer) return 0;
    if (side == 0) {
        if (buffer->before < bytes) {
            capacity = (unsigned short)(bytes + 32 + buffer->capacity);
            allocation = malloc(capacity);
            for (index = 0; index < capacity; ++index) allocation[index] = 0;
            count = (unsigned)buffer->after - buffer->before - 1;
            source = buffer->bytes + buffer->before + 1;
            destination = allocation + bytes + 33;
            for (index = 0; index < count; ++index) destination[index] = source[index];
            free(buffer->bytes);
            buffer->bytes = allocation;
            buffer->after += (short)bytes + 32;
            buffer->before = (short)bytes + 32;
            buffer->capacity = (unsigned short)capacity;
        }
        old = buffer->before;
        buffer->before = old - (short)bytes;
        return buffer->bytes + old;
    }
    if (side == 1) {
        if (buffer->capacity <= buffer->after + bytes) {
            capacity = (unsigned short)(buffer->capacity + 32 + bytes);
            allocation = malloc(capacity);
            for (index = 0; index < capacity; ++index) allocation[index] = 0;
            for (index = 0; index < buffer->after; ++index)
                allocation[index] = buffer->bytes[index];
            free(buffer->bytes);
            buffer->bytes = allocation;
            buffer->capacity = (unsigned short)capacity;
        }
        old = buffer->after;
        buffer->after = (short)bytes + old;
        return buffer->bytes + old;
    }
    return 0;
}

/* Original0x42c20. */
/* Original0x42c20..0x42c5a. Handles are compared as signed sixteen-bit
   identifiers, including the original search behavior for zero. */
DeclarationTextBuffer *__stdcall find_declaration_text(short handle)
{
    unsigned short index;
    for (index = 0; index < 32; ++index)
        if (declaration_text_pool[index].handle == handle) return &declaration_text_pool[index];
    return 0;
}

/* Original0x424f0. */
short __stdcall write_declarator_attributes(short handle, DeclarationTextWriter writer,
    unsigned short attributes, FrontendSymbol *base, TypeShape *following)
{
    short temporary;
    switch (attributes & 0x70) {
    case 0x20: writer(handle,"__pascal "); break;
    case 0x40: writer(handle,"__fastcall "); break;
    case 0x10: writer(handle,"__cdecl "); break;
    case 0x60: writer(handle,"__stdcall "); break;
    case 0x50: writer(handle,"__syscall "); break;
    case 0: writer(handle,""); break;
    }
    if (!options.extensions_enabled) return handle;
    if (attributes & 0x0c) {
        temporary = build_based_text(attributes,base);
        writer(handle,declaration_text_value(temporary));
        release_declaration_text(temporary);
    } else {
        switch (attributes & 7) {
        case 1: case 0: writer(handle,""); break;
        case 2: writer(handle,"__far "); break;
        case 3: writer(handle,"__far16 "); break;
        }
    }
    if (attributes & 0x100) writer(handle,"__interrupt ");
    if (following && (following->flags & 7) != 3 && (following->flags & 7) != 1) {
        if (attributes & 0x400) writer(handle,"__export ");
        if (attributes & 0x800) writer(handle,"__loadds ");
        if (attributes & 0x200) writer(handle,"__saveregs ");
        if ((attributes & 0x70) == 0x40) writer(handle,"__fastcall ");
    }
    return handle;
}
/* Original0x426e0. */
short __stdcall append_type_shapes(short handle, TypeShape *shape)
{
    unsigned kind;
    char count[80];
    while (shape) {
        kind = shape->flags & 7;
        switch (kind) {
        case 1: case 3:
            prepend_pointer_text(handle,shape);
            shape = shape->related;
            if (!shape) return handle;
            if (!(shape->flags & 1)) {
                append_declaration_character(handle,')');
                prepend_declaration_character(handle,'(');
            } else append_declaration_character(handle,' ');
            break;
        case 2:
            append_declaration_character(handle,'[');
            if (shape->field08) {
                format_integer(shape->field08,count,10);
                append_declaration_text(handle,count);
            }
            append_declaration_character(handle,']');
            shape = shape->related;
            break;
        case 4:
            append_declaration_character(handle,'(');
            append_parameter_text(handle,(ParameterInfo *)shape->field08,shape->flags);
            append_declaration_character(handle,')');
            shape = shape->related;
            break;
        }
    }
    return handle;
}

/* Original0x42430. */
short __stdcall build_based_text(unsigned short attributes, FrontendSymbol *base)
{ __asm { int 3 } return 0; }

/* Original0x42b80. */
short __stdcall prepend_pointer_text(short handle, TypeShape *shape)
{
    unsigned short attributes = shape->attributes;
    if (attributes & 0x1000) prepend_declaration_text(handle,"const ");
    if (attributes & 0x2000) prepend_declaration_text(handle,"volatile ");
    prepend_declaration_character(handle,'*');
    if (!options.extensions_enabled) return handle;
    return write_declarator_attributes(handle,prepend_declaration_text,
        attributes,(FrontendSymbol *)shape->field08,shape);
}
/* Original0x42100. */
void __stdcall prepend_declaration_character(short handle, int character)
{
    char *destination = reserve_declaration_text(handle,0,1);
    *destination = (unsigned char)character;
}
/* Original0x42a40. */
/* Original0x42a40..0x42b7e; the third stack slot is passed by the
   shape walker and discarded here, despite having no semantic consumer. */
short __stdcall append_parameter_text(short handle, ParameterInfo *parameters,
    unsigned short shape_flags)
{
    FrontendSymbol *parameter = 0, *first = 0;
    CType **type;
    int remaining;
    short temporary;
    if (declaration_text_is_function) parameter = function_parameter_declarations;
    declaration_text_is_function = 0;
    remaining = parameters->count;
    type = parameters->types;
    while (parameter && (parameter->kind & 31) != 10 && (parameter->kind & 31) != 11)
        parameter = parameter->next;
    if (parameter) first = parameter = parameter->detail.parameter_next;
    while (*type) {
        temporary = parameter ? build_declaration_text(parameter,0) : build_declaration_text(0,*type);
        append_declaration_text(handle,declaration_text_value(temporary));
        release_declaration_text(temporary);
        if (parameter) {
            parameter = parameter->detail.parameter_next;
            if (parameter == first) parameter = 0;
        }
        ++type;
        if (--remaining < 1) return handle;
        append_declaration_character(handle,',');
    }
    append_declaration_text(handle,remaining ? "..." : "void");
    return handle;
}

/* Original0x42980. */
short __stdcall build_storage_text(short handle, unsigned storage)
{
    char *text;
    if (!handle) handle = new_declaration_text();
    switch (storage & 7) {
    case 0: text = ""; break;
    case 1: text = "extern"; break;
    case 2: text = "/*global*/"; break;
    case 3: text = "static"; break;
    case 4: text = "auto"; break;
    case 5: text = "register"; break;
    case 6: text = "/* really */ register"; break;
    case 7: text = "typedef"; break;
    default: text = "/*CLASS?*/"; break;
    }
    append_declaration_text(handle,text);
    append_declaration_character(handle,' ');
    return append_storage_attributes(handle,storage & 0xffffff);
}

/* Original0x42880. */
short __stdcall append_storage_attributes(short handle, unsigned storage)
{
    short temporary = new_declaration_text();
    char *text;
    if (storage & 0x40000) {
        if (!(storage & 0x80000)) append_declaration_text(temporary,"dllimport ");
        if ((storage & 0xc0000) == 0xc0000) append_declaration_text(temporary,"dllexport ");
    }
    if (storage & 0x10000) append_declaration_text(temporary,"naked ");
    if (storage & 0x20000) append_declaration_text(temporary,"thread ");
    text = declaration_text_value(temporary);
    if (*text) {
        append_declaration_text(handle,"__declspec");
        append_declaration_character(handle,'(');
        append_declaration_text(handle,text);
        append_declaration_text(handle,") ");
    }
    return handle;
}


/* Original0x42d20..0x42d41. Nested scopes share buffers; the outer
   close releases handles while retaining allocations for later declarations. */
void end_declaration_text(void)
{
    unsigned index;
    if (--declaration_text_depth) return;
    for (index = 0; index < 32; ++index) declaration_text_pool[index].handle = 0;
}
