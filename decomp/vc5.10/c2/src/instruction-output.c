#include "functions.h"
#include "code-records.h"
#include "function-records.h"
#include "registers.h"
#include "output-callbacks.h"
#include <recon.h>

extern int c2_cpu_generation, c2_favor_speed;
unsigned char *encoding_cursor, *encoding_origin, *encoding_marks;
void *encoding_relocations;

void __fastcall emit_encoding_integer(unsigned value, int bytes)
{
    switch (bytes) {
    case 0: return;
    case 1: *encoding_cursor = (unsigned char)value; break;
    case 2: *(unsigned short *)encoding_cursor = (unsigned short)value; break;
    case 4: *(unsigned *)encoding_cursor = value; break;
    }
    /* The original advances other widths without writing any payload. */
    encoding_cursor += bytes;
}
void __fastcall encode_modrm_operand(C2Expression *record, C2Expression *operand, int reg);
int __fastcall encode_immediate_operand(C2Expression *operand, int bytes, int allow_byte);
void __fastcall encode_far_symbol_operand(C2Expression *operand, int bytes)
{
    C2Symbol *symbol = operand->value.symbol;
    unsigned offset = encoding_cursor - encoding_origin;
    if (encoding_relocations) {
        if (bytes == 6) {
            output_slot_499750.target(offset, symbol);
            output_slot_491070.target(offset + 2U, symbol);
        } else output_slot_491078.target(offset, symbol);
    }
    if (bytes == 6) {
        emit_encoding_integer(0, 2);
        emit_encoding_integer(0, 4);
    } else emit_encoding_integer(0, bytes);
}
int __fastcall immediate_fits_signed_byte(int value)
{
    return (int)(signed char)value == value;
}

/* These are semantic encoding choices, not copied image rows. Other opcodes
 * remain unavailable until their own recipes are encountered and understood. */
typedef struct C2EncodingChoice { unsigned char opcode, action; } C2EncodingChoice;
typedef struct C2OpcodeEncoding {
    unsigned flags;
    signed char operands;
    unsigned char modrm;
    unsigned char choice_count;
    C2EncodingChoice choices[6];
} C2OpcodeEncoding;
enum { ENC_MODRM=1, ENC_IMMEDIATE=4, ENC_REGISTER=6, ENC_SEGMENT=12 };
static const C2OpcodeEncoding push_encoding = {
    /* The unrelated original0x400 flag is not consumed here. */
    0, 1, 6 << 3, 5,
    {{0x06,ENC_SEGMENT}, {0x68,ENC_IMMEDIATE|0x20},
     {0x50,ENC_REGISTER}, {0xff,ENC_MODRM}, {0,0}}
};
/* MOV alternatives: segment-register transfer, accumulator absolute-address
 * transfer, register immediate, r/m immediate, then general ModRM transfer. */
static const C2OpcodeEncoding move_encoding = {
    0x800, 2, 0, 6,
    {{0x8c,11|0x40}, {0xa0,3|0x80}, {0xb0,5},
     {0xc6,2|0x80}, {0x88,1|0x40|0x80}, {0,0}}
};
/* OR: accumulator immediate, r/m immediate with byte relaxation, general
 * register/r/m transfer. Only flags consumed by this encoder are represented. */
static const C2OpcodeEncoding or_encoding = {
    0, 2, 1 << 3, 4,
    {{0x0c,20|0x80|0x20}, {0x80,2|0x80|0x20},
     {0x08,1|0x40|0x80}, {0,0}}
};
/* XOR has the same operand alternatives, with the /6 group extension. */
static const C2OpcodeEncoding xor_encoding = {
    0, 2, 6 << 3, 4,
    {{0x34,20|0x80|0x20}, {0x80,2|0x80|0x20},
     {0x30,1|0x40|0x80}, {0,0}}
};
/* TEST does not choose a direction bit or sign-extend an eight-bit immediate:
 * accumulator-immediate, /0 r/m-immediate, then symmetric r/m-register. */
static const C2OpcodeEncoding test_encoding = {
    0, 2, 0, 4,
    {{0xa8,20|0x80}, {0xf6,2|0x80}, {0x84,1|0x80}, {0,0}}
};
/* REP MOVS: emit the repeat prefix followed by the MOVS byte opcode.
 * The existing width action selects A4/A5 (and the word-size prefix). */
static const C2OpcodeEncoding repeat_move_encoding = {
    0, 0, 0xa4, 1, {{0xf3,17|0x80}}
};
/* Unsigned DIV uses the /6 F6/F7 unary ModRM encoding. */
static const C2OpcodeEncoding divide_encoding = {
    0, 1, 6 << 3, 2, {{0xf6,19|0x80}, {0,0}}
};
/* The current function also reaches RET, POP, AND and the two logical shifts.
 * Adjacent zero/default rows outside these alternatives are not modeled. */
static const C2OpcodeEncoding return_encoding = {
    0, 1, 0, 2, {{0xc2,13}, {0,0}}
};
static const C2OpcodeEncoding pop_encoding = {
    0x800, 1, 0, 5,
    {{0x07,ENC_SEGMENT}, {0,ENC_IMMEDIATE|0x20},
     {0x58,ENC_REGISTER}, {0x8f,ENC_MODRM}, {0,0}}
};
static const C2OpcodeEncoding and_encoding = {
    0, 2, 4 << 3, 4,
    {{0x24,20|0x80|0x20}, {0x80,2|0x80|0x20},
     {0x20,1|0x40|0x80}, {0,0}}
};
static const C2OpcodeEncoding shift_left_encoding = {
    0, 2, 4 << 3, 2, {{0xc0,18|0x80}, {0xd2,0}}
};
static const C2OpcodeEncoding shift_right_encoding = {
    0, 2, 5 << 3, 2, {{0xc0,18|0x80}, {0xd2,0}}
};
/* Reached integer arithmetic recipes. INC chooses the compact register
 * opcode or the byte/ModRM form; CDQ has no explicit operand. IDIV uses /7.
 * ADD/SUB use accumulator-immediate, group-immediate and register/r/m forms.
 * SAR shares the shift-count actions with the existing logical shifts. */
static const C2OpcodeEncoding increment_encoding = {
    0, 1, 0, 3, {{0x40,ENC_REGISTER}, {0xfe,ENC_MODRM|0x80}, {0,0}}
};
static const C2OpcodeEncoding sign_extend_encoding = {
    0x20, 0, 0, 2, {{0x99,7}, {0,0}}
};
static const C2OpcodeEncoding signed_divide_encoding = {
    0, 1, 7 << 3, 2, {{0xf6,19|0x80}, {0,0}}
};
static const C2OpcodeEncoding add_encoding = {
    0, 2, 0, 4,
    {{0x04,20|0x80|0x20}, {0x80,2|0x80|0x20}, {0x00,1|0x40|0x80}, {0,0}}
};
static const C2OpcodeEncoding subtract_encoding = {
    0, 2, 5 << 3, 4,
    {{0x2c,20|0x80|0x20}, {0x80,2|0x80|0x20}, {0x28,1|0x40|0x80}, {0,0}}
};
static const C2OpcodeEncoding arithmetic_shift_right_encoding = {
    0, 2, 7 << 3, 2, {{0xc0,18|0x80}, {0xd2,0}}
};
/* Reached final records: comparisons, effective addresses, two-operand
 * multiplication, calls, decrement and local conditional/unconditional jumps.
 * These choices retain their distinct immediate widths and ModRM groups. */
static const C2OpcodeEncoding compare_encoding = {
    0, 2, 7 << 3, 4,
    {{0x3c,20|0x80|0x20}, {0x80,2|0x80|0x20}, {0x38,1|0x40|0x80}, {0,0}}
};
static const C2OpcodeEncoding address_encoding = {
    0x800, 2, 0, 2, {{0x8d,ENC_MODRM}, {0,0}}
};
static const C2OpcodeEncoding multiply_encoding = {
    0x10, 2, 0, 3, {{0x69,13}, {0xaf,ENC_MODRM}, {0,0}}
};
static const C2OpcodeEncoding call_encoding = {
    0, 1, 2 << 3, 4, {{0xe8,9}, {0x9a,10}, {0xff,ENC_MODRM}, {0,0}}
};
static const C2OpcodeEncoding decrement_encoding = {
    0, 1, 1 << 3, 3, {{0x48,ENC_REGISTER}, {0xfe,ENC_MODRM|0x80}, {0,0}}
};
static const C2OpcodeEncoding conditional_jump_encoding = {
    0x20, 1, 0, 2, {{0x70,13}, {0x80,0}}
};
static const C2OpcodeEncoding jump_encoding = {
    0x20, 1, 4 << 3, 5, {{0xeb,8}, {0xe9,9}, {0xea,10}, {0xff,ENC_MODRM}, {0,0}}
};
/* FSTP: the register and ten-byte special forms precede the ordinary
 * single/double memory form. Original63's0xc000 classification bits are
 * not consumed by this encoder;0x800 selects the output as first operand. */
static const C2OpcodeEncoding float_store_pop_encoding = {
    0x801, 2, 3 << 3, 3, {{0xd9,13}, {0xd9,15}, {0,0}}
};
/* FLD uses the same single/double and extended-real choices with /0.
 * Only0x801 of original0x4801 is consumed by this encoder. */
static const C2OpcodeEncoding float_load_encoding = {
    0x801, 2, 0, 3, {{0xd9,13}, {0xd9,15}, {0,0}}
};
/* FILD: the eight-byte integer special form uses DF /5; the ordinary
 * WORD/DWORD form uses DF/DB /0. Only0x801 is consumed of original0x4801. */
static const C2OpcodeEncoding float_integer_load_encoding = {
    0x801, 2, 0, 3, {{0xdf,13}, {0xdb,15|0x40}, {0,0}}
};
/* FSUB uses D8 /4 for ST0 minus memory and DC /5 for ST(i) minus
 * ST0. The second choice's byte is that reverse register group selector. */
static const C2OpcodeEncoding float_subtract_encoding = {
    1, 2, 4 << 3, 2, {{0xd8,14}, {5,0}}
};
/* FST retains the stack top: register form DD /2, or D9/DD /2 for
 * single/double memory. Its genuine ten-byte error remains in action13. */
static const C2OpcodeEncoding float_store_encoding = {
    0x801, 2, 2 << 3, 3, {{0xd9,13}, {0xd9,15}, {0,0}}
};
/* The reached x87 arithmetic forms retain their group selectors and pop
 * actions. Classification-only flag bits are not encoder inputs. */
static const C2OpcodeEncoding float_multiply_encoding = {
    1, 2, 1 << 3, 2, {{0xd8,14}, {1,0}}
};
static const C2OpcodeEncoding float_add_pop_encoding = {
    1, 2, 0xc1, 2, {{0xda,14|0x20}, {0,0}}
};
static const C2OpcodeEncoding float_multiply_pop_encoding = {
    1, 2, 0xc9, 2, {{0xda,14|0x20}, {1,0}}
};
static const C2OpcodeEncoding float_subtract_pop_encoding = {
    1, 2, 0xe9, 2, {{0xde,14|0x20}, {5,0}}
};
static const C2OpcodeEncoding float_divide_pop_encoding = {
    1, 2, 0xf9, 2, {{0xde,14|0x20}, {7,0}}
};
static const C2OpcodeEncoding float_compare_encoding = {
    1, 2, 2 << 3, 2, {{0xd8,14}, {2,0}}
};
static const C2OpcodeEncoding float_exchange_encoding = {
    1, 1, 1 << 3, 1, {{0xd9,14|0x20}}
};
/* FNSTSW chooses the register status-word form or DD /7 memory. */
static const C2OpcodeEncoding float_status_encoding = {
    0x801, 1, 7 << 3, 2, {{0xdf,13}, {0xdd,16}}
};
static const C2OpcodeEncoding float_change_sign_encoding = {
    1, 0, 0xe0, 1, {{0xd9,7}}
};
#define ENCODING_ERROR(line) c2_report_internal("E:\\\\utc\\\\src\\\\\\\\P2\\\\x86\\\\code.c", line)
#define KNOWN_ENCODING_NUMBER(reg) \
 do { if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
   (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
   (reg) != (C2FunctionRegister *)&machine_registers[0] && \
   (reg) != (C2FunctionRegister *)&machine_registers[1] && \
   (reg) != (C2FunctionRegister *)&machine_registers[2] && \
   (reg) != (C2FunctionRegister *)&machine_registers[3] && \
   (reg) != (C2FunctionRegister *)&machine_registers[4] && \
   (reg) != (C2FunctionRegister *)&machine_registers[5] && \
   (reg) != (C2FunctionRegister *)&machine_registers[6] && \
   (reg) != (C2FunctionRegister *)&machine_registers[7] && \
   (reg) != (C2FunctionRegister *)&machine_registers[8] && \
   (reg) != (C2FunctionRegister *)&machine_registers[31] && \
   (reg) != (C2FunctionRegister *)&machine_registers[32] && \
   (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } } while (0)
#define KNOWN_ENCODING_CODE(reg) \
 do { if ((reg) != (C2FunctionRegister *)&machine_registers[1] && \
   (reg) != (C2FunctionRegister *)&machine_registers[2] && \
   (reg) != (C2FunctionRegister *)&machine_registers[3] && \
   (reg) != (C2FunctionRegister *)&machine_registers[4] && \
   (reg) != (C2FunctionRegister *)&machine_registers[5] && \
   (reg) != (C2FunctionRegister *)&machine_registers[6] && \
   (reg) != (C2FunctionRegister *)&machine_registers[7] && \
   (reg) != (C2FunctionRegister *)&machine_registers[8] && \
   (reg) != (C2FunctionRegister *)&machine_registers[9] && \
   (reg) != (C2FunctionRegister *)&machine_registers[17] && \
   (reg) != (C2FunctionRegister *)&machine_registers[20] && \
   (reg) != (C2FunctionRegister *)&machine_registers[21] && \
   (reg) != (C2FunctionRegister *)&machine_registers[31] && \
   (reg) != (C2FunctionRegister *)&machine_registers[32] && \
   (reg) != (C2FunctionRegister *)&machine_registers[33] && \
   (reg) != (C2FunctionRegister *)&machine_registers[34] && \
   (reg) != (C2FunctionRegister *)&machine_registers[35] && \
   (reg) != (C2FunctionRegister *)&machine_registers[19]) { __asm { int 3 } } } while (0)
#define REGISTER_CODE(reg) ((unsigned char)((C2MachineRegister *)(reg))->parameter_number)
#define IMMEDIATE_OPERAND(op) ((op)->kind == 7 || (op)->kind == 4 || \
 ((op)->kind == 3 && (op)->opcode != 0x120) || \
 ((op)->kind == 5 && ((op)->opcode == 0x126 || (op)->opcode == 0x127)))

int __fastcall encode_immediate_operand(C2Expression *operand, int bytes, int allow_byte)
{
    C2FunctionRegister *reg, *root;
    C2Symbol *symbol = NULL;
    unsigned offset = encoding_cursor - encoding_origin;
    unsigned value = 0, storage;
    int opcode = operand->opcode, relaxed = 0;

    switch (opcode) {
    case 0x11d:
        value = operand->integer.low;
        break;
    case 0x120:
        emit_encoding_integer((unsigned)operand->value.reg->displacement -
            operand->stack_adjustment, bytes);
        return 0;
    case 0x121:
        reg = operand->value.reg;
        root = reg->canonical;
        symbol = root->symbol;
        value = (unsigned)reg->displacement - root->displacement;
        break;
    case 0x124:
        emit_encoding_integer((unsigned)operand->tail.aggregate_register->displacement -
            operand->stack_adjustment + operand->detail.aggregate_displacement, bytes);
        return 0;
    case 0x126:
        if (bytes != 4) ENCODING_ERROR(1006);
        if (!operand->tail.aggregate_register)
            value = (unsigned)encoding_marks - current_symbol->parameter_offset;
        else
            bytes = 4;
        goto register_displacement;
    case 0x129: case 0x12a:
        symbol = operand->value.symbol;
        if (opcode != 0x129 && symbol->kind != 4) {
            value = (unsigned)symbol->parameter_offset - (unsigned)encoding_marks - offset - bytes;
            symbol = NULL;
        }
        break;
    case 0x12b:
        symbol = operand->tail.aggregate_register->symbol;
        value = operand->detail.aggregate_displacement;
        if (symbol->kind != 4) {
            value += symbol->parameter_offset - (unsigned)encoding_marks - offset - bytes;
            symbol = NULL;
        }
        break;
    case 0x12c:
        value = (unsigned)operand->detail.aggregate_displacement - offset - bytes;
        break;
    default:
register_displacement:
        value += operand->detail.aggregate_displacement;
        reg = operand->tail.aggregate_register;
        if (reg) {
            root = reg->canonical;
            symbol = root->symbol;
            value += (unsigned)reg->displacement - root->displacement;
        }
        if (symbol && symbol->kind == 1) {
            storage = ((C2InputSymbol *)symbol)->attributes & 0x70;
            if (storage == 0x10 || storage == 0x30)
                value += symbol->parameter_offset;
        }
        break;
    }
    if (encoding_relocations) {
        storage = symbol && symbol->kind == 1 ?
            ((C2InputSymbol *)symbol)->attributes & 0x70 : 0;
        if (symbol && storage != 0x10 && storage != 0x30 && bytes >= 2) {
            if (bytes == 6) {
                output_slot_499750.target(offset, symbol);
                output_slot_491070.target(offset + 2, symbol);
            } else if (opcode == 0x127) {
                output_slot_499750.target(offset, symbol);
            } else if (opcode == 0x12a || opcode == 0x12b) {
                output_slot_491078.target(offset, symbol);
            } else {
                output_slot_491070.target(offset, symbol);
            }
        } else if (opcode == 0x126 && !operand->tail.aggregate_register) {
            output_slot_491070.target(offset, current_symbol);
        }
    }
    if (allow_byte && bytes > 1 && opcode == 0x11d && immediate_fits_signed_byte(value)) {
        bytes = 1;
        relaxed = 2;
    }
    if (bytes == 6) {
        emit_encoding_integer(0, 2);
        bytes = 4;
    }
    emit_encoding_integer(value, bytes);
    return relaxed;
}

extern int generate_frame, generate_prologue;
unsigned __fastcall encoding_displacement_mode(C2Expression *record,
    C2Expression *operand, int *bytes)
{
    C2FunctionRegister *reg;
    int displacement;
    reg = operand->opcode == 0x120 ? operand->value.reg : operand->tail.aggregate_register;
    if (!reg) {
        displacement = operand->detail.aggregate_displacement;
        if (!displacement) {
            reg = operand->aggregate_value->original_register;
            KNOWN_ENCODING_NUMBER(reg);
            if (reg->number != 6) {
                *bytes = 0;
                return 0; /* Actual492088[0]: no ModRM displacement. */
            }
        }
    } else if (operand->opcode == 0x120) {
        displacement = (int)((unsigned)reg->displacement - operand->stack_adjustment);
    } else if (operand->opcode == 0x124) {
        displacement = (int)((unsigned)operand->detail.aggregate_displacement -
            operand->stack_adjustment + reg->displacement);
    } else {
        *bytes = 4;
        return 0x80; /* Actual492088[4]: DWORD displacement. */
    }
    *bytes = immediate_fits_signed_byte(displacement) ? 1 : 4;
    /* The only generated widths index actual DWORD values0x40 and0x80;
     * no other table rows or neighboring data are modeled. */
    return *bytes == 1 ? 0x40 : 0x80;
}
void __fastcall encode_modrm_operand(C2Expression *record, C2Expression *operand, int reg)
{
    C2Expression *base, *index, *temporary;
    C2FunctionRegister *root, *frame;
    unsigned char bits = (reg & 7) << 3;
    unsigned mode, code, scale;
    int bytes = reg, use_stack;

    switch (operand->opcode) {
    case 0x11f:
        KNOWN_ENCODING_CODE(operand->original_register);
        *encoding_cursor++ = REGISTER_CODE(operand->original_register) | bits | 0xc0;
        return;
    case 0x120:
        root = operand->value.reg;
        use_stack = (generate_frame && root->kind != 5) || (generate_prologue && root->kind == 5);
        mode = encoding_displacement_mode(record, operand, &bytes);
        if (!use_stack) {
            frame = (C2FunctionRegister *)&machine_registers[6];
            KNOWN_ENCODING_CODE(frame);
            *encoding_cursor++ = mode | bits | REGISTER_CODE(frame);
            break;
        }
stack_base:
        *encoding_cursor++ = mode | bits | 4;
        frame = (C2FunctionRegister *)&machine_registers[5];
        KNOWN_ENCODING_CODE(frame);
        code = REGISTER_CODE(frame) & 7;
        *encoding_cursor++ = (code << 3) | code;
        break;
    case 0x121: case 0x128: case 0x129:
        *encoding_cursor++ = bits | 5;
        bytes = 4;
        encode_immediate_operand(operand, bytes, 0);
        return;
    case 0x122:
        base = operand->aggregate_value;
        KNOWN_ENCODING_NUMBER(base->original_register);
        use_stack = base->original_register->number == 5;
        mode = encoding_displacement_mode(record, operand, &bytes);
        if (use_stack) goto stack_base;
        KNOWN_ENCODING_CODE(base->original_register);
        *encoding_cursor++ = mode | bits | REGISTER_CODE(base->original_register);
        break;
    case 0x123:
        index = operand->aggregate_second;
        KNOWN_ENCODING_NUMBER(index->original_register);
        if (index->original_register->number == 5) {
            temporary = operand->aggregate_value;
            operand->aggregate_value = index;
            operand->aggregate_second = temporary;
        }
        mode = encoding_displacement_mode(record, operand, &bytes);
        base = operand->aggregate_value;
        index = operand->aggregate_second;
        scale = operand->location.expression.properties;
        KNOWN_ENCODING_CODE(base->original_register);
        KNOWN_ENCODING_CODE(index->original_register);
        *encoding_cursor++ = mode | bits | 4;
        *encoding_cursor++ = ((scale << 3 | (REGISTER_CODE(index->original_register) & 7)) << 3) |
            (REGISTER_CODE(base->original_register) & 7);
        break;
    case 0x124:
        root = operand->tail.aggregate_register;
        use_stack = (generate_prologue && root->kind == 5) || (generate_frame && root->kind != 5);
        mode = encoding_displacement_mode(record, operand, &bytes);
        index = operand->aggregate_second;
        frame = (C2FunctionRegister *)&machine_registers[use_stack ? 5 : 6];
        KNOWN_ENCODING_CODE(frame);
        KNOWN_ENCODING_CODE(index->original_register);
        scale = operand->location.expression.properties;
        *encoding_cursor++ = mode | bits | 4;
        *encoding_cursor++ = (((REGISTER_CODE(index->original_register) & 7) | scale << 3) << 3) |
            (REGISTER_CODE(frame) & 7);
        break;
    case 0x125:
        index = operand->aggregate_second;
        KNOWN_ENCODING_CODE(index->original_register);
        scale = operand->location.expression.properties;
        *encoding_cursor++ = bits | 4;
        *encoding_cursor++ = (((REGISTER_CODE(index->original_register) & 7) | scale << 3) << 3) | 5;
        bytes = 4;
        encode_immediate_operand(operand, bytes, 0);
        return;
    }
    encode_immediate_operand(operand, bytes, 0);
}

/* Byte-view numbers17/20 are now consumed here; all other unknown
 * static numbers remain behind the existing column guard. */
#define FINAL_KNOWN_ENCODING_NUMBER(reg) do { \
    if ((reg) != (C2FunctionRegister *)&machine_registers[17] && \
        (reg) != (C2FunctionRegister *)&machine_registers[20] && \
        (reg) != (C2FunctionRegister *)&machine_registers[21] && \
        (reg) != (C2FunctionRegister *)&machine_registers[33] && \
        (reg) != (C2FunctionRegister *)&machine_registers[34] && \
        (reg) != (C2FunctionRegister *)&machine_registers[35]) KNOWN_ENCODING_NUMBER(reg); \
} while (0)
int __fastcall encode_final_record(C2Expression *record, unsigned char *encoding,
    unsigned char *marks, void *relocations)
{
    const C2OpcodeEncoding *format;
    const C2EncodingChoice *choice;
    C2Expression *first, *second, *other;
    C2FunctionRegister *reg;
    unsigned char *start, *opcode, *padding;
    unsigned operation, flags, segment, value, number, choice_index, i;
    unsigned char bits;
    int width, count, first_accumulator, second_accumulator;

    operation = record->opcode;
    if (!operation || operation > 0x11a) return 0;
    switch (operation) {
    case 0xfe: format = &repeat_move_encoding; break;
    case 0x21: format = &increment_encoding; break;
    case 0xdf: format = &sign_extend_encoding; break;
    case 0x20: format = &signed_divide_encoding; break;
    case 0x2d: format = &add_encoding; break;
    case 0x32: format = &subtract_encoding; break;
    case 0x29: format = &arithmetic_shift_right_encoding; break;
    case 0xd: format = &push_encoding; break;
    case 0x2e: format = &and_encoding; break;
    case 0x2b: format = &shift_right_encoding; break;
    case 0x2a: format = &shift_left_encoding; break;
    case 0xc: format = &pop_encoding; break;
    case 0x8: format = &return_encoding; break;
    case 1: format = &move_encoding; break;
    case 0x30: format = &or_encoding; break;
    case 0x34: format = &xor_encoding; break;
    case 0x33: format = &test_encoding; break;
    case 0x2f: format = &compare_encoding; break;
    case 0x12: format = &address_encoding; break;
    case 0xc1: format = &multiply_encoding; break;
    case 0xe: format = &call_encoding; break;
    case 0x1e: format = &decrement_encoding; break;
    case 0xf: case 0x102: format = &conditional_jump_encoding; break;
    case 0x10: case 0x104: format = &jump_encoding; break;
    case 0x1f: format = &divide_encoding; break;
    case 0x63: format = &float_store_pop_encoding; break;
    case 0x60: format = &float_load_encoding; break;
    case 0x56: format = &float_integer_load_encoding; break;
    case 0x47: format = &float_subtract_encoding; break;
    case 0x62: format = &float_store_encoding; break;
    case 0x46: format = &float_multiply_encoding; break;
    case 0x4b: format = &float_add_pop_encoding; break;
    case 0x4c: format = &float_multiply_pop_encoding; break;
    case 0x4d: format = &float_subtract_pop_encoding; break;
    case 0x4f: format = &float_divide_pop_encoding; break;
    case 0x5c: format = &float_compare_encoding; break;
    case 0x5f: format = &float_exchange_encoding; break;
    case 0x6e: format = &float_status_encoding; break;
    case 0x77: format = &float_change_sign_encoding; break;
    default: { __asm { int 3 } } return 0;
    }
    flags = format->flags;
    if (flags & 0x2000) current_function->flags |= 0x20000;
    if (operation == 0xe5) current_function->flags |= 0x40000;
    encoding_relocations = relocations;
    encoding_marks = marks;
    encoding_origin = encoding;
    start = encoding;
    /* The original preserves its fourth incoming value after reporting an
     * invalid record kind. Retain that defined fallback, not an invented zero. */
    width = (int)relocations;
    switch (record->kind) {
    case 11: width = record->type & 0xfff; break;
    case 13: width = operation == 0xe ? 4 : 6; break;
    case 14: width = 0; break;
    case 16:
        switch (operation) {
        case 0xf: case 0x10: width = 4; break;
        case 0x16: *start++ = 0x67; /* address-size prefix */
        case 0x17: case 0x18: case 0x19:
        case 0x102: case 0x104: case 0x10a: width = 1; break;
        case 0x103: width = 6; break;
        default: ENCODING_ERROR(0xb6); break;
        }
        break;
    default: ENCODING_ERROR(0xbc); break;
    }
    count = format->operands;
    if (flags & 0x800) {
        first = record->tail.right;
        second = record->left;
    } else {
        first = record->left;
        second = first ? first->next : NULL;
    }
    if (flags & 0x200) *start++ = 0x9b; /* FWAIT */
    if (width == 2 && !(flags & 0x21)) *start++ = 0x66;
    segment = 0;
    if (count > 0 && (first->location.expression.comparison & 7))
        segment = first->location.expression.comparison & 7;
    else if (count > 1 && (second->location.expression.comparison & 7))
        segment = second->location.expression.comparison & 7;
    if (segment) {
        segment += 24;
        if (segment < 29) {
            reg = (C2FunctionRegister *)&machine_registers[segment];
            KNOWN_ENCODING_CODE(reg);
            *start++ = (REGISTER_CODE(reg) & 3) * 8 | 0x26;
        } else *start++ = segment == 29 ? 0x64 : 0x65;
    }
    choice_index = 0;
next_choice:
    if (choice_index >= format->choice_count) { __asm { int 3 } }
    choice = &format->choices[choice_index];
    opcode = start;
    if (flags & 0x10) *opcode++ = 0x0f;
    *opcode = choice->opcode;
    encoding_cursor = opcode + 1;
    other = second;
    first_accumulator = second_accumulator = 0;
    if ((choice->action & 31) == 3 || (choice->action & 31) == 20 ||
        ((choice->action & 31) == 13 && operation == 9)) {
        if (first->opcode == 0x11f) {
            FINAL_KNOWN_ENCODING_NUMBER(first->original_register->canonical);
            if (first->original_register->canonical->number == 1) {
                FINAL_KNOWN_ENCODING_NUMBER(first->original_register);
                first_accumulator = first->original_register->number != 21;
            }
        }
        if (second->opcode == 0x11f) {
            FINAL_KNOWN_ENCODING_NUMBER(second->original_register->canonical);
            if (second->original_register->canonical->number == 1) {
                FINAL_KNOWN_ENCODING_NUMBER(second->original_register);
                second_accumulator = second->original_register->number != 21;
            }
        }
    }
    switch (choice->action & 31) {
    case 1: case 19:
modrm:
        if (count == 1) {
            value = (int)(signed char)format->modrm >> 3;
            if (width == 6 && (operation == 0x103 || operation == 0x105)) value |= 1;
            encode_modrm_operand(record, first, value);
        } else if (first->opcode == 0x11f) {
            KNOWN_ENCODING_CODE(first->original_register);
            encode_modrm_operand(record, other, REGISTER_CODE(first->original_register));
            if (choice->action & 0x40) *opcode |= 2;
        } else {
            if (other->opcode != 0x11f) ENCODING_ERROR(0x117);
            KNOWN_ENCODING_CODE(other->original_register);
            encode_modrm_operand(record, first, REGISTER_CODE(other->original_register));
        }
        goto encoded;
    case 2:
        if (IMMEDIATE_OPERAND(second)) {
            encode_modrm_operand(record, first, (int)(signed char)format->modrm >> 3);
            value = choice->action & 0x20;
            goto immediate;
        }
        break;
    case 3:
        if (first_accumulator && ((second->opcode == 0x121 && second->kind == 2) || second->opcode == 0x128)) {
            encode_immediate_operand(second, 4, 0); goto encoded;
        }
        if (second_accumulator && ((first->opcode == 0x121 && first->kind == 2) || first->opcode == 0x128)) {
            encode_immediate_operand(first, 4, 0); *opcode |= 2; goto encoded;
        }
        break;
    case 4:
        if (IMMEDIATE_OPERAND(first)) {
            *opcode |= encode_immediate_operand(first, width, 1); goto encoded;
        }
        break;
    case 5:
        if (first->opcode == 0x11f && IMMEDIATE_OPERAND(second)) {
            KNOWN_ENCODING_CODE(first->original_register);
            bits = *opcode | REGISTER_CODE(first->original_register);
            *opcode = width > 1 ? bits | 8 : bits;
            encode_immediate_operand(second, width, 0); goto encoded;
        }
        break;
    case 6:
        if (first->opcode == 0x11f && width != 1) {
            KNOWN_ENCODING_CODE(first->original_register);
            *opcode |= REGISTER_CODE(first->original_register); goto encoded;
        }
        break;
    case 7:
        if (format->modrm) *encoding_cursor++ = format->modrm;
        goto encoded;
    case 8:
        if (width == 1) {
            if (first->kind == 4 || first->opcode == 0x12c) {
                encode_immediate_operand(first, 1, 0); goto encoded;
            }
            if (first->opcode == 0x126) {
                first->opcode = 0x12b; encode_immediate_operand(first, 1, 0);
                first->opcode = 0x126; goto encoded;
            }
        }
        break;
    case 9: case 10:
        if (((choice->action & 31) == 9 && width < 5) || ((choice->action & 31) == 10 && width == 6)) {
            i = (choice->action & 31) == 9 ? 4 : 6;
            if (((choice->action & 31) == 9 && (first->kind == 4 || first->opcode == 0x12c)) ||
                ((choice->action & 31) == 10 && record->kind == 4)) {
                if (first->opcode == 0x129 && first->value.symbol->kind == 4)
                    encode_far_symbol_operand(first, i);
                else encode_immediate_operand(first, i, 0);
                goto encoded;
            }
            if (first->opcode == 0x126) {
                first->opcode = 0x12b; encode_immediate_operand(first, i, 0);
                first->opcode = 0x126; goto encoded;
            }
        }
        break;
    case 11: case 12:
        if (first->opcode == 0x11f) {
            FINAL_KNOWN_ENCODING_NUMBER(first->original_register);
            number = first->original_register->number;
            if (number > 24 && number < 31) {
                KNOWN_ENCODING_CODE(first->original_register);
                if ((choice->action & 31) == 11) {
                    encode_modrm_operand(record, second, REGISTER_CODE(first->original_register) & 7);
                    *opcode |= 2;
                } else {
                    bits = choice->opcode;
                    opcode = encoding; *opcode = bits; encoding_cursor = opcode + 1;
                    if (number < 29) *opcode |= (REGISTER_CODE(first->original_register) & 3) << 3;
                    else { *opcode = 0x0f; *encoding_cursor++ =
                        (REGISTER_CODE(first->original_register) & 7) * 8 | (operation != 13) + 0x80; }
                }
                goto encoded;
            }
        }
        if ((choice->action & 31) == 11 && second->opcode == 0x11f) {
            FINAL_KNOWN_ENCODING_NUMBER(second->original_register);
            number = second->original_register->number;
            if (number > 24 && number < 31) {
                KNOWN_ENCODING_CODE(second->original_register);
                encode_modrm_operand(record, first, REGISTER_CODE(second->original_register) & 7);
                goto encoded;
            }
        }
        break;
    case 13:
        if (operation < 8) {
            if (operation == 7) {
                if (first->integer.low != 3) { *opcode |= 1; *encoding_cursor++ = first->integer.low; }
            } else if (operation == 4) {
                emit_encoding_integer(first->integer.low, 2);
                emit_encoding_integer(second->integer.low, 1);
            } else goto invalid_choice;
            goto encoded;
        }
        switch (operation) {
        case 0x56:
            width = second->type & 0xfff;
        case 0x5b:
            if (width == 8) {
                *opcode |= 4;
                if (operation != 0x56) second = first;
                encode_modrm_operand(record, second, ((int)(signed char)format->modrm | 0x28) >> 3);
                goto encoded;
            }
            break;
        case 0x62:
            if (width == 10) ENCODING_ERROR(0x20b);
        case 0x63:
            if (first->opcode == 0x11f) {
                *opcode |= 4; encode_modrm_operand(record, first, (int)(signed char)format->modrm >> 3);
                goto encoded;
            }
        case 0x60:
            if (width == 10 && (operation != 0x60 || second->opcode != 0x11f)) {
                *opcode |= 2;
                if (operation != 0x60) second = first;
                encode_modrm_operand(record, second, ((int)(signed char)format->modrm | 0x28) >> 3);
                goto encoded;
            }
            break;
        case 0x6c: case 0x6e:
            if (first->opcode == 0x11f) { encode_modrm_operand(record, first, 4); goto encoded; }
            break;
        case 0xa3:
            KNOWN_ENCODING_CODE(first->original_register);
            encode_modrm_operand(record, second, REGISTER_CODE(first->original_register));
            *opcode |= encode_immediate_operand(second->next, width, 1); goto encoded;
        case 0xa9:
            if (!width) width = record->control_flags;
            /* Padding recipes have not been encountered. Zero length needs no
             * table read; a positive length stops before unknown contents. */
            if (width) { __asm { int 3 } }
            padding = NULL;
            for (i = 0; i < (unsigned)width; ++i) opcode[i] = padding[i];
            encoding_cursor = opcode + width; goto encoded;
        case 0xbb:
            encoding_cursor = start;
            if (first->kind == 4) {
                emit_encoding_integer(0, width);
                if (encoding_relocations) output_slot_491070.target(0, first->value.symbol);
            } else if (first->kind == 7) emit_encoding_integer(first->integer.low, width);
            goto encoded;
        case 0xc1:
            if (second->kind == 7) {
                KNOWN_ENCODING_CODE(first->original_register);
                *start = *opcode; opcode = start; encoding_cursor = start + 1;
                encode_modrm_operand(record, first, REGISTER_CODE(first->original_register));
                value = 1; goto immediate;
            }
            break;
        case 0xca: case 0xcb:
            if (second->opcode != 0x11f) ENCODING_ERROR(0x1d3);
            KNOWN_ENCODING_CODE(second->original_register);
            encode_modrm_operand(record, first, REGISTER_CODE(second->original_register));
            if (second->next->opcode == 0x11f) *opcode |= 1;
            else *encoding_cursor++ = second->next->integer.low;
            goto encoded;
        case 0xcc: case 0xcd: case 0xce: case 0xcf:
            if (second->kind == 7) {
                if (second->opcode != 0x11d) ENCODING_ERROR(0x229);
                encode_modrm_operand(record, first, (int)(signed char)format->modrm >> 3);
                emit_encoding_integer(second->integer.low, 1);
            } else {
                if (second->opcode != 0x11f) ENCODING_ERROR(0x22d);
                ++choice_index;
                if (choice_index >= format->choice_count) { __asm { int 3 } }
                choice = &format->choices[choice_index];
                encoding_cursor[-1] = choice->opcode;
                KNOWN_ENCODING_CODE(second->original_register);
                encode_modrm_operand(record, first, REGISTER_CODE(second->original_register));
            }
            goto encoded;
        case 0xd0:
            if (first->opcode == 0x11f) {
                FINAL_KNOWN_ENCODING_NUMBER(first->original_register);
                number = first->original_register->number;
            } else number = 0;
            if (number >= 39 && number <= 57) { *opcode |= 2; first = second; }
            else { FINAL_KNOWN_ENCODING_NUMBER(second->original_register); number = second->original_register->number; }
            if (number >= 42 && number <= 49) *opcode |= 1;
            else if (number > 49 && number < 58) *opcode |= 4;
            if (number >= C2_MACHINE_REGISTER_COUNT) { __asm { int 3 } }
            reg = (C2FunctionRegister *)&machine_registers[number];
            KNOWN_ENCODING_CODE(reg); KNOWN_ENCODING_CODE(first->original_register);
            encoding_cursor = opcode + 2;
            opcode[1] = (REGISTER_CODE(reg) | 0xf8) << 3 | REGISTER_CODE(first->original_register);
            goto encoded;
        case 0xd9: case 0xda:
            KNOWN_ENCODING_CODE(second->original_register);
            encode_modrm_operand(record, first, REGISTER_CODE(second->original_register));
            goto width_bit;
        case 0xe6:
            if (width == 8) *opcode |= 1;
            if (second->kind == 1) {
                FINAL_KNOWN_ENCODING_NUMBER(second->original_register);
                number = second->original_register->number;
                if (number > 57 && number < 66) {
                    if (first->kind == 1) { other = first; first = second; }
                    *opcode |= 0x10; goto modrm;
                }
            }
            goto modrm;
        case 0xe7:
            if (choice_index + 1 >= format->choice_count) { __asm { int 3 } }
            if (width == 2) *opcode = format->choices[choice_index + 1].opcode;
            goto modrm;
        case 0x102:
            *opcode |= record->condition_code;
            encode_immediate_operand(first, width, 0); goto encoded;
        case 0x106:
            *opcode |= 8;
        case 8:
            if (!first->integer.low) *opcode |= 1;
            else emit_encoding_integer(first->integer.low, 2);
            goto encoded;
        case 9:
            if (width == 1) break;
            if (first_accumulator && second->opcode == 0x11f) {
                KNOWN_ENCODING_CODE(second->original_register);
                *opcode |= REGISTER_CODE(second->original_register); goto encoded;
            }
            if (second_accumulator && first->opcode == 0x11f) {
                KNOWN_ENCODING_CODE(first->original_register);
                *opcode |= REGISTER_CODE(first->original_register); goto encoded;
            }
            break;
        case 10: case 11:
            if (choice_index + 1 >= format->choice_count) { __asm { int 3 } }
            if (operation == 10 && second->opcode == 0x11d) {
                *opcode = format->choices[choice_index + 1].opcode;
                *encoding_cursor = second->integer.low; encoding_cursor = opcode + 2;
            } else if (operation == 11 && first->opcode == 0x11d) {
                *opcode = format->choices[choice_index + 1].opcode;
                *encoding_cursor = first->integer.low; encoding_cursor = opcode + 2;
            }
            if (!(flags & 1)) goto width_bit;
            goto encoded;
        case 0xf:
            if (choice_index + 1 >= format->choice_count) { __asm { int 3 } }
            *opcode = 0x0f; encoding_cursor = opcode + 2;
            opcode[1] = format->choices[choice_index + 1].opcode | record->condition_code;
            encode_immediate_operand(first, width, 0); goto encoded;
        default:
            if (operation >= 0x10b && operation <= 0x11a) {
                *opcode |= (unsigned char)operation - 0xb; goto encoded;
            }
            goto invalid_choice;
        }
        break;
    case 14:
        if (first->opcode != 0x11f) ENCODING_ERROR(0x297);
        if (count == 1) goto first_modrm;
        FINAL_KNOWN_ENCODING_NUMBER(first->original_register);
        if (second->opcode == 0x11f) FINAL_KNOWN_ENCODING_NUMBER(second->original_register);
        if (first->original_register->number == 31 &&
            (second->opcode != 0x11f || second->original_register->number != 31)) {
            if (choice->action & 0x20) ENCODING_ERROR(0x2b0);
            if (second->opcode != 0x11f &&
                ((choice->action & 0x40) ? width == 2 : width == 8)) *opcode |= 4;
            encode_modrm_operand(record, second, (int)(signed char)format->modrm >> 3);
        } else {
            *opcode |= 4;
            if (second->opcode != 0x11f) ENCODING_ERROR(0x2bb);
            if (choice_index + 1 >= format->choice_count) { __asm { int 3 } }
            encode_modrm_operand(record, first, (int)(signed char)format->choices[choice_index+1].opcode);
        }
        goto encoded;
    case 15:
        if (first->opcode == 0x11f) first = second;
        if (first->opcode != 0x11f &&
            ((choice->action & 0x40) ? width == 2 : width == 8)) *opcode |= 4;
first_modrm:
    case 16:
        encode_modrm_operand(record, first, (int)(signed char)format->modrm >> 3); goto encoded;
    case 17:
        *encoding_cursor++ = format->modrm; ++opcode; goto encoded;
    case 18:
        encode_modrm_operand(record, first, (int)(signed char)format->modrm >> 3);
        if (second->opcode == 0x11f) {
            if (choice_index + 1 >= format->choice_count) { __asm { int 3 } }
            *opcode |= format->choices[choice_index+1].opcode;
        }
        else if (second->integer.low == 1 && (c2_cpu_generation != 4 || !c2_favor_speed)) *opcode |= 0x10;
        else emit_encoding_integer(second->integer.low, 1);
        goto encoded;
    case 20:
        if (first_accumulator && IMMEDIATE_OPERAND(second) &&
            (width != 4 || !(choice->action & 0x20) || second->opcode != 0x11d ||
             !immediate_fits_signed_byte(second->integer.low))) {
            value = 0;
immediate:
            *opcode |= encode_immediate_operand(second, width, value); goto encoded;
        }
        break;
    case 21:
        *opcode = (unsigned char)(*opcode + (width >> 1)); goto modrm;
    case 22:
        if (first->kind != 1) ENCODING_ERROR(0x2e8);
        if (second->kind == 7) {
            encode_modrm_operand(record, first, (int)(signed char)format->modrm >> 3);
            *opcode += width >> 2;
            emit_encoding_integer(second->integer.low, 1); goto encoded;
        }
        *opcode = format->choices[choice_index+1].opcode + (width >> 2); goto modrm;
    default:
invalid_choice:
        ENCODING_ERROR(0x2fc); goto encoded;
    }
    ++choice_index;
    goto next_choice;
width_bit:
    if (width != 1) *opcode |= 1;
encoded:
    /* The machine code reloads the owner opcode after its operand helpers. */
    switch (record->opcode) {
    case 0xfe: flags = repeat_move_encoding.flags; break;
    case 0x21: flags = increment_encoding.flags; break;
    case 0xdf: flags = sign_extend_encoding.flags; break;
    case 0x20: flags = signed_divide_encoding.flags; break;
    case 0x2d: flags = add_encoding.flags; break;
    case 0x32: flags = subtract_encoding.flags; break;
    case 0x29: flags = arithmetic_shift_right_encoding.flags; break;
    case 1: flags = move_encoding.flags; break;
    case 0x30: flags = or_encoding.flags; break;
    case 0x34: flags = xor_encoding.flags; break;
    case 0x33: flags = test_encoding.flags; break;
    case 0x2f: flags = compare_encoding.flags; break;
    case 0x12: flags = address_encoding.flags; break;
    case 0xc1: flags = multiply_encoding.flags; break;
    case 0xe: flags = call_encoding.flags; break;
    case 0x1e: flags = decrement_encoding.flags; break;
    case 0xf: case 0x102: flags = conditional_jump_encoding.flags; break;
    case 0x10: case 0x104: flags = jump_encoding.flags; break;
    case 0x1f: flags = divide_encoding.flags; break;
    case 0x63: flags = float_store_pop_encoding.flags; break;
    case 0x60: flags = float_load_encoding.flags; break;
    case 0x56: flags = float_integer_load_encoding.flags; break;
    case 0x47: flags = float_subtract_encoding.flags; break;
    case 0x62: flags = float_store_encoding.flags; break;
    case 0x46: flags = float_multiply_encoding.flags; break;
    case 0x4b: flags = float_add_pop_encoding.flags; break;
    case 0x4c: flags = float_multiply_pop_encoding.flags; break;
    case 0x4d: flags = float_subtract_pop_encoding.flags; break;
    case 0x4f: flags = float_divide_pop_encoding.flags; break;
    case 0x5c: flags = float_compare_encoding.flags; break;
    case 0x5f: flags = float_exchange_encoding.flags; break;
    case 0x6e: flags = float_status_encoding.flags; break;
    case 0x77: flags = float_change_sign_encoding.flags; break;
    case 0xd: flags = push_encoding.flags; break;
    case 0x2e: flags = and_encoding.flags; break;
    case 0x2b: flags = shift_right_encoding.flags; break;
    case 0x2a: flags = shift_left_encoding.flags; break;
    case 0xc: flags = pop_encoding.flags; break;
    case 0x8: flags = return_encoding.flags; break;
    default: { __asm { int 3 } }
    }
    if (!(flags & 1) && (choice->action & 0x80) && width != 1) *opcode |= 1;
    return encoding_cursor - encoding;
}

#undef FINAL_KNOWN_ENCODING_NUMBER
