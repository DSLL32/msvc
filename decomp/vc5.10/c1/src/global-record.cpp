#include <string.h>
#include "frontend.h"
#include "keywords.h"
#include "global-record.h"
#pragma intrinsic(strlen, memcpy)
#include <recon.h>

/* Source-only contextual cells for currently unsupported domains. They are
 * UNMODELED native representation, excluded from defaults/snapshot/equality.
 * The entry guard precedes every native read or effect involving them. */
/* Shared physical source owners also written by function initialization.
 * Native defaults/transfer are still unmodeled until those writes are reached. */
#define c1_disable_inline_records c1_data.disable_inline_records /* RVA5f378 */
#define c1_nonil_type_index c1_bss.nonil_type_index /* RVA54c80 */
int c1_include_pch_reference_counts; /* RVA5f33c; shared directory/control owner */
int c1_inline_record_flag; /* RVA54d98; directory sets before the global writer reads */
static struct {
    unsigned pch_reference_count;           /* RVA5e9d8 */
} writer_unmodeled_data;
const char *c1_global_string_record; /* RVA54018, shared NameMap result */

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
    /* Only grounded scalar/header and function-shaped alternatives are
     * supported; incomplete semantic domains stop in both builds. */
    if (!c1_data.c9_il || (!symbol && opcode != 6 && opcode != 13 &&
        opcode != 17 && opcode != 18 && opcode != 10)) {
#ifdef __linux__
        recon_unsupported("global record: unmodeled symbol or non-IL domain");
#else
        __asm { int 3 }
#endif
    }
    if (symbol && !((opcode == 1 && symbol->kind == 10 &&
        !(symbol->scope_bits & 1) && (symbol->storage_class & 7) == 3 &&
        symbol->function_flags == 3 && !c1_data.options.debug_info &&
        (symbol->value_type == c1_data.builtin_types.type_0063 ||
         (symbol->value_type->flags == 0x63 && symbol->value_type->attributes == 0x100 &&
          symbol->value_type->shape && symbol->value_type->shape->flags == 2 &&
          !symbol->value_type->shape->attributes && !symbol->value_type->shape->base &&
          symbol->value_type->shape->value) ||
         (symbol->value_type->flags == 0x61 && symbol->value_type->attributes == 0x100 &&
          symbol->value_type->shape && symbol->value_type->shape->flags == 2 &&
          !symbol->value_type->shape->attributes && symbol->value_type->shape->base &&
          symbol->value_type->shape->base->flags == 1 &&
          !symbol->value_type->shape->base->attributes && !symbol->value_type->shape->base->base))) ||
        (opcode == 1 && symbol->kind == 10 &&
        !(symbol->scope_bits & 1) && (symbol->storage_class & 7) == 4 &&
        !(symbol->function_flags & 1) && !c1_data.options.debug_info &&
        symbol->value_type->shape && symbol->value_type->shape->flags == 2 &&
        !symbol->value_type->shape->attributes &&
        ((!symbol->value_type->attributes && !symbol->value_type->shape->base &&
          (symbol->value_type->flags == 0x61 || symbol->value_type->flags == 0x63 ||
           symbol->value_type->flags == 0x107)) ||
         (symbol->value_type->flags == 0x61 && symbol->value_type->attributes == 0x100 &&
          symbol->value_type->shape->base && symbol->value_type->shape->base->flags == 1 &&
          !symbol->value_type->shape->base->attributes && !symbol->value_type->shape->base->base))) ||
        (opcode == 9 && symbol->kind == 14 &&
        !c1_data.options.debug_info && symbol->value_type == c1_data.builtin_types.type_0802 &&
        (symbol->segment_kind == 0 || symbol->segment_kind == 1 ||
         symbol->segment_kind == 3 || symbol->segment_kind == 4) && !(symbol->scope_bits & 1) && !(symbol->function_flags & 1)) ||
        (opcode == 1 && symbol->kind == 10 && !symbol->binding.name &&
         (symbol->declaration_flags & 12) == 12 && !(symbol->scope_bits & 1) &&
         (symbol->storage_class & 7) == 3 && symbol->function_flags == 1 &&
         !c1_data.options.debug_info && symbol->value_type->flags == 0x61 &&
         !symbol->value_type->attributes && symbol->value_type->shape &&
         symbol->value_type->shape->flags == 2 && !symbol->value_type->shape->attributes &&
         !symbol->value_type->shape->base) || (opcode == 1 && symbol->kind == 10 &&
        !(symbol->scope_bits & 1) && ((symbol->storage_class & 7) == 2 ||
        (symbol->storage_class & 7) == 3 || (symbol->storage_class & 7) == 7) && !(symbol->function_flags & 1) && !c1_data.options.debug_info &&
        symbol->value_type->flags == 0x63 && !symbol->value_type->attributes &&
        symbol->value_type->shape && symbol->value_type->shape->flags == 2 &&
        !symbol->value_type->shape->attributes && !symbol->value_type->shape->base) ||
        (opcode == 1 && symbol->kind == 10 &&
        !(symbol->scope_bits & 1) && (symbol->storage_class & 7) == 3 &&
        !(symbol->function_flags & 1) && !c1_data.options.debug_info &&
        symbol->value_type->flags == 0x61 && symbol->value_type->attributes == 0x100 &&
        symbol->value_type->shape && symbol->value_type->shape->flags == 1 &&
        !symbol->value_type->shape->attributes && !symbol->value_type->shape->base) ||
        (opcode == 1 && symbol->kind == 10 &&
        !(symbol->scope_bits & 1) && (symbol->storage_class & 7) == 3 &&
        !(symbol->function_flags & 1) && !c1_data.options.debug_info &&
        ((symbol->value_type->flags == 0x63 && !(symbol->value_type->attributes & ~0x100U) &&
        symbol->value_type->shape && symbol->value_type->shape->flags == 2 &&
        !symbol->value_type->shape->attributes && symbol->value_type->shape->base &&
        symbol->value_type->shape->base->flags == 2 &&
        !symbol->value_type->shape->base->attributes && !symbol->value_type->shape->base->base) ||
        (symbol->value_type->flags == 0x402 && !symbol->value_type->attributes &&
        !symbol->value_type->shape && symbol->value_type->symbol_owner &&
        symbol->value_type->symbol_owner->kind == 4 && (symbol->value_type->symbol_owner->flags & 1)) ||
        (symbol->value_type->flags == 0x63 && !symbol->value_type->attributes &&
        symbol->value_type->shape && symbol->value_type->shape->flags == 2 &&
        !symbol->value_type->shape->attributes && symbol->value_type->shape->base &&
        symbol->value_type->shape->base->flags == 3 && symbol->value_type->shape->base->attributes == 0x10 &&
        symbol->value_type->shape->base->base && symbol->value_type->shape->base->base->flags == 0x404 &&
        symbol->value_type->shape->base->base->attributes == 0x10 && !symbol->value_type->shape->base->base->base) ||
        (symbol->value_type->flags == 0x61 && symbol->value_type->attributes == 0x100 &&
        symbol->value_type->shape && symbol->value_type->shape->flags == 2 &&
        !symbol->value_type->shape->attributes && symbol->value_type->shape->base &&
        symbol->value_type->shape->base->flags == 1 && !symbol->value_type->shape->base->attributes &&
        !symbol->value_type->shape->base->base))) ||
        (opcode == 1 && symbol->kind == 10 &&
        !(symbol->scope_bits & 1) && (symbol->storage_class & 7) == 3 &&
        !(symbol->function_flags & 1) && !c1_data.options.debug_info &&
        (symbol->value_type->flags == 0x402 || symbol->value_type->flags == 0x61 ||
        symbol->value_type->flags == 0x4062) &&
        !(symbol->value_type->attributes & ~0x100U) &&
        symbol->value_type->shape && symbol->value_type->shape->flags == 2 &&
        !symbol->value_type->shape->base && !symbol->value_type->shape->attributes &&
        (symbol->value_type->flags == 0x61 || symbol->value_type->flags == 0x4062 ||
        (symbol->value_type->symbol_owner && symbol->value_type->symbol_owner->kind == 4 &&
        (symbol->value_type->symbol_owner->flags & 1)))) ||
        (opcode == 1 && symbol->kind == 10 &&
        !(symbol->scope_bits & 1) && ((symbol->storage_class & 7) == 7 ||
        (symbol->storage_class & 7) == 3) &&
        !(symbol->function_flags & 1) && !c1_data.options.debug_info &&
        !symbol->value_type->shape && (symbol->value_type->flags == 0x63 || symbol->value_type->flags == 0x64 || symbol->value_type->flags == 0x4c ||
        symbol->value_type->flags == 0x404c || symbol->value_type->flags == 0x106 ||
        symbol->value_type->flags == 0x107) && !symbol->value_type->attributes) ||
        (opcode == 1 && symbol->kind == 10 && !(symbol->scope_bits & 1) &&
         (symbol->storage_class & 7) == 3 && !(symbol->function_flags & 1) &&
         !c1_data.options.debug_info && symbol->value_type->shape &&
         symbol->value_type->shape->flags == 2 && !symbol->value_type->shape->attributes &&
         !symbol->value_type->shape->base &&
         (((symbol->value_type->flags == 0x106 || symbol->value_type->flags == 0x107) &&
           !symbol->value_type->attributes) ||
          (symbol->value_type->flags == 0x62 && symbol->value_type->attributes == 0x200))) ||
        (opcode == 1 && symbol->kind == 10 && !(symbol->scope_bits & 1) &&
         (symbol->storage_class & 7) == 3 && !(symbol->function_flags & 1) &&
         !c1_data.options.debug_info && !symbol->value_type->shape &&
         symbol->value_type->flags == 0x4063 && symbol->value_type->attributes == 0x200 &&
         !symbol->value_type->auxiliary) ||
        (opcode == 1 && symbol->kind == 10 && !(symbol->scope_bits & 1) &&
        (symbol->storage_class & 7) == 7 && !(symbol->function_flags & 1) &&
        !c1_data.options.debug_info && symbol->value_type->flags == 0x63 &&
        !symbol->value_type->attributes && symbol->value_type->shape &&
        symbol->value_type->shape->flags == 3 &&
        (symbol->value_type->shape->attributes == 0x10 ||
         symbol->value_type->shape->attributes == 0x40 ||
         symbol->value_type->shape->attributes == 0x60) &&
        symbol->value_type->shape->base && symbol->value_type->shape->base->flags == 0x404 &&
        symbol->value_type->shape->base->attributes == symbol->value_type->shape->attributes &&
        !symbol->value_type->shape->base->base) ||
        (opcode == 1 && (symbol->kind == 10 || symbol->kind == 11) &&
        !(symbol->scope_bits & 1) && (symbol->storage_class & 7) == 4 &&
        !(symbol->function_flags & 1) && !c1_data.options.debug_info &&
        ((symbol->value_type->flags == 0x63 && !symbol->value_type->attributes &&
         symbol->value_type->shape && symbol->value_type->shape->flags == 1 &&
         !symbol->value_type->shape->attributes && symbol->value_type->shape->base &&
         symbol->value_type->shape->base->flags == 2 && !symbol->value_type->shape->base->attributes &&
         symbol->value_type->shape->base->value == 2 && symbol->value_type->shape->base->base &&
         symbol->value_type->shape->base->base->flags == 3 &&
         symbol->value_type->shape->base->base->attributes == 0x10 &&
         symbol->value_type->shape->base->base->base &&
         symbol->value_type->shape->base->base->base->flags == 0x404 &&
         symbol->value_type->shape->base->base->base->attributes == 0x10 &&
         !symbol->value_type->shape->base->base->base->base) ||
        (!symbol->value_type->shape && !symbol->value_type->attributes &&
        ((symbol->value_type->flags & ~0x4000) == 0x63 ||
         (symbol->kind == 11 && symbol->value_type->flags == 0x62) ||
         (symbol->value_type->flags & ~0x4000) == 0x4c ||
         symbol->value_type->flags == 0x4064 || symbol->value_type->flags == 0x44 ||
         symbol->value_type->flags == 0x64 || symbol->value_type->flags == 0x4062 ||
         symbol->value_type->flags == 0x61 || symbol->value_type->flags == 0x106 ||
         symbol->value_type->flags == 0x107)) ||
        (symbol->value_type->flags == 0x63 && symbol->value_type->attributes == 0x100 &&
         !symbol->value_type->auxiliary && symbol->value_type->shape &&
         symbol->value_type->shape->flags == 1 && !symbol->value_type->shape->attributes &&
         !symbol->value_type->shape->base) ||
        (symbol->value_type->flags == 0x62 && symbol->value_type->attributes == 0x200 &&
         symbol->value_type->shape && symbol->value_type->shape->flags == 1 &&
         !symbol->value_type->shape->attributes && !symbol->value_type->shape->base) ||
        (symbol->value_type->flags == 0x63 && symbol->value_type->attributes == 0x100 &&
        symbol->value_type->shape && symbol->value_type->shape->flags == 1 &&
        !symbol->value_type->shape->attributes && symbol->value_type->shape->base &&
        symbol->value_type->shape->base->flags == 2 &&
        !symbol->value_type->shape->base->attributes && !symbol->value_type->shape->base->base) ||
        (symbol->value_type->flags == 0x80 && !symbol->value_type->attributes &&
        symbol->value_type->shape && symbol->value_type->shape->flags == 1 &&
        !symbol->value_type->shape->attributes && !symbol->value_type->shape->base) ||
        (symbol->value_type->flags == 0x63 &&
        !symbol->value_type->attributes && symbol->value_type->shape &&
        symbol->value_type->shape->flags == 3 &&
        (symbol->value_type->shape->attributes == 0x10 ||
         symbol->value_type->shape->attributes == 0x40 ||
         symbol->value_type->shape->attributes == 0x60) &&
        symbol->value_type->shape->base && symbol->value_type->shape->base->flags == 0x404 &&
        symbol->value_type->shape->base->attributes == symbol->value_type->shape->attributes &&
        !symbol->value_type->shape->base->base) ||
        (!symbol->value_type->shape && !symbol->value_type->attributes &&
        (symbol->value_type->flags == 0x402 || symbol->value_type->flags == 0x404) &&
        symbol->value_type->symbol_owner && symbol->value_type->symbol_owner->kind == 4 &&
        (symbol->value_type->symbol_owner->flags & 1)) ||
        (symbol->value_type->flags == 0x402 && symbol->value_type->shape &&
        (symbol->value_type->shape->flags & 7) == 1 &&
        symbol->value_type->symbol_owner->kind == 4 &&
        (symbol->value_type->symbol_owner->flags & 1)))) ||
        (opcode == 1 && symbol->kind == 10 && !(symbol->scope_bits & 1) &&
        (symbol->storage_class & 7) == 3 && !(symbol->function_flags & 1) &&
        !c1_data.options.debug_info && !symbol->value_type->shape &&
        symbol->value_type->flags == 0x404 && !symbol->value_type->attributes &&
        symbol->value_type->symbol_owner && symbol->value_type->symbol_owner->kind == 4 &&
        (symbol->value_type->symbol_owner->flags & 1)) ||
        (opcode == 3 && symbol->kind == 9 && !c1_data.options.debug_info &&
        !(symbol->scope_bits & 1) && (symbol->storage_class & 7) != 7 &&
        !(symbol->function_flags & 1) && symbol->value_type == c1_data.builtin_types.type_0063) ||
        ((opcode == 4 || opcode == 14) && symbol->kind == 7 && !(symbol->scope_bits & 1) &&
        ((symbol->storage_class & 7) != 7 ||
         (symbol->value_type->flags == 0x63 && symbol->value_type->attributes == 0x10 &&
          !symbol->value_type->auxiliary && symbol->value_type->shape &&
          symbol->value_type->shape->flags == 0x404 &&
          symbol->value_type->shape->attributes == 0x10 && !symbol->value_type->shape->base)) &&
        (!(symbol->function_flags & 1) || symbol->function_flags == 3) &&
        symbol->value_type->shape && (symbol->value_type->shape->flags & 7) == 4 &&
        !c1_data.options.debug_info))) {
#ifdef __linux__
        recon_unsupported("global record: symbol alternative is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    if (opcode == 10 && c1_data.pch_record_name) {
#ifdef __linux__
        recon_unsupported("global record: PCH name or option-list contents are not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    if (opcode == 10) {
        for (CompilerGlobalOption *item = c1_data.global_options; item; item = item->next) {
            if (item->kind != 4 && item->kind != 6) {
#ifdef __linux__
                recon_unsupported("global record: unsupported linker-comment kind");
#else
                __asm { int 3 }
#endif
            }
        }
    }
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
                    CompilerIrSymbol *context = lookup_ir_symbol(symbol);
                    if (symbol->function_flags == 3 &&
                        (symbol->kind == 7 || symbol->kind == 10) &&
                        (!context || context->kind != 14 ||
                         context->value_type != c1_data.builtin_types.type_0802 ||
                         (symbol->kind == 7 ? context->segment_kind != 0 :
                          (context->segment_kind != 1 && context->segment_kind != 3 && context->segment_kind != 4)))) {
#ifdef __linux__
                        recon_unsupported("symbol output context is outside the recovered code/data-segment domain");
#else
                        __asm { int 3 }
#endif
                    }
                    if (context->kind == 14 &&
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
        buffer == &c1_bss.output_buffers[2] && !c1_data.omit_external_names &&
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
            GLOBAL_BYTE(opcode == 3 ? symbol->label_nesting : c1_bss.declaration_scope_active);
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
                    (type->symbol_owner->tag_definition_flags & 0x100))) storage = 0x10;
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
                GLOBAL_INDEX(c1_bss.function_expression_offset);
                index = c1_bss.function_symbol_offset +
                    (c1_include_pch_reference_counts ? writer_unmodeled_data.pch_reference_count : 0);
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
                    (c1_inline_record_flag ? 0x20 : 0) |
                    (c1_data.ir_symbol_state_changed ? 8 : 0) |
                    (c1_include_pch_reference_counts ? 1 : 0) |
                    ((symbol->symbol_qualifiers & 1) << 7);
                if ((type->shape->flags & 0x800) && !(type->shape->flags & 0x4000))
                    c1_disable_inline_records = 1;
                int choose_inline = c1_data.options.inline_level == 1;
                if (c1_data.options.inline_level == 2) {
                    if (!c1_data.ir_symbol_state_ready) choose_inline = 1;
                    else {
                        if (!c1_disable_inline_records &&
                            (((symbol->storage_class & 7) == 3 && (symbol->reference_flags & 3) == 1) ||
                             c1_bss.ir_record_count < (unsigned)c1_data.options.option_bu))
                            index |= 0x40;
                        choose_inline = 1;
                    }
                }
                if (choose_inline) {
                    if (c1_disable_inline_records || (type->shape->flags & 0x8000)) {
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
            GLOBAL_INDEX(c1_data.pch_type_record_value);
            text = c1_data.pch_record_name;
            if (text) {
                index = strlen(text) + 1;
                if (GLOBAL_RESERVE(index + 1)) {
                    memcpy(buffer->cursor, text, index);
                    buffer->cursor += index;
                }
            } else GLOBAL_ZERO();
            CompilerGlobalOption *item = c1_data.global_options;
            if (!item) { GLOBAL_INDEX(0); break; }
            unsigned count = 0;
            c1_data.global_options = NULL;
            while (item) {
                CompilerGlobalOption *next = item->next;
                item->next = c1_data.global_options;
                c1_data.global_options = item;
                item = next;
                ++count;
            }
            GLOBAL_INDEX(count);
            for (item = c1_data.global_options; item; item = item->next) {
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
            short value = (short)c1_bss.parameter_symbol_count;
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
            text = c1_global_string_record;
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
            if (!c1_data.c9_il) GLOBAL_INDEX(c1_nonil_type_index);
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
extern "C" unsigned __cdecl primitive_type_size(WORD, unsigned);
extern "C" unsigned __cdecl pointer_storage_size(WORD);
extern "C" unsigned __cdecl type_shape_element_size(WORD, unsigned, CompilerTypeShape *);
extern "C" unsigned __cdecl global_type_size(CompilerType *type)
{
    CompilerTypeShape *shape = type->shape;
    if (!shape) return primitive_type_size(type->flags, type->auxiliary);
    if ((shape->flags & 7) != 2) return pointer_storage_size(shape->attributes);
    unsigned element = type_shape_element_size(type->flags, type->auxiliary, shape);
    unsigned bytes = element * shape->value;
    if ((int)bytes < (int)element && shape->value) {
        compiler_warning(148);
        shape->value = 1;
    }
    return bytes;
}
/* Original low-four-bit primitive width encodings, RVA64970. Several
 * encodings have no recovered language name; their numeric identities stay
 * explicit rather than assigning a speculative scalar type. */
extern "C" const signed char c1_primitive_widths[16] = {
    0, 1, 2, 4, 4, 2, 4, 8, 8, 1, 2, 4, 8, 16, 0, 0
};
struct AggregateSizeOwnerView {
    BYTE unmodeled_00[8];
    CompilerIrSymbol *owner;
};
extern "C" unsigned __cdecl primitive_type_size(WORD flags, unsigned auxiliary)
{
    if (flags & 0x400) {
        CompilerIrSymbol *symbol = (CompilerIrSymbol *)auxiliary;
        if (symbol->kind != 4) {
#ifdef __linux__
            recon_unsupported("indirect aggregate size owner is not yet modeled");
#else
            __asm { int 3 }
#endif
            symbol = ((AggregateSizeOwnerView *)symbol->abstract_type)->owner;
        }
        return symbol->integer_value;
    }
    if (flags & 0x200) return c1_primitive_widths[3];
    if ((flags & 0x340) || !(flags & 0xfff0))
        return c1_primitive_widths[flags & 15];
    return flags & 15;
}
extern "C" unsigned __cdecl pointer_storage_size(WORD)
{
    return 4;
}
extern "C" unsigned __cdecl type_shape_element_size(WORD flags, unsigned auxiliary,
    CompilerTypeShape *shape)
{
    CompilerTypeShape *part;
    unsigned element, bytes;
    for (;;) {
        if (!shape) return primitive_type_size(flags, auxiliary);
        switch (shape->flags & 7) {
        case 1:
            part = shape->base;
            if (!part) return primitive_type_size(flags, auxiliary);
            switch (part->flags & 7) {
            case 1: case 3:
                return pointer_storage_size(part->attributes);
            case 2:
                return type_shape_element_size(flags, auxiliary, part) * part->value;
            default:
                return 0;
            }
        case 2:
            part = shape->base;
            if (part && (part->flags & 1)) return pointer_storage_size(part->attributes);
            element = type_shape_element_size(flags, auxiliary, part);
            if (!part) return element;
            bytes = element * part->value;
            if ((int)bytes < (int)element && part->value) {
                compiler_warning(148);
                part->value = 1;
            }
            return bytes;
        case 3:
            return pointer_storage_size(shape->attributes);
        case 4:
            shape = shape->base;
            if (shape && (shape->flags & 1)) return pointer_storage_size(shape->attributes);
            break;
        default:
            return 0;
        }
    }
}
extern "C" void __cdecl report_incomplete_global_type(CompilerType *type) { __asm { int 3 } }
extern "C" CompilerType *__cdecl get_function_return_type(CompilerType *type)
{
    if (c1_data.type_arena_kind >= 6) {
#ifdef __linux__
        recon_unsupported("function return type: unsupported allocation arena");
#else
        __asm { int 3 }
#endif
    }
    CompilerArena *arena = &c1_bss.arenas[c1_data.type_arena_kind];
    CompilerType *result = (CompilerType *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    /* Preserve all16 original storage bytes, including the opaque cache. */
    memcpy(result, type, 16);
    result->shape = type->shape->base;
    return result;
}
extern "C" void __cdecl write_global_type_flags(CompilerOutputBuffer *buffer,
    CompilerIrSymbol *symbol, unsigned flags)
{
    CompilerIrSymbol *target;
    unsigned index;
    int available;
    if (!(symbol->function_flags & 1)) {
        buffer->WriteIndex(flags);
        return;
    }
    target = lookup_ir_symbol(symbol);
    flags |= 0x200;
    GLOBAL_SHORT(flags);
    if (c1_data.c9_il) index = target->record_id & 0xffffff;
    else if (target->reference_flags & ~7U) index = target->reference_flags >> 3;
    else index = assign_symbol_output_index(target);
    if (c1_data.c9_il) buffer->WriteIndex(index);
    else GLOBAL_COMPACT(index);
}
extern "C" {
unsigned __cdecl finish_debug_type(CompilerIrSymbol *);
void __cdecl prepare_debug_type(CompilerType *);
void __cdecl remember_symbol_reference(CompilerSymbolBinding *);
void __cdecl attach_declaration_context(CompilerIrSymbol *, void *);
void __cdecl write_global_binding_record(unsigned, CompilerSymbolBinding *, int);
}
extern "C" void __cdecl write_global_reference(CompilerSymbolBinding *binding)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
    CompilerIrSymbol *target;
    CompilerType *type;
    int suppress = 0, defer = 0, omit_name = 0;
    unsigned opcode;
    if (c1_bss.parser_early_result > 0 || (symbol->flags & 2)) return;
    if (symbol->kind == 14) {
        write_global_record(9, symbol);
        symbol->flags |= 2;
        return;
    }
    /* PCH/debug providers and replay semantics are beyond this case. */
    if (c1_data.pch_separate_debug || c1_data.options.debug_info ||
        (symbol->flags & 0x80)) {
#ifdef __linux__
        recon_unsupported("global reference: PCH or debug symbol domain is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    if (c1_data.pch_separate_debug && !c1_bss.declaration_scope_active &&
        (!symbol->IsNamed() || !(symbol->declaration_flags & 8))) defer = 1;
    if ((symbol->flags & 0x80) && !c1_data.options.pch_no_preprocess &&
        c1_data.options.pch_use && !c1_data.options.pch_create) suppress = 1;
    if (symbol->kind == 4) {
        if (c1_data.options.debug_info && !(symbol->flags & 0x40))
            symbol->value_type->debug_index = finish_debug_type(symbol);
        if (defer && (symbol->flags & 1)) { symbol->flags |= 0x80; return; }
        if (!binding->name || binding->name == c1_data.builtin_names.unnamed) return;
        if (c1_data.options.debug_info && (symbol->flags & 1) &&
            !(symbol->flags & 0x40) && !suppress) {
            opcode = 11;
            goto emit;
        }
        goto marked;
    }
    if (symbol->kind == 5) {
        if (!c1_data.options.debug_info) goto marked;
        type = symbol->value_type;
        if (!c1_bss.options.prototypes_only) {
            prepare_debug_type(type);
            if (type->symbol_owner && type->symbol_owner->kind == 4 &&
                !type->symbol_owner->alternate_size) finish_debug_type(type->symbol_owner);
        }
        if (defer) { symbol->flags |= 0x80; return; }
        if (suppress) goto marked;
        opcode = 8;
        goto emit;
    }
    if (defer) {
        remember_symbol_reference(binding);
        if (symbol->kind == 10 && (symbol->storage_class & 7) == 7)
            symbol->flags |= 0x80;
        return;
    }
    if (symbol->kind == 10 && (symbol->storage_class & 7) == 7 &&
        (symbol->flags & 0x80)) {
        if (!suppress) write_global_record(11, symbol);
        goto marked;
    }
    if (symbol->kind == 10 && (symbol->declaration_flags & 0x40)) {
        target = resolve_symbol_alias(symbol);
        target->declaration_flags = (target->declaration_flags & ~4) |
            (symbol->declaration_flags & 4);
        ((BYTE *)&target->qualifier_flags)[0] = (BYTE)(symbol->qualifier_flags ^
            ((symbol->qualifier_flags ^ target->qualifier_flags) & 0x7f));
        target->symbol_value = symbol->symbol_value;
        if (symbol->function_flags & 1) {
            CompilerIrSymbol *context = lookup_ir_symbol(symbol);
            if (context) attach_declaration_context(target, context);
            else target->function_flags &= ~1;
        }
        target->value_type = symbol->value_type;
        if (!c1_data.options.debug_info) return;
        if ((target->reference_flags & 3) < 3)
            target->reference_flags = (target->reference_flags & ~3U) |
                ((target->reference_flags + 1) & 3);
        target->reference_flags = (target->reference_flags & ~4U) |
            ((c1_data.ir_reference_epoch & 1) << 2);
        type = symbol->value_type;
        if (!c1_bss.options.prototypes_only) {
            prepare_debug_type(type);
            if (type->symbol_owner && type->symbol_owner->kind == 4 &&
                !type->symbol_owner->alternate_size) finish_debug_type(type->symbol_owner);
        }
        if (symbol->flags & 0x40) return;
        opcode = 7;
        goto emit;
    }
    omit_name = (symbol->kind != 6 && symbol->kind != 7 && symbol->kind != 8 &&
        !(symbol->kind == 10 && (symbol->storage_class & 7) == 7) &&
        (symbol->storage_class & 7) != 1);
    if (!(symbol->reference_flags & 3)) {
        if (!omit_name) return;
        if (symbol->kind == 11) report_language_diagnostic(4, 100, symbol);
        else if (c1_bss.declaration_scope_active) report_language_diagnostic(3, 101, symbol);
    }
    opcode = (symbol->kind == 6 || symbol->kind == 7 || symbol->kind == 8) ? 4 : 1;
    if (c1_data.options.debug_info) remember_symbol_reference(binding);
emit:
    write_global_binding_record(opcode, binding, omit_name);
marked:
    symbol->flags |= 2;
}
extern "C" void __cdecl remember_symbol_reference(CompilerSymbolBinding *) { __asm { int 3 } }
int RECON_THISCALL CompilerIrSymbol::IsNamed(void) { __asm { int 3 } return 0; }

extern "C" void __cdecl global_debug_location(unsigned index, int kind, CompilerGlobalLocation **location) { __asm { int 3 } }
extern "C" unsigned __cdecl global_array_element_size(CompilerIrSymbol *symbol)
{
    CompilerType *type = symbol->value_type;
    CompilerTypeShape *shape = type->shape;
    while (shape && !(shape->flags & 1)) shape = shape->base;
    if (!shape) return primitive_type_size(type->flags, type->auxiliary);
    return pointer_storage_size(shape->attributes);
}
/* Shared naming scratch. Only the written string prefix is defined;
 * no pristine default, snapshot or untouched-tail equality is claimed. */
static char decorated_symbol_name[0x1800]; /* original RVA55d58 */
/* RVA55d50 is written on every entry before use; no pristine pointer or
 * untouched buffer-tail equality is asserted. Both naming paths share55d58. */
static char *decorated_name_start;
extern "C" char *__cdecl encode_declaration_number(char *, unsigned);
extern "C" char *__cdecl format_static_declaration_name(CompilerIrSymbol *symbol, unsigned depth)
{
    char *cursor;
    const char *name;
    decorated_name_start = decorated_symbol_name;
    cursor = decorated_symbol_name;
    *cursor++ = '?';
    for (name = symbol->binding.name->name; *name; ++name) *cursor++ = *name;
    *cursor++ = '@';
    *cursor++ = '?';
    cursor = encode_declaration_number(cursor, depth & 0xffff);
    *cursor++ = '?';
    *cursor++ = '?';
    if (symbol->binding.name == c1_bss.current_scope_symbol->binding.name)
        *cursor++ = '0';
    else
        for (name = c1_bss.current_scope_symbol->binding.name->name; *name; ++name)
            *cursor++ = *name;
    for (name = "@@9@9"; *name; ++name) *cursor++ = *name;
    *cursor = 0;
    return decorated_symbol_name;
}
extern "C" char *__cdecl encode_declaration_number(char *cursor, unsigned value)
{
    unsigned mask, shift;
    if (value >= 1 && value <= 10) {
        *cursor++ = (char)(value + '0' - 1);
        return cursor;
    }
    mask = 0xf0000000U;
    shift = 28;
    while (shift && !(value & mask)) { shift -= 4; mask >>= 4; }
    while (shift) {
        *cursor++ = (char)('A' + ((value & mask) >> shift));
        shift -= 4;
        mask >>= 4;
    }
    *cursor++ = (char)('A' + (value & 15));
    *cursor++ = '@';
    return cursor;
}

extern "C" int __cdecl format_buffer(char *, const char *, unsigned, ...);
extern "C" void __cdecl compute_parameter_bytes(CompilerIrSymbol *);
extern "C" const char *__cdecl global_symbol_name(CompilerIrSymbol *symbol)
{
    unsigned convention;
    int length;
    if (symbol->kind != 6 && symbol->kind != 7 && symbol->kind != 8)
        return symbol->binding.name->name;
    convention = symbol->value_type->attributes & 0x70;
    if (convention != 0x60 && convention != 0x40)
        return symbol->binding.name->name;
    /* The genuine pending formatter bounds this as-yet unexecuted branch
     * before it can write the scratch buffer. */
    length = format_buffer(decorated_symbol_name, "%s", 0x1800,
        symbol->binding.name->name);
    if (!(symbol->qualifier_flags & 0x40)) compute_parameter_bytes(symbol);
    length += format_buffer(decorated_symbol_name + length, "@%d", 0x1800 - length,
        symbol->symbol_value);
    if (length >= 0x800) {
        report_language_diagnostic(1, 503, 83, symbol);
        length = 0x7ff;
    }
    decorated_symbol_name[length] = 0;
    return decorated_symbol_name;
}
extern "C" void __cdecl compute_parameter_bytes(CompilerIrSymbol *symbol) { __asm { int 3 } }

unsigned RECON_THISCALL CompilerType::GetCallingConvention(void)
{
    CompilerTypeShape *selected = shape ? shape->FindMatching(&CompilerTypeShape::IsNotArray) : NULL;
    return (selected ? selected->attributes : attributes) & 0x1f00;
}
CompilerTypeShape *RECON_THISCALL CompilerTypeShape::FindMatching(CompilerTypeShapePredicate predicate)
{
    CompilerTypeShape *shape;
    for (shape = this; shape; shape = shape->base)
        if ((shape->*predicate)()) return shape;
    return NULL;
}
int RECON_THISCALL CompilerTypeShape::IsNotArray(void)
{ return (flags & 7) != 2; }
unsigned RECON_THISCALL CompilerSourceFile::OutputIndex(void)
{
    if (c1_data.c9_il) return record_id & 0xffffff;
#ifdef RECON_NATIVE_TEST
    recon_unsupported("source-file non-IL reference field is not yet modeled");
#endif
    if (!(reference_flags & ~7U)) return AssignOutputIndex();
    return reference_flags >> 3;
}
unsigned RECON_THISCALL CompilerSourceFile::AssignOutputIndex(void)
{ __asm { int 3 } return 0; }
const char *RECON_THISCALL CompilerContextState::FileName(void)
{
    /* Native execution supports the genuine NULL or owned source-record
     * domain; Windows retains the original defensive SEH fallback. */
    __try {
        if (!object) return "";
        return ((CompilerSourceFile *)object)->name->name;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return "";
    }
}
void RECON_THISCALL CompilerOutputBuffer::MarkLocation(unsigned index, unsigned kind) { __asm { int 3 } }
void RECON_THISCALL CompilerOutputBuffer::MarkGlobal(unsigned index) { __asm { int 3 } }
