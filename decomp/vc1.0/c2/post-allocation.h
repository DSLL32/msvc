#ifndef POST_ALLOCATION_H
#define POST_ALLOCATION_H
#include "instruction-selection.h"
/* Native2e6a0 allocates eight-byte copy-list nodes in arena3. */
typedef struct PhysicalRegisterCopy {
    MachineInstruction *instruction;
    struct PhysicalRegisterCopy *next;
} PhysicalRegisterCopy;
typedef char PhysicalRegisterCopySizeCheck[sizeof(PhysicalRegisterCopy) == 8 ? 1 : -1];
/*31 heads7c634..7c6b0, recycler7c6b0. Adjacent allocation statistics
  end7c634 and physical costs start7c6b4; these are distinct objects. */
extern PhysicalRegisterCopy *physical_register_copies[31];
extern PhysicalRegisterCopy *free_physical_register_copies;
void __fastcall prepare_machine_blocks(WorkItem *work);
void __fastcall remember_physical_copy(PhysicalRegisterCopy **copies, MachineInstruction *instruction);
void __fastcall release_physical_copies(PhysicalRegisterCopy *copies);
int __fastcall propagate_physical_copy(MachineInstruction *instruction);
int __fastcall propagate_physical_operand(MachineInstruction *instruction, MachineOperand *operand, unsigned index);
int __fastcall equivalent_stack_copy_operands(MachineInstruction *first, MachineOperand *first_operand,
    MachineInstruction *last, MachineOperand *last_operand);
int __fastcall stack_copy_operand_changed(MachineInstruction *first, MachineInstruction *last,
    MachineOperand *operand, unsigned bytes);
#endif
