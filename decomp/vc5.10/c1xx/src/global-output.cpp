#include <string.h>
#include "frontend-init.h"
#include "pointer-array.h"
#include "ir.h"
#include "global-output.h"
#pragma intrinsic(strlen, memcpy)
#include <recon.h>

/* Source-only cells have actual original owners but no native storage,
 * default, snapshot or equality claim. Unknown native access stops first. */
struct GlobalWriterUnmodeledData {
    unsigned pch_reference_count; /* ccb70, 0 */
    unsigned global_reference_limit; /* ccb64, 0x3effff */
};
#ifndef RECON_NATIVE_TEST
static GlobalWriterUnmodeledData writer_unmodeled_data = {
    0, 0x3effff
};
#else
#define writer_unmodeled_data (*(GlobalWriterUnmodeledData *)(recon_unsupported("global writer: unknown initialized cell"), 0))
#endif

/* The operand-format column of the 24 encountered global records. The
 * separate original table column is not consumed by this writer. */
static const char *global_record_formats[24] = {
    "\x03", "\x03\x07\x05\x08\x09\x0d\x0b\x0c\x10\x01\x02\x06",
    "\x03\x07\x05\x08\x09\x0d\x0b\x0c\x10\x01\x02\x06\x04",
    "\x03\x07\x05\x08\x09\x0e\x0a",
    "\x03\x05\x08\x09\x0d\x11\x01\x02\x06", "\x03\x16\x05\x01\x08\x06",
    "\x03", "\x03\x07\x05\x09\x0d\x0b\x01\x10\x13\x06\x15\x04",
    "\x03\x07\x05\x09\x06\x0d\x0b\x01\x14", "\x03\x05\x09\x17\x0f",
    "\x03\x21", "\x03\x07\x05\x09\x0b\x06", "", "\x03\x07",
    "\x03\x05\x08\x09\x0d\x11\x01\x02\x06\x18\x19\x1a\x23\x28\x29",
    "\x03\x1b", "\x03\x05\x08\x09\x0d\x11\x01\x02\x06\x1c",
    "\x03\x1d\x1e\x25\x20", "\x03\x22", "\x03\x24",
    "\x03\x05\x15\x16", "\x03\x05\x16", "\x03\x26", "\x03\x27"
};

/* Source expansions preserve the original calls and post-call good load. */
#define GLOBAL_RESERVE(n) (available = buffer->Ensure(n), buffer->ok &= available)
#define GLOBAL_BYTE(v) do { BYTE byte_value = (BYTE)(v); if (GLOBAL_RESERVE(2)) *buffer->cursor++ = byte_value; } while (0)
#define GLOBAL_WORD(v,n) do { WORD word_value = (WORD)(v); if (GLOBAL_RESERVE(n)) { \
    *(WORD *)buffer->cursor = word_value; buffer->cursor += 2; } } while (0)
#define GLOBAL_DWORD(v,n) do { DWORD dword_value = (DWORD)(v); if (GLOBAL_RESERVE(n)) { \
    *(DWORD *)buffer->cursor = dword_value; buffer->cursor += 4; } } while (0)
#define GLOBAL_INDEX(v) do { unsigned index_value = (v); \
    if (GLOBAL_RESERVE(6)) { \
        if (index_value & ~0x7fU) { *buffer->cursor++ = 0x80; \
            *(DWORD *)buffer->cursor = index_value; buffer->cursor += 4; } \
        else *buffer->cursor++ = (BYTE)index_value; } } while (0)
#define GLOBAL_COMPACT(v) do { unsigned compact_value = (v); \
    if (GLOBAL_RESERVE(4)) { \
        if (compact_value >= 0x3fff80) *buffer->cursor++ = (BYTE)(compact_value + 0x80); \
        else if (compact_value <= 0x3fff) { \
            *buffer->cursor++ = (BYTE)((compact_value & 0x3f) | 0x80); \
            *buffer->cursor++ = (BYTE)(compact_value >> 6); \
        } else { *buffer->cursor++ = (BYTE)(compact_value | 0xc0); \
            *buffer->cursor++ = (BYTE)(compact_value >> 6); \
            *buffer->cursor++ = (BYTE)(compact_value >> 14); } } } while (0)
#define GLOBAL_SHORT(v) do { int short_value = (v); \
    if (GLOBAL_RESERVE(5)) { \
        if (short_value > 0x7fff) { \
            *(DWORD *)buffer->cursor = (((short_value & 0xffffc000U) | 0x4000) << 1) | (short_value & 0x7fff); \
            buffer->cursor += 4; \
        } else { *(WORD *)buffer->cursor = (WORD)short_value; buffer->cursor += 2; } } } while (0)
#define GLOBAL_ZERO() GLOBAL_BYTE(0)

extern "C" {
void __cdecl print_compiler_statistics(void);
CompilerSymbol *__cdecl resolve_symbol_reference(CompilerSymbol *);
CompilerSymbol *__cdecl get_import_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl get_template_symbol(CompilerSymbol *);
unsigned __cdecl external_symbol_index(CompilerSymbol *);
CompilerType *__cdecl qualify_type(CompilerSymbol *, unsigned, WORD);
unsigned __cdecl global_type_size(CompilerType *);
void __cdecl report_incomplete_global_type(CompilerType *);
CompilerType *__cdecl get_function_return_type(CompilerType *);
void __cdecl write_global_type_flags(OutputBuffer *, CompilerGlobalSymbolView *, unsigned);
void __cdecl write_global_reference(CompilerSymbolLink *);
void __cdecl global_debug_location(unsigned, int, CompilerGlobalLocation **);
unsigned __cdecl global_array_element_size(CompilerGlobalSymbolView *);
const char *__cdecl format_symbol_name(CompilerSymbol *, CompilerSymbol *);
const char *__cdecl format_global_debug_name(CompilerSymbol *, char *, unsigned);
int __cdecl is_external_symbol(CompilerGlobalSymbolView *);
const char *__cdecl lookup_global_symbol_name(CompilerGlobalSymbolView *);
unsigned __cdecl get_symbol_type_debug_index(CompilerGlobalSymbolView *);
unsigned __cdecl get_function_calling_convention(CompilerType *);
int __cdecl is_extended_pointer_type(CompilerType *);
CompilerGlobalSymbolView *__cdecl get_symbol_context(CompilerGlobalSymbolView *, CompilerGlobalSymbolView **);
int __cdecl report_compiler_error(int);
void __cdecl compiler_warning(int, ...);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
}
#define GLOBAL_RESOLVE(s) ((CompilerGlobalSymbolView *)resolve_symbol_reference((CompilerSymbol *)(s)))
#define GLOBAL_TYPE_OWNER(t) ((CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[(short)(t)->auxiliary])

extern "C" void __cdecl write_global_ir_record(unsigned, struct CompilerSymbol *);

extern "C" void __cdecl write_global_header(void)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[2]; /* original bf358 */
    buffer->Flush();
    fflush(buffer->file);
    rewind(buffer->file);
    frontend_bss.global_output_active = 0;
    write_global_ir_record(17, NULL);
}


extern "C" void __cdecl write_global_ir_record(unsigned opcode, CompilerSymbol *input)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    OutputBuffer *buffer;
    CompilerType *type;
    CompilerTypeShape *shape;
    const signed char *format;
    unsigned type_flags, attributes;
    unsigned emitted_flags, index, saved_inline_flags;
    /* Without a copied debug location the original writes only this low WORD.
     * Its upper WORD remains unknown, and the native non-IL guard excludes
     * the later full-DWORD consumer until that domain is supported. */
    union { unsigned raw; WORD count; } recorded_count;
    unsigned qualifiers;
    int array = 0, has_location = 0, available;
    CompilerGlobalLocation *location;
    CompilerGlobalLocation saved_location;
    CompilerType *return_type = NULL;
    CompilerGlobalSymbolView *context_symbol;
    const char *text;
#ifdef RECON_NATIVE_TEST
    /* Original-safe source views below retain their actual Win32 owners.
     * Native supports NULL header/file/scope markers and the encountered
     * nameless internal-label form. Other native owner domains remain held. */
    int label_domain = symbol && opcode == 3 && symbol->kind == 9 &&
        symbol->type == frontend_data.builtin_types.type_0063 && !symbol->type->shape &&
        !symbol->name && (symbol->storage_class & 7) != 7 && !(symbol->reference_flags & 1);
    int function_domain = symbol && opcode == 14 && symbol->kind == 7 &&
        symbol->name == frontend_data.builtin_names.main_entry &&
        symbol->type && symbol->type->shape && (symbol->type->shape->flags & 0x77) == 4 &&
        !(symbol->reference_flags & 1) && !(symbol->template_flags & 1) &&
        !(symbol->type_flags & 0x10) && (symbol->storage_class & 7) != 7 &&
        !frontend_data.include_pch_reference_counts;
    /* Storage-class7 declarations become opcode11, which is suppressed
     * before formatting when debug information is disabled. */
    int suppressed_typedef_domain = symbol && symbol->kind == 10 &&
        (symbol->storage_class & 7) == 7 && !frontend_data.options.debug_info &&
        symbol->type;
    if (!frontend_data.option_c9il ||
        (symbol ? (!label_domain && !function_domain && !suppressed_typedef_domain) :
                  (opcode != 17 && opcode != 18 && opcode != 6 && opcode != 13 &&
                   !(opcode == 10 && !frontend_data.options.debug_info))) ||
        (opcode == 17 && frontend_bss.global_output_active))
        recon_unsupported("global record: unmodeled symbol or non-IL domain");
#endif
    if (frontend_data.report_statistics)
        print_compiler_statistics();
    if (symbol && symbol->kind != 23) {
        if (symbol->kind >= 1 && symbol->kind <= 14 && symbol->kind != 5 &&
            (symbol->scope_flags & 4))
            return;
        type = symbol->type;
        type_flags = type->flags;
        attributes = type->attributes;
        shape = type->shape;
        if (symbol->kind != 5) {
            if ((symbol->storage_class & 7) == 7) opcode = 11;
            else {
                if (shape && (shape->flags & 0x77) == 2) {
                    array = 1;
                    if (opcode == 1) opcode = 2;
                }
                if (shape && (shape->flags & 0x77) == 4) {
                    if (symbol->template_flags & 1)
                        return_type = qualify_type((CompilerSymbol *)frontend_data.frontend_pointer_table->items[(short)type->auxiliary], 0, 0);
                    else if (symbol->type_flags & 0x10)
                        return_type = shape->payload.function->type;
                    else
                        return_type = get_function_return_type(type);
                }
                if (symbol->reference_flags & 1) {
                    if (GLOBAL_RESOLVE(symbol)->kind == 14 &&
                        !(GLOBAL_RESOLVE(symbol)->definition_flags & 8) &&
                        !frontend_bss.global_output_active)
                        write_global_reference(&((CompilerNamedSymbol *)GLOBAL_RESOLVE(symbol))->link);
                }
            }
            if (opcode == 11 && !frontend_data.options.debug_info) return;
        }
    } else {
        type = NULL; shape = NULL; type_flags = attributes = 0;
    }
    format = (const signed char *)global_record_formats[opcode];
    if (opcode == 18 || (!frontend_bss.global_output_active && frontend_data.option_c9il))
        buffer = &frontend_bss.output_buffers[2];
    else if (frontend_bss.global_output_active)
        buffer = &frontend_bss.output_buffers[1];
    else {
        switch (opcode) {
        case 1: case 2:
            buffer = &frontend_bss.output_buffers[2];
            if (!(symbol->references & 4)) {
                if (!(symbol->definition_flags & 4) ||
                    ((symbol->storage_class & 7) == 3 &&
                     (get_function_calling_convention(symbol->type) & 0x100)))
                    buffer = &frontend_bss.output_buffers[6];
            }
            break;
        case 4: case 16:
            buffer = (symbol->references & 4) ?
                &frontend_bss.output_buffers[2] : &frontend_bss.output_buffers[6]; break;
        default: buffer = &frontend_bss.output_buffers[2]; break;
        }
    }
    if (frontend_data.emit_global_locations && opcode == 14) {
        has_location = 1;
        unsigned location_index;
        if (frontend_data.option_c9il) location_index = symbol->record_index & 0xffffff;
        else if (symbol->references & ~7U) location_index = symbol->references >> 3;
        else location_index = external_symbol_index((CompilerSymbol *)symbol);
        global_debug_location(location_index, 5, &location);
        saved_location = *location;
        emitted_flags = saved_location.first;
        recorded_count.raw = saved_location.second;
        saved_inline_flags = saved_location.third;
    }
    if ((opcode == 14 || opcode == 1 || opcode == 2) &&
        buffer == &frontend_bss.output_buffers[2] && !frontend_data.omit_external_names &&
        symbol && symbol->name && symbol->name->name) {
        char debug_name[1024];
        int valid = frontend_data.option_c9il || frontend_bss.debug_module->Slot74(
            format_global_debug_name((CompilerSymbol *)symbol, debug_name, sizeof(debug_name)));
        if (!valid) report_compiler_error(55);
    }
    while (*format) {
        switch (*format++) {
        case 1:
            index = (symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) ?
                global_type_size(return_type) : global_type_size(type);
            if (!index && (symbol->kind == 11 || symbol->kind == 12 || symbol->kind == 22))
                report_incomplete_global_type(type);
            if ((symbol->attributes & 4) && (type_flags & 0x400) &&
                (GLOBAL_TYPE_OWNER(type)->qualifiers & 1) && !shape)
                index += symbol->alternate_size;
            if (frontend_data.options.runtime_type_info && (symbol->scope_flags & 0x80))
                index += 4;
            GLOBAL_INDEX(index);
            break;
        case 2:
            if (has_location) {
                write_global_type_flags(buffer, symbol, emitted_flags);
                break;
            }
            qualifiers = (symbol->qualifiers & 0x80) &&
                symbol->kind != 6 && symbol->kind != 7 && symbol->kind != 8 ? 0x80 : 0;
            if (symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) {
                if (symbol->qualifiers & 2) qualifiers |= 0x100;
                if (symbol->template_flags & 0xc) qualifiers |= 0x400;
                if (symbol->member_flags & 1) qualifiers |= 0x400;
            }
            if (symbol->reference_flags & 0x10) qualifiers |= 0x800;
            if (symbol->references & 3) {
                if ((symbol->kind == 10 || symbol->kind == 11) &&
                    (symbol->qualifiers & 8)) qualifiers |= 0x2000;
                if (symbol->kind != 6 && symbol->kind != 7 && symbol->kind != 8 &&
                    (symbol->name != frontend_data.builtin_names.this_parameter ||
                     (symbol->references & 3) != 1)) qualifiers |= 0x100;
            }
            if (symbol->attributes & 8) qualifiers |= 0x400;
            switch (attributes & 0x70) {
            case 0x40: qualifiers |= 4; break;
            case 0x50: qualifiers |= 0x18; break;
            case 0x60:
                if (!shape || (shape->flags & 0x77) != 4 || !(shape->flags & 0x8000))
                    qualifiers |= 0x10;
                break;
            case 0x70: qualifiers |= 0x14; break;
            }
            if ((symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) &&
                !(symbol->template_flags & 0xc)) {
                if (symbol->qualifiers & 0x10) qualifiers |= 0x8000;
                if (symbol->qualifiers & 1) qualifiers |= 0x1000;
            }
            if ((symbol->member_flags & 2) && symbol->kind != 7 && symbol->kind != 8)
                qualifiers |= 0x1000;
            if (symbol->kind == 10 && (symbol->symbol_flags & 0x80)) qualifiers |= 0x40;
            if ((symbol->storage_class & 0x40) &&
                (opcode == 14 || (symbol->attributes & 4) || !(attributes & 3)) &&
                !(symbol->member_flags & 8)) {
                qualifiers |= 0x20;
                if (frontend_data.options.runtime_type_info && (symbol->scope_flags & 0x80))
                    qualifiers |= 0xa0000;
            }
            write_global_type_flags(buffer, symbol, qualifiers);
            emitted_flags = qualifiers;
            break;
        case 3:
            GLOBAL_BYTE(opcode);
            break;
        case 4:
            index = array ? global_array_element_size(symbol) : 0;
            GLOBAL_INDEX(index);
            break;
        case 5:
            if (frontend_data.option_c9il) index = symbol->record_index & 0xffffff;
            else if (symbol->references & ~7U) index = symbol->references >> 3;
            else index = external_symbol_index((CompilerSymbol *)symbol);
            if (frontend_data.option_c9il) buffer->WriteIndex(index);
            else GLOBAL_COMPACT(index);
            break;
        case 6:
            index = 0;
            if (frontend_data.options.debug_info && symbol->kind != 23) {
                if (symbol->kind >= 1 && symbol->kind <= 14 && symbol->kind != 5 &&
                    (symbol->reference_flags & 1) &&
                    GLOBAL_RESOLVE(symbol)->name == frontend_data.builtin_names.directive_section) {
                    GLOBAL_ZERO();
                    break;
                }
                if (symbol->kind == 4) index = symbol->class_detail->debug_index;
                else if (GLOBAL_TYPE_OWNER(type) && !type->shape &&
                         GLOBAL_TYPE_OWNER(type)->kind == 4)
                    index = GLOBAL_TYPE_OWNER(type)->class_detail->debug_index;
                else if (symbol->kind == 8 && (symbol->extended_flags & 0x10))
                    index = get_symbol_type_debug_index(symbol);
                if (!index) index = type->debug_cache;
            }
            GLOBAL_INDEX(index);
            break;
        case 7:
            GLOBAL_BYTE(opcode == 3 ? symbol->parameter_kind : frontend_bss.global_output_active);
            break;
        case 8:
            if (symbol->kind == 9) index = symbol->symbol_value & 0xff;
            else if (symbol->attributes & 8) index = (symbol->storage_class & 7) == 2 ? 0 : 0x25;
            else if (symbol->kind == 10 && (symbol->storage_class & 7) == 3) index = 0x24;
            else index = 0;
            GLOBAL_BYTE(index);
            break;
        case 9:
            if (!symbol->name || symbol->name == frontend_data.builtin_names.unnamed) {
                GLOBAL_ZERO();
                break;
            }
            text = NULL;
            if (!frontend_data.option_c9il && (symbol->references >> 3) >= 16 &&
                (symbol->references >> 3) < writer_unmodeled_data.global_reference_limit &&
                !is_external_symbol(symbol))
                text = lookup_global_symbol_name(symbol);
            if (!text)
                text = format_symbol_name((CompilerSymbol *)symbol, 0);
            buffer->WriteCString(text);
            break;
        case 10:
            GLOBAL_BYTE((symbol->qualifiers & 4) ? 2 : 1);
            break;
        case 11: case 32:
            GLOBAL_ZERO();
            break;
        case 12:
            if (symbol->kind == 11 || symbol->kind == 12 || symbol->kind == 22) index = 3;
            else if ((symbol->storage_class & 7) >= 1 && (symbol->storage_class & 7) <= 3) {
                unsigned convention = type->GetCallingConvention();
                if ((convention & 0x100) &&
                    !((symbol->reference_flags & 1) && GLOBAL_RESOLVE(symbol)->kind == 14 &&
                      (GLOBAL_RESOLVE(symbol)->record_index & 0xffffff) != 3) &&
                    !(symbol->scope_flags & 0x20) &&
                    (type->shape || !(type->flags & 0x400) || (short)type->auxiliary < 0 ||
                     !(GLOBAL_TYPE_OWNER(type)->class_detail->template_flags & 0x10))) index = 4;
                else if ((symbol->attributes & 8) && (symbol->storage_class & 0x40) &&
                    frontend_data.options.readonly_strings &&
                    (!(symbol->reference_flags & 1) || GLOBAL_RESOLVE(symbol)->kind != 14 ||
                     (GLOBAL_RESOLVE(symbol)->record_index & 0xffffff) == 3)) index = 4;
                else index = 2;
            } else index = 1;
            GLOBAL_BYTE(index);
            break;
        case 13: {
            unsigned shape_flags;
            BYTE storage = 0;
            CompilerType *classified_type = type;
            if (opcode == 4 || opcode == 16 || opcode == 14) {
                classified_type = return_type;
                shape_flags = return_type->shape ? return_type->shape->flags : 0;
            } else {
                shape_flags = shape ? shape->flags : 0;
                if ((type_flags & 0x400) &&
                    (type_flags == 0x404 || (GLOBAL_TYPE_OWNER(type)->class_detail->context_flags & 1)))
                    storage = 0x10;
            }
            if ((shape_flags & 0x77) == 2) storage |= 6;
            else if ((shape_flags & 0x30) && is_extended_pointer_type(classified_type)) storage |= 6;
            else if (shape_flags & 0x10) storage |= 1;
            else if (shape_flags & 0x20) storage |= 3;
            else if (shape_flags & 0x4b) storage |= 3;
            else if (type_flags & 0x4000) storage |= 2;
            else {
                switch (type_flags & 0xffff) {
                case 0x41: case 0x61:
                    storage |= (!frontend_data.options.unsigned_char || (type_flags & 0x8000)) ? 1 : 2;
                    break;
                case 0x42: case 0x43: case 0x44: case 0x4c: case 0x4d:
                case 0x62: case 0x63: case 0x64: case 0x203: case 0x8041: case 0x8061:
                    storage |= 1; break;
                case 0x80: storage |= 7; break;
                case 0x106: case 0x107: case 0x108: storage |= 5; break;
                case 0x402: case 0x404: case 0x408: storage |= 6; break;
                case 0x802: storage |= 2; break;
                }
            }
            if (storage > 0x7f) GLOBAL_BYTE((storage >> 8) | 0x80);
            GLOBAL_BYTE(storage);
            break;
        }
        case 14:
            index = symbol->references & 3;
            if (GLOBAL_RESERVE(4) && !buffer->WriteCompact((short)index)) {
                *buffer->cursor++ = 0x80;
                *(WORD *)buffer->cursor = (WORD)index;
                buffer->cursor += 2;
            }
            break;
        case 15:
            if (symbol->alternate_name && symbol->alternate_name != frontend_data.builtin_names.unnamed) {
                text = symbol->alternate_name->name;
                index = strlen(text) + 1;
                if (GLOBAL_RESERVE(index + 1)) {
                    memcpy(buffer->cursor, text, index);
                    buffer->cursor += index;
                }
            } else GLOBAL_ZERO();
            break;
        case 16:
            if ((symbol->member_flags & 4) && opcode != 7)
                index = (symbol->definition_flags & 4) ? 7 : 2;
            else if ((symbol->member_flags & 8) && opcode != 7) index = 8;
            else if (symbol->storage_class & 0x40) index = ((~symbol->attributes & 4) | 2) >> 1;
            else if (((symbol->storage_class & 7) >= 4 && (symbol->storage_class & 7) <= 6) ||
                     ((symbol->storage_class & 7) == 3 && !(symbol->type_flags & 4))) index = 4;
            else if ((symbol->attributes & 4) || (symbol->definition_flags & 4)) index = 1;
            else if ((symbol->storage_class & 7) == 1) index = 2;
            else if (array && !shape->payload.value) index = 2;
            else index = 3;
            GLOBAL_BYTE(index);
            break;
        case 17:
            if ((symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) &&
                (symbol->definition_flags & 4)) {
                if (symbol->member_flags & 4) index = 9;
                else if (symbol->member_flags & 8) index = 10;
                else if (symbol->storage_class & 0x40) index = 5;
                else index = (symbol->storage_class & 7) == 3 && !(symbol->type_flags & 4) ? 3 : 5;
            } else {
                if ((symbol->storage_class & 7) == 3 && !(symbol->type_flags & 4) &&
                    !(frontend_data.options.incremental_compile && !frontend_data.options.debug_types)) {
                    compiler_warning(129, symbol);
                    GLOBAL_BYTE(2);
                    break;
                }
                if ((symbol->symbol_flags & 1) && (symbol->storage_class & 7) != 1)
                    report_language_diagnostic(1, 506, symbol);
                index = (symbol->member_flags & 8) ? 10 : 4;
            }
            GLOBAL_BYTE(index);
            break;
        case 19:
            if (symbol->qualifiers & 0x10) {
                CompilerGlobalSymbolView *owner;
                context_symbol = get_symbol_context(symbol, &owner);
                GLOBAL_INDEX(owner->symbol_value);
            } else GLOBAL_ZERO();
            break;
        case 20: {
            int value = symbol->integer_value;
            int width = value >= -32768 && value <= 32767 ? 2 : 4;
            if (GLOBAL_RESERVE(4) && !buffer->WriteCompact(width)) {
                *buffer->cursor++ = 0x80;
                *(WORD *)buffer->cursor = (WORD)width;
                buffer->cursor += 2;
            }
            GLOBAL_BYTE(value);
            GLOBAL_BYTE(value >> 8);
            if (width == 4) {
                GLOBAL_BYTE(value >> 16);
                GLOBAL_BYTE(value >> 24);
            }
            break;
        }
        case 21: case 22: case 28: {
            CompilerGlobalSymbolView *target;
            if (format[-1] == 28)
                target = (CompilerGlobalSymbolView *)get_import_symbol((CompilerSymbol *)symbol);
            else if (format[-1] == 22)
                target = GLOBAL_RESOLVE(symbol);
            else if ((symbol->attributes & 0x40) && symbol->kind == 10)
                target = (CompilerGlobalSymbolView *)get_template_symbol((CompilerSymbol *)symbol);
            else if ((context_symbol->attributes & 0x40) && context_symbol->kind == 10)
                target = (CompilerGlobalSymbolView *)get_template_symbol((CompilerSymbol *)context_symbol);
            else target = context_symbol;
            if (frontend_data.option_c9il) index = target->record_index & 0xffffff;
            else if (target->references & ~7U) index = target->references >> 3;
            else index = external_symbol_index((CompilerSymbol *)target);
            if (frontend_data.option_c9il) {
                if (buffer->Prepare(4)) {
                    if ((int)index > 0x7fff) {
                        *(DWORD *)buffer->cursor = (((index & 0xffffc000) | 0x4000) << 1) | (index & 0x7fff);
                        buffer->cursor += 4;
                    } else {
                        *(WORD *)buffer->cursor = (WORD)index;
                        buffer->cursor += 2;
                    }
                }
            } else GLOBAL_COMPACT(index);
            break;
        }
        case 23:
            index = (BYTE)symbol->parameter_kind;
            if (index != 0 && index != 2) index = index == 3 ? 3 : 1;
            GLOBAL_BYTE(index);
            break;
        case 24:
            if (frontend_data.option_c9il) {
                GLOBAL_INDEX(frontend_bss.saved_reference_count);
                index = frontend_bss.local_reference_count +
                    (frontend_data.include_pch_reference_counts ? writer_unmodeled_data.pch_reference_count : 0);
                GLOBAL_INDEX(index);
            }
            break;
        case 25:
            if (!has_location) recorded_count.count = frontend_bss.ir_record_count > 0xffff ?
                0xffff : (WORD)frontend_bss.ir_record_count;
            if (GLOBAL_RESERVE(4)) {
                if ((unsigned)(int)(short)recorded_count.count & ~0x7fU) {
                    *buffer->cursor++ = 0x80;
                    *(WORD *)buffer->cursor = recorded_count.count;
                    buffer->cursor += 2;
                } else *buffer->cursor++ = (BYTE)recorded_count.count;
            }
            break;
        case 26:
            if (!has_location) {
                index = ((symbol->symbol_flags & 1) || (symbol->template_flags & 0xc)) ? 0x80 : 0;
                index |= ((symbol->qualifiers >> 5) & 4) |
                    (frontend_bss.inline_record_flag ? 0x20 : 0) |
                    (frontend_data.force_type_serialization ? 8 : 0) |
                    (frontend_data.include_pch_reference_counts ? 1 : 0) |
                    ((symbol->extended_flags & 0xe) << 7);
                int choose_inline = frontend_data.options.inline_level == 1;
                if (frontend_data.options.inline_level == 2) {
                    if (!frontend_data.type_debug_mode) choose_inline = 1;
                    else {
                        if (!frontend_data.disable_inline_records &&
                            (((symbol->storage_class & 7) == 3 && !(symbol->type_flags & 4) &&
                              (symbol->references & 3) == 1) ||
                             frontend_bss.ir_record_count < (unsigned)frontend_data.options.option_bu))
                            index |= 0x40;
                        choose_inline = 1;
                    }
                }
                if (choose_inline) {
                    if (frontend_data.disable_inline_records || (type->shape->flags & 0x8000)) {
                        index = (unsigned)(int)(short)index & ~0x40U;
                        if (symbol->symbol_flags & 1)
                            report_language_diagnostic(4, 710, symbol);
                    } else if (symbol->symbol_flags & 1) index |= 0x40;
                }
                saved_inline_flags = index;
            } else index = saved_inline_flags;
            GLOBAL_SHORT(index);
            break;
        case 29:
            GLOBAL_WORD(0x386, 3);
            break;
        case 30:
            GLOBAL_DWORD(frontend_data.option_c9il ? 0x01309260 : 0x01309261, 5);
            break;
        case 37:
            GLOBAL_DWORD(frontend_data.option_c9il ? frontend_data.next_symbol_record_id : 0x400000, 5);
            break;
        case 33: {
            GLOBAL_INDEX(frontend_data.global_name_count);
            text = frontend_data.global_option_string;
            if (text) {
                index = strlen(text) + 1;
                if (GLOBAL_RESERVE(index + 1)) {
                    memcpy(buffer->cursor, text, index);
                    buffer->cursor += index;
                }
            } else GLOBAL_ZERO();
            CompilerGlobalOption *item = frontend_data.global_options;
            if (!item) { GLOBAL_INDEX(0); break; }
            unsigned count = 0;
            frontend_data.global_options = NULL;
            while (item) {
                CompilerGlobalOption *next = item->next;
                item->next = frontend_data.global_options;
                frontend_data.global_options = item;
                item = next;
                ++count;
            }
            GLOBAL_INDEX(count);
            for (item = frontend_data.global_options; item; item = item->next) {
                GLOBAL_BYTE(item->kind);
                if (item->kind == 7) {
                    const BYTE *bytes = item->value;
                    unsigned length = (unsigned)(int)(signed char)(bytes[0] + 1);
                    while (length--) GLOBAL_BYTE(*bytes++);
                } else {
                    text = (const char *)item->value;
                    index = strlen(text) + 1;
                    if (GLOBAL_RESERVE(index + 1)) {
                        memcpy(buffer->cursor, text, index);
                        buffer->cursor += index;
                    }
                }
            }
            break;
        }
        case 34: {
            CompilerSourceFile *file = (CompilerSourceFile *)frontend_bss.saved_context_stack.origin;
            if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(55);
            file->record_index = (file->record_index & 0xff000000) |
                (frontend_data.next_symbol_record_id++ & 0xffffff);
            index = file->OutputIndex();
            if (frontend_data.option_c9il) GLOBAL_SHORT(index);
            else GLOBAL_COMPACT(index);
            text = frontend_bss.saved_context_stack.FileName();
            index = strlen(text) + 1;
            available = 0;
            if ((int)(index + 1) >= 0) {
                available = 1;
                if (buffer->cursor + index + 1 > buffer->data + buffer->capacity)
                    available = buffer->Grow(index + 1);
            }
            buffer->ok &= available;
            if (buffer->ok) {
                memcpy(buffer->cursor, text, index);
                buffer->cursor += index;
            }
            break;
        }
        case 35: {
            short value = frontend_bss.function_parameter_count;
            available = 1;
            if (buffer->cursor + 4 > buffer->data + buffer->capacity)
                available = buffer->Grow(4);
            buffer->ok &= available;
            if (buffer->ok) {
                if ((unsigned)(int)value & ~0x7fU) {
                    *buffer->cursor++ = 0x80;
                    *(WORD *)buffer->cursor = (WORD)value;
                    buffer->cursor += 2;
                } else *buffer->cursor++ = (BYTE)value;
            }
            break;
        }
        case 38:
            text = frontend_bss.global_string_record;
            index = strlen(text) + 1;
            available = 0;
            if ((int)(index + 1) >= 0) {
                available = 1;
                if (buffer->cursor + index + 1 > buffer->data + buffer->capacity)
                    available = buffer->Grow(index + 1);
            }
            buffer->ok &= available;
            if (buffer->ok) {
                memcpy(buffer->cursor, text, index);
                buffer->cursor += index;
            }
            break;
        case 40:
            if (!frontend_data.option_c9il) GLOBAL_INDEX(frontend_bss.nonil_type_index);
            break;
        case 41:
            if (!frontend_data.option_c9il) {
                available = 1;
                if (buffer->cursor + 4 > buffer->data + buffer->capacity)
                    available = buffer->Grow(4);
                buffer->ok &= available;
                if (buffer->ok) *buffer->cursor++ = 0;
            }
            break;
        default:
            break;
        }
    }
    if (!frontend_data.option_c9il && symbol) {
        if (buffer == &frontend_bss.output_buffers[6])
            buffer->MarkLocation(symbol->references >> 3, 1);
        else if (buffer == &frontend_bss.output_buffers[2])
            frontend_bss.output_buffers[6].MarkGlobal(symbol->references >> 3);
        if (!has_location && opcode == 14) {
            buffer = &frontend_bss.output_buffers[7];
            available = 1;
            if (buffer->cursor + 13 > buffer->data + buffer->capacity)
                available = buffer->Grow(13);
            buffer->ok &= available;
            if (buffer->ok) {
                *(DWORD *)buffer->cursor = emitted_flags;
                *(DWORD *)(buffer->cursor + 4) = recorded_count.raw;
                *(DWORD *)(buffer->cursor + 8) = saved_inline_flags;
                buffer->cursor += 12;
            }
            buffer->MarkLocation(symbol->references >> 3,
                (symbol->symbol_flags & 1) ? 2 : 1);
        }
    }

}

/* Genuine next boundaries, without credit from the caller body. */
extern "C" {
unsigned __cdecl pointer_type_size(WORD); /*1437*/
unsigned __cdecl unshaped_type_size(WORD, CompilerSymbol *); /*1454*/
unsigned __cdecl array_element_size(WORD, CompilerSymbol *, CompilerTypeShape *); /*1522*/
unsigned __cdecl based_shape_size(CompilerTypeShape *); /*b3e12*/
unsigned __cdecl member_pointer_size(CompilerTypeShape *); /*4072a*/
}
extern "C" unsigned __cdecl global_type_size(CompilerType *type)
{
    CompilerTypeShape *shape = type->shape;
    CompilerSymbol *owner;
    int index;
    unsigned bytes, count, element_size;
    if (!shape) {
        index = (short)type->auxiliary;
        if (index < 0) return unshaped_type_size(0x63, NULL);
        if (index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
        owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
        return unshaped_type_size(type->flags, owner);
    }
    if ((shape->flags & 0x77) == 2) {
        if ((type->flags & 0x2fdf) == 0x12)
            element_size = unshaped_type_size(0x63, NULL);
        else {
            index = (short)type->auxiliary;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            owner = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[index];
            element_size = array_element_size(type->flags, owner, shape);
        }
        count = shape->payload.value;
        bytes = element_size * count;
        if ((LONG)bytes < (LONG)element_size && count) {
            compiler_warning(0x94);
            shape->payload.value = 1;
        }
        /* The original returns the previously computed product even after
         * changing the invalid count; it does not recompute the size here. */
        return bytes;
    }
    if (shape->flags & 0x10) return based_shape_size(shape);
    if (shape->flags & 0x20) return member_pointer_size(shape);
    return pointer_type_size(shape->attributes);
}
extern "C" unsigned __cdecl pointer_type_size(WORD) { return 4; }
extern "C" CompilerSymbol *__cdecl instantiate_declaration_class(CompilerSymbol *);
extern "C" void __cdecl merge_declaration_classes(CompilerSymbol *, CompilerSymbol *);
extern "C" unsigned __cdecl unshaped_type_size(WORD flags, CompilerSymbol *symbol)
{
    if (flags & 0x400) {
        CompilerGlobalSymbolView *owner = (CompilerGlobalSymbolView *)symbol;
        if (symbol->kind != 4) {
            int index = (short)owner->type->auxiliary;
            if (index < 0 || index >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            owner = (CompilerGlobalSymbolView *)frontend_data.frontend_pointer_table->items[index];
            return owner->class_detail->size;
        }
        CompilerGlobalClassDetail *detail = owner->class_detail;
        if ((detail->completion_flags & 1) && !(detail->template_flags & 1) &&
            !(owner->definition_flags & 4) && !(detail->completion_flags & 4) &&
            !(owner->scope_flags & 4) && !(frontend_data.source_mode_flags & 0x200020)) {
            int index = frontend_data.declaration_context_index;
            if (index >= 16) { __asm { int 3 } }
            if (index < 0 || frontend_bss.class_scope_rows[index].symbol->name != owner->name) {
                CompilerGlobalSymbolView *instantiated =
                    (CompilerGlobalSymbolView *)instantiate_declaration_class(symbol);
                if (instantiated->class_detail->completion_flags & 8)
                    merge_declaration_classes((CompilerSymbol *)instantiated, symbol);
            }
        }
        if (frontend_data.options.incremental_rebuild &&
            ((ClassDependencyRecordsView *)frontend_data.dependency->records)->record_members)
            frontend_data.dependency->RecordMember(symbol);
        return owner->class_detail->size;
    }
    if (flags & 0x200) return compiler_primitive_type_bytes[3];
    if ((flags & 0x340) || !(flags & ~15U))
        return compiler_primitive_type_bytes[flags & 15];
    return flags & 15;
}
extern "C" unsigned __cdecl array_element_size(WORD flags, CompilerSymbol *symbol,
    CompilerTypeShape *shape)
{
    CompilerTypeShape *element;
    unsigned bytes, count, size;
    for (;;) {
        if (!shape) return unshaped_type_size(flags, symbol);
        switch (shape->flags & 0x77) {
        case 1:
            element = shape->base;
            if (!element) return unshaped_type_size(flags, symbol);
            switch (element->flags & 0x77) {
            case 1: case 3: case 0x40: case 0x44:
                return pointer_type_size(element->attributes);
            case 2:
                size = array_element_size(flags, symbol, element->base);
                return size * element->payload.value;
            case 0x10:
                return based_shape_size(element);
            case 0x20:
                return member_pointer_size(element);
            default:
                return 0;
            }
        case 2:
            element = shape->base;
            if (element) {
                if (element->flags & 1) return pointer_type_size(element->attributes);
                if (element->flags & 0x40) return pointer_type_size(element->attributes);
                if (element->flags & 0x20) return member_pointer_size(element);
                if (element->flags & 0x10) return based_shape_size(element);
            }
            size = array_element_size(flags, symbol, element);
            if (!element) return size;
            count = element->payload.value;
            bytes = size * count;
            if ((LONG)bytes < (LONG)size && count) {
                compiler_warning(0x94);
                element->payload.value = 1;
            }
            return bytes;
        case 3: case 0x40: case 0x44:
            return pointer_type_size(shape->attributes);
        case 4:
            shape = shape->base;
            if (shape && (shape->flags & 1)) return pointer_type_size(shape->attributes);
            break;
        case 0x10:
            return based_shape_size(shape);
        case 0x20:
            return member_pointer_size(shape);
        default:
            return 0;
        }
    }
}
extern "C" unsigned __cdecl based_shape_size(CompilerTypeShape *) { __asm { int 3 } return 0; }
extern "C" unsigned __cdecl member_pointer_size(CompilerTypeShape *) { __asm { int 3 } return 0; }
extern "C" void __cdecl report_incomplete_global_type(CompilerType *type) { __asm { int 3 } }
extern "C" CompilerType *__cdecl get_function_return_type(CompilerType *type)
{
    unsigned index = frontend_data.tagged_type_arena_index;
    if (index >= 9) { __asm { int 3 } }
    CompilerArena *arena = &frontend_bss.arenas[index];
    CompilerType *result = (CompilerType *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    /* Preserve all16 original bytes, including the uninterpreted gap. */
    memcpy(result, type, 16);
    result->shape = type->shape->base;
    return result;
}
extern "C" void __cdecl write_global_type_flags(OutputBuffer *buffer, CompilerGlobalSymbolView *symbol, unsigned flags)
{
    unsigned index;
    int available;
    if (!(symbol->reference_flags & 1)) {
        buffer->WriteIndex(flags);
        return;
    }
    symbol = (CompilerGlobalSymbolView *)resolve_symbol_reference((CompilerSymbol *)symbol);
    flags |= 0x200;
    available = buffer->Ensure(5);
    buffer->ok &= available;
    if (buffer->ok) {
        if ((int)flags > 0x7fff) {
            *(DWORD *)buffer->cursor = (((flags & 0xffffc000U) | 0x4000) << 1) | (flags & 0x7fff);
            buffer->cursor += 4;
        } else {
            *(WORD *)buffer->cursor = (WORD)flags;
            buffer->cursor += 2;
        }
    }
    if (frontend_data.option_c9il) index = symbol->record_index & 0xffffff;
    else if (symbol->references & ~7U) index = symbol->references >> 3;
    else index = external_symbol_index((CompilerSymbol *)symbol);
    if (frontend_data.option_c9il) {
        buffer->WriteIndex(index);
        return;
    }
    available = buffer->Ensure(4);
    buffer->ok &= available;
    if (!buffer->ok) return;
    if (index >= 0x3fff80) {
        *buffer->cursor++ = (BYTE)(index + 0x80);
    } else if (index <= 0x3fff) {
        *buffer->cursor++ = (BYTE)((index & 0x3f) | 0x80);
        *buffer->cursor++ = (BYTE)(index >> 6);
    } else {
        *buffer->cursor++ = (BYTE)(index | 0xc0);
        *buffer->cursor++ = (BYTE)(index >> 6);
        *buffer->cursor++ = (BYTE)(index >> 14);
    }
}
extern "C" {
void __cdecl remember_symbol_reference(CompilerSymbolLink *); /*8db96*/
void __cdecl prepare_debug_type(CompilerType *); /*82ab9*/
unsigned __cdecl finish_class_debug_type(CompilerSymbol *); /*8523a*/
void __cdecl write_global_binding_record(unsigned, CompilerSymbolLink *, unsigned); /*3d69*/
void __cdecl set_declaration_reference(CompilerSymbol *, CompilerSymbol *); /*4877e*/
CompilerExpression *__cdecl symbol_expression(CompilerSymbol *, unsigned); /*47e6*/
}
extern "C" void __cdecl write_global_reference(CompilerSymbolLink *binding)
{
    CompilerGlobalSymbolView *symbol, *target, *owner;
    CompilerClassMemberIterator members;
    CompilerType *type;
    unsigned references, opcode;
    int suppress = 0, defer = 0, omit_name = 0;
    BYTE scope_flags;
    if (frontend_bss.error_count > 0) return;
    symbol = (CompilerGlobalSymbolView *)((BYTE *)binding - 8);
    if (symbol->definition_flags & 8) return;
    if (((CompilerSymbol *)symbol)->SuppressGlobalReference()) return;
    if (symbol->kind == 14) {
        write_global_ir_record(9, (CompilerSymbol *)symbol);
        goto marked;
    }
    if (frontend_data.pch_separate_debug && !frontend_bss.global_output_active &&
        (!((CompilerSymbol *)symbol)->IsNamed() || !(symbol->attributes & 8)))
        defer = 1;
    scope_flags = symbol->scope_flags;
    if ((scope_flags & 2) && !frontend_data.options.pch_no_preprocess &&
        frontend_data.options.pch_use && !frontend_data.options.pch_create)
        suppress = 1;
    if (symbol->kind == 23) {
        if (defer) { symbol->scope_flags |= 2; return; }
        if (frontend_data.options.debug_info && !suppress) {
            opcode = 11;
            goto emit;
        }
        goto marked;
    }
    if (symbol->kind == 4) {
        if (frontend_data.options.debug_info && !(scope_flags & 1) &&
            !(symbol->class_detail->template_flags & 8) && !symbol->class_detail->debug_index)
            symbol->type->debug_cache = finish_class_debug_type((CompilerSymbol *)symbol);
        if (defer && (symbol->definition_flags & 4)) {
            symbol->scope_flags |= 2;
            return;
        }
        if (symbol->type->flags & 0x200) {
            if (!(frontend_data.compiler_flags & 0x20000)) {
                members.Initialize(binding, 2);
                CompilerMemberVariantIterator variants(NULL);
                for (;;) {
                    while (variants.Advance()) write_global_reference(variants.current);
                    if (!members.Next()) break;
                    variants.SetLink(members.entry);
                }
            }
            members.Initialize(binding, 3);
            while (members.current) {
                members.entry = members.current;
                members.current = members.AdvanceChain() ? members.chain : NULL;
                if (members.MatchesMode()) write_global_reference(members.entry);
            }
        }
        if (frontend_data.options.debug_info && (symbol->definition_flags & 4) &&
            !(symbol->scope_flags & 1) && !suppress && !(symbol->class_detail->template_flags & 8)) {
            opcode = 11;
            goto emit;
        }
        goto marked;
    }
    if (symbol->kind == 5) {
        if (!frontend_data.options.debug_info) goto marked;
        type = symbol->type;
        if (!frontend_bss.suppress_parser_codegen) {
            prepare_debug_type(type);
            if ((short)type->auxiliary < 0 || (short)type->auxiliary >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            owner = GLOBAL_TYPE_OWNER(type);
            if (owner && owner->kind == 4 && !owner->class_detail->debug_index)
                finish_class_debug_type((CompilerSymbol *)owner);
        }
        if (defer) { symbol->scope_flags |= 2; return; }
        if (suppress) goto marked;
        opcode = 8;
        goto emit;
    }
    if (defer) {
        remember_symbol_reference(binding);
        if (symbol->kind == 10 && (symbol->storage_class & 7) == 7)
            symbol->scope_flags |= 2;
        return;
    }
    if (symbol->kind == 10 && (symbol->storage_class & 7) == 7 && (scope_flags & 2)) {
        if (!suppress) write_global_ir_record(11, (CompilerSymbol *)symbol);
        goto marked;
    }
    if (symbol->kind >= 26 || frontend_data.symbol_record_bytes[symbol->kind] < 32) { __asm { int 3 } }
    if (symbol->kind == 10 && (symbol->attributes & 0x40)) {
        if (frontend_data.symbol_record_bytes[symbol->kind] < 44) { __asm { int 3 } }
        target = (CompilerGlobalSymbolView *)get_template_symbol((CompilerSymbol *)symbol);
        if (target->kind >= 26 || frontend_data.symbol_record_bytes[target->kind] < 44) { __asm { int 3 } }
        target->attributes = (target->attributes & ~4U) | (symbol->attributes & 4);
        target->qualifiers = (target->qualifiers & 0x7f) | (symbol->qualifiers & 0x80);
        target->alternate_size = symbol->alternate_size;
        if (symbol->reference_flags & 1) {
            owner = (CompilerGlobalSymbolView *)resolve_symbol_reference((CompilerSymbol *)symbol);
            if (owner) set_declaration_reference((CompilerSymbol *)target, (CompilerSymbol *)owner);
            else target->reference_flags &= ~1U;
        }
        target->type = symbol->type;
        if (!frontend_data.options.debug_info) return;
        references = target->references;
        if ((references & 3) < 3) target->references = (references & ~3U) | ((references + 1) & 3);
        target->references = (target->references & ~4U) | ((frontend_data.reference_context & 1) << 2);
        type = symbol->type;
        if (!frontend_bss.suppress_parser_codegen) {
            prepare_debug_type(type);
            if ((short)type->auxiliary < 0 || (short)type->auxiliary >= frontend_data.frontend_pointer_table->count) { __asm { int 3 } }
            owner = GLOBAL_TYPE_OWNER(type);
            if (owner && owner->kind == 4 && !owner->class_detail->debug_index)
                finish_class_debug_type((CompilerSymbol *)owner);
        }
        if (symbol->scope_flags & 1) return;
        opcode = 7;
        goto emit;
    }
    if (symbol->kind == 10 && (symbol->qualifiers & 0x10)) {
        if (!frontend_data.options.debug_info) return;
        remember_symbol_reference(binding);
        target = get_symbol_context(symbol, &owner);
        references = target->references;
        if ((references & 3) < 3) target->references = (references & ~3U) | ((references + 1) & 3);
        target->references = (target->references & ~4U) | ((frontend_data.reference_context & 1) << 2);
        write_global_reference(&((CompilerNamedSymbol *)target)->link);
        opcode = 7;
        goto emit;
    }
    if ((symbol->storage_class & 7) == 1) omit_name = 0;
    else if (((CompilerSymbol *)symbol)->IsFunction()) omit_name = 0;
    else if ((symbol->storage_class & 7) == 3 && (get_function_calling_convention(symbol->type) & 0x100))
        omit_name = 0;
    else if (symbol->kind == 10 && (symbol->storage_class & 7) == 7) omit_name = 0;
    else omit_name = 1;
    if (!(symbol->references & 3)) {
        if (!omit_name) return;
        if (symbol->kind == 11) report_language_diagnostic(4, 0x64, symbol);
        else if (frontend_bss.global_output_active) report_language_diagnostic(3, 0x65, symbol);
    }
    if (symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) {
        if ((symbol->kind == 7 || symbol->kind == 8) && (symbol->linkage_flags & 2)) {
            target = (CompilerGlobalSymbolView *)get_import_symbol((CompilerSymbol *)symbol);
            if (!(target->definition_flags & 8)) write_global_reference(&((CompilerNamedSymbol *)target)->link);
            opcode = 16;
        } else opcode = 4;
    } else {
        if ((symbol->kind == 11 || symbol->kind == 12 || symbol->kind == 22) && (symbol->qualifiers & 4))
            symbol->type = symbol_expression((CompilerSymbol *)symbol, 0)->type;
        opcode = 1;
    }
    if (frontend_data.options.debug_info) remember_symbol_reference(binding);
emit:
    write_global_binding_record(opcode, binding, omit_name);
marked:
    symbol->definition_flags |= 8;
}

extern "C" void __cdecl global_debug_location(unsigned index, int kind, CompilerGlobalLocation **location) { __asm { int 3 } }
extern "C" unsigned __cdecl global_array_element_size(CompilerGlobalSymbolView *symbol) { __asm { int 3 } return 0; }
extern "C" {
char *__cdecl append_decorated_symbol_name(char *, CompilerSymbol *, CompilerSymbol *); /*2b88*/
char *__cdecl append_qualified_symbol_name(char *, CompilerSymbol *); /*8ba21*/
}
extern "C" const char *__cdecl format_symbol_name(CompilerSymbol *symbol, CompilerSymbol *context)
{
    char *end;
    int length;
    if (symbol->kind == 21) return ((CompilerNamedSymbol *)symbol)->link.name->name;
    frontend_bss.name_format_state = 0;
    frontend_bss.name_format_reset_values[1] = 0;
    frontend_bss.name_format_reset_values[0] = 0;
    frontend_bss.symbol_name_cursor = frontend_bss.symbol_name_buffer;
    if (symbol->kind == 4 || symbol->kind == 23)
        end = append_qualified_symbol_name(frontend_bss.symbol_name_buffer, symbol);
    else end = append_decorated_symbol_name(frontend_bss.symbol_name_buffer, symbol, context);
    length = (int)(end - frontend_bss.symbol_name_buffer);
    if (length >= 2048) {
        report_language_diagnostic(1, 0x1f7, 0x53, symbol);
        length = 2047;
    }
    if (length < 0) { __asm { int 3 } }
    frontend_bss.symbol_name_buffer[length] = 0;
    return frontend_bss.symbol_name_buffer;
}
extern "C" {
unsigned __cdecl symbol_decoration_kind(CompilerSymbol *); /*2ce1*/
void __cdecl compute_parameter_bytes(CompilerSymbol *); /*33e67*/
int __cdecl format_compiler_text(char *, const char *, unsigned, ...); /*33ed7*/
char *__cdecl append_encoded_identifier(char *, CompilerNameEntry *, unsigned); /*1e92d*/
char *__cdecl append_symbol_name_code(char *, CompilerSymbol *, unsigned); /*1e9c3*/
char *__cdecl append_symbol_owner_name(char *, CompilerSymbol *); /*1ea01*/
char *__cdecl append_symbol_type_code(char *, CompilerSymbol *, CompilerSymbol *, unsigned); /*1eb18*/
}
/* Only the actual2048-byte workspace is supported. Each direct write checks
 * that owner before mutation; other input spans remain unsupported. */
#define NAME_PUT(ch) do { \
    if ((unsigned long)out < (unsigned long)frontend_bss.symbol_name_buffer || \
        (unsigned long)out >= (unsigned long)(frontend_bss.symbol_name_buffer + 2048)) { __asm { int 3 } } \
    *out++ = (char)(ch); \
} while (0)
extern "C" char *__cdecl append_decorated_symbol_name(char *buffer, CompilerSymbol *input, CompilerSymbol *context)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    CompilerGlobalSymbolView *owner;
    char *out = buffer;
    const char *name;
    unsigned kind, sequence;
    int saved_mode = frontend_data.name_format_mode;
    int finished;
    WORD convention;
    frontend_data.name_format_mode = 0;
    kind = symbol_decoration_kind(input);
    if (frontend_bss.error_count || !kind) {
        name = symbol->name->name;
        if (kind && frontend_bss.error_count) {
            sequence = frontend_data.name_format_sequence++;
            out += format_compiler_text(out, " %s %d", ~0U, name, (int)sequence);
            goto finished_name;
        }
        if (saved_mode) {
            for (;;) {
                char character = *name++;
                NAME_PUT(character);
                if (!character) break;
            }
            --out;
        } else {
            NAME_PUT('?');
            out = append_encoded_identifier(out, symbol->name, 0);
            NAME_PUT('@'); NAME_PUT('9');
        }
        if (symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) {
            convention = symbol->type->attributes & 0x70;
            if (convention == 0x60 || convention == 0x40) {
                if ((symbol->type_flags & 2) ||
                    ((symbol->name == frontend_data.builtin_names.main_entry ||
                      symbol->name == frontend_data.builtin_names.winmain_entry ||
                      symbol->name == frontend_data.builtin_names.dllmain_entry) &&
                     !symbol->declaration_qualifiers)) {
                    if (!(symbol->qualifiers & 0x40)) compute_parameter_bytes(input);
                    out += format_compiler_text(out, "@%d", ~0U, (unsigned)(WORD)symbol->alternate_size);
                }
            }
        }
        goto finished_name;
    }
    owner = context ? (CompilerGlobalSymbolView *)context : symbol;
    NAME_PUT('?');
    if (kind == 0x24 || kind == 0x25 || kind == 0x26) {
        name = owner->name->name + 1;
        if (kind == 0x24) {
            owner->type_flags &= ~3U;
            owner->storage_class = (owner->storage_class & ~7U) ^ 3;
            while (*name && *name != '@') NAME_PUT(*name++);
            NAME_PUT('@');
            goto append_owner;
        }
        finished = 0;
        while (*name && !finished) {
            if (*name == '@' && out[-1] == '@' && name[1] != '@') {
                NAME_PUT('@'); ++name;
                NAME_PUT('$'); NAME_PUT(kind == 0x25 ? 'D' : 'E');
                finished = 1;
            } else NAME_PUT(*name++);
        }
        goto finished_name;
    }
    out = append_symbol_name_code(out, input, kind);
append_owner:
    out = append_symbol_owner_name(out, (CompilerSymbol *)owner);
    NAME_PUT('@');
    if (kind == 0x24) {
        out = append_symbol_type_code(out, (CompilerSymbol *)owner, input, kind);
        owner->type_flags = (owner->type_flags & ~1U) | 2;
        owner->storage_class = (owner->storage_class & ~7U) ^ 1;
    } else out = append_symbol_type_code(out, input, NULL, kind);
finished_name:
    frontend_data.name_format_mode = saved_mode;
    return out;
}
#undef NAME_PUT
/* Low byte selects the name form; the optional high byte carries the
 * operator/special-name code. The sparse operator switch replaces only
 * this function's actual compressed dispatch relations. */
extern "C" unsigned __cdecl symbol_decoration_kind(CompilerSymbol *input)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    CompilerNameEntry *name;
    CompilerTypeShape *shape;
    unsigned group = 0, code = 0, access, linkage, operation;
    BYTE flags = symbol->type_flags;
    if (flags & 2) return 0;
    name = symbol->name;
    if (symbol->type && (shape = symbol->type->shape) != NULL &&
        (shape->flags & 0x77) == 4 && !shape->payload.function &&
        name != frontend_data.builtin_names.local_static_destructor) return 0;
    if ((name == frontend_data.builtin_names.main_entry ||
         name == frontend_data.builtin_names.winmain_entry ||
         name == frontend_data.builtin_names.dllmain_entry) &&
        !symbol->declaration_qualifiers &&
        (symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) &&
        (symbol->storage_class & 7) != 3) return 0;
    access = symbol->storage_class & 7;
    if (access == 7) return 0;
    if (symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) {
        linkage = symbol->linkage_flags & 0xc;
        if (linkage == 4) group = 6;
        else if (linkage == 8) group = 0x1a;
        if (flags & 0x10) { code = 0x30; goto function_form; }
        if (flags & 0x20) { code = 0x31; goto function_form; }
        if (flags & 0x40) { code = 0x42; goto function_form; }
        if (linkage == 0xc) { group = 0x39; code = 0xb9; goto function_form; }
        if (name == frontend_data.builtin_names.default_constructor_closure) code = 0xc6;
        else if (name == frontend_data.builtin_names.local_vftable_constructor_closure) code = 0xd4;
        else if (name == frontend_data.builtin_names.copy_constructor_closure) code = 0xcf;
        else if (name == frontend_data.builtin_names.vector_constructor) code = 0xc8;
        else if (name == frontend_data.builtin_names.vector_vbase_constructor) code = 0xca;
        else if (name == frontend_data.builtin_names.vector_destructor) code = 0xc9;
        else if (name == frontend_data.builtin_names.eh_vector_constructor) code = 0xcc;
        else if (name == frontend_data.builtin_names.eh_vector_vbase_constructor) code = 0xce;
        else if (name == frontend_data.builtin_names.eh_vector_destructor) code = 0xcd;
        else if (name == frontend_data.builtin_names.vbase_destructor) code = 0xc4;
        else if (name == frontend_data.builtin_names.deleting_destructor) code = 0xc7;
        else if (name == frontend_data.builtin_names.vector_deleting_destructor) code = 0xc5;
        else if (symbol->function_class != 0 && symbol->function_class != 1) {
            operation = symbol->function_class;
            switch (operation) {
            case 49: code = 0x49; break; /* & */
            case 50: code = 0x56; break; /* && */
            case 51: code = 0xb4; break; /* &= */
            case 52: code = 0x34; break; /* = */
            case 55: code = 0x51; break; /* comma */
            case 56: case 84: code = 0x46; break; /* -- */
            case 57: code = 0x4b; break; /* / */
            case 58: code = 0xb0; break; /* /= */
            case 59: code = 0x38; break; /* == */
            case 60: code = 0x37; break; /* ! */
            case 61: case 74: code = 0x44; break; /* * */
            case 62: code = 0x4f; break; /* > */
            case 63: code = 0x50; break; /* >= */
            case 64: case 85: code = 0x45; break; /* ++ */
            case 65: code = 0x41; break; /* [] */
            case 66: code = 0xb3; break; /* <<= */
            case 67: code = 0x36; break; /* << */
            case 68: code = 0x4d; break; /* < */
            case 69: code = 0x4e; break; /* <= */
            case 70: case 90: code = 0x47; break; /* - */
            case 71: code = 0x5a; break; /* -= */
            case 72: code = 0x4c; break; /* % */
            case 73: code = 0xb1; break; /* %= */
            case 75: code = 0x58; break; /* *= */
            case 76: code = 0x39; break; /* != */
            case 77: code = 0x55; break; /* | */
            case 78: code = 0xb5; break; /* |= */
            case 79: code = 0x57; break; /* || */
            case 81: case 134: code = 0x48; break; /* + */
            case 82: code = 0x59; break; /* += */
            case 83: code = 0x43; break; /* -> */
            case 87: code = 0xb2; break; /* >>= */
            case 88: code = 0x35; break; /* >> */
            case 89: code = 0x53; break; /* ~ */
            case 91: code = 0x54; break; /* ^ */
            case 92: code = 0xb6; break; /* ^= */
            case 118: code = 0x52; break; /* () */
            case 140: code = name == frontend_data.builtin_names.operator_new_array ? 0xd5 : 0x32; break;
            case 141: code = name == frontend_data.builtin_names.operator_delete_array ? 0xd6 : 0x33; break;
            case 147: code = 0x4a; break; /* ->* */
            default: code = 0xc0; break;
            }
        } else if (name == frontend_data.builtin_names.local_static_destructor) group = 0x24;
        else if (name == frontend_data.builtin_names.template_static_constructor) group = 0x25;
        else if (name == frontend_data.builtin_names.template_static_destructor) group = 0x26;
function_form:
        if (group == 0x1a) {
            if ((symbol->definition_flags & 3) == 2) group = 0x1c;
            else if ((symbol->definition_flags & 3) == 3) group = 0x1e;
        } else if (group == 6 || group == 0) {
            if (!group) {
                if (!(flags & 4)) { group = 0x18; goto qualified_form; }
                if (symbol->symbol_flags & 2) group = 4;
                else group = ((~symbol->builtin_flags) & 1) << 1;
            }
            if ((symbol->definition_flags & 3) == 2) group |= 8;
            else if ((symbol->definition_flags & 3) == 3) group |= 0x10;
        }
qualified_form:
        if (group != 0x24 && group != 0x25 && group != 0x26) {
            if (group != 0x39) group += 0x41;
            if ((symbol->reference_flags & 1) && (group == 0x39 || !(symbol->reference_flags & 2)))
                group |= 0x80;
        }
        goto packed_form;
    }
    if (name == frontend_data.builtin_names.vftable) { code = 0xb7; group = 0x36; }
    else if (name == frontend_data.builtin_names.local_vftable) { code = 0xd3; group = 0x36; }
    else if (name == frontend_data.builtin_names.vbtable) { code = 0xb8; group = 0x37; }
    else if (name == frontend_data.builtin_names.vdispmap) { code = 0xcb; group = 0x3a; }
    else if (symbol->kind == 3) {
        if (access == 3) {
            group = 0x30;
            if ((symbol->definition_flags & 3) == 2) group = 0x31;
            else if ((symbol->definition_flags & 3) == 3) group = 0x32;
        }
    } else if (name == frontend_data.builtin_names.local_static_guard || symbol->kind == 10) {
        if (name != frontend_data.builtin_names.local_static_guard &&
            symbol->kind == 10 && access >= 4 && access <= 6) return 0;
        if (access != 3) group = 0x33;
        else {
            if (!symbol->declaration_scope_number) return 0;
            if (name == frontend_data.builtin_names.local_static_guard) { code = 0xc2; group = 0x35; }
            else group = 0x34;
        }
    } else return 0;
packed_form:
    return group | (code << 8);
}
extern "C" void __cdecl compute_parameter_bytes(CompilerSymbol *) { __asm { int 3 } }
extern "C" char *__cdecl append_encoded_identifier(char *, CompilerNameEntry *, unsigned) { __asm { int 3 } return NULL; }
extern "C" char *__cdecl append_symbol_name_code(char *, CompilerSymbol *, unsigned) { __asm { int 3 } return NULL; }
extern "C" char *__cdecl append_symbol_owner_name(char *, CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" char *__cdecl append_symbol_type_code(char *, CompilerSymbol *, CompilerSymbol *, unsigned) { __asm { int 3 } return NULL; }
extern "C" char *__cdecl append_qualified_symbol_name(char *, CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" int __cdecl is_external_symbol(CompilerGlobalSymbolView *symbol) { __asm { int 3 } return 0; }
extern "C" const char *__cdecl lookup_global_symbol_name(CompilerGlobalSymbolView *symbol) { __asm { int 3 } return NULL; }
extern "C" unsigned __cdecl get_symbol_type_debug_index(CompilerGlobalSymbolView *symbol) { __asm { int 3 } return 0; }
extern "C" CompilerTypeShape *__cdecl find_function_type_shape(CompilerType *);
extern "C" unsigned __cdecl get_function_calling_convention(CompilerType *type)
{
    CompilerTypeShape *function = find_function_type_shape(type);
    return (function ? function->attributes : type->attributes) & 0x1f00;
}
extern "C" CompilerTypeShape *__cdecl skip_array_function_shapes(CompilerTypeShape *);
extern "C" CompilerTypeShape *__cdecl find_function_type_shape(CompilerType *type)
{
    return skip_array_function_shapes(type->shape);
}
extern "C" CompilerTypeShape *__cdecl skip_array_function_shapes(CompilerTypeShape *shape)
{
    while (shape) {
        unsigned kind = shape->flags & 0x77;
        if (kind != 2 && kind != 4) break;
        shape = shape->base;
    }
    return shape;
}
extern "C" int __cdecl is_extended_pointer_type(CompilerType *type) { __asm { int 3 } return 0; }
extern "C" CompilerGlobalSymbolView *__cdecl get_symbol_context(CompilerGlobalSymbolView *symbol, CompilerGlobalSymbolView **owner) { __asm { int 3 } return NULL; }
unsigned RECON_THISCALL CompilerType::GetCallingConvention(void) { __asm { int 3 } return 0; }
unsigned RECON_THISCALL CompilerSourceFile::OutputIndex(void)
{
    if (frontend_data.option_c9il) return record_index & 0xffffff;
    if (!(reference_flags & ~7U)) return AssignReference();
    return reference_flags >> 3;
}
unsigned RECON_THISCALL CompilerSourceFile::AssignReference(void) { __asm { int 3 } return 0; }
const char *RECON_THISCALL FrontendContextStack::FileName(void)
{
    /* Native supports the ordinary valid-owner/NULL paths. The genuine
     * Win32 fault fallback remains expressed as SEH, with no native fault
     * interception claim. Both NULL and fault return the shared empty string. */
    __try {
        return origin ? ((CompilerSourceFile *)origin)->name->name : compiler_empty_string;
    } __except (1) {
        return compiler_empty_string;
    }
}
void RECON_THISCALL OutputBuffer::MarkLocation(unsigned index, unsigned kind) { __asm { int 3 } }
void RECON_THISCALL OutputBuffer::MarkGlobal(unsigned index) { __asm { int 3 } }
void RECON_THISCALL OutputBuffer::WriteCString(const char *text)
{
    unsigned bytes = strlen(text) + 1;
    unsigned needed = bytes + 1;
    int available = 0;
    /* The original reserves one byte beyond the NUL it copies. Grow may
     * relocate this buffer and update ok, so read both again after the call. */
    if ((int)needed >= 0) {
        available = 1;
        if (cursor + needed > data + capacity)
            available = Grow((int)needed) != 0;
    }
    ok &= available;
    if (ok) {
        memcpy(cursor, text, bytes);
        cursor += bytes;
    }
}

extern "C" const char *__cdecl format_global_debug_name(CompilerSymbol *symbol, char *text, unsigned capacity) { __asm { int 3 } return NULL; }

extern "C" void __cdecl remember_symbol_reference(CompilerSymbolLink *) { __asm { int 3 } }
extern "C" void __cdecl prepare_debug_type(CompilerType *) { __asm { int 3 } }
extern "C" unsigned __cdecl finish_class_debug_type(CompilerSymbol *) { __asm { int 3 } return 0; }
extern "C" void __cdecl write_global_binding_record(unsigned opcode, CompilerSymbolLink *binding, unsigned omit_names)
{
    unsigned saved = frontend_data.omit_external_names;
    frontend_data.omit_external_names = omit_names;
    write_global_ir_record(opcode, (CompilerSymbol *)((BYTE *)binding - 8));
    frontend_data.omit_external_names = saved;
}
int RECON_THISCALL CompilerSymbol::SuppressGlobalReference(void)
{
    return kind == 16 || kind == 17 || kind == 25;
}
int RECON_THISCALL CompilerSymbol::IsNamed(void) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerClassMemberIterator::AdvanceChain(void) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerClassMemberIterator::MatchesMode(void) { __asm { int 3 } return 0; }

extern "C" int __cdecl can_defer_global_reference(CompilerSymbol *); /*33e04*/
void RECON_THISCALL CompilerScope::Check(void)
{
    CompilerSymbolLink *link, *next, *deferred = NULL, *previous;
    CompilerSymbol *symbol;
    unsigned index;
    int changed;
    if (frontend_bss.error_count) return;
    frontend_bss.global_output_active = level == 0xfb ? 0 : level;
    if (level) write_global_ir_record(13, NULL);
    if (buckets && bucket_mask + 1) {
        index = 0;
        do {
            link = buckets[index];
            while (link) {
                symbol = (CompilerSymbol *)((BYTE *)link - 8);
                if (symbol->kind == 4 &&
                    (((CompilerGlobalSymbolView *)symbol)->class_detail->completion_flags & 2)) {
                    /* Already completed class; no reference record here. */
                } else if (symbol->kind == 16 || symbol->kind == 17) {
                    CheckGroup(link);
                } else if (symbol->kind == 24) {
                    /* Alias entries have no independent record. */
                } else if (symbol->kind == 23) {
                    ((CompilerNamespaceSymbol *)symbol)->data->scope->Check();
                } else if (frontend_bss.global_output_active || frontend_data.pch_separate_debug ||
                           !can_defer_global_reference(symbol)) {
                    write_global_reference(link);
                } else {
                    next = link->next;
                    link->next = deferred;
                    deferred = link;
                    link = next;
                    continue;
                }
                link = link->next;
            }
            ++index;
        } while (index < bucket_mask + 1);
    }
    changed = 1;
    while (deferred && changed) {
        previous = NULL;
        changed = 0;
        link = deferred;
        while (link) {
            symbol = (CompilerSymbol *)((BYTE *)link - 8);
            if (can_defer_global_reference(symbol)) {
                previous = link;
            } else {
                write_global_reference(link);
                changed = 1;
                if (previous) previous->next = link->next;
                else deferred = link->next;
            }
            link = link->next;
        }
    }
}
extern "C" int __cdecl can_defer_global_reference(CompilerSymbol *input)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    if (symbol->kind == 4 || symbol->kind == 5)
        return (symbol->definition_flags & 8) && !(symbol->scope_flags & 2);
    if (symbol->kind == 10 && (symbol->storage_class & 7) == 7) {
        CompilerGlobalSymbolView *owner = GLOBAL_TYPE_OWNER(symbol->type);
        if (owner && !(symbol->scope_flags & 2) &&
            can_defer_global_reference((CompilerSymbol *)owner)) return 1;
    }
    return 0;
}
void RECON_THISCALL CompilerScope::CheckGroup(CompilerSymbolLink *) { __asm { int 3 } }
