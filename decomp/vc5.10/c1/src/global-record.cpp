#include <string.h>
#include "frontend.h"
#include "keywords.h"
#include "global-record.h"
#pragma intrinsic(strlen, memcpy)
#include <recon.h>

/* Source-only contextual cells for currently unsupported domains. They are
 * UNMODELED native representation, excluded from defaults/snapshot/equality.
 * The entry guard precedes every native read or effect involving them. */
static struct {
    const char *global_option_string;       /* RVA5f320 */
    int include_pch_reference_counts;       /* RVA5f33c */
    int disable_inline_records;            /* RVA5f378 */
    int omit_external_names;               /* RVA5f4d0 */
    CompilerGlobalOption *global_options;   /* RVA5e578 */
    unsigned pch_reference_count;           /* RVA5e9d8 */
    unsigned global_name_count;             /* RVA5e9e0 */
} writer_unmodeled_data;
static struct {
    const char *global_string_record;       /* RVA54018 */
    unsigned nonil_type_index;              /* RVA54c80 */
    int inline_record_flag;                 /* RVA54d98 */
    unsigned local_reference_count;         /* RVA54f34 */
    short global_record_option_word;        /* RVA54fd0, low WORD only */
    unsigned saved_reference_count;         /* RVA550fc */
} writer_unmodeled_bss;

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
#define GLOBAL_RESERVE(n) (available = buffer->Ensure(n), buffer->good &= available)
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
CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *);
CompilerIrSymbol *__cdecl resolve_symbol_alias(CompilerIrSymbol *);
unsigned __cdecl assign_symbol_output_index(CompilerIrSymbol *);
unsigned __cdecl global_type_size(CompilerType *);
void __cdecl report_incomplete_global_type(CompilerType *);
CompilerType *__cdecl get_function_return_type(CompilerType *);
void __cdecl write_global_type_flags(CompilerOutputBuffer *, CompilerIrSymbol *, unsigned);
void __cdecl write_global_reference(CompilerSymbolBinding *);
void __cdecl global_debug_location(unsigned, int, CompilerGlobalLocation **);
unsigned __cdecl global_array_element_size(CompilerIrSymbol *);
const char *__cdecl global_symbol_name(CompilerIrSymbol *);
void __cdecl compiler_fatal(int);
void __cdecl compiler_warning(int, ...);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
}

extern "C" void __cdecl write_global_record(unsigned opcode, CompilerIrSymbol *symbol)
{
    CompilerOutputBuffer *buffer;
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
    const char *text;
#ifdef RECON_NATIVE_TEST
    /* The current input establishes only the NULL global-header record.
     * Symbol owners and non-IL state require their own reached evidence. */
    if (symbol || opcode != 17 || !c1_data.c9_il)
        recon_unsupported("global record: unmodeled symbol or non-IL domain");
#endif
    if (c1_data.report_statistics)
        print_compiler_statistics();
    if (symbol) {
        if (symbol->kind >= 1 && symbol->kind <= 14 && symbol->kind != 5 &&
            (symbol->scope_bits & 1))
            return;
        type = symbol->value_type;
        type_flags = type->flags;
        attributes = type->attributes;
        shape = type->shape;
        if (symbol->kind != 5) {
            if ((symbol->storage_class & 7) == 7) opcode = 11;
            else {
                if (shape && (shape->flags & 7) == 2) {
                    array = 1;
                    if (opcode == 1) opcode = 2;
                }
                if (shape && (shape->flags & 7) == 4)
                    return_type = get_function_return_type(type);
                if (symbol->function_flags & 1) {
                    if (lookup_ir_symbol(symbol)->kind == 14 &&
                        !(lookup_ir_symbol(symbol)->flags & 2) &&
                        !c1_bss.declaration_scope_active)
                        write_global_reference(&lookup_ir_symbol(symbol)->binding);
                }
            }
            if (opcode == 11 && !c1_data.options.debug_info) return;
        }
    } else {
        type = NULL; shape = NULL; type_flags = attributes = 0;
    }
    format = (const signed char *)global_record_formats[opcode];
    if (opcode == 18 || (!c1_bss.declaration_scope_active && c1_data.c9_il))
        buffer = &c1_bss.output_buffers[2];
    else if (c1_bss.declaration_scope_active)
        buffer = &c1_bss.output_buffers[1];
    else {
        switch (opcode) {
        case 1: case 2:
            buffer = ((symbol->reference_flags & 4) || (symbol->flags & 1)) ?
                &c1_bss.output_buffers[2] : &c1_bss.output_buffers[6]; break;
        case 4: case 16:
            buffer = (symbol->reference_flags & 4) ?
                &c1_bss.output_buffers[2] : &c1_bss.output_buffers[6]; break;
        case 11: buffer = &c1_bss.output_buffers[2]; break;
        default: buffer = &c1_bss.output_buffers[2]; break;
        }
    }
    if (c1_data.emit_global_locations && opcode == 14) {
        has_location = 1;
        unsigned location_index;
        if (c1_data.c9_il) location_index = symbol->record_id & 0xffffff;
        else if (symbol->reference_flags & ~7U) location_index = symbol->reference_flags >> 3;
        else location_index = assign_symbol_output_index(symbol);
        global_debug_location(location_index, 5, &location);
        saved_location = *location;
        emitted_flags = saved_location.first;
        recorded_count.raw = saved_location.second;
        saved_inline_flags = saved_location.third;
    }
    if ((opcode == 14 || opcode == 1 || opcode == 2) &&
        buffer == &c1_bss.output_buffers[2] && !writer_unmodeled_data.omit_external_names &&
        symbol && symbol->binding.name && symbol->binding.name->name) {
        int valid = c1_data.c9_il || c1_bss.debug_module->Slot74(symbol->binding.name->name);
        if (!valid) compiler_fatal(55);
    }
    while (*format) {
        switch (*format++) {
        case 1:
            index = (symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) ?
                global_type_size(return_type) : global_type_size(type);
            if (!index && (symbol->kind == 11 || symbol->kind == 12))
                report_incomplete_global_type(type);
            if ((symbol->declaration_flags & 4) && (type_flags & 0x400) &&
                (type->symbol_owner->qualifier_flags & 1) && !shape)
                index += symbol->symbol_value;
            GLOBAL_INDEX(index);
            break;
        case 2:
            if (has_location) {
                write_global_type_flags(buffer, symbol, emitted_flags);
                break;
            }
            qualifiers = (symbol->qualifier_flags & 0x80) &&
                symbol->kind != 6 && symbol->kind != 7 && symbol->kind != 8 ? 0x80 : 0;
            if (symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) {
                if (symbol->qualifier_flags & 2) qualifiers |= 0x100;
                if (symbol->linkage_flags & 1) qualifiers |= 0x400;
            }
            if (symbol->reference_flags & 3) {
                if ((symbol->kind == 10 || symbol->kind == 11) &&
                    (symbol->qualifier_flags & 8)) qualifiers |= 0x2000;
                if (symbol->kind != 6 && symbol->kind != 7 && symbol->kind != 8)
                    qualifiers |= 0x100;
            }
            if (symbol->declaration_flags & 8) qualifiers |= 0x400;
            switch (attributes & 0x70) {
            case 0x40: qualifiers |= 4; break;
            case 0x50: qualifiers |= 0x18; break;
            case 0x60:
                if (!shape || (shape->flags & 7) != 4 || !(shape->flags & 0x8000))
                    qualifiers |= 0x10;
                break;
            case 0x70: qualifiers |= 0x14; break;
            }
            if (symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) {
                if (symbol->qualifier_flags & 0x10) qualifiers |= 0x8000;
                if (symbol->qualifier_flags & 1) qualifiers |= 0x1000;
            }
            if ((symbol->linkage_flags & 2) && symbol->kind != 7 && symbol->kind != 8)
                qualifiers |= 0x1000;
            if (symbol->kind == 10 && (symbol->symbol_qualifiers & 0x80)) qualifiers |= 0x40;
            if ((symbol->storage_class & 0x40) &&
                (opcode == 14 || (symbol->declaration_flags & 4) || !(attributes & 3)) &&
                !(symbol->linkage_flags & 8)) qualifiers |= 0x20;
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
            if (c1_data.c9_il) index = symbol->record_id & 0xffffff;
            else if (symbol->reference_flags & ~7U) index = symbol->reference_flags >> 3;
            else index = assign_symbol_output_index(symbol);
            if (c1_data.c9_il) buffer->WriteIndex(index);
            else GLOBAL_COMPACT(index);
            break;
        case 6:
            index = 0;
            if (c1_data.options.debug_info) {
                if (symbol->kind >= 1 && symbol->kind <= 14 && symbol->kind != 5 &&
                    (symbol->function_flags & 1) &&
                    lookup_ir_symbol(symbol)->binding.name == c1_data.builtin_names.directive_section) {
                    GLOBAL_INDEX(0);
                    break;
                }
                if (symbol->kind == 4) index = symbol->alternate_size;
                else if (type->symbol_owner && !type->shape && type->symbol_owner->kind == 4)
                    index = type->symbol_owner->alternate_size;
                if (!index) index = type->debug_index;
            }
            GLOBAL_INDEX(index);
            break;
        case 7:
            GLOBAL_BYTE(opcode == 3 ? symbol->parameter_kind : c1_bss.declaration_scope_active);
            break;
        case 8:
            if (symbol->kind == 9) index = symbol->symbol_value & 0xff;
            else if (symbol->declaration_flags & 8) index = (symbol->storage_class & 7) == 2 ? 0 : 0x25;
            else if (symbol->kind == 10 && (symbol->storage_class & 7) == 3) index = 0x24;
            else index = 0;
            GLOBAL_BYTE(index);
            break;
        case 9:
            if (!symbol->binding.name || symbol->binding.name == c1_data.builtin_names.unnamed) {
                GLOBAL_ZERO();
                break;
            }
            text = global_symbol_name(symbol);
            index = strlen(text) + 1;
            if (buffer->Reserve(index)) buffer->WriteBytes(text, index);
            break;
        case 10:
            GLOBAL_BYTE((symbol->qualifier_flags & 4) ? 2 : 1);
            break;
        case 11: case 32:
            GLOBAL_ZERO();
            break;
        case 12:
            if (symbol->kind == 11 || symbol->kind == 12) index = 3;
            else if ((symbol->storage_class & 7) >= 1 && (symbol->storage_class & 7) <= 3) {
                unsigned convention = type->GetCallingConvention();
                if (convention & 0x100) {
                    if ((symbol->function_flags & 1) && lookup_ir_symbol(symbol)->kind == 14 &&
                        (lookup_ir_symbol(symbol)->record_id & 0xffffff) != 3)
                        index = 2;
                    else index = 4;
                } else if ((symbol->declaration_flags & 8) && (symbol->storage_class & 0x40) &&
                    c1_data.options.readonly_strings &&
                    (!(symbol->function_flags & 1) || lookup_ir_symbol(symbol)->kind != 14 ||
                     (lookup_ir_symbol(symbol)->record_id & 0xffffff) == 3)) index = 4;
                else index = 2;
            } else index = 1;
            GLOBAL_BYTE(index);
            break;
        case 13: {
            unsigned shape_flags;
            BYTE storage = 0;
            if (opcode == 4 || opcode == 16 || opcode == 14)
                shape_flags = return_type->shape ? return_type->shape->flags : 0;
            else {
                shape_flags = shape ? shape->flags : 0;
                if ((type_flags & 0x400) && (type_flags == 0x404 ||
                    (type->symbol_owner->unmodeled_2d_flags & 1))) storage = 0x10;
            }
            if ((shape_flags & 7) == 2) storage |= 6;
            else if (shape_flags & 0xb) storage |= 3;
            else if (type_flags & 0x4000) storage |= 2;
            else {
                switch (type_flags & 0xffff) {
                case 0x41: case 0x61:
                    storage |= (c1_data.options.language_extensions ||
                        !c1_data.options.unsigned_char || (type_flags & 0x8000)) ? 1 : 2;
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
            index = symbol->reference_flags & 3;
            if (GLOBAL_RESERVE(4) && !buffer->WriteSmall((short)index)) {
                *buffer->cursor++ = 0x80;
                *(WORD *)buffer->cursor = (WORD)index;
                buffer->cursor += 2;
            }
            break;
        case 15:
            if (symbol->alternate_name && symbol->alternate_name != c1_data.builtin_names.unnamed) {
                text = symbol->alternate_name->name;
                index = strlen(text) + 1;
                if (GLOBAL_RESERVE(index + 1)) {
                    memcpy(buffer->cursor, text, index);
                    buffer->cursor += index;
                }
            } else GLOBAL_ZERO();
            break;
        case 16:
            if ((symbol->linkage_flags & 4) && opcode != 7) {
                if (!(symbol->flags & 1)) index = 2;
                else index = (symbol->declaration_flags & 4) ? 7 : 9;
            } else if ((symbol->linkage_flags & 8) && opcode != 7) index = 8;
            else if (symbol->storage_class & 0x40) index = ((~symbol->declaration_flags & 4) | 2) >> 1;
            else if ((symbol->storage_class & 7) == 3) index = 4;
            else if ((symbol->storage_class & 7) >= 4 && (symbol->storage_class & 7) <= 6) index = 4;
            else if (symbol->declaration_flags & 4) index = 1;
            else if ((symbol->storage_class & 7) == 1) index = 2;
            else if (array && !shape->value) index = 2;
            else index = 3;
            GLOBAL_BYTE(index);
            break;
        case 17:
            if ((symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) &&
                (symbol->flags & 1)) {
                if (symbol->linkage_flags & 4) index = 9;
                else if (symbol->linkage_flags & 8) index = 10;
                else if (symbol->storage_class & 0x40) index = 5;
                else index = (symbol->storage_class & 7) == 3 ? 3 : 5;
            } else {
                if ((symbol->storage_class & 7) == 3 &&
                    !(c1_data.options.incremental_compile && !c1_data.options.debug_types)) {
                    compiler_warning(129, symbol);
                    GLOBAL_BYTE(2);
                    break;
                }
                if ((symbol->symbol_qualifiers & 1) && (symbol->storage_class & 7) != 1)
                    report_language_diagnostic(1, 506, symbol);
                index = (symbol->linkage_flags & 8) ? 10 : 4;
            }
            GLOBAL_BYTE(index);
            break;
        case 19:
            GLOBAL_INDEX(0);
            break;
        case 20: {
            int value = symbol->integer_value;
            int width = value >= -32768 && value <= 32767 ? 2 : 4;
            if (GLOBAL_RESERVE(4) && !buffer->WriteSmall(width)) {
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
        case 21: case 22: {
            CompilerIrSymbol *target = format[-1] == 21 ?
                resolve_symbol_alias(symbol) : lookup_ir_symbol(symbol);
            if (c1_data.c9_il) index = target->record_id & 0xffffff;
            else if (target->reference_flags & ~7U) index = target->reference_flags >> 3;
            else index = assign_symbol_output_index(target);
            if (c1_data.c9_il) {
                if (buffer->Reserve(4)) {
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
            if (c1_data.c9_il) {
                GLOBAL_INDEX(writer_unmodeled_bss.saved_reference_count);
                index = writer_unmodeled_bss.local_reference_count +
                    (writer_unmodeled_data.include_pch_reference_counts ? writer_unmodeled_data.pch_reference_count : 0);
                GLOBAL_INDEX(index);
            }
            break;
        case 25:
            if (!has_location) recorded_count.count = c1_bss.ir_record_count > 0xffff ?
                0xffff : (WORD)c1_bss.ir_record_count;
            if (GLOBAL_RESERVE(4)) {
                if ((unsigned)(int)(short)recorded_count.count & ~0x7fU) {
                    *buffer->cursor++ = 0x80;
                    *(WORD *)buffer->cursor = recorded_count.count;
                    buffer->cursor += 2;
                } else *buffer->cursor++ = (BYTE)recorded_count.count;
            }
            break;
        case 26:
            if (has_location) index = saved_inline_flags;
            else {
                index = ((symbol->qualifier_flags >> 5) & 4) |
                    (writer_unmodeled_bss.inline_record_flag ? 0x20 : 0) |
                    (c1_data.ir_symbol_state_changed ? 8 : 0) |
                    (writer_unmodeled_data.include_pch_reference_counts ? 1 : 0) |
                    ((symbol->symbol_qualifiers & 1) << 7);
                if ((type->shape->flags & 0x800) && !(type->shape->flags & 0x4000))
                    writer_unmodeled_data.disable_inline_records = 1;
                int choose_inline = c1_data.options.inline_level == 1;
                if (c1_data.options.inline_level == 2) {
                    if (!c1_data.ir_symbol_state_ready) choose_inline = 1;
                    else {
                        if (!writer_unmodeled_data.disable_inline_records &&
                            (((symbol->storage_class & 7) == 3 && (symbol->reference_flags & 3) == 1) ||
                             c1_bss.ir_record_count < (unsigned)c1_data.options.option_bu))
                            index |= 0x40;
                        choose_inline = 1;
                    }
                }
                if (choose_inline) {
                    if (writer_unmodeled_data.disable_inline_records || (type->shape->flags & 0x8000)) {
                        index = (unsigned)(int)(short)index & ~0x40U;
                        if (symbol->symbol_qualifiers & 1)
                            report_language_diagnostic(4, 710, symbol);
                    } else if (symbol->symbol_qualifiers & 1) index |= 0x40;
                }
            }
            GLOBAL_SHORT(index);
            saved_inline_flags = index;
            break;
        case 28: case 29:
            GLOBAL_WORD(0x386, 3);
            break;
        case 30:
            GLOBAL_DWORD(c1_data.c9_il ? 0x01309260 : 0x01309261, 5);
            break;
        case 37:
            GLOBAL_DWORD(c1_data.c9_il ? c1_data.next_symbol_record_id : 0x400000, 5);
            break;
        case 33: {
            GLOBAL_INDEX(writer_unmodeled_data.global_name_count);
            text = writer_unmodeled_data.global_option_string;
            if (text) {
                index = strlen(text) + 1;
                if (GLOBAL_RESERVE(index + 1)) {
                    memcpy(buffer->cursor, text, index);
                    buffer->cursor += index;
                }
            } else GLOBAL_ZERO();
            CompilerGlobalOption *item = writer_unmodeled_data.global_options;
            if (!item) { GLOBAL_INDEX(0); break; }
            unsigned count = 0;
            writer_unmodeled_data.global_options = NULL;
            while (item) {
                CompilerGlobalOption *next = item->next;
                item->next = writer_unmodeled_data.global_options;
                writer_unmodeled_data.global_options = item;
                item = next;
                ++count;
            }
            GLOBAL_INDEX(count);
            for (item = writer_unmodeled_data.global_options; item; item = item->next) {
                GLOBAL_BYTE(item->kind);
                if (item->kind == 7) {
                    const BYTE *bytes = item->value;
                    int length = (signed char)(bytes[0] + 1);
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
            CompilerSourceFile *file = (CompilerSourceFile *)c1_bss.current_context.object;
            if (c1_data.next_symbol_record_id >= 0x7fffffff) compiler_fatal(55);
            file->record_id = (file->record_id & 0xff000000) |
                (c1_data.next_symbol_record_id++ & 0xffffff);
            index = file->OutputIndex();
            if (c1_data.c9_il) GLOBAL_SHORT(index);
            else GLOBAL_COMPACT(index);
            text = c1_bss.current_context.FileName();
            index = strlen(text) + 1;
            available = 0;
            if ((int)(index + 1) >= 0) {
                available = 1;
                if (buffer->cursor + index + 1 > buffer->data + buffer->capacity)
                    available = buffer->Grow(index + 1);
            }
            buffer->good &= available;
            if (buffer->good) {
                memcpy(buffer->cursor, text, index);
                buffer->cursor += index;
            }
            break;
        }
        case 35: {
            short value = writer_unmodeled_bss.global_record_option_word;
            available = 1;
            if (buffer->cursor + 4 > buffer->data + buffer->capacity)
                available = buffer->Grow(4);
            buffer->good &= available;
            if (buffer->good) {
                if ((unsigned)(int)value & ~0x7fU) {
                    *buffer->cursor++ = 0x80;
                    *(WORD *)buffer->cursor = (WORD)value;
                    buffer->cursor += 2;
                } else *buffer->cursor++ = (BYTE)value;
            }
            break;
        }
        case 38:
            text = writer_unmodeled_bss.global_string_record;
            index = strlen(text) + 1;
            available = 0;
            if ((int)(index + 1) >= 0) {
                available = 1;
                if (buffer->cursor + index + 1 > buffer->data + buffer->capacity)
                    available = buffer->Grow(index + 1);
            }
            buffer->good &= available;
            if (buffer->good) {
                memcpy(buffer->cursor, text, index);
                buffer->cursor += index;
            }
            break;
        case 40:
            if (!c1_data.c9_il) GLOBAL_INDEX(writer_unmodeled_bss.nonil_type_index);
            break;
        case 41:
            if (!c1_data.c9_il) {
                available = 1;
                if (buffer->cursor + 4 > buffer->data + buffer->capacity)
                    available = buffer->Grow(4);
                buffer->good &= available;
                if (buffer->good) *buffer->cursor++ = 0;
            }
            break;
        default:
            break;
        }
    }
    if (!c1_data.c9_il && symbol) {
        if (buffer == &c1_bss.output_buffers[6])
            buffer->MarkLocation(symbol->reference_flags >> 3, 1);
        else if (buffer == &c1_bss.output_buffers[2])
            c1_bss.output_buffers[6].MarkGlobal(symbol->reference_flags >> 3);
        if (!has_location && opcode == 14) {
            buffer = &c1_bss.output_buffers[7];
            available = 1;
            if (buffer->cursor + 13 > buffer->data + buffer->capacity)
                available = buffer->Grow(13);
            buffer->good &= available;
            if (buffer->good) {
                *(DWORD *)buffer->cursor = emitted_flags;
                *(DWORD *)(buffer->cursor + 4) = recorded_count.raw;
                *(DWORD *)(buffer->cursor + 8) = saved_inline_flags;
                buffer->cursor += 12;
            }
            buffer->MarkLocation(symbol->reference_flags >> 3,
                (symbol->symbol_qualifiers & 1) ? 2 : 1);
        }
    }

}

/* Genuine next boundaries; none of these bodies is credited by this writer. */
extern "C" unsigned __cdecl global_type_size(CompilerType *type) { __asm { int 3 } return 0; }
extern "C" void __cdecl report_incomplete_global_type(CompilerType *type) { __asm { int 3 } }
extern "C" CompilerType *__cdecl get_function_return_type(CompilerType *type) { __asm { int 3 } return NULL; }
extern "C" void __cdecl write_global_type_flags(CompilerOutputBuffer *buffer, CompilerIrSymbol *symbol, unsigned flags) { __asm { int 3 } }
extern "C" void __cdecl write_global_reference(CompilerSymbolBinding *binding) { __asm { int 3 } }
extern "C" void __cdecl global_debug_location(unsigned index, int kind, CompilerGlobalLocation **location) { __asm { int 3 } }
extern "C" unsigned __cdecl global_array_element_size(CompilerIrSymbol *symbol) { __asm { int 3 } return 0; }
extern "C" const char *__cdecl global_symbol_name(CompilerIrSymbol *symbol) { __asm { int 3 } return NULL; }
unsigned RECON_THISCALL CompilerType::GetCallingConvention(void) { __asm { int 3 } return 0; }
unsigned RECON_THISCALL CompilerSourceFile::OutputIndex(void) { __asm { int 3 } return 0; }
const char *RECON_THISCALL CompilerContextState::FileName(void) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerOutputBuffer::MarkLocation(unsigned index, unsigned kind) { __asm { int 3 } }
void RECON_THISCALL CompilerOutputBuffer::MarkGlobal(unsigned index) { __asm { int 3 } }
