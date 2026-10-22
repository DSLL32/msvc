#include "instruction-selection.h"
#include "function-input.h"
#include "instruction-rewrite.h"
#include "register-use.h"
/* Original [0001a620,0001a640). Kind 11 carries two signed control words. */
void __fastcall emit_machine_directive(short code, short value)
{
    MachineInstruction *instruction = append_machine_instruction(11);
    instruction->record.directive.code = code;
    instruction->record.directive.value = value;
}
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
            call->argument_bytes = va_arg(arguments, unsigned);
            call->register_mask = va_arg(arguments, unsigned);
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

/* Original [0001dd60,0001dd80). The selected stream has sentinel records
   on both sides of an instruction eligible for removal. */
void __fastcall remove_machine_instruction(MachineInstruction *instruction)
{
    instruction->previous->next = instruction->next;
    instruction->next->previous = instruction->previous;
    release_machine_instruction(instruction);
}

/* Original [0001a540,0001a550). Recycling retains the record's other words;
   the allocator clears the complete record when it is reused. */
void __fastcall release_machine_instruction(MachineInstruction *instruction)
{
    instruction->next = free_machine_instructions;
    free_machine_instructions = instruction;
}

/* Original [000197e0,00019800). Labels retain their machine-stream position
   for subsequent branch redirection. */
void __fastcall emit_label(SymbolHeader *symbol)
{
    MachineInstruction *instruction = append_machine_instruction(1);
    instruction->record.symbol = symbol;
    ((LabelSymbol *)symbol)->instruction = instruction;
}

/* Original [0001a730,0001a940), including its12-entry kind dispatch.
   Compare record payloads and, for string operations, their repeat prefixes.
   Physical register lifetime/byte-lane flags are not part of this comparison. */
int __fastcall machine_records_equal(MachineInstruction *left, MachineInstruction *right)
{
    unsigned difference = left->opcode ^ right->opcode;
    unsigned operation, properties;
    int count;
    MachineInstruction *left_prefix, *right_prefix;
    MachineBlockHeader *left_block, *right_block;
    if (difference & 15) return 0;
    switch (left->opcode & 15) {
    case 0: case 8: case 9: case 10:
        if (difference & 0xfff0U) return 0;
        operation = (left->opcode >> 8) & 255;
        count = machine_operand_counts[operation];
        if (count == 1) {
            if ((operation == MC_JCC || operation == MC_SCC) &&
                ((left->operands[1].flags ^ right->operands[1].flags) & 15)) return 0;
            if (!operands_equal(&left->operands[0], &right->operands[0])) return 0;
        } else if (count == 2) {
            if (!operands_equal(&left->operands[1], &right->operands[1]) ||
                !operands_equal(&left->operands[0], &right->operands[0])) return 0;
        }
        properties = machine_selection_properties[operation];
        if (properties & MC_STRING_OPERATION) {
            left_prefix = previous_machine_operation(left);
            right_prefix = previous_machine_operation(right);
            if (!left_prefix) {
                if (right_prefix &&
                    (machine_selection_properties[(right_prefix->opcode >> 8) & 255] & MC_REPEAT_PREFIX))
                    return 0;
            } else {
                if (!right_prefix &&
                    (machine_selection_properties[(left_prefix->opcode >> 8) & 255] & MC_REPEAT_PREFIX))
                    return 0;
                /* Native streams have a preceding operation here unless the
                   missing prefix already fails the repeat-prefix check. */
                if (((machine_selection_properties[(left_prefix->opcode >> 8) & 255] ^
                      machine_selection_properties[(right_prefix->opcode >> 8) & 255]) & MC_REPEAT_PREFIX) ||
                    !machine_records_equal(left_prefix, right_prefix)) return 0;
            }
        }
        return 1;
    case 1:
        return left->record.symbol == right->record.symbol;
    case 7:
        left_block = (MachineBlockHeader *)left;
        right_block = (MachineBlockHeader *)right;
        return left_block->number == right_block->number &&
            left_block->uses == right_block->uses && left_block->definitions == right_block->definitions;
    case 11:
        return left->record.directive.code == right->record.directive.code &&
            left->record.directive.value == right->record.directive.value;
    default:
        compiler_internal_error("@(#)codegen.c:1.167", 0x8d5);
        return 1;
    }
}
