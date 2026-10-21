/* Per-function machine-stream and temporary-register state. */
#include "machine.h"
#include "frame.h"
MachineInstruction *machine_tail; /* 0007b94c */
MachineInstruction *free_machine_instructions; /* 0007b950 */
MachineInstruction *deferred_instructions, *deferred_tail; /* 0007b954, 0007b958 */
void *machine_unused_pool; /* 0007b95c: this build only clears the slot */
MachineAddress *free_machine_addresses; /* 0007b960 */
BitSet *machine_register_set; /* 0007b964 */
unsigned short next_machine_register = 32; /* 0008000c */
unsigned short next_tree_register = 32; /* 000838a8 */
/* Original [00068300,00068370). */
void reset_machine_state(void)
{
    machine_register_set = 0;
    free_machine_addresses = 0;
    machine_unused_pool = 0;
    machine_instructions = 0;
    deferred_tail = 0;
    deferred_instructions = 0;
    free_machine_instructions = 0;
    machine_tail = 0;
    next_machine_register = 32;
    next_tree_register = 32;
    reset_stack_frame();
}
