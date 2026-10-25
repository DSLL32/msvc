#include <string.h>
#include "frontend.h"
#pragma intrinsic(memcpy)
#include <recon.h>

/* Operand-format column of the182 encountered opcode records at RVA64c48.
 * The other column belongs to other IL consumers and is not copied here. */
static const char *ir_operand_formats[182] = {
    "\x0f", /* 0x0 */
    "\x0f", /* 0x1 */
    NULL, /* 0x2 */
    NULL, /* 0x3 */
    NULL, /* 0x4 */
    NULL, /* 0x5 */
    NULL, /* 0x6 */
    NULL, /* 0x7 */
    NULL, /* 0x8 */
    NULL, /* 0x9 */
    NULL, /* 0xa */
    NULL, /* 0xb */
    NULL, /* 0xc */
    NULL, /* 0xd */
    NULL, /* 0xe */
    "\x06", /* 0xf */
    "\x06", /* 0x10 */
    "\x06", /* 0x11 */
    "\x06", /* 0x12 */
    "\x06", /* 0x13 */
    "\x06", /* 0x14 */
    "\x06", /* 0x15 */
    "\x06", /* 0x16 */
    "\x06", /* 0x17 */
    "\x06", /* 0x18 */
    "\x06", /* 0x19 */
    NULL, /* 0x1a */
    NULL, /* 0x1b */
    NULL, /* 0x1c */
    NULL, /* 0x1d */
    NULL, /* 0x1e */
    NULL, /* 0x1f */
    NULL, /* 0x20 */
    NULL, /* 0x21 */
    NULL, /* 0x22 */
    NULL, /* 0x23 */
    NULL, /* 0x24 */
    NULL, /* 0x25 */
    "\x08", /* 0x26 */
    "\x06", /* 0x27 */
    "\x08", /* 0x28 */
    "\x08\x0f", /* 0x29 */
    "\x08\x06\x0d\x01\x0f", /* 0x2a */
    "\x08\x01\x0f", /* 0x2b */
    "\x06\x0e", /* 0x2c */
    "\x08\x0f", /* 0x2d */
    NULL, /* 0x2e */
    "\x06", /* 0x2f */
    "\x06", /* 0x30 */
    "\x06", /* 0x31 */
    "\x06", /* 0x32 */
    "\x0b", /* 0x33 */
    "\x06\x0e", /* 0x34 */
    "\x06", /* 0x35 */
    "\x06", /* 0x36 */
    NULL, /* 0x37 */
    "\x08\x0f", /* 0x38 */
    "\x08\x0f", /* 0x39 */
    "\x08\x0f", /* 0x3a */
    "\x08\x0f", /* 0x3b */
    "\x06\x08\x0f", /* 0x3c */
    "\x08\x05\x0f", /* 0x3d */
    "\x06\x03", /* 0x3e */
    "\x03", /* 0x3f */
    "\x06", /* 0x40 */
    "\x06\x0f", /* 0x41 */
    NULL, /* 0x42 */
    NULL, /* 0x43 */
    NULL, /* 0x44 */
    "\x09\x0f", /* 0x45 */
    "\x0f", /* 0x46 */
    "\x0f", /* 0x47 */
    "\x0f", /* 0x48 */
    "\x08\x0f", /* 0x49 */
    "\x0f", /* 0x4a */
    "\x0f", /* 0x4b */
    "\x0f", /* 0x4c */
    NULL, /* 0x4d */
    NULL, /* 0x4e */
    "\x04\x0f", /* 0x4f */
    "\x02", /* 0x50 */
    "\x02", /* 0x51 */
    "\x02\x0f", /* 0x52 */
    "\x0f", /* 0x53 */
    "\x02\x0f", /* 0x54 */
    "\x06\x0f", /* 0x55 */
    "\x0f", /* 0x56 */
    "\x0f", /* 0x57 */
    "\x06", /* 0x58 */
    NULL, /* 0x59 */
    "\x06", /* 0x5a */
    "\x02", /* 0x5b */
    "\x06\x14", /* 0x5c */
    "\x16\x14", /* 0x5d */
    "\x16\x14", /* 0x5e */
    NULL, /* 0x5f */
    "\x0f", /* 0x60 */
    "\x15", /* 0x61 */
    "\x0f", /* 0x62 */
    "\x08", /* 0x63 */
    "\x06\x0f", /* 0x64 */
    "\x08\x0f", /* 0x65 */
    NULL, /* 0x66 */
    NULL, /* 0x67 */
    NULL, /* 0x68 */
    "\x08", /* 0x69 */
    NULL, /* 0x6a */
    NULL, /* 0x6b */
    NULL, /* 0x6c */
    NULL, /* 0x6d */
    NULL, /* 0x6e */
    NULL, /* 0x6f */
    NULL, /* 0x70 */
    NULL, /* 0x71 */
    NULL, /* 0x72 */
    NULL, /* 0x73 */
    NULL, /* 0x74 */
    NULL, /* 0x75 */
    NULL, /* 0x76 */
    "\x06", /* 0x77 */
    "\x06\x07\x0f", /* 0x78 */
    NULL, /* 0x79 */
    NULL, /* 0x7a */
    NULL, /* 0x7b */
    NULL, /* 0x7c */
    "\x06", /* 0x7d */
    "\x06\x03", /* 0x7e */
    "\x0c\x0f", /* 0x7f */
    NULL, /* 0x80 */
    "\x10", /* 0x81 */
    "\x10", /* 0x82 */
    "\x10", /* 0x83 */
    NULL, /* 0x84 */
    NULL, /* 0x85 */
    NULL, /* 0x86 */
    "\x70", /* 0x87 */
    NULL, /* 0x88 */
    NULL, /* 0x89 */
    NULL, /* 0x8a */
    NULL, /* 0x8b */
    NULL, /* 0x8c */
    "\x06\x0f", /* 0x8d */
    "\x0f", /* 0x8e */
    "\x0f", /* 0x8f */
    "\x0f", /* 0x90 */
    NULL, /* 0x91 */
    NULL, /* 0x92 */
    NULL, /* 0x93 */
    NULL, /* 0x94 */
    NULL, /* 0x95 */
    NULL, /* 0x96 */
    NULL, /* 0x97 */
    NULL, /* 0x98 */
    "\x06", /* 0x99 */
    "\x06", /* 0x9a */
    "\x06\x11", /* 0x9b */
    "\x06", /* 0x9c */
    "\x0f", /* 0x9d */
    NULL, /* 0x9e */
    NULL, /* 0x9f */
    NULL, /* 0xa0 */
    NULL, /* 0xa1 */
    NULL, /* 0xa2 */
    NULL, /* 0xa3 */
    NULL, /* 0xa4 */
    NULL, /* 0xa5 */
    NULL, /* 0xa6 */
    NULL, /* 0xa7 */
    NULL, /* 0xa8 */
    NULL, /* 0xa9 */
    NULL, /* 0xaa */
    NULL, /* 0xab */
    NULL, /* 0xac */
    NULL, /* 0xad */
    NULL, /* 0xae */
    NULL, /* 0xaf */
    NULL, /* 0xb0 */
    NULL, /* 0xb1 */
    NULL, /* 0xb2 */
    NULL, /* 0xb3 */
    "\x19", /* 0xb4 */
    NULL, /* 0xb5 */
};
static const signed char integer_widths[16] = {
    0, 1, 2, 4, 4, 2, 4, 8, 8, 1, 2, 4, 8, 16, 0, 0
};

extern "C" {
void __cdecl emit_pending_line_record(void);
void __cdecl write_ir_operand(int, const CompilerIntegerConstant *, int);
void __cdecl write_symbol_record(CompilerIrSymbol *);
int __cdecl has_debug_symbol_index(unsigned);
unsigned __cdecl get_symbol_output_index(CompilerIrSymbol *);
CompilerIrSymbol *__cdecl resolve_symbol_alias(CompilerIrSymbol *);
CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *);
unsigned __cdecl assign_symbol_output_index(CompilerIrSymbol *);
void __cdecl write_ir_integer_words(unsigned, unsigned, int);
}

/* Preserve the caller's load of good AFTER Ensure returns. These are source
 * expansions, with no additional helper vertices. */
#define IR_RESERVE(n) (available = buffer->Ensure(n), buffer->good &= available)
#define UPDATE_REFERENCE(s) do { \
    unsigned f = (s)->reference_flags; \
    if ((f & 3) < 3) f = (f & ~3U) | ((f + 1) & 3); \
    f = (f & ~4U) | ((c1_data.ir_reference_epoch & 1) << 2); \
    (s)->reference_flags = f; \
} while (0)

extern "C" void __cdecl write_ir_record(int kind, const void *value)
{
    const CompilerIntegerConstant *node = (const CompilerIntegerConstant *)value;
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[3];
    const unsigned char *format;
    CompilerIrSymbol *symbol;
    CompilerTypeShape *shape;
    unsigned code, index, bits;
    /* The original reuses the incoming value slot for tag3 widths. Its
     * initial carried value is the node address, retained until overwritten. */
    unsigned width = (unsigned)value;
    const BYTE *bytes;
    int available;
    if (kind == 181)
        compiler_error(1, ".\\toil.c", 3645);
    emit_pending_line_record();
    if (IR_RESERVE(2))
        *buffer->cursor++ = (BYTE)kind;
    ++c1_bss.ir_record_count;
    format = (const unsigned char *)ir_operand_formats[kind];
    if (!format) return;
    while (*format) {
        switch (*format++) {
        case 1:
            bits = node->extended[0] == 11;
            if (IR_RESERVE(2)) *buffer->cursor++ = (BYTE)bits;
            break;
        case 2:
            bits = (unsigned)node->integer;
            if (IR_RESERVE(6)) {
                if (bits & ~0x7fU) {
                    *buffer->cursor++ = 0x80;
                    *(DWORD *)buffer->cursor = bits;
                    buffer->cursor += 4;
                } else {
                    *buffer->cursor++ = (BYTE)bits;
                }
            }
            break;
        case 3:
            shape = node->symbol->type->shape;
            c1_data.ir_symbol_state_changed = 1;
            switch (shape->storage & 0x70) {
            case 0x10: width = 0; break;
            case 0x20: width = 8; break;
            case 0x40: width = 4; break;
            case 0x50: width = 24; break;
            case 0x60:
                if ((shape->flags & 7) != 4) shape = shape->base;
                width = (~(unsigned)shape->flags >> 11) & 0x10;
                break;
            case 0x70: width = 20; break;
            default:
                /* Original retains the carried argument-slot value here.
                 * Cross-address output from these classes is unsupported. */
#ifdef RECON_NATIVE_TEST
                recon_unsupported("write_ir_record: unmodeled storage width");
#endif
                break;
            }
            if (IR_RESERVE(2)) *buffer->cursor++ = (BYTE)width;
            break;
        case 6:
            write_ir_operand(kind, node, 2);
            break;
        case 7:
        case 13:
        case 14:
            if (buffer->Reserve(1)) *buffer->cursor++ = 0;
            break;
        case 8:
            symbol = NULL;
            index = 0;
            switch (kind) {
            case 0x26:
                symbol = node->symbol;
                if (has_debug_symbol_index(symbol->reference_flags >> 3))
                    c1_bss.debug_module->Slot80();
                break;
            case 0x27:
            case 0x28:
                break;
            case 0x29:
            case 0x2a:
            case 0x2b:
            case 0x2d:
                if (c1_data.c9_il) {
                    index = node->symbol->record_id & 0xffffff;
                } else {
                    index = node->symbol->reference_flags;
                    if (index & ~7U) index >>= 3;
                    else index = assign_symbol_output_index(node->symbol);
                }
                break;
            case 0x38:
            case 0x39:
            case 0x3a:
                symbol = node->symbol;
                write_symbol_record(symbol);
                break;
            case 0x3b:
            case 0x3c:
                symbol = c1_bss.current_function;
                UPDATE_REFERENCE(symbol);
                write_symbol_record(symbol);
                break;
            case 0x3d:
                index = node->index;
                break;
            default:
                symbol = node->symbol;
                break;
            }
            if (symbol) {
                UPDATE_REFERENCE(symbol);
                if (symbol->declaration_flags & 0x40) {
                    symbol = resolve_symbol_alias(symbol);
                    UPDATE_REFERENCE(symbol);
                }
                if ((symbol->identifier_flags & 1) &&
                    (lookup_ir_symbol(symbol)->record_id & 0xffffff) == 5 &&
                    c1_bss.current_scope_symbol &&
                    !(c1_bss.current_scope_symbol->scope_flags & 1)) {
                    c1_data.ir_symbol_state_changed = 1;
                    c1_data.ir_symbol_state_ready = 0;
                }
                index = get_symbol_output_index(symbol);
            }
            if (c1_data.c9_il) {
                buffer->WriteIndex(index);
            } else if (IR_RESERVE(4)) {
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
            break;
        case 11:
            write_ir_operand(kind, node, 1);
            if (node->type->shape) {
                bits = (unsigned)node->integer;
                if (IR_RESERVE(6)) {
                    if (bits & ~0x7fU) {
                        *buffer->cursor++ = 0x80;
                        *(DWORD *)buffer->cursor = bits;
                        buffer->cursor += 4;
                    } else *buffer->cursor++ = (BYTE)bits;
                }
            } else if ((node->type->flags & 0x40) || (node->type->flags & 0x200)) {
                bits = node->type->flags;
                code = bits & 0xf;
                if (!(bits & 0x340) && (bits & 0xfff0)) width = code;
                else width = integer_widths[code];
                write_ir_integer_words(node->words.low, node->words.high, width);
            } else if (node->type->flags & 0x100) {
                bytes = node->extended;
                if (IR_RESERVE(11)) {
                    memcpy(buffer->cursor, bytes, 10);
                    buffer->cursor += 10;
                }
                bytes = node->extended + 10;
                if (IR_RESERVE(3)) buffer->WriteBytes(bytes, 2);
            } else if (node->type->flags & 0x80) {
                bits = node->index;
                if (IR_RESERVE(6) && !buffer->WriteSmall(bits)) {
                    *buffer->cursor++ = 0x80;
                    *(DWORD *)buffer->cursor = bits;
                    buffer->cursor += 4;
                }
            }
            break;
        default:
            break;
        }
    }
}
#undef UPDATE_REFERENCE
#undef IR_RESERVE

extern "C" void __cdecl emit_pending_line_record(void)
{
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[3];
    int available, delta;
    if (c1_bss.pending_line_delta &&
        c1_bss.pending_line_delta != c1_data.last_line_delta) {
        c1_data.last_line_delta = c1_bss.pending_line_delta;
        available = 1;
        if (buffer->cursor + 2 > buffer->data + buffer->capacity)
            available = buffer->Grow(2);
        buffer->good &= available;
        if (buffer->good) *buffer->cursor++ = 0x4f;
        available = 1;
        if (buffer->cursor + 2 > buffer->data + buffer->capacity)
            available = buffer->Grow(2);
        buffer->good &= available;
        if (buffer->good) *buffer->cursor++ = 0x24;
        delta = (short)c1_bss.pending_line_delta;
        available = 1;
        if (buffer->cursor + 4 > buffer->data + buffer->capacity)
            available = buffer->Grow(4);
        buffer->good &= available;
        if (buffer->good) {
            if ((unsigned)delta & ~0x7fU) {
                *buffer->cursor++ = 0x80;
                *(WORD *)buffer->cursor = (WORD)delta;
                buffer->cursor += 2;
            } else *buffer->cursor++ = (BYTE)delta;
        }
    }
}
extern "C" void __cdecl write_ir_operand(int kind, const CompilerIntegerConstant *node, int mode) { __asm { int 3 } }
extern "C" void __cdecl write_symbol_record(CompilerIrSymbol *symbol) { __asm { int 3 } }
extern "C" int __cdecl has_debug_symbol_index(unsigned index) { __asm { int 3 } return 0; }
extern "C" unsigned __cdecl get_symbol_output_index(CompilerIrSymbol *symbol) { __asm { int 3 } return 0; }
extern "C" CompilerIrSymbol *__cdecl resolve_symbol_alias(CompilerIrSymbol *symbol) { __asm { int 3 } return NULL; }
extern "C" CompilerIrSymbol *__cdecl lookup_ir_symbol(CompilerIrSymbol *symbol) { __asm { int 3 } return NULL; }
extern "C" unsigned __cdecl assign_symbol_output_index(CompilerIrSymbol *symbol) { __asm { int 3 } return 0; }
extern "C" void __cdecl write_ir_integer_words(unsigned low, unsigned high, int width) { __asm { int 3 } }
int RECON_THISCALL CompilerOutputBuffer::Ensure(int bytes)
{
    if (bytes < 0) return 0;
    if (cursor + bytes > data + capacity && !Grow(bytes)) return 0;
    return 1;
}
int RECON_THISCALL CompilerOutputBuffer::Reserve(int bytes) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerOutputBuffer::WriteIndex(unsigned index) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerOutputBuffer::WriteBytes(const void *bytes, unsigned count) { __asm { int 3 } }
int RECON_THISCALL CompilerOutputBuffer::WriteSmall(unsigned value) { __asm { int 3 } return 0; }
