#include "register-use.h"
#include "instruction-selection.h"
#include "work-prepare.h"
BitSet *block_live_registers; /* 0007e1bc */
static RegisterLifetime *register_lifetimes[16]; /* 0007cac0 */
/* Original [00022e30,00022ea0). Registers retain their full tagged 16-bit
   identity; only the low four bits select a hash bucket. */
RegisterLifetime *__fastcall lookup_register_lifetime(unsigned short reg, int create)
{
    RegisterLifetime **bucket = &register_lifetimes[reg & 15];
    RegisterLifetime *lifetime;
    for (lifetime = *bucket; lifetime; lifetime = lifetime->next)
        if (lifetime->reg == reg) return lifetime;
    if (!create) return 0;
    lifetime = arena_alloc(3, sizeof(RegisterLifetime));
    lifetime->reg = reg;
    lifetime->definition = 0;
    lifetime->previous_definition = 0;
    lifetime->last_use = 0;
    lifetime->next = *bucket;
    *bucket = lifetime;
    return lifetime;
}
/* Original [00023300,00023350). Link the previous definition to a direct
   operand use; implicit uses have index 2 and carry no direct-use link. */
void __fastcall record_register_use(unsigned short reg, MachineInstruction *instruction, int index)
{
    RegisterLifetime *lifetime = lookup_register_lifetime(reg, 1);
    MachineInstruction *definition = lifetime->definition;
    if (definition && index < 2) {
        definition->record.next_use = instruction;
        definition->opcode = (definition->opcode & ~0x10000U) | (((unsigned)index << 16) & 0x10000);
        lifetime->previous_definition = definition;
    }
    lifetime->last_use = instruction;
    lifetime->operand_index = (short)index;
    lifetime->definition = 0;
}
const unsigned char machine_selection_properties[MC_OPCODE_COUNT] = {
#define OP(name,count,properties) properties,
#include "machine-opcodes.def"
#undef OP
};
/* Original [00022ea0,00022fc0). Mark the final use and any now-dead
   definition. Fixed frame/stack and noninteger physical registers are retained. */
void __fastcall finish_register_lifetime(RegisterLifetime *lifetime)
{
    unsigned short reg = lifetime->reg;
    short index;
    MachineInstruction *use, *previous, *definition;
    MachineOperand *operand;
    unsigned opcode;
    if ((reg > 7 && reg < 32) || reg == REG_EBP || reg == REG_ESP) return;
    use = lifetime->last_use;
    if (use) {
        index = lifetime->operand_index;
        if (index == 2) use->operands[1].flags |= 1U << (reg & 31);
        else {
            operand = &use->operands[index == 0 ? 0 : 1];
            if ((operand->flags & 15) == OPERAND_INDEXED && operand->value.address->index_register == reg)
                operand->flags |= 0x800;
            else operand->flags |= 0x400;
        }
        previous = lifetime->previous_definition;
        if (previous && previous->record.next_use == use &&
            ((previous->opcode >> 16) & 1) == (int)index) previous->opcode |= 0x20000;
        if (use != lifetime->definition) goto clear_use;
    }
    definition = lifetime->definition;
    if (!instruction_has_side_effects(definition)) definition->opcode |= 0x20000;
    else {
        opcode = (definition->opcode >> 8) & 255;
        if (machine_selection_properties[opcode] & MC_IMPLICIT_REGISTERS)
            definition->operands[1].flags |= 1U << (reg & 31);
    }
clear_use:
    lifetime->last_use = 0;
}
/* Original [00023630,00023670). Equal-register arithmetic can consume
   neither old value; use the first-operand overwrite test for both operands. */
int __fastcall overwrites_second_register(MachineInstruction *instruction, int check_live)
{
    unsigned opcode = (instruction->opcode >> 8) & 255;
    if ((opcode == MC_SBB || opcode == MC_SUB || opcode == MC_XOR) &&
        (instruction->operands[1].flags & 15) == OPERAND_REGISTER &&
        (instruction->operands[0].flags & 15) == OPERAND_REGISTER)
        return overwrites_first_register(instruction, check_live);
    return 0;
}
/* Original [000229d0,00022a40). These operations read their explicit first
   operand, or write only implicit registers. OR of equal operands is an
   identity and does not establish a new definition. */
int __fastcall writes_first_operand(MachineInstruction *instruction)
{
    unsigned opcode = (instruction->opcode >> 8) & 255;
    if (opcode == MC_IMUL || opcode == MC_OUT ||
        (opcode >= MC_PUSH && opcode <= MC_JMP) || opcode == MC_DIV ||
        opcode == MC_IDIV || opcode == MC_MUL || opcode == MC_CMP ||
        opcode == MC_TEST || opcode == MC_EPUSH) return 0;
    if (opcode == MC_OR) return !operands_equal(&instruction->operands[0], &instruction->operands[1]);
    return 1;
}
/* Original [00023350,000233c0). An indexed address reads index, then base. */
void __fastcall record_operand_uses(MachineInstruction *instruction, MachineOperand *operand, int index)
{
    unsigned kind = operand->flags & 15;
    if (kind == OPERAND_REGISTER)
        record_register_use((unsigned short)operand->value.constant, instruction, index);
    else if (kind == OPERAND_BASED || kind == OPERAND_INDEXED) {
        if (kind == OPERAND_INDEXED)
            record_register_use(operand->value.address->index_register, instruction, index);
        record_register_use(operand->value.address->base_register, instruction, index);
    }
}
/* Original [00023750,00023830). Identity moves and identity LEAs retain
   the old register value. Equal-operand subtract/borrow/XOR ignore that value. */
int __fastcall register_result_ignores_old_value(MachineInstruction *instruction)
{
    unsigned opcode = (instruction->opcode >> 8) & 255;
    MachineOperand *left = &instruction->operands[0], *right = &instruction->operands[1];
    MachineAddress *address;
    if (!machine_operand_counts[opcode] || (left->flags & 15) != OPERAND_REGISTER) return 0;
    if (opcode == MC_MOV) {
        if ((right->flags & 15) == OPERAND_REGISTER &&
            (unsigned short)left->value.constant == (unsigned short)right->value.constant) return 0;
        return 1;
    }
    if (opcode == MC_LEA) {
        if ((right->flags & 15) == OPERAND_BASED) {
            address = right->value.address;
            if ((unsigned short)left->value.constant == address->base_register &&
                !address->first.displacement && !address->symbol) return 0;
        }
        return 1;
    }
    if (opcode == MC_SBB || opcode == MC_SUB || opcode == MC_XOR)
        return operands_equal(left, right);
    if (opcode == MC_FSTSW || opcode == MC_FNSTSW || opcode == MC_SCC ||
        (opcode >= MC_MOVZX && opcode <= MC_MOVSXW)) return 1;
    return 0;
}
/* Original [00023490,00023630). Determine whether the first operand's
   definition covers its prior value, accounting for live registers and
   partial-byte operations before and after register allocation. */
int __fastcall overwrites_first_register(MachineInstruction *instruction, int check_live)
{
    unsigned short reg = (unsigned short)instruction->operands[0].value.constant;
    unsigned flags = instruction->opcode, width, byte_lane, opcode;
    RegisterLifetime *lifetime;
    MachineInstruction *definition;
    opcode = (flags >> 8) & 255;
    if ((flags & 0x80000) || (machine_operand_counts[opcode] > 1 && (flags & 0x100000))) {
        if (!(reg & 0xc000) || backend_stage < 7 || !register_partial_write_safe(reg)) return 0;
    }
    if (!register_result_ignores_old_value(instruction)) return 0;
    if (!check_live) return 1;
    if (!(reg & 0x8000) && block_live_registers && bitset_contains(block_live_registers, reg & 0x3fff)) {
        if (reg & 0x4000) {
            width = register_storage_bytes((unsigned short)instruction->operands[0].value.constant);
            if (width > ((instruction->opcode >> 4) & 15)) return 0;
        } else if ((instruction->opcode & 0xf0) < 0x40) return 0;
    }
    if ((reg & 0x4000) && (unsigned)register_storage_bytes(reg) == ((instruction->opcode >> 4) & 15))
        return 1;
    lifetime = lookup_register_lifetime(reg, 0);
    if (lifetime) {
        definition = lifetime->definition;
        if (!definition) definition = lifetime->previous_definition;
        if (!definition) return 1;
        width = register_definition_width(definition, reg, &byte_lane);
        flags = instruction->opcode;
        if (((flags >> 4) & 15) < width ||
            ((flags & 0xf0) == 0x10 && ((flags >> 19) & 1) != byte_lane)) {
            if ((reg & 0xc000) && backend_stage >= 7 && register_partial_write_safe(reg)) return 1;
            return 0;
        }
    }
    return 1;
}
/* Original [00022dd0,00022e30). Keep live definitions; finalize the other
   lifetimes before discarding the per-block arena and hash chains. */
void __fastcall finish_block_registers(BitSet *live)
{
    RegisterLifetime *lifetime;
    unsigned bucket;
    for (bucket = 0; bucket < 16; ++bucket) {
        for (lifetime = register_lifetimes[bucket]; lifetime; lifetime = lifetime->next) {
            if ((lifetime->reg & 0x8000) || !live || !bitset_contains(live, lifetime->reg & 0x3fff))
                finish_register_lifetime(lifetime);
        }
        register_lifetimes[bucket] = 0;
    }
    reset_arena(3);
}
/* Original [00021490,00021790). Establish register-use/definition links for
   one selected block. Operand flags below are per-analysis annotations. */
void __fastcall analyze_selected_block(MachineBlockHeader *block)
{
    MachineInstruction *instruction;
    MachineOperand *operand;
    RegisterLifetime *lifetime;
    unsigned kind, opcode, flags, width, byte_lane;
    unsigned uses, definitions, pending, bit;
    unsigned short reg;
    int count;
    block_live_registers = block->uses;
    for (instruction = block->next; (instruction->opcode & 15) != 7;
         instruction = instruction->next) {
        kind = instruction->opcode & 15;
        if (kind == 0 || kind == 8) {
            instruction->record.next_use = 0;
            instruction->opcode &= ~0x60000U;
            opcode = (instruction->opcode >> 8) & 255;
            if (machine_selection_properties[opcode] & MC_IMPLICIT_REGISTERS)
                instruction->operands[1].flags = 0;
            count = machine_operand_counts[opcode];
            if (count == 2) {
                operand = &instruction->operands[1];
                operand->flags &= ~0x7c00U;
                if (!overwrites_second_register(instruction, 1))
                    record_operand_uses(instruction, operand, 1);
            }
            if (count == 1 || count == 2) {
                operand = &instruction->operands[0];
                operand->flags &= ~0x7c00U;
                if ((operand->flags & 15) == OPERAND_REGISTER) {
                    if (overwrites_first_register(instruction, 1)) {
                        instruction->opcode |= 0x40000;
                        lifetime = lookup_register_lifetime((unsigned short)operand->value.constant, 0);
                        if (lifetime) finish_register_lifetime(lifetime);
                        goto record_definition;
                    }
                    flags = instruction->opcode;
                    if ((flags & 0xf0) < 0x40 && overwrites_first_register(instruction, 0)) {
                        reg = (unsigned short)operand->value.constant;
                        lifetime = lookup_register_lifetime(reg, 0);
                        if (lifetime && lifetime->definition) {
                            width = register_definition_width(lifetime->definition, reg, &byte_lane);
                            if (width <= ((instruction->opcode >> 4) & 15) &&
                                ((instruction->opcode >> 19) & 1) == byte_lane) {
                                finish_register_lifetime(lifetime);
                                goto record_definition;
                            }
                        }
                    }
                }
                record_operand_uses(instruction, operand, 0);
record_definition:
                if ((operand->flags & 15) == OPERAND_REGISTER && writes_first_operand(instruction)) {
                    lifetime = lookup_register_lifetime((unsigned short)operand->value.constant, 1);
                    lifetime->definition = instruction;
                    lifetime->previous_definition = 0;
                }
            }
            opcode = (instruction->opcode >> 8) & 255;
            if (machine_selection_properties[opcode] & MC_IMPLICIT_REGISTERS) {
                uses = implicit_register_mask(instruction, 1);
                pending = uses;
                for (reg = 0; pending && reg < 32; ++reg) {
                    bit = 1U << reg;
                    if (uses & bit) {
                        record_register_use(reg, instruction, 2);
                        pending &= ~bit;
                    }
                }
                definitions = implicit_register_mask(instruction, 2);
                pending = definitions;
                for (reg = 0; pending && reg < 32; ++reg) {
                    bit = 1U << reg;
                    if (definitions & bit) {
                        lifetime = lookup_register_lifetime(reg, 0);
                        if (lifetime && !(uses & bit)) finish_register_lifetime(lifetime);
                        lifetime = lookup_register_lifetime(reg, 1);
                        lifetime->definition = instruction;
                        lifetime->previous_definition = 0;
                        pending &= ~bit;
                    }
                }
            }
        } else if (kind == 9 || kind == 10) {
            if (kind == 9) instruction->operands[1].flags &= ~0x400U;
            reg = (unsigned short)instruction->operands[1].value.constant;
            lifetime = lookup_register_lifetime(reg, 1);
            lifetime->last_use = instruction;
            lifetime->operand_index = 1;
            lifetime->definition = 0;
        }
    }
    finish_block_registers(block->definitions);
    block_live_registers = 0;
}
