#include "instruction-selection.h"
#include <stdarg.h>
/* Original [00017cf0,00017e00). The opcode determines how many operands
   follow type; conditional operations take one additional condition code.
   Only floating classification and x87 depth effects are used from the
   original opcode properties here, expressed with the compiler's opcode names. */
MachineInstruction *__cdecl emit_machine_operation(unsigned opcode, unsigned type, ...)
{
    MachineInstruction *instruction = append_machine_instruction(0);
    MachineOperand *operand;
    int count;
    va_list arguments;
    instruction->opcode = (instruction->opcode & ~0xff00U) | ((opcode << 8) & 0xff00);
    instruction->opcode = (instruction->opcode & ~0xf0U) | ((type << 4) & 0xf0);
    va_start(arguments, type);
    count = machine_operand_counts[opcode];
    if (count == 1 || count == 2) {
        operand = va_arg(arguments, MachineOperand *);
        instruction->operands[0] = *operand;
        if (count == 2) {
            operand = va_arg(arguments, MachineOperand *);
            instruction->operands[1] = *operand;
        }
    }
    if ((opcode >= MC_FADD && opcode <= MC_FLD1) ||
        (opcode >= MC_FUCOM && opcode <= MC_FCOS)) {
        current_function->field28 |= 8;
        selected_block->flags |= 0x400;
    }
    if (opcode == MC_FCOMPP || opcode == MC_FUCOMPP) x87_stack_depth -= 2;
    else if ((opcode >= MC_FADDP && opcode <= MC_FDIVRP) ||
        opcode == MC_FICOMP || opcode == MC_FISTP || opcode == MC_FCOMP ||
        opcode == MC_FSTP || opcode == MC_FBSTP || opcode == MC_FPATAN ||
        opcode == MC_FYL2X || opcode == MC_FYL2XP1 || opcode == MC_FUCOMP)
        --x87_stack_depth;
    else if (opcode == MC_FILD || opcode == MC_FLD || opcode == MC_FBLD ||
        (opcode >= MC_FLDZ && opcode <= MC_FLDLN2) || opcode == MC_FPTAN ||
        opcode == MC_FXTRACT || opcode == MC_FLD1 || opcode == MC_FSINCOS)
        ++x87_stack_depth;
    if (x87_stack_depth < 0) compiler_internal_error("@(#)codegen.c:1.167", 0x78a);
    if (x87_stack_depth > 8) compiler_internal_error("@(#)codegen.c:1.167", 0x78b);
    if (opcode == MC_JCC || opcode == MC_SCC)
        instruction->operands[1].flags = (instruction->operands[1].flags & ~15U) |
                                        (va_arg(arguments, unsigned) & 15);
    va_end(arguments);
    return instruction;
}
