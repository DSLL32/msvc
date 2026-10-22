#include "instruction-selection.h"
#include "register-use.h"
#include "instruction-rewrite.h"
#include "options.h"
#include "register-transfers.h"
#include "value-selection.h"
static MachineOperand temporary_operands[100]; /* 0007cb28 */
/* Original [00017e00,00017e20). Each reference receives its own temporary
   operand so later per-use annotations do not alter the shared register data. */
MachineOperand *__fastcall operand_for_register(unsigned short reg)
{
    return new_operand(OPERAND_REGISTER, reg, 0, 0);
}
/* Original [00019c90,00019d00). Wide integer values reserve two adjacent
   temporary registers; values held by the x87 stack use its fixed ST(0) ID. */
unsigned short __fastcall new_value_register(unsigned short type)
{
    unsigned short result;
    if (type & 0x3000) return REG_ST0;
    result = next_machine_register | 0x8000;
    next_machine_register = (unsigned short)(next_machine_register + ((type & 255) > 4 ? 2 : 1));
    if ((int)next_machine_register - 1 >= 0x4000)
        compiler_report(0x35, symbol_display_name(&current_symbol->symbol));
    return result;
}
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

/* Original [00022c20,00022c40). Address payloads are shared by the memory,
   symbolic-address and reserved address operand forms (kind 12). */
void __fastcall release_operand_address(MachineOperand *operand)
{
    unsigned kind = operand->flags & 15;
    if ((kind >= 3 && kind <= 7) || kind == 12)
        release_machine_address(operand->value.address);
}

/* Original [0001a1e0,0001a240). Locals, parameters and frame temporaries
   use EBP-based homes. Other symbols select memory or address operands. */
MachineOperand *__fastcall symbol_operand(Node *reference, int indirect)
{
    unsigned offset;
    SymbolHeader *symbol;
    if (get_stack_address(reference, &offset, &symbol))
        return new_operand(OPERAND_BASED, REG_EBP, offset, symbol);
    return new_operand(indirect ? OPERAND_ABSOLUTE : OPERAND_LABEL_ADDRESS,
        0, reference->left.value, reference->right.symbol);
}

/* Original [0001a550,0001a590). Recycled address records must be cleared;
   fresh arena-one storage already supplies the same sixteen zero bytes. */
MachineAddress *new_machine_address(void)
{
    MachineAddress *address = free_machine_addresses;
    if (!address) return arena_alloc(1, sizeof(MachineAddress));
    free_machine_addresses = address->first.next;
    address->first.displacement = 0;
    address->symbol = 0;
    address->base.value = address->index.value = 0;
    return address;
}

/* Original [000198e0,00019930). A virtual leaf already names its register;
   a direct symbol supplies its own memory operand. Other addresses are
   evaluated into a base register before constructing the memory operand. */
MachineOperand *__fastcall select_lvalue_operand(Node *node)
{
    if (node->opcode == 0xa2)
        return new_operand(OPERAND_REGISTER, (unsigned short)node->left.value, 0, 0);
    if (node->opcode == 0x26) return symbol_operand(node, 1);
    return new_operand(OPERAND_BASED, select_value_register(node), 0, 0);
}

/* Original [00019800,00019830). This lookup's native stop key is255,
   although the encountered maps end with -1. Preserve that distinction;
   valid callers must request a key present in their contextual map. */
unsigned __fastcall lookup_opcode(const OpcodeMap *map, unsigned opcode)
{
    for (; map->tree_opcode != 255; ++map)
        if (map->tree_opcode == opcode) return map->selection;
    compiler_internal_error("@(#)codegen.c:1.167", 0x8fa);
    return 0;
}
