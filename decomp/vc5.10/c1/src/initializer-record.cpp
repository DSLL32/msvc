#include <string.h>
#include "frontend.h"
#pragma intrinsic(memcpy)
#include <recon.h>

/* Original compiler real constants use an explicit extended representation,
 * independent of either compiler's spelling of long double. Its two tail
 * bytes remain opaque and are never given an invented value. */
struct InitializerExtendedReal {
    DWORD significand_low, significand_high;
    WORD exponent, unmodeled_tail;
};
typedef char InitializerExtendedReal_size[sizeof(InitializerExtendedReal) == 12 ? 1 : -1];

extern "C" {
void __cdecl write_initializer_symbol(CompilerIrSymbol *, CompilerParserNode *, CompilerOutputBuffer *);
int __cdecl initializer_accepts_string(CompilerIrSymbol *, CompilerParserNode *);
CompilerParserNode *__cdecl lower_ir_node(CompilerParserNode *);
int __cdecl get_static_initializer_reference(CompilerParserNode *, CompilerIrSymbol **, int *);
CompilerIntegerConstant *__cdecl make_integer_constant(int);
CompilerIrSymbol *__cdecl resolve_symbol_alias(CompilerIrSymbol *);
CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *);
unsigned __cdecl assign_symbol_output_index(CompilerIrSymbol *);
unsigned __cdecl global_type_size(CompilerType *);
unsigned __cdecl global_array_element_size(CompilerIrSymbol *);
unsigned __cdecl get_initializer_storage_size(int);
void __cdecl write_initializer_integer(const unsigned __int64 *, unsigned, CompilerOutputBuffer *);
const char *__cdecl format_initializer_integer(const unsigned __int64 *, char *, unsigned, int);
int __cdecl parse_initializer_real(const char *, InitializerExtendedReal *);
int __cdecl initializer_real_to_double(const InitializerExtendedReal *, double *);
int __cdecl initializer_real_to_float(const InitializerExtendedReal *, float *);
int __cdecl convert_initializer_float_to_long(const BYTE *, int *);
int __cdecl initializer_real_to_integer(const InitializerExtendedReal *, unsigned __int64 *);
void __cdecl compiler_warning(int, ...);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
extern const signed char c1_primitive_widths[16];
}

/* BYTE directives consumed by this particular record writer. The other
 * column of the original eight-byte format rows is not represented. */
static const char *initializer_formats[9] = {
    "mdox", "tpzv", "tsfz", "tl", "ti", "tr", "t", "t", "tn"
};

/* These are genuinely inlined buffer operations. Calling a new convenience
 * function here would introduce an application edge absent from the target. */
#define INIT_RESERVE(n) \
    (output->good &= output->cursor + (n) <= output->data + output->capacity ? 1 : output->Grow(n))
#define INIT_BYTE(v) do { if (INIT_RESERVE(2)) *output->cursor++ = (BYTE)(v); } while (0)
#define INIT_COMPACT(v) do { \
    unsigned encoded_value = (unsigned)(v); \
    output->good &= output->Ensure(6); \
    if (output->good) { \
        if (!(encoded_value & 0xffffff80U)) *output->cursor++ = (BYTE)encoded_value; \
        else { *output->cursor++ = 0x80; *(DWORD *)output->cursor = encoded_value; output->cursor += 4; } \
    } \
} while (0)

extern "C" void __cdecl write_initializer_bytes(CompilerIrSymbol *symbol,
    CompilerParserNode *value, CompilerOutputBuffer *output)
{
    CompilerIrSymbol *referenced = NULL;
    CompilerType *type;
    CompilerTypeShape *shape;
    const char *format;
    int displacement = 0, literal_string = 0, unsigned_value, integer_value;
    unsigned record, encoding = 0, width = 0, index, flags;
    unsigned __int64 bits, narrowed;
    InitializerExtendedReal real;
    char integer_text[24];
    double real64;
    float real32;
    int converted;
    if (!value || (c1_data.function_record_flags & 0x800)) return;
    if (c1_data.last_initializer_symbol && output == &c1_bss.output_buffers[4]) {
        write_initializer_symbol(c1_data.last_initializer_symbol, 0, &c1_bss.output_buffers[4]);
        c1_data.last_initializer_symbol = NULL;
        c1_data.lexer_flags |= 0x40000;
    }
    if (!symbol) record = 8;
    else if ((value->flags & 7) == 3) record = 1;
    else {
        if ((value->flags & 7) == 5) {
            literal_string = 1;
            if (initializer_accepts_string(symbol, value)) record = 3;
            else {
                value = lower_ir_node(value);
                if (!get_static_initializer_reference(value, &referenced, &displacement)) {
#ifdef __linux__
                    recon_unsupported("initializer record kind is unwritten on failed string lowering");
#else
                    __asm { int 3 }
#endif
                }
                record = 2;
            }
        } else {
            if (!get_static_initializer_reference(value, &referenced, &displacement)) {
#ifdef __linux__
                recon_unsupported("initializer record kind is unwritten on failed reference extraction");
#else
                __asm { int 3 }
#endif
            }
            if (referenced) record = 2;
            else { value = (CompilerParserNode *)make_integer_constant(displacement); record = 1; }
        }
    }
    format = initializer_formats[record];
    while (format && *format) {
        switch (*format++) {
        case 't': INIT_BYTE(record); break;
        case 'i': INIT_BYTE(0); break;
        case 'f': INIT_COMPACT(displacement); break;
        case 'n': INIT_COMPACT(value->low); break;
        case 'l':
            width = value->string_length;
            output->good &= output->Ensure(width + 4);
            if (output->good) {
                if (!((int)(short)value->string_length & ~0x7f)) *output->cursor++ = (BYTE)width;
                else { *output->cursor++ = 0x80; *(WORD *)output->cursor = (WORD)width; output->cursor += 2; }
                memcpy(output->cursor, value->text, width);
                output->cursor += width;
            }
            break;
        case 's':
            if ((referenced->reference_flags & 3) < 3) ++referenced->reference_flags;
            referenced->reference_flags = (referenced->reference_flags & ~4U) | ((c1_data.ir_reference_epoch & 1) << 2);
            if (referenced->declaration_flags & 0x40) {
                referenced = resolve_symbol_alias(referenced);
                if ((referenced->reference_flags & 3) < 3) ++referenced->reference_flags;
                referenced->reference_flags = (referenced->reference_flags & ~4U) | ((c1_data.ir_reference_epoch & 1) << 2);
            }
            if ((referenced->function_flags & 1) &&
                (lookup_ir_symbol(referenced)->record_id & 0xffffff) == 5 && c1_bss.current_scope_symbol &&
                !(c1_bss.current_scope_symbol->symbol_qualifiers & 1)) {
                c1_data.ir_symbol_state_changed = 1;
                c1_data.ir_symbol_state_ready = 0;
            }
            if (c1_data.c9_il) index = referenced->record_id & 0xffffff;
            else index = (referenced->reference_flags & ~7U) ? referenced->reference_flags >> 3 : assign_symbol_output_index(referenced);
            output->good &= output->Ensure(c1_data.c9_il ? 5 : 4);
            if (output->good) {
                if (c1_data.c9_il) {
                    if ((int)index > 0x7fff) {
                        *(DWORD *)output->cursor = (((index & 0xffffc000U) | 0x4000) << 1) | (index & 0x7fff);
                        output->cursor += 4;
                    } else { *(WORD *)output->cursor = (WORD)index; output->cursor += 2; }
                } else if (index >= 0x3fff80) *output->cursor++ = (BYTE)(index + 0x80);
                else if (index <= 0x3fff) {
                    *output->cursor++ = (BYTE)((index & 0x3f) | 0x80);
                    *output->cursor++ = (BYTE)(index >> 6);
                } else {
                    *output->cursor++ = (BYTE)(index | 0xc0);
                    *output->cursor++ = (BYTE)(index >> 6);
                    *output->cursor++ = (BYTE)(index >> 14);
                }
            }
            break;
        case 'p':
            type = symbol->value_type;
            flags = type->flags;
            shape = type->shape;
            if (shape && (shape->flags & 1)) encoding = 3;
            else if (flags & 0x4000) encoding = 2;
            else if (!c1_data.options.language_extensions && c1_data.options.unsigned_char &&
                (flags & 0x2fcf) == 0x41 && !(flags & 0x8000)) encoding = 2;
            else if (flags & 0x40) encoding = 1;
            else if (flags & 0x100) encoding = 5;
            else if (flags & 0x800) encoding = 2;
            else if (flags & 0x200) encoding = 1;
            INIT_BYTE(encoding);
            break;
        case 'z':
            type = symbol->value_type;
            shape = type->shape;
            width = shape && (shape->flags & 7) == 2 ? global_array_element_size(symbol) : global_type_size(type);
            if (width == 2 && (!literal_string || width != (unsigned)(int)c1_primitive_widths[1]) &&
                !(type->flags & 0x40) && !(type->flags & 0x800) && !(shape && (shape->flags & 1))) {
                compiler_warning(97);
                width = get_initializer_storage_size(displacement);
            }
            INIT_BYTE(width);
            break;
        case 'v':
            type = value->type;
            shape = type->shape;
            flags = type->flags;
            integer_value = (flags & 0x40) || (shape && (shape->flags & 1)) || (flags & 0x200);
            unsigned_value = !!(flags & 0x4000) ||
                (!c1_data.options.language_extensions && c1_data.options.unsigned_char &&
                 (flags & 0x2fcf) == 0x41 && !(flags & 0x8000));
            if (!integer_value && !(flags & 0x100)) {
#ifdef __linux__
                recon_unsupported("initializer value leaves numeric stack storage unwritten");
#else
                __asm { int 3 }
#endif
            }
            if (integer_value) bits = ((unsigned __int64)value->high << 32) | value->low;
            if (encoding == 5) {
                if (integer_value && parse_initializer_real(format_initializer_integer(&bits, integer_text, 10, unsigned_value), &real))
                    report_language_diagnostic(2, 56);
                if (width == 8) {
                    if (initializer_real_to_double(integer_value ? &real : (InitializerExtendedReal *)value->low, &real64))
                        report_language_diagnostic(2, 56);
                    if (INIT_RESERVE(9)) { memcpy(output->cursor, &real64, 8); output->cursor += 8; }
                } else if (width == 4) {
                    if (initializer_real_to_float(integer_value ? &real : (InitializerExtendedReal *)value->low, &real32))
                        report_language_diagnostic(2, 56);
                    if (INIT_RESERVE(5)) { memcpy(output->cursor, &real32, 4); output->cursor += 4; }
                } else {
                    if (!integer_value) memcpy(&real, (const void *)value->low, 12);
#ifdef __linux__
                    recon_unsupported("extended initializer trailing stack word is not yet qualified");
#else
                    __asm { int 3 }
#endif
                    if (INIT_RESERVE(11)) { memcpy(output->cursor, &real, 10); output->cursor += 10; }
                    if (INIT_RESERVE(3)) { *(WORD *)output->cursor = real.unmodeled_tail; output->cursor += 2; }
                }
            } else {
                if (!integer_value) {
                    if (width == 8 || width == 16) {
                        if (initializer_real_to_integer((InitializerExtendedReal *)value->low, &bits)) report_language_diagnostic(2, 56);
                    } else {
                        if (convert_initializer_float_to_long((const BYTE *)value->low, &converted)) report_language_diagnostic(2, 56);
#ifdef __linux__
                        recon_unsupported("floating initializer carried high stack word is not yet qualified");
#else
                        __asm { int 3 }
#endif
                        bits = (unsigned)converted;
                    }
                }
                if (width == 1) INIT_BYTE((BYTE)bits);
                else if (width == 2 || width == 4) {
                    unsigned shift = width == 2 ? 48 : 32;
                    narrowed = (unsigned __int64)((__int64)(bits << shift) >> shift);
                    write_initializer_integer(&narrowed, width, output);
                } else if (width == 8 && c1_data.internal_zb_value >= 8) write_initializer_integer(&bits, 8, output);
                else if (width == 16 && c1_data.internal_zb_value >= 16) {
#ifdef __linux__
                    recon_unsupported("sixteen-byte initializer integer span is not yet qualified");
#else
                    __asm { int 3 }
#endif
                    narrowed = 0;
                    write_initializer_integer(&narrowed, 16, output);
                }
            }
            break;
        }
    }
}

/* Genuine pending children; no conversion or storage semantics are supplied. */
extern "C" void __cdecl write_initializer_symbol(CompilerIrSymbol *symbol,
    CompilerParserNode *value, CompilerOutputBuffer *output)
{
    const char *format;
    CompilerIrSymbol *target;
    unsigned index;
    if (c1_data.function_record_flags & 0x800) return;
    format = initializer_formats[0];
    while (format && *format) {
        switch (*format++) {
        case 'm': INIT_BYTE(0); break;
        case 'o':
            output->good &= output->Ensure(6);
            if (output->good) *output->cursor++ = 0;
            break;
        case 'x': write_initializer_bytes(symbol, value, output); break;
        case 'd':
            if (symbol->declaration_flags & 0x40) {
                resolve_symbol_alias(symbol)->declaration_flags |= 4;
                target = resolve_symbol_alias(symbol);
            } else target = symbol;
            if (c1_data.c9_il) index = target->record_id & 0xffffff;
            else index = (target->reference_flags & ~7U) ? target->reference_flags >> 3 : assign_symbol_output_index(target);
            output->good &= output->Ensure(c1_data.c9_il ? 5 : 4);
            if (output->good) {
                if (c1_data.c9_il) {
                    if ((int)index > 0x7fff) {
                        *(DWORD *)output->cursor = (((index & 0xffffc000U) | 0x4000) << 1) | (index & 0x7fff);
                        output->cursor += 4;
                    } else { *(WORD *)output->cursor = (WORD)index; output->cursor += 2; }
                } else if (index >= 0x3fff80) *output->cursor++ = (BYTE)(index + 0x80);
                else if (index <= 0x3fff) {
                    *output->cursor++ = (BYTE)((index & 0x3f) | 0x80);
                    *output->cursor++ = (BYTE)(index >> 6);
                } else {
                    *output->cursor++ = (BYTE)(index | 0xc0);
                    *output->cursor++ = (BYTE)(index >> 6);
                    *output->cursor++ = (BYTE)(index >> 14);
                }
            }
            symbol->declaration_flags |= 4;
            symbol->qualifier_flags |= 0x80;
            break;
        }
    }
}
extern "C" unsigned __cdecl get_initializer_storage_size(int) { __asm { int 3 } return 0; }
extern "C" void __cdecl write_initializer_integer(const unsigned __int64 *value,
    unsigned width, CompilerOutputBuffer *output)
{
    WORD short_value;
    DWORD long_value;
    __int64 wide_value;
    unsigned offset;
    if (width != 2 && width != 4 && width != 8 && width != 16) {
#ifdef __linux__
        recon_unsupported("initializer integer width is not yet qualified");
#else
        __asm { int 3 }
#endif
    }
    if (width == 2) {
        short_value = *(const WORD *)value;
        if (!INIT_RESERVE(4)) return;
        if (!((int)(short)short_value & ~0x7f)) *output->cursor++ = (BYTE)short_value;
        else {
            *output->cursor++ = 0x80;
            *(WORD *)output->cursor = short_value;
            output->cursor += 2;
        }
    } else if (width == 4) {
        long_value = *(const DWORD *)value;
        INIT_COMPACT(long_value);
    } else {
        output->good &= output->Ensure(width + 2);
        if (!output->good) return;
        wide_value = (__int64)*value;
        if (wide_value >= -127 && wide_value <= 127) *output->cursor++ = (BYTE)wide_value;
        else {
            *output->cursor++ = 0x80;
            for (offset = 0; (int)offset < (int)width; offset += 4) {
                *(DWORD *)output->cursor = ((const DWORD *)value)[offset / 4];
                output->cursor += 4;
            }
        }
    }
}
extern "C" const char *__cdecl format_initializer_integer(const unsigned __int64 *, char *, unsigned, int) { __asm { int 3 } return NULL; }
extern "C" int __cdecl parse_initializer_real(const char *, InitializerExtendedReal *) { __asm { int 3 } return 0; }
extern "C" int __cdecl initializer_real_to_double(const InitializerExtendedReal *, double *) { __asm { int 3 } return 0; }
extern "C" int __cdecl initializer_real_to_float(const InitializerExtendedReal *, float *) { __asm { int 3 } return 0; }
extern "C" int __cdecl initializer_real_to_integer(const InitializerExtendedReal *, unsigned __int64 *) { __asm { int 3 } return 0; }

#undef INIT_COMPACT
#undef INIT_BYTE
#undef INIT_RESERVE
