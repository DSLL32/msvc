/* Isolated whole instruction decoder probe; operand decoding is a contract. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "instruction-selection.h"
#include "function-input.h"
#include <recon.h>
#undef printf
MachineInstruction *free_machine_instructions;
MachineAddress *free_machine_addresses;
static SymbolHeader test_symbol;
static unsigned allocations[20], allocated, decoded;
void *__fastcall arena_alloc(unsigned arena, unsigned bytes)
{
    if (arena != 1 || allocated >= 20) abort();
    allocations[allocated++] = bytes;
    return calloc(1, bytes);
}
void __fastcall compiler_internal_error(char *source, unsigned code)
{
    printf("ERR %s:%x\n", source, code);
    abort();
}
unsigned char *__fastcall decode_assembly_operand(MachineOperand *operand,
    unsigned char *bytes, ScopeHash *scope)
{
    unsigned value;
    if (scope != (ScopeHash *)&test_symbol) abort();
    memcpy(&operand->flags, bytes, 4);
    memcpy(&value, bytes+4, 4);
    if ((operand->flags & 15) == OPERAND_LABEL_ADDRESS) {
        operand->value.address = calloc(1, sizeof(MachineAddress));
        operand->value.address->first.displacement = value & 1 ? 4 : 0;
        operand->value.address->symbol = value & 2 ? &test_symbol : NULL;
    } else operand->value.constant = value;
    ++decoded;
    return bytes+8;
}
MachineInstruction *__fastcall decode_assembly_instruction(unsigned char *, ScopeHash *);
int main(int argc, char **argv)
{
    unsigned char bytes[27];
    unsigned operation, i, count, flags, value, row[8];
    MachineInstruction *instruction;
    MachineOperand *operand;
    MachineAddress *address;
    if (argc < 3 || (argc-3)%2 || argc > 9) return 2;
    operation = strtoul(argv[1], NULL, 10);
    count = (argc-3)/2;
    bytes[0] = 3+count*8;
    bytes[1] = operation;
    bytes[2] = strtoul(argv[2], NULL, 10);
    for (i=0; i<count; ++i) {
        flags = strtoul(argv[3+2*i], NULL, 10);
        value = strtoul(argv[4+2*i], NULL, 10);
        memcpy(bytes+3+8*i, &flags, 4);
        memcpy(bytes+7+8*i, &value, 4);
    }
    instruction = decode_assembly_instruction(bytes, (ScopeHash *)&test_symbol);
    printf("%u %u %u", instruction->opcode, decoded, free_machine_addresses != NULL);
    operation = (instruction->opcode >> 8) & 255;
    for (i=0; i<2; ++i) {
        operand = &instruction->operands[i];
        memset(row, 0, sizeof(row));
        row[0] = operand->flags;
        if (i==1 && operation==MC_CALL) {
            row[6] = operand->value.call->argument_bytes;
            row[7] = operand->value.call->register_mask;
        } else if (i==1 && (operation==MC_JCC || operation==MC_SCC)) {
            /* The second slot stores a condition nibble, not an address. */
        } else if (((operand->flags&15)>=3 && (operand->flags&15)<=7) ||
                   (operand->flags&15)==10) {
            address = operand->value.address;
            row[2] = address->first.displacement;
            row[3] = address->symbol == &test_symbol ? 42 : 0;
            row[4] = address->base.value;
            row[5] = address->index.value;
        } else if ((operand->flags&15)==OPERAND_SYMBOL) {
            row[1] = operand->value.symbol == &test_symbol ? 42 : 0;
        } else row[1] = operand->value.constant;
        for (value=0; value<8; ++value) printf(" %u", row[value]);
    }
    printf(" %u", allocated);
    for (i=0; i<allocated; ++i) printf(" %u", allocations[i]);
    printf("\n");
    return 0;
}
