/* Machine scheduling windows, x87 value state and nested-region tracking. */
#include "scheduler.h"
#include "machine-layout.h"
#include "instruction-selection.h"
#include "instruction-rewrite.h"
#include "register-use.h"
#include <stdlib.h>
#include <recon.h>
extern int cpu_model, optimize_speed;
SchedulerRegionFrame *scheduler_regions; /*82ff0*/
MachineInstruction *scheduler_instruction; /*7c300*/
unsigned short scheduler_register_capacity; /*7e290*/
SchedulerRegisterState **scheduler_register_states; /*7c348*/
int scheduler_track_registers; /*7c308*/
/* 7e288: i486 address-delay mode; generic scheduling writes1/4 at
  4024f/400f0 and selects its current mode at40568. Original BSS starts0. */
int scheduler_address_penalty;
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
   History pairs hold instructions; selected/ready records hold undo chains. */
SchedulerUndo *scheduler_undo; /*7c304*/
MachineInstruction *scheduler_cursor; /*7c2fc*/
MachineInstruction *scheduler_last_float; /*7c34c*/
signed char scheduler_float_delay; /*82fcc: native BYTE writes*/
int scheduler_stop_window; /*82ff4*/
int scheduler_ready_count; /*82ff8*/
SchedulerPairs scheduler_pairs; /*7c2ac..7c2fc: history, selected pair and ready set*/

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
        for (i = 0; i < 3; ++i) {
            scheduler_pairs.previous[i].first = 0;
            scheduler_pairs.previous[i].second = 0;
        }
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
    scheduler_pairs.selected[0].instruction = 0;
    scheduler_pairs.selected[0].undo = 0;
    scheduler_pairs.selected[1].instruction = 0;
    scheduler_pairs.selected[1].undo = 0;
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
    chosen = scheduler_pairs.selected[candidate - 1].instruction;
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
    prune_pentium_candidate_undos(instruction, scheduler_ready_count);
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
        prune_pentium_candidate_undos(other, scheduler_ready_count);
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
/* Original [00045490,000456c7). Recognize the dependency that can be
   accommodated by a same-cycle pair without a conflicting address input. */
int __fastcall pentium_pair_dependency(MachineInstruction *first, MachineInstruction *last)
{
    unsigned left = (first->opcode >> 8) & 255;
    unsigned right = (last->opcode >> 8) & 255;
    unsigned source_kind, destination_kind;
    InstructionSchedule *a = first->record.scheduling, *b = last->record.scheduling;
    int relation;
    if (!(a->flags & 1) || !(b->flags & 1) || !machine_operand_counts[left] ||
        !machine_operand_counts[right] || (last->operands[0].flags & 15) != OPERAND_REGISTER) return 0;
    if ((b->uses & a->defines) && !SCHEDULER_STACK_TRANSFER(left)) return 0;
    if (!(a->uses & b->defines) || (a->defines & b->defines)) return 0;
    if (machine_operand_counts[left] == 1 || machine_operand_counts[right] == 1) return 1;
    source_kind = last->operands[1].flags & 15;
    if (source_kind != OPERAND_BASED && source_kind != OPERAND_INDEXED && source_kind != OPERAND_ABSOLUTE) return 1;
    destination_kind = first->operands[0].flags & 15;
    if (destination_kind == OPERAND_REGISTER)
        return !operand_mentions_register(&last->operands[1], first->operands[0].value.reg.words.number);
    if (destination_kind != OPERAND_BASED && destination_kind != OPERAND_INDEXED && destination_kind != OPERAND_ABSOLUTE) return 1;
    if (right == MC_LEA) return 1;
    if (compare_scheduled_memory(&first->operands[0], (first->opcode >> 4) & 15,
        &last->operands[1], (last->opcode >> 4) & 15, 0, 0, &relation, 0, 0)) return 2;
    return 1;
}
/* Original [00044600,00044676). Seed the ready set with the current
   operation and extend it in each direction, up to five candidates. */
void __fastcall collect_pentium_candidates(MachineInstruction *instruction, int pipe_hint)
{
    if (!(instruction->record.scheduling->flags & 0x100)) {
        scheduler_pairs.ready[scheduler_ready_count].instruction = instruction;
        scheduler_pairs.ready[scheduler_ready_count].undo = 0;
        ++scheduler_ready_count;
        instruction->record.scheduling->flags |= 0x100;
    }
    if (scheduler_ready_count < 5) gather_pentium_candidates(instruction, 1, pipe_hint);
    if (scheduler_ready_count < 5 && scheduler_pairs.previous[0].first)
        gather_pentium_candidates(instruction, 0, pipe_hint);
}
/* Original [00045ee0,0004612b). Search up to six nearby operations,
   retaining movement undo chains for at most five ready candidates. */
void __fastcall gather_pentium_candidates(MachineInstruction *instruction,
    int forward, int pipe_hint)
{
    MachineInstruction *before = instruction->previous;
    unsigned operation;
    int count = 0, allow_backward, conflict, i;
    if (forward == 1) allow_backward = 0;
    else {
        instruction = scheduler_pairs.previous[0].first;
        allow_backward = pipe_hint != 0;
    }
    do {
        do {
            instruction = forward ? instruction->next : instruction->previous;
        } while (instruction && !SCHEDULABLE_KIND(instruction->opcode & 15) && !SCHEDULER_BARRIER(instruction));
        if (instruction) {
            if (SCHEDULER_BARRIER(instruction)) instruction = 0;
            else {
                operation = (instruction->opcode >> 8) & 255;
                if (!(instruction->record.scheduling->flags & 0x140) &&
                    (forward || !SCHEDULER_FLOAT_OPERATION(operation)) &&
                    scan_schedule_interval(instruction, before, forward, 0, 0, allow_backward, 0)) {
                    conflict = operation == MC_FXCH;
                    if (!conflict && !forward) {
                        for (i = 0; i < scheduler_ready_count; ++i)
                            if (scheduler_move_conflict(scheduler_pairs.ready[i].instruction, instruction, 2, 0)) {
                                conflict = 1;
                                break;
                            }
                    } else if (!conflict && SCHEDULER_FLOAT_OPERATION(operation) &&
                        scheduler_float_depth + scheduler_region_balance > 7 &&
                        (operation == MC_FLD || operation == MC_FILD || operation == MC_FLDZ || operation == MC_FLD1))
                        conflict = 1;
                    if (!conflict) {
                        scheduler_pairs.ready[scheduler_ready_count].instruction = instruction;
                        scheduler_pairs.ready[scheduler_ready_count].undo = scheduler_undo;
                        instruction->record.scheduling->flags |= 0x100;
                        ++scheduler_ready_count;
                    }
                }
                scheduler_undo = 0;
            }
        }
        ++count;
    } while (instruction && count <= 5 && scheduler_ready_count <= 4);
}
/* Original [00041240,00041795). Test each intervening instruction before
   moving a scheduling candidate; optionally commit its new position. */
int __fastcall scan_schedule_interval(MachineInstruction *instruction,
    MachineInstruction *before, int forward, int flags, int force,
    int allow_backward, int restore)
{
    MachineInstruction *previous, *next, *scan, *end;
    unsigned operation = (instruction->opcode >> 8) & 255, prior_operation;
    unsigned schedule_flags = instruction->record.scheduling->flags;
    int conflict;
    if ((schedule_flags & 0x20) || ((schedule_flags >> 8) & 6) > 2) return 0;
    if (operation == MC_REP || operation == MC_REPZ || operation == MC_REPNZ ||
        operation == MC_SBB || operation == MC_SCC || operation == MC_STI || operation == MC_CLI) return 0;
    previous = instruction->previous;
    if (previous && SCHEDULABLE_KIND(previous->opcode & 15)) {
        prior_operation = (previous->opcode >> 8) & 255;
        if (prior_operation == MC_REP || prior_operation == MC_REPZ || prior_operation == MC_REPNZ ||
            prior_operation == MC_SBB || prior_operation == MC_SCC) return 0;
    }
    if (cpu_model == 5) {
        if (allow_backward && !can_schedule_pentium(instruction, forward)) return 0;
    } else if (!flags) {
        if (SCHEDULER_FLOAT_OPERATION(operation)) return 0;
    } else {
        if (cpu_model != 3 && instruction_pair_conflict(instruction, before)) return 0;
        while (previous && !SCHEDULABLE_KIND(previous->opcode & 15) && !SCHEDULER_BARRIER(previous))
            previous = previous->previous;
        next = instruction;
        if (!SCHEDULER_BARRIER(instruction)) {
            for (next = instruction->next; next; next = next->next)
                if (SCHEDULABLE_KIND(next->opcode & 15) || SCHEDULER_BARRIER(next)) break;
        }
        if (previous && next && !SCHEDULER_BARRIER(previous) && !SCHEDULER_BARRIER(next) &&
            SCHEDULABLE_KIND(previous->opcode & 15) && SCHEDULABLE_KIND(next->opcode & 15) &&
            cpu_model != 3 && instruction_pair_conflict(next, previous)) return 0;
        for (next = before->next; next; next = next->next)
            if (SCHEDULABLE_KIND(next->opcode & 15) || SCHEDULER_BARRIER(next)) break;
        if (next && !SCHEDULER_BARRIER(next) && SCHEDULABLE_KIND(next->opcode & 15) &&
            cpu_model == 4 && instruction_pair_conflict(next, instruction)) return 0;
    }
    scan = forward ? instruction->previous : instruction->next;
    end = forward ? before : before->next;
    while (scan != end) {
        conflict = scheduler_move_conflict(scan, instruction, forward, 1);
        if (conflict && restore && next_machine_operation(instruction) == scan) conflict = 0;
        if (conflict) {
            scheduler_undo = 0;
            return 0;
        }
        scan = forward ? scan->previous : scan->next;
    }
    if (force) place_scheduled_instruction(before, instruction);
    return 1;
}
/* Original [00041df0,00041e72). Recover a symbolic address from the
   latest simple register definition when it is still available. */
void __fastcall resolve_scheduled_address(SchedulerMemoryAccess *memory, short reg)
{
    SchedulerRegisterState *state = scheduler_register_states[(unsigned short)reg & 0x3fff];
    MachineInstruction *definition;
    MachineOperand *source;
    unsigned operation, kind;
    if (!state || !state->recoverable || state->clobbered) return;
    definition = state->definition;
    operation = (definition->opcode >> 8) & 255;
    if (operation == MC_CALL || machine_operand_counts[operation] < 2) return;
    source = &definition->operands[1];
    kind = source->flags & 15;
    if (kind == OPERAND_LABEL_ADDRESS) memory->symbol = source->value.address->symbol;
    else if (operation == MC_LEA && kind >= OPERAND_BASED && kind <= OPERAND_INDEXED) {
        memory->symbol = source->value.address->symbol;
        memory->displacement = source->value.address->first.displacement;
    }
}
/* Original [00042310,00042780). Describe an address adjustment that
   permits an operation to cross a register increment or stack push. The
   adjustment is deferred in the current candidate's undo chain. */
int __fastcall rewrite_scheduled_dependence(MachineInstruction *first,
    MachineInstruction *last, int forward, int reverse)
{
    unsigned operation = (last->opcode >> 8) & 255;
    unsigned first_operation = (first->opcode >> 8) & 255;
    unsigned kind = last->operands[0].flags & 15, memory_kind;
    unsigned short reg, replacement = 0xffff;
    int first_count = machine_operand_counts[first_operation];
    long adjustment, base_adjustment = 0, index_adjustment = 0;
    MachineOperand *operand;
    MachineAddress *address, *lea;
    SchedulerUndo *undo;
    if (!machine_operand_counts[operation]) return 0;
    if (kind != OPERAND_REGISTER && operation != MC_PUSH && operation != MC_EPUSH) return 0;
    reg = operation == MC_PUSH || operation == MC_EPUSH ? REG_ESP : last->operands[0].value.reg.words.number & 0x3fff;
    if (first_count == 2 && (first->operands[1].flags & 15) == OPERAND_REGISTER &&
        (first->operands[1].value.reg.words.number & 0x3fff) == reg) return 0;
    if (machine_operand_counts[operation] == 2 && (last->operands[1].flags & 15) != OPERAND_IMMEDIATE && operation != MC_LEA) return 0;
    if (reg == REG_ESP && SCHEDULER_STACK_TRANSFER(first_operation)) return 0;
    switch (operation) {
    case MC_PUSH: case MC_EPUSH: adjustment = 4; break;
    case MC_LEA:
        if ((last->operands[1].flags & 15) != OPERAND_BASED) return 0;
        lea = last->operands[1].value.address;
        if (lea->symbol) return 0;
        if (reg != lea->base.words.number) {
            if (forward) return 0;
            replacement = lea->base.words.number;
            if (reverse) return 0;
        }
        adjustment = 0UL - (unsigned long)lea->first.displacement;
        break;
    case MC_DEC: adjustment = 1; break;
    case MC_INC: adjustment = -1; break;
    case MC_ADD: adjustment = 0UL - (unsigned long)last->operands[1].value.constant; break;
    case MC_SUB: adjustment = last->operands[1].value.constant; break;
    default: return 0;
    }
    if (!first_count) return 0;
    operand = &first->operands[0];
    memory_kind = operand->flags & 15;
    if (memory_kind != OPERAND_BASED && memory_kind != OPERAND_INDEXED && memory_kind != OPERAND_ABSOLUTE) {
        if (first_count != 2) return 0;
        operand = &first->operands[1];
        memory_kind = operand->flags & 15;
        if (memory_kind != OPERAND_BASED && memory_kind != OPERAND_INDEXED && memory_kind != OPERAND_ABSOLUTE) return 0;
    }
    address = operand->value.address;
    if ((operation == MC_PUSH || operation == MC_EPUSH) && (kind == OPERAND_BASED || kind == OPERAND_INDEXED) &&
        operand == &first->operands[0] && (memory_kind == OPERAND_BASED || memory_kind == OPERAND_INDEXED) &&
        address->symbol == last->operands[0].value.address->symbol) return 0;
    if (cpu_model == 4 && scheduler_address_penalty && first_count == 2 &&
        (first->operands[1].flags & 15) == OPERAND_IMMEDIATE &&
        !first->operands[0].value.address->first.displacement && !first->operands[0].value.address->symbol) return 0;
    if (cpu_model == 3 && !address->first.displacement && !address->symbol) return 0;
    if (memory_kind == OPERAND_ABSOLUTE) return 0;
    if (memory_kind == OPERAND_BASED) {
        if (address->base.words.number != reg) return 0;
    } else if (memory_kind == OPERAND_INDEXED) {
        if (replacement != 0xffff) return 0;
        if (address->index.words.number == reg)
            index_adjustment = (unsigned long)adjustment << ((operand->flags >> 8) & 3);
        if (address->base.words.number == reg) base_adjustment = adjustment;
        adjustment = (unsigned long)base_adjustment + (unsigned long)index_adjustment;
        if (!adjustment) return 0;
    } else return 0;
    if (!forward) adjustment = 0UL - (unsigned long)adjustment;
    if (adjustment < 0 && reg == REG_ESP) {
        if (!address->symbol) {
            if (address->first.displacement + adjustment < 0) return 0;
        } else if ((long)address->symbol->field04 + address->first.displacement + adjustment < 0) return 0;
    }
    undo = arena_alloc(3, sizeof(SchedulerUndo));
    undo->operand = operand;
    undo->owner = reverse ? last : first;
    undo->adjustment = adjustment;
    undo->replacement = replacement;
    undo->next = scheduler_undo;
    scheduler_undo = undo;
    return 1;
}
/* Original [000437e0,000445f5). Pentium choice policy: complete a prior
   pair, prefer available x87 work, then seek compatible integer pairs before
   falling back to an operation whose dependency/float delay must be paid. */
#define SELECT_READY(index) do { \
    scheduler_pairs.selected[0] = scheduler_pairs.ready[index]; \
    scheduler_pairs.ready[index] = scheduler_pairs.ready[scheduler_ready_count - 1]; \
    *candidate = *count = 1; \
} while (0)
#define PENTIUM_PAIR_COMPATIBLE(a,b,ap,bp) \
    ((((ap) == 1 || (ap) == 4) && ((bp) == 2 || (bp) == 4) && !instruction_pair_conflict(b,a)) || \
     (((bp) == 1 || (bp) == 4) && ((ap) == 2 || (ap) == 4) && !instruction_pair_conflict(a,b)))
#define SCHEDULER_FLOAT_ROOM(op) (scheduler_float_depth + scheduler_region_balance < 8 || \
    ((op) != MC_FLD && (op) != MC_FILD && (op) != MC_FLDZ && (op) != MC_FLD1))
void __fastcall select_pentium_pair(int *candidate, int *count,
    int *wait, int *float_wait, int *dependent)
{
    MachineInstruction *previous = scheduler_pairs.previous[0].first, *a, *b;
    SchedulerCandidate temporary, *saved_a = 0, *saved_b = 0;
    int i, j, pipe = 4, other_pipe, saved_i = 0, saved_j = 0, best;
    unsigned operation, previous_operation;
    *float_wait = 0;
    *wait = 0;
    if (previous && !scheduler_pairs.previous[0].second &&
        SCHEDULER_STACK_TRANSFER((previous->opcode >> 8) & 255)) {
        for (i = 0; i < scheduler_ready_count; ++i) {
            operation = (scheduler_pairs.ready[i].instruction->opcode >> 8) & 255;
            if (SCHEDULER_STACK_TRANSFER(operation) || operation == MC_CALL) {
                SELECT_READY(i);
                *dependent = 1;
                return;
            }
        }
    }
    if (scheduler_ready_count == 1) {
        scheduler_pairs.selected[0] = scheduler_pairs.ready[0];
        *candidate = *count = 1;
        a = scheduler_pairs.selected[0].instruction;
        *wait = instruction_schedule_delay(a);
        operation = (a->opcode >> 8) & 255;
        if (SCHEDULER_FLOAT_OPERATION(operation)) {
            *float_wait = instruction_float_not_ready(a);
            return;
        }
        if (previous && !scheduler_pairs.previous[0].second &&
            classify_pentium_pipe(previous, &pipe) && classify_pentium_pipe(a, &pipe))
            *dependent = pentium_pair_dependency(previous, a) || pentium_pair_independent(previous, a);
        return;
    }
    if (previous && ((previous->opcode >> 8) & 255) != MC_CALL && !scheduler_pairs.previous[0].second &&
        classify_pentium_pipe(previous, &pipe)) {
        for (i = 0; i < scheduler_ready_count; ++i) {
            other_pipe = 1;
            a = scheduler_pairs.ready[i].instruction;
            if (classify_pentium_pipe(a, &other_pipe) &&
                (pentium_pair_dependency(previous, a) || pentium_pair_independent(previous, a))) {
                SELECT_READY(i);
                *dependent = 1;
                return;
            }
        }
    }
    if (scheduler_has_float && previous) {
        previous_operation = (previous->opcode >> 8) & 255;
        if (previous_operation == MC_FLD || previous_operation == MC_FLDZ ||
            previous_operation == MC_FLD1 || previous_operation == MC_FST) {
            for (i = 0; i < scheduler_ready_count; ++i) {
                a = scheduler_pairs.ready[i].instruction;
                operation = (a->opcode >> 8) & 255;
                if (SCHEDULER_FLOAT_OPERATION(operation) &&
                    a->record.scheduling->float_inputs[0] == previous->record.scheduling->float_inputs[0] &&
                    (previous_operation == MC_FST ||
                     (operation != MC_FSTP && operation != MC_FST && operation != MC_FISTP && operation != MC_FIST))) {
                    SELECT_READY(i);
                    return;
                }
            }
        }
    }
    /* Native descending-priority selection sort, retaining equal-priority order
       exactly as this sequence of swaps does. */
    for (i = 0; i < scheduler_ready_count; ++i) {
        for (j = i + 1; j < scheduler_ready_count; ++j) {
            if (scheduler_pairs.ready[i].instruction->record.scheduling->priority <
                scheduler_pairs.ready[j].instruction->record.scheduling->priority) {
                temporary = scheduler_pairs.ready[i];
                scheduler_pairs.ready[i] = scheduler_pairs.ready[j];
                scheduler_pairs.ready[j] = temporary;
            }
        }
    }
    if (scheduler_has_float) {
        for (i = 0; i < scheduler_ready_count; ++i) {
            a = scheduler_pairs.ready[i].instruction;
            operation = (a->opcode >> 8) & 255;
            if (SCHEDULER_FLOAT_OPERATION(operation) && !instruction_schedule_delay(a) &&
                !instruction_float_not_ready(a) && SCHEDULER_FLOAT_ROOM(operation)) {
                SELECT_READY(i);
                return;
            }
        }
    }
    for (i = 0; i < scheduler_ready_count; ++i) {
        pipe = 0;
        a = scheduler_pairs.ready[i].instruction;
        if (!classify_pentium_pipe(a, &pipe) && !instruction_schedule_delay(a) &&
            !SCHEDULER_FLOAT_OPERATION((a->opcode >> 8) & 255)) {
            SELECT_READY(i);
            return;
        }
    }
    for (i = 0; i < scheduler_ready_count; ++i) {
        a = scheduler_pairs.ready[i].instruction;
        pipe = 0;
        if (((a->opcode >> 8) & 255) == MC_CALL || !classify_pentium_pipe(a, &pipe) ||
            instruction_schedule_delay(a) || pentium_previous_memory_delay(a)) continue;
        for (j = i + 1; j < scheduler_ready_count; ++j) {
            b = scheduler_pairs.ready[j].instruction;
            other_pipe = pipe;
            if (!classify_pentium_pipe(b, &other_pipe) || !PENTIUM_PAIR_COMPATIBLE(a,b,pipe,other_pipe)) continue;
            if (!instruction_schedule_delay(b) && !pentium_previous_memory_delay(b)) {
                scheduler_pairs.selected[0] = scheduler_pairs.ready[i];
                scheduler_pairs.selected[1] = scheduler_pairs.ready[j];
                remove_pentium_ready_pair(i, j);
                if (pipe == 2 || other_pipe == 1 || instruction_pair_conflict(b, a)) {
                    temporary = scheduler_pairs.selected[0];
                    scheduler_pairs.selected[0] = scheduler_pairs.selected[1];
                    scheduler_pairs.selected[1] = temporary;
                }
                *count = *candidate = 2;
                return;
            }
            if (!saved_a) {
                if (pipe == 2 || other_pipe == 1 || instruction_pair_conflict(b, a)) {
                    saved_a = &scheduler_pairs.ready[j]; saved_b = &scheduler_pairs.ready[i];
                    saved_i = j; saved_j = i;
                } else {
                    saved_a = &scheduler_pairs.ready[i]; saved_b = &scheduler_pairs.ready[j];
                    saved_i = i; saved_j = j;
                }
            }
        }
    }
    if (saved_a && saved_b) {
        scheduler_pairs.selected[0] = *saved_a;
        scheduler_pairs.selected[1] = *saved_b;
        remove_pentium_ready_pair(saved_i, saved_j);
        *count = *candidate = 2;
        *wait = 1;
        return;
    }
    for (i = 0; i < scheduler_ready_count; ++i) {
        a = scheduler_pairs.ready[i].instruction;
        pipe = 0;
        if (((a->opcode >> 8) & 255) == MC_CALL || !classify_pentium_pipe(a, &pipe)) continue;
        for (j = i + 1; j < scheduler_ready_count; ++j) {
            b = scheduler_pairs.ready[j].instruction;
            other_pipe = pipe;
            if (!classify_pentium_pipe(b, &other_pipe) || !PENTIUM_PAIR_COMPATIBLE(a,b,pipe,other_pipe)) continue;
            scheduler_pairs.selected[0] = scheduler_pairs.ready[i];
            scheduler_pairs.selected[1] = scheduler_pairs.ready[j];
            remove_pentium_ready_pair(i, j);
            if (pipe == 2 || other_pipe == 1 || instruction_pair_conflict(b, a)) {
                temporary = scheduler_pairs.selected[0];
                scheduler_pairs.selected[0] = scheduler_pairs.selected[1];
                scheduler_pairs.selected[1] = temporary;
            }
            *count = *candidate = 2;
            *wait = 1;
            return;
        }
    }
    pipe = 0;
    for (i = 0; i < scheduler_ready_count; ++i) {
        a = scheduler_pairs.ready[i].instruction;
        if (classify_pentium_pipe(a, &pipe) && !instruction_schedule_delay(a) && !pentium_previous_memory_delay(a)) {
            SELECT_READY(i);
            return;
        }
    }
    for (i = 0; i < scheduler_ready_count; ++i) {
        a = scheduler_pairs.ready[i].instruction;
        operation = (a->opcode >> 8) & 255;
        if (SCHEDULER_FLOAT_OPERATION(operation) && !instruction_float_not_ready(a) && SCHEDULER_FLOAT_ROOM(operation)) {
            SELECT_READY(i);
            *wait = 1;
            return;
        }
    }
    best = -1;
    for (i = 0; i < scheduler_ready_count; ++i) {
        a = scheduler_pairs.ready[i].instruction;
        operation = (a->opcode >> 8) & 255;
        if (SCHEDULER_FLOAT_OPERATION(operation) && SCHEDULER_FLOAT_ROOM(operation)) {
            if (best == -1 || scheduled_float_latency(scheduler_pairs.ready[best].instruction) > scheduled_float_latency(a)) best = i;
        }
    }
    if (best != -1) {
        SELECT_READY(best);
        *float_wait = 1;
        return;
    }
    for (i = 0; i < scheduler_ready_count; ++i) {
        pipe = 0;
        if (!classify_pentium_pipe(scheduler_pairs.ready[i].instruction, &pipe)) {
            SELECT_READY(i);
            *wait = 1;
            return;
        }
    }
    SELECT_READY(0);
    *wait = instruction_schedule_delay(scheduler_pairs.selected[0].instruction);
}
#undef SELECT_READY
#undef PENTIUM_PAIR_COMPATIBLE
#undef SCHEDULER_FLOAT_ROOM
/* Original [000435d0,000437d6). Commit the selected operation(s), apply
   deferred address changes and advance history/dependency tracking. */
void __fastcall emit_pentium_pair(int candidate, int count, int dependent)
{
    MachineInstruction *first = scheduler_pairs.selected[0].instruction, *last;
    unsigned operation = (first->opcode >> 8) & 255;
    if (SCHEDULER_FLOAT_OPERATION(operation) && !scheduler_float_invalid) apply_scheduled_float(first);
    if (operation == MC_CALL && !scheduler_float_invalid && (first->operands[0].flags & 15) == OPERAND_SYMBOL &&
        (((FunctionSymbol *)first->operands[0].value.symbol)->field24 & 15) == 5) apply_scheduled_float(first);
    prune_pentium_candidate_undos(first, scheduler_ready_count - count);
    scheduler_undo = scheduler_pairs.selected[0].undo;
    place_scheduled_instruction(scheduler_cursor, first);
    if (candidate == 2) remove_pentium_candidate_undo(first, &scheduler_pairs.selected[1]);
    if (!dependent) {
        rotate_pentium_history();
        scheduler_cursor = first;
        scheduler_pairs.previous[0].first = first;
    } else {
        scheduler_cursor = first;
        scheduler_pairs.previous[0].second = first;
        scheduler_pairs.previous[0].first->record.scheduling->flags |= 0x40;
        first->record.scheduling->flags |= 0x40 | 0x80;
    }
    first->record.scheduling->flags |= 0x800;
    first->record.scheduling->flags &= ~0x100U;
    if (scheduler_track_registers) update_scheduled_registers(first);
    if (candidate == 2) {
        last = scheduler_pairs.selected[1].instruction;
        operation = (last->opcode >> 8) & 255;
        if (operation == MC_CALL && !scheduler_float_invalid && (last->operands[0].flags & 15) == OPERAND_SYMBOL &&
            (((FunctionSymbol *)last->operands[0].value.symbol)->field24 & 15) == 5) apply_scheduled_float(last);
        prune_pentium_candidate_undos(last, scheduler_ready_count - count);
        scheduler_undo = scheduler_pairs.selected[1].undo;
        place_scheduled_instruction(first, last);
        scheduler_cursor = last;
        scheduler_pairs.previous[0].second = last;
        last->record.scheduling->flags |= 0x80 | 0x40;
        first->record.scheduling->flags |= 0x40;
        last->record.scheduling->flags |= 0x800;
        last->record.scheduling->flags &= ~0x100U;
        if (scheduler_track_registers) update_scheduled_registers(scheduler_pairs.previous[0].second);
    }
}
/* Original [00045900,00045927). A committed instruction no longer
   needs a deferred adjustment in another pending candidate's undo chain. */
void __fastcall prune_pentium_candidate_undos(MachineInstruction *instruction, int ready_count)
{
    int i;
    for (i = 0; i < ready_count; ++i)
        if (scheduler_pairs.ready[i].instruction != instruction)
            remove_pentium_candidate_undo(instruction, &scheduler_pairs.ready[i]);
}
/* Original [00040450,000404bd). Place last immediately after first,
   update movement history, then apply its deferred address corrections. */
void __fastcall place_scheduled_instruction(MachineInstruction *first, MachineInstruction *last)
{
    MachineInstruction *next;
    unsigned flags;
    if (first && last != first->next) {
        next = first->next;
        if (last->next) last->next->previous = last->previous;
        last->previous->next = last->next;
        first->next = last;
        last->previous = first;
        last->next = next;
        if (next) next->previous = last;
    }
    flags = last->record.scheduling->flags;
    if (cpu_model == 5) last->record.scheduling->flags = (flags & ~0x600U) | ((flags + 0x200) & 0x600);
    else last->record.scheduling->flags = flags | 0x20;
    apply_scheduler_undo();
    scheduler_undo = 0;
}
/* Original [000404c0,000404f0). Apply exactly the deferred displacement
   and optional base-register edits in the selected undo chain. */
void __cdecl apply_scheduler_undo(void)
{
    MachineAddress *address;
    while (scheduler_undo) {
        address = scheduler_undo->operand->value.address;
        address->first.displacement = (unsigned long)address->first.displacement + (unsigned long)scheduler_undo->adjustment;
        if (scheduler_undo->replacement != 0xffff) address->base.words.number = scheduler_undo->replacement;
        scheduler_undo = scheduler_undo->next;
    }
}
/* Original [00045e60,00045e9d). Shift the three-cycle pipeline history
   and clear the newest pair before assigning its first operation. */
void __cdecl rotate_pentium_history(void)
{
    scheduler_pairs.previous[2] = scheduler_pairs.previous[1];
    scheduler_pairs.previous[1] = scheduler_pairs.previous[0];
    scheduler_pairs.previous[0].first = 0;
    scheduler_pairs.previous[0].second = 0;
}
/* Original [0003ffd0,00040091). Track the definitions made by the
   instruction just committed to the scheduled stream. */
void __fastcall update_scheduled_registers(MachineInstruction *instruction)
{
    unsigned operation = (instruction->opcode >> 8) & 255, mask = 0, i;
    if (operation == MC_CALL) {
        record_scheduled_register(REG_EAX, instruction, 1);
        record_scheduled_register(REG_ECX, instruction, 1);
        record_scheduled_register(REG_EDX, instruction, 1);
        return;
    }
    if (machine_operand_counts[operation] > 0 && (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
        writes_first_operand(instruction))
        record_scheduled_register((unsigned short)(instruction->operands[0].value.reg.words.number & 0x3fff), instruction, 0);
    if (machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS) mask = implicit_register_mask(instruction, 2);
    for (i = 0; i < 8; ++i)
        if (mask & (1U << i)) record_scheduled_register((unsigned short)i, instruction, 1);
}
/* Original [00041e80,00041f2a). Record a physical-register definition
   and whether its symbolic value can be recovered without self-reference. */
void __fastcall record_scheduled_register(unsigned short reg,
    MachineInstruction *instruction, unsigned char clobbered)
{
    SchedulerRegisterState *state;
    MachineAddress *address;
    unsigned operation;
    if (reg >= scheduler_register_capacity) compiler_internal_error("@(#)sched.c:1.33", 0x526);
    state = scheduler_register_states[reg];
    if (!state) {
        state = arena_alloc(3, sizeof(SchedulerRegisterState));
        scheduler_register_states[reg] = state;
    }
    state->definition = instruction;
    state->clobbered = clobbered;
    operation = (instruction->opcode >> 8) & 255;
    if (operation == MC_MOV) state->recoverable = 1;
    else if (operation == MC_LEA) {
        address = instruction->operands[1].value.address;
        state->recoverable = (address->base.words.number & 0x3fff) != reg &&
            (address->index.words.number & 0x3fff) != reg;
    } else state->recoverable = 0;
}
/* Original [00043480,000434d3). A prefix-constrained operation waits
   behind a multicycle operation in either slot of the preceding pair. */
int __fastcall pentium_previous_memory_delay(MachineInstruction *instruction)
{
    if (!pentium_prefix_instruction(instruction)) return 0;
    if (scheduler_pairs.previous[0].first && (short)scheduler_instruction_cycles(scheduler_pairs.previous[0].first) > 1) return 1;
    if (scheduler_pairs.previous[0].second && (short)scheduler_instruction_cycles(scheduler_pairs.previous[0].second) > 1) return 1;
    return 0;
}
/* Original [00040f80,00041063). Address-generation dependency: an
   earlier instruction that defines this memory operand's base/index delays
   its use, with Pentium stack-transfer exceptions. */
int __fastcall instruction_pair_conflict(MachineInstruction *first, MachineInstruction *last)
{
    SchedulerMemoryAccess *memory;
    unsigned mask, operation = (last->opcode >> 8) & 255;
    if (operation == MC_CALL) return 0;
    if (!(first->record.scheduling->flags & 1)) analyze_scheduled_instruction(first);
    if (!(last->record.scheduling->flags & 1)) analyze_scheduled_instruction(last);
    memory = first->record.scheduling->memory;
    if (!memory || (memory->base_register == -1 && cpu_model != 5)) return 0;
    mask = last->record.scheduling->defines;
    if (mask & (1U << (memory->base_register & 31))) {
        if (cpu_model != 5) return 1;
        return !SCHEDULER_STACK_TRANSFER(operation);
    }
    if (memory->index_register != -1 && cpu_model == 5 &&
        (mask & (1U << (memory->index_register & 31)))) return 1;
    return 0;
}
/* Original [000452e0,00045490). Independent pairs cannot read/write the
   same defined register and must not both carry an explicit memory operand. */
int __fastcall pentium_pair_independent(MachineInstruction *first, MachineInstruction *last)
{
    InstructionSchedule *a = first->record.scheduling, *b = last->record.scheduling;
    unsigned left = (first->opcode >> 8) & 255, right = (last->opcode >> 8) & 255;
    int first_count = machine_operand_counts[left], last_count = machine_operand_counts[right];
    unsigned kind;
    int memory;
    if (!(a->flags & 1) || !(b->flags & 1) || (b->uses & a->defines) || (b->defines & a->defines)) return 0;
    if (first_count != 1 && first_count != 2) return 0;
    kind = first->operands[0].flags & 15;
    memory = kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE;
    if (first_count == 2 && !memory) {
        kind = first->operands[1].flags & 15;
        memory = kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE;
    }
    if (!memory) return 1;
    if (last_count != 1 && last_count != 2) return 0;
    kind = last->operands[0].flags & 15;
    if (kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE) return 0;
    if (last_count == 2) {
        kind = last->operands[1].flags & 15;
        if (kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE) return 0;
    }
    return 1;
}
/* Original [000319f0,00031d7b). Recombine speed-split loads/stores when
   scheduling leaves their consumer adjacent and x86 operand rules permit it. */
#define RECOMBINE_MEMORY_OPERAND(p) (((p)->flags & 15) == OPERAND_BASED || \
    ((p)->flags & 15) == OPERAND_INDEXED || ((p)->flags & 15) == OPERAND_ABSOLUTE)
/* Native opcode property0x400, expressed by the permitted operation set. */
#define RECOMBINE_MEMORY_OPERATION(op) ((op) == MC_XCHG || (op) == MC_PUSH || \
    (op) == MC_DEC || (op) == MC_INC || ((op) >= MC_NEG && (op) <= MC_XOR))
MachineInstruction *__fastcall recombine_scheduled_loads(MachineInstruction *first, MachineInstruction *last)
{
    MachineInstruction *before, *end, *next, *consumer, *store = 0, *tail;
    MachineOperand *second;
    unsigned operation, kind;
    unsigned short reg;
    int seen_operation = 0, boundary = 0, destination;
    if (!optimize_speed) return last;
    for (before = first->previous; (before->opcode & 15) == 4 || (before->opcode & 15) == 7; before = before->previous) ;
    if ((before->opcode & 15) == 1 || ((before->opcode & 15) == 0 && ((before->opcode >> 8) & 255) == MC_JCC)) boundary = 1;
    end = last->next;
    while (first != end) {
        next = first->next;
        if ((first->opcode & 15) == 0 && ((first->opcode >> 8) & 255) == MC_MOV &&
            (first->operands[0].flags & 15) == OPERAND_REGISTER &&
            ((first->opcode & 0xf0) == 0x10 || (first->opcode & 0xf0) == 0x40) &&
            RECOMBINE_MEMORY_OPERAND(&first->operands[1]) && (first->opcode & 0x400000UL) &&
            !(first->record.scheduling->flags & 0x40)) {
            seen_operation = 1;
            if (first == last) return first->previous;
            reg = first->operands[0].value.reg.words.number;
            for (consumer = next; (consumer->opcode & 15) == 4; consumer = consumer->next) ;
            operation = (consumer->opcode >> 8) & 255;
            if ((consumer->opcode & 15) != 0 || !RECOMBINE_MEMORY_OPERATION(operation) ||
                ((first->opcode ^ consumer->opcode) & 0xf0)) goto next_instruction;
            second = machine_operand_counts[operation] == 2 ? &consumer->operands[1] : 0;
            kind = consumer->operands[0].flags & 15;
            if (kind == OPERAND_REGISTER && consumer->operands[0].value.reg.words.number == reg) {
                if (second && (operand_mentions_register(second, reg) || RECOMBINE_MEMORY_OPERAND(second))) goto next_instruction;
                destination = 1;
            } else if (second && (second->flags & 15) == OPERAND_REGISTER && second->value.reg.words.number == reg &&
                kind != OPERAND_BASED && kind != OPERAND_INDEXED && kind != OPERAND_ABSOLUTE) destination = 0;
            else goto next_instruction;
            if (!replace_machine_operand(consumer, !destination, &first->operands[1], 0)) goto next_instruction;
            if (destination && writes_first_operand(consumer)) {
                if (consumer == last) return first->previous;
                for (store = consumer->next; (store->opcode & 15) == 4; store = store->next) ;
                if ((store->opcode & 15) != 0 || ((store->opcode >> 8) & 255) != MC_MOV ||
                    ((first->opcode ^ store->opcode) & 0xf0) ||
                    (store->operands[1].flags & 15) != OPERAND_REGISTER || store->operands[1].value.reg.words.number != reg ||
                    !operands_equal(&store->operands[0], &first->operands[1]) || !(store->opcode & 0x400000UL)) goto next_instruction;
            }
            if (boundary) {
                tail = destination && writes_first_operand(consumer) ? store->next : consumer->next;
                while ((tail->opcode & 15) == 4 || (tail->opcode & 15) == 7) tail = tail->next;
                operation = (tail->opcode >> 8) & 255;
                if ((tail->opcode & 15) == 0 && (operation == MC_JCC || operation == MC_JMP || operation == MC_RET)) goto next_instruction;
            }
            if (destination) consumer->operands[0] = first->operands[1];
            else consumer->operands[1] = first->operands[1];
            remove_machine_instruction(first);
            if (destination && writes_first_operand(consumer)) remove_machine_instruction(store);
            next = consumer->next;
        } else if ((first->opcode & 15) == 0) seen_operation = 1;
next_instruction:
        first = next;
        boundary = !seen_operation;
    }
    return end->previous;
}
#undef RECOMBINE_MEMORY_OPERAND
#undef RECOMBINE_MEMORY_OPERATION
/* Original [0003c980,0003c999). Call metadata records up to two x87
   results independently in bits14/15. */
int __fastcall call_float_result_count(MachineInstruction *instruction)
{
    unsigned flags = instruction->operands[1].value.call->result_flags;
    int count = (flags & 0x4000) != 0;
    if (flags & 0x8000) ++count;
    return count;
}
/* Original [00042780,00042863). Check whether changing ESP can affect
   another operation's memory reference; fixed ESP/EBP-based stack locations
   and absolute addresses have the native exceptions. */
int __fastcall scheduled_stack_overlap(MachineInstruction *first, MachineInstruction *last)
{
    unsigned operation = (first->opcode >> 8) & 255, kind;
    int count;
    MachineOperand *operand;
    MachineAddress *address;
    if ((operation != MC_MOV && operation != MC_ADD) ||
        (first->operands[0].flags & 15) != OPERAND_REGISTER) return 0;
    count = machine_operand_counts[(last->opcode >> 8) & 255];
    if (count != 1 && count != 2) return 1;
    operand = &last->operands[0];
    if (count == 2) {
        kind = last->operands[1].flags & 15;
        if (kind == OPERAND_BASED || kind == OPERAND_INDEXED || kind == OPERAND_ABSOLUTE)
            operand = &last->operands[1];
    }
    kind = operand->flags & 15;
    if (kind != OPERAND_BASED && kind != OPERAND_INDEXED && kind != OPERAND_ABSOLUTE) return 1;
    if (kind == OPERAND_ABSOLUTE) return 0;
    if (kind != OPERAND_BASED) return 1;
    address = operand->value.address;
    if (address->base.words.number == REG_ESP) return 0;
    if (address->base.words.number == REG_EBP && !(current_function->field28 & 0x10)) {
        if (address->symbol) return (long)address->symbol->field04 + address->first.displacement < 0;
        return address->first.displacement < 0;
    }
    return 1;
}
/* Original [00042260,00042305). Defer an ESP-relative floating-store
   displacement correction when a push can move across it. This native
   undo variant does not assign the owner word; retain that write behavior. */
int __fastcall rewrite_scheduled_stack(MachineInstruction *first, MachineInstruction *last)
{
    unsigned operation = (first->opcode >> 8) & 255, kind;
    SchedulerUndo *undo;
    if ((operation != MC_FST && operation != MC_FSTP) ||
        (first->operands[0].flags & 15) != OPERAND_BASED ||
        first->operands[0].value.address->base.words.number != REG_ESP) return 0;
    operation = (last->opcode >> 8) & 255;
    if (operation != MC_PUSH && operation != MC_EPUSH) return 0;
    kind = last->operands[0].flags & 15;
    if ((kind == OPERAND_BASED || kind == OPERAND_INDEXED) &&
        last->operands[0].value.address->symbol == first->operands[0].value.address->symbol) return 0;
    undo = arena_alloc(3, sizeof(SchedulerUndo));
    undo->operand = &first->operands[0];
    undo->adjustment = (last->opcode >> 4) & 15;
    undo->replacement = 0xffff;
    undo->next = scheduler_undo;
    scheduler_undo = undo;
    return 1;
}
/* Original [00044f00,00044f8c). Fill the holes left by two selected
   entries using the ready set's last two records, preserving native order. */
void __fastcall remove_pentium_ready_pair(int first, int last)
{
    int count = scheduler_ready_count;
    if (last < count - 2) {
        scheduler_pairs.ready[first] = scheduler_pairs.ready[count - 2];
        scheduler_pairs.ready[last] = scheduler_pairs.ready[count - 1];
    } else if (count - last == 2) scheduler_pairs.ready[first] = scheduler_pairs.ready[count - 1];
    else scheduler_pairs.ready[first] = scheduler_pairs.ready[count - 2];
}
/* Original [00045ea0,00045edd). Unlink the first pending adjustment
   owned by an instruction that has now been committed. Arena ownership is
   retained; the native function does not release the detached record. */
void __fastcall remove_pentium_candidate_undo(MachineInstruction *instruction, SchedulerCandidate *candidate)
{
    SchedulerUndo **link = &candidate->undo, *undo;
    while ((undo = *link) != 0) {
        if (undo->owner == instruction) {
            *link = undo->next;
            return;
        }
        link = &undo->next;
    }
}
/* Original [000434e0,000435c1). Discard candidates that another
   committed scheduling unit has already consumed. */
void __cdecl compact_pentium_candidates(void)
{
    int first = 0, last = scheduler_ready_count - 1;
    while (last >= 0 && (scheduler_pairs.ready[last].instruction->record.scheduling->flags & 0x800)) {
        --last;
        --scheduler_ready_count;
    }
    if (last == -1) scheduler_ready_count = 0;
    while (first <= last && !(scheduler_pairs.ready[first].instruction->record.scheduling->flags & 0x800)) ++first;
    while (first <= last) {
        scheduler_pairs.ready[first] = scheduler_pairs.ready[last];
        --scheduler_ready_count;
        --last;
        ++first;
        if (first <= last) {
            /* The unmarked record just copied below first bounds this scan. */
            while (scheduler_pairs.ready[last].instruction->record.scheduling->flags & 0x800) --last;
            while (first < scheduler_ready_count && !(scheduler_pairs.ready[first].instruction->record.scheduling->flags & 0x800)) ++first;
        }
    }
}
/* Original [00042cc0,00043087). Moving a candidate backward must not
   expose an address-generation dependency between its former neighbors or
   across the preceding pair. Forward movement needs no extra check here. */
int __fastcall can_schedule_pentium(MachineInstruction *instruction, int forward)
{
    MachineInstruction *previous, *next;
    unsigned flags, operation;
    if (forward == 1) return 1;
    flags = instruction->record.scheduling->flags;
    if (flags & 0x40) return 0;
    operation = (instruction->opcode >> 8) & 255;
    if (SCHEDULER_FLOAT_OPERATION(operation) || ((flags >> 8) & 6) > 2) return 0;
    previous = instruction->previous;
    while (previous && !SCHEDULABLE_KIND(previous->opcode & 15) && !SCHEDULER_BARRIER(previous))
        previous = previous->previous;
    next = instruction;
    if (!SCHEDULER_BARRIER(instruction)) {
        for (next = instruction->next; next; next = next->next)
            if (SCHEDULABLE_KIND(next->opcode & 15) || SCHEDULER_BARRIER(next)) break;
    }
    if (next) {
        if (!previous) return 1;
        if (!SCHEDULER_BARRIER(previous) && !SCHEDULER_BARRIER(next) &&
            SCHEDULABLE_KIND(previous->opcode & 15) && SCHEDULABLE_KIND(next->opcode & 15) &&
            instruction_pair_conflict(next, previous)) return 0;
    }
    if (previous && !SCHEDULER_BARRIER(previous) && (previous->record.scheduling->flags & 0x40)) {
        for (previous = previous->previous; previous; previous = previous->previous)
            if (SCHEDULABLE_KIND(previous->opcode & 15) || SCHEDULER_BARRIER(previous)) break;
        if (!SCHEDULER_BARRIER(previous) && instruction_pair_conflict(next, previous)) return 0;
    }
    return 1;
}
/* Original [0003f830,0003f83a). Release scheduler-owned heap arrays. */
void __fastcall scheduler_release(void *memory)
{
    free(memory);
}
/* Original [00042920,00042953). End the scheduling payload's lifetime
   before subsequent passes reuse the instruction record union. */
void finish_machine_schedule(void)
{
    MachineInstruction *instruction;
    for (instruction = machine_instructions; instruction; instruction = instruction->next)
        if (SCHEDULABLE_KIND(instruction->opcode & 15)) instruction->record.scheduling = 0;
}
