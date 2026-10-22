#include "instruction-selection.h"
unsigned selected_save_register_mask; /* 0007c950 */
/* Original [00019560,000195a0). Emit saves in register order; the epilogue
   restores the same mask in reverse order. This is selection state, distinct
   from the final output saved-register mask at 0007e8a8. */
void emit_saved_register_pushes(void)
{
    int reg;
    MachineOperand *operand;
    for (reg = 0; reg < 8; ++reg) {
        if (selected_save_register_mask & (1U << reg)) {
            operand = new_operand(OPERAND_REGISTER, (unsigned short)reg, 0, (SymbolHeader *)0);
            emit_machine_operation(MC_EPUSH, 0x104, operand);
        }
    }
}
