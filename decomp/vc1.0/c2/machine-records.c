#include "instruction-selection.h"
#include "function-input.h"
/* Original [00019620,00019650). The returned kind-2 marker also serves
   as the function context passed to later machine passes. */
MachineInstruction *__fastcall emit_function_symbol(FunctionSymbol *symbol)
{
    MachineInstruction *instruction = append_machine_instruction(2);
    instruction->record.symbol = &symbol->symbol;
    emit_source_marker(current_source_location);
    return instruction;
}
/* Original [00017ca0,00017cc0). */
void __fastcall emit_source_marker(unsigned source_location)
{
    MachineInstruction *instruction = append_machine_instruction(4);
    instruction->record.source_location = source_location;
}
/* Original [00019380,000193a0). Filename records borrow the tree string. */
void __fastcall emit_source_filename(char *filename)
{
    MachineInstruction *instruction = append_machine_instruction(5);
    instruction->record.filename = filename;
}
#include <stdarg.h>
/* Original [000172f0,00017340). All variants occupy eight words. */
MachineInstruction *__fastcall new_machine_instruction(unsigned kind)
{
    MachineInstruction *instruction = free_machine_instructions;
    unsigned *word, count;
    if (instruction) {
        free_machine_instructions = instruction->next;
        word = (unsigned *)instruction;
        count = 8;
        while (count--) *word++ = 0;
    } else instruction = arena_alloc(1, sizeof(MachineInstruction));
    instruction->opcode = (instruction->opcode & ~15U) | (kind & 15);
    return instruction;
}
/* Original [0001a5e0,0001a620). Append a cleared record to the machine stream. */
MachineInstruction *__fastcall append_machine_instruction(unsigned kind)
{
    MachineInstruction *instruction = new_machine_instruction(kind);
    if (!machine_instructions) {
        machine_instructions = instruction;
        instruction->previous = 0;
    } else {
        machine_tail->next = instruction;
        instruction->previous = machine_tail;
    }
    machine_tail = instruction;
    return instruction;
}
/* Original [00017160,000172f0), including the eleven-entry kind table.
   The trailing arguments depend on the record kind and, for instructions,
   on the recovered opcode operand count. */
MachineInstruction *__cdecl new_machine_record(unsigned kind, ...)
{
    MachineInstruction *instruction = new_machine_instruction(kind);
    MachineBlockHeader *block;
    MachineOperand *operand;
    MachineCallInfo *call;
    SymbolHeader *symbol;
    unsigned opcode, size;
    int count;
    va_list arguments;
    va_start(arguments, kind);
    switch (kind) {
    case 0: case 8: case 9: case 10:
        opcode = va_arg(arguments, unsigned);
        instruction->opcode = (instruction->opcode & ~0xff00U) | ((opcode << 8) & 0xff00);
        size = va_arg(arguments, unsigned);
        instruction->opcode = (instruction->opcode & ~0xf0U) | ((size << 4) & 0xf0);
        count = machine_operand_counts[(instruction->opcode >> 8) & 255];
        if (count == 1 || count == 2) {
            operand = va_arg(arguments, MachineOperand *);
            instruction->operands[0] = *operand;
            if (count == 2) {
                operand = va_arg(arguments, MachineOperand *);
                instruction->operands[1] = *operand;
            }
        }
        opcode = (instruction->opcode >> 8) & 255;
        if (opcode == MC_JCC || opcode == MC_SCC)
            instruction->operands[1].flags = (instruction->operands[1].flags & ~15U) |
                                            (va_arg(arguments, unsigned) & 15);
        else if (opcode == MC_CALL) {
            call = arena_alloc(1, sizeof(MachineCallInfo));
            instruction->operands[1].value.call = call;
            call->field00 = va_arg(arguments, unsigned);
            call->field04 = va_arg(arguments, unsigned);
        }
        break;
    case 1:
        symbol = va_arg(arguments, SymbolHeader *);
        instruction->record.symbol = symbol;
        ((LabelSymbol *)symbol)->instruction = instruction;
        break;
    case 2:
        instruction->record.symbol = va_arg(arguments, SymbolHeader *);
        break;
    case 3:
        break;
    case 4:
        instruction->record.source_location = va_arg(arguments, unsigned);
        break;
    case 7:
        block = (MachineBlockHeader *)instruction;
        block->uses = va_arg(arguments, BitSet *);
        block->definitions = va_arg(arguments, BitSet *);
        block->number = va_arg(arguments, int);
        block->flags = (unsigned short)va_arg(arguments, int);
        break;
    default:
        compiler_internal_error("@(#)codegen.c:1.167", 0x75c);
        break;
    }
    va_end(arguments);
    return instruction;
}
