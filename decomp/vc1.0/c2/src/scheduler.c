/* Machine scheduling windows, x87 value state and nested-region tracking. */
#include "scheduler.h"
#include "machine-layout.h"
#include "instruction-selection.h"
#include "instruction-rewrite.h"
#include <stdlib.h>
#include <recon.h>
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

/* Window state:7c304 is an undo-record pointer (restore traversal404c0).
   Both pair words are instruction pointers (432e0/437e0). */
SchedulerUndo *scheduler_undo; /*7c304*/
MachineInstruction *scheduler_cursor; /*7c2fc*/
MachineInstruction *scheduler_last_float; /*7c34c*/
signed char scheduler_float_delay; /*82fcc: native BYTE writes*/
int scheduler_stop_window; /*82ff4*/
int scheduler_ready_count; /*82ff8*/
SchedulerPairs scheduler_pairs; /*7c2ac..7c2d4*/

/* Legacy non-Pentium x87 delay lookup, expressed by opcode semantics.
   Original83000..83050 covers FADD..FLD1. Later FUCOM..FCOS indices read
  8308c..83092, the tail of "fp concurrency" and start of "@(#)sched.c";
   preserve those fixed-image values without copying neighboring data. */
#define SET_SCHEDULER_FLOAT_DELAY(destination, operation) do { \
    switch (operation) { \
    case MC_FICOM: case MC_FICOMP: case MC_FCOM: case MC_FCOMP: case MC_FCOMPP: \
        (destination) = 1; break; \
    case MC_F2XM1: case MC_FPREM: case MC_FSCALE: \
        (destination) = 2; break; \
    case MC_FILD: case MC_FPATAN: (destination) = 5; break; \
    case MC_FADD: case MC_FSUB: case MC_FSUBR: case MC_FADDP: case MC_FSUBP: case MC_FSUBRP: \
    case MC_FIADD: case MC_FISUB: case MC_FISUBR: case MC_FBLD: case MC_FRNDINT: \
        (destination) = 7; break; \
    case MC_FIMUL: (destination) = 8; break; \
    case MC_FMUL: case MC_FMULP: (destination) = 11; break; \
    case MC_FYL2X: case MC_FYL2XP1: (destination) = 13; break; \
    case MC_FDIV: case MC_FDIVR: case MC_FDIVP: case MC_FDIVRP: \
    case MC_FIDIV: case MC_FIDIVR: case MC_FPTAN: (destination) = 70; break; \
    case MC_FUCOM: (destination) = 99; break; \
    case MC_FUCOMP: (destination) = 121; break; \
    case MC_FSINCOS: (destination) = 64; break; \
    case MC_FSIN: (destination) = 40; break; \
    case MC_FCOS: (destination) = 35; break; \
    default: (destination) = 0; break; \
    } \
} while (0)

/* Original [0003f840,0003fa40). Set up one inclusive scheduling window,
   retain processor-specific entry rules and place the resulting end barrier. */
void __fastcall schedule_window(MachineInstruction *first, MachineInstruction *last)
{
    MachineInstruction *end = last->next, *chosen, *tail;
    unsigned kind, operation, i;
    scheduler_instruction = 0;
    scheduler_last_float = 0;
    scheduler_undo = 0;
    scheduler_float_delay = 0;
    scheduler_stop_window = 0;
    if (cpu_model == 5) {
        scheduler_ready_count = 0;
        for (i = 0; i < 2; ++i) {
            scheduler_pairs.previous[i].first = 0;
            scheduler_pairs.previous[i].second = 0;
        }
        scheduler_pairs.candidates[0].first = 0;
        scheduler_pairs.candidates[0].second = 0;
        for (i = 0; i < 8; ++i) scheduler_float_values[i] = -1;
    }
    if (first == last || !prepare_schedule_window(first, last)) return;
    while (first != end && !SCHEDULABLE_KIND(first->opcode & 15)) first = first->next;
    scheduler_cursor = first->previous;
    if (cpu_model != 5) {
        chosen = first;
        if (scheduler_after_label && cpu_model == 3) {
            first = schedule_generic_instruction(first);
            chosen = first->previous;
            scheduler_after_label = 0;
        }
        if (first != end) first = first->next;
        operation = (chosen->opcode >> 8) & 255;
        if (SCHEDULER_FLOAT_OPERATION(operation)) {
            scheduler_last_float = chosen;
            SET_SCHEDULER_FLOAT_DELAY(scheduler_float_delay, operation);
        }
        scheduler_instruction = chosen;
        if (instruction_accesses_memory(chosen)) ++scheduler_memory_run;
        else scheduler_memory_run = 0;
    }
    while (first != end && !scheduler_stop_window) {
        kind = first->opcode & 15;
        if (SCHEDULABLE_KIND(kind)) {
            if (cpu_model == 5) first = schedule_pentium_instruction(first);
            else first = schedule_generic_instruction(first);
        } else {
            scheduler_cursor = first;
            first = first->next;
        }
    }
    for (tail = end->previous; tail && !SCHEDULABLE_KIND(tail->opcode & 15); tail = tail->previous) ;
    if (tail) tail->opcode |= 0x800000UL;
}

unsigned scheduler_float_position_capacity; /*7c344: serial-indexed position slots*/

/* Original [0003fd30,0003fe70). Prepare x87 value-position storage and
   annotate flag definitions/consumers while counting scheduling operations. */
int __fastcall prepare_schedule_window(MachineInstruction *first, MachineInstruction *last)
{
    MachineInstruction *scan, *before;
    unsigned kind, operation, properties, i;
    int flags_live, count = 0;
    if (scheduler_float_depth) scheduler_float_invalid = 1;
    if (scheduler_has_float && cpu_model == 5 && !scheduler_float_invalid) {
        scheduler_float_position_capacity = (unsigned)scheduler_float_serial + 1;
        if ((scheduler_float_position_capacity & 0x3fffffffUL) < 250)
            scheduler_float_positions = arena_alloc(3, scheduler_float_position_capacity * sizeof(int));
        else {
            scheduler_float_positions = scheduler_allocate(scheduler_float_position_capacity * sizeof(int));
            scheduler_float_positions_owned = 1;
        }
        if (scheduler_float_serial >= 0)
            for (i = 0; i < scheduler_float_position_capacity; ++i) scheduler_float_positions[i] = -1;
    }
    flags_live = find_flags_use(last) != 0;
    before = first->previous;
    for (scan = last; scan != before; scan = scan->previous) {
        kind = scan->opcode & 15;
        if (!SCHEDULABLE_KIND(kind)) continue;
        ++count;
        operation = (scan->opcode >> 8) & 255;
        properties = machine_selection_properties[operation];
        if (((properties & MC_WRITES_FLAGS) || operation == MC_CALL) && flags_live) {
            flags_live = 0;
            scan->record.scheduling->flags |= 4;
        }
        if (properties & MC_READS_FLAGS) {
            flags_live = 1;
            scan->record.scheduling->flags |= 2;
        }
    }
    return count;
}

/* Original [00042960,00042a0b). Metadata bit11 bypasses pair selection;
   otherwise clear the two extra candidates and select one scheduling unit. */
MachineInstruction *__fastcall schedule_pentium_instruction(MachineInstruction *instruction)
{
    MachineInstruction *chosen, *next;
    unsigned operation;
    if (instruction->record.scheduling->flags & 0x800) {
        scheduler_instruction = instruction;
        scheduler_cursor = instruction;
        if (scheduler_track_registers) update_scheduled_registers(instruction);
        return instruction->next;
    }
    scheduler_undo = 0;
    scheduler_pairs.candidates[1].first = 0;
    scheduler_pairs.candidates[1].second = 0;
    scheduler_pairs.candidates[2].first = 0;
    scheduler_pairs.candidates[2].second = 0;
    chosen = choose_pentium_instruction(instruction);
    next = chosen->next;
    operation = (chosen->opcode >> 8) & 255;
    if (operation == MC_REP || operation == MC_REPZ || operation == MC_REPNZ) {
        next = chosen->next->next;
        chosen = next->previous;
    }
    scheduler_instruction = chosen;
    scheduler_cursor = chosen;
    return next;
}
