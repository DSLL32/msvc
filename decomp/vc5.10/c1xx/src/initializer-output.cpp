#include <string.h>
#include "frontend-init.h"
#include "global-output.h"
#include "parser-actions.h"
#pragma intrinsic(memcpy)
#include <recon.h>

extern "C" {
CompilerSymbol *__cdecl get_template_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl resolve_generated_symbol(CompilerSymbol *);
unsigned __cdecl external_symbol_index(CompilerSymbol *);
void __cdecl write_initializer_value(CompilerSymbol *, CompilerExpression *, OutputBuffer *); /*1c7ff*/
int __cdecl classify_symbol_initializer(CompilerSymbol *, CompilerExpression *);
int __cdecl decode_static_initializer(CompilerExpression *, CompilerSymbol **, unsigned *);
CompilerExpression *__cdecl prepare_ir_expression(CompilerExpression *); /*2bd4b*/
void __cdecl normalize_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl get_import_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl resolve_symbol_reference(CompilerSymbol *);
unsigned __cdecl global_type_size(CompilerType *);
unsigned __cdecl global_array_element_size(CompilerGlobalSymbolView *);
unsigned __cdecl initializer_storage_size(unsigned); /*8e0b6*/
void __cdecl write_initializer_integer(const void *, unsigned, OutputBuffer *); /*2d28b*/
int __cdecl initializer_real_to_double(const void *, double *); /*4b107*/
int __cdecl initializer_real_to_float(const void *, float *); /*a1b2d*/
int __cdecl initializer_real_to_long(const void *, LONG *); /*a1b76*/
int __cdecl initializer_real_to_integer(const void *, unsigned __int64 *); /*85aec*/
void __cdecl compiler_warning(int, ...);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
}

/* Only formats selected by encountered record-kind producers are present.
 * The unrelated second DWORD of each original row is not represented. */
static const char *initializer_formats[9] = {
    "mdox", "tpzv", "tsfz", "tl", NULL, NULL, NULL, NULL, "tn"
};

#define INIT_ROOM(n) (output->cursor + (n) <= output->data + output->capacity ? 1 : output->Grow(n))
#define INIT_BYTE(v) do { int room = INIT_ROOM(2); output->ok &= room; \
    if (output->ok) *output->cursor++ = (BYTE)(v); } while (0)
#define INIT_DWORD(v) do { unsigned encoded = (unsigned)(v); int room = output->Ensure(6); \
    output->ok &= room; if (output->ok) { \
        if (encoded & ~0x7fU) { *output->cursor++ = 0x80; *(DWORD *)output->cursor = encoded; output->cursor += 4; } \
        else *output->cursor++ = (BYTE)encoded; } } while (0)

extern "C" void __cdecl annotate_initializer(CompilerSymbol *input,
    CompilerExpression *value, OutputBuffer *output)
{
    /* The first operand-format row at D06C0 describes the symbol header.
     * Other rows are not read by this owner. */
    const char *format = initializer_formats[0];
    CompilerIrSymbol *target;
    unsigned index;
    int available, il;
    if (frontend_data.source_mode_flags & 0x800) return;
    while (*format) {
        switch (*format++) {
        case 'm':
            available = output->cursor + 2 <= output->data + output->capacity ? 1 : output->Grow(2);
            output->ok &= available;
            if (output->ok) *output->cursor++ = 0;
            break;
        case 'd':
            target = (CompilerIrSymbol *)input;
            if (input->attributes & 0x40) {
                if (input->kind == 3) target = (CompilerIrSymbol *)resolve_generated_symbol(input);
                else if (input->kind == 10) target = (CompilerIrSymbol *)get_template_symbol(input);
                else { __asm { int 3 } } /* original would use a NULL alias */
                target->attributes |= 4;
                if (input->kind == 3) target = (CompilerIrSymbol *)resolve_generated_symbol(input);
                else if ((input->attributes & 0x40) && input->kind == 10)
                    target = (CompilerIrSymbol *)get_template_symbol(input);
                else { __asm { int 3 } }
            }
            if (frontend_data.option_c9il) index = target->encoded_index & 0xffffff;
            else index = (target->emission_state & ~7U) ? target->emission_state >> 3 :
                external_symbol_index(target);
            il = frontend_data.option_c9il;
            available = output->Ensure(il ? 5 : 4);
            output->ok &= available;
            if (output->ok) {
                if (il) {
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
            input->attributes |= 4;
            input->qualifiers |= 0x80;
            break;
        case 'o':
            available = output->Ensure(6);
            output->ok &= available;
            if (output->ok) *output->cursor++ = 0;
            break;
        case 'x': write_initializer_value(input, value, output); break;
        }
    }
}

extern "C" void __cdecl write_initializer_value(CompilerSymbol *input,
    CompilerExpression *value, OutputBuffer *output)
{
    CompilerSymbol *reference = NULL;
    CompilerIrSymbol *target, *imported;
    CompilerType *type;
    CompilerTypeShape *shape;
    const char *format;
    unsigned displacement = 0, record, encoding = 0, width, index, flags;
    int literal_string = 0, integer_value, room, il;
    unsigned __int64 bits, narrowed;
    LONG converted;
    double real64;
    float real32;
    if (!value || (frontend_data.source_mode_flags & 0x800)) return;
    if (frontend_data.last_initializer_symbol && output == &frontend_bss.output_buffers[4]) {
        annotate_initializer(frontend_data.last_initializer_symbol, NULL, &frontend_bss.output_buffers[4]);
        flags = frontend_data.compiler_flags | 0x40000;
        frontend_data.last_initializer_symbol = NULL;
        frontend_data.compiler_flags = flags;
    }
    if (!input) record = 8;
    else if ((value->flags & 7) == 3) record = 1;
    else if ((value->flags & 7) == 5) {
        literal_string = 1;
        if (classify_symbol_initializer(input, value)) record = 3;
        else {
            value = prepare_ir_expression(value);
            if (!decode_static_initializer(value, &reference, &displacement)) {
                __asm { int 3 } /* original record-kind local is unwritten here */
                return;
            }
            record = 2;
        }
    } else {
        if (!decode_static_initializer(value, &reference, &displacement)) {
            __asm { int 3 } /* original record-kind local is unwritten here */
            return;
        }
        if (reference) record = 2;
        else { value = (CompilerExpression *)make_integer_constant((int)displacement); record = 1; }
    }
    format = initializer_formats[record];
    while (format && *format) {
        switch (*format++) {
        case 't': INIT_BYTE(record); break;
        case 'i': INIT_BYTE(0); break;
        case 'f': INIT_DWORD(displacement); break;
        case 'n': INIT_DWORD(value->value.words.low); break;
        case 'l': {
            WORD length = value->value.quoted.length;
            room = INIT_ROOM((unsigned)length + 4);
            output->ok &= room;
            if (output->ok) {
                if ((int)(short)length & ~0x7f) {
                    *output->cursor++ = 0x80;
                    *(WORD *)output->cursor = length;
                    output->cursor += 2;
                } else *output->cursor++ = (BYTE)length;
                memcpy(output->cursor, value->value.quoted.text, length);
                output->cursor += length;
            }
            break;
        }
        case 's':
            target = (CompilerIrSymbol *)reference;
            if (target->kind == 8) normalize_symbol(target);
            if ((target->emission_state & 3) < 3) ++target->emission_state;
            target->emission_state = (target->emission_state & ~4U) | ((frontend_data.reference_context & 1) << 2);
            if (target->attributes & 0x40) {
                if (target->kind == 3) target = (CompilerIrSymbol *)resolve_generated_symbol(target);
                else if (target->kind == 10) target = (CompilerIrSymbol *)get_template_symbol(target);
                else { __asm { int 3 } return; }
                if ((target->emission_state & 3) < 3) ++target->emission_state;
                target->emission_state = (target->emission_state & ~4U) | ((frontend_data.reference_context & 1) << 2);
            } else if (target->kind == 7 || target->kind == 8) {
                CompilerRuntimeFunction *function = (CompilerRuntimeFunction *)target;
                if (function->queue_flags & 0x80) frontend_bss.pending_groups.Add(target);
                if (function->linkage_flags & 2) {
                    imported = (CompilerIrSymbol *)get_import_symbol(target);
                    if ((imported->emission_state & 3) < 3) ++imported->emission_state;
                    imported->emission_state = (imported->emission_state & ~4U) |
                        ((frontend_data.reference_context & 1) << 2);
                }
            }
            if ((target->reference_flags & 1) &&
                (((CompilerIrSymbol *)resolve_symbol_reference(target))->encoded_index & 0xffffff) == 5 &&
                frontend_bss.current_function &&
                !(((CompilerIrSymbol *)frontend_bss.current_function)->function_flags & 1)) {
                frontend_data.force_type_serialization = 1;
                frontend_data.type_debug_mode = 0;
            }
            if (frontend_data.option_c9il) index = target->encoded_index & 0xffffff;
            else index = (target->emission_state & ~7U) ? target->emission_state >> 3 : external_symbol_index(target);
            il = frontend_data.option_c9il;
            room = output->Ensure(il ? 5 : 4);
            output->ok &= room;
            if (output->ok) {
                if (il) {
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
            type = ((CompilerIrSymbol *)input)->type;
            shape = type->shape;
            flags = type->flags;
            if (shape && (shape->flags & 0x21)) encoding = 3;
            else if (shape && (shape->flags & 0x10)) encoding = 1;
            else if (flags & 0x4000) encoding = 2;
            else if (frontend_data.options.unsigned_char && (flags & 0x2fdf) == 0x41 && !(flags & 0x8000)) encoding = 2;
            else if (flags & 0x40) encoding = 1;
            else if (flags & 0x100) encoding = 5;
            else if (flags & 0x800) encoding = 2;
            else if (flags & 0x200) encoding = 1;
            INIT_BYTE(encoding);
            break;
        case 'z':
            type = ((CompilerIrSymbol *)input)->type;
            shape = type->shape;
            flags = shape ? shape->flags : 0;
            width = (flags & 0x77) == 2 ? global_array_element_size((CompilerGlobalSymbolView *)input) :
                global_type_size(type);
            if (record == 2 && (!literal_string || width == (unsigned)(signed char)compiler_primitive_type_bytes[1]) &&
                !(type->flags & 0x40) && !(type->flags & 0x800) && !(flags & 0x71)) {
                compiler_warning(0x61);
                width = initializer_storage_size(width);
            }
            INIT_BYTE(width);
            break;
        case 'v':
            type = value->type;
            shape = type->shape;
            flags = type->flags;
            integer_value = (flags & 0x40) || (shape && (shape->flags & 0x11)) || (flags & 0x200);
            if (!integer_value && !(flags & 0x100)) {
                __asm { int 3 } /* original integer flag/value stack storage is unwritten */
                return;
            }
            if (integer_value) bits = (unsigned __int64)value->value.integer;
            if (encoding == 5) {
                if (width == 8) {
                    initializer_real_to_double(value->value.pointer, &real64);
                    room = INIT_ROOM(9); output->ok &= room;
                    if (output->ok) { memcpy(output->cursor, &real64, 8); output->cursor += 8; }
                } else {
                    initializer_real_to_float(value->value.pointer, &real32);
                    room = INIT_ROOM(5); output->ok &= room;
                    if (output->ok) { memcpy(output->cursor, &real32, 4); output->cursor += 4; }
                }
            } else if (width == 1) {
                if (!integer_value) {
                    if (initializer_real_to_long(value->value.pointer, &converted)) report_language_diagnostic(2, 0x38);
                    bits = (unsigned)converted;
                }
                INIT_BYTE((BYTE)bits);
            } else if (width == 2 || width == 4) {
                if (!integer_value) {
                    if (initializer_real_to_long(value->value.pointer, &converted)) report_language_diagnostic(2, 0x38);
                    __asm { int 3 } /* high carried stack word remains unqualified */
                    return;
                }
                unsigned shift = width == 2 ? 48 : 32;
                narrowed = (unsigned __int64)((__int64)(bits << shift) >> shift);
                write_initializer_integer(&narrowed, width, output);
            } else if (width == 8 && frontend_data.option_zb >= 8) {
                if (!integer_value && initializer_real_to_integer(value->value.pointer, &bits))
                    report_language_diagnostic(2, 0x38);
                write_initializer_integer(&bits, 8, output);
            } else if (width == 16 && frontend_data.option_zb >= 16) {
                if (!integer_value && initializer_real_to_integer(value->value.pointer, &bits))
                    report_language_diagnostic(2, 0x38);
                __asm { int 3 } /* original16-byte transfer exceeds the established8-byte value */
                return;
            }
            break;
        }
    }
}

extern "C" unsigned __cdecl initializer_storage_size(unsigned) { __asm { int 3 } return 0; }
extern "C" void __cdecl write_initializer_integer(const void *input, unsigned width, OutputBuffer *output)
{
    int room, offset;
    if (width != 2 && width != 4 && width != 8 && width != 16) {
        __asm { int 3 } /* other input-span contracts remain unqualified */
        return;
    }
    if (width == 2) {
        WORD value = *(const WORD *)input;
        room = INIT_ROOM(4);
        output->ok &= room;
        if (!output->ok) return;
        if ((int)(short)value & ~0x7f) {
            *output->cursor++ = 0x80;
            *(WORD *)output->cursor = value;
            output->cursor += 2;
        } else *output->cursor++ = (BYTE)value;
    } else if (width == 4) {
        INIT_DWORD(*(const DWORD *)input);
    } else {
        room = output->Ensure(width + 2);
        output->ok &= room;
        if (!output->ok) return;
        __int64 value = *(const __int64 *)input;
        if (value >= -127 && value <= 127) *output->cursor++ = (BYTE)value;
        else {
            *output->cursor++ = 0x80;
            for (offset = 0; offset < (int)width; offset += 4) {
                *(DWORD *)output->cursor = ((const DWORD *)input)[offset / 4];
                output->cursor += 4;
            }
        }
    }
}
extern "C" int __cdecl initializer_real_to_double(const void *, double *) { __asm { int 3 } return 0; }
extern "C" int __cdecl initializer_real_to_float(const void *, float *) { __asm { int 3 } return 0; }
extern "C" int __cdecl initializer_real_to_long(const void *, LONG *) { __asm { int 3 } return 0; }
extern "C" int __cdecl initializer_real_to_integer(const void *, unsigned __int64 *) { __asm { int 3 } return 0; }
