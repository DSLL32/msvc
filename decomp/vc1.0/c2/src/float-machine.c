/* Floating virtual-register preparation and x87 stack tracking. */
#include "float-machine.h"
#include "backend-passes.h"
#include "scheduler.h"
#include "work-prepare.h"
#include "instruction-selection.h"
#include "intrinsic-selection.h"
#include <recon.h>
void *__fastcall float_register_heap_allocate(unsigned bytes);
int float_machine_register_count; /* 0007e1e8 */
int float_machine_conservative; /* 0007e1ec */
FloatMachineRegister *float_machine_registers; /* 0007e210 */
int float_machine_stack[8]; /* 0007e1f0..0007e210: virtual value at each ST slot */
int float_machine_virtual_depth; /* 0007e260 */
int float_machine_depth; /* 0007e25c */
int float_machine_range_entry_depth; /* 0007e264 */
int float_machine_maximum_depth; /* 0007e280 */
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

/* Signed two-bit x87 stack effects from the opcode property words. The
   contextual ISA classes replace a copied property table; the independent
   probe checks every original row, including zero-effect operations. */
#define FM_STACK_EFFECT(op) (((op) == MC_FCOMPP || (op) == MC_FUCOMPP) ? -2 : \
    (((op) >= MC_FADDP && (op) <= MC_FDIVRP) || (op) == MC_FICOMP || \
     (op) == MC_FISTP || (op) == MC_FCOMP || (op) == MC_FSTP || \
     (op) == MC_FBSTP || (op) == MC_FPATAN || (op) == MC_FYL2X || \
     (op) == MC_FYL2XP1 || (op) == MC_FUCOMP) ? -1 : \
    ((op) == MC_FILD || (op) == MC_FLD || (op) == MC_FBLD || \
     ((op) >= MC_FLDZ && (op) <= MC_FLDLN2) || (op) == MC_FPTAN || \
     (op) == MC_FXTRACT || (op) == MC_FLD1 || (op) == MC_FSINCOS) ? 1 : 0)
int __fastcall function_float_stack_depth(FunctionSymbol *symbol);
/* Original [0003ce00,0003d413). First find a control-flow boundary and
   measure the physical x87 depth. A reverse walk then summarizes each
   virtual register's future uses and the remaining stack pressure at stores. */
int __fastcall scan_float_machine_range(MachineInstruction *first, MachineInstruction **end)
{
    MachineInstruction *instruction, *following = 0;
    MachineOperand *operand;
    FloatMachineRegister *reg;
    unsigned operation, kind, number, flags;
    int depth = float_machine_incoming_depth, pending = 0, maximum_pending = 0;
    int unsafe = 0, has_float = 0, has_virtual = 0, effect;
    float_machine_range_entry_depth = float_machine_incoming_depth;
    float_machine_depth = float_machine_incoming_depth;
    float_machine_maximum_depth = 0;
    for (instruction = first; instruction; instruction = instruction->next) {
        operation = FM_OPERATION(instruction);
        kind = FM_KIND(instruction);
        if (FM_ELIGIBLE(instruction) && (operation == MC_JCC || operation == MC_JMP ||
            (operation == MC_CALL && !call_preserves_float_stack(instruction)) ||
            operation == MC_LEAVE || operation == MC_RET)) break;
        if (kind == 1 && FM_LABEL_BARRIER(instruction)) break;
        if (kind == 7) {
            if (((MachineBlockHeader *)instruction)->flags & 2) unsafe = 1;
        } else if (FM_ELIGIBLE(instruction) && (FM_FLOAT(instruction) || operation == MC_CALL)) {
            has_float = 1;
            if (operation == MC_CALL) {
                if (float_machine_maximum_depth < depth + function_float_stack_depth((FunctionSymbol *)instruction->operands[0].value.symbol))
                    float_machine_maximum_depth = depth + function_float_stack_depth((FunctionSymbol *)instruction->operands[0].value.symbol);
                depth -= intrinsic_float_cleanup_count((FunctionSymbol *)instruction->operands[0].value.symbol);
                float_machine_depth -= intrinsic_float_cleanup_count((FunctionSymbol *)instruction->operands[0].value.symbol);
                if (float_machine_depth < 0) unsafe = 1;
            } else {
                effect = FM_STACK_EFFECT(operation);
                if (effect == -2) {
                    float_machine_depth -= 2;
                    if (float_machine_depth < 0) unsafe = 1;
                    if (float_machine_maximum_depth < depth) float_machine_maximum_depth = depth;
                    depth -= 2;
                } else if (effect == -1) {
                    if (float_machine_maximum_depth < depth) float_machine_maximum_depth = depth;
                    --depth;
                    --float_machine_depth;
                    if (float_machine_depth < 0) unsafe = 1;
                } else if (effect == 1) {
                    ++depth;
                    ++float_machine_depth;
                }
            }
        }
    }
    *end = instruction;
    if (!has_float || !instruction) return 0;
    if (unsafe) {
        float_machine_depth = 2;
        return 1;
    }
    for (; instruction != first->previous; instruction = instruction->previous) {
        operation = FM_OPERATION(instruction);
        if (!FM_ELIGIBLE(instruction) || (!FM_FLOAT(instruction) &&
            (operation != MC_CALL || !call_preserves_float_stack(instruction)))) continue;
        if (operation != MC_CALL && machine_operand_counts[operation] != 0) {
            operand = &instruction->operands[0];
            number = operand->value.reg.words.number & 0x3fff;
            if ((operand->flags & 15) == OPERAND_REGISTER && number > 21) {
                reg = &float_machine_registers[number];
                if (operation == MC_FST || operation == MC_FSTP || operation == MC_FIST || operation == MC_FISTP) {
                    flags = instruction->record.float_flags;
                    flags = (flags & ~4U) | ((reg->flags >> 2) & 4);
                    flags = (flags & ~16U) | ((reg->flags << 1) & 16);
                    reg->flags |= 8;
                    flags = (flags & ~2U) | ((reg->flags >> 4) & 2);
                    reg->flags &= ~32U;
                    instruction->record.float_flags = (flags & ~8U) | ((reg->flags << 1) & 8);
                } else reg->flags |= 32 | 16;
                if (operand->flags & 0x400) reg->flags |= 4;
                has_virtual = 1;
            }
        }
        if (operation != MC_CALL && machine_operand_counts[operation] == 2) {
            operand = &instruction->operands[1];
            number = operand->value.reg.words.number & 0x3fff;
            if ((operand->flags & 15) == OPERAND_REGISTER && number > 21) {
                reg = &float_machine_registers[number];
                instruction->record.float_flags = (instruction->record.float_flags & ~4U) | ((reg->flags >> 2) & 4);
                if (!(reg->flags & 16)) instruction->record.float_flags |= 1;
                reg->flags |= 16;
                if (operation != MC_FLD || !following ||
                    (FM_OPERATION(following) != MC_FST && FM_OPERATION(following) != MC_FSTP)) reg->flags |= 32;
                if (operand->flags & 0x400) reg->flags |= 4;
                has_virtual = 1;
            }
        }
        following = instruction;
        if (operation == MC_CALL) {
            if (maximum_pending < pending + function_float_stack_depth((FunctionSymbol *)instruction->operands[0].value.symbol))
                maximum_pending = pending + function_float_stack_depth((FunctionSymbol *)instruction->operands[0].value.symbol);
            pending -= intrinsic_float_cleanup_count((FunctionSymbol *)instruction->operands[0].value.symbol);
            if (pending < 0) pending = 0;
        } else {
            effect = FM_STACK_EFFECT(operation);
            if (effect == -2) {
                if (maximum_pending < pending) maximum_pending = pending;
                if (pending > 1) pending -= 2;
            } else if (effect == -1) {
                if (maximum_pending < pending) maximum_pending = pending;
                if (pending > 0) --pending;
            } else if (effect == 1) ++pending;
            if (operation == MC_FIST || operation == MC_FISTP || operation == MC_FST || operation == MC_FSTP)
                instruction->record.float_flags = (instruction->record.float_flags & ~0x1e0U) | ((maximum_pending << 5) & 0x1e0);
        }
    }
    return has_virtual;
}
