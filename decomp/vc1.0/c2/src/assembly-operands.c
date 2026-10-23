/* Inline-assembly operand grammar and symbol-relative address decoding. */
#include "function-input.h"
#include "instruction-selection.h"
#include "records.h"
#include <recon.h>
/* Original823e0..82490: the inline-operand grammar,22 eight-byte entries.
   m introduces a machine operand; s/r/o/d/b/i/x/g/v mean symbol, register,
   offset, displacement, base, index, scale, size class and immediate value.
   Zero-kind frontend-only/reserved descriptors retain their original errors. */
typedef struct AssemblyOperandFormat {
    unsigned kind;
    char *pattern;
} AssemblyOperandFormat;
static const AssemblyOperandFormat assembly_operand_formats[22] = {
    { 0, "" },       /* no operand */
    { 1, "mr" },     /* register */
    { 2, "mv" },     /* immediate */
    { 3, "msog" },   /* absolute symbol plus offset */
    { 4, "mdbg" },   /* base plus displacement */
    { 4, "msobg" },  /* symbolic base plus offset */
    { 5, "mdibxg" }, /* base/index/scale plus displacement */
    { 5, "msoibxg" },/* symbolic base/index/scale plus offset */
    { 6, "mso" },    /* label address */
    { 7, "mso" },    /* alternate symbolic address */
    { 8, "ms" },     /* symbol */
    { 9, "ms" },     /* frame-size marker, symbol identifier10 */
    { 10, "mv" },    /* special immediate */
    { 0, "m#" },     /* frontend-only directive */
    { 0, "ms" },     /* reserved symbol form */
    { 0, "ms" },     /* reserved symbol form */
    { 12, "mdg" },   /* displacement with size class */
    { 0, "mso" },    /* reserved symbolic address */
    { 0, "msr" },    /* symbol/register prefix, decoded before the operand */
    { 0, "mdbg" },   /* reserved based form */
    { 11, "mr" },    /* special register class */
    { 0, "" }        /* reserved */
};
typedef char AssemblyOperandFormatSize[sizeof(AssemblyOperandFormat) == 8 ? 1 : -1];
unsigned __fastcall read_assembly_symbol_id(unsigned char **cursor);
int __fastcall integer_log2(unsigned value);
/* Original1ae20..1b17c with grammar switch1b17c..1b1d8. The original
   frontend supplies a valid descriptor index; no new range guard is added.
   Unaligned DWORD payload reads follow the original x86 transport format. */
unsigned char *__fastcall decode_assembly_operand(MachineOperand *operand,
    unsigned char *cursor, ScopeHash *scope)
{
    unsigned tag, kind, identifier;
    int prefixed, replaced;
    short prefix_register;
    unsigned char byte;
    SymbolHeader *prefix_symbol, *symbol;
    char *pattern;
    operand->flags = operand->value.constant = 0;
    tag = *cursor++;
    if (tag == 18) {
        pattern = assembly_operand_formats[18].pattern;
        if (pattern[0] != 'm' || pattern[1] != 's' || pattern[2] != 'r' || pattern[3])
            compiler_internal_error("@(#)codegen.c:1.167", 0xa3a);
        prefixed = 1;
        prefix_symbol = lookup_symbol(scope, read_assembly_symbol_id(&cursor));
        if (!prefix_symbol)
            compiler_internal_error("@(#)codegen.c:1.167", 0xa42);
        prefix_register = (signed char)*cursor++;
        tag = *cursor++;
    } else {
        prefixed = 0;
        prefix_symbol = NULL;
        prefix_register = 26;
    }
    operand->flags = (operand->flags & ~15U) | (assembly_operand_formats[tag].kind & 15);
    kind = operand->flags & 15;
    if (!kind) compiler_internal_error("@(#)codegen.c:1.167", 0xa4c);
    if ((kind >= 3 && kind <= 7) || kind == 12)
        operand->value.address = new_machine_address();
    pattern = assembly_operand_formats[tag].pattern;
    if (*pattern != 'm') compiler_internal_error("@(#)codegen.c:1.167", 0xa51);
    for (++pattern; *pattern; ++pattern) {
        switch (*pattern) {
        case 'b':
            kind = operand->flags & 15;
            if (kind != OPERAND_BASED && kind != OPERAND_INDEXED)
                compiler_internal_error("@(#)codegen.c:1.167", 0xa5c);
            byte = *cursor++;
            operand->value.address->base.words.number = byte == 255 ? 27 : (signed char)byte;
            break;
        case 'd': case 'o':
            operand->value.address->first.displacement = *(long *)cursor;
            cursor += 4;
            break;
        case 'g':
            operand->flags = (operand->flags & ~0xf0U) | (((unsigned)(signed char)*cursor++ << 4) & 0xf0);
            break;
        case 'i':
            if ((operand->flags & 15) != OPERAND_INDEXED)
                compiler_internal_error("@(#)codegen.c:1.167", 0xa62);
            operand->value.address->index.words.number = (signed char)*cursor++;
            break;
        case 'r':
            operand->value.reg.words.number = (signed char)*cursor++;
            break;
        case 's':
            identifier = read_assembly_symbol_id(&cursor);
            symbol = lookup_symbol(scope, identifier);
            kind = operand->flags & 15;
            if (kind == OPERAND_FRAME_SIZE && (identifier != 10 || symbol))
                compiler_internal_error("@(#)codegen.c:1.167", 0xa7c);
            if (kind == OPERAND_SYMBOL) operand->value.symbol = symbol;
            else if (kind != OPERAND_FRAME_SIZE) operand->value.address->symbol = symbol;
            break;
        case 'v':
            operand->value.constant = *(unsigned *)cursor;
            cursor += 4;
            break;
        case 'x':
            if ((operand->flags & 15) != OPERAND_INDEXED)
                compiler_internal_error("@(#)codegen.c:1.167", 0xa69);
            operand->flags = (operand->flags & ~0x300U) |
                ((integer_log2((signed char)*cursor++) << 8) & 0x300);
            break;
        default:
            compiler_internal_error("@(#)codegen.c:1.167", 0xa85);
            break;
        }
    }
    if (prefixed) {
        replaced = 0;
        kind = operand->flags & 15;
        if (kind == OPERAND_INDEXED && operand->value.address->index.words.number == (unsigned short)prefix_register) {
            operand->value.address->index.words.number = REG_EBP;
            replaced = 1;
        }
        if (kind == OPERAND_BASED || kind == OPERAND_INDEXED) {
            if (operand->value.address->base.words.number == (unsigned short)prefix_register) {
                operand->value.address->base.words.number = REG_EBP;
                ++replaced;
            }
        } else compiler_internal_error("@(#)codegen.c:1.167", 0xa96);
        if (replaced != 1) compiler_internal_error("@(#)codegen.c:1.167", 0xaa1);
        if (operand->value.address->symbol)
            compiler_internal_error("@(#)codegen.c:1.167", 0xaa3);
        operand->value.address->symbol = prefix_symbol;
    }
    return cursor;
}

/* Original1b1e0..1b210. Bit15 extends the compact identifier by a second
   little-endian WORD; the combined payload has31 significant bits. */
unsigned __fastcall read_assembly_symbol_id(unsigned char **cursor)
{
    unsigned value = *(unsigned short *)*cursor;
    *cursor += 2;
    if (value & 0x8000) {
        value = (value & 0x7fff) | ((unsigned)*(unsigned short *)*cursor << 15);
        *cursor += 2;
    }
    return value;
}
