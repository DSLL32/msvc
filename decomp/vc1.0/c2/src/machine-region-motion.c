/* Instruction motion inside the real structured machine-region hierarchy. */
#include "machine-region-motion.h"
#include "instruction-selection.h"
#include "register-use.h"
#include "instruction-rewrite.h"
#include "physical-rewrite.h"
#include "transformation-cursor.h"
#include "register-variables.h"
#include "temporaries.h"
#include "post-allocation.h"
#include <recon.h>
extern int cpu_model;
/* Original48b20..48b9d, ECX region/RET0. Nested regions keep their own
   instruction runs; process the intervening selected blocks and finally
   simplify the preheader that receives moved operations. */
void __fastcall transform_machine_region_blocks(MachineFlowRegion *region)
{
    MachineFlowRegion *child = region->children;
    MachineInstruction *first = region->first, *end = region->last->next;
    MachineInstruction *instruction, *boundary;
    prepare_region_insertion(region);
    boundary = child ? child->first : end;
    instruction = first;
    while (instruction != end) {
        if (instruction == boundary) {
            instruction = child->last->next;
            child = child->next;
            boundary = child ? child->first : end;
        } else if ((instruction->opcode & 15) == 7 &&
                   ((((MachineBlockHeader *)instruction)->flags & 0x200) || instruction == first)) {
            instruction = move_region_block_operations((MachineBlockHeader *)instruction, boundary);
        } else instruction = instruction->next;
    }
    finish_selected_block(region_insertion_block, 1);
}

MachineBlockHeader *region_motion_block; /* original830cc: current analyzed owner */
int region_motion_changed; /* original7c2a0: repeat block-use analysis after motion */
/* Original491d0..4922b, ECX block/EDX upper boundary/RET0. An instruction
   rewrite can invalidate its use links; rerun analysis until the block settles. */
MachineInstruction *__fastcall move_region_block_operations(MachineBlockHeader *block,
    MachineInstruction *limit)
{
    MachineInstruction *instruction, *next;
    region_motion_block = block;
    do {
        region_motion_changed = 0;
        analyze_selected_block(block);
        instruction = block->next;
        while ((instruction->opcode & 15) != 7 && instruction != limit) {
            next = instruction->next;
            if (!(instruction->opcode & 15)) next = move_region_instruction(instruction);
            instruction = next;
        }
    } while (region_motion_changed);
    return instruction;
}

/* Original49280..49ba5. Hoist invariant definitions and form region induction
   addresses without changing live values at the region's entry/exit. */
MachineInstruction *__fastcall move_region_instruction(MachineInstruction *instruction)
{
    MachineInstruction *next = instruction->next, *user = instruction->record.next_use;
    MachineInstruction *update, *scan, *created, *copy;
    MachineRegionRegister *record;
    MachineOperand saved, replacement, first, second, remaining_operand, *use;
    MachineAddress *address, *used_address;
    SymbolHeader *symbol;
    unsigned flags = instruction->opcode, operation, base_constant, index_constant;
    unsigned use_base_constant, use_index_constant, base_value, invariant_value, base_scale, index_scale;
    unsigned scale, use_scale, combined_scale, displacement, adjustment, factor;
    unsigned short number, base, index, other, use_base, use_index, old_number, moved_number;
    if (user && ((user->opcode ^ flags) & 0xff000000U)) user = 0;
    operation = (flags >> 8) & 255;
    switch (operation) {
    case MC_MOV:
        if ((instruction->operands[0].flags & 15) != OPERAND_REGISTER ||
            !((number = instruction->operands[0].value.reg.words.number) & 0xc000) ||
            (instruction->operands[1].flags & 15) != OPERAND_IMMEDIATE ||
            (flags & 0xf0) != 0x40 || !user ||
            ((user->opcode >> 8) & 255) != MC_SUB ||
            (user->operands[0].flags & 15) != OPERAND_REGISTER ||
            (flags & 0x10000) || (user->opcode & 0xf0) != 0x40 ||
            (user->operands[1].flags & 15) != OPERAND_REGISTER) break;
        other = user->operands[1].value.reg.words.number;
        record = find_machine_region_register(active_machine_region, number, 0);
        if (!record || record->definitions != 2 || !(record->flags & 2) || !(record->flags & 8)) break;
        update = find_active_region_increment(other);
        if (!update) break;
        for (scan = active_machine_region->first; scan != instruction && scan != update; scan = scan->next) ;
        if (scan != instruction) break;
        for (scan = update; scan != active_machine_region->last; scan = scan->next)
            if (instruction_mentions_register(scan, number)) break;
        if (scan != active_machine_region->last) break;
        moved_number = hoist_region_definition(instruction, 0);
        operation = ((update->opcode >> 8) & 255) == MC_ADD ? MC_ADD : MC_SUB;
        initialize_operand(&first, OPERAND_REGISTER, moved_number, 0, 0);
        initialize_operand(&second, OPERAND_IMMEDIATE, 0, update->operands[1].value.constant, 0);
        created = new_machine_record(0, operation, 0x104, &first, &second);
        created = insert_machine_after(update->previous, created);
        if (moved_number != number) {
            --record->definitions;
            adjust_region_register_uses(active_machine_region, number, -1);
            record = find_machine_region_register(active_machine_region, moved_number, 0);
            ++record->definitions;
            ++record->uses;
        }
        record->definition = created;
        record->flags |= 4;
        record->increment = ((created->opcode >> 8) & 255) == MC_ADD ?
            created->operands[1].value.constant : 0U - created->operands[1].value.constant;
        user->operands[0].value.reg.words.number = moved_number;
        ++record->definitions;
        ++record->uses;
        if (user == next) next = next->next;
        hoist_region_definition(user, 0);
        break;

    case MC_LEA:
        number = instruction->operands[0].value.reg.words.number;
        record = find_machine_region_register(active_machine_region, number, 0);
        if (!record || !(number & 0xc000) || !(record->flags & 2) ||
            record->definitions != 1 || !(record->flags & 8)) break;
        address = instruction->operands[1].value.address;
        if ((instruction->operands[1].flags & 15) == OPERAND_INDEXED) {
            base = address->base.words.number;
            index = address->index.words.number;
            scale = (instruction->operands[1].flags >> 8) & 3;
            displacement = address->first.displacement;
            symbol = address->symbol;
            base_constant = region_register_is_constant(base);
            index_constant = region_register_is_constant(index);
            if (base_constant && index_constant) {
                copy_machine_operand(&instruction->operands[1], &saved);
                moved_number = hoist_region_definition(instruction, 1);
                replace_region_operand_uses(&saved, moved_number, active_machine_region->first);
                break;
            }
            if (!base_constant && !index_constant) break;
            if (user) {
                if (base_constant) {
                    base_value = address->base.value;
                    base_scale = 0;
                    other = index;
                    index_scale = scale;
                } else {
                    base_value = address->index.value;
                    base_scale = scale;
                    other = base;
                    index_scale = 0;
                }
                use = &user->operands[(flags & 0x10000) != 0];
                initialize_operand(&remaining_operand, OPERAND_REGISTER, other, 0, 0);
                if ((use->flags & 15) != OPERAND_INDEXED ||
                    operand_changed_between(instruction, user, &remaining_operand, 0x104) ||
                    (!(flags & 0x20000) && !region_operand_replacement_safe(user, number, use)))
                    goto indexed_fallback;
                used_address = use->value.address;
                if (symbol && used_address->symbol) goto indexed_fallback;
                use_base = used_address->base.words.number;
                use_index = used_address->index.words.number;
                use_base_constant = region_register_is_constant(use_base);
                use_index_constant = region_register_is_constant(use_index);
                if (!use_base_constant && !use_index_constant) goto indexed_fallback;
                use_scale = (use->flags >> 8) & 3;
                if (use_base_constant) {
                    invariant_value = used_address->base.value;
                    adjustment = 0;
                    if (use_index != number) compiler_internal_error("@(#)range.c:1.0", 0x476);
                    if (use_scale && symbol) compiler_internal_error("@(#)range.c:1.0", 0x477);
                    factor = use_scale;
                } else {
                    invariant_value = used_address->index.value;
                    adjustment = use_scale;
                    if (use_base != number) compiler_internal_error("@(#)range.c:1.0", 0x47c);
                    factor = 0;
                }
                combined_scale = index_scale + factor;
                if ((int)combined_scale > 3) goto indexed_fallback;
                if (!symbol) symbol = used_address->symbol;
                /* Original497bd multiplies by the encoded scale count here,
                   rather than shifting; preserve its actual modulo32 formula. */
                moved_number = new_region_address_register(instruction, base_scale + factor,
                    base_value, invariant_value, adjustment, symbol,
                    displacement * factor + used_address->first.displacement);
                copy_machine_operand(use, &saved);
                /* Original497f3..49858 only writes kind/scale/width in this
                   local flag word; retain its partial initialization. Unknown
                   analysis bits are not a license to choose internal values. */
                replacement.value.address = new_machine_address();
                replacement.flags = (replacement.flags & ~0xaU) | OPERAND_INDEXED;
                replacement.value.address->base.words.number = moved_number;
                replacement.value.address->base.words.flags = 1;
                replacement.value.address->index.words.number = other;
                replacement.flags = (replacement.flags & ~0x300U) | ((combined_scale << 8) & 0x300);
                replacement.value.address->symbol = 0;
                replacement.value.address->first.displacement = 0;
                replacement.flags &= ~0xf0U;
                replace_region_operand(active_machine_region, user, &saved, &replacement);
                region_motion_changed = 1;
                remove_region_instruction(active_machine_region, instruction);
                break;
            }
indexed_fallback:
            if (base_constant) update = find_active_region_increment(index);
            else {
                update = find_active_region_increment(base);
                scale = 0;
            }
            if (!update) break;
            for (scan = active_machine_region->first; scan != instruction && scan != update; scan = scan->next) ;
            if (scan != instruction) break;
            for (scan = update; scan != active_machine_region->last; scan = scan->next)
                if (instruction_mentions_register(scan, number)) break;
            if (scan != active_machine_region->last) break;
            old_number = update->operands[0].value.reg.words.number;
            if (register_dead_after_region(old_number, active_machine_region, 0) && cpu_model >= 4) {
                scale_region_increment(old_number, update, 1, &index_constant);
                if ((instruction->operands[1].flags & 15) != OPERAND_INDEXED) break;
                scale = (instruction->operands[1].flags >> 8) & 3;
            }
            copy_machine_operand(&instruction->operands[1], &saved);
            address = instruction->operands[1].value.address;
            if (base_constant) {
                old_number = address->index.words.number;
                address->index.words.number = update->operands[0].value.reg.words.number;
            } else {
                address->base.words.number = update->operands[0].value.reg.words.number;
                old_number = address->base.words.number;
            }
            moved_number = hoist_region_definition(instruction, 1);
            if (old_number != update->operands[0].value.reg.words.number) {
                adjust_region_register_uses(active_machine_region, update->operands[0].value.reg.words.number, 1);
                adjust_region_register_uses(active_machine_region, old_number, -1);
            }
            operation = (update->opcode >> 8) & 255;
            initialize_operand(&first, OPERAND_REGISTER, moved_number, 0, 0);
            first.value.reg.words.flags = 1;
            initialize_operand(&second, OPERAND_IMMEDIATE, 0,
                update->operands[1].value.constant << scale, 0);
            created = new_machine_record(0, operation, 0x104, &first, &second);
            created = insert_machine_after(update->previous, created);
            if (moved_number != number) record = find_machine_region_register(active_machine_region, moved_number, 0);
            record->definition = created;
            ++record->definitions;
            ++record->uses;
            record->flags |= 4;
            record->increment = ((created->opcode >> 8) & 255) == MC_ADD ?
                created->operands[1].value.constant : 0U - created->operands[1].value.constant;
            record->flags &= ~1U;
            replace_region_operand_uses(&saved, moved_number, next);
        } else if ((instruction->operands[1].flags & 15) == OPERAND_BASED) {
            base = address->base.words.number;
            if (!address->first.displacement && !address->symbol) {
                /* The original generation-zero arm reads an unused local into
                   EDX. Only the nonzero arm consumes a clone in the call below. */
                if (flags >> 24) copy = clone_machine_record(instruction);
                instruction->opcode = (instruction->opcode & ~0xfe00U) | (MC_MOV << 8);
                instruction->operands[1].flags = (instruction->operands[1].flags & ~0xeU) | OPERAND_REGISTER;
                instruction->operands[1].value.reg.words.number = base;
                if (instruction->opcode >> 24)
                    replace_machine_definition(instruction->opcode >> 24, copy, instruction);
                region_motion_changed = 1;
            } else if (region_register_is_constant(base)) {
                copy_machine_operand(&instruction->operands[1], &saved);
                moved_number = hoist_region_definition(instruction, 1);
                replace_region_operand_uses(&saved, moved_number, active_machine_region->first);
            }
        }
        break;

    case MC_ADD: case MC_SUB:
        if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
            ((number = instruction->operands[0].value.reg.words.number) & 0xc000) &&
            (instruction->operands[1].flags & 15) == OPERAND_IMMEDIATE && cpu_model == 4) {
            update = find_active_region_increment(number);
            if (update && register_dead_after_region(number, active_machine_region, 0))
                scale_region_increment(number, update, 1, &base_constant);
        }
        break;
    }
    return next;
}

/* Original49fd0..4a00d, CX register/RET0. Virtual registers use the active
   region's no-definition flag; the two frame-base identifiers are fixed. */
int __fastcall region_register_is_constant(unsigned short number)
{
    MachineRegionRegister *record = find_machine_region_register(active_machine_region, number, 0);
    if (record && (number & 0xc000)) return record->flags & 1;
    return number == 27 || number == REG_EBP;
}

/* Original4a420..4a81c. Build one or two preheader LEAs for the requested
   base/index scales, retaining the instruction's deferred-generation ownership.
   The source operand's flag word has only kind/width/scale assigned in the
   original. Preserve that partial initialization; do not choose analysis bits. */
unsigned short __fastcall new_region_address_register(MachineInstruction *instruction,
    unsigned base_scale, unsigned base, unsigned index, unsigned index_scale,
    SymbolHeader *symbol, unsigned displacement)
{
    unsigned short number;
    MachineOperand destination, source;
    MachineInstruction *created, *first;
    number = reserve_region_result_register(instruction, 1);
    initialize_operand(&destination, OPERAND_REGISTER, number, 0, 0);
    source.flags = (source.flags & ~15U) | OPERAND_INDEXED;
    source.flags &= ~0xf0U;
#define APPEND_REGION_LEA() do { \
    created = new_machine_record(0, MC_LEA, 0x104, &destination, &source); \
    created = insert_machine_after(transformation_cursor.instruction, created); \
} while (0)
    if (base_scale && index_scale) {
        if (base_scale == index_scale) {
            source.value.address = new_machine_address();
            source.value.address->base.value = base;
            source.value.address->index.value = index;
            source.flags &= ~0x300U;
            source.value.address->symbol = 0;
            source.value.address->first.displacement = 0;
            APPEND_REGION_LEA();
            transformation_cursor.instruction = first = created;
            source.value.address = new_machine_address();
            source.value.address->base.words.number = 27;
            source.value.address->index.words.number = number;
            source.flags = (source.flags & ~0x300U) | ((base_scale << 8) & 0x300);
        } else {
            source.value.address = new_machine_address();
            source.value.address->base.words.number = 27;
            source.value.address->index.value = base;
            source.flags = (source.flags & ~0x300U) | ((base_scale << 8) & 0x300);
            source.value.address->symbol = 0;
            source.value.address->first.displacement = 0;
            APPEND_REGION_LEA();
            transformation_cursor.instruction = first = created;
            source.value.address = new_machine_address();
            source.value.address->base.words.number = number;
            source.value.address->index.value = index;
            source.flags = (source.flags & ~0x300U) | ((index_scale << 8) & 0x300);
        }
        goto append_final_address;
    }
    if (!base_scale) {
        source.value.address = new_machine_address();
        if (((unsigned short)base != 27 || index_scale) && (unsigned short)index != REG_EBP) {
            source.value.address->base.value = base;
            source.value.address->index.value = index;
            source.flags = (source.flags & ~0x300U) | ((index_scale << 8) & 0x300);
        } else {
            source.value.address->base.value = index;
            if ((unsigned short)base == 27)
                source.flags = (source.flags & ~15U) | OPERAND_BASED;
            else {
                source.value.address->index.value = base;
                source.flags &= ~0x300U;
            }
        }
        source.value.address->symbol = symbol;
        source.value.address->first.displacement = (long)displacement;
        APPEND_REGION_LEA();
        first = created;
    } else if ((int)base_scale <= 3) {
        source.value.address = new_machine_address();
        source.value.address->base.value = index;
        source.value.address->index.value = base;
        source.flags = (source.flags & ~0x300U) | ((base_scale << 8) & 0x300);
        source.value.address->symbol = symbol;
        source.value.address->first.displacement = (long)displacement;
        APPEND_REGION_LEA();
        first = created;
    } else {
        source.value.address = new_machine_address();
        source.value.address->base.words.number = 27;
        source.value.address->index.value = base;
        ++base_scale;
        source.flags = (source.flags & ~0x300U) | ((base_scale << 8) & 0x300);
        source.value.address->symbol = 0;
        source.value.address->first.displacement = 0;
        APPEND_REGION_LEA();
        transformation_cursor.instruction = first = created;
        source.value.address = new_machine_address();
        source.value.address->base.value = index;
        source.value.address->index.words.number = number;
        source.flags |= 0x300;
append_final_address:
        source.value.address->symbol = symbol;
        source.value.address->first.displacement = (long)displacement;
        APPEND_REGION_LEA();
    }
#undef APPEND_REGION_LEA
    transformation_cursor.instruction = created;
    if (instruction->opcode >> 24) {
        created->opcode = (created->opcode & 0xffffffU) | (instruction->opcode & 0xff000000U);
        replace_machine_definition(instruction->opcode >> 24, instruction, created);
    } else record_machine_definition_range(number, first, created);
    return number;
}

/* Original4a9c0..4ab3a. A machine-only result receives a tree-class home,
   plus an explicit copy back to its old name. Extend the real allocator's
   block interval to include the preheader before updating region summaries. */
unsigned short __fastcall reserve_region_result_register(MachineInstruction *instruction,
    unsigned short register_flags)
{
    unsigned short number = instruction->operands[0].value.reg.words.number, fresh;
    MachineOperand old_operand, new_operand;
    MachineInstruction *copy;
    MachineRegionRegister *record;
    RegisterVariable *variable;
    TemporaryRegister *home;
    int first, last, preheader;
    if (!(number & 0x4000)) {
        fresh = next_tree_register++ | 0x4000;
        home = allocate_stack_temporary(4);
        record_register_variable_range(fresh, (short)active_machine_region->first_block,
            (short)active_machine_region->last_block, 0, 0, 4, (SymbolHeader *)home, 0);
        initialize_operand(&old_operand, OPERAND_REGISTER, number, 0, 0);
        initialize_operand(&new_operand, OPERAND_REGISTER, fresh, 0, 0);
        new_operand.value.reg.words.flags = register_flags;
        copy = new_machine_record(0, MC_MOV, 0x104, &old_operand, &new_operand);
        copy = insert_machine_after(instruction->previous, copy);
        record = find_machine_region_register(active_machine_region, number, 0);
        if (record->definition != instruction)
            compiler_internal_error("@(#)range.c:1.0", 0x53b);
        record->definition = copy;
        ++record->definitions;
        record = find_machine_region_register(active_machine_region, fresh, 1);
        record->flags |= 3;
        ++record->uses;
        number = fresh;
    }
    preheader = region_insertion_block->number;
    first = active_machine_region->first_block;
    last = active_machine_region->last_block;
    if (preheader != -1) {
        if (first >= preheader) first = preheader;
        if (last <= preheader) last = preheader;
    }
    variable = lookup_register_variable(number, 1);
    if (variable->first_block != -1) {
        if (variable->first_block >= first) variable->first_block = (short)first;
        if (variable->last_block <= last) variable->last_block = (short)last;
    } else {
        variable->first_block = (short)first;
        variable->last_block = (short)last;
    }
    record_region_register(active_machine_region, number, 1, 0);
    return number;
}

/* Original4a200..4a41d. Recurse over the real child runs and replace matching
   operand bases, retaining each use's displacement and analysis flags. */
void __fastcall replace_region_operand(MachineFlowRegion *region, MachineInstruction *first,
    MachineOperand *previous, MachineOperand *replacement)
{
    MachineFlowRegion *child = region->children;
    MachineInstruction *instruction, *end = region->last->next, *boundary, *next;
    MachineInstruction *saved;
    unsigned kind, operation, changed;
    int count = 0, displacement;
    if (child && region->first != first) {
        child = 0;
        for (instruction = first->next; instruction != end; instruction = instruction->next)
            if ((instruction->opcode & 15) == 1 && instruction->labels.attachment) {
                child = instruction->labels.attachment;
                break;
            }
    }
    boundary = child ? child->first : end;
    for (instruction = region->first; instruction != end; instruction = next) {
        if (instruction == boundary) {
            replace_region_operand(child, boundary, previous, replacement);
            next = child->last->next;
            child = child->next;
            boundary = child ? child->first : end;
            continue;
        }
        next = instruction->next;
        kind = instruction->opcode & 15;
        if (kind && kind != 8 && kind != 9 && kind != 10) continue;
        operation = (instruction->opcode >> 8) & 255;
        changed = 0;
        switch (machine_operand_counts[operation]) {
        case 2:
            if (same_machine_operand_base(&instruction->operands[1], previous)) {
                if (instruction->opcode >> 24) saved = clone_machine_record(instruction);
                displacement = region_operand_displacement(&instruction->operands[1], previous);
                replace_region_operand_value(replacement, &instruction->operands[1]);
                ++count;
                if (displacement) instruction->operands[1].value.address->first.displacement += displacement;
                changed = 1;
            }
            /* fall through */
        case 1:
            if (same_machine_operand_base(&instruction->operands[0], previous)) {
                if (instruction->opcode >> 24) {
                    if (!changed) saved = clone_machine_record(instruction);
                    changed = 1;
                }
                displacement = region_operand_displacement(&instruction->operands[0], previous);
                replace_region_operand_value(replacement, &instruction->operands[0]);
                ++count;
                if (displacement) instruction->operands[0].value.address->first.displacement += displacement;
            }
            break;
        }
        if ((instruction->opcode >> 24) && changed) {
            replace_machine_definition(instruction->opcode >> 24, saved, instruction);
            release_machine_instruction(saved);
        }
    }
    if (count) {
        adjust_region_operand_uses(region, replacement, count);
        adjust_region_operand_uses(region, previous, -count);
    }
}

/* Original4b0d0..4b112. Matching bases may differ by an address addend. */
int __fastcall region_operand_displacement(MachineOperand *operand, MachineOperand *base)
{
    unsigned kind;
    if ((operand->flags ^ base->flags) & 15)
        compiler_internal_error("@(#)range.c:1.0", 0x800);
    kind = operand->flags & 15;
    if (kind >= 3 && kind <= 6)
        return (int)((unsigned)operand->value.address->first.displacement -
                     (unsigned)base->value.address->first.displacement);
    return 0;
}

/* Original4b010..4b0c3. Replace the value and its owned address, but keep
   all six independently established use/definition annotation bits at the
   destination. The source's other bits retain their original partial writes. */
void __fastcall replace_region_operand_value(MachineOperand *source, MachineOperand *destination)
{
    unsigned annotations = destination->flags, kind;
    MachineAddress *address;
    *destination = *source;
    kind = source->flags & 15;
    if ((kind >= 3 && kind <= 7) || kind == 12) {
        address = new_machine_address();
        destination->value.address = address;
        *address = *source->value.address;
    }
    destination->flags = (destination->flags & ~0xfc00U) | (annotations & 0xfc00);
}

/* Original4ae30..4ae99. Count each register carried by a value/address,
   retaining separate index and base updates for indexed forms. */
void __fastcall adjust_region_operand_uses(MachineFlowRegion *region, MachineOperand *operand, int delta)
{
    switch (operand->flags & 15) {
    case OPERAND_REGISTER:
        adjust_region_register_uses(region, operand->value.reg.words.number, delta);
        break;
    case OPERAND_INDEXED:
        adjust_region_register_uses(region, operand->value.address->index.words.number, delta);
        /* fall through */
    case OPERAND_BASED:
        adjust_region_register_uses(region, operand->value.address->base.words.number, delta);
        break;
    }
}

/* Original4a080..4a0f3. When the final regional use disappears, remove
   its definition and withdraw a tree value absent from all live boundaries. */
void __fastcall adjust_region_register_uses(MachineFlowRegion *region, unsigned short number, int delta)
{
    MachineRegionRegister *record;
    if (!(number & 0xc000)) return;
    record = find_machine_region_register(region, number, 0);
    if (!record) return;
    record->uses += delta;
    if (record->uses) return;
    if ((number & 0x4000) && register_dead_after_region(number, region, 0) &&
        !register_has_region_uses(number, region->children))
        record_region_register(region, number, 0, 0);
    if (record->definitions) remove_region_instruction(region, record->definition);
}

/* Original4a100..4a1fb. Retire a two-operand register definition together
   with every actual source/base/index use before unlinking its record. */
void __fastcall remove_region_instruction(MachineFlowRegion *region, MachineInstruction *instruction)
{
    MachineRegionRegister *record;
    MachineOperand *source;
    unsigned operation = (instruction->opcode >> 8) & 255;
    if ((instruction->operands[0].flags & 15) != OPERAND_REGISTER ||
        machine_operand_counts[operation] != 2)
        compiler_internal_error("@(#)range.c:1.0", 0x5dd);
    record = find_machine_region_register(region, instruction->operands[0].value.reg.words.number, 0);
    if (writes_first_operand(instruction)) {
        --record->definitions;
        if (record->definition == instruction) record->definition = 0;
    }
    if (!record->definitions && (record->flags & 2)) record->flags |= 1;
    if (!overwrites_first_register(instruction, 0))
        adjust_region_register_uses(region, instruction->operands[0].value.reg.words.number, -1);
    source = &instruction->operands[1];
    switch (source->flags & 15) {
    case OPERAND_REGISTER:
        if (!overwrites_second_register(instruction, 0))
            adjust_region_register_uses(region, source->value.reg.words.number, -1);
        break;
    case OPERAND_INDEXED:
        adjust_region_register_uses(region, source->value.address->index.words.number, -1);
        /* fall through */
    case OPERAND_BASED:
        adjust_region_register_uses(region, source->value.address->base.words.number, -1);
        break;
    }
    remove_machine_instruction(instruction);
}
