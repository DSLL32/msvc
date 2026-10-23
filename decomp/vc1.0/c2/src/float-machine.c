/* Floating virtual-register preparation and x87 stack tracking. */
#include "float-machine.h"
#include "backend-passes.h"
#include "scheduler.h"
#include <recon.h>
void *__fastcall float_register_heap_allocate(unsigned bytes);
int float_machine_register_count; /* 0007e1e8 */
int float_machine_conservative; /* 0007e1ec */
FloatMachineRegister *float_machine_registers; /* 0007e210 */
int float_machine_stack[8]; /* 0007e1f0..0007e210: virtual value at each ST slot */
int float_machine_virtual_depth; /* 0007e260 */
int float_machine_depth; /* 0007e25c */
int float_machine_incoming_depth; /* 0007e274 */
#define FM_KIND(i) ((i)->opcode & 15)
#define FM_OPERATION(i) (((i)->opcode >> 8) & 255)
#define FM_ELIGIBLE(i) (FM_KIND(i) == 0 || FM_KIND(i) == 8 || FM_KIND(i) == 9 || FM_KIND(i) == 10)
/* Original raw opcode-property bit1, independently checked across the ISA
   table by the floating peephole property proof. */
#define FM_FLOAT(i) ((FM_OPERATION(i) >= MC_FADD && FM_OPERATION(i) <= MC_FLD1) || \
    (FM_OPERATION(i) >= MC_FUCOM && FM_OPERATION(i) <= MC_FCOS))
#define FM_LABEL_BARRIER(i) (!(i)->record.symbol || ((LabelSymbol *)(i)->record.symbol)->references || ((LabelSymbol *)(i)->record.symbol)->label_kind == 4)
/* Original [0003cb30,0003cd52). Scan each range before rewriting its floating
   instructions. Calls with a known small floating signature may stay inside
   a range; other calls, branches and visible labels flush the stack state. */
void __fastcall prepare_float_machine_data(int has_protected_blocks)
{
    MachineInstruction *instruction = machine_instructions, *end, *next;
    int has_virtual_registers;
    unsigned operation, kind;
    float_machine_register_count = (unsigned)next_tree_register + 1;
    float_machine_registers = float_register_heap_allocate(float_machine_register_count * sizeof(*float_machine_registers));
    initialize_float_register_homes();
    reset_float_register_stack();
    float_machine_conservative = 0;
    has_virtual_registers = scan_float_machine_range(instruction, &end);
    if (float_machine_depth != 0 && float_machine_depth != 1) float_machine_conservative = 1;
    while (instruction) {
        if (!has_virtual_registers && !float_machine_conservative) instruction = end;
        else {
            if (has_protected_blocks) float_machine_conservative = 1;
            while (instruction != end) {
                if (FM_ELIGIBLE(instruction) && (FM_FLOAT(instruction) || FM_OPERATION(instruction) == MC_CALL)) {
                    prepare_float_machine_instruction(instruction, &next);
                    instruction = next->previous;
                }
                instruction = instruction->next;
            }
        }
        if (!instruction) break;
        operation = FM_OPERATION(instruction);
        kind = FM_KIND(instruction);
        if ((FM_ELIGIBLE(instruction) && (operation == MC_JCC || operation == MC_JMP ||
             (operation == MC_CALL && !call_preserves_float_stack(instruction)) ||
             operation == MC_LEAVE || operation == MC_RET)) ||
            (kind == 1 && FM_LABEL_BARRIER(instruction))) {
            finish_float_machine_range(instruction);
            reset_float_register_stack();
            kind = FM_KIND(instruction);
            operation = FM_OPERATION(instruction);
            if (FM_ELIGIBLE(instruction)) {
                if (operation == MC_CALL) float_machine_incoming_depth = call_float_result_count(instruction);
                else if (operation == MC_JCC || operation == MC_JMP) {
                    float_machine_incoming_depth = 0;
                    float_machine_conservative = 0;
                }
            } else if (kind == 1) {
                float_machine_incoming_depth = 0;
                float_machine_conservative = 0;
            }
            has_virtual_registers = scan_float_machine_range(instruction->next, &end);
            if (!float_machine_conservative && float_machine_depth != 0 && float_machine_depth != 1)
                float_machine_conservative = 1;
        }
    }
    exclude_float_machine_registers();
    release_float_machine_data();
}

/* Original [0003e230,0003e277). Home operands and widths are reset once
   per function. Other record fields belong to the per-range reset. */
void initialize_float_register_homes(void)
{
    int index;
    float_machine_conservative = 0;
    for (index = 0; index < float_machine_register_count; ++index) {
        float_machine_registers[index].home = 0;
        float_machine_registers[index].width = 0;
    }
    float_machine_depth = 0;
    float_machine_incoming_depth = 0;
}

/* Original [0003d420,0003d491). No virtual value is initially resident.
   Six per-range flags are cleared while unrelated flag bits are retained. */
void reset_float_register_stack(void)
{
    int index;
    for (index = 0; index < float_machine_register_count; ++index) {
        float_machine_registers[index].stack_slot = -1;
        float_machine_registers[index].flags &= ~63U;
    }
    for (index = 0; index < 8; ++index) float_machine_stack[index] = -1;
    float_machine_depth = 0;
    float_machine_virtual_depth = 0;
}
