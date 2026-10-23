/* Floating virtual-register preparation and x87 stack tracking. */
#include <stdlib.h>
#include "float-machine.h"
#include "backend-passes.h"
#include "scheduler.h"
#include "work-prepare.h"
#include "instruction-selection.h"
#include "intrinsic-selection.h"
#include "instruction-rewrite.h"
#include "value-selection.h"
#include "records.h"
#include "global-registers.h"
#include "float-peepholes.h"
#include "register-use.h"
#include "machine-layout.h"
#include <recon.h>
void *__fastcall float_register_heap_allocate(unsigned bytes);
extern int cpu_model, optimize_alias;
int float_machine_register_count; /* 0007e1e8 */
int float_machine_conservative; /* 0007e1ec */
FloatMachineRegister *float_machine_registers; /* 0007e210 */
int float_machine_stack[8]; /* 0007e1f0..0007e210: virtual value at each ST slot */
int float_machine_virtual_depth; /* 0007e260 */
int float_machine_exit_depth; /* 0007e25c */
int float_machine_range_entry_depth; /* 0007e264 */
int float_machine_maximum_depth; /* 0007e280 */
int float_machine_stack_depth; /* 0007e274 */
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
    if (float_machine_exit_depth != 0 && float_machine_exit_depth != 1) float_machine_conservative = 1;
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
                if (operation == MC_CALL) float_machine_stack_depth = call_float_result_count(instruction);
                else if (operation == MC_JCC || operation == MC_JMP) {
                    float_machine_stack_depth = 0;
                    float_machine_conservative = 0;
                }
            } else if (kind == 1) {
                float_machine_stack_depth = 0;
                float_machine_conservative = 0;
            }
            has_virtual_registers = scan_float_machine_range(instruction->next, &end);
            if (!float_machine_conservative && float_machine_exit_depth != 0 && float_machine_exit_depth != 1)
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
    float_machine_exit_depth = 0;
    float_machine_stack_depth = 0;
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
    float_machine_exit_depth = 0;
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
    int depth = float_machine_stack_depth, pending = 0, maximum_pending = 0;
    int unsafe = 0, has_float = 0, has_virtual = 0, effect;
    float_machine_range_entry_depth = float_machine_stack_depth;
    float_machine_exit_depth = float_machine_stack_depth;
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
                float_machine_exit_depth -= intrinsic_float_cleanup_count((FunctionSymbol *)instruction->operands[0].value.symbol);
                if (float_machine_exit_depth < 0) unsafe = 1;
            } else {
                effect = FM_STACK_EFFECT(operation);
                if (effect == -2) {
                    float_machine_exit_depth -= 2;
                    if (float_machine_exit_depth < 0) unsafe = 1;
                    if (float_machine_maximum_depth < depth) float_machine_maximum_depth = depth;
                    depth -= 2;
                } else if (effect == -1) {
                    if (float_machine_maximum_depth < depth) float_machine_maximum_depth = depth;
                    --depth;
                    --float_machine_exit_depth;
                    if (float_machine_exit_depth < 0) unsafe = 1;
                } else if (effect == 1) {
                    ++depth;
                    ++float_machine_exit_depth;
                }
            }
        }
    }
    *end = instruction;
    if (!has_float || !instruction) return 0;
    if (unsafe) {
        float_machine_exit_depth = 2;
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
/* Address identity of these original operand rows does not escape: the
   dispatcher copies an eight-byte ST operand selected by its stack slot. */
#define FM_SET_STACK_OPERAND(operand, slot) do { \
    (operand).flags = OPERAND_REGISTER; \
    (operand).value.reg.value = REG_ST0 + (slot); \
} while (0)
/* Original six-row pop/reversal view82f60..82f78. ADD/MUL are symmetric. */
#define FM_REVERSE_POP(op) ((op) == MC_FSUBP ? MC_FSUBRP : \
    (op) == MC_FSUBRP ? MC_FSUBP : (op) == MC_FDIVP ? MC_FDIVRP : \
    (op) == MC_FDIVRP ? MC_FDIVP : (op))
#define FM_SET_OPERATION(i, op) ((i)->opcode = ((i)->opcode & ~0xff00U) | ((op) << 8))
#define FM_PRESERVE_TOP(invalid_code, depth_code, unbind) do { \
    if (float_machine_stack[0] != -1) { \
        number = float_machine_stack[0]; \
        if (number < 0 || number >= float_machine_register_count) \
            compiler_internal_error("@(#)fltreg.c:1.31", invalid_code); \
        reg = &float_machine_registers[number]; \
        if (!(reg->flags & 2) && find_float_register_reload(instruction, (unsigned short)number, &reload)) \
            save_float_machine_register(instruction, reg, (unsigned short)number, reload); \
        --float_machine_virtual_depth; \
        if (float_machine_virtual_depth < 0) compiler_internal_error("@(#)fltreg.c:1.31", depth_code); \
        if (unbind) { \
            float_machine_registers[float_machine_stack[0]].stack_slot = -1; \
            float_machine_stack[0] = -1; \
        } \
    } \
} while (0)
/* Original [0003d770,0003e1a1), plus switch rows3e1a4..3e22c. The caller
   supplies a next-instruction cursor because a redundant store can vanish.
   Its only caller ignores EAX; the incidental register value at RET is not
   a returned instruction. Both arguments are ECX/EDX, with plain RET. */
void __fastcall prepare_float_machine_instruction(MachineInstruction *instruction, MachineInstruction **next)
{
    FloatMachineRegister *reg;
    MachineInstruction *reload, *exchange;
    unsigned operation;
    int number = -1, slot, keep_top = 0, removed = 0, count;
    *next = instruction->next;
    if (float_machine_conservative && float_machine_virtual_depth > 0)
        compiler_internal_error("@(#)fltreg.c:1.31", 0x10cf);
    if (FM_OPERATION(instruction) == MC_FSTP && float_machine_stack[0] != -1 &&
        ((instruction->opcode >> 4) & 15) != 10 &&
        ((instruction->operands[0].flags & 15) != OPERAND_REGISTER ||
         (instruction->operands[0].value.reg.words.number & 0x3fff) <= 21) &&
        can_keep_float_store(instruction, (unsigned short)float_machine_stack[0], 0))
        FM_SET_OPERATION(instruction, MC_FST);
    operation = FM_OPERATION(instruction);
    if (machine_operand_counts[operation] != 0) {
        if (FM_FLOAT(instruction) && (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
            (instruction->operands[0].value.reg.words.number & 0x3fff) > 21) {
            if (operation != MC_FST && operation != MC_FSTP)
                compiler_internal_error("@(#)fltreg.c:1.31", 0x10df);
            number = instruction->operands[0].value.reg.words.number & 0x3fff;
            reg = &float_machine_registers[number];
            slot = reg->stack_slot;
            if (slot != -1) FM_SET_STACK_OPERAND(instruction->operands[0], slot);
            else if (!float_machine_conservative &&
                (float_machine_stack_depth > 0 || operation != MC_FSTP) &&
                float_machine_range_entry_depth + float_machine_stack_depth <
                    8 - (int)((instruction->record.float_flags >> 5) & 15) && float_machine_stack[0] == -1) {
                if (number >= float_machine_register_count || number <= 0)
                    compiler_internal_error("@(#)fltreg.c:1.31", 0x111b);
                reg->width = (instruction->opcode >> 4) & 15;
                if (!float_store_value_dead(instruction) ||
                    (!(instruction->record.float_flags & 2) && previous_float_is_load(instruction))) {
                    if (!reg->home) {
                        reg->home = arena_alloc(3, sizeof(*reg->home));
                        initialize_float_register_home(reg->home, (unsigned short)number);
                    }
                    instruction->record.float_flags |= 0x200;
                    assign_machine_operand(&instruction->operands[0], reg->home);
                    reg->flags |= 2;
                    reg->last_store = instruction;
                    if (cpu_model < 4 && FM_OPERATION(instruction) == MC_FSTP &&
                        ((instruction->opcode >> 4) & 15) != 10 &&
                        can_keep_float_store(instruction, (unsigned short)number, 1))
                        FM_SET_OPERATION(instruction, MC_FST);
                    if (FM_OPERATION(instruction) == MC_FSTP && float_machine_stack[0] != -1 &&
                        ((instruction->opcode >> 4) & 15) != 10 &&
                        can_keep_float_store(instruction, (unsigned short)float_machine_stack[0], 0))
                        FM_SET_OPERATION(instruction, MC_FST);
                    if (FM_OPERATION(instruction) == MC_FST) {
                        keep_top = 1;
                        ++float_machine_virtual_depth;
                    }
                    reg->flags |= 2;
                } else {
                    *next = instruction->next;
                    remove_machine_instruction(instruction);
                    removed = 1;
                    reg->flags &= ~2U;
                    reg->last_store = 0;
                    keep_top = 1;
                    ++float_machine_virtual_depth;
                }
            } else {
                if (number >= float_machine_register_count)
                    compiler_internal_error("@(#)fltreg.c:1.31", 0x10f5);
                reg->width = (instruction->opcode >> 4) & 15;
                if (!reg->home) {
                    reg->home = arena_alloc(3, sizeof(*reg->home));
                    initialize_float_register_home(reg->home, (unsigned short)number);
                }
                assign_machine_operand(&instruction->operands[0], reg->home);
                reg->flags |= 2;
                reg->last_store = instruction;
                if (float_machine_stack_depth > 0 &&
                    float_machine_range_entry_depth + float_machine_stack_depth <
                        8 - (int)((instruction->record.float_flags >> 5) & 15) &&
                    FM_OPERATION(instruction) == MC_FSTP && float_machine_stack[0] != -1 &&
                    ((instruction->opcode >> 4) & 15) != 10 &&
                    can_keep_float_store(instruction, (unsigned short)float_machine_stack[0], 0))
                    FM_SET_OPERATION(instruction, MC_FST);
            }
        }
        if (removed) goto finish;
        operation = FM_OPERATION(instruction);
        if (machine_operand_counts[operation] == 2 &&
            (instruction->operands[1].flags & 15) == OPERAND_REGISTER &&
            (instruction->operands[1].value.reg.words.number & 0x3fff) > 21) {
            number = instruction->operands[1].value.reg.words.number & 0x3fff;
            if (number >= float_machine_register_count || number <= 0)
                compiler_internal_error("@(#)fltreg.c:1.31", 0x1160);
            float_machine_registers[number].width = (instruction->opcode >> 4) & 15;
            removed = prepare_float_register_source(instruction, number);
        }
    }
    if (removed) goto finish;
    operation = FM_OPERATION(instruction);
    switch (operation) {
    case MC_CALL:
        FM_PRESERVE_TOP(0x1253, 0x125e, 1);
        count = 0;
        if ((instruction->operands[0].flags & 15) == OPERAND_SYMBOL)
            count = intrinsic_float_cleanup_count((FunctionSymbol *)instruction->operands[0].value.symbol);
        while (count > 0) {
            pop_float_machine_stack();
            --count;
        }
        break;
    case MC_FADD: case MC_FMUL: case MC_FSUB: case MC_FSUBR:
    case MC_FDIV: case MC_FDIVR: case MC_FIADD: case MC_FIDIV:
    case MC_FIDIVR: case MC_FIMUL: case MC_FISUB: case MC_FISUBR:
    case MC_F2XM1: case MC_FABS: case MC_FCHS: case MC_FPREM:
    case MC_FRNDINT: case MC_FSCALE: case MC_FSQRT:
    case MC_FPREM1: case MC_FSIN: case MC_FCOS:
        FM_PRESERVE_TOP(0x1232, 0x123d, 1);
        break;
    case MC_FADDP: case MC_FMULP: case MC_FSUBP: case MC_FSUBRP:
    case MC_FDIVP: case MC_FDIVRP: case MC_FPATAN: case MC_FYL2X: case MC_FYL2XP1:
        FM_PRESERVE_TOP(0x11af, 0x11b8, 0);
        if ((machine_operand_counts[FM_OPERATION(instruction)] == 0 ||
            (instruction->operands[0].value.reg.words.number & 0x3fff) == REG_ST0 + 1) &&
            float_machine_stack[1] != -1) {
            number = float_machine_stack[1];
            if (number < 0 || number >= float_machine_register_count)
                compiler_internal_error("@(#)fltreg.c:1.31", 0x11cb);
            reg = &float_machine_registers[number];
            if (!(reg->flags & 2) && find_float_register_reload(instruction, (unsigned short)number, &reload)) {
                exchange = new_machine_instruction(0);
                FM_SET_OPERATION(exchange, MC_FXCH);
                exchange->opcode = (exchange->opcode & ~0xf0U) | 0x80;
                exchange->operands[0] = operand_st1;
                insert_machine_after(instruction->previous, exchange);
                save_float_machine_register(instruction, reg, (unsigned short)number, reload);
                operation = FM_OPERATION(instruction);
                if (operation >= MC_FADDP && operation <= MC_FDIVRP)
                    FM_SET_OPERATION(instruction, FM_REVERSE_POP(operation));
                else {
                    exchange = new_machine_instruction(0);
                    FM_SET_OPERATION(exchange, MC_FXCH);
                    exchange->opcode = (exchange->opcode & ~0xf0U) | 0x80;
                    exchange->operands[0] = operand_st1;
                    insert_machine_after(instruction->previous, exchange);
                }
            }
            reg->stack_slot = -1;
            float_machine_stack[1] = -1;
            --float_machine_virtual_depth;
            if (float_machine_virtual_depth < 0)
                compiler_internal_error("@(#)fltreg.c:1.31", 0x11e8);
        }
        pop_float_machine_stack();
        break;
    case MC_FXTRACT: case MC_FPTAN: case MC_FSINCOS:
        FM_PRESERVE_TOP(0x1188, 0x1193, 1);
        push_float_machine_stack();
        break;
    case MC_FILD: case MC_FLD: case MC_FBLD:
    case MC_FLDZ: case MC_FLDPI: case MC_FLDL2E: case MC_FLDL2T:
    case MC_FLDLG2: case MC_FLDLN2: case MC_FLD1:
        push_float_machine_stack();
        break;
    case MC_FICOMP: case MC_FISTP: case MC_FCOMP: case MC_FSTP: case MC_FBSTP: case MC_FUCOMP:
        FM_PRESERVE_TOP(0x11ff, 0x1208, 0);
        pop_float_machine_stack();
        break;
    case MC_FCOMPP: case MC_FUCOMPP:
        pop_float_machine_stack();
        pop_float_machine_stack();
        break;
    case MC_FXCH:
        exchange_float_machine_stack(0, (instruction->operands[0].value.reg.words.number & 0x3fff) - REG_ST0);
        break;
    default:
        break;
    }
finish:
    if (keep_top) {
        float_machine_stack[0] = number;
        float_machine_registers[number].stack_slot = 0;
    }
}

/* Original [0003e280,0003e2fd). Push an unbound physical ST(0), move
   resident virtual values down one slot, and enforce the x87 depth limit. */
void push_float_machine_stack(void)
{
    int index;
    for (index = 7; index > 0; --index) float_machine_stack[index] = float_machine_stack[index - 1];
    float_machine_stack[0] = -1;
    for (index = 0; index < float_machine_register_count; ++index) {
        if (float_machine_registers[index].stack_slot != -1) {
            ++float_machine_registers[index].stack_slot;
            if (float_machine_registers[index].stack_slot >= 8)
                compiler_internal_error("@(#)fltreg.c:1.31", 0x12c3);
        }
    }
    ++float_machine_stack_depth;
    if (float_machine_stack_depth > 8) compiler_internal_error("@(#)fltreg.c:1.31", 0x12c8);
}

/* Original [0003f1f0,0003f208). The reverse-use summary proves a dead
   value when its kill flag is set, or when both later-use tests agree. */
int __fastcall float_store_value_dead(MachineInstruction *instruction)
{
    unsigned flags = instruction->record.float_flags;
    return (flags & 8) != 0 || ((flags & 2) != 0 && (flags & 16) != 0);
}

/* Original [0003f130,0003f168). A remembered floating declaration supplies
   the symbol for its EBP-based backing operand; no synthetic home is made. */
void __fastcall initialize_float_register_home(MachineOperand *home, unsigned short number)
{
    FloatRegisterDeclaration *declaration;
    for (declaration = float_register_declarations; declaration; declaration = declaration->next) {
        if (declaration->number == number) {
            initialize_operand(home, OPERAND_BASED, REG_EBP, 0, declaration->symbol);
            return;
        }
    }
    compiler_internal_error("@(#)fltreg.c:1.31", 0x14d2);
}

/* Original [0003e610,0003e65d). Pop the physical stack and decrement
   resident virtual positions; position0 becomes the absent sentinel. */
void pop_float_machine_stack(void)
{
    int index;
    for (index = 0; index < 7; ++index) float_machine_stack[index] = float_machine_stack[index + 1];
    float_machine_stack[7] = -1;
    for (index = 0; index < float_machine_register_count; ++index)
        if (float_machine_registers[index].stack_slot != -1) --float_machine_registers[index].stack_slot;
    --float_machine_stack_depth;
}

/* Original [0003d4a0,0003d770). Reconcile the virtual stack with the
   physical depth required at a boundary. Preserve one real result at the
   bottom when needed, then store or discard each extra virtual value. */
void __fastcall finish_float_machine_range(MachineInstruction *boundary)
{
    MachineInstruction *instruction, *store;
    FloatMachineRegister *reg;
    int index, live = 0, number;
    if (float_machine_exit_depth == float_machine_stack_depth) return;
    for (index = 0; index < float_machine_stack_depth; ++index)
        if (float_machine_stack[index] != -1) ++live;
    if (FM_ELIGIBLE(boundary) && FM_OPERATION(boundary) == MC_LEAVE) {
        do { boundary = boundary->previous; } while (FM_KIND(boundary) != 7);
    }
    if (!live) return;
    if (float_machine_conservative) compiler_internal_error("@(#)fltreg.c:1.31", 0x1477);
    if (float_machine_exit_depth + live < float_machine_stack_depth)
        compiler_internal_error("@(#)fltreg.c:1.31", 0x147a);
    if (float_machine_exit_depth != 1 && float_machine_exit_depth != 0)
        compiler_internal_error("@(#)fltreg.c:1.31", 0x147b);
    if (float_machine_exit_depth == 1 && float_machine_stack_depth > 1) {
        instruction = new_machine_instruction(0);
        FM_SET_OPERATION(instruction, MC_FXCH);
        instruction->opcode = (instruction->opcode & ~0xf0U) | 0x80;
        FM_SET_STACK_OPERAND(instruction->operands[0], float_machine_stack_depth - 1);
        insert_machine_after(boundary->previous, instruction);
        number = float_machine_stack[float_machine_stack_depth - 1];
        if (number == -1) compiler_internal_error("@(#)fltreg.c:1.31", 0x1487);
        float_machine_stack[float_machine_stack_depth - 1] = float_machine_stack[0];
        float_machine_stack[0] = number;
        float_machine_registers[number].stack_slot = 0;
    }
    while (float_machine_exit_depth < float_machine_stack_depth) {
        number = float_machine_stack[0];
        if (number >= float_machine_register_count || number == -1)
            compiler_internal_error("@(#)fltreg.c:1.31", 0x1490);
        reg = &float_machine_registers[number];
        store = new_machine_instruction(0);
        FM_SET_OPERATION(store, MC_FSTP);
        store->opcode = (store->opcode & ~0xf0U) | ((reg->width << 4) & 0xf0);
        store->operands[1] = operand_st0;
        if (!(reg->flags & 3)) {
            if (!reg->home) {
                reg->home = arena_alloc(3, sizeof(*reg->home));
                initialize_float_register_home(reg->home, (unsigned short)float_machine_stack[0]);
            }
            assign_machine_operand(&store->operands[0], reg->home);
        } else if (reg->last_store && FM_OPERATION(reg->last_store) == MC_FST) {
            remove_machine_instruction(reg->last_store);
            if (!(reg->flags & 1)) assign_machine_operand(&store->operands[0], reg->home);
            else store->operands[0] = operand_st0;
        } else store->operands[0] = operand_st0;
        pop_float_machine_stack();
        insert_machine_after(boundary->previous, store);
    }
    if (float_machine_exit_depth != float_machine_stack_depth)
        compiler_internal_error("@(#)fltreg.c:1.31", 0x14ba);
    float_machine_stack_depth = float_machine_exit_depth;
}
/* Original [0003e660,0003ee05). Materialize a virtual floating source from
   its home or resident ST slot. Last uses can consume the slot, and adjacent
   loads or popping arithmetic can absorb the needed exchange. */
int __fastcall prepare_float_register_source(MachineInstruction *instruction, int number)
{
    FloatMachineRegister *reg;
    MachineInstruction *previous, *following, *exchange;
    MachineOperand *source = &instruction->operands[1];
    const X87ArithmeticForms *forms;
    unsigned operation, previous_operation, kind;
    int slot;
    if (number >= float_machine_register_count || number <= 0)
        compiler_internal_error("@(#)fltreg.c:1.31", 0x131e);
    reg = &float_machine_registers[number];
    if (reg->stack_slot == -1) {
        instruction->record.float_flags |= 0x200;
        if (!reg->home) {
            if (!reg->width) reg->width = (instruction->opcode >> 4) & 15;
            reg->home = arena_alloc(3, sizeof(*reg->home));
            initialize_float_register_home(reg->home, (unsigned short)number);
        }
        assign_machine_operand(source, reg->home);
        return 0;
    }
    if (reg->stack_slot >= 8) compiler_internal_error("@(#)fltreg.c:1.31", 0x1322);
    operation = FM_OPERATION(instruction);
    if (!(source->flags & 0x400) && (!(instruction->record.float_flags & 1) || !(reg->flags & 2))) {
        if (operation == MC_FLD) {
            following = instruction->next;
            previous_operation = FM_OPERATION(following);
            if (FM_ELIGIBLE(following) && FM_FLOAT(following) &&
                (float_machine_stack[0] != -1 || reg->stack_slot == 1) &&
                previous_operation >= MC_FADD && previous_operation <= MC_FDIVR &&
                operands_equal(source, &following->operands[1]) && (following->operands[1].flags & 0x400)) {
                if (reg->stack_slot == 0) {
                    remove_machine_instruction(instruction);
                    return 1;
                }
                FM_SET_OPERATION(instruction, MC_FXCH);
                instruction->opcode = (instruction->opcode & ~0xf0U) | 0x80;
                FM_SET_STACK_OPERAND(instruction->operands[0], reg->stack_slot);
                return 0;
            }
        }
        instruction->record.float_flags |= 0x200;
        reg->flags &= ~1U;
        FM_SET_STACK_OPERAND(*source, reg->stack_slot);
        if (((instruction->opcode >> 4) & 15) == 10)
            instruction->opcode = (instruction->opcode & ~0xf0U) | 0x80;
        return 0;
    }
    reg->flags = (reg->flags & ~1U) | ((source->flags >> 10) & 1);
    if (operation == MC_FLD) {
        if (reg->stack_slot == 0) {
            remove_machine_instruction(instruction);
            return 1;
        }
        if (float_machine_stack[0] == -1 && reg->stack_slot != 1) {
            instruction->record.float_flags |= 0x200;
            FM_SET_STACK_OPERAND(*source, reg->stack_slot);
            if (((instruction->opcode >> 4) & 15) == 10)
                instruction->opcode = (instruction->opcode & ~0xf0U) | 0x80;
        } else {
            FM_SET_OPERATION(instruction, MC_FXCH);
            instruction->opcode = (instruction->opcode & ~0xf0U) | 0x80;
            FM_SET_STACK_OPERAND(instruction->operands[0], reg->stack_slot);
        }
        return 0;
    }
    if (operation < MC_FADD || operation > MC_FDIVR) goto ordinary_source;
    forms = &x87_arithmetic_forms[operation - MC_FADD];
    slot = reg->stack_slot;
    if (slot == 0) {
        *source = operand_st0;
        float_machine_registers[float_machine_stack[0]].stack_slot = -1;
        float_machine_stack[0] = -1;
        --float_machine_virtual_depth;
        if (float_machine_virtual_depth < 0) compiler_internal_error("@(#)fltreg.c:1.31", 0x135a);
        return 0;
    }
    if (slot == 1) {
        FM_SET_OPERATION(instruction, forms->right_popping);
        instruction->operands[0] = operand_st1;
        *source = operand_st0;
        if (number != float_machine_stack[1]) compiler_internal_error("@(#)fltreg.c:1.31", 0x1366);
        float_machine_registers[float_machine_stack[1]].stack_slot = -1;
        float_machine_stack[1] = -1;
        --float_machine_virtual_depth;
        if (float_machine_virtual_depth < 0) compiler_internal_error("@(#)fltreg.c:1.31", 0x136b);
        return 0;
    }
    previous = instruction->previous;
    previous_operation = FM_OPERATION(previous);
    kind = previous->operands[1].flags & 15;
    if (slot > 1 && float_machine_stack[1] != -1 && FM_ELIGIBLE(previous) && FM_FLOAT(previous) &&
        ((previous->opcode >> 4) & 15) != 10 && (previous_operation == MC_FLD || previous_operation == MC_FILD) &&
        (kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE ||
         (kind == OPERAND_REGISTER && (previous->operands[1].value.reg.words.number & 0x3fff) > 21))) {
        pop_float_machine_stack();
        if (FM_OPERATION(previous) == MC_FLD) FM_SET_OPERATION(instruction, forms->reversed);
        else FM_SET_OPERATION(instruction, forms->right_integer);
        instruction->opcode = (instruction->opcode & ~0xf0U) | (previous->opcode & 0xf0);
        FM_SET_OPERATION(previous, MC_FXCH);
        previous->opcode = (previous->opcode & ~0xf0U) | 0x80;
        *source = previous->operands[1];
        FM_SET_STACK_OPERAND(previous->operands[0], reg->stack_slot);
        float_machine_stack[reg->stack_slot] = float_machine_stack[0];
        /* Original compares the distinct marker26 here, not the absent -1.
           Preserve this narrow state rule rather than strengthening it. */
        if (float_machine_stack[0] != 26)
            float_machine_registers[float_machine_stack[0]].stack_slot = reg->stack_slot;
        float_machine_stack[0] = -1;
        reg->stack_slot = -1;
        --float_machine_virtual_depth;
        if (float_machine_virtual_depth < 0) compiler_internal_error("@(#)fltreg.c:1.31", 0x139a);
        return 0;
    }
    following = instruction->next;
    previous_operation = FM_OPERATION(following);
    if (FM_ELIGIBLE(following) && previous_operation >= MC_FADDP && previous_operation <= MC_FDIVRP && slot == 2) {
        FM_SET_OPERATION(instruction, forms->right_popping);
        FM_SET_STACK_OPERAND(instruction->operands[0], reg->stack_slot);
        *source = operand_st0;
        FM_SET_OPERATION(following, FM_REVERSE_POP(previous_operation));
        float_machine_stack[2] = -1;
        reg->stack_slot = -1;
        --float_machine_virtual_depth;
        if (float_machine_virtual_depth < 0) compiler_internal_error("@(#)fltreg.c:1.31", 0x13b0);
    } else if (float_machine_exit_depth != 0 && slot == 2) {
        FM_SET_OPERATION(instruction, forms->right_popping);
        FM_SET_STACK_OPERAND(instruction->operands[0], reg->stack_slot);
        *source = operand_st0;
        exchange = new_machine_instruction(0);
        FM_SET_OPERATION(exchange, MC_FXCH);
        exchange->opcode = (exchange->opcode & ~0xf0U) | 0x80;
        exchange->operands[0] = operand_st1;
        insert_machine_after(instruction, exchange);
        float_machine_stack[2] = -1;
        reg->stack_slot = -1;
        --float_machine_virtual_depth;
        if (float_machine_virtual_depth < 0) compiler_internal_error("@(#)fltreg.c:1.31", 0x13c8);
        if (float_machine_stack[1] != -1) {
            float_machine_registers[float_machine_stack[1]].stack_slot = 2;
            float_machine_stack[2] = float_machine_stack[1];
            float_machine_stack[1] = -1;
        }
    } else {
ordinary_source:
        instruction->record.float_flags |= 0x200;
        FM_SET_STACK_OPERAND(*source, reg->stack_slot);
    }
    return 0;
}

/* Original [0003a030,0003a05e). Only a direct known target function
   with recorded stack demand below eight can keep surrounding x87 values. */
int __fastcall call_preserves_float_stack(MachineInstruction *instruction)
{
    FunctionSymbol *symbol;
    if ((instruction->operands[0].flags & 15) != OPERAND_SYMBOL) return 0;
    symbol = (FunctionSymbol *)instruction->operands[0].value.symbol;
    if ((symbol->field24 & 15) != 5 || symbol->symbol.kind != 4) return 0;
    return function_float_stack_depth(symbol) < 8;
}

/* Original [0003cda0,0003cdf7). Virtual x87 values no longer participate
   in the ordinary physical-register uses and definitions of each block. */
void exclude_float_machine_registers(void)
{
    FloatRegisterDeclaration *declaration;
    MachineInstruction *instruction;
    MachineBlockHeader *block;
    BitSet *registers = 0;
    for (declaration = float_register_declarations; declaration; declaration = declaration->next)
        registers = bitset_include(registers, declaration->number);
    if (registers) {
        for (instruction = machine_instructions; instruction; instruction = next_machine_block(instruction)) {
            block = (MachineBlockHeader *)instruction;
            subtract_bitset(block->uses, registers);
            subtract_bitset(block->definitions, registers);
        }
    }
}

/* Original [0003cd60,0003cd95). Only the malloc-backed declaration links
   and register array belong to this pass; arena home operands survive it. */
void release_float_machine_data(void)
{
    FloatRegisterDeclaration *declaration = float_register_declarations, *next;
    while (declaration) {
        next = declaration->next;
        float_register_heap_release(declaration);
        declaration = next;
    }
    float_register_declarations = 0;
    float_register_heap_release(float_machine_registers);
}

/* Original [000394f0,000394fa). Private fastcall wrapper around CRT free. */
void __fastcall float_register_heap_release(void *allocation)
{
    free(allocation);
}

MemoryMotionContext *memory_motion_context; /* 0007e230 */
MemoryMotionContext *float_stream_context; /* 0007e228 */
FloatStreamUse **float_stream_register_uses; /* 0007e218 */
FloatStreamUse *float_stream_uses; /* 00082e6c: next link at1c */
FloatStreamUse **float_stream_sorted_uses; /* 00082e70: sorted pointer view */
int float_stream_register_count; /* 0007e270 */
int float_stream_order; /* 0007e21c */
MachineInstruction *float_stream_next; /* 0007e278: mutation-safe scan cursor */
/* Original [00038e20,00038fd3). A stream with computed jumps or protected
   block2 cannot supply the label/call flow required for floating use motion.
   Otherwise allocate only the encountered register map and let each record
   kind update its context, preserving a mutable next-instruction cursor. */
void prepare_float_stream_uses(void)
{
    MachineInstruction *instruction;
    unsigned kind, operation;
    unsigned short highest;
    for (instruction = machine_instructions; instruction; instruction = instruction->next) {
        kind = FM_KIND(instruction);
        operation = FM_OPERATION(instruction);
        if (FM_ELIGIBLE(instruction) && (operation == MC_JCC || operation == MC_JMP) &&
            (instruction->operands[0].flags & 15) != OPERAND_SYMBOL) return;
        if (kind == 7 && (((MachineBlockHeader *)instruction)->flags & 2)) return;
    }
    instruction = machine_instructions;
    highest = next_tree_register > next_machine_register ? next_tree_register : next_machine_register;
    float_stream_register_count = (unsigned)highest + 1;
    float_stream_register_uses = float_register_heap_allocate(float_stream_register_count * sizeof(*float_stream_register_uses));
    float_stream_uses = 0;
    float_stream_context = 0;
    memory_motion_context = 0;
    float_stream_sorted_uses = 0;
    float_machine_stack_depth = 0;
    if (!initialize_float_stream_context()) {
        float_register_heap_release(float_stream_register_uses);
        return;
    }
    float_stream_order = 1;
    while (instruction) {
        float_stream_next = instruction->next;
        if (FM_KIND(instruction) == 1) process_float_stream_label(instruction);
        kind = FM_KIND(instruction);
        operation = FM_OPERATION(instruction);
        if (kind == 7 && (((MachineBlockHeader *)instruction)->flags & 1))
            process_float_stream_protected_block(instruction);
        else if (FM_ELIGIBLE(instruction)) {
            if (operation == MC_CALL) process_float_stream_call(instruction);
            else if (operation == MC_JCC || operation == MC_JMP) process_float_stream_branch(instruction);
            else if (operation == MC_RET) process_float_stream_return(instruction);
            else if (FM_FLOAT(instruction)) process_float_stream_operation(instruction);
        }
        instruction = float_stream_next;
    }
    finish_float_stream_uses();
}

MemoryMotionContext *float_stream_first_context; /* 0007e220 */
MemoryMotionContext *float_stream_last_context; /* 0007e224 */
/* Original [00039500,00039582). Find the function header and its initial
   block, then construct one complete zeroed arena4 motion record. The
   register map is separately owned and has not been initialized here. */
int initialize_float_stream_context(void)
{
    MachineInstruction *instruction;
    MemoryMotionContext *context;
    unsigned *word, count;
    for (instruction = machine_instructions; instruction; instruction = instruction->next)
        if (FM_KIND(instruction) == 2) break;
    if (!instruction) return 0;
    while (FM_KIND(instruction) != 7) instruction = instruction->next;
    instruction = instruction->next;
    context = arena_alloc(4, sizeof(*context));
    float_stream_first_context = context;
    word = (unsigned *)context;
    count = sizeof(*context) / sizeof(*word);
    while (count--) *word++ = 0;
    context->boundary = instruction;
    float_stream_context = context;
    float_stream_last_context = context;
    return 1;
}

MachineInstruction *float_stream_previous; /* 0007e214 */
/* Original [00039170,00039247). Measure stack depth and peak demand for
   one motion context. A non-pushing first float operation marks its context
   as consuming an existing stack value. */
void __fastcall process_float_stream_operation(MachineInstruction *instruction)
{
    unsigned operation = FM_OPERATION(instruction);
    int effect;
    if (operation == MC_CALL) {
        if (float_stream_context->maximum_depth < float_stream_context->stack_depth +
            function_float_stack_depth((FunctionSymbol *)instruction->operands[0].value.symbol))
            float_stream_context->maximum_depth = float_stream_context->stack_depth +
                function_float_stack_depth((FunctionSymbol *)instruction->operands[0].value.symbol);
        float_stream_context->stack_depth -= intrinsic_float_cleanup_count((FunctionSymbol *)instruction->operands[0].value.symbol);
        float_stream_previous = instruction;
        return;
    }
    effect = FM_STACK_EFFECT(operation);
    if (effect < 0 && float_stream_context->maximum_depth < float_stream_context->stack_depth)
        float_stream_context->maximum_depth = float_stream_context->stack_depth;
    if (effect <= 0 && !float_stream_previous) float_stream_context->flags |= 2;
    float_stream_context->stack_depth += effect;
    float_stream_previous = instruction;
}

/* Original [00039000,00039168). Forward branches record label/depth
   requirements. A branch returning to a live context can close and analyze
   that range, then either resume a prior context or start its fallthrough. */
void __fastcall process_float_stream_branch(MachineInstruction *instruction)
{
    MemoryMotionContext *context, *previous;
    FloatStreamBranch *branch;
    MachineInstruction *last;
    context = find_float_stream_label_context((LabelSymbol *)instruction->operands[0].value.symbol);
    if (!context) {
        branch = arena_alloc(4, sizeof(*branch));
        branch->instruction = instruction;
        branch->references = 1;
        branch->label = (LabelSymbol *)instruction->operands[0].value.symbol;
        branch->stack_depth = float_stream_context->stack_depth;
        branch->flags &= ~3U;
        if (!float_stream_context->last_branch) float_stream_context->branches = branch;
        else float_stream_context->last_branch->next = branch;
        float_stream_context->last_branch = branch;
        return;
    }
    --float_stream_order;
    context->last = instruction;
    if (context->flags & 2) {
        start_float_stream_context(instruction->next, 0);
        return;
    }
    if (!match_float_stream_branches(context->branches, instruction, &last)) {
        close_float_stream_context();
        start_float_stream_context(instruction->next, 0);
        return;
    }
    context->last = last ? last : instruction;
    memory_motion_context = context;
    analyze_float_motion_range(context->boundary, context->last, 2);
    previous = context->previous;
    if (previous && previous->label && !(previous->flags & 2)) {
        if (previous->last) {
            float_stream_next = instruction->next;
            merge_float_stream_context();
            start_float_stream_context(float_stream_next, 0);
        } else float_stream_context = previous;
        return;
    }
    float_stream_next = instruction->next;
    if (!(memory_motion_context->flags & 2)) merge_float_stream_context();
    start_float_stream_context(float_stream_next, 0);
}

/* Original [00039f90,00039fc6). Only the current matching label can
   receive a backward branch, and an unsafe later context prevents reuse. */
MemoryMotionContext *__fastcall find_float_stream_label_context(LabelSymbol *label)
{
    MemoryMotionContext *context;
    if (float_stream_context->label != label) return 0;
    for (context = float_stream_context->next; context; context = context->next)
        if (context->flags & 2) return 0;
    return float_stream_context;
}

/* Original [000393d0,000394e7). Referenced labels delimit contexts; a
   single-reference label may continue a range until its matching back edge.
   Multiple incoming references close the preceding unbranched range first. */
void __fastcall process_float_stream_label(MachineInstruction *instruction)
{
    LabelSymbol *label = (LabelSymbol *)instruction->record.symbol;
    MemoryMotionContext *context;
    if (!label->references && label->label_kind != 4) return;
    if (resolve_float_stream_label(label, instruction)) return;
    context = float_stream_context;
    if (label->references > 1) {
        if (context->boundary != instruction && !context->branches && !context->next) {
            context->last = instruction->previous;
            memory_motion_context = context;
            analyze_float_motion_range(context->boundary, context->last, 1);
        }
        close_float_stream_context();
        start_float_stream_context(instruction->next, 0);
        return;
    }
    if (!context->label) {
        if (context->boundary != instruction && !context->branches) {
            context->last = instruction->previous;
            memory_motion_context = context;
            analyze_float_motion_range(context->boundary, context->last, 1);
            goto start;
        }
        context->last = instruction;
    } else if (!context->next) goto start;
    close_float_stream_context();
start:
    float_stream_next = instruction->next;
    start_float_stream_context(instruction, label);
    float_stream_context->order = float_stream_order;
    ++float_stream_order;
}

/* Original [0003a0a0,0003a25c). Resolve incoming branch records against a
   label, remembering their agreed x87 depth. Mismatched depths close unsafe
   contexts; a label forwarded from the following context can resolve at0. */
int __fastcall resolve_float_stream_label(LabelSymbol *label, MachineInstruction *instruction)
{
    FloatStreamBranch *branch, *previous = 0;
    FloatStreamLabel *resolved = 0;
    MemoryMotionContext *context;
    int remaining = label->references;
    for (branch = float_stream_context->branches; branch; branch = branch->next) {
        if (branch->label == label) {
            if (!resolved) {
                resolved = arena_alloc(4, sizeof(*resolved));
                resolved->label = label;
                resolved->next = float_stream_context->labels;
                float_stream_context->labels = resolved;
            } else if (resolved->stack_depth != branch->stack_depth) close_float_stream_context();
            resolved->stack_depth = branch->stack_depth;
            if (float_stream_context->stack_depth != branch->stack_depth &&
                (!FM_ELIGIBLE(instruction->previous) || FM_OPERATION(instruction->previous) != MC_JMP ||
                 float_stream_context->last_branch->instruction != branch->next->instruction))
                close_float_stream_context();
            float_stream_context->stack_depth = branch->stack_depth;
            if (!previous || float_stream_context->branches == branch) {
                float_stream_context->branches = branch->next;
                if (!float_stream_context->branches) float_stream_context->last_branch = 0;
            } else {
                previous->next = branch->next;
                if (!branch->next) float_stream_context->last_branch = previous;
            }
            if (--remaining == 0) return 1;
        }
        previous = branch;
    }
    context = float_stream_context->next;
    if (context && context->label && float_stream_context->stack_depth == 0) {
        for (branch = context->branches; branch; branch = branch->next) {
            if (branch->label == label) {
                if (branch->references != (unsigned)label->references) return 0;
                resolved = arena_alloc(4, sizeof(*resolved));
                resolved->label = label;
                resolved->next = float_stream_context->labels;
                float_stream_context->labels = resolved;
                resolved->stack_depth = 0;
                return 1;
            }
        }
    }
    return 0;
}

/* Original [000395c0,000395f5). Merge each completed safe context before
   closing all entries. Merging may change links, so advance afterward. */
void close_float_stream_context(void)
{
    MemoryMotionContext *context;
    for (context = float_stream_first_context; context; context = context->next) {
        if (!(context->flags & 3) && context->last) {
            memory_motion_context = context;
            merge_float_stream_context();
        }
        context->flags |= 2;
    }
}
MemoryMotionContext *float_stream_active_context; /* 0007e22c */
/* Original [00039b30,00039d7d). Replay the active context range and its
   nested ranges only when some context has floating uses to move. Preserve
   stack state at labels and calls while each child performs its own rewrite. */
void merge_float_stream_context(void)
{
    MemoryMotionContext *context, *probe, *saved;
    MachineInstruction *instruction, *next_boundary;
    FloatStreamLabel *label;
    int nesting = 0;
    for (probe = memory_motion_context; probe; probe = probe->next)
        if (probe->uses) break;
    if (!probe) return;
    context = memory_motion_context;
    context->stack_depth = 0;
    float_stream_active_context = context;
    initialize_float_context_uses(context);
    next_boundary = context->next ? context->next->boundary : 0;
    instruction = memory_motion_context->boundary;
    while (instruction != memory_motion_context->last) {
        if (instruction == next_boundary) {
            if (context->stack_depth) compiler_internal_error("@(#)fltreg.c:1.31", 0x704);
            context = context->next;
            float_stream_active_context = context;
            context->stack_depth = 0;
            initialize_float_context_uses(context);
            next_boundary = context->next ? context->next->boundary : 0;
            ++nesting;
        }
        if (context->last == instruction) {
            finish_float_context_motion(context);
            if (context->rewrite_end) instruction = context->rewrite_end->next;
            context->flags |= 1;
            if (nesting) {
                context = context->previous;
                --nesting;
                float_stream_active_context = context;
            }
        }
        if (FM_ELIGIBLE(instruction)) {
            if (FM_FLOAT(instruction)) rewrite_float_motion_operation(instruction);
            else if (FM_OPERATION(instruction) == MC_CALL && call_preserves_float_stack(instruction)) {
                saved = float_stream_active_context;
                saved->stack_depth -= intrinsic_float_cleanup_count((FunctionSymbol *)instruction->operands[0].value.symbol);
            } else if (FM_OPERATION(instruction) == MC_JCC || FM_OPERATION(instruction) == MC_JMP)
                process_float_motion_branch(instruction, context);
        } else if (FM_KIND(instruction) == 1 && ((LabelSymbol *)instruction->record.symbol)->references &&
            context->boundary != instruction && !label_in_float_context_chain(context->next, instruction)) {
            for (label = context->labels; label; label = label->next)
                if (&label->label->symbol == instruction->record.symbol) break;
            if (!label) compiler_internal_error("@(#)fltreg.c:1.31", 0x745);
            context->stack_depth = label->stack_depth;
        }
        instruction = instruction->next;
    }
    if (FM_ELIGIBLE(instruction) && FM_FLOAT(instruction)) rewrite_float_motion_operation(instruction);
    else if (FM_OPERATION(instruction) == MC_CALL && call_preserves_float_stack(instruction)) {
        saved = float_stream_active_context;
        saved->stack_depth -= intrinsic_float_cleanup_count((FunctionSymbol *)instruction->operands[0].value.symbol);
    }
    finish_float_context_motion(memory_motion_context);
    memory_motion_context->flags |= 1;
}

/* Original [00039600,0003967e). Append a zeroed arena4 context after
   the prior tail and reset the per-context floating instruction cursor. */
void __fastcall start_float_stream_context(MachineInstruction *instruction, LabelSymbol *label)
{
    MemoryMotionContext *context;
    unsigned *word, count;
    if (!float_stream_context) compiler_internal_error("@(#)fltreg.c:1.31", 0x447);
    context = arena_alloc(4, sizeof(*context));
    word = (unsigned *)context;
    count = sizeof(*context) / sizeof(*word);
    while (count--) *word++ = 0;
    float_stream_last_context->next = context;
    context->previous = float_stream_last_context;
    context->label = label;
    context->boundary = instruction;
    float_stream_context = context;
    float_stream_last_context = context;
    context->stack_depth = 0;
    float_stream_context->maximum_depth = 0;
    float_stream_previous = 0;
}

LabelSymbol *float_motion_split_label; /* 00082e78 */
/* Original [00039d80,00039f8d). Require consistent stack depths at outgoing
   branches, split an immediate following label when needed, and remember
   eligible target ranges for the motion pass. Third argument is RET4. */
int __fastcall match_float_stream_branches(FloatStreamBranch *branches, MachineInstruction *instruction, MachineInstruction **last)
{
    FloatStreamBranch *branch;
    FloatStreamTarget *entry;
    MachineInstruction *probe, *target, *previous, *created;
    LabelSymbol *following_label = 0, *split_label = 0;
    int found_label = 0;
    *last = 0;
    if (!branches) return 1;
    for (probe = instruction->next; probe; probe = probe->next) {
        if (FM_ELIGIBLE(probe)) break;
        if (FM_KIND(probe) == 1) {
            following_label = (LabelSymbol *)probe->record.symbol;
            found_label = 1;
            break;
        }
    }
    if (found_label) {
        split_label = new_internal_label();
        float_motion_split_label = split_label;
        created = new_machine_instruction(1);
        split_label->references = 0;
        created->record.symbol = &split_label->symbol;
        probe = instruction->next;
        while (FM_KIND(probe) == 4) probe = probe->next;
        if (FM_KIND(probe) == 7) probe = probe->next;
        insert_machine_after(probe->previous, created);
        *last = created;
    }
    for (branch = branches; branch; branch = branch->next) {
        if (float_stream_context->stack_depth != branch->stack_depth) return 0;
        if (branch->label == following_label) {
            --following_label->references;
            ++split_label->references;
            branch->instruction->operands[0].value.symbol = &split_label->symbol;
            branch->label = split_label;
        }
    }
    mark_float_branch_targets(branches);
    for (branch = branches; branch; branch = branch->next) {
        if (branch->label == split_label) continue;
        target = label_machine_instruction(branch->label);
        if (!target) {
            compiler_internal_error("@(#)fltreg.c:1.31", 0xec7);
            return 0;
        }
        previous = target->previous;
        while (previous && FM_KIND(previous) != 1 && FM_KIND(previous) != 0) previous = previous->previous;
        if (!can_merge_float_branch(branch, previous, target)) {
            if (branch->label->references != (long)branch->references) return 0;
            if (!(branch->flags & 1) && (!previous || FM_KIND(previous) != 0 || FM_OPERATION(previous) != MC_JMP)) return 0;
            if (!can_reuse_float_branch_target(target)) return 0;
            entry = arena_alloc(4, sizeof(*entry));
            entry->instruction = target->next;
            entry->next = float_stream_context->targets;
            float_stream_context->targets = entry;
        }
    }
    return 1;
}
int float_motion_mode; /* 0007e234 */
/* Original82e68 is an encountered reset word immediately before the use
   list. No direct consumer is known; retain the original store for now. */
unsigned float_motion_reset_word;
MachineInstruction *float_motion_previous; /* 0007e27c */
#define FM_MOTION_BARRIER(i) ((FM_ELIGIBLE(i) && (FM_OPERATION(i) == MC_JCC || \
    FM_OPERATION(i) == MC_JMP || FM_OPERATION(i) == MC_CALL || \
    FM_OPERATION(i) == MC_LEAVE || FM_OPERATION(i) == MC_RET)) || \
    (FM_KIND(i) == 1 && (FM_LABEL_BARRIER(i) || backend_stage == 5)))
/* Original [00039680,00039b2d). Trim non-loop ranges to their useful
   floating endpoints, validate stack capacity and collect uses through any
   nested context. Mode2 keeps its loop boundary instead of trimming it. */
void __fastcall analyze_float_motion_range(MachineInstruction *first, MachineInstruction *last, int mode)
{
    MachineInstruction *last_float = 0;
    int nested = 0;
    if (!memory_motion_context) compiler_internal_error("@(#)fltreg.c:1.31", 0x49d);
    initialize_float_motion_uses();
    if (first == last || (memory_motion_context->flags & 3) ||
        (memory_motion_context->branches && mode != 2) || memory_motion_context->maximum_depth >= 8) goto invalid;
    float_motion_mode = mode;
    float_motion_reset_word = 0;
    float_motion_previous = 0;
    if (memory_motion_context->label && mode != 2) {
        first = first->next;
        memory_motion_context->boundary = first;
    }
    if (mode != 2) {
        while (first) {
            if ((FM_ELIGIBLE(first) && FM_FLOAT(first)) || first == last || FM_MOTION_BARRIER(first)) break;
            first = first->next;
        }
        if (!first || first == last || FM_MOTION_BARRIER(first)) goto invalid;
        memory_motion_context->boundary = first;
        while (last) {
            if ((FM_ELIGIBLE(last) && FM_FLOAT(last)) || last == first || FM_MOTION_BARRIER(last)) break;
            last = last->previous;
        }
        if (!last || last == first || (FM_MOTION_BARRIER(last) && memory_motion_context->last != last)) goto invalid;
        memory_motion_context->last = last;
    }
    if ((!memory_motion_context->maximum_depth &&
         (!memory_motion_context->next || !memory_motion_context->next->uses)) ||
        (memory_motion_context->stack_depth != 0 && memory_motion_context->stack_depth != 1)) goto invalid;
    while (first != last->next) {
        if (!first) compiler_internal_error("@(#)fltreg.c:1.31", 0x4f9);
        else if (memory_motion_context->next && memory_motion_context->next->boundary == first) nested = 1;
        if (nested && memory_motion_context->next->last == first) nested = 0;
        if (FM_ELIGIBLE(first)) {
            if (FM_FLOAT(first)) {
                record_float_operation_uses(first, nested);
                last_float = first;
            } else record_float_nonfloat_uses(first);
            if (FM_OPERATION(first) == MC_JMP || (FM_OPERATION(first) == MC_JCC && first != last))
                record_float_motion_branch(first);
        } else if (FM_KIND(first) == 1) resolve_float_motion_branch(first);
        first = first->next;
    }
    if (last_float) {
        finish_float_motion_uses();
        release_float_motion_scratch();
        float_motion_split_label = 0;
    } else {
        close_float_stream_context();
        release_float_motion_scratch();
    }
    return;
invalid:
    close_float_stream_context();
    release_float_motion_scratch();
    memory_motion_context->flags |= 2;
}

FloatStreamUse *float_motion_buckets[8]; /* 0007e238..0007e258 */
int float_motion_writes_unknown_memory; /* 0007e268 */
int float_motion_reads_unknown_memory; /* 0007e26c */
FloatMotionBranch *float_motion_pending_branches; /* 00082e7c */
/* Original [0003a740,0003a7a7). Clear this range's use hash and branch
   worklist, reserving ordinary physical registers while EBP remains usable
   as a canonical stack-base value. */
void initialize_float_motion_uses(void)
{
    int index;
    float_motion_writes_unknown_memory = 0;
    float_motion_reads_unknown_memory = 0;
    float_motion_pending_branches = 0;
    for (index = 0; index < 8; ++index) float_motion_buckets[index] = 0;
    for (index = 0; index < float_stream_register_count; ++index)
        float_stream_register_uses[index] = index < 32 && index != REG_EBP ? FLOAT_STREAM_FIXED_REGISTER : 0;
}

/* Original [0003a2a0,0003a2f5). A defined label resolves every pending
   outgoing edge for that label. Nodes remain arena-owned after unlinking. */
void __fastcall resolve_float_motion_branch(MachineInstruction *instruction)
{
    FloatMotionBranch *branch = float_motion_pending_branches, *previous = 0;
    while (branch) {
        if (&branch->label->symbol == instruction->record.symbol) {
            if (!previous || float_motion_pending_branches == branch) {
                float_motion_pending_branches = float_motion_pending_branches->next;
                previous = branch;
            } else previous->next = branch->next;
        } else previous = branch;
        branch = branch->next;
    }
}

/* Original [0003a4b0,0003a73d). Collect weighted floating memory/virtual
   uses. Adjacent load/store pairs carry their original CPU cost penalty;
   nested contexts mark an operand unavailable and stop collection early. */
void __fastcall record_float_operation_uses(MachineInstruction *instruction, int nested)
{
    MachineOperand *operand;
    MachineInstruction *probe;
    FloatStreamUse *use;
    unsigned operation = FM_OPERATION(instruction);
    int adjacent_store;
    if (machine_operand_counts[operation] == 0) return;
    operand = &instruction->operands[0];
    if ((operand->flags & 15) != OPERAND_REGISTER || (operand->value.reg.words.number & 0x3fff) > 21) {
        use = find_float_stream_use(operand, 1, (instruction->opcode >> 4) & 15, operation);
        if (nested) {
            use->flags |= 2;
            return;
        }
        use->flags = (use->flags & ~0x40U) | ((instruction->record.float_flags >> 3) & 0x40);
        if (operation == MC_FST || operation == MC_FSTP) {
            use->flags |= 0x20;
            update_float_use_access(use, 1);
            if (FM_OPERATION(instruction) == MC_FSTP && float_motion_previous && FM_OPERATION(float_motion_previous) == MC_FLD) {
                use->cost_adjustment -= cpu_model < 4 ? 43 : 9;
                goto second_operand;
            }
            ++use->stores;
        }
        use->weighted_uses += float_motion_mode;
    }
second_operand:
    operation = FM_OPERATION(instruction);
    operand = &instruction->operands[1];
    if (machine_operand_counts[operation] == 2 &&
        ((operand->flags & 15) != OPERAND_REGISTER || (operand->value.reg.words.number & 0x3fff) > 21)) {
        use = find_float_stream_use(operand, 1, (instruction->opcode >> 4) & 15, operation);
        if (nested) {
            use->flags |= 2;
            return;
        }
        use->flags = (use->flags & ~0x40U) | ((instruction->record.float_flags >> 3) & 0x40);
        if (FM_OPERATION(instruction) == MC_FLD) {
            adjacent_store = 0;
            for (probe = instruction->next; probe; probe = probe->next) {
                if (FM_MOTION_BARRIER(probe)) break;
                if (FM_ELIGIBLE(probe) && FM_FLOAT(probe)) {
                    adjacent_store = FM_OPERATION(probe) == MC_FSTP;
                    break;
                }
            }
            if (adjacent_store) use->cost_adjustment -= cpu_model < 4 ? 43 : 9;
            else {
                ++use->loads;
                use->weighted_uses += float_motion_mode;
            }
        } else {
            use->weighted_uses += float_motion_mode;
            if (FM_OPERATION(instruction) == MC_FMUL && cpu_model == 4) ++use->multiplies;
            else ++use->arithmetic;
        }
        update_float_use_access(use, 2);
    }
    float_motion_previous = instruction;
}

/* Original [0003a8f0,0003aac2). Intern a typed use record by canonical
   operand equality. The record owns copies of its operand and address so
   later instruction rewrites cannot change the lookup key. */
FloatStreamUse *__fastcall find_float_stream_use(MachineOperand *operand, int create, unsigned width, unsigned operation)
{
    unsigned bucket = float_operand_hash(operand), kind = operand->flags & 15;
    FloatStreamUse *use;
    unsigned *word, count;
    for (use = float_motion_buckets[bucket]; use; use = use->hash_next) {
        if (operands_equal(operand, use->operand)) {
            if (!(use->flags & 4) && ((operation >= MC_FADD && operation <= MC_FLD1) ||
                (operation >= MC_FUCOM && operation <= MC_FCOS))) use->flags |= 4;
            if (kind == OPERAND_REGISTER) use->flags = (use->flags & ~1U) | ((operand->flags >> 10) & 1);
            return use;
        }
    }
    if (create != 1) return use;
    use = arena_alloc(4, sizeof(*use));
    word = (unsigned *)use;
    count = sizeof(*use) / sizeof(*word);
    while (count--) *word++ = 0;
    if ((operation >= MC_FADD && operation <= MC_FLD1) || (operation >= MC_FUCOM && operation <= MC_FCOS)) use->flags |= 4;
    use->operand = arena_alloc(4, sizeof(*use->operand));
    *use->operand = *operand;
    if (kind != OPERAND_REGISTER) {
        use->operand->value.address = arena_alloc(4, sizeof(MachineAddress));
        *use->operand->value.address = *operand->value.address;
    }
    if (kind == OPERAND_REGISTER) {
        use->operand->flags &= ~0x400U;
        use->flags = (use->flags & ~1U) | ((operand->flags >> 10) & 1);
    } else use->flags &= ~1U;
    use->stack_slot = -1;
    use->base = use->index = 0xffff;
    use->width = width;
    use->operation = operation;
    if (kind == OPERAND_BASED || kind == OPERAND_INDEXED) {
        if (kind == OPERAND_INDEXED) use->index = operand->value.address->index.words.number & 0x3fff;
        if (operand->value.address->base.words.number != 27) use->base = operand->value.address->base.words.number & 0x3fff;
    }
    use->hash_next = float_motion_buckets[bucket];
    float_motion_buckets[bucket] = use;
    return use;
}

/* Original [0003a7b0,0003a8cd). Hash the original operand identity fields.
   The indexed form deliberately adds the base twice (unless absent) and
   does not hash its index; equality still distinguishes colliding operands. */
unsigned __fastcall float_operand_hash(MachineOperand *operand)
{
    MachineAddress *address;
    SymbolHeader *symbol;
    unsigned hash = 0, kind = operand->flags & 15, base;
    switch (kind) {
    case OPERAND_REGISTER:
        hash = operand->value.reg.words.number & 0x3fff;
        break;
    case OPERAND_ABSOLUTE:
        address = operand->value.address;
        hash = float_symbol_name_hash(symbol_display_name(address->symbol)) + (unsigned)address->first.displacement;
        break;
    case OPERAND_BASED: case OPERAND_INDEXED:
        address = operand->value.address;
        symbol = address->symbol;
        if (symbol) {
            if (symbol->kind == 19) hash = symbol->field04;
            else hash = float_symbol_name_hash(symbol_display_name(symbol));
        }
        base = address->base.words.number;
        if (kind == OPERAND_INDEXED) {
            if (base != 27) hash += base;
            hash += (operand->flags & 0x300) >> 8;
        }
        hash += base + (unsigned)address->first.displacement;
        break;
    case OPERAND_SYMBOL:
        hash = float_symbol_name_hash(symbol_display_name(operand->value.symbol));
        break;
    default:
        compiler_internal_error("@(#)fltreg.c:1.31", 0x564);
        break;
    }
    /* Unsigned subtraction preserves the original32-bit abs wrap at
       INT_MIN, whose signed remainder modulo8 is also zero. */
    if ((int)hash < 0) hash = 0U - hash;
    return hash & 7;
}

/* Original [0003aad0,0003ab76). Record first read/write access and its
   pending branch. Conflicting accesses across unmatched edges exclude the
   operand; absolute external storage does not acquire a branch dependency. */
void __fastcall update_float_use_access(FloatStreamUse *use, unsigned access_kind)
{
    unsigned prior = (use->flags >> 3) & 3;
    SymbolHeader *symbol;
    if (!prior) {
        use->flags = (use->flags & ~0x18U) | ((access_kind << 3) & 0x18);
        if ((use->operand->flags & 15) == OPERAND_ABSOLUTE) {
            symbol = use->operand->value.address->symbol;
            if (symbol && (((BackendSymbol *)symbol)->attributes & 0x70) == 0x40) return;
        }
        if (float_motion_pending_branches) use->branch = float_motion_pending_branches->instruction;
    } else if (use->branch) {
        if (prior != access_kind) {
            if (!float_motion_pending_branches || float_motion_pending_branches->instruction != use->branch) use->flags |= 2;
            else use->branch = 0;
        } else if (!float_motion_pending_branches) use->branch = 0;
    }
}

/* Original [0003a8d0,0003a8e2). Signed-byte symbol hash local to the
   floating-use module; retain this individual helper and its fastcall ABI. */
unsigned __fastcall float_symbol_name_hash(char *name)
{
    unsigned hash = 0;
    while (*name) hash += (unsigned)(int)(signed char)*name++;
    return hash;
}

/* Original [0003a350,0003a48a), with operand-kind switch3a48c..3a4b0.
   Integer definitions reserve their register-map cells. Memory accesses
   exclude matching floating uses and track non-stack aliasing directions. */
void __fastcall record_float_nonfloat_uses(MachineInstruction *instruction)
{
    MachineOperand *operand;
    FloatStreamUse *use;
    unsigned operation = FM_OPERATION(instruction), number;
    int index;
    if (operation == MC_JCC || operation == MC_JMP || operation == MC_CALL) return;
    for (index = 0; index < machine_operand_counts[FM_OPERATION(instruction)]; ++index) {
        operand = &instruction->operands[index ? 1 : 0];
        switch (operand->flags & 15) {
        case OPERAND_REGISTER:
            if (index == 0) {
                number = operand->value.reg.words.number & 0x3fff;
                if (number >= (unsigned short)float_stream_register_count)
                    compiler_internal_error("@(#)fltreg.c:1.31", 0xb61);
                float_stream_register_uses[number] = FLOAT_STREAM_FIXED_REGISTER;
            }
            break;
        case OPERAND_IMMEDIATE: case OPERAND_LABEL_ADDRESS:
        case OPERAND_SYMBOL: case OPERAND_FRAME_SIZE:
            break;
        case OPERAND_INDEXED:
            if (FM_OPERATION(instruction) == MC_LEA) break;
            if (!optimize_alias) {
                if (index == 0) float_motion_writes_unknown_memory = 1;
                else float_motion_reads_unknown_memory = 1;
            }
            /* fall through: indexed addresses also have the base test */
        case OPERAND_BASED:
            if (FM_OPERATION(instruction) == MC_LEA) break;
            if (operand->value.address->base.words.number != REG_EBP && operand->value.address->base.words.number != 27 && !optimize_alias) {
                if (index == 0) float_motion_writes_unknown_memory = 1;
                else float_motion_reads_unknown_memory = 1;
            }
            /* fall through: memory references enter the same operand hash */
        default:
            use = find_float_stream_use(operand, 1, (instruction->opcode >> 4) & 15, FM_OPERATION(instruction));
            use->flags |= 2;
            break;
        }
    }
}
