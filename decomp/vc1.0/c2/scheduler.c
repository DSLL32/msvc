/* Machine scheduling windows, x87 value state and nested-region tracking. */
#include "scheduler.h"
#include "machine-layout.h"
#include "instruction-selection.h"
#include "instruction-rewrite.h"
#include <stdlib.h>
extern int cpu_model;
SchedulerRegionFrame *scheduler_regions; /*82ff0*/
MachineInstruction *scheduler_instruction; /*7c300*/
unsigned short scheduler_register_capacity; /*7e290*/
SchedulerRegisterState **scheduler_register_states; /*7c348*/
int scheduler_track_registers; /*7c308*/
int scheduler_after_label; /*82fc8*/
int scheduler_memory_run; /*82fd0*/
int scheduler_region_balance; /*82ffc*/
int scheduler_float_invalid; /*7c30c*/
int scheduler_float_depth; /*7c310*/
int scheduler_has_float; /*7c314*/
/* Native7c2e0 + architectural register14..21 indexes this same array.
   Future consumers must translate that base/index pair to reg-REG_ST0. */
int scheduler_float_values[8]; /*7c318..7c338*/
int scheduler_float_serial; /*7c338*/
int *scheduler_float_positions; /*7c33c: serial-indexed x87 value locations*/
int scheduler_float_positions_owned; /*7c340: malloc rather than arena3*/
#define SCHEDULABLE_KIND(k) ((k) == 0 || (k) == 8 || (k) == 9 || (k) == 10)
#define SCHEDULER_FLOAT_OPERATION(op) (((op) >= MC_FADD && (op) <= MC_FLD1) || \
    ((op) >= MC_FUCOM && (op) <= MC_FCOS))
/* These resets are inline in both original locations; no new call edge. */
#define RESET_FLOAT_SCHEDULE() do { \
    unsigned reg; \
    scheduler_has_float = 0; scheduler_float_serial = 1; \
    scheduler_float_positions_owned = 0; scheduler_float_depth = 0; \
    scheduler_float_invalid = 0; \
    for (reg = 0; reg < 8; ++reg) scheduler_float_values[reg] = -1; \
} while (0)

/* Original [0003f240,0003f830). Divide the stream into scheduling windows,
   preserving protected/float blocks, prologue barriers and region nesting. */
void __fastcall schedule_machine_instructions(int track_registers)
{
    MachineInstruction *scan = machine_instructions, *first, *boundary, *saved_next;
    MachineInstruction *before, *last, *mark, *target;
    SchedulerRegionFrame *frame;
    unsigned kind, operation, i;
    int prologue = 1, ordinal = 0, boundary_record;
    int float_block = 0, seen_float_block = 0, protected_block = 0, seen_protected_block = 0;
    int marked_tail;
    if (!scan) return;
    if (scheduler_regions) compiler_internal_error("@(#)sched.c:1.33", 0x131);
    scheduler_regions = 0;
    scheduler_region_balance = 0;
    scheduler_float_invalid = 0;
    scheduler_memory_run = 0;
    for (; scan; scan = scan->next) {
        kind = scan->opcode & 15;
        if (kind == 11) {
            if (scan->record.directive.code == 15) prologue = 0;
        } else if (kind == 7) {
            protected_block = (((MachineBlockHeader *)scan)->flags & 2) != 0;
            if (protected_block) seen_protected_block = 1;
        } else if (SCHEDULABLE_KIND(kind)) {
            if (!prologue) break;
            scan->opcode |= 0x800000UL;
            if (instruction_accesses_memory(scan)) ++scheduler_memory_run;
            else scheduler_memory_run = 0;
        }
    }
    if (!scan) return;
    scheduler_instruction = scan;
    scheduler_register_capacity = (unsigned short)(next_machine_register + 1);
    scheduler_track_registers = track_registers;
    if (track_registers) {
        scheduler_register_states = scheduler_allocate(scheduler_register_capacity * sizeof(*scheduler_register_states));
        for (i = 0; i < scheduler_register_capacity; ++i) scheduler_register_states[i] = 0;
    }
    if (cpu_model == 5) RESET_FLOAT_SCHEDULE();
    first = scan;
    scheduler_after_label = 0;
    while (scan) {
        kind = scan->opcode & 15;
        operation = (scan->opcode >> 8) & 255;
        if (kind == 7) {
            float_block = (((MachineBlockHeader *)scan)->flags & 1) != 0;
            if (float_block) seen_float_block = 1;
            protected_block = (((MachineBlockHeader *)scan)->flags & 2) != 0;
            if (protected_block) seen_protected_block = 1;
        }
        boundary_record = (SCHEDULABLE_KIND(kind) &&
            (operation == MC_JCC || operation == MC_JMP || operation == MC_LEAVE || operation == MC_RET)) || kind == 1;
        if (!boundary_record && ordinal >= 60 && SCHEDULABLE_KIND(kind) &&
            operation != MC_REP && operation != MC_REPZ && operation != MC_REPNZ &&
            !scheduler_float_depth && !SCHEDULER_FLOAT_OPERATION(operation) && !find_flags_use(scan))
            boundary_record = 1;
        last = scan;
        if (boundary_record) {
            boundary = scan;
            if (SCHEDULABLE_KIND(kind)) {
                boundary->opcode |= 0x800000UL;
                prepare_scheduled_instruction(boundary, ordinal);
            }
            ordinal = 0;
            before = first->previous;
            saved_next = boundary->next;
            if (!seen_float_block && !seen_protected_block) {
                schedule_window(first, boundary);
                scheduler_memory_run = 0;
            }
            if (!float_block) seen_float_block = 0;
            if (!protected_block) seen_protected_block = 0;
            last = saved_next->previous;
            if (cpu_model == 5) {
                kind = last->opcode & 15;
                marked_tail = SCHEDULABLE_KIND(kind) && (last->opcode & 0x800000UL);
                last = recombine_scheduled_loads(before->next, last);
                if (marked_tail && boundary != last) {
                    mark = last;
                    if (!SCHEDULABLE_KIND(last->opcode & 15)) mark = previous_machine_operation(last);
                    mark->opcode |= 0x800000UL;
                }
                RESET_FLOAT_SCHEDULE();
                ordinal = 0;
                kind = boundary->opcode & 15;
                operation = (boundary->opcode >> 8) & 255;
                if (SCHEDULABLE_KIND(kind) && (operation == MC_JCC || operation == MC_JMP) &&
                    (boundary->operands[0].flags & 15) == OPERAND_SYMBOL &&
                    boundary->operands[0].value.symbol->kind == 3) {
                    target = label_machine_instruction((LabelSymbol *)boundary->operands[0].value.symbol);
                    if (target) scheduler_region_balance -= (target->labels.flags & 0xfffcU) >> 2;
                    else if (scheduler_region_balance)
                        compiler_internal_error("@(#)sched.c:1.33", 0x1c7);
                } else if (kind == 1) scheduler_region_balance += (boundary->labels.flags & 0xfffcU) >> 2;
            }
            if (scheduler_track_registers)
                for (i = 0; i < scheduler_register_capacity; ++i) scheduler_register_states[i] = 0;
            reset_arena(3);
            if (scheduler_float_positions_owned) free(scheduler_float_positions);
            first = last->next;
            scheduler_after_label = (last->opcode & 15) == 1;
            if (scheduler_after_label && last->labels.attachment) {
                frame = arena_alloc(4, sizeof(SchedulerRegionFrame));
                frame->header = last;
                frame->end = last->labels.attachment->last;
                frame->previous = scheduler_regions;
                scheduler_regions = frame;
            } else if (scheduler_regions && scheduler_regions->end == last)
                scheduler_regions = scheduler_regions->previous;
        } else if (SCHEDULABLE_KIND(kind)) {
            scan->opcode &= ~0x800000UL;
            prepare_scheduled_instruction(scan, ordinal);
            ++ordinal;
            if (scheduler_regions && scheduler_regions->end == scan)
                scheduler_regions = scheduler_regions->previous;
        }
        scan = last->next;
    }
    if (scheduler_track_registers) scheduler_release(scheduler_register_states);
    reset_arena(4);
}

/* Original [0003fb40,0003fbf0). Scheduler memory-run classification:
   LEA is address-only; explicit memory, stack and selected string operations
   participate. This is the native scheduling cost predicate, not an ISA-wide
   enumeration of every implicit memory effect (CALL/RET are separate barriers). */
int __fastcall instruction_accesses_memory(MachineInstruction *instruction)
{
    unsigned operation = (instruction->opcode >> 8) & 255, kind;
    int count;
    if (operation == MC_LEA) return 0;
    switch (operation) {
    case MC_MOVS: case MC_LODS: case MC_STOS:
    case MC_PUSH: case MC_EPUSH: case MC_POP: case MC_EPOP: return 1;
    }
    count = machine_operand_counts[operation];
    if (!count) return 0;
    kind = instruction->operands[0].flags & 15;
    if (kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE) return 1;
    if (count != 2) return 0;
    kind = instruction->operands[1].flags & 15;
    return kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE;
}

/* Original [0003fb10,0003fb40). Scheduler-owned arrays use CRT malloc;
   allocation failure is an internal scheduling error. */
void *__fastcall scheduler_allocate(unsigned bytes)
{
    void *memory = malloc(bytes);
    if (!memory) compiler_internal_error("@(#)sched.c:1.33", 0x5ba);
    return memory;
}

/* Original [0003fa40,0003fb10). Attach per-window scheduling information;
   preserve the native reverse-ordinal priority and Pentium x87 preference. */
void __fastcall prepare_scheduled_instruction(MachineInstruction *instruction, int ordinal)
{
    InstructionSchedule *info = new_instruction_schedule();
    unsigned operation;
    instruction->record.scheduling = info;
    info->ordinal = ordinal;
    info->priority = -ordinal;
    if (cpu_model != 5) return;
    operation = (instruction->opcode >> 8) & 255;
    if (SCHEDULER_FLOAT_OPERATION(operation)) {
        scheduler_has_float = 1;
        if (!scheduler_float_invalid) prepare_float_schedule(instruction);
        if (operation == MC_FLD || operation == MC_FILD || operation == MC_FLDT || operation == MC_FLD1)
            info->priority -= 100;
        info->priority += 1000;
    } else if (operation == MC_CALL) {
        if (call_float_result_count(instruction) > 1) scheduler_float_invalid = 1;
        if ((instruction->operands[0].flags & 15) == OPERAND_SYMBOL &&
            (((FunctionSymbol *)instruction->operands[0].value.symbol)->field24 & 15) == 5 &&
            !scheduler_float_invalid)
            prepare_float_schedule(instruction);
    }
}

/* Original [00041dc0,00041df0), including RET at41de0. Define the
  44-byte record's zero state explicitly, without adding a memset call edge. */
InstructionSchedule *__cdecl new_instruction_schedule(void)
{
    InstructionSchedule *info = arena_alloc(3, sizeof(InstructionSchedule));
    info->field00 = info->field04 = info->field1c = 0;
    info->float_inputs[0] = info->float_inputs[1] = 0;
    info->float_result = info->float_arity = info->float_depth = 0;
    info->ordinal = info->priority = 0;
    info->flags = 0;
    return info;
}
