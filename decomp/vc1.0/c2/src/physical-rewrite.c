/* Register-value reuse after physical coloring. */
#include "physical-rewrite.h"
#include "backend-passes.h"
#include "bitsets.h"
#include "instruction-rewrite.h"
#include "register-use.h"
#include "frame.h"
#include <recon.h>
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

/* Original [0002f3f0,0002f500). A linked definition/use interval cannot
   recycle any physical color written in that interval, nor cross a label. */
unsigned __fastcall register_reuse_barriers(MachineInstruction *definition, MachineInstruction *use)
{
    MachineInstruction *limit = use->next;
    RegisterVariable *variable;
    unsigned mask = 0, kind, operation, color;
    unsigned short number;
    for (; definition != limit; definition = definition->next) {
        kind = definition->opcode & 15;
        if (kind == 1) return 0;
        if (kind != 0 && kind != 8 && kind != 9 && kind != 10) continue;
        operation = (definition->opcode >> 8) & 255;
        if (machine_operand_counts[operation] > 0 &&
            (definition->operands[0].flags & 15) == OPERAND_REGISTER &&
            writes_first_operand(definition)) {
            number = definition->operands[0].value.reg.words.number;
            if (!DIRECT_REGISTER_COLOR(number)) {
                variable = lookup_register_variable(number, 1);
                variable = lookup_register_variable(variable->representative, 1);
                color = variable->flags & 63;
                mask |= 1U << color;
            }
        }
        if (machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS)
            mask |= implicit_register_mask(definition, 2);
    }
    return mask;
}

/* Original [00030e90,00031130). Prefer equivalent cached values and
   compatible source/use colors; retain the current color as the search stop. */
void __fastcall order_reusable_registers(MachineInstruction *definition, MachineInstruction *use, unsigned short color)
{
    unsigned short first, second, use_color, swap;
    unsigned mask = 0, operation, i;
    int last = -1;
    MachineOperand *operand;
    PhysicalRegisterCache *cache;
    first = preferred_reuse_colors(definition, &second, 1);
    if (first != 26) {
        reusable_register_order[0] = first;
        last = 0;
        if (second != 26) {
            last = 1;
            if (second == color) {
                reusable_register_order[0] = color;
                reusable_register_order[1] = first;
            } else reusable_register_order[1] = second;
            mask = 1U << second;
        }
        mask |= 1U << first;
    } else {
        operation = (definition->opcode >> 8) & 255;
        for (i = 0; i < 8; ++i) {
            if ((operation == MC_MOV && MEMORY_OPERAND(&definition->operands[1])) || operation == MC_LEA) {
                operand = &definition->operands[1];
                cache = &register_value_cache[i];
                if ((cache->reference[0] && cache->operation == operation &&
                    cached_operands_equal(operand, cache->reference[0]) &&
                    !operand_value_depends_on_color(operand, (unsigned short)i)) ||
                    (cache->reference[1] && cached_operands_equal(operand, cache->reference[1]) &&
                    !operand_value_depends_on_color(operand, (unsigned short)i))) {
                    reusable_register_order[++last] = (unsigned short)i;
                    mask |= 1U << i;
                }
            }
        }
    }
    use_color = preferred_reuse_colors(use, &second, 0);
    if (use_color != 26) {
        if (last < 0) {
            last = 0;
            reusable_register_order[0] = use_color;
        } else if (last > 0 && use_color == reusable_register_order[1]) {
            swap = reusable_register_order[1];
            reusable_register_order[1] = reusable_register_order[0];
            reusable_register_order[0] = swap;
            goto complete_order;
        } else if (use_color == reusable_register_order[0]) goto complete_order;
        else reusable_register_order[++last] = use_color;
        mask |= 1U << use_color;
    }
complete_order:
    if (!(mask & (1U << color))) {
        if (latest_rewrite_register == color || previous_rewrite_register == color) {
            for (i = 0; i < 8; ++i) {
                if (i != latest_rewrite_register && i != previous_rewrite_register && !(mask & (1U << i)))
                    reusable_register_order[++last] = (unsigned short)i;
            }
        }
        if (previous_rewrite_register != 26 && !(mask & (1U << previous_rewrite_register)) &&
            previous_rewrite_register != color && latest_rewrite_register == color)
            reusable_register_order[++last] = previous_rewrite_register;
        reusable_register_order[last + 1] = color;
    }
}

/* Original [00031130,00031510). MOV relates its two register operands;
   LEA can prefer an unscaled base/index color or its destination's color.
   Pseudo27 is the absent base;26 means no preference. */
unsigned short __fastcall preferred_reuse_colors(MachineInstruction *instruction, unsigned short *second, int source)
{
    unsigned operation, kind;
    unsigned short number, index;
    RegisterVariable *variable;
    MachineAddress *address;
    *second = 26;
    operation = (instruction->opcode >> 8) & 255;
    if (operation == MC_MOV) {
        if ((instruction->operands[0].flags & 15) != OPERAND_REGISTER ||
            (instruction->operands[1].flags & 15) != OPERAND_REGISTER) return 26;
        number = instruction->operands[source ? 1 : 0].value.reg.words.number;
    } else if (operation == MC_LEA) {
        kind = instruction->operands[1].flags & 15;
        address = instruction->operands[1].value.address;
        if (kind == OPERAND_BASED) {
            number = source ? address->base.words.number : instruction->operands[0].value.reg.words.number;
            if (source && number == REG_EBP) return 26;
        } else if (kind == OPERAND_INDEXED && !address->first.displacement && !address->symbol) {
            if (!(instruction->operands[1].flags & 0x300)) {
                number = address->base.words.number;
                if (number == REG_EBP) compiler_internal_error("@(#)color.c:1.101", 0x15e1);
                if (source) {
                    index = address->index.words.number;
                    if (DIRECT_REGISTER_COLOR(index)) *second = index;
                    else {
                        variable = lookup_register_variable(index, 1);
                        variable = lookup_register_variable(variable->representative, 1);
                        *second = (unsigned short)(variable->flags & 63);
                    }
                } else number = instruction->operands[0].value.reg.words.number;
            } else if (address->base.words.number == 27) {
                number = source ? address->index.words.number : instruction->operands[0].value.reg.words.number;
            } else return 26;
        } else return 26;
    } else return 26;
    if (DIRECT_REGISTER_COLOR(number)) return number;
    variable = lookup_register_variable(number, 1);
    variable = lookup_register_variable(variable->representative, 1);
    return (unsigned short)(variable->flags & 63);
}

/* Original [00031640,00031900), including eight switch entries318d8..318f8.
   Compare cached addressing values after canonical representative coloring.
   Register27 is an absent base and must never be looked up as a variable. */
int __fastcall cached_operands_equal(MachineOperand *left, MachineOperand *right)
{
    unsigned short a, b;
    RegisterVariable *variable;
    MachineAddress *left_address, *right_address;
    if ((left->flags ^ right->flags) & 15) return 0;
    switch (left->flags & 15) {
    case OPERAND_REGISTER:
        a = left->value.reg.words.number;
        b = right->value.reg.words.number;
        if (a == b) return 1;
        if (!DIRECT_REGISTER_COLOR(a)) {
            variable = lookup_register_variable(a, 1);
            variable = lookup_register_variable(variable->representative, 1);
            a = (unsigned short)(variable->flags & 63);
        }
        if (!DIRECT_REGISTER_COLOR(b)) {
            variable = lookup_register_variable(b, 1);
            variable = lookup_register_variable(variable->representative, 1);
            b = (unsigned short)(variable->flags & 63);
        }
        return a == b;
    case OPERAND_IMMEDIATE: return left->value.constant == right->value.constant;
    case OPERAND_INDEXED:
        if ((left->flags ^ right->flags) & 0x300) return 0;
        left_address = left->value.address;
        right_address = right->value.address;
        a = left_address->index.words.number;
        b = right_address->index.words.number;
        if (a != b) {
            if (!DIRECT_REGISTER_COLOR(a)) {
                variable = lookup_register_variable(a, 1);
                variable = lookup_register_variable(variable->representative, 1);
                a = (unsigned short)(variable->flags & 63);
            }
            if (!DIRECT_REGISTER_COLOR(b)) {
                variable = lookup_register_variable(b, 1);
                variable = lookup_register_variable(variable->representative, 1);
                b = (unsigned short)(variable->flags & 63);
            }
            if (a != b) return 0;
        }
        /* fall through */
    case OPERAND_BASED:
        left_address = left->value.address;
        right_address = right->value.address;
        a = left_address->base.words.number;
        b = right_address->base.words.number;
        if (a != b) {
            if (a == 27 || b == 27) return 0;
            if (!DIRECT_REGISTER_COLOR(a)) {
                variable = lookup_register_variable(a, 1);
                variable = lookup_register_variable(variable->representative, 1);
                a = (unsigned short)(variable->flags & 63);
            }
            if (!DIRECT_REGISTER_COLOR(b)) {
                variable = lookup_register_variable(b, 1);
                variable = lookup_register_variable(variable->representative, 1);
                b = (unsigned short)(variable->flags & 63);
            }
            if (a != b) return 0;
        }
        /* fall through */
    case OPERAND_ABSOLUTE: case OPERAND_LABEL_ADDRESS:
        return left->value.address->symbol == right->value.address->symbol &&
            left->value.address->first.displacement == right->value.address->first.displacement;
    case OPERAND_SYMBOL: return left->value.symbol == right->value.symbol;
    }
    return 0;
}

/* Original [00030d20,00030e90). Test a last-use annotation against a
   physical color after resolving logical representatives. Indexed addresses
   retire index and base independently, and27 denotes an absent base. */
int __fastcall operand_ends_physical_register(MachineOperand *operand, unsigned short color)
{
    unsigned kind = operand->flags & 15;
    unsigned short number;
    RegisterVariable *variable;
    if (kind == OPERAND_REGISTER) {
        if (!(operand->flags & 0x400)) return 0;
        number = operand->value.reg.words.number;
    } else {
        if (kind == OPERAND_INDEXED) {
            if (operand->flags & 0x800) {
                number = operand->value.address->index.words.number;
                if (!DIRECT_REGISTER_COLOR(number)) {
                    variable = lookup_register_variable(number, 1);
                    variable = lookup_register_variable(variable->representative, 1);
                    number = (unsigned short)(variable->flags & 63);
                }
                if (number == color) return 1;
            }
            if (operand->value.address->base.words.number == 27) return 0;
        } else if (kind != OPERAND_BASED) return 0;
        if (!(operand->flags & 0x400)) return 0;
        number = operand->value.address->base.words.number;
    }
    if (!DIRECT_REGISTER_COLOR(number)) {
        variable = lookup_register_variable(number, 1);
        variable = lookup_register_variable(variable->representative, 1);
        number = (unsigned short)(variable->flags & 63);
    }
    return number == color;
}

/* Original [0002b330,0002b6a0). Collect colors actually written and
   frame-symbol requirements, then prune unused save/restore and frame setup
   instructions. The historical name predates identification of this role. */
void __fastcall assign_machine_registers(MachineInstruction *instructions)
{
    MachineInstruction *instruction, *next;
    MachineOperand *operand;
    RegisterVariable *variable;
    SymbolHeader *symbol;
    unsigned written = 0, kind, operation, count, number, mask;
    int locals_unused = 1, parameters_unused = 1, in_prologue = 0, remove;
    for (instruction = instructions; instruction; instruction = instruction->next) {
        kind = instruction->opcode & 15;
        operation = (instruction->opcode >> 8) & 255;
        if (kind == 2) {
            if (instruction->record.symbol->field0e & 0x1000) written |= selected_save_register_mask;
            continue;
        }
        if (kind != 0 && kind != 8 && kind != 9) continue;
        if (kind == 0 && (operation == MC_EPUSH || operation == MC_EPOP)) continue;
        count = machine_operand_counts[operation];
        if (count == 2) {
            operand = &instruction->operands[1];
            kind = operand->flags & 15;
            if ((kind == OPERAND_BASED || kind == OPERAND_INDEXED) && operand->value.address->symbol) {
                symbol = operand->value.address->symbol;
                if ((symbol->kind == 1 && (((BackendSymbol *)symbol)->attributes & 0x70) == 0x10) || symbol->kind == 19)
                    locals_unused = 0;
                else if (symbol->kind == 1 && (((BackendSymbol *)symbol)->attributes & 0x70) == 0x30)
                    parameters_unused = 0;
            }
        }
        if (count == 1 || count == 2) {
            operand = &instruction->operands[0];
            kind = operand->flags & 15;
            if (kind == OPERAND_REGISTER) {
                number = operand->value.reg.words.number;
                if (!(number > 7 && number <= 31) && number != REG_EBP && number != REG_ESP &&
                    writes_first_operand(instruction)) {
                    variable = lookup_register_variable((unsigned short)number, 1);
                    if (variable->flags & 0x800) variable = lookup_register_variable(variable->representative, 1);
                    written |= 1U << (variable->flags & 63);
                }
            } else if ((kind == OPERAND_BASED || kind == OPERAND_INDEXED) && operand->value.address->symbol) {
                symbol = operand->value.address->symbol;
                if ((symbol->kind == 1 && (((BackendSymbol *)symbol)->attributes & 0x70) == 0x10) || symbol->kind == 19)
                    locals_unused = 0;
                else if (symbol->kind == 1 && (((BackendSymbol *)symbol)->attributes & 0x70) == 0x30)
                    parameters_unused = 0;
            }
        }
        if (machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS)
            written |= implicit_register_mask(instruction, 2);
    }
    if (frame_register_allocated && !(written & selected_save_register_mask))
        compiler_internal_error("@(#)color.c:1.101", 0xb73);
    for (instruction = instructions; instruction; instruction = next) {
        next = instruction->next;
        kind = instruction->opcode & 15;
        if (kind == 2) { in_prologue = 1; continue; }
        if (kind == 7) { in_prologue = 0; continue; }
        if (kind != 0 && kind != 8 && kind != 9) continue;
        operation = (instruction->opcode >> 8) & 255;
        remove = 0;
        switch (operation) {
        case MC_MOV:
            if (in_prologue && locals_unused && parameters_unused && !stack_frame_extent &&
                (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                (instruction->operands[1].flags & 15) == OPERAND_REGISTER &&
                instruction->operands[0].value.reg.words.number == REG_EBP &&
                instruction->operands[1].value.reg.words.number == REG_ESP) remove = 1;
            break;
        case MC_SUB:
            if (in_prologue && (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                instruction->operands[0].value.reg.words.number == REG_ESP && locals_unused) remove = 1;
            break;
        case MC_LEAVE:
            if (locals_unused && parameters_unused && !stack_frame_extent) remove = 1;
            break;
        case MC_EPUSH:
            if (instruction->operands[0].value.reg.words.number == REG_EBP &&
                locals_unused && parameters_unused && !stack_frame_extent) { remove = 1; break; }
            /* fall through */
        case MC_EPOP:
            mask = 1U << instruction->operands[0].value.reg.words.number;
            if ((selected_save_register_mask & mask) && !(written & mask)) remove = 1;
            break;
        }
        if (remove) remove_machine_instruction(instruction);
    }
}
/* Original [0002fdf0,0003067a). Prefer an available cached memory value,
   then a free color. If neither exists, preserve one region register in its
   spill home and borrow its physical color across the transformed region. */
int __fastcall choose_memory_split_register(MachineInstruction *instruction, unsigned short *number)
{
    MachineOperand *memory = &instruction->operands[0];
    MachineOperand reg, home, copied_home, label;
    PhysicalRegisterCache *cache;
    PhysicalRegisterAlias *alias;
    RegisterVariable *variable;
    MachineFlowRegion *candidate, *selected = 0, *preservation;
    MachineInstruction *record, *position, *move, *end_label;
    MachineBlockHeader *entry_block = 0, *exit_block = 0;
    unsigned kind, operation, color, index;
    unsigned short saved_number = 0;
    int bracket, mark_frame = 1;
    if (!MEMORY_OPERAND(memory)) memory = &instruction->operands[1];
    for (color = 0; color < 8; ++color) {
        cache = &register_value_cache[color];
        if (((cache->reference[0] && cache->operation == MC_MOV &&
              cached_operands_equal(memory, cache->reference[0])) ||
             (cache->reference[1] && cached_operands_equal(memory, cache->reference[1]))) &&
            !(live_physical_registers & (1U << color)) &&
            (((instruction->opcode & 0xf0) > 0x10) ||
             byte_register_available((unsigned short)color))) {
            *number = (unsigned short)color;
            previous_rewrite_register = latest_rewrite_register;
            latest_rewrite_register = *number;
            if (*number == REG_EBP) *number = 24;
            return 1;
        }
    }
    if (register_value_region && (register_value_region->flags & 0x20) &&
        latest_rewrite_register != register_value_region->split_color &&
        (((instruction->opcode & 0xf0) > 0x10) ||
         byte_register_available(register_value_region->split_color))) {
        *number = register_value_region->split_color;
        mark_frame = 0;
        goto cache_value;
    }
    for (index = 0; index < 8; ++index) {
        color = next_split_register_color(index);
        if (!(live_physical_registers & (1U << (color & 31))) &&
            (((instruction->opcode & 0xf0) > 0x10) ||
             byte_register_available((unsigned short)color))) {
            *number = (unsigned short)color;
            goto cache_value;
        }
    }
    if (!register_value_region) return 0;
    for (color = 0; color < 8; ++color) {
        if ((instruction->opcode & 0xf0) <= 0x10 &&
            !byte_register_available((unsigned short)color)) continue;
        alias = physical_register_aliases[color];
        if (!alias || alias->next || !(alias->number & 0x4000)) continue;
        candidate = find_evictable_machine_region(alias->number, register_value_region);
        if (candidate && (!selected || machine_region_is_ancestor(candidate, selected))) {
            selected = candidate;
            saved_number = alias->number;
        }
    }
    if (!selected) return 0;
    initialize_operand(&reg, OPERAND_REGISTER, saved_number, 0, 0);
    operand_for_spill(&home, saved_number, 1);
    variable = lookup_register_variable(saved_number, 1);
    preservation = find_register_preservation_region(saved_number, selected->parent);
    if (!preservation) preservation = selected;
    if (preservation->saved_split_register != saved_number) {
        prepare_machine_region_transform(preservation);
        move = new_machine_record(0, MC_MOV, variable->storage_bytes, &home, &reg);
        move = insert_machine_after(preservation->first->previous, move);
        record = move->previous;
        while ((record->opcode & 15) != 7) record = record->previous;
        if (!bitset_contains(((MachineBlockHeader *)record)->definitions, saved_number & 0x3fff))
            ((MachineBlockHeader *)record)->uses = bitset_include(
                ((MachineBlockHeader *)record)->uses, saved_number & 0x3fff);
        preservation->saved_split_register = saved_number;
    }
    record = selected->last;
    operation = (record->opcode >> 8) & 255;
    if (operation != MC_JCC && operation != MC_JMP)
        compiler_internal_error("@(#)color.c:1.101", 0x16e1);
    if (operation != MC_JMP) {
        position = record->next;
        while ((position->opcode & 15) == 4) position = position->next;
        if ((position->opcode & 15) != 7 ||
            bitset_contains(((MachineBlockHeader *)position)->uses, saved_number & 0x3fff)) {
            if ((position->opcode & 15) == 7) {
                entry_block = (MachineBlockHeader *)position;
                bitset_remove(entry_block->uses, saved_number & 0x3fff);
                position = position->next;
            }
            copy_machine_operand(&home, &copied_home);
            move = new_machine_record(0, MC_MOV, variable->storage_bytes, &reg, &copied_home);
            insert_machine_after(position->previous, move);
            if (!entry_block) {
                record = position->previous;
                while ((record->opcode & 15) != 7) record = record->previous;
                entry_block = (MachineBlockHeader *)record;
            }
        }
    }
    if (selected->flags & 0x10) {
        end_label = selected->exit_label;
        position = end_label->previous;
        record = position;
        while ((record->opcode & 15) != 7) record = record->previous;
        exit_block = (MachineBlockHeader *)record;
        if ((entry_block && entry_block == exit_block) ||
            bitset_contains(exit_block->uses, saved_number & 0x3fff)) {
            kind = position->opcode & 15;
            while (kind != 10 && kind != 8 && kind != 9 && kind != 0) {
                position = position->previous;
                kind = position->opcode & 15;
            }
            if ((((position->opcode >> 8) & 255) != MC_JMP ||
                 (position->operands[0].flags & 15) != OPERAND_SYMBOL ||
                 position->operands[0].value.symbol == end_label->record.symbol) &&
                (preservation->flags & 0x10)) {
                initialize_operand(&label, OPERAND_SYMBOL, 0, 0, end_label->record.symbol);
                move = new_machine_record(0, MC_JMP, 0x104, &label);
                insert_machine_after(position, move);
                ++((LabelSymbol *)end_label->record.symbol)->references;
            }
            end_label = split_machine_region_exit(selected);
            record = end_label->previous;
            bracket = (record->opcode & 15) != 7;
            while ((record->opcode & 15) == 4) record = record->previous;
            if ((record->opcode & 15) == 7)
                bitset_remove(((MachineBlockHeader *)record)->uses, saved_number & 0x3fff);
            while ((record->opcode & 15) != 7) record = record->previous;
            exit_block = (MachineBlockHeader *)record;
            if (bracket) {
                move = new_machine_record(10, MC_MOV, 0x104, &reg, &reg);
                end_label = insert_machine_after(end_label, move);
            }
            copy_machine_operand(&home, &copied_home);
            move = new_machine_record(0, MC_MOV, 0x104, &reg, &copied_home);
            end_label = insert_machine_after(end_label, move);
            if (bracket) {
                move = new_machine_record(10, MC_MOV, 0x104, &reg, &reg);
                end_label = insert_machine_after(end_label, move);
            }
            position = end_label->next;
            while ((position->opcode & 15) == 4) position = position->next;
            if ((position->opcode & 15) != 1)
                exit_block->definitions = bitset_include(exit_block->definitions, saved_number & 0x3fff);
        }
    }
    if (DIRECT_REGISTER_COLOR(saved_number)) *number = saved_number;
    else {
        variable = lookup_register_variable(saved_number, 1);
        variable = lookup_register_variable(variable->representative, 1);
        *number = (unsigned short)(variable->flags & 63);
    }
    live_physical_registers &= ~(1U << (*number & 31));
    if (!entry_block) entry_block = exit_block;
    mark_machine_region_split(selected, *number, saved_number, 1, entry_block);
cache_value:
    previous_rewrite_register = latest_rewrite_register;
    latest_rewrite_register = *number;
    cache = &register_value_cache[*number];
    cache->reference[0] = &cache->saved[0];
    copy_machine_operand(memory, cache->reference[0]);
    cache->operation = MC_MOV;
    cache->reference[1] = 0;
    if (*number == REG_EBP) {
        *number = 24;
        if (mark_frame) frame_register_allocated = 1;
    }
    return 1;
}

/* Original7e1dc/7e1e0: state shared by successive calls in the color walk. */
static int split_color_cursor, split_color_phase;
/* Original [00031900,000319e7). Enumerate colors with no cached operands
   first, then cached colors, leaving the latest color for the final slot. */
unsigned __fastcall next_split_register_color(unsigned index)
{
    int color, phase;
    unsigned short latest = latest_rewrite_register;
    if (latest == 26) return index;
    if (index == 7) return latest;
    color = index ? split_color_cursor : -1;
    phase = index ? split_color_phase : 0;
    split_color_phase = phase;
    if (!phase) {
        split_color_cursor = color;
        for (++color; color < 8; ++color) {
            if (color != latest && !register_value_cache[color].reference[0] &&
                !register_value_cache[color].reference[1]) {
                split_color_cursor = color;
                return color;
            }
        }
        split_color_phase = 1;
        color = -1;
    }
    split_color_cursor = color;
    for (++color; color < 8; ++color) {
        if (color != latest && (register_value_cache[color].reference[0] ||
            register_value_cache[color].reference[1])) {
            split_color_cursor = color;
            return color;
        }
    }
    compiler_internal_error("@(#)color.c:1.101", 0x1768);
    return 0;
}
