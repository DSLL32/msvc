#include "frontend.h"
#include "frontend-init.h"
#include "ir.h"
#include "global-output.h"
#include "pointer-array.h"
#include <recon.h>

/* Each opcode selects a sequence of operand encodings. The original table's
 * unrelated second DWORD is not consumed here and has no projected field. */
enum IrOperandEncoding {
    IR_OPERAND_IS_LABEL = 1,
    IR_UNSIGNED_VALUE = 2,
    IR_TYPE_CLASS = 3,
    IR_UNUSED_4 = 4,
    IR_UNUSED_5 = 5,
    IR_TYPE = 6,
    IR_ZERO = 7,
    IR_SYMBOL = 8,
    IR_UNUSED_9 = 9,
    IR_UNUSED_10 = 10,
    IR_CONSTANT = 11,
    IR_UNUSED_12 = 12,
    IR_ZERO_13 = 13,
    IR_ZERO_14 = 14,
    IR_END_OPERAND = 15,
    IR_UNUSED_16 = 16,
    IR_INDEX = 17,
    IR_UNUSED_18 = 18,
    IR_UNUSED_19 = 19,
    IR_HIGH_WORD = 20,
    IR_STRING = 21,
    IR_LOW_12_BITS = 22,
    IR_UNUSED_25 = 25,
    IR_UNUSED_112 = 112
};
static const BYTE opcode_formats[][6] = {
    { IR_END_OPERAND, 0 }, /* 0x0 */
    { IR_END_OPERAND, 0 }, /* 0x1 */
    { 0 }, /* 0x2 */
    { 0 }, /* 0x3 */
    { 0 }, /* 0x4 */
    { 0 }, /* 0x5 */
    { 0 }, /* 0x6 */
    { 0 }, /* 0x7 */
    { 0 }, /* 0x8 */
    { 0 }, /* 0x9 */
    { 0 }, /* 0xa */
    { 0 }, /* 0xb */
    { 0 }, /* 0xc */
    { 0 }, /* 0xd */
    { 0 }, /* 0xe */
    { IR_TYPE, 0 }, /* 0xf */
    { IR_TYPE, 0 }, /* 0x10 */
    { IR_TYPE, 0 }, /* 0x11 */
    { IR_TYPE, 0 }, /* 0x12 */
    { IR_TYPE, 0 }, /* 0x13 */
    { IR_TYPE, 0 }, /* 0x14 */
    { IR_TYPE, 0 }, /* 0x15 */
    { IR_TYPE, 0 }, /* 0x16 */
    { IR_TYPE, 0 }, /* 0x17 */
    { IR_TYPE, 0 }, /* 0x18 */
    { IR_TYPE, 0 }, /* 0x19 */
    { 0 }, /* 0x1a */
    { 0 }, /* 0x1b */
    { 0 }, /* 0x1c */
    { 0 }, /* 0x1d */
    { 0 }, /* 0x1e */
    { 0 }, /* 0x1f */
    { 0 }, /* 0x20 */
    { 0 }, /* 0x21 */
    { 0 }, /* 0x22 */
    { 0 }, /* 0x23 */
    { 0 }, /* 0x24 */
    { 0 }, /* 0x25 */
    { IR_SYMBOL, 0 }, /* 0x26 */
    { IR_TYPE, 0 }, /* 0x27 */
    { IR_SYMBOL, 0 }, /* 0x28 */
    { IR_SYMBOL, IR_END_OPERAND, 0 }, /* 0x29 */
    { IR_SYMBOL, IR_TYPE, IR_ZERO_13, IR_OPERAND_IS_LABEL, IR_END_OPERAND, 0 }, /* 0x2a */
    { IR_SYMBOL, IR_OPERAND_IS_LABEL, IR_END_OPERAND, 0 }, /* 0x2b */
    { IR_TYPE, IR_ZERO_14, 0 }, /* 0x2c */
    { IR_SYMBOL, IR_END_OPERAND, 0 }, /* 0x2d */
    { 0 }, /* 0x2e */
    { IR_TYPE, 0 }, /* 0x2f */
    { IR_TYPE, 0 }, /* 0x30 */
    { IR_TYPE, 0 }, /* 0x31 */
    { IR_TYPE, 0 }, /* 0x32 */
    { IR_CONSTANT, 0 }, /* 0x33 */
    { IR_TYPE, IR_ZERO_14, 0 }, /* 0x34 */
    { IR_TYPE, 0 }, /* 0x35 */
    { IR_TYPE, 0 }, /* 0x36 */
    { 0 }, /* 0x37 */
    { IR_SYMBOL, IR_END_OPERAND, 0 }, /* 0x38 */
    { IR_SYMBOL, IR_END_OPERAND, 0 }, /* 0x39 */
    { IR_SYMBOL, IR_END_OPERAND, 0 }, /* 0x3a */
    { IR_SYMBOL, IR_END_OPERAND, 0 }, /* 0x3b */
    { IR_TYPE, IR_SYMBOL, IR_END_OPERAND, 0 }, /* 0x3c */
    { IR_SYMBOL, IR_UNUSED_5, IR_END_OPERAND, 0 }, /* 0x3d */
    { IR_TYPE, IR_TYPE_CLASS, 0 }, /* 0x3e */
    { IR_TYPE_CLASS, 0 }, /* 0x3f */
    { IR_TYPE, 0 }, /* 0x40 */
    { IR_TYPE, IR_END_OPERAND, 0 }, /* 0x41 */
    { 0 }, /* 0x42 */
    { 0 }, /* 0x43 */
    { 0 }, /* 0x44 */
    { IR_UNUSED_9, IR_END_OPERAND, 0 }, /* 0x45 */
    { IR_END_OPERAND, 0 }, /* 0x46 */
    { IR_END_OPERAND, 0 }, /* 0x47 */
    { IR_END_OPERAND, 0 }, /* 0x48 */
    { IR_SYMBOL, IR_END_OPERAND, 0 }, /* 0x49 */
    { IR_END_OPERAND, 0 }, /* 0x4a */
    { IR_END_OPERAND, 0 }, /* 0x4b */
    { IR_END_OPERAND, 0 }, /* 0x4c */
    { 0 }, /* 0x4d */
    { 0 }, /* 0x4e */
    { IR_UNUSED_4, IR_END_OPERAND, 0 }, /* 0x4f */
    { IR_UNSIGNED_VALUE, 0 }, /* 0x50 */
    { IR_UNSIGNED_VALUE, 0 }, /* 0x51 */
    { IR_UNSIGNED_VALUE, IR_END_OPERAND, 0 }, /* 0x52 */
    { IR_END_OPERAND, 0 }, /* 0x53 */
    { IR_UNSIGNED_VALUE, IR_END_OPERAND, 0 }, /* 0x54 */
    { IR_TYPE, IR_END_OPERAND, 0 }, /* 0x55 */
    { IR_END_OPERAND, 0 }, /* 0x56 */
    { IR_END_OPERAND, 0 }, /* 0x57 */
    { IR_TYPE, 0 }, /* 0x58 */
    { 0 }, /* 0x59 */
    { IR_TYPE, 0 }, /* 0x5a */
    { IR_UNSIGNED_VALUE, 0 }, /* 0x5b */
    { IR_TYPE, IR_HIGH_WORD, 0 }, /* 0x5c */
    { IR_LOW_12_BITS, IR_HIGH_WORD, 0 }, /* 0x5d */
    { IR_LOW_12_BITS, IR_HIGH_WORD, 0 }, /* 0x5e */
    { 0 }, /* 0x5f */
    { IR_END_OPERAND, 0 }, /* 0x60 */
    { IR_STRING, 0 }, /* 0x61 */
    { IR_END_OPERAND, 0 }, /* 0x62 */
    { IR_SYMBOL, 0 }, /* 0x63 */
    { IR_TYPE, IR_END_OPERAND, 0 }, /* 0x64 */
    { IR_SYMBOL, IR_END_OPERAND, 0 }, /* 0x65 */
    { 0 }, /* 0x66 */
    { 0 }, /* 0x67 */
    { 0 }, /* 0x68 */
    { IR_SYMBOL, 0 }, /* 0x69 */
    { 0 }, /* 0x6a */
    { 0 }, /* 0x6b */
    { 0 }, /* 0x6c */
    { 0 }, /* 0x6d */
    { 0 }, /* 0x6e */
    { 0 }, /* 0x6f */
    { 0 }, /* 0x70 */
    { 0 }, /* 0x71 */
    { 0 }, /* 0x72 */
    { 0 }, /* 0x73 */
    { 0 }, /* 0x74 */
    { 0 }, /* 0x75 */
    { 0 }, /* 0x76 */
    { IR_TYPE, 0 }, /* 0x77 */
    { IR_TYPE, IR_ZERO, IR_END_OPERAND, 0 }, /* 0x78 */
    { 0 }, /* 0x79 */
    { 0 }, /* 0x7a */
    { 0 }, /* 0x7b */
    { 0 }, /* 0x7c */
    { IR_TYPE, 0 }, /* 0x7d */
    { IR_TYPE, IR_TYPE_CLASS, 0 }, /* 0x7e */
    { IR_UNUSED_12, IR_END_OPERAND, 0 }, /* 0x7f */
    { 0 }, /* 0x80 */
    { IR_UNUSED_16, 0 }, /* 0x81 */
    { IR_UNUSED_16, 0 }, /* 0x82 */
    { IR_UNUSED_16, 0 }, /* 0x83 */
    { 0 }, /* 0x84 */
    { 0 }, /* 0x85 */
    { 0 }, /* 0x86 */
    { IR_UNUSED_112, 0 }, /* 0x87 */
    { 0 }, /* 0x88 */
    { 0 }, /* 0x89 */
    { 0 }, /* 0x8a */
    { 0 }, /* 0x8b */
    { 0 }, /* 0x8c */
    { IR_TYPE, IR_END_OPERAND, 0 }, /* 0x8d */
    { IR_END_OPERAND, 0 }, /* 0x8e */
    { IR_END_OPERAND, 0 }, /* 0x8f */
    { IR_END_OPERAND, 0 }, /* 0x90 */
    { 0 }, /* 0x91 */
    { 0 }, /* 0x92 */
    { 0 }, /* 0x93 */
    { 0 }, /* 0x94 */
    { 0 }, /* 0x95 */
    { 0 }, /* 0x96 */
    { 0 }, /* 0x97 */
    { 0 }, /* 0x98 */
    { IR_TYPE, 0 }, /* 0x99 */
    { IR_TYPE, 0 }, /* 0x9a */
    { IR_TYPE, IR_INDEX, 0 }, /* 0x9b */
    { IR_TYPE, 0 }, /* 0x9c */
    { IR_END_OPERAND, 0 }, /* 0x9d */
    { 0 }, /* 0x9e */
    { 0 }, /* 0x9f */
    { 0 }, /* 0xa0 */
    { 0 }, /* 0xa1 */
    { 0 }, /* 0xa2 */
    { 0 }, /* 0xa3 */
    { 0 }, /* 0xa4 */
    { 0 }, /* 0xa5 */
    { 0 }, /* 0xa6 */
    { 0 }, /* 0xa7 */
    { 0 }, /* 0xa8 */
    { 0 }, /* 0xa9 */
    { 0 }, /* 0xaa */
    { 0 }, /* 0xab */
    { 0 }, /* 0xac */
    { 0 }, /* 0xad */
    { 0 }, /* 0xae */
    { 0 }, /* 0xaf */
    { 0 }, /* 0xb0 */
    { 0 }, /* 0xb1 */
    { 0 }, /* 0xb2 */
    { 0 }, /* 0xb3 */
    { IR_UNUSED_25, 0 }, /* 0xb4 */
    { 0 } /* invalid opcode b5, diagnosed before lookup */
};

extern "C" {
void __cdecl write_current_source_line(void);
void __cdecl serialize_ir_type(int, CompilerExpression *, int);
void __cdecl prepare_ir_symbol_reference(CompilerSymbol *);
int __cdecl needs_external_symbol(unsigned);
unsigned __cdecl get_symbol_index(CompilerSymbol *);
void __cdecl normalize_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl get_template_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl resolve_generated_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl get_import_symbol(CompilerSymbol *);
CompilerSymbol *__cdecl resolve_symbol_reference(CompilerSymbol *);
CompilerType *__cdecl create_default_type(void);
void __cdecl write_integer_payload(unsigned, unsigned, int);
unsigned __cdecl external_symbol_index(CompilerSymbol *);
}

/* These are inline stores in the original, not additional graph vertices. */
#define ROOM(bytes) (buffer->ok &= buffer->Ensure(bytes))
#define BYTE_OUT(value) (*buffer->cursor++ = (BYTE)(value))
#define DWORD_OUT(value) do { \
    *(DWORD *)buffer->cursor = (DWORD)(value); buffer->cursor += 4; \
} while (0)
#define WIDE_VALUE(value) do { BYTE_OUT(0x80); DWORD_OUT(value); } while (0)
#define INDEX_OUT(value) do { \
    if ((value) >= 0x3fff80U) BYTE_OUT((value) + 0x80); \
    else if ((value) <= 0x3fffU) { \
        BYTE_OUT(((value) & 0x3f) | 0x80); BYTE_OUT((value) >> 6); \
    } else { \
        BYTE_OUT((value) | 0xc0); BYTE_OUT((value) >> 6); BYTE_OUT((value) >> 14); \
    } \
} while (0)
#define REFERENCE_SYMBOL(symbol) do { \
    unsigned references = ((CompilerIrSymbol *)(symbol))->emission_state & 3; \
    if (references < 3) \
        ((CompilerIrSymbol *)(symbol))->emission_state = (((CompilerIrSymbol *)(symbol))->emission_state & ~3U) | (references + 1); \
    ((CompilerIrSymbol *)(symbol))->emission_state = (((CompilerIrSymbol *)(symbol))->emission_state & ~4U) | \
        ((frontend_data.reference_context & 1) << 2); \
} while (0)

extern "C" void __cdecl write_ir_record(int kind, const void *value)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[3];
    CompilerExpression *node = (CompilerExpression *)value;
    CompilerExpression *operand;
    CompilerSymbol *symbol, *resolved;
    CompilerType *type;
    const BYTE *format, *bytes;
    unsigned string_owner_reference, string_data_reference;
    unsigned index, length, string_tag, number;
    /* The original reuses its incoming node slot for this temporary. Valid
     * type classes overwrite it; an unsupported class exposes its previous
     * value, initially an address. No address-byte equivalence is claimed. */
    unsigned type_class = (unsigned)value;
    static const signed char integer_bytes[16] = {
        1, 1, 2, 4, 4, 2, 4, 8, 8, 1, 2, 4, 8, 16, 0, 0
    };

    if (kind == 0xb5)
        report_compiler_diagnostic(1, ".\\toil.c", 0xe45);
    write_current_source_line();
    if (ROOM(2)) BYTE_OUT(kind);
    ++frontend_bss.ir_record_count;
    for (format = opcode_formats[kind]; *format; ++format) {
        switch (*format) {
        case IR_OPERAND_IS_LABEL:
            number = node->value.operands.first->kind == 11;
            if (ROOM(2)) BYTE_OUT(number);
            break;
        case IR_UNSIGNED_VALUE:
            number = node->value.words.low;
            if (ROOM(6)) {
                if (!(number & ~0x7fU)) BYTE_OUT(number);
                else WIDE_VALUE(number);
            }
            break;
        case IR_TYPE_CLASS:
            operand = node->value.operands.first;
            if (operand && operand->kind == 0x37)
                operand = operand->value.operands.second;
            type = (CompilerType *)operand->type->shape;
            frontend_data.force_type_serialization = 1;
            switch (type->attributes & 0x70) {
            case 0x10: type_class = 0; break;
            case 0x20: type_class = 8; break;
            case 0x40: type_class = 4; break;
            case 0x50: type_class = 24; break;
            case 0x60:
                if ((type->flags & 0x77) != 4)
                    type = (CompilerType *)type->shape;
                type_class = (~type->flags >> 11) & 0x10;
                break;
            case 0x70: type_class = 20; break;
            }
            if (ROOM(2)) BYTE_OUT(type_class);
            break;
        case IR_TYPE:
            serialize_ir_type(kind, node, 2);
            break;
        case IR_ZERO:
        case IR_ZERO_13:
        case IR_ZERO_14:
            if (ROOM(2)) BYTE_OUT(0);
            break;
        case IR_SYMBOL:
            symbol = NULL;
            index = 0;
            switch (kind) {
            case 0x26:
                symbol = node->value.symbol;
                if (needs_external_symbol(((CompilerIrSymbol *)symbol)->emission_state >> 3))
                    frontend_bss.debug_module->Slot80();
                break;
            case 0x27:
            case 0x28:
                break;
            case 0x29:
            case 0x2a:
            case 0x2b:
            case 0x2d:
                resolved = node->value.symbol;
                if (frontend_data.option_c9il)
                    index = ((CompilerIrSymbol *)resolved)->encoded_index & 0xffffff;
                else if (!(((CompilerIrSymbol *)resolved)->emission_state & ~7U))
                    index = external_symbol_index(resolved);
                else
                    index = ((CompilerIrSymbol *)resolved)->emission_state >> 3;
                break;
            case 0x38:
            case 0x39:
            case 0x3a:
                symbol = node->value.symbol;
                prepare_ir_symbol_reference(symbol);
                break;
            case 0x3b:
            case 0x3c:
                symbol = frontend_bss.current_ir_symbol;
                REFERENCE_SYMBOL(symbol);
                prepare_ir_symbol_reference(symbol);
                break;
            case 0x3d:
                index = node->value.words.low;
                break;
            default:
                symbol = node->value.symbol;
                break;
            }
            if (symbol) {
                if (symbol->kind == 8) normalize_symbol(symbol);
                REFERENCE_SYMBOL(symbol);
                if (symbol->attributes & 0x40) {
                    if (symbol->kind == 3)
                        symbol = resolve_generated_symbol(symbol);
                    else if (symbol->kind == 10)
                        symbol = get_template_symbol(symbol);
                    else
                        symbol = NULL;
                    symbol->MarkReferenced();
                } else if (symbol->kind == 7 || symbol->kind == 8) {
                    if (((CompilerRuntimeFunction *)symbol)->queue_flags & 0x80)
                        frontend_bss.pending_groups.Add(symbol);
                    if (((CompilerRuntimeFunction *)symbol)->linkage_flags & 2)
                        get_import_symbol(symbol)->MarkReferenced();
                }
                if (((CompilerIrSymbol *)symbol)->reference_flags & 1) {
                    resolved = resolve_symbol_reference(symbol);
                    if ((((CompilerIrSymbol *)resolved)->encoded_index & 0xffffff) == 5 &&
                        frontend_bss.current_function &&
                        !(((CompilerIrSymbol *)frontend_bss.current_function)->function_flags & 1)) {
                        frontend_data.force_type_serialization = 1;
                        frontend_data.type_debug_mode = 0;
                    }
                }
                index = get_symbol_index(symbol);
            }
            if (frontend_data.option_c9il) buffer->WriteIndex(index);
            else if (ROOM(4)) INDEX_OUT(index);
            break;
        case IR_CONSTANT:
            type = (CompilerType *)node->type->shape;
            if (type && (type->flags & 0x10)) node->type = create_default_type();
            serialize_ir_type(kind, node, 1);
            type = node->type;
            if (type->shape) {
                number = node->value.words.low;
                if (ROOM(6) && !buffer->WriteCompact(number)) WIDE_VALUE(number);
            } else if (type->flags & 0x240) {
                write_integer_payload(node->value.words.low, node->value.words.high,
                    integer_bytes[type->flags & 0xf]);
            } else if (type->flags & 0x100) {
                bytes = node->value.bytes;
                if (ROOM(11)) buffer->WriteBytes(bytes, 10);
                bytes = node->value.bytes + 10;
                if (ROOM(3)) buffer->WriteBytes(bytes, 2);
            } else if (type->flags & 0x80) {
                number = node->value.words.low;
                if (ROOM(6) && !buffer->WriteCompact(number)) WIDE_VALUE(number);
            }
            break;
        case IR_INDEX:
            index = node->value.words.low;
            if (frontend_data.option_c9il) buffer->WriteIndex(index);
            else if (ROOM(4)) INDEX_OUT(index);
            break;
        case IR_HIGH_WORD:
            number = (WORD)(node->value.words.high >> 16);
            if (buffer->Prepare(5) && !buffer->WriteCompact(number)) WIDE_VALUE(number);
            break;
        case IR_STRING:
            node->value.string->GetReferences(&string_owner_reference, &string_data_reference, &string_tag);
            if (frontend_data.option_c9il) buffer->WriteIndex(string_owner_reference);
            else if (buffer->Prepare(3)) INDEX_OUT(string_owner_reference);
            buffer->WriteReferenceIndex((int)string_data_reference);
            buffer->WriteStringTag(string_tag);
            break;
        case IR_LOW_12_BITS:
            buffer->WriteUShort(node->value.words.high & 0xfff);
            break;
        default:
            /* Reserved format tags consume no serialized operand. */
            break;
        }
    }
}

/* The line delta is relative to the current source-file line origin. Only
 * its signed low WORD is serialized by this compact line record. */
extern "C" void __cdecl write_current_source_line(void)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[3];
    unsigned delta = frontend_bss.ir_line_delta;
    short line;
    int good;
    if (!delta || delta == frontend_data.last_ir_line_delta)
        return;
    frontend_data.last_ir_line_delta = delta;
    good = buffer->cursor + 2 <= buffer->data + buffer->capacity || buffer->Grow(2) != 0;
    buffer->ok &= good;
    if (buffer->ok) BYTE_OUT(0x4f);
    good = buffer->cursor + 2 <= buffer->data + buffer->capacity || buffer->Grow(2) != 0;
    buffer->ok &= good;
    if (buffer->ok) BYTE_OUT(0x24);
    delta = frontend_bss.ir_line_delta;
    good = buffer->cursor + 4 <= buffer->data + buffer->capacity || buffer->Grow(4) != 0;
    buffer->ok &= good;
    if (buffer->ok) {
        line = (short)delta;
        if (!((unsigned)(int)line & ~0x7fU)) BYTE_OUT(line);
        else {
            BYTE_OUT(0x80);
            *(WORD *)buffer->cursor = (WORD)line;
            buffer->cursor += 2;
        }
    }
}
extern "C" {
unsigned __cdecl pointer_type_size(WORD);
int __cdecl is_extended_pointer_type(CompilerType *);
unsigned __cdecl classify_extended_pointer(CompilerType *); /*40399*/
unsigned __cdecl serialize_type_reference(CompilerType *); /*18eb3*/
}
#define TYPE_SPACE(n) (buffer->ok &= (buffer->cursor + (n) <= buffer->data + buffer->capacity || buffer->Grow(n) != 0))
#define TYPE_WIDTH(f) (((f) & 0x340) || !((f) & 0xfff0) ? \
    (unsigned)compiler_primitive_type_bytes[(f) & 15] : (unsigned)((f) & 15))
/* The original reuses its first two private cdecl argument slots for
 * width/ok. Both write_ir_record callers discard them with ADD ESP,12;
 * only the mutable node's type is observed after the call. */
extern "C" void __cdecl serialize_ir_type(int record, CompilerExpression *node, int mode)
{
    enum { TYPE_SIGNED = 1, TYPE_UNSIGNED = 2, TYPE_POINTER = 3, TYPE_REAL = 5,
        TYPE_REFERENCE = 6, TYPE_VOID = 7, TYPE_REAL_CONSTANT = 10 };
    OutputBuffer *buffer = &frontend_bss.output_buffers[3];
    CompilerType *type;
    CompilerTypeShape *shape;
    unsigned encoding, width, inline_width;
    WORD attributes, flags, packed;
    if (frontend_bss.error_count) return;
    type = node->type;
    shape = type->shape;
    if (shape) {
        if (!(shape->flags & 0x30) ||
            ((shape->flags & 0x20) && classify_extended_pointer(type) == 1)) {
            attributes = shape->attributes;
            encoding = TYPE_POINTER;
            width = pointer_type_size(attributes);
            goto emit_type;
        }
        if ((shape->flags & 0x30) && is_extended_pointer_type(type)) {
            attributes = shape->attributes;
            if (record == 0x27 && node->value.operands.first->kind == 0x9c) {
                encoding = TYPE_POINTER;
                width = pointer_type_size(0);
            } else {
                encoding = TYPE_REFERENCE;
                width = record == 0x27 ? 0 : serialize_type_reference(node->type);
            }
            goto emit_type;
        }
    }
    /* The original retains these attributes even when it replaces a
     * special shaped type with the default type below. */
    attributes = node->type->attributes;
    if (node->type->shape && (node->type->shape->flags & 0x10))
        node->type = create_default_type();
    type = node->type;
    flags = type->flags;
    if (flags & 0x40) {
        encoding = TYPE_SIGNED;
        if ((flags & 0x4000) || (flags & 0x2fff) == 0x60 ||
            (frontend_data.options.unsigned_char && (flags & 0x2fdf) == 0x41 && !(flags & 0x8000)))
            encoding = TYPE_UNSIGNED;
        width = TYPE_WIDTH(flags);
    } else if (flags & 0x200) {
        encoding = TYPE_SIGNED;
        width = compiler_primitive_type_bytes[3];
    } else if (flags & 0x100) {
        if (mode == 1) { encoding = TYPE_REAL_CONSTANT; width = 10; }
        else { encoding = TYPE_REAL; width = TYPE_WIDTH(flags); }
    } else if (flags & 0x400) {
        encoding = TYPE_REFERENCE;
        width = record == 0x27 ? 0 : serialize_type_reference(type);
    } else if (flags & 0x80) { encoding = TYPE_VOID; width = 0; }
    else if (flags & 0x800) { encoding = TYPE_UNSIGNED; width = 2; }
    else { encoding = 0; width = 0; }
emit_type:
    inline_width = encoding == TYPE_REFERENCE && width >= 0x20 ? 0 : width;
    packed = (WORD)(encoding | (inline_width << 4) | ((attributes & 0x100) << 5) |
        ((attributes & 0x200) << 3));
    if (packed > 0x7f) {
        if (TYPE_SPACE(2)) BYTE_OUT((packed >> 8) | 0x80);
    }
    if (TYPE_SPACE(2)) BYTE_OUT(packed);
    if (encoding == TYPE_REFERENCE && !(WORD)inline_width && TYPE_SPACE(6)) {
        if (!(width & ~0x7fU)) BYTE_OUT(width);
        else { BYTE_OUT(0x80); DWORD_OUT(width); }
    }
}
#undef TYPE_SPACE
#undef TYPE_WIDTH
extern "C" unsigned __cdecl classify_extended_pointer(CompilerType *) { __asm { int 3 } return 0; }
extern "C" unsigned __cdecl serialize_type_reference(CompilerType *) { __asm { int 3 } return 0; }
extern "C" void __cdecl compiler_warning(int, ...);
extern "C" void __cdecl prepare_ir_symbol_reference(CompilerSymbol *input)
{
    CompilerGlobalSymbolView *symbol = (CompilerGlobalSymbolView *)input;
    unsigned depth, qualifier;
    if (!(symbol->definition_flags & 4)) {
        /* Only the actual allocation-width rows are supported before the
         * extended BYTE is read or written; compact namespace records do
         * not acquire a larger view from this cast. */
        if (symbol->kind >= 26 || frontend_data.symbol_record_bytes[symbol->kind] <= 0x25) { __asm { int 3 } }
        depth = (unsigned)frontend_data.symbol_reference_depths[0] +
            (unsigned)frontend_data.symbol_reference_depths[1] +
            (unsigned)frontend_data.symbol_reference_depths[2];
        if (!(symbol->references & 3) || (int)symbol->reference_position.depth > (int)depth)
            symbol->reference_position.depth = (BYTE)depth;
    } else if (symbol->qualifiers & 2) {
        if (symbol->kind >= 26 || frontend_data.symbol_record_bytes[symbol->kind] < 24) { __asm { int 3 } }
        qualifier = symbol->symbol_value == 19 ? 0x63 : symbol->symbol_value == 20 ? 0x67 : 6;
        if (!(frontend_data.compiler_mode_flags & 0x10000)) {
            if (qualifier == 6) compiler_warning(0x2c1, 0x53, input);
            else compiler_warning(0x2c1, 0x4c, qualifier);
        }
    }
    if (symbol->definition_flags & 4) symbol->qualifiers |= 4;
}
extern "C" int __cdecl needs_external_symbol(unsigned index)
{
    if (!index) return 0;
    return index == frontend_data.debug_notification_index ? 1 : 0;
}
extern "C" unsigned __cdecl get_symbol_index(CompilerSymbol *input)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)input;
    if (frontend_data.option_c9il) return symbol->encoded_index & 0xffffff;
    if (symbol->emission_state & ~7U) return symbol->emission_state >> 3;
    return external_symbol_index(symbol);
}
extern "C" void __cdecl complete_expression_class(CompilerSymbol *, unsigned); /*32406*/
/* 18a15..18a86: walk the existing enclosing-symbol chain before output. */
extern "C" void __cdecl normalize_symbol(CompilerSymbol *symbol)
{
    CompilerPointerArray *table = frontend_data.frontend_pointer_table;
    int index = (short)symbol->declaration_qualifiers;
    if (index < 0 || index >= table->count) { __asm { int 3 } }
    CompilerSymbol *scope = (CompilerSymbol *)table->items[index];
    while (scope) {
        scope->definition_flags &= ~8;
        if (scope->kind == 4) {
            CompilerGlobalClassDetail *detail = ((CompilerGlobalSymbolView *)scope)->class_detail;
            if ((detail->completion_flags & 1) && !(detail->template_flags & 1)) {
                complete_expression_class(scope, 0);
                if (!(symbol->definition_flags & 4) && !(symbol->scope_flags & 1))
                    frontend_bss.debug_module->Slot6c();
            }
        }
        index = (short)scope->declaration_qualifiers;
        table = frontend_data.frontend_pointer_table;
        if (index < 0 || index >= table->count) { __asm { int 3 } }
        scope = (CompilerSymbol *)table->items[index];
    }
}
extern "C" CompilerSymbol *__cdecl get_template_symbol(CompilerSymbol *symbol) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl resolve_generated_symbol(CompilerSymbol *symbol) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl get_import_symbol(CompilerSymbol *symbol) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl resolve_symbol_reference(CompilerSymbol *symbol) { __asm { int 3 } return NULL; }
extern "C" CompilerType *__cdecl create_default_type(void) { __asm { int 3 } return NULL; }
#define PAYLOAD_SPACE(n) (buffer->ok &= (buffer->cursor + (n) <= buffer->data + buffer->capacity || buffer->Grow(n) != 0))
/* The original may reuse its private byte-count slot for buffer status.
 * Its sole writer caller immediately discards all12 argument bytes. */
extern "C" void __cdecl write_integer_payload(unsigned low, unsigned high, int bytes)
{
    OutputBuffer *buffer = &frontend_bss.output_buffers[3];
    int remaining;
    /* 0x80 is reserved for the wide prefix, so -128 is deliberately
     * excluded from the otherwise signed one-byte range. */
    if ((high == 0 && low <= 0x7f) || (high == 0xffffffff && low >= 0xffffff81)) {
        if (PAYLOAD_SPACE(2)) BYTE_OUT(low);
        return;
    }
    remaining = bytes / 4 - 1;
    /* The supplied low/high slots establish eight payload bytes. Wider
     * requests would read beyond those slots in the original and remain
     * unsupported in both builds before any output is produced. */
    if (remaining > 1) { __asm { int 3 } }
    if (PAYLOAD_SPACE(2)) BYTE_OUT(0x80);
    if (PAYLOAD_SPACE(5)) DWORD_OUT(low);
    if (remaining == 1 && PAYLOAD_SPACE(5)) DWORD_OUT(high);
}
#undef PAYLOAD_SPACE
extern "C" unsigned __cdecl external_symbol_index(CompilerSymbol *symbol) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerSymbol::MarkReferenced(void)
{
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)this;
    unsigned references = symbol->emission_state & 3;
    if (references < 3)
        symbol->emission_state = (symbol->emission_state & ~3U) | (references + 1);
    symbol->emission_state = (symbol->emission_state & ~4U) |
        ((frontend_data.reference_context & 1U) << 2);
}
void RECON_THISCALL CompilerString::GetReferences(unsigned *first, unsigned *second, unsigned *tag) { __asm { int 3 } }
CompilerSymbol *RECON_THISCALL PendingGroupList::Add(CompilerSymbol *symbol) { __asm { int 3 } return NULL; }
int RECON_THISCALL OutputBuffer::Ensure(int bytes)
{
    if (bytes < 0) return 0;
    if (cursor + bytes <= data + capacity) return 1;
    return Grow(bytes) != 0;
}
int RECON_THISCALL OutputBuffer::Prepare(int bytes) { __asm { int 3 } return 0; }
void RECON_THISCALL OutputBuffer::WriteIndex(unsigned value)
{
    int good = cursor + 5 <= data + capacity || Grow(5) != 0;
    ok &= good;
    if (!ok) return;
    if ((int)value > 0x7fff) {
        *(DWORD *)cursor = (((value & 0xffffc000U) | 0x4000) << 1) | (value & 0x7fff);
        cursor += 4;
    } else {
        *(WORD *)cursor = (WORD)value;
        cursor += 2;
    }
}
int RECON_THISCALL OutputBuffer::WriteCompact(unsigned value)
{
    if (value & ~0x7fU) return 0;
    *cursor++ = (BYTE)value;
    return 1;
}
int RECON_THISCALL OutputBuffer::WriteBytes(const void *data, unsigned bytes) { __asm { int 3 } return 0; }
void RECON_THISCALL OutputBuffer::WriteUShort(unsigned value) { __asm { int 3 } }
void RECON_THISCALL OutputBuffer::WriteReferenceIndex(int value)
{
    unsigned number = (unsigned)value;
    int available;
    if (frontend_data.option_c9il) {
        available = cursor + 5 <= data + capacity ? 1 : Grow(5) != 0;
        ok &= available;
        if (!ok) return;
        if (value > 0x7fff) {
            *(DWORD *)cursor = (((number & 0xffffc000U) | 0x4000) << 1) | (number & 0x7fff);
            cursor += 4;
        } else {
            *(WORD *)cursor = (WORD)number;
            cursor += 2;
        }
    } else {
        available = cursor + 4 <= data + capacity ? 1 : Grow(4) != 0;
        ok &= available;
        if (!ok) return;
        if (number >= 0x3fff80) {
            *cursor++ = (BYTE)(number + 0x80);
        } else if (number <= 0x3fff) {
            *cursor++ = (BYTE)((number & 0x3f) | 0x80);
            *cursor++ = (BYTE)(number >> 6);
        } else {
            *cursor++ = (BYTE)(number | 0xc0);
            *cursor++ = (BYTE)(number >> 6);
            *cursor++ = (BYTE)(number >> 14);
        }
    }
}
void RECON_THISCALL OutputBuffer::WriteStringTag(unsigned value) { __asm { int 3 } }
