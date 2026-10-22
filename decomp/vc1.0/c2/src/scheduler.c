/* Machine scheduling windows, x87 value state and nested-region tracking. */
#include "scheduler.h"
#include "machine-layout.h"
#include "instruction-selection.h"
#include "instruction-rewrite.h"
#include "register-use.h"
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
/* Inline native barrier predicate, shared by scheduling scans. */
#define SCHEDULER_BARRIER(i) (((i)->opcode & 15) == 1 || \
    (SCHEDULABLE_KIND((i)->opcode & 15) && \
     ((((i)->opcode >> 8) & 255) == MC_JCC || (((i)->opcode >> 8) & 255) == MC_JMP || \
      ((i)->opcode & 0x800000UL) || (((i)->opcode >> 8) & 255) == MC_LEAVE || \
      (((i)->opcode >> 8) & 255) == MC_RET)))
#define SCHEDULER_STACK_TRANSFER(op) ((op) == MC_POP || (op) == MC_EPOP || (op) == MC_PUSH || (op) == MC_EPUSH)
#define SCHEDULER_CALL_SERIALIZED(op) ((op) == MC_MOVS || (op) == MC_STOS || \
    (op) == MC_REP || (op) == MC_REPZ || (op) == MC_REPNZ || ((op) >= MC_FADD && (op) <= MC_FLD1))
#define SCHEDULER_FLOAT_COMPARE(op) ((op) == MC_FCOM || (op) == MC_FCOMP || (op) == MC_FCOMPP)
#define SCHEDULER_FLOAT_LOAD(op) ((op) == MC_FLD || (op) == MC_FLDZ || (op) == MC_FLD1)
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
    info->uses = info->defines = 0;
    info->memory = 0;
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
/* Original [00042a10,00042cc0). Choose one Pentium scheduling unit,
   preferring a pair and maintaining the prior cycle's pairing constraints. */
MachineInstruction *__fastcall choose_pentium_instruction(MachineInstruction *instruction)
{
    MachineInstruction *saved_previous = instruction->previous, *scan, *chosen;
    unsigned operation;
    int pipe_hint, candidate, count, wait, float_wait, dependent, pipe;
    while (!scheduler_float_invalid && ((instruction->opcode >> 8) & 255) == MC_FXCH) {
        scan = instruction->next;
        while ((scan->opcode & 15) != 0) {
            if (SCHEDULER_BARRIER(scan)) break;
            scan = scan->next;
        }
        remove_scheduled_float_exchange(instruction);
        remove_machine_instruction(instruction);
        if (SCHEDULER_BARRIER(scan)) return saved_previous;
        instruction = scan;
    }
    pipe_hint = -1;
    dependent = 0;
    if (try_schedule_barrier(instruction, &chosen, &pipe_hint)) return chosen;
    collect_pentium_candidates(instruction, pipe_hint);
    select_pentium_pair(&candidate, &count, &wait, &float_wait, &dependent);
    emit_pentium_pair(candidate, count, dependent);
    chosen = scheduler_pairs.candidates[candidate].first;
    scheduler_ready_count -= count;
    if (scheduler_ready_count) compact_pentium_candidates();
    if (!dependent) {
        chosen->opcode &= ~0x200000UL;
        if (!wait || (scheduler_pairs.previous[1].first &&
            SCHEDULER_FLOAT_OPERATION((scheduler_pairs.previous[0].first->opcode >> 8) & 255))) {
            if (float_wait) {
                insert_pentium_stall(0);
                if (scheduler_pairs.previous[1].first) {
                    operation = (scheduler_pairs.previous[1].first->opcode >> 8) & 255;
                    if (operation != MC_FST && operation != MC_FSTP) {
                        operation = (chosen->opcode >> 8) & 255;
                        if (operation == MC_FST || operation == MC_FSTP) chosen->opcode |= 0x200000UL;
                    }
                }
            } else if (pentium_previous_memory_delay(scheduler_pairs.previous[0].first))
                insert_pentium_stall(3);
            else {
                operation = (chosen->opcode >> 8) & 255;
                if ((operation == MC_FST || operation == MC_FSTP) &&
                    scheduler_pairs.previous[1].first && !scheduler_pairs.previous[1].second) {
                    pipe = 4;
                    if (classify_pentium_pipe(scheduler_pairs.previous[1].first, &pipe))
                        chosen->opcode |= 0x200000UL;
                }
            }
        } else insert_pentium_stall(1);
    }
    return chosen;
}
/* Original [00044680,00044cba). Handle fixed scheduling boundaries and
   readily paired adjacent operations before constructing a candidate set. */
int __fastcall try_schedule_barrier(MachineInstruction *instruction,
    MachineInstruction **chosen, int *pipe_hint)
{
    MachineInstruction *other = 0, *temporary;
    unsigned operation = (instruction->opcode >> 8) & 255;
    int count = 0, pipe = 0, other_pipe;
    if (SCHEDULER_BARRIER(instruction) || operation == MC_CALL) count = 1;
    else if (operation == MC_REP || operation == MC_REPZ || operation == MC_REPNZ) {
        for (other = instruction->next; other && !SCHEDULABLE_KIND(other->opcode & 15); other = other->next) ;
        count = 2;
    }
    if (scheduler_has_float) {
        if (!count) return 0;
    } else if (!count) {
        if (instruction_schedule_delay(instruction)) return 0;
        pipe = 4;
        if (scheduler_pairs.previous[0].first && !scheduler_pairs.previous[0].second &&
            classify_pentium_pipe(scheduler_pairs.previous[0].first, &pipe)) return 0;
        if (SCHEDULER_FLOAT_OPERATION(operation) && instruction_float_not_ready(instruction)) {
            *pipe_hint = 0;
            return 0;
        }
        pipe = 0;
        if (SCHEDULER_FLOAT_OPERATION(operation) || !classify_pentium_pipe(instruction, &pipe)) count = 1;
        if (!count) {
            other = instruction;
            if (!SCHEDULER_BARRIER(instruction)) {
                for (other = instruction->next; other; other = other->next)
                    if (SCHEDULABLE_KIND(other->opcode & 15) || SCHEDULER_BARRIER(other)) break;
            }
            if (SCHEDULER_BARRIER(instruction) || !other || SCHEDULER_BARRIER(other)) count = 1;
            if (!count) {
                if (scheduler_move_conflict(instruction, other, 1, 0)) {
                    scheduler_undo = 0;
                    operation = (instruction->opcode >> 8) & 255;
                    if (SCHEDULER_STACK_TRANSFER(operation)) {
                        operation = (other->opcode >> 8) & 255;
                        if (SCHEDULER_STACK_TRANSFER(operation) || operation == MC_CALL) count = 2;
                    }
                    if (!count) {
                        if (pentium_pair_dependency(instruction, other) && classify_pentium_pipe(other, &pipe) &&
                            (!scheduler_pairs.previous[0].first || !instruction_pair_conflict(other, scheduler_pairs.previous[0].first)) &&
                            (!scheduler_pairs.previous[0].second || !instruction_pair_conflict(other, scheduler_pairs.previous[0].second)))
                            count = 2;
                        if (!count) return 0;
                    }
                }
                scheduler_undo = 0;
            }
        }
    }
    if (!count) {
        other_pipe = pipe;
        if (!classify_pentium_pipe(other, &other_pipe) || instruction_pair_conflict(other, instruction) ||
            (scheduler_pairs.previous[0].first && instruction_pair_conflict(other, scheduler_pairs.previous[0].first)) ||
            (scheduler_pairs.previous[0].second && instruction_pair_conflict(other, scheduler_pairs.previous[0].second))) return 0;
        if (pipe == 2 || other_pipe == 1) {
            if (scheduler_move_conflict(other, instruction, 0, 1)) {
                scheduler_undo = 0;
                return 0;
            }
            place_scheduled_instruction(other, instruction);
            temporary = instruction; instruction = other; other = temporary;
        }
        count = 2;
    }
    operation = (instruction->opcode >> 8) & 255;
    if (SCHEDULER_FLOAT_OPERATION(operation) && !scheduler_float_invalid) apply_scheduled_float(instruction);
    if (operation == MC_CALL && !scheduler_float_invalid &&
        (instruction->operands[0].flags & 15) == OPERAND_SYMBOL &&
        (((FunctionSymbol *)instruction->operands[0].value.symbol)->field24 & 15) == 5)
        apply_scheduled_float(instruction);
    rotate_pentium_history();
    scheduler_pairs.previous[0].first = instruction;
    if (scheduler_track_registers) update_scheduled_registers(instruction);
    if (instruction->record.scheduling->flags & 0x100) {
        instruction->record.scheduling->flags &= ~0x100U;
        remove_scheduled_float_exchange(instruction);
    }
    advance_pentium_float_positions(instruction, scheduler_ready_count);
    instruction->record.scheduling->flags |= 0x800;
    *chosen = instruction;
    scheduler_cursor = instruction;
    if (count == 2) {
        operation = (other->opcode >> 8) & 255;
        if (operation == MC_CALL && !scheduler_float_invalid && (other->operands[0].flags & 15) == OPERAND_SYMBOL &&
            (((FunctionSymbol *)other->operands[0].value.symbol)->field24 & 15) == 5)
            apply_scheduled_float(other);
        *chosen = other;
        scheduler_pairs.previous[0].second = other;
        if (scheduler_track_registers) update_scheduled_registers(other);
        scheduler_cursor = other;
        if (other->record.scheduling->flags & 0x100) {
            other->record.scheduling->flags &= ~0x100U;
            remove_scheduled_float_exchange(other);
        }
        advance_pentium_float_positions(other, scheduler_ready_count);
        other->record.scheduling->flags |= 0x800;
        instruction->record.scheduling->flags |= 0x40;
        other->record.scheduling->flags |= 0x40 | 0x80;
    }
    return 1;
}
/* Original [00044d20,00044d69). An outstanding instruction in either
   preceding pipeline blocks this candidate when their operands conflict. */
int __fastcall instruction_schedule_delay(MachineInstruction *instruction)
{
    if (scheduler_pairs.previous[0].first &&
        instruction_pair_conflict(instruction, scheduler_pairs.previous[0].first)) return 1;
    if (scheduler_pairs.previous[0].second &&
        instruction_pair_conflict(instruction, scheduler_pairs.previous[0].second)) return 1;
    return 0;
}
/* Original [00043090,000432dc), including switches at43210..432dc. Return a
   compatible U/V pipe class:1 U,2 V,4 either, excluding nonpairable forms. */
int __fastcall classify_pentium_pipe(MachineInstruction *instruction, int *pipe)
{
    unsigned operation = (instruction->opcode >> 8) & 255;
    unsigned destination = instruction->operands[0].flags & 15;
    unsigned source = instruction->operands[1].flags & 15;
    int selected;
    if (SCHEDULER_FLOAT_OPERATION(operation)) return 0;
    if (machine_operand_counts[operation] == 2 && (source == OPERAND_IMMEDIATE || source == OPERAND_LABEL_ADDRESS) &&
        (destination == OPERAND_BASED || destination == OPERAND_INDEXED || destination == OPERAND_ABSOLUTE) &&
        (instruction->operands[0].value.address->first.displacement || instruction->operands[0].value.address->symbol)) return 0;
    selected = pentium_prefix_instruction(instruction) ? 1 : 4;
    switch (operation) {
    case MC_MOV: case MC_LEA: case MC_DEC: case MC_INC:
    case MC_ADD: case MC_AND: case MC_CMP: case MC_OR: case MC_SUB: case MC_XOR:
        break;
    case MC_POP: case MC_PUSH:
        if (destination == OPERAND_BASED || destination == OPERAND_INDEXED || destination == OPERAND_ABSOLUTE) return 0;
        break;
    case MC_CALL: case MC_JCC: case MC_JMP: selected = 2; break;
    case MC_ROL: case MC_ROR:
        if (source != OPERAND_IMMEDIATE || instruction->operands[1].value.constant != 1) return 0;
        selected = 1; break;
    case MC_SHL:
        if (source != OPERAND_IMMEDIATE) return 0;
        selected = 1; break;
    case MC_ADC: case MC_SBB: selected = 1; break;
    case MC_TEST:
        if (destination == OPERAND_BASED || destination == OPERAND_INDEXED || destination == OPERAND_ABSOLUTE) return 0;
        if (source == OPERAND_IMMEDIATE && (instruction->operands[0].value.reg.words.number & 0x3fff)) return 0;
        break;
    default: return 0;
    }
    if (selected != 4 && *pipe == selected) return 0;
    *pipe = selected;
    return 1;
}
/* Original [00040720,000407a6). Sixteen-bit and selected extended
   operations constrain pairing. Preserve native inlined operand-null guards. */
int __fastcall pentium_prefix_instruction(MachineInstruction *instruction)
{
    unsigned operation;
    if ((instruction->opcode & 0xf0) == 0x20) return 1;
    operation = (instruction->opcode >> 8) & 255;
    switch (operation) {
    case MC_IMUL:
        return (unsigned)instruction + 16 != 0 &&
            (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
            (unsigned)instruction + 24 != 0;
    case MC_REP: case MC_REPNZ: case MC_REPZ: case MC_SCC:
    case MC_MOVZX: case MC_MOVSX: case MC_SHLD: case MC_SHRD: case MC_BT:
        return 1;
    default: return 0;
    }
}
/* Original [000407b0,00040edf). Dependence test for reordering a pair:
   flags, physical registers, tracked addresses and x87 stack identities. */
int __fastcall scheduler_move_conflict(MachineInstruction *first,
    MachineInstruction *last, int forward, int restore)
{
    unsigned left, right, first_flags, last_flags;
    unsigned r0, r1, l0, l1;
    InstructionSchedule *a, *b;
    SchedulerMemoryAccess *am, *bm;
    int scratch;
    if (!SCHEDULABLE_KIND(first->opcode & 15) || !SCHEDULABLE_KIND(last->opcode & 15)) return 0;
    left = (first->opcode >> 8) & 255;
    right = (last->opcode >> 8) & 255;
    if ((left == MC_CLD || left == MC_STD) &&
        (right == MC_MOVS || right == MC_STOS || right == MC_CLD || right == MC_STD)) return 1;
    if ((right == MC_CLD || right == MC_STD) &&
        (left == MC_MOVS || left == MC_STOS || left == MC_CLD || left == MC_STD)) return 1;
    if (left == MC_CALL && SCHEDULER_CALL_SERIALIZED(right)) return 1;
    if (right == MC_CALL && SCHEDULER_CALL_SERIALIZED(left)) return 1;
    a = first->record.scheduling;
    b = last->record.scheduling;
    if (SCHEDULER_FLOAT_OPERATION(left) && SCHEDULER_FLOAT_OPERATION(right)) {
        if (cpu_model != 5 || scheduler_float_invalid || !forward) return 1;
        if ((SCHEDULER_FLOAT_COMPARE(left) && (right == MC_FNSTSW || right == MC_FSTSW)) ||
            (SCHEDULER_FLOAT_COMPARE(right) && (left == MC_FNSTSW || left == MC_FSTSW))) return 1;
        if (SCHEDULER_FLOAT_LOAD(left) && SCHEDULER_FLOAT_LOAD(right)) return 1;
        l0 = first->operands[0].value.reg.words.number & 0x3fff;
        l1 = first->operands[1].value.reg.words.number & 0x3fff;
        r0 = last->operands[0].value.reg.words.number & 0x3fff;
        r1 = last->operands[1].value.reg.words.number & 0x3fff;
        if ((first->operands[0].flags & 15) == OPERAND_REGISTER && l0 > REG_ST0 && l0 < REG_ST0 + 8 &&
            (last->operands[1].flags & 15) == OPERAND_REGISTER && r1 >= REG_ST0 && r1 < REG_ST0 + 8 &&
            a->float_depth - b->float_depth + r1 == l0) return 1;
        if ((last->operands[0].flags & 15) == OPERAND_REGISTER && r0 > REG_ST0 && r0 < REG_ST0 + 8) {
            if ((first->operands[1].flags & 15) == OPERAND_REGISTER && l1 >= REG_ST0 && l1 < REG_ST0 + 8 &&
                b->float_depth - a->float_depth + r0 == l1) return 1;
            if ((first->operands[0].flags & 15) == OPERAND_REGISTER && l0 > REG_ST0 && l0 < REG_ST0 + 8 &&
                a->float_depth - b->float_depth + r0 == l0) return 1;
        }
        if (scheduled_float_conflict(first, last, &scratch)) return 1;
    }
    first_flags = a->flags; last_flags = b->flags;
    if ((first_flags & 6) && ((machine_selection_properties[right] & MC_WRITES_FLAGS) || right == MC_CALL)) return 1;
    if ((last_flags & 6) && ((machine_selection_properties[left] & MC_WRITES_FLAGS) || left == MC_CALL)) return 1;
    if (!(first_flags & 1)) analyze_scheduled_instruction(first);
    else if (a->memory && !a->memory->symbol && (a->memory->operand->flags & 15) == OPERAND_BASED)
        resolve_scheduled_address(a->memory, a->memory->base_register);
    if (!(b->flags & 1)) analyze_scheduled_instruction(last);
    else if (b->memory && !b->memory->symbol && (b->memory->operand->flags & 15) == OPERAND_BASED)
        resolve_scheduled_address(b->memory, b->memory->base_register);
    if (a->defines & b->defines) return 1;
    if (((a->defines & (1U << REG_ESP)) && (b->flags & 0x1000) && scheduled_stack_overlap(first, last)) ||
        ((b->defines & (1U << REG_ESP)) && (a->flags & 0x1000) && scheduled_stack_overlap(last, first))) return 1;
    if (b->uses & a->defines) {
        if (cpu_model == 5 && forward == 1 && restore && !(a->uses & b->defines) &&
            rewrite_scheduled_dependence(last, first, 0, 1)) return 0;
        return 1;
    }
    if (restore) {
        if (rewrite_scheduled_dependence(first, last, forward, 0)) return 0;
        if (forward == 1 && rewrite_scheduled_stack(first, last)) return 0;
    }
    if (a->uses & b->defines) return 1;
    if (left == MC_LEA || right == MC_LEA) return 0;
    am = a->memory; bm = b->memory;
    if ((right == MC_CALL && (a->flags & 0x1000)) ||
        (left == MC_CALL && (b->flags & 0x1000))) return 1;
    if (!am && !bm) return ((a->flags | b->flags) & 0x1000) != 0;
    if (left == MC_MOVS || right == MC_MOVS || left == MC_STOS || right == MC_STOS ||
        left == MC_STI || right == MC_STI || left == MC_CLI || right == MC_CLI) return 1;
    if (am && bm) {
        if (!((am->flags ^ bm->flags) & 3) && !(bm->flags & 3)) return 0;
        return scheduled_memory_overlap(am, bm, forward);
    }
    return 0;
}
/* Original [000417b0,00041bd4). Lazily classify explicit/implicit
   register dependencies and attach a tracked memory operand. */
void __fastcall analyze_scheduled_instruction(MachineInstruction *instruction)
{
    MachineOperand *destination, *source;
    SchedulerMemoryAccess *memory = 0;
    InstructionSchedule *info = instruction->record.scheduling;
    unsigned operation, kind, implicit_uses = 0, implicit_defines = 0, i;
    unsigned short source_register = 0xffff, destination_register = 0xffff;
    short source_index = -1, destination_index = -1;
    if (!SCHEDULABLE_KIND(instruction->opcode & 15)) return;
    operation = (instruction->opcode >> 8) & 255;
    destination = machine_operand_counts[operation] ? &instruction->operands[0] : 0;
    source = machine_operand_counts[operation] == 2 ? &instruction->operands[1] : 0;
    if (operation == MC_CALL) { source = destination; destination = 0; }
    else if (!destination || !(destination->flags & 15)) goto implicit_registers;
    if (source) {
        kind = source->flags & 15;
        if (kind == OPERAND_REGISTER) {
            source_register = source->value.reg.words.number & 0x3fff;
            if (source_register > scheduler_register_capacity)
                compiler_internal_error("@(#)sched.c:1.33", 0x456);
            if (machine_operand_bytes(instruction, 1) == 1 && source_register > 3) source_register -= 4;
            if (!SCHEDULER_FLOAT_OPERATION(operation)) info->uses |= 1U << (source_register & 31);
        } else if (kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE) {
            info->flags |= 0x1000;
            analyze_scheduled_address(instruction, source, (short *)&source_register, &source_index, &memory);
            if (source_register != 0xffff) info->uses |= 1U << (source_register & 31);
            if (source_index != -1) info->uses |= 1U << (source_index & 31);
            if (scheduler_track_registers) memory->flags &= ~3U;
            info->memory = memory;
        }
    }
    if (!destination) goto implicit_registers;
    kind = destination->flags & 15;
    if (kind == OPERAND_REGISTER) {
        destination_register = destination->value.reg.words.number & 0x3fff;
        if (machine_operand_bytes(instruction, 0) == 1 && destination_register > 3) destination_register -= 4;
        if (scheduled_reads_first(operation) && !SCHEDULER_FLOAT_OPERATION(operation))
            info->uses |= 1U << (destination_register & 31);
        if (writes_first_operand(instruction) && !SCHEDULER_FLOAT_OPERATION(operation))
            info->defines |= 1U << (destination_register & 31);
    } else if (kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE) {
        info->flags |= 0x1000;
        analyze_scheduled_address(instruction, destination, (short *)&destination_register, &destination_index, &memory);
        if (destination_register != 0xffff) info->uses |= 1U << (destination_register & 31);
        if (destination_index != -1) info->uses |= 1U << (destination_index & 31);
        if (scheduler_track_registers) {
            if (!scheduled_reads_first(operation)) memory->flags = (memory->flags & ~3U) | 1;
            else if (!writes_first_operand(instruction)) memory->flags &= ~3U;
            else memory->flags = (memory->flags & ~3U) | 2;
        }
        info->memory = memory;
    } else if (kind != OPERAND_LABEL_ADDRESS && kind != OPERAND_IMMEDIATE && kind != OPERAND_SYMBOL)
        scheduler_stop_window = 1;
implicit_registers:
    if (machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS) {
        implicit_defines = implicit_register_mask(instruction, 2);
        implicit_uses = implicit_register_mask(instruction, 1);
    }
    if (operation == MC_REP || operation == MC_REPZ || operation == MC_REPNZ) {
        implicit_defines |= (1U << REG_ESI) | (1U << REG_EDI);
        implicit_uses |= (1U << REG_ESI) | (1U << REG_EDI) | (1U << REG_EAX);
    }
    if (SCHEDULER_STACK_TRANSFER(operation) || operation == MC_CALL) {
        implicit_defines |= 1U << REG_ESP;
        implicit_uses |= 1U << REG_ESP;
    }
    if (operation == MC_FNSTSW || operation == MC_FSTSW) implicit_defines |= 1U << REG_EAX;
    for (i = 0; i < 8; ++i) {
        if (implicit_uses & (1U << i)) info->uses |= 1U << i;
        if (implicit_defines & (1U << i)) info->defines |= 1U << i;
    }
    info->flags |= 1;
}
/* Original [00041c50,00041dbf). Capture a memory operand's address
   dependencies, retaining the original operand and optional resolved symbol. */
void __fastcall analyze_scheduled_address(MachineInstruction *instruction,
    MachineOperand *operand, short *base, short *index, SchedulerMemoryAccess **memory)
{
    SchedulerMemoryAccess *record;
    MachineAddress *address;
    unsigned kind;
    *base = *index = -1;
    kind = operand->flags & 15;
    if (scheduler_track_registers) {
        record = arena_alloc(3, sizeof(SchedulerMemoryAccess));
        *memory = record;
        record->next = 0; record->instruction = 0; record->displacement = 0;
        record->symbol = 0; record->operand = 0;
        record->base_register = record->index_register = 0;
        record->width = record->flags = 0;
        record->instruction = instruction;
        record->operand = operand;
        record->width = (instruction->opcode >> 4) & 15;
        record->base_register = record->index_register = -1;
        if (kind == OPERAND_BASED && !operand->value.address->symbol)
            resolve_scheduled_address(record, (short)operand->value.address->base.words.number);
        if (kind == OPERAND_ABSOLUTE) return;
        if (kind == OPERAND_BASED) {
            record->base_register = *base = (short)operand->value.address->base.words.number;
            return;
        }
        if (kind != OPERAND_INDEXED) {
            compiler_internal_error("@(#)sched.c:1.33", 0x518);
            return;
        }
        address = operand->value.address;
        if ((short)address->base.words.number != 27)
            record->base_register = *base = (short)address->base.words.number;
        record->index_register = *index = (short)address->index.words.number;
    } else {
        if (kind == OPERAND_ABSOLUTE) return;
        if (kind == OPERAND_BASED) {
            *base = (short)operand->value.address->base.words.number;
            return;
        }
        if (kind != OPERAND_INDEXED) {
            compiler_internal_error("@(#)sched.c:1.33", 0x518);
            return;
        }
        address = operand->value.address;
        if ((short)address->base.words.number != 27) *base = (short)address->base.words.number;
        *index = (short)address->index.words.number;
    }
}
/* Original [00041be0,00041c43). Operations that replace their first
   operand do not consume its prior value. */
int __fastcall scheduled_reads_first(unsigned operation)
{
    switch (operation) {
    case MC_MOV: case MC_IN: case MC_POP: case MC_LEA:
    case MC_FIST: case MC_FISTP: case MC_FST: case MC_FSTP:
    case MC_FSTSW: case MC_FNSTSW: case MC_SCC:
    case MC_MOVZX: case MC_MOVZXW: case MC_MOVSX: case MC_MOVSXW:
        return 0;
    default: return 1;
    }
}
