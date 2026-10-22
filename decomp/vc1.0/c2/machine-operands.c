#include "instruction-selection.h"
#include "register-use.h"
static MachineOperand temporary_operands[100]; /* 0007cb28 */
/* Original [000219d0,00021aa0), including the eight-way operand-kind table.
   Compare only addressing identity, leaving analysis flags out of equality.
   Indexed addressing additionally carries its scale in flag bits 8 and 9. */
int __fastcall operands_equal(MachineOperand *left, MachineOperand *right)
{
    unsigned difference = left->flags ^ right->flags;
    unsigned kind = left->flags & 15;
    MachineAddress *a, *b;
    if (difference & 15) return 0;
    switch (kind) {
    case OPERAND_REGISTER:
        return (unsigned short)left->value.constant == (unsigned short)right->value.constant;
    case OPERAND_IMMEDIATE:
        return left->value.constant == right->value.constant;
    case OPERAND_SYMBOL:
        return left->value.symbol == right->value.symbol;
    case OPERAND_INDEXED:
        if (difference & 0x300) return 0;
        a = left->value.address;
        b = right->value.address;
        if (a->index.words.number != b->index.words.number) return 0;
        /* Fall through to compare the base and displacement. */
    case OPERAND_BASED:
        a = left->value.address;
        b = right->value.address;
        if (a->base.words.number != b->base.words.number) return 0;
        /* Fall through to the common address fields. */
    case 3: case 6:
        a = left->value.address;
        b = right->value.address;
        return a->symbol == b->symbol && a->first.displacement == b->first.displacement;
    }
    return 0;
}
/* Original [00019710,000197e0), including the nine-way kind table.
   Address payloads retain the initialized defaults supplied by their allocator. */
void __fastcall initialize_operand(MachineOperand *operand, unsigned kind,
    unsigned short reg, unsigned displacement, SymbolHeader *symbol)
{
    operand->flags = kind & 15;
    operand->value.constant = 0;
    switch (kind) {
    case OPERAND_REGISTER:
        operand->value.constant = reg;
        break;
    case OPERAND_IMMEDIATE: case OPERAND_FRAME_SIZE:
        operand->value.constant = displacement;
        break;
    case 3: case 6: case 7:
        operand->value.address = new_machine_address();
        operand->value.address->symbol = symbol;
        operand->value.address->first.displacement = (long)displacement;
        break;
    case OPERAND_BASED:
        operand->value.address = new_machine_address();
        operand->value.address->base.words.number = reg;
        operand->value.address->symbol = symbol;
        operand->value.address->first.displacement = (long)displacement;
        break;
    case OPERAND_SYMBOL:
        operand->value.symbol = symbol;
        break;
    }
}
/* Original [00019330,00019380). The signed bound and cursor are shared
   with recursive selection; select_expression restores its caller's cursor. */
MachineOperand *__fastcall new_operand(unsigned kind, unsigned short reg,
    unsigned displacement, SymbolHeader *symbol)
{
    MachineOperand *operand;
    if (temporary_operand_count >= 100)
        compiler_internal_error("@(#)codegen.c:1.167", 0x630);
    operand = &temporary_operands[temporary_operand_count++];
    initialize_operand(operand, kind, reg, displacement, symbol);
    return operand;
}
