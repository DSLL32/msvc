#include "functions.h"
#include "function-records.h"
#include "symbol-storage.h"
#include "function-reader.h"
#include "register-sets.h"
#include "registers.h"
#include "global-optimizer.h"
#include "output-callbacks.h"
#include "optimizer.h"
#include <recon.h>

extern int c2_global_optimize, c2_exception_mode, c2_favor_speed;
void __fastcall layout_optimized_stack_frame(C2Function *function);
void __fastcall layout_unoptimized_stack_frame(C2Function *function) { __asm { int 3 } }
void finish_exception_stack_frame(void) { __asm { int 3 } }
void __fastcall layout_funclet_stack_frame(C2Function *function) { __asm { int 3 } }

void __fastcall layout_function_stack_frame(C2Function *function)
{
    C2FunctionSymbol *symbol = function->symbol;

    function->frame_layout_state = 0;
    if (!c2_global_optimize || (symbol->symbol.flags & 0x1000) ||
        ((symbol->additional_flags & 0x200) && !c2_exception_mode)) {
        layout_unoptimized_stack_frame(function);
    } else {
        layout_optimized_stack_frame(function);
        if (function->symbol->additional_flags & 0x200)
            finish_exception_stack_frame();
    }
    if (function->flags & 0x8000) layout_funclet_stack_frame(function);
}

/* Encountered20-byte block stride: only the two set-pointer columns are
 * consumed here. The producer owns allocation and the remaining12 bytes. */
typedef struct C2FrameBlock {
    C2RegisterSet *uses, *definitions;
    unsigned char pending08[12];
} C2FrameBlock;
typedef char C2FrameBlockStrideCheck[sizeof(C2FrameBlock) == 20 ? 1 : -1];
C2FunctionRegister **frame_registers; /* Original4910b4; real allocation below. */
C2FrameBlock *frame_blocks; /* Original4910b8; deferred producer-owned array. */
C2RegisterSet **frame_register_conflicts; /* Original4910c8; real allocation below. */
int frame_register_count, frame_fixed_register_count; /* 4910bc/4910b0 */
int frame_block_count; /* 4910c0 */
/* These are the same former truth-value slots, now encountered as symbols.
 * The extra-slot owner has only its first symbol pointer established here. */
C2Symbol *frame_reserved_symbol; /* Original491084. */
C2FunctionRegister *frame_extra_slot; /* Original491088; no complete pointee extent claimed. */
extern int optimizer_input_changed;
C2FunctionRegister *frame_register_chain; /* 4910ac */
C2FunctionRegister *frame_aux_registers; /* 4911b0 */
C2RegisterSet *frame_fixed_registers; /* 4910a8 */
unsigned frame_total_bytes, frame_group_count, frame_aux_count; /* 4910a4/c4/4911ac */
unsigned frame_allocated_offset; /* 49107c */
void initialize_frame_allocation(void);
void __fastcall insert_frame_register(C2FunctionRegister *previous,
    C2FunctionRegister *reg)
{
    C2FunctionRegister *next;
    if (previous) {
        next = previous->frame_next;
        if (next) next->frame_previous = reg;
        reg->frame_next = next;
        previous->frame_next = reg;
        reg->frame_previous = previous;
    } else {
        next = frame_register_chain;
        if (next) next->frame_previous = reg;
        reg->frame_next = next;
        reg->frame_previous = NULL;
        frame_register_chain = reg;
    }
}

void __fastcall register_frame_variable(C2FunctionRegister *reg, int references)
{
    C2FunctionRegister *current, *previous, *old_previous, *next;
    int width, found = 0;

    if (reg->symbol && (reg->symbol == frame_reserved_symbol ||
        (!c2_exception_mode && reg == frame_extra_slot))) return;
    width = (int)reg->value_bytes;
    previous = NULL;
    current = frame_register_chain;
    while (current && (int)current->value_bytes < width) {
        previous = current;
        current = current->frame_next;
    }
    while (current && (int)current->value_bytes == width) {
        if (current == reg) {
            reg->frame_uses = (int)((unsigned)reg->frame_uses + references);
            found = 1;
            while (previous && previous->frame_uses < reg->frame_uses &&
                   (int)previous->value_bytes == width)
                previous = previous->frame_previous;
            old_previous = reg->frame_previous;
            if (previous != old_previous) {
                next = reg->frame_next;
                if (next) next->frame_previous = old_previous;
                if (old_previous) old_previous->frame_next = next;
                insert_frame_register(previous, reg);
            }
            break;
        }
        previous = current;
        current = current->frame_next;
    }
    if (!found) {
        reg->frame_uses = references;
        reg->frame_next = NULL;
        reg->frame_previous = NULL;
        insert_frame_register(previous, reg);
        ++frame_register_count;
    }
}

void __fastcall collect_frame_registers(C2Function *function)
{
    C2BasicBlock *block;
    C2Expression *record, *operand;
    C2FunctionRegister *reg;
    C2OptimizerUse *use, *saved;
    int side;

    for (block = function->flow->last; block; block = block->previous) {
        for (record = (C2Expression *)block->records;
             record != (C2Expression *)block->last_record; record = record->next) {
            if (!(record->flags & 1)) continue;
            for (side = 0; side < 2; ++side) {
                for (operand = side ? record->tail.right : record->left;
                     operand; operand = operand->next) {
                    switch (operand->kind) {
                    case 2: case 3:
                        reg = operand->value.reg->canonical;
                        if (reg->kind == 3 || reg->kind == 4 || reg->kind == 5)
                            register_frame_variable(reg, 1);
                        break;
                    case 5: case 6:
                        reg = operand->tail.aggregate_register;
                        if (reg && (reg = reg->canonical) != NULL &&
                            (reg->kind == 3 || reg->kind == 4 || reg->kind == 5))
                            register_frame_variable(reg, 1);
                        break;
                    case 10:
                        if (record->kind != 20) break;
                        use = get_global_index_uses(operand->value.optimizer_index);
                        saved = NULL;
                        while (use) {
                            reg = use->reg;
                            if (reg->kind == 12) {
                                saved = use;
                                use = reg->optimizer_uses;
                                if (!use) {
                                    use = saved->next;
                                    goto resume_frame_uses;
                                }
                                reg = use->reg;
                            }
                            reg = reg->canonical;
                            if (reg->kind == 3 || reg->kind == 4 || reg->kind == 5)
                                register_frame_variable(reg, 1);
                            use = use->next;
                            if (!use && saved) {
                                use = saved->next;
                                saved = NULL;
                            }
resume_frame_uses:
                            ;
                        }
                        break;
                    }
                }
            }
        }
    }
}
unsigned index_frame_registers(void)
{
    C2FunctionRegister *reg;
    unsigned index = 0;
    for (reg = frame_register_chain; reg; reg = reg->frame_next) {
        frame_registers[index] = reg;
        if (reg->kind == 5) {
            add_register_to_set(frame_fixed_registers, index);
            ++frame_fixed_register_count;
        }
        reg->frame_index = index++;
        frame_total_bytes += reg->value_bytes;
    }
    return index;
}
int __fastcall can_omit_frame_pointer(C2Function *function, int available)
{
    __asm { int 3 }
    return 0;
}

void __fastcall limit_frame_register_pressure(C2Function *function, int available)
{
    /* EDX is carried unchanged through the original hot path into the cold
     * child. Its mode input must remain explicit, even though only the child
     * consumes it. The caller's available-register calculation supplies it. */
    if ((function->flags & 0x10) && !(machine_registers[6].flags & 0x10) &&
        !c2_favor_speed && !can_omit_frame_pointer(function, available))
        function->flags &= ~0x10U;
}
void __fastcall build_frame_register_conflicts(C2Function *function) { __asm { int 3 } }
void __fastcall compute_frame_register_liveness(C2Function *function) { __asm { int 3 } }
void __fastcall assign_frame_register_offsets(C2Function *function, int limited,
    unsigned count) { __asm { int 3 } }

void __fastcall layout_optimized_stack_frame(C2Function *function)
{
    unsigned count;
    int i, slots, limited;

    initialize_frame_allocation();
    collect_frame_registers(function);
    frame_registers = c2_pool_allocate(4, (unsigned)frame_register_count * 4);
    count = index_frame_registers();
    slots = !c2_exception_mode && frame_extra_slot ? 1 : 0;
    limit_frame_register_pressure(function,
        (frame_reserved_symbol != 0) + frame_register_count - frame_fixed_register_count + slots);
    if (frame_register_count != frame_fixed_register_count) {
        build_frame_register_conflicts(function);
        frame_register_conflicts = c2_pool_allocate(4, (unsigned)frame_register_count << 2);
        for (i = 0; i < frame_register_count; ++i)
            frame_register_conflicts[i] = new_register_set(6);
        compute_frame_register_liveness(function);
        limited = (function->flags & 0x10) || (function->flags & 0x300000) ? 1 : 0;
        assign_frame_register_offsets(function, limited, count);
        for (i = 0; i < frame_block_count; ++i) {
            release_optimizer_symbol_set(frame_blocks[i].definitions);
            release_optimizer_symbol_set(frame_blocks[i].uses);
        }
        for (i = 0; i < frame_register_count; ++i)
            release_optimizer_symbol_set(frame_register_conflicts[i]);
        reset_input_pool(4);
        optimizer_input_changed = 1;
    }
}

void __fastcall clear_frame_symbol_state(C2Symbol *symbol)
{
    /* Only kind1 admits the input-symbol attributes beyond the common prefix. */
    if (symbol->kind == 1 && (((C2InputSymbol *)symbol)->attributes & 0x70) == 0x10)
        symbol->flags &= ~0x800U;
}

void initialize_frame_allocation(void)
{
    C2RegisterBlock *block;
    C2FunctionRegister *reg;
    C2Symbol *symbol;

    frame_register_chain = NULL;
    frame_register_count = 0;
    frame_total_bytes = 0;
    frame_fixed_register_count = 0;
    frame_registers = NULL;
    frame_register_conflicts = NULL;
    frame_block_count = 0;
    frame_group_count = 0;
    frame_aux_count = 0;
    frame_aux_registers = NULL;
    visit_symbols(1, clear_frame_symbol_state);
    frame_allocated_offset = 0;
    if (frame_reserved_symbol) {
        symbol = frame_reserved_symbol;
        symbol->parameter_offset = allocate_local_storage((unsigned)symbol->value, 0);
        symbol->flags |= 0x800;
    }
    if (!c2_exception_mode && frame_extra_slot) {
        symbol = frame_extra_slot->symbol;
        symbol->parameter_offset = allocate_local_storage((unsigned)symbol->value, 0);
        symbol->flags |= 0x800;
    }
    for (block = first_function_register_block; block; block = block->next) {
        /* Original+4 is the used-record cursor, not the capacity end at+8. */
        for (reg = block->records; reg < block->cursor; ++reg) {
            if (reg->kind == 0x10) continue;
            reg->frame_index = -1;
            if (reg->kind == 5) {
                reg->frame_next = frame_aux_registers;
                frame_aux_registers = reg;
            }
        }
    }
    frame_fixed_registers = new_register_set(6);
}

void __fastcall finalize_stack_frame_size(C2Function *function)
{
    unsigned alignment = (unsigned)local_storage_alignment;
    function->symbol->frame_stack_bytes =
        (alignment - frame_allocated_offset - 1) & ~(alignment - 1);
}

typedef char FinalLabelStackDepthCheck[offsetof(C2InputSymbol, stack_depth) == 54 ? 1 : -1];
typedef char FinalRecordVisitedCheck[offsetof(C2Expression, control_flags) == 18 ? 1 : -1];
C2Expression *__fastcall peephole_432ff1(C2Expression *record);
extern C2Expression *stack_probe_record, *prologue_special_stack_record;
extern int prologue_has_reserved_storage;
void __fastcall propagate_final_stack_depths(C2Expression *record, int depth);
/* Original49106c is the prologue selector;491068 is the frame selector. */
extern int generate_prologue, generate_frame;
int __fastcall get_frame_register_adjustment(C2FunctionRegister *reg)
{
    C2FunctionRegister *root;
    if (reg->kind == 5) {
        if ((((C2InputSymbol *)reg->canonical->symbol)->attributes & 0x70) != 0x30)
            return 0;
    } else {
        root = reg->canonical;
        if (reg->kind == 3) {
            if (root->frame_displacement <= 0) return 0;
        } else if (reg->kind == 4) {
            if (root->symbol->parameter_offset <= 0) return 0;
        } else return 0;
    }
    if (c2_favor_speed) return 4;
    if ((current_function->flags & 0x8000) ||
        (current_function->symbol->additional_flags & 0x200)) return 0;
    return 4;
}
void __fastcall adjust_final_stack_operands(C2Expression *operands, int depth)
{
    C2FunctionRegister *reg;
    for (; operands; operands = operands->next) {
        if (operands->opcode == 0x120) reg = operands->value.reg;
        else if (operands->opcode == 0x124) reg = operands->tail.aggregate_register;
        else continue;
        /* Unknown static register payloads remain unavailable in both builds. */
        if ((unsigned long)reg >= (unsigned long)machine_registers &&
            (unsigned long)reg < (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) &&
            reg != (C2FunctionRegister *)&machine_registers[0] &&
            reg != (C2FunctionRegister *)&machine_registers[1] &&
            reg != (C2FunctionRegister *)&machine_registers[2] &&
            reg != (C2FunctionRegister *)&machine_registers[3] &&
            reg != (C2FunctionRegister *)&machine_registers[4] &&
            reg != (C2FunctionRegister *)&machine_registers[5] &&
            reg != (C2FunctionRegister *)&machine_registers[6] &&
            reg != (C2FunctionRegister *)&machine_registers[7] &&
            reg != (C2FunctionRegister *)&machine_registers[8] &&
            reg != (C2FunctionRegister *)&machine_registers[18] &&
            reg != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } }
        if ((generate_prologue && reg->kind == 5) || (generate_frame && reg->kind != 5))
            operands->stack_adjustment = (long)((unsigned)get_frame_register_adjustment(reg) + depth);
        else operands->stack_adjustment = 0;
    }
}
/* Static machine rows are partial semantic records. Do not read an unknown
 * number column merely because its reserved backing happens to be zero. */
#define STACK_KNOWN_REGISTER_NUMBER(reg) \
    do { if ((unsigned long)(reg) >= (unsigned long)machine_registers && \
        (unsigned long)(reg) <= (unsigned long)(machine_registers + C2_MACHINE_REGISTER_COUNT) && \
        (reg) != (C2FunctionRegister *)&machine_registers[0] && \
        (reg) != (C2FunctionRegister *)&machine_registers[1] && \
        (reg) != (C2FunctionRegister *)&machine_registers[2] && \
        (reg) != (C2FunctionRegister *)&machine_registers[3] && \
        (reg) != (C2FunctionRegister *)&machine_registers[4] && \
        (reg) != (C2FunctionRegister *)&machine_registers[6] && \
        (reg) != (C2FunctionRegister *)&machine_registers[7] && \
        (reg) != (C2FunctionRegister *)&machine_registers[8] && \
        (reg) != (C2FunctionRegister *)&machine_registers[31] && \
        (reg) != (C2FunctionRegister *)&machine_registers[32] && \
        (reg) != (C2FunctionRegister *)&machine_registers[66]) { __asm { int 3 } } } while (0)
int __fastcall adjust_final_stack_depth(C2Expression *record, int depth)
{
    C2Expression *output, *input, *target, *branch;
    unsigned number;

    if (!(record->flags & 1)) return depth;
    output = record->tail.right;
    adjust_final_stack_operands(output, depth);
    input = record->left;
    adjust_final_stack_operands(input, depth);
    switch (record->opcode) {
    case 1:
        if (output->kind != 1) break;
        STACK_KNOWN_REGISTER_NUMBER(output->original_register);
        if (output->original_register->number != 5 || input->kind != 1) break;
        STACK_KNOWN_REGISTER_NUMBER(input->original_register);
        if (input->original_register->number == 6) return -4;
        break;
    case 0xc: case 0xb9:
        return (int)((unsigned)depth + 4);
    case 0xd: case 0xba:
        return (int)((unsigned)depth - 4);
    case 0xe:
        if (record == stack_probe_record) {
            if (!prologue_has_reserved_storage)
                return (int)((unsigned)depth - ((C2FunctionSymbol *)current_symbol)->frame_stack_bytes);
            return (int)((unsigned)depth + frame_reserved_symbol->value -
                ((C2FunctionSymbol *)current_symbol)->frame_stack_bytes);
        }
        if (record == prologue_special_stack_record)
            return (int)((unsigned)depth - 12);
        return (int)((unsigned)depth + record->detail.aggregate_displacement);
    case 0xf: case 0x10: case 0x16: case 0x102: case 0x10a:
        if (input->kind != 4) break;
        target = ((C2InputSymbol *)input->value.symbol)->branch_record;
        if (!target) break;
        if (!target->control_flags) {
            target->declaration->stack_depth = depth;
            propagate_final_stack_depths(target, depth);
        } else if (target->declaration->stack_depth != depth) {
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\MDmisc.c", 0x653);
        }
        break;
    case 0xa7:
        return 0;
    case 0x15d:
        for (branch = record->aggregate_value; branch; branch = branch->next) {
            target = branch->value.branch_record;
            if (!target)
                c2_report_internal("E:\\utc\\src\\\\P2\\x86\\MDmisc.c", 0x65d);
            /* The original continues through this read if the internal-error
             * reporter returns, including its defective NULL-target domain. */
            if (!target->control_flags) {
                target->declaration->stack_depth = depth;
                propagate_final_stack_depths(target, depth);
            } else if (target->declaration->stack_depth != depth) {
                c2_report_internal("E:\\utc\\src\\\\P2\\x86\\MDmisc.c", 0x65f);
            }
        }
        target = (C2Expression *)record->detail.data;
        if (!target->control_flags) {
            target->declaration->stack_depth = depth;
            propagate_final_stack_depths(target, depth);
        } else if (target->declaration->stack_depth != depth) {
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\MDmisc.c", 0x667);
        }
        break;
    case 0x32: case 0x2d: case 0x2e:
        if (output->kind != 1) break;
        STACK_KNOWN_REGISTER_NUMBER(output->original_register);
        number = output->original_register->number;
        if (number != 5) break;
        if (record->opcode == 0x32)
            return (int)((unsigned)depth - input->next->integer.low);
        if (record->opcode == 0x2d)
            return (int)((unsigned)depth + input->next->integer.low);
        if (input->next->kind == 7 && input->next->integer.low == (unsigned)-8)
            return 0;
        break;
    }
    return depth;
}
void __fastcall propagate_final_stack_depths(C2Expression *record, int depth)
{
    C2Expression *next, *previous;
    int seen_instruction = 0;
    for (; record; record = next) {
        next = record->next;
        if (record->kind == 14) return;
        if (record->kind != 25) {
            depth = adjust_final_stack_depth(record, depth);
            record->control_flags = 1;
            seen_instruction = 1;
        } else {
            previous = peephole_432ff1(record);
            if (record->control_flags) return;
            if (previous && seen_instruction && (previous->opcode == 0x10 || previous->kind == 14)) return;
            record->declaration->stack_depth = depth;
            record->control_flags = 1;
        }
    }
}
void __fastcall initialize_final_stack_depths(C2Function *function)
{
    C2Expression *first = (C2Expression *)function->flow->last->records, *record;
    for (record = first; record; record = record->next) record->control_flags = 0;
    propagate_final_stack_depths(first, 0);
}

/* The switch lowering child is genuine and remains pending until reached. */
void __fastcall lower_final_switch(C2Expression *record, C2Function *function)
{ __asm { int 3 } }
void __fastcall lower_final_switch_records(C2Function *function)
{
    C2Expression *record, *next;
    C2Expression *last = (C2Expression *)function->flow->first->last_record;
    for (record = (C2Expression *)function->flow->last->records->next;
         record != last; record = next) {
        next = record->next;
        if (record->opcode == 0x15d) {
            if (record->location.line) {
                record_line_delta = record->location.line;
                record_source_line = function->source_line_base + record_line_delta;
            }
            lower_final_switch(record, function);
        }
    }
}
