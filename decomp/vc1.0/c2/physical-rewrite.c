/* Register-value reuse after physical coloring. */
#include "physical-rewrite.h"
#include "backend-passes.h"
#include "bitsets.h"
#include "instruction-rewrite.h"
#include "register-use.h"
extern int optimize_speed, optimization_recycle;
#define DIRECT_REGISTER_COLOR(n) ((((n) > 7 && (n) <= 31) || (n) == REG_ESP || (n) == REG_EBP) && (n) != 24)
#define MEMORY_OPERAND(p) (((p)->flags & 15) == OPERAND_ABSOLUTE || \
    ((p)->flags & 15) == OPERAND_BASED || ((p)->flags & 15) == OPERAND_INDEXED)

PhysicalRegisterCache register_value_cache[8];
unsigned short reusable_register_order[8];
PhysicalRegisterAlias *physical_register_aliases[8];
MachineFlowRegion *register_value_region;
unsigned prior_live_physical_registers;
unsigned live_physical_registers;
unsigned short previous_rewrite_register, latest_rewrite_register;

/* Original [0002b6a0,0002b6d0). Rebuild region boundaries around the
   colored stream, then compact labels only if rewriting invalidated them. */
void optimize_allocated_machine_regions(void)
{
    analyze_machine_instructions(machine_instructions, 0);
    rewrite_register_regions(machine_region_root);
    if (machine_analysis_needs_label_cleanup) compact_machine_context(machine_context);
}

/* Original [0002f690,0002f850). Visit child regions first and process the
   parent's uncovered blocks. Cache references/live aliases belong to a block;
   held operand payloads are deliberately not blanket-cleared between blocks. */
void __fastcall rewrite_register_regions(MachineFlowRegion *regions)
{
    MachineFlowRegion *child;
    MachineInstruction *instruction, *end, *limit, *new_end;
    MachineBlockHeader *block;
    int member;
    unsigned i;
    unsigned short number;
    for (; regions; regions = regions->next) {
        rewrite_register_regions(regions->children);
        register_value_region = (regions->flags & 1) ? 0 : regions;
        child = regions->children;
        end = regions->last->next;
        limit = child ? child->first : end;
        instruction = regions->first;
        while (instruction != end) {
            if (instruction == limit) {
                instruction = child->last->next;
                child = child->next;
                limit = child ? child->first : regions->last->next;
            } else if ((instruction->opcode & 15) == 7 &&
                !(((MachineBlockHeader *)instruction)->flags & 3)) {
                block = (MachineBlockHeader *)instruction;
                analyze_selected_block(block);
                live_physical_registers = 0;
                previous_rewrite_register = latest_rewrite_register = 26;
                for (i = 0; i < 8; ++i) physical_register_aliases[i] = 0;
                for (i = 0; i < 8; ++i) {
                    register_value_cache[i].reference[0] = 0;
                    register_value_cache[i].reference[1] = 0;
                }
                if (block->uses) {
                    for (member = next_bitset_member(block->uses); member != -1;
                         member = next_bitset_member(0)) {
                        number = (unsigned short)member;
                        if (member > 31) number |= 0x4000;
                        add_physical_register_alias(number);
                    }
                }
                live_physical_registers |= 1U << REG_ESP;
                if (!frame_register_available) live_physical_registers |= 1U << REG_EBP;
                instruction = rewrite_register_block(block, limit);
                new_end = regions->last->next;
                if (end != new_end && end != instruction) {
                    limit = end ? new_end : end;
                    end = new_end;
                }
                reset_arena(3);
            } else instruction = instruction->next;
        }
    }
}

/* Original [0002f850,0002fb80), including11 dispatch entries2fb4c..2fb78.
   Split selected memory operations through a free physical register when
   speed is preferred; inserted moves carry the original byte-use markers. */
MachineInstruction *__fastcall rewrite_register_block(MachineBlockHeader *block, MachineInstruction *limit)
{
    MachineInstruction *instruction, *next, *result, *move;
    MachineOperand *memory, *second, register_operand, saved_operand;
    unsigned operation, width, kind, byte_use, saved_kind;
    unsigned short number;
    int disabled = 0, split_memory, first;
    instruction = block->next;
    while (instruction && instruction != limit) {
        next = instruction->next;
        kind = instruction->opcode & 15;
        switch (kind) {
        case 0: case 8: case 9: case 10:
            instruction->opcode &= ~0x400000U;
            if (!disabled) {
                prior_live_physical_registers = live_physical_registers;
                record_written_register_aliases(instruction);
                reuse_physical_register_values(instruction);
            }
            result = instruction;
            operation = (instruction->opcode >> 8) & 255;
            width = (instruction->opcode >> 4) & 15;
            /* Native property0x400 at82210..823de means these operations
               permit the memory-to-register speed transformation. Recover
               the encountered semantic set, independently of normalized
               machine_selection_properties (whose low eight bits differ). */
            split_memory = 0;
            switch (operation) {
            case MC_XCHG: case MC_PUSH: case MC_DEC: case MC_INC:
            case MC_NEG: case MC_NOT: case MC_RCL: case MC_RCR:
            case MC_ROL: case MC_ROR: case MC_SAR: case MC_SHL:
            case MC_SHR: case MC_ADC: case MC_ADD: case MC_AND:
            case MC_CMP: case MC_OR: case MC_SBB: case MC_SUB:
            case MC_TEST: case MC_XOR: split_memory = 1; break;
            }
            if (optimize_speed && split_memory && (width == 4 || width == 1)) {
                memory = &instruction->operands[0];
                second = machine_operand_counts[operation] == 2 ? &instruction->operands[1] : 0;
                first = MEMORY_OPERAND(memory);
                if ((first || (second && MEMORY_OPERAND(second))) &&
                    choose_memory_split_register(instruction, &number)) {
                    if (!first) memory = second;
                    byte_use = (instruction->opcode >> (first ? 19 : 20)) & 1;
                    initialize_operand(&register_operand, OPERAND_REGISTER, number, 0, 0);
                    copy_machine_operand(memory, &saved_operand);
                    move = new_machine_record(0, MC_MOV, width, &register_operand, &saved_operand);
                    move = insert_machine_after(instruction->previous, move);
                    move->operands[0].flags &= ~0x400U;
                    move->opcode = (move->opcode & ~0x180000U) | (byte_use << 20) |
                        (byte_use << 19) | 0x400000U;
                    if (first && writes_first_operand(instruction)) {
                        saved_kind = move->operands[1].flags & 15;
                        if (saved_kind == OPERAND_INDEXED) move->operands[1].flags &= ~0x800U;
                        if (saved_kind == OPERAND_REGISTER || saved_kind == OPERAND_BASED ||
                            saved_kind == OPERAND_INDEXED) move->operands[1].flags &= ~0x400U;
                        copy_machine_operand(memory, &saved_operand);
                        move = new_machine_record(0, MC_MOV, width, memory, &register_operand);
                        move = insert_machine_after(instruction, move);
                        move->operands[1].flags |= 0x400;
                        move->opcode = (move->opcode & ~0x180000U) | (byte_use << 20) |
                            (byte_use << 19) | 0x400000U;
                    }
                    result = clone_machine_record(instruction);
                    memory->flags = (memory->flags & ~14U) | OPERAND_REGISTER;
                    memory->value.reg.words.number = number;
                    if (!first || !writes_first_operand(instruction)) memory->flags |= 0x400;
                }
            }
            if (!disabled) retire_physical_register_aliases(result);
            break;
        case 1:
            previous_rewrite_register = latest_rewrite_register = 26;
            break;
        case 2: disabled = 1; break;
        case 7: return instruction;
        }
        instruction = next;
    }
    return instruction;
}

/* Original [00030bd0,00030c50). Include explicit and implicit definitions
   in the live physical aliases before attempting value substitutions. */
void __fastcall record_written_register_aliases(MachineInstruction *instruction)
{
    unsigned operation, mask, number;
    operation = (instruction->opcode >> 8) & 255;
    if (machine_operand_counts[operation] > 0 &&
        (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
        writes_first_operand(instruction))
        add_physical_register_alias(instruction->operands[0].value.reg.words.number);
    if (machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS) {
        mask = implicit_register_mask(instruction, 2);
        for (number = 0; number < 31; ++number)
            if (mask & (1U << number)) add_physical_register_alias((unsigned short)number);
    }
}

/* Original [0002fb80,0002fc50). Each physical color owns the logical
   register numbers currently represented by its value. Nodes last one block. */
void __fastcall add_physical_register_alias(unsigned short number)
{
    RegisterVariable *variable;
    PhysicalRegisterAlias **bucket, *alias;
    unsigned short color;
    if (number != 24 && ((number > 7 && number <= 31) ||
        number == REG_ESP || number == REG_EBP)) return;
    if (((number > 7 && number <= 31) || number == REG_ESP ||
        number == REG_EBP) && number != 24) color = number;
    else {
        variable = lookup_register_variable(number, 1);
        variable = lookup_register_variable(variable->representative, 1);
        color = (unsigned short)(variable->flags & 63);
    }
    live_physical_registers |= 1U << color;
    bucket = &physical_register_aliases[color];
    for (alias = *bucket; alias && alias->number != number; alias = alias->next) ;
    if (alias) return;
    alias = arena_alloc(3, sizeof(PhysicalRegisterAlias));
    alias->next = *bucket;
    alias->number = number;
    *bucket = alias;
}

/* Original [00030680,00030bd0). Recycle a definition's physical color
   when its linked use permits it, and remember values loaded from memory.
   CALL's write mask is confined to the eight integer colors (initially7,
   or the explicit EAX/ECX/EDX/ESI/EDI calling-convention mask). */
void __fastcall reuse_physical_register_values(MachineInstruction *instruction)
{
    MachineInstruction *use;
    MachineOperand *operand;
    PhysicalRegisterCache *cache;
    RegisterVariable *variable;
    PhysicalRegisterAlias *alias;
    unsigned operation, mask, color, candidate, i, width;
    unsigned short number, replacement;
    int changed, candidate_available;
    if (!optimization_recycle) return;
    operation = (instruction->opcode >> 8) & 255;
    if (machine_operand_counts[operation] > 0 &&
        (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
        (instruction->operands[0].value.reg.words.number & 0xc000) &&
        writes_first_operand(instruction)) {
        number = instruction->operands[0].value.reg.words.number;
        changed = 0;
        if (DIRECT_REGISTER_COLOR(number)) color = number;
        else {
            variable = lookup_register_variable(number, 1);
            variable = lookup_register_variable(variable->representative, 1);
            color = variable->flags & 63;
        }
        width = (instruction->opcode >> 4) & 15;
        if (latest_rewrite_register != 26 && !(instruction->operands[0].flags & 0x400) &&
            (instruction->opcode & 0x20000) && instruction->record.next_use &&
            ((width == 4 && overwrites_first_register(instruction, 0)) ||
             !(prior_live_physical_registers & (1U << color)))) {
            use = instruction->record.next_use;
            mask = register_reuse_barriers(instruction, use);
            if (mask) {
                order_reusable_registers(instruction, use, (unsigned short)color);
                for (i = 0; i < 8; ++i) {
                    candidate = reusable_register_order[i];
                    if (candidate == color) goto keep_color;
                    candidate_available = !(live_physical_registers & (1U << candidate));
                    if (!candidate_available && machine_operand_counts[operation] == 2 &&
                        operand_ends_physical_register(&instruction->operands[1], (unsigned short)candidate)) {
                        alias = physical_register_aliases[candidate];
                        candidate_available = alias && !alias->next;
                    }
                    if (candidate_available && !(mask & (1U << candidate))) {
                        if (width > 1) {
                            operand = &use->operands[(instruction->opcode & 0x10000) ? 1 : 0];
                            if (((use->opcode >> 4) & 15) > 1 ||
                                (operand->flags & 15) != OPERAND_REGISTER) break;
                        }
                        if (byte_register_available((unsigned short)candidate)) break;
                    }
                }
                if (i >= 8) compiler_internal_error("@(#)color.c:1.101", 0x14e7);
                if (machine_operand_counts[operation] < 2 ||
                    !operand_mentions_register(&instruction->operands[1], number))
                    remove_physical_register_alias(number);
                replacement = reusable_register_order[i];
                if (replacement == REG_EBP) replacement = 24;
                if (overwrites_second_register(instruction, 0)) {
                    if ((instruction->operands[1].flags & 15) != OPERAND_REGISTER)
                        compiler_internal_error("@(#)color.c:1.101", 0x14ef);
                    instruction->operands[1].value.reg.words.number = replacement;
                }
                instruction->operands[0].value.reg.words.number = replacement;
                operand = &use->operands[(instruction->opcode & 0x10000) ? 1 : 0];
                switch (operand->flags & 15) {
                case OPERAND_REGISTER: operand->value.reg.words.number = replacement; break;
                case OPERAND_INDEXED:
                    if (operand->value.address->index.words.number == number)
                        operand->value.address->index.words.number = replacement;
                    /* fall through */
                case OPERAND_BASED:
                    if (operand->value.address->base.words.number == number)
                        operand->value.address->base.words.number = replacement;
                    break;
                }
                if (reusable_register_order[i] == REG_EBP) frame_register_allocated = 1;
                add_physical_register_alias(replacement);
                previous_rewrite_register = latest_rewrite_register;
                latest_rewrite_register = reusable_register_order[i];
                changed = 1;
            }
        }
keep_color:
        if (!changed) {
            previous_rewrite_register = latest_rewrite_register;
            if (DIRECT_REGISTER_COLOR(number)) latest_rewrite_register = number;
            else {
                variable = lookup_register_variable(number, 1);
                variable = lookup_register_variable(variable->representative, 1);
                latest_rewrite_register = (unsigned short)(variable->flags & 63);
            }
        }
        operation = (instruction->opcode >> 8) & 255;
        if ((operation == MC_MOV && MEMORY_OPERAND(&instruction->operands[1])) || operation == MC_LEA) {
            cache = &register_value_cache[latest_rewrite_register];
            if (!cache->reference[0] || cache->operation != operation ||
                !operands_equal(&instruction->operands[1], cache->reference[0])) {
                cache->reference[0] = &cache->saved[0];
                copy_machine_operand(&instruction->operands[1], &cache->saved[0]);
                cache = &register_value_cache[latest_rewrite_register];
                cache->operation = (instruction->opcode >> 8) & 255;
                cache->reference[1] = 0;
            }
        }
        return;
    }
    if (operation == MC_MOV && MEMORY_OPERAND(&instruction->operands[0]) &&
        (instruction->operands[1].flags & 15) == OPERAND_REGISTER) {
        number = instruction->operands[1].value.reg.words.number;
        if (DIRECT_REGISTER_COLOR(number)) color = number;
        else {
            variable = lookup_register_variable(number, 1);
            variable = lookup_register_variable(variable->representative, 1);
            color = variable->flags & 63;
        }
        cache = &register_value_cache[color];
        cache->reference[1] = &cache->saved[1];
        copy_machine_operand(&instruction->operands[0], &cache->saved[1]);
        return;
    }
    if (operation == MC_CALL) {
        mask = implicit_register_mask(instruction, 2);
        if (mask & (1U << previous_rewrite_register)) previous_rewrite_register = 0;
        if (mask & (1U << latest_rewrite_register)) latest_rewrite_register = previous_rewrite_register;
        for (i = 0; i < 31; ++i) {
            if (mask & (1U << i)) {
                register_value_cache[i].reference[0] = 0;
                register_value_cache[i].reference[1] = 0;
            }
        }
    }
}

/* Original [00049f80,00049fd0). Preserve all operand annotations while
   giving address-bearing kinds their own mutable16-byte address object.
   Encountered bit0 of the14-entry native operand-kind properties82490..824c8:
   kinds3..7 and12 own an address, also established by initialize_operand. */
void __fastcall copy_machine_operand(MachineOperand *source, MachineOperand *destination)
{
    unsigned kind;
    MachineAddress *address;
    *destination = *source;
    kind = source->flags & 15;
    if ((kind >= 3 && kind <= 7) || kind == 12) {
        address = new_machine_address();
        destination->value.address = address;
        *address = *source->value.address;
    }
}

/* Original [0002fc50,0002fdf0). Retire aliases on last uses, preserving
   explicit/implicit definitions until their corresponding lifetime ends. */
void __fastcall retire_physical_register_aliases(MachineInstruction *instruction)
{
    unsigned operation, mask = 0, number, kind;
    unsigned short definition = 26;
    MachineOperand *operand;
    int i;
    operation = (instruction->opcode >> 8) & 255;
    if (machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS)
        mask = implicit_register_mask(instruction, 2);
    if (machine_operand_counts[operation] > 0 &&
        (instruction->operands[0].flags & 15) == OPERAND_REGISTER && writes_first_operand(instruction))
        definition = instruction->operands[0].value.reg.words.number;
    if ((instruction->opcode & 15) == 10) {
        if (instruction->operands[1].flags & 0x400)
            remove_physical_register_alias(instruction->operands[1].value.reg.words.number);
        return;
    }
    for (i = machine_operand_counts[operation]; i > 0; --i) {
        operand = &instruction->operands[i == 1 ? 0 : 1];
        kind = operand->flags & 15;
        if (kind == OPERAND_REGISTER) {
            number = operand->value.reg.words.number;
            if ((operand->flags & 0x400) && number != definition &&
                ((number & 0xc000) || !(mask & (1U << number))))
                remove_physical_register_alias((unsigned short)number);
        } else {
            if (kind == OPERAND_INDEXED) {
                number = operand->value.address->index.words.number;
                if ((operand->flags & 0x800) && number != definition &&
                    ((number & 0xc000) || !(mask & (1U << number))))
                    remove_physical_register_alias((unsigned short)number);
                if (operand->value.address->base.words.number == 27) continue;
            }
            if (kind == OPERAND_BASED || kind == OPERAND_INDEXED) {
                number = operand->value.address->base.words.number;
                if ((operand->flags & 0x400) && number != definition &&
                    ((number & 0xc000) || !(mask & (1U << number))))
                    remove_physical_register_alias((unsigned short)number);
            }
        }
    }
    if (machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS) {
        mask = instruction->operands[1].flags;
        for (number = 0; number < 31; ++number)
            if (mask & (1U << number)) remove_physical_register_alias((unsigned short)number);
    }
}

/* Original [00030c50,00030d20). Remove a logical lifetime from its
   physical color; only an empty bucket makes that color available again. */
void __fastcall remove_physical_register_alias(unsigned short number)
{
    RegisterVariable *variable;
    PhysicalRegisterAlias **bucket, **link;
    unsigned short color;
    if (number != 24 && ((number > 7 && number <= 31) ||
        number == REG_ESP || number == REG_EBP)) return;
    if (DIRECT_REGISTER_COLOR(number)) color = number;
    else {
        variable = lookup_register_variable(number, 1);
        variable = lookup_register_variable(variable->representative, 1);
        color = (unsigned short)(variable->flags & 63);
    }
    bucket = &physical_register_aliases[color];
    if (!*bucket) compiler_internal_error("@(#)color.c:1.101", 0x166e);
    for (link = bucket; (*link)->number != number; link = &(*link)->next) ;
    *link = (*link)->next;
    if (!*bucket) live_physical_registers &= ~(1U << color);
}
