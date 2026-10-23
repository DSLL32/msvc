#include "instruction-rewrite.h"
#include "register-use.h"
#include "tree-rewrite.h"
#include "algebra.h"
#include "register-variables.h"
#include <recon.h>
extern int optimize_speed, cpu_model, optimization_risc;

#define MACHINE_OP(i) (((i)->opcode >> 8) & 255)
#define MACHINE_BYTES(i) (((i)->opcode >> 4) & 15)
#define OPERAND_KIND(o) ((o)->flags & 15)
#define OPERAND_REG(o) ((o)->value.reg.words.number)
#define USER_SECOND 0x10000U
#define DEAD_RESULT 0x20000U
#define HIGH_FIRST 0x80000U
#define HIGH_SECOND 0x100000U
#define LAST_USE 0x400U
#define LOW_BYTE_ZERO 0x1000U
#define HIGH_BYTE_ZERO 0x2000U
#define HIGH_WORD_ZERO 0x4000U
#define SET_OPERATION(i,op) ((i)->opcode = ((i)->opcode & ~0xff00U) | ((op) << 8))
#define SET_BYTES(i,n) ((i)->opcode = ((i)->opcode & ~0xf0U) | ((n) << 4))
/* Exchange operations while retaining the stream's links and use chains. */
#define SWAP_OPERATIONS(a,b) do { \
    unsigned saved_opcode = (a)->opcode; \
    MachineOperand saved_operand = (a)->operands[0]; \
    (a)->opcode = (b)->opcode; (b)->opcode = saved_opcode; \
    (a)->operands[0] = (b)->operands[0]; (b)->operands[0] = saved_operand; \
    saved_operand = (a)->operands[1]; \
    (a)->operands[1] = (b)->operands[1]; (b)->operands[1] = saved_operand; \
} while (0)
#define SWAP_INSTRUCTION_DATA(a,b) do { \
    MachineInstruction *saved_use = (a)->record.next_use; \
    SWAP_OPERATIONS(a,b); \
    (a)->record.next_use = (b)->record.next_use; (b)->record.next_use = saved_use; \
} while (0)

/* Original [0001dd80,00021490). Rewrite one selected instruction, using
   the register definition/use links established by analyze_selected_block. */
MachineInstruction *__fastcall finish_machine_instruction(MachineInstruction *instruction, int mode)
{
    MachineInstruction *next = instruction->next;
    MachineInstruction *user = instruction->record.next_use;
    MachineInstruction *use, *following, *first, *second, *copy;
    MachineOperand *operand, *source, *other;
    MachineAddress *address, *source_address;
    SymbolHeader *symbol;
    unsigned flags, opcode, kind, user_flags, user_opcode, user_index;
    unsigned bytes, user_bytes, value, mask, amount, high;
    unsigned short reg, other_reg;
    long displacement;
    if (!user) {
        if (instruction->opcode & DEAD_RESULT) goto remove_instruction;
    } else if (mode && ((user->opcode ^ instruction->opcode) & 0xff000000)) user = 0;
    flags = instruction->opcode;
    opcode = MACHINE_OP(instruction);
    switch (opcode) {
    case MC_CMP:
        if (!find_flags_use(instruction)) goto remove_instruction;
        if (eliminate_redundant_compare(instruction, &next)) return next;
        user = instruction->next;
        user_flags = user->opcode;
        if ((user_flags & 15) != 0) return next;
        if (OPERAND_KIND(&instruction->operands[0]) == OPERAND_REGISTER &&
            OPERAND_KIND(&instruction->operands[1]) == OPERAND_IMMEDIATE &&
            !instruction->operands[1].value.constant && MACHINE_OP(user) == MC_JCC) {
            kind = user->operands[1].flags & 15;
            if (kind == 2) {
                if (next == user) next = user->next;
                adjust_machine_symbol_references(user, -1);
                remove_machine_instruction(user);
            } else if (kind == 3) {
                SET_OPERATION(user, MC_JMP);
                SET_BYTES(user, 4);
            } else if (kind == 6 || kind == 7) {
                user->operands[1].flags = (user->operands[1].flags & ~15U) | (kind == 6 ? 4 : 5);
                tree_changed = 1;
                goto narrow_compare;
            } else goto narrow_compare;
            goto remove_instruction;
        }
        goto narrow_compare;
narrow_compare:
        if (!mode || (MACHINE_OP(user) != MC_JCC && MACHINE_OP(user) != MC_SCC)) return next;
        operand = &instruction->operands[0];
        other = &instruction->operands[1];
        if ((operand->flags & HIGH_BYTE_ZERO) && MACHINE_BYTES(instruction) > 1) {
            if ((OPERAND_KIND(other) != OPERAND_IMMEDIATE || (other->value.constant & ~255U)) &&
                !(other->flags & HIGH_BYTE_ZERO)) return next;
            SET_BYTES(instruction, 1);
            user->operands[1].flags = (user->operands[1].flags & ~15U) |
                (narrow_comparison_condition(user->operands[1].flags & 15) & 15);
            tree_changed = 1;
        } else if ((operand->flags & LOW_BYTE_ZERO) && MACHINE_BYTES(instruction) > 1) {
            bytes = MACHINE_BYTES(instruction);
            if (!(operand->flags & HIGH_WORD_ZERO) && bytes != 2) return next;
            if (OPERAND_KIND(other) != OPERAND_IMMEDIATE || (other->value.constant & 0xffff00ff)) {
                if (!(other->flags & HIGH_WORD_ZERO) && bytes != 2) return next;
                if (!(other->flags & LOW_BYTE_ZERO)) return next;
            }
            instruction->opcode |= HIGH_FIRST | HIGH_SECOND;
            SET_BYTES(instruction, 1);
            user->operands[1].flags = (user->operands[1].flags & ~15U) |
                (narrow_comparison_condition(user->operands[1].flags & 15) & 15);
            tree_changed = 1;
        }
        return next;

    case MC_SHL:
        operand = &instruction->operands[0];
        other = &instruction->operands[1];
        if (OPERAND_KIND(operand) == OPERAND_REGISTER && OPERAND_KIND(other) == OPERAND_IMMEDIATE) {
            amount = other->value.constant;
            if (!amount) goto remove_instruction;
            if ((long)amount < 4) {
                SET_OPERATION(instruction, MC_LEA);
                address = new_machine_address();
                other->value.address = address;
                other->flags = (other->flags & ~15U) | OPERAND_INDEXED;
                address->base.words.number = 27;
                address->index.value = operand->value.constant;
                other->flags = (other->flags & ~0x300U) | ((amount << 8) & 0x300);
                tree_changed = 1;
                return next;
            }
        }
        if (!user) return next;
        kind = OPERAND_KIND(other);
        if (kind == OPERAND_IMMEDIATE && other->value.constant == 8) {
            user_flags = user->opcode;
            user_opcode = MACHINE_OP(user);
            use = user->record.next_use;
            if (user_opcode == MC_AND && OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE &&
                user->operands[1].value.constant == 0xff00 && use && MACHINE_OP(use) == MC_MOV &&
                MACHINE_BYTES(use) == 1 && !(use->opcode & HIGH_FIRST) && !(user_flags & USER_SECOND)) {
                user->operands[1].value.constant = 255;
                SET_BYTES(instruction, 2);
                SWAP_OPERATIONS(instruction, user);
                tree_changed = 1;
                return next;
            }
            if (user_opcode == MC_OR && (flags & USER_SECOND) &&
                (user->operands[0].flags & HIGH_BYTE_ZERO) &&
                OPERAND_KIND(&user->operands[1]) == OPERAND_REGISTER &&
                (MACHINE_BYTES(user) == 2 || (MACHINE_BYTES(user) == 4 && (operand->flags & HIGH_BYTE_ZERO))) &&
                (mode || (byte_register_available(OPERAND_REG(&instruction->operands[0])) &&
                          byte_register_available(OPERAND_REG(&user->operands[0]))))) {
                SET_OPERATION(user, MC_MOV);
                SET_BYTES(user, 1);
                user->opcode |= HIGH_FIRST;
                goto remove_instruction;
            }
            if (MACHINE_BYTES(instruction) == 2 ||
                (MACHINE_BYTES(instruction) == 4 && (operand->flags & HIGH_BYTE_ZERO))) {
                if (!(flags & USER_SECOND)) {
                    if (user_opcode == MC_MOV && MACHINE_BYTES(user) == 1 && !(user_flags & HIGH_FIRST)) {
                        SET_OPERATION(instruction, MC_MOV);
                        SET_BYTES(instruction, 1);
                        instruction->operands[1] = instruction->operands[0];
                        instruction->opcode = (instruction->opcode & ~HIGH_SECOND) | HIGH_FIRST;
                        tree_changed = 1;
                        return next;
                    }
                } else if (user_opcode == MC_MOV && MACHINE_BYTES(user) == 1 && (user_flags & HIGH_SECOND)) {
                    user->opcode &= ~HIGH_SECOND;
                    goto remove_instruction;
                }
            }
            user_index = (flags & USER_SECOND) != 0;
            source = &user->operands[user_index];
            if (OPERAND_KIND(source) == OPERAND_REGISTER && machine_operand_bytes(user, user_index) == 1 &&
                ((flags & DEAD_RESULT) || (!user_index && OPERAND_REG(&user->operands[0]) == OPERAND_REG(operand) &&
                                         (user->operands[0].flags & LAST_USE)))) {
                high = (user_flags >> (user_index ? 20 : 19)) & 1;
                if (high) {
                    SET_OPERATION(instruction, MC_MOV);
                    SET_BYTES(instruction, 1);
                    instruction->operands[1] = instruction->operands[0];
                    instruction->opcode |= HIGH_FIRST;
                    tree_changed = 1;
                    return next;
                }
            }
        }
        source = &user->operands[(flags & USER_SECOND) != 0];
        if (OPERAND_KIND(source) != OPERAND_REGISTER) return next;
        if (kind == OPERAND_IMMEDIATE && (long)other->value.constant > 7) source->flags |= LOW_BYTE_ZERO;
        if (operand->flags & HIGH_WORD_ZERO) {
            if (MACHINE_BYTES(instruction) > 3) {
                if (!(operand->flags & HIGH_BYTE_ZERO) || kind != OPERAND_IMMEDIATE || (long)other->value.constant > 8)
                    return next;
            }
            source->flags |= HIGH_WORD_ZERO;
        }
        return next;

    case MC_SAR:
        other = &instruction->operands[1];
        if (OPERAND_KIND(other) != OPERAND_IMMEDIATE) return next;
        if (user && MACHINE_OP(user) == MC_AND && OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE &&
            (long)user->operands[1].value.constant <=
                (long)(1U << ((integer_width_bits(MACHINE_BYTES(instruction)) - other->value.constant) & 31))) {
            SET_OPERATION(instruction, MC_SHR);
            tree_changed = 1;
            return next;
        }
        if (other->value.constant != 8 || !user) return next;
        user_index = (flags & USER_SECOND) != 0;
        source = &user->operands[user_index];
        if (OPERAND_KIND(source) != OPERAND_REGISTER || machine_operand_bytes(user, user_index) != 1) return next;
        high = (user->opcode >> (user_index ? 20 : 19)) & 1;
        if (high) return next;
        if (!(flags & DEAD_RESULT) &&
            (user_index || OPERAND_REG(&user->operands[0]) != OPERAND_REG(&instruction->operands[0]) ||
             !(user->operands[0].flags & LAST_USE))) return next;
        SET_OPERATION(instruction, MC_MOV);
        SET_BYTES(instruction, 1);
        instruction->operands[1] = instruction->operands[0];
        instruction->opcode |= HIGH_SECOND;
        tree_changed = 1;
        return next;

    case MC_SHR:
        if (!user) return next;
        operand = &instruction->operands[0];
        other = &instruction->operands[1];
        kind = OPERAND_KIND(other);
        if (kind == OPERAND_IMMEDIATE) {
            if (!other->value.constant) goto remove_instruction;
            if (other->value.constant == 8) {
                if ((OPERAND_KIND(&user->operands[0]) == OPERAND_BASED ||
                     OPERAND_KIND(&user->operands[0]) == OPERAND_INDEXED) &&
                    operand_mentions_register(&user->operands[0], OPERAND_REG(operand))) return next;
                if (machine_operand_counts[MACHINE_OP(user)] > 1 &&
                    (OPERAND_KIND(&user->operands[1]) == OPERAND_BASED ||
                     OPERAND_KIND(&user->operands[1]) == OPERAND_INDEXED) &&
                    operand_mentions_register(&user->operands[1], OPERAND_REG(operand))) return next;
                user_flags = user->opcode;
                user_opcode = MACHINE_OP(user);
                use = user->record.next_use;
                if (user_opcode == MC_AND && OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE &&
                    user->operands[1].value.constant == 255 && use && MACHINE_OP(use) == MC_MOV &&
                    MACHINE_BYTES(use) == 1 && (use->opcode & HIGH_FIRST) && !(user_flags & USER_SECOND)) {
                    user->operands[1].value.constant = 0xff00;
                    SET_BYTES(instruction, 2);
                    SWAP_OPERATIONS(instruction, user);
                    tree_changed = 1;
                    return next;
                }
                if (user_opcode == MC_OR && (flags & USER_SECOND) && MACHINE_BYTES(instruction) == 2 &&
                    MACHINE_BYTES(user) == 2 && (user->operands[0].flags & LOW_BYTE_ZERO) &&
                    OPERAND_KIND(&user->operands[1]) == OPERAND_REGISTER &&
                    (mode || (byte_register_available(OPERAND_REG(&user->operands[0])) &&
                              byte_register_available(OPERAND_REG(&user->operands[1]))))) {
                    SET_OPERATION(user, MC_MOV);
                    SET_BYTES(user, 1);
                    user->opcode = (user->opcode & ~HIGH_FIRST) | HIGH_SECOND;
                    goto remove_instruction;
                }
                if (MACHINE_BYTES(instruction) == 2 ||
                    (MACHINE_BYTES(instruction) == 4 && (operand->flags & HIGH_WORD_ZERO))) {
                    if (!(flags & USER_SECOND) || OPERAND_KIND(&user->operands[1]) != OPERAND_REGISTER) {
                        if (user_opcode == MC_MOV && MACHINE_BYTES(user) == 1 &&
                            OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER && (user_flags & HIGH_FIRST)) {
                            SET_OPERATION(instruction, MC_MOV);
                            SET_BYTES(instruction, 1);
                            instruction->operands[1] = instruction->operands[0];
                            instruction->opcode = (instruction->opcode & ~HIGH_FIRST) | HIGH_SECOND;
                            tree_changed = 1;
                            return next;
                        }
                    } else if (user_opcode == MC_MOV && MACHINE_BYTES(user) == 1 && !(user_flags & HIGH_SECOND)) {
                        user->opcode |= HIGH_SECOND;
                        goto remove_instruction;
                    }
                }
                if ((flags & DEAD_RESULT) && MACHINE_BYTES(instruction) > 1) {
                    user_index = (flags & USER_SECOND) != 0;
                    source = &user->operands[user_index];
                    if (OPERAND_KIND(source) == OPERAND_REGISTER &&
                        !((user_flags >> (user_index ? 20 : 19)) & 1) &&
                        machine_operand_bytes(user, user_index) == 1) {
                        SET_OPERATION(instruction, MC_MOV);
                        SET_BYTES(instruction, 1);
                        instruction->operands[1] = instruction->operands[0];
                        instruction->opcode |= HIGH_SECOND;
                        tree_changed = 1;
                        return next;
                    }
                }
            }
        }
        source = &user->operands[(flags & USER_SECOND) != 0];
        if (OPERAND_KIND(source) != OPERAND_REGISTER) return next;
        if (kind == OPERAND_IMMEDIATE && (long)other->value.constant > 7) {
            if (MACHINE_BYTES(instruction) == 2 ||
                (MACHINE_BYTES(instruction) == 4 && (operand->flags & HIGH_WORD_ZERO)))
                source->flags |= HIGH_BYTE_ZERO;
        }
        if ((operand->flags & HIGH_WORD_ZERO) ||
            (MACHINE_BYTES(instruction) == 4 && kind == OPERAND_IMMEDIATE && (long)other->value.constant >= 16))
            source->flags |= HIGH_WORD_ZERO;
        return next;

    case MC_AND:
        operand = &instruction->operands[0];
        other = &instruction->operands[1];
        if (user && OPERAND_KIND(other) == OPERAND_IMMEDIATE) {
            value = other->value.constant;
            bytes = MACHINE_BYTES(instruction);
            mask = integer_width_mask(bytes);
            if (flags & HIGH_SECOND) mask <<= 8;
            if ((mask & value) == mask) goto remove_instruction;
            if (bytes > 1 || !(flags & HIGH_FIRST)) {
                user_flags = user->opcode;
                user_opcode = MACHINE_OP(user);
                if ((user_opcode == MC_TEST || user_opcode == MC_AND) && !(flags & USER_SECOND) &&
                    OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER &&
                    OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE) {
                    amount = user->operands[1].value.constant;
                    user->operands[1].value.constant = (~mask | value) & amount;
                    if (user_opcode != MC_TEST || (flags & DEAD_RESULT)) {
                        SET_OPERATION(user, MC_AND);
                        goto remove_instruction;
                    }
                    if (amount != user->operands[1].value.constant) {
                        tree_changed = 1;
                        return next;
                    }
                } else if (user_opcode == MC_SAR && !(flags & USER_SECOND) &&
                    OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER &&
                    !(value & integer_sign_bit(MACHINE_BYTES(user)))) {
                    SET_OPERATION(user, MC_SHR);
                    tree_changed = 1;
                    return next;
                } else if ((user_opcode == MC_MOVZX || user_opcode == MC_MOVZXW) &&
                    OPERAND_KIND(&user->operands[1]) == OPERAND_REGISTER && !(user_flags & HIGH_SECOND)) {
                    other->value.constant = mask & value;
                    SET_OPERATION(user, MC_MOV);
                    SET_BYTES(instruction, MACHINE_BYTES(user));
                    tree_changed = 1;
                    return next;
                } else if ((user_opcode == MC_MOVSX || user_opcode == MC_MOVSXW) &&
                    OPERAND_KIND(&user->operands[1]) == OPERAND_REGISTER) {
                    user_bytes = machine_operand_bytes(user, 1);
                    if (user_bytes <= bytes && !(value & integer_sign_bit(user_bytes))) {
                        SET_OPERATION(user, MC_MOV);
                        SET_BYTES(instruction, MACHINE_BYTES(user));
                        tree_changed = 1;
                        return next;
                    }
                }
            }
            if (value == 255) {
                if (!(flags & USER_SECOND)) {
                    if (MACHINE_OP(user) == MC_SHL &&
                        OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE &&
                        user->operands[1].value.constant == 8) {
                        user->operands[0].flags |= HIGH_WORD_ZERO;
                        use = user->record.next_use;
                        if (use && MACHINE_OP(use) == MC_MOV && (user->opcode & USER_SECOND)) {
                            other->value.constant = 0xff00;
                            SWAP_OPERATIONS(instruction, user);
                            tree_changed = 1;
                            return next;
                        }
                    }
                } else if (MACHINE_OP(user) == MC_OR && (user->operands[0].flags & LOW_BYTE_ZERO) &&
                    OPERAND_KIND(&user->operands[1]) == OPERAND_REGISTER &&
                    (mode || (byte_register_available(OPERAND_REG(&user->operands[0])) &&
                              byte_register_available(OPERAND_REG(&user->operands[1]))))) {
                    SET_OPERATION(user, MC_MOV);
                    SET_BYTES(user, 1);
                    goto remove_instruction;
                }
            }
            user_index = (flags & USER_SECOND) != 0;
            source = &user->operands[user_index];
            if ((value & 255) == 255 && OPERAND_KIND(source) == OPERAND_REGISTER) {
                user_flags = user->opcode;
                if ((MACHINE_OP(user) == MC_MOV && MACHINE_BYTES(user) == 1 &&
                     (((user_flags & HIGH_FIRST) && bytes == 2 && !user_index) ||
                      (!(user_flags & HIGH_SECOND) && user_index &&
                       ((flags & DEAD_RESULT) || operands_equal(&user->operands[0], &user->operands[1]))))) ||
                    (MACHINE_BYTES(user) == 1 && (flags & DEAD_RESULT))) goto remove_instruction;
            }
            if (value == 0xff00 || (bytes == 2 && (value & 0xffff) == 0xff00)) {
                if (!user_index) {
                    if (MACHINE_OP(user) == MC_SHR &&
                        OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE &&
                        user->operands[1].value.constant == 8) {
                        if (MACHINE_BYTES(user) == 4) user->operands[0].flags |= HIGH_WORD_ZERO;
                        use = user->record.next_use;
                        if (!use || MACHINE_OP(use) != MC_MOV) return next;
                        if (user->opcode & USER_SECOND) {
                            other->value.constant = 255;
                            SWAP_OPERATIONS(instruction, user);
                            tree_changed = 1;
                        }
                        return next;
                    }
                } else if (MACHINE_OP(user) == MC_OR && (user->operands[0].flags & HIGH_BYTE_ZERO) &&
                    OPERAND_KIND(&user->operands[1]) == OPERAND_REGISTER &&
                    (mode || (byte_register_available(OPERAND_REG(&user->operands[0])) &&
                              byte_register_available(OPERAND_REG(&user->operands[1]))))) {
                    SET_OPERATION(user, MC_MOV);
                    SET_BYTES(user, 1);
                    user->opcode |= HIGH_FIRST | HIGH_SECOND;
                    tree_changed = 1;
                    return next;
                }
            }
            if ((value & 0xff00) == 0xff00 && MACHINE_OP(user) == MC_MOV && MACHINE_BYTES(user) == 1 &&
                ((!(user->opcode & HIGH_FIRST) && bytes == 2 && !user_index) ||
                 ((user->opcode & HIGH_SECOND) && user_index &&
                  ((flags & DEAD_RESULT) || operands_equal(&user->operands[0], &user->operands[1])))))
                goto remove_instruction;
            if (bytes > 1 && (flags & DEAD_RESULT) && mode &&
                OPERAND_KIND(source) == OPERAND_REGISTER && machine_operand_bytes(user, user_index) == 1) {
                high = (user->opcode >> (user_index ? 20 : 19)) & 1;
                SET_BYTES(instruction, 1);
                if (high) instruction->opcode |= HIGH_FIRST | HIGH_SECOND;
                tree_changed = 1;
                return next;
            }
            if (OPERAND_KIND(source) == OPERAND_REGISTER) {
                if (value < 256) source->flags |= HIGH_BYTE_ZERO;
                else if (!(value & 255)) source->flags |= LOW_BYTE_ZERO;
                if (!(value & 0xffff0000) && bytes == 4) source->flags |= HIGH_WORD_ZERO;
            }
        }
        if (OPERAND_KIND(operand) != OPERAND_REGISTER) return next;
        if (OPERAND_KIND(other) == OPERAND_IMMEDIATE) {
            value = other->value.constant;
            if (operand->flags & LAST_USE) {
                if (value == 255 || value == 0xff00) {
                    SET_OPERATION(instruction, MC_CMP);
                    SET_BYTES(instruction, 1);
                    other->value.constant = 0;
                    if (value == 0xff00) instruction->opcode |= HIGH_FIRST;
                    tree_changed = 1;
                    return next;
                }
            }
            if (user && (flags & DEAD_RESULT) && !(value & (value - 1)) && MACHINE_OP(user) == MC_CMP &&
                operands_equal(other, &user->operands[1]) && (use = user->next) != 0 &&
                MACHINE_OP(use) == MC_JCC &&
                ((use->operands[1].flags & 15) == 4 || (use->operands[1].flags & 15) == 5) &&
                !find_flags_use(instruction)) {
                SET_OPERATION(user, MC_TEST);
                use->operands[1].flags = (use->operands[1].flags & ~15U) |
                    ((use->operands[1].flags & 15) == 4 ? 5 : 4);
                goto remove_instruction;
            }
        }
        if (operand->flags & LAST_USE) {
            SET_OPERATION(instruction, MC_TEST);
            tree_changed = 1;
        }
        return next;

    case MC_MOV:
        operand = &instruction->operands[0];
        other = &instruction->operands[1];
        if ((MACHINE_BYTES(instruction) > 1 || !(((flags >> 19) ^ (flags >> 20)) & 1)) &&
            operands_equal(operand, other)) goto remove_instruction;
        if (!mode && (next->opcode & 15) == 0 && MACHINE_OP(next) == MC_MOV &&
            OPERAND_KIND(operand) != OPERAND_REGISTER && operands_equal(operand, &next->operands[0]) &&
            MACHINE_BYTES(instruction) <= MACHINE_BYTES(next) &&
            !((instruction->opcode ^ next->opcode) & HIGH_FIRST) && !(operand->flags & 0x8000))
            goto remove_instruction;
        if (!user) return next;
        if ((!(instruction->opcode & DEAD_RESULT) && OPERAND_KIND(other) != OPERAND_IMMEDIATE) ||
            operand_changed_between(instruction, user, operand, MACHINE_BYTES(instruction)) ||
            operand_changed_between(instruction, user, other, MACHINE_BYTES(instruction))) goto move_fallback;
        user_index = (instruction->opcode & USER_SECOND) != 0;
        source = &user->operands[user_index];
        kind = OPERAND_KIND(other);
        if (kind == OPERAND_REGISTER &&
            ((other->value.reg.words.flags && register_source_changed_between(instruction, user, OPERAND_REG(other))) ||
             register_replacement_blocked(instruction, 0, other->value.constant))) goto move_followup;
        if (OPERAND_KIND(source) == OPERAND_REGISTER) {
            /* Some 486/Pentium scheduling choices retain a separate load. */
            if ((kind == 3 || kind == OPERAND_BASED || kind == OPERAND_INDEXED) && optimize_speed &&
                ((MACHINE_OP(user) == MC_PUSH && (cpu_model == 4 || (cpu_model == 5 && !optimization_risc))) ||
                 (!mode && cpu_model == 5))) goto move_followup;
            if (!user_index && writes_first_operand(user)) goto move_followup;
            bytes = MACHINE_BYTES(instruction);
            user_bytes = machine_operand_bytes(user, user_index);
            if (user_bytes > bytes) {
                if (!(instruction->opcode & 0x40000) || kind == 3) goto move_followup;
                if (kind == OPERAND_BASED || kind == OPERAND_INDEXED) {
                    symbol = other->value.address->symbol;
                    if (!symbol) goto move_followup;
                    if (symbol->kind == 1) {
                        value = ((BackendSymbol *)symbol)->attributes & 0x70;
                        if (value != 0x10 && value != 0x30) goto move_followup;
                    } else if (symbol->kind != 0x13) goto move_followup;
                }
            }
            if (bytes == 1) {
                high = (user->opcode >> (user_index ? 20 : 19)) & 1;
                if (((instruction->opcode >> 19) & 1) != high ||
                    ((instruction->opcode & HIGH_SECOND) && machine_operand_bytes(user, user_index) != 1))
                    goto move_followup;
            }
            if (!replace_machine_operand(user, user_index, other, 1)) goto move_followup;
            if (bytes == 1) {
                if (user_index) user->opcode = (user->opcode & ~HIGH_SECOND) | (instruction->opcode & HIGH_SECOND);
                else user->opcode = (user->opcode & ~HIGH_FIRST) | ((instruction->opcode >> 1) & HIGH_FIRST);
            }
            goto move_forwarded;
        }
        if (OPERAND_KIND(source) == OPERAND_BASED) {
            if (instruction->opcode & HIGH_SECOND) goto move_followup;
            user_bytes = MACHINE_OP(user) == MC_LEA ? MACHINE_BYTES(user) : 4;
            if (user_bytes > MACHINE_BYTES(instruction)) goto move_followup;
            address = source->value.address;
            if (kind == OPERAND_REGISTER) {
                address->base.value = other->value.constant;
                goto remove_instruction;
            }
            if (kind == OPERAND_LABEL_ADDRESS && !address->symbol) {
                source_address = other->value.address;
                source->flags = (source->flags & ~15U) | 3;
                address->symbol = source_address->symbol;
                address->first.displacement = (long)((unsigned)address->first.displacement +
                    (unsigned)source_address->first.displacement);
                goto remove_instruction;
            }
            if (kind != OPERAND_IMMEDIATE) goto move_followup;
            displacement = (long)((unsigned)address->first.displacement + other->value.constant);
            if (MACHINE_OP(user) == MC_LEA && !address->symbol) {
                SET_OPERATION(user, MC_MOV);
                source->flags = (source->flags & ~15U) | OPERAND_IMMEDIATE;
                source->value.constant = (unsigned)displacement;
            } else {
                if (!address->symbol) goto move_followup;
                source->flags = (source->flags & ~15U) | 3;
                address->first.displacement = displacement;
            }
            goto move_forwarded;
        }
        if (OPERAND_KIND(source) == OPERAND_INDEXED) {
            if (instruction->opcode & HIGH_SECOND) goto move_followup;
            user_bytes = MACHINE_OP(user) == MC_LEA ? MACHINE_BYTES(user) : 4;
            if (user_bytes > MACHINE_BYTES(instruction)) goto move_followup;
            address = source->value.address;
            reg = OPERAND_REG(operand);
            if (kind == OPERAND_REGISTER) {
                if (address->base.words.number == reg) address->base.value = other->value.constant;
                if (address->index.words.number == reg) address->index.value = other->value.constant;
                goto remove_instruction;
            }
            if (kind == OPERAND_LABEL_ADDRESS && !address->symbol) {
                if (address->base.words.number == reg) {
                    if (address->index.words.number == reg) goto move_followup;
                    if (!(source->flags & 0x300)) {
                        source->flags = (source->flags & ~15U) | OPERAND_BASED;
                        address->base = address->index;
                    } else address->base.words.number = 27;
                } else {
                    if (source->flags & 0x300) goto move_followup;
                    source->flags = (source->flags & ~15U) | OPERAND_BASED;
                }
                source_address = other->value.address;
                address->symbol = source_address->symbol;
                address->first.displacement = (long)((unsigned)address->first.displacement +
                    (unsigned)source_address->first.displacement);
                goto remove_instruction;
            }
            if (kind != OPERAND_IMMEDIATE) goto move_followup;
            if (address->base.words.number == reg) {
                address->first.displacement = (long)((unsigned)address->first.displacement + other->value.constant);
                if (!(source->flags & 0x300)) {
                    source->flags = (source->flags & ~15U) | OPERAND_BASED;
                    address->base = address->index;
                    if (address->base.words.number == reg) {
                        tree_changed = 1;
                        return next;
                    }
                } else {
                    address->base.words.number = 27;
                    if (address->index.words.number == reg) {
                        tree_changed = 1;
                        return next;
                    }
                }
                goto move_forwarded;
            }
            address->first.displacement = (long)((unsigned)address->first.displacement +
                (other->value.constant << ((source->flags >> 8) & 3)));
            source->flags &= ~0x300U;
            if (address->base.words.number == 27) {
                if (address->symbol || MACHINE_OP(user) != MC_LEA) source->flags = (source->flags & ~15U) | 3;
                else {
                    SET_OPERATION(user, MC_MOV);
                    source->value.constant = (unsigned)address->first.displacement;
                    source->flags = (source->flags & ~15U) | OPERAND_IMMEDIATE;
                }
            } else source->flags = (source->flags & ~15U) | OPERAND_BASED;
            goto move_forwarded;
        }
        compiler_internal_error("@(#)peeps.c:1.148", 0x1af);
move_followup:
        if (OPERAND_KIND(operand) == OPERAND_REGISTER && OPERAND_KIND(other) == OPERAND_IMMEDIATE &&
            MACHINE_OP(user) == MC_ADD && !(instruction->opcode & USER_SECOND) &&
            MACHINE_BYTES(instruction) == MACHINE_BYTES(user) &&
            OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER &&
            OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE) {
            SET_OPERATION(user, MC_MOV);
            user->operands[1].value.constant += other->value.constant;
            goto remove_instruction;
        }
        user_index = (instruction->opcode & USER_SECOND) != 0;
        source = &user->operands[user_index];
        kind = OPERAND_KIND(other);
        if (MACHINE_OP(user) == MC_LEA && user_index &&
            (kind == 3 || kind == OPERAND_BASED || kind == OPERAND_INDEXED)) {
            address = source->value.address;
            if ((OPERAND_KIND(source) == OPERAND_BASED ||
                 (OPERAND_KIND(source) == OPERAND_INDEXED && !(source->flags & 0x300) &&
                  !address->symbol && !address->first.displacement &&
                  address->base.words.number != address->index.words.number &&
                  !operand_mentions_register(source, OPERAND_REG(&user->operands[0])) &&
                  !address->base.words.flags && !address->index.words.flags)) &&
                OPERAND_REG(operand) != OPERAND_REG(&user->operands[0]) &&
                !register_replacement_blocked(0, 0, user->operands[0].value.constant) &&
                !register_mentioned_between(instruction, user, OPERAND_REG(&user->operands[0]))) {
                if (OPERAND_KIND(source) == OPERAND_INDEXED && address->index.words.number == OPERAND_REG(operand))
                    address->index.value = user->operands[0].value.constant;
                else address->base.value = user->operands[0].value.constant;
                operand->value.constant = user->operands[0].value.constant;
                tree_changed = 1;
                return next;
            }
        }
        if (MACHINE_OP(user) == MC_LEA && user_index && mode) {
            if (MACHINE_BYTES(user) <= MACHINE_BYTES(instruction) &&
                (kind == 3 || kind == OPERAND_BASED || kind == OPERAND_INDEXED) &&
                !operand_mentions_register(other, OPERAND_REG(&user->operands[0])) &&
                OPERAND_KIND(source) == OPERAND_INDEXED && !(source->flags & 0x300)) {
                address = source->value.address;
                reg = OPERAND_REG(&user->operands[0]);
                if (address->base.words.number != address->index.words.number &&
                    !address->first.displacement && !address->symbol && !register_replacement_blocked(0, 0, reg)) {
                    other_reg = address->base.words.number;
                    if (OPERAND_REG(operand) == other_reg) other_reg = address->index.words.number;
                    if ((instruction->next == user || !register_replacement_blocked(0, 0, other_reg)) &&
                        !register_mentioned_between(instruction, user, reg) && !find_flags_use(user)) {
                        copy = new_machine_instruction(0);
                        *copy = *instruction;
                        SET_BYTES(copy, MACHINE_BYTES(user));
                        copy->operands[0].value.constant = user->operands[0].value.constant;
                        copy->operands[1].flags = (copy->operands[1].flags & ~15U) | OPERAND_REGISTER;
                        copy->operands[1].value.constant = address->base.words.number == OPERAND_REG(operand) ?
                            address->index.value : address->base.value;
                        copy->previous = user->previous;
                        copy->next = user;
                        user->previous->next = copy;
                        user->previous = copy;
                        SET_OPERATION(user, MC_ADD);
                        value = user->operands[1].value.constant;
                        user->operands[1] = *other;
                        other->value.constant = value;
                        next = user->next;
                        goto remove_instruction;
                    }
                }
            }
        }
        goto move_fallback;
move_forwarded:
        if (instruction->opcode & DEAD_RESULT) goto remove_instruction;
        tree_changed = 1;
        return next;
move_fallback:
        if (!mode && OPERAND_KIND(operand) == OPERAND_REGISTER && OPERAND_KIND(other) == OPERAND_REGISTER &&
            MACHINE_OP(user) == MC_ADD && !(instruction->opcode & USER_SECOND) &&
            MACHINE_BYTES(instruction) == MACHINE_BYTES(user) &&
            OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER &&
            (OPERAND_KIND(&user->operands[1]) == 3 ||
             OPERAND_KIND(&user->operands[1]) == OPERAND_BASED ||
             OPERAND_KIND(&user->operands[1]) == OPERAND_INDEXED) &&
            !register_replacement_blocked(instruction, 0, other->value.constant) &&
            !operand_changed_between(instruction, user->next, other, MACHINE_BYTES(instruction))) {
            SET_OPERATION(user, MC_MOV);
            copy = clone_machine_record(instruction);
            user = insert_machine_after(user, copy);
            SET_OPERATION(user, MC_ADD);
            goto remove_instruction;
        }
        user_flags = user->opcode;
        user_opcode = MACHINE_OP(user);
        flags = instruction->opcode;
        if (user_opcode == MC_MOV) {
            if (!(flags & USER_SECOND)) {
                if (MACHINE_BYTES(instruction) == 2 && MACHINE_BYTES(user) == 1 &&
                    OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER &&
                    (OPERAND_KIND(other) != OPERAND_REGISTER || mode || byte_register_available(OPERAND_REG(other)))) {
                    SET_BYTES(instruction, 1);
                    if (!(user_flags & HIGH_FIRST)) instruction->opcode |= HIGH_FIRST | HIGH_SECOND;
                    tree_changed = 1;
                    return next;
                }
            } else {
                if (MACHINE_BYTES(user) == 1 &&
                    ((flags & DEAD_RESULT) || operands_equal(&user->operands[0], &user->operands[1])) &&
                    OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER &&
                    OPERAND_KIND(&user->operands[1]) == OPERAND_REGISTER &&
                    (MACHINE_BYTES(instruction) == 1 || mode) &&
                    !operand_changed_between(instruction, user, other, MACHINE_BYTES(instruction)) &&
                    !register_replacement_blocked(instruction, 0, other->value.constant)) {
                    user->operands[1] = *other;
                    goto remove_instruction;
                }
                if (OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER &&
                    OPERAND_KIND(&user->operands[1]) == OPERAND_REGISTER &&
                    operands_equal(other, &user->operands[0]) &&
                    MACHINE_BYTES(instruction) >= MACHINE_BYTES(user) &&
                    !(((user_flags >> 19) ^ (user_flags >> 20)) & 1) &&
                    !operand_changed_between(instruction, user, other, MACHINE_BYTES(instruction)) &&
                    !register_replacement_blocked(instruction, 0, other->value.constant)) goto remove_user;
            }
            goto move_last_byte;
        }
        if (user_opcode == MC_ADD) {
            if (OPERAND_KIND(&user->operands[0]) != OPERAND_REGISTER) goto move_last_byte;
            if (OPERAND_KIND(other) == OPERAND_REGISTER &&
                (register_replacement_blocked(instruction, 0, other->value.constant) ||
                 operand_changed_between(instruction, user, other, MACHINE_BYTES(instruction)))) goto move_last_byte;
            if ((instruction->opcode & 0x190000) || find_flags_use(user) ||
                (MACHINE_BYTES(instruction) != 4 &&
                 (MACHINE_BYTES(user) > MACHINE_BYTES(instruction) || !narrow_result_safe(user)))) goto move_last_byte;
            source = &user->operands[1];
            if (OPERAND_KIND(other) == OPERAND_REGISTER) {
                source = other;
                other = &user->operands[1];
            }
            kind = OPERAND_KIND(other);
            if ((kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS) &&
                OPERAND_KIND(source) == OPERAND_REGISTER) {
                value = source->value.constant;
                if (kind == OPERAND_LABEL_ADDRESS) {
                    symbol = other->value.address->symbol;
                    displacement = other->value.address->first.displacement;
                } else {
                    symbol = 0;
                    displacement = (long)other->value.constant;
                }
                SET_OPERATION(user, MC_LEA);
                address = new_machine_address();
                user->operands[1].value.address = address;
                address->base.value = value;
                address->symbol = symbol;
                address->first.displacement = displacement;
                user->operands[1].flags = (user->operands[1].flags & ~15U) | OPERAND_BASED;
                goto remove_instruction;
            }
            if (kind == OPERAND_REGISTER && OPERAND_KIND(source) == OPERAND_REGISTER) {
                SET_OPERATION(user, MC_LEA);
                value = user->operands[1].value.constant;
                user->operands[1].flags = (user->operands[1].flags & ~15U) | OPERAND_INDEXED;
                address = new_machine_address();
                user->operands[1].value.address = address;
                address->base.value = value;
                address->index.value = instruction->operands[1].value.constant;
                user->operands[1].flags &= ~0x300U;
                goto remove_instruction;
            }
            goto move_last_byte;
        }
        if (user_opcode == MC_AND) {
            use = user->record.next_use;
            if (MACHINE_BYTES(user) == 4 && OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE && use &&
                operands_equal(&use->operands[0], &user->operands[0])) {
                value = user->operands[1].value.constant;
                if (MACHINE_OP(use) == MC_MOV && MACHINE_BYTES(use) == 1 &&
                    ((value == 0xff00 && !(use->opcode & HIGH_FIRST)) || (value == 255 && (use->opcode & HIGH_FIRST)))) {
                    SET_OPERATION(user, MC_MOV);
                    SET_BYTES(user, MACHINE_BYTES(instruction) == 1 ? 1 : 2);
                    user->opcode = (user->opcode & ~(HIGH_FIRST | HIGH_SECOND)) |
                        (instruction->opcode & (HIGH_FIRST | HIGH_SECOND));
                    user->operands[1] = instruction->operands[1];
                    SET_OPERATION(instruction, MC_SUB);
                    SET_BYTES(instruction, 4);
                    instruction->opcode &= ~(HIGH_FIRST | HIGH_SECOND);
                    instruction->operands[1] = instruction->operands[0];
                    tree_changed = 1;
                    return next;
                }
                if ((mode || byte_register_available(OPERAND_REG(operand))) &&
                    (OPERAND_KIND(other) != OPERAND_REGISTER || mode || byte_register_available(OPERAND_REG(other))) &&
                    value == 0xff00 && MACHINE_OP(use) == MC_SHR &&
                    OPERAND_KIND(&use->operands[1]) == OPERAND_IMMEDIATE && use->operands[1].value.constant == 8) {
                    if (MACHINE_BYTES(instruction) == 1) {
                        if (!(instruction->opcode & HIGH_FIRST)) goto move_last_byte;
                    } else {
                        SET_BYTES(instruction, 1);
                        instruction->opcode |= HIGH_SECOND;
                    }
                    instruction->opcode &= ~HIGH_FIRST;
                    SET_OPERATION(use, MC_AND);
                    use->operands[1].value.constant = 255;
                    SET_BYTES(use, 4);
                    goto remove_user;
                }
            }
            goto move_last_byte;
        }
        if (user_opcode == MC_DEC || user_opcode == MC_SUB) {
            if (OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER && !(instruction->opcode & USER_SECOND) &&
                OPERAND_KIND(other) == OPERAND_REGISTER && OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE &&
                !register_replacement_blocked(instruction, 0, other->value.constant) &&
                !operand_changed_between(instruction, user, other, MACHINE_BYTES(instruction)) &&
                !find_flags_use(user) &&
                (MACHINE_BYTES(instruction) == 4 ||
                 (MACHINE_BYTES(instruction) >= MACHINE_BYTES(user) && narrow_result_safe(user)))) {
                SET_OPERATION(user, MC_LEA);
                value = user->operands[1].value.constant;
                user->operands[1].flags = (user->operands[1].flags & ~15U) | OPERAND_BASED;
                address = new_machine_address();
                user->operands[1].value.address = address;
                address->first.displacement = (long)(0U - value);
                address->base.value = other->value.constant;
                goto remove_instruction;
            }
        }
move_last_byte:
        user_index = (instruction->opcode & USER_SECOND) != 0;
        if (machine_operand_bytes(user, user_index) != 1 || MACHINE_BYTES(instruction) <= 1 ||
            !(instruction->opcode & DEAD_RESULT) || (!mode && !byte_register_available(OPERAND_REG(operand)))) return next;
        if ((user->opcode >> (user_index ? 20 : 19)) & 1) return next;
        if (OPERAND_KIND(&user->operands[user_index]) == OPERAND_REGISTER) {
            SET_BYTES(instruction, 1);
            tree_changed = 1;
        }
        return next;

    case MC_JCC: case MC_JMP:
        return simplify_branch(instruction);

    case MC_IDIV:
        operand = &instruction->operands[0];
        if (OPERAND_KIND(operand) != OPERAND_IMMEDIATE) return next;
        value = operand->value.constant;
        if (value & (value - 1)) return next;
        if (value == 2) {
            first = new_machine_instruction(0);
            SET_OPERATION(first, MC_SUB);
            SET_BYTES(first, MACHINE_BYTES(instruction));
            initialize_operand(&first->operands[0], OPERAND_REGISTER, REG_EAX, 0, 0);
            initialize_operand(&first->operands[1], OPERAND_REGISTER, REG_EDX, 0, 0);
            first->previous = instruction->previous;
            instruction->previous->next = first;
        } else {
            second = new_machine_instruction(0);
            SET_OPERATION(second, MC_AND);
            SET_BYTES(second, MACHINE_BYTES(instruction));
            initialize_operand(&second->operands[0], OPERAND_REGISTER, REG_EDX, 0, 0);
            initialize_operand(&second->operands[1], OPERAND_IMMEDIATE, 0, value - 1, 0);
            second->previous = instruction->previous;
            instruction->previous->next = second;
            first = new_machine_instruction(0);
            SET_OPERATION(first, MC_ADD);
            SET_BYTES(first, MACHINE_BYTES(instruction));
            initialize_operand(&first->operands[0], OPERAND_REGISTER, REG_EAX, 0, 0);
            initialize_operand(&first->operands[1], OPERAND_REGISTER, REG_EDX, 0, 0);
            second->next = first;
            first->previous = second;
        }
        second = new_machine_instruction(0);
        SET_OPERATION(second, MC_SAR);
        SET_BYTES(second, MACHINE_BYTES(instruction));
        initialize_operand(&second->operands[0], OPERAND_REGISTER, REG_EAX, 0, 0);
        initialize_operand(&second->operands[1], OPERAND_IMMEDIATE, 0, integer_log2(value), 0);
        first->next = second;
        second->previous = first;
        second->next = instruction;
        instruction->previous = second;
        goto remove_instruction;

    case MC_LEA:
        operand = &instruction->operands[0];
        other = &instruction->operands[1];
        if (user && (flags & DEAD_RESULT) && !address_changed_between(instruction, user, other)) {
            source = &user->operands[(flags & USER_SECOND) != 0];
            if (fold_lea_address(instruction, source, 1)) goto remove_instruction;
            if (MACHINE_OP(user) == MC_MOV && (instruction->opcode & USER_SECOND) &&
                OPERAND_KIND(source) == OPERAND_REGISTER &&
                OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER &&
                MACHINE_BYTES(instruction) == MACHINE_BYTES(user) && !(user->opcode & (HIGH_FIRST | HIGH_SECOND)) &&
                !(OPERAND_REG(source) & 0x4000) &&
                !register_mentioned_between(instruction, user, OPERAND_REG(&user->operands[0]))) {
                operand->value.constant = user->operands[0].value.constant;
                goto remove_user;
            }
        }
        kind = OPERAND_KIND(other);
        address = other->value.address;
        if (kind == OPERAND_BASED || (kind == OPERAND_INDEXED && address->base.words.number == 27)) {
            if (user) {
                if (MACHINE_OP(user) == MC_ADD &&
                    OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER &&
                    OPERAND_KIND(&user->operands[1]) == OPERAND_REGISTER &&
                    (!(instruction->opcode & USER_SECOND) || (user->operands[1].flags & LAST_USE)) &&
                    OPERAND_REG(&user->operands[1]) != OPERAND_REG(&user->operands[0]) &&
                    address->base.words.number != OPERAND_REG(operand) && !find_flags_use(user) &&
                    !address_changed_between(instruction, user, other)) {
                    source = &user->operands[(instruction->opcode & USER_SECOND) ? 0 : 1];
                    if (kind == OPERAND_INDEXED) address->base.value = source->value.constant;
                    else {
                        address->index.value = source->value.constant;
                        other->flags = (other->flags & ~15U) | OPERAND_INDEXED;
                    }
                    if (instruction->opcode & USER_SECOND) operand->value.constant = OPERAND_REG(&user->operands[0]);
                    user->operands[0] = *operand;
                    user->operands[1] = *other;
                    SET_OPERATION(user, MACHINE_OP(instruction));
                    goto remove_instruction;
                }
                goto lea_shift;
            }
        } else {
lea_shift:
            if (user && MACHINE_OP(user) == MC_SHL &&
                OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE &&
                (long)user->operands[1].value.constant < 4) {
                amount = user->operands[1].value.constant;
                if (kind == OPERAND_BASED) {
                    address->index = address->base;
                    address->base.words.number = 27;
                    other->flags = (other->flags & ~0x300U) | ((amount << 8) & 0x300);
                    address->first.displacement = (long)((unsigned)address->first.displacement << (amount & 31));
                    other->flags = (other->flags & ~15U) | OPERAND_INDEXED;
                    goto remove_user;
                }
                if (kind == OPERAND_INDEXED && address->base.words.number == 27 &&
                    ((other->flags >> 8) & 3) + amount < 4) {
                    other->flags = (other->flags & ~0x300U) | ((other->flags + (amount << 8)) & 0x300);
                    address->first.displacement = (long)((unsigned)address->first.displacement << (amount & 31));
                    goto remove_user;
                }
            }
        }
        kind = OPERAND_KIND(other);
        reg = OPERAND_REG(operand);
        address = other->value.address;
        if (kind == OPERAND_BASED) {
            if (address->base.words.number == reg &&
                (mode || MACHINE_BYTES(instruction) != 1 || byte_register_available(reg)) && !find_flags_use(instruction)) {
                SET_OPERATION(instruction, MC_ADD);
                if (!address->symbol) {
                    other->flags = (other->flags & ~15U) | OPERAND_IMMEDIATE;
                    other->value.constant = (unsigned)address->first.displacement;
                    release_machine_address(address);
                } else other->flags = (other->flags & ~15U) | OPERAND_LABEL_ADDRESS;
                tree_changed = 1;
                return next;
            }
            if (address->symbol) return next;
            if (!address->first.displacement) {
                SET_OPERATION(instruction, MC_MOV);
                other->value.constant = address->base.value;
                other->flags = (other->flags & ~15U) | OPERAND_REGISTER;
                release_machine_address(address);
                tree_changed = 1;
            }
            return next;
        }
        if (kind == OPERAND_INDEXED) {
            if (address->symbol || address->first.displacement || (other->flags & 0x300)) return next;
            if (reg != address->base.words.number && reg != address->index.words.number) return next;
            if (!mode && MACHINE_BYTES(instruction) == 1 &&
                (!byte_register_available(address->index.words.number) ||
                 (address->base.words.number != 27 && !byte_register_available(address->base.words.number)))) return next;
            if (find_flags_use(instruction)) return next;
            if (address->base.words.number != 27) {
                value = reg == address->base.words.number ? address->index.value : address->base.value;
                SET_OPERATION(instruction, MC_ADD);
                release_machine_address(address);
                other->flags = (other->flags & ~15U) | OPERAND_REGISTER;
                other->value.constant = value;
                tree_changed = 1;
                return next;
            }
        } else {
            if (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS) {
                SET_OPERATION(instruction, MC_MOV);
                tree_changed = 1;
                return next;
            }
            if (!user || !(instruction->opcode & DEAD_RESULT) || !(instruction->opcode & USER_SECOND) ||
                MACHINE_OP(user) != MC_MOV || OPERAND_KIND(&user->operands[0]) != OPERAND_REGISTER ||
                MACHINE_BYTES(instruction) != MACHINE_BYTES(user) ||
                operand_changed_between(instruction, user, other, MACHINE_BYTES(instruction))) return next;
            SET_OPERATION(user, MC_LEA);
            source = &user->operands[1];
            {
                MachineOperand saved_operand = *source;
                *source = *other;
                *other = saved_operand;
            }
        }
        goto remove_instruction;

    case MC_IMUL2:
        if (mode && optimize_speed && MACHINE_BYTES(instruction) == 4 &&
            OPERAND_KIND(&instruction->operands[1]) == OPERAND_IMMEDIATE && expand_constant_multiply(instruction))
            tree_changed = 1;
        return next;

    case MC_OR: case MC_XOR:
        operand = &instruction->operands[0];
        other = &instruction->operands[1];
        if (user && MACHINE_BYTES(user) == 1 && MACHINE_BYTES(instruction) != 1 &&
            ((flags & DEAD_RESULT) ||
             (OPERAND_REG(&user->operands[0]) == OPERAND_REG(operand) && (user->operands[0].flags & LAST_USE)))) {
            user_index = (flags & USER_SECOND) != 0;
            if (OPERAND_KIND(&user->operands[user_index]) == OPERAND_REGISTER) {
                high = (user->opcode >> (user_index ? 20 : 19)) & 1;
                SET_BYTES(instruction, 1);
                instruction->opcode = (instruction->opcode & ~(HIGH_FIRST | HIGH_SECOND)) |
                    (high ? HIGH_FIRST | HIGH_SECOND : 0);
                tree_changed = 1;
            }
        }
        if (OPERAND_KIND(other) == OPERAND_IMMEDIATE) {
            value = other->value.constant;
            mask = integer_width_mask(MACHINE_BYTES(instruction));
            if ((operand->flags & HIGH_BYTE_ZERO) && !(mask & value & 0xffff00ff) &&
                (mode || byte_register_available(OPERAND_REG(operand)))) {
                SET_OPERATION(instruction, MC_MOV);
                SET_BYTES(instruction, 1);
                instruction->opcode |= HIGH_FIRST | HIGH_SECOND;
                tree_changed = 1;
                return next;
            }
            if ((operand->flags & LOW_BYTE_ZERO) && !(mask & value & 0xffffff00) &&
                (mode || byte_register_available(OPERAND_REG(operand)))) {
                SET_OPERATION(instruction, MC_MOV);
                SET_BYTES(instruction, 1);
                tree_changed = 1;
                return next;
            }
            if (user && (MACHINE_OP(user) == MC_SHR || MACHINE_OP(user) == MC_SAR) &&
                OPERAND_KIND(&user->operands[1]) == OPERAND_IMMEDIATE &&
                user->operands[1].value.constant == 8 && MACHINE_BYTES(instruction) == 4 &&
                MACHINE_BYTES(user) == 4 && !(value & 0xff00ff00)) {
                other->value.constant = (unsigned)((long)value >> 8);
                SWAP_INSTRUCTION_DATA(instruction, user);
                user->record.next_use = 0;
                tree_changed = 1;
                return next;
            }
        }
        if (MACHINE_OP(instruction) == MC_XOR && OPERAND_KIND(operand) == OPERAND_REGISTER &&
            (operand->flags & LAST_USE) && (instruction->next->opcode & 15) == 0 &&
            MACHINE_OP(instruction->next) == MC_JCC &&
            ((instruction->next->operands[1].flags & 15) == 4 || (instruction->next->operands[1].flags & 15) == 5)) {
            SET_OPERATION(instruction, MC_CMP);
            tree_changed = 1;
        }
        return next;

    case MC_TEST:
        if (!find_flags_use(instruction)) goto remove_instruction;
        user = instruction->next;
        if ((user->opcode & 15) != 0 || MACHINE_OP(user) != MC_JCC) return next;
        use = user;
        do {
            use = use->next;
            if (!use->next) break;
        } while ((use->opcode & 15) == 4 || (use->opcode & 15) == 7);
        if ((use->opcode & 15) == 0 && MACHINE_OP(use) == MC_TEST &&
            operands_equal(&instruction->operands[0], &use->operands[0])) {
            if (!operands_equal(&instruction->operands[1], &use->operands[1])) {
                if (OPERAND_KIND(&instruction->operands[1]) != OPERAND_IMMEDIATE ||
                    OPERAND_KIND(&use->operands[1]) != OPERAND_IMMEDIATE || (user->operands[1].flags & 15) != 5)
                    goto narrow_test;
                following = use->next;
                if ((following->opcode & 15) != 0 || MACHINE_OP(following) != MC_JCC ||
                    (following->operands[1].flags & 15) != 5 || MACHINE_BYTES(user) != MACHINE_BYTES(following) ||
                    !operands_equal(&user->operands[0], &following->operands[0])) goto narrow_test;
                instruction->operands[1].value.constant |= use->operands[1].value.constant;
                remove_machine_instruction(use);
                adjust_machine_symbol_references(following, -1);
                use = following;
            }
            remove_machine_instruction(use);
        }
narrow_test:
        operand = &instruction->operands[0];
        other = &instruction->operands[1];
        if (OPERAND_KIND(operand) == OPERAND_REGISTER && OPERAND_KIND(other) == OPERAND_IMMEDIATE &&
            (other->value.constant == 255 || other->value.constant == 0xff00)) {
            value = other->value.constant;
            SET_OPERATION(instruction, MC_CMP);
            SET_BYTES(instruction, 1);
            other->value.constant = 0;
            if (value == 0xff00) instruction->opcode |= HIGH_FIRST;
            tree_changed = 1;
        }
        return next;

    case MC_SUB:
        operand = &instruction->operands[0];
        other = &instruction->operands[1];
        if (OPERAND_KIND(other) == OPERAND_IMMEDIATE && !find_carry_flag_use(instruction)) {
            SET_OPERATION(instruction, MC_ADD);
            other->value.constant = 0U - other->value.constant;
            goto fold_add;
        }
        if (!user || MACHINE_OP(user) != MC_ADD || OPERAND_KIND(&user->operands[0]) != OPERAND_REGISTER ||
            !(instruction->opcode & USER_SECOND) || OPERAND_KIND(&user->operands[1]) != OPERAND_REGISTER ||
            !(user->operands[1].flags & LAST_USE) || MACHINE_BYTES(instruction) != MACHINE_BYTES(user) ||
            register_replacement_blocked(instruction, 0, user->operands[0].value.constant) || find_flags_use(instruction))
            return next;
        reg = OPERAND_REG(&user->operands[0]);
        for (use = instruction->next; use != user; use = use->next) {
            kind = use->opcode & 15;
            if ((kind == 0 || kind == 8 || kind == 9 || kind == 10) && instruction_mentions_register(use, reg)) return next;
        }
        instruction->operands[0] = user->operands[0];
        instruction->opcode = (instruction->opcode & ~HIGH_FIRST) | (user->opcode & HIGH_FIRST);
        tree_changed = 1;
        return next;

    case MC_ADD:
        operand = &instruction->operands[0];
        other = &instruction->operands[1];
fold_add:
        if (!find_flags_use(instruction) && OPERAND_KIND(operand) == OPERAND_REGISTER && user &&
            (instruction->opcode & DEAD_RESULT)) {
            source = &user->operands[(instruction->opcode & USER_SECOND) != 0];
            kind = OPERAND_KIND(other);
            reg = OPERAND_REG(operand);
            if ((kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS) &&
                (OPERAND_KIND(source) == OPERAND_BASED || OPERAND_KIND(source) == OPERAND_INDEXED) &&
                (kind == OPERAND_IMMEDIATE || !source->value.address->symbol)) {
                address = source->value.address;
                value = kind == OPERAND_IMMEDIATE ? other->value.constant :
                    (unsigned)other->value.address->first.displacement;
                amount = 0;
                if (OPERAND_KIND(source) == OPERAND_INDEXED && address->index.words.number == reg)
                    amount = value << ((source->flags >> 8) & 3);
                if (address->base.words.number == reg) amount += value;
                address->first.displacement = (long)((unsigned)address->first.displacement + amount);
                if (kind == OPERAND_LABEL_ADDRESS) address->symbol = other->value.address->symbol;
                goto remove_instruction;
            }
            if (kind == OPERAND_REGISTER) {
                if (OPERAND_KIND(source) == OPERAND_BASED &&
                    !register_replacement_blocked(instruction, 0, other->value.constant) &&
                    !operand_changed_between(instruction, user, other, MACHINE_BYTES(instruction))) {
                    source->value.address->index.value = other->value.constant;
                    source->flags = (source->flags & ~(0x300U | 15U)) | OPERAND_INDEXED;
                    goto remove_instruction;
                }
            } else if (kind != OPERAND_IMMEDIATE && kind != OPERAND_LABEL_ADDRESS &&
                OPERAND_KIND(source) == OPERAND_BASED && mode) {
                reg = new_value_register(0x104);
                source->value.address->index.value = reg;
                source->flags = (source->flags & ~(0x300U | 15U)) | OPERAND_INDEXED;
                SET_OPERATION(instruction, MC_MOV);
                operand->value.constant = source->value.address->index.value;
                tree_changed = 1;
                return next;
            }
            if (MACHINE_OP(user) == MC_MOV && OPERAND_KIND(source) == OPERAND_REGISTER &&
                (instruction->opcode & USER_SECOND) && OPERAND_KIND(&user->operands[0]) == OPERAND_REGISTER &&
                MACHINE_BYTES(user) <= MACHINE_BYTES(instruction) &&
                (MACHINE_BYTES(user) == 4 || narrow_result_safe(user))) {
                if (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS) {
                    SET_OPERATION(user, MC_LEA);
                    user->operands[1].flags = (user->operands[1].flags & ~15U) | OPERAND_BASED;
                    address = new_machine_address();
                    user->operands[1].value.address = address;
                    if (kind == OPERAND_LABEL_ADDRESS) {
                        address->symbol = other->value.address->symbol;
                        address->first.displacement = other->value.address->first.displacement;
                    } else address->first.displacement = (long)other->value.constant;
                    address->base.value = operand->value.constant;
                    goto remove_instruction;
                }
                if (kind == OPERAND_REGISTER && !register_replacement_blocked(instruction, 0, other->value.constant) &&
                    !operand_changed_between(instruction, user, other, MACHINE_BYTES(instruction))) {
                    SET_OPERATION(user, MC_LEA);
                    user->operands[1].flags = (user->operands[1].flags & ~(0x300U | 15U)) | OPERAND_INDEXED;
                    address = new_machine_address();
                    user->operands[1].value.address = address;
                    address->base.value = operand->value.constant;
                    address->index.value = other->value.constant;
                    goto remove_instruction;
                }
            }
        }
        if (OPERAND_KIND(other) == OPERAND_IMMEDIATE && !other->value.constant) goto remove_instruction;
        return next;

    case MC_MOVZX: case MC_MOVSX:
        if (MACHINE_BYTES(instruction) == 2 && user && (flags & DEAD_RESULT) && MACHINE_BYTES(user) == 4 &&
            (flags & USER_SECOND) && OPERAND_KIND(&user->operands[1]) == OPERAND_REGISTER &&
            ((opcode == MC_MOVZX && MACHINE_OP(user) == MC_MOVZXW) || MACHINE_OP(user) == MC_MOVSXW) &&
            (OPERAND_KIND(&instruction->operands[1]) != OPERAND_REGISTER ||
             !register_replacement_blocked(instruction, 0, instruction->operands[1].value.constant)) &&
            !operand_changed_between(instruction, user, &instruction->operands[1], MACHINE_BYTES(instruction))) {
            SET_OPERATION(user, MACHINE_OP(instruction));
            goto extension_forwarded;
        }
        /* Fall through to the common extension-width and copy rules. */
    case MC_MOVZXW: case MC_MOVSXW:
        if (!user || !(instruction->opcode & DEAD_RESULT)) return next;
        if (mode) {
            user_index = (instruction->opcode & USER_SECOND) != 0;
            bytes = machine_operand_bytes(user, user_index);
            if (bytes < MACHINE_BYTES(instruction) && OPERAND_KIND(&user->operands[user_index]) == OPERAND_REGISTER &&
                !((user->opcode >> (user_index ? 20 : 19)) & 1)) {
                if (MACHINE_BYTES(instruction) != 4 || bytes != 2 ||
                    MACHINE_OP(instruction) == MC_MOVSXW || MACHINE_OP(instruction) == MC_MOVZXW)
                    SET_OPERATION(instruction, MC_MOV);
                SET_BYTES(instruction, bytes);
                tree_changed = 1;
                return next;
            }
        }
        if (MACHINE_OP(user) != MC_MOV || MACHINE_BYTES(instruction) != MACHINE_BYTES(user) ||
            OPERAND_KIND(&user->operands[0]) != OPERAND_REGISTER ||
            OPERAND_KIND(&user->operands[1]) != OPERAND_REGISTER ||
            operand_changed_between(instruction, user, &instruction->operands[1], MACHINE_BYTES(instruction)) ||
            (OPERAND_KIND(&instruction->operands[1]) == OPERAND_REGISTER &&
             register_replacement_blocked(instruction, 0, instruction->operands[1].value.constant))) return next;
        SET_OPERATION(user, MACHINE_OP(instruction));
extension_forwarded:
        user->operands[1] = instruction->operands[1];
        user->opcode = (user->opcode & ~HIGH_SECOND) | (instruction->opcode & HIGH_SECOND);
        goto remove_instruction;
    }
    return next;
remove_user:
    tree_changed = 1;
    if (next == user) next = user->next;
    remove_machine_instruction(user);
    return next;
remove_instruction:
    remove_machine_instruction(instruction);
    tree_changed = 1;
    return next;
}

/* Original [00021cb0,00021fc0), including the 23-entry operation dispatch
   at 21f5c..21fb8. Reuse an earlier flags definition when both compared
   operands are unchanged, then combine adjacent compatible conditions. */
int __fastcall eliminate_redundant_compare(MachineInstruction *instruction, MachineInstruction **next)
{
    enum { CC_B = 2, CC_AE = 3, CC_E = 4, CC_NE = 5, CC_BE = 6, CC_A = 7,
           CC_S = 8, CC_NS = 9, CC_L = 12, CC_GE = 13, CC_LE = 14, CC_G = 15 };
    /* Original 80348..80358. A positive relation denotes a strict condition
       implying its paired inclusive condition; the negative reverse relation
       cannot be combined with equality. Indexed by the x86 condition number. */
    static const signed char condition_relation[16] = {
        0, 0, -CC_BE, CC_A, 0, 0, CC_B, -CC_AE,
        0, 0, 0, 0, -CC_LE, CC_G, CC_L, -CC_GE
    };
    MachineInstruction *producer, *consumer, *preceding;
    unsigned kind, operation, condition, earlier_condition, difference;
    int exact_compare, relation;
    for (producer = instruction->previous; producer; producer = producer->previous) {
        kind = producer->opcode & 15;
        if (kind == 1) return 0;
        if (!kind && (machine_selection_properties[MACHINE_OP(producer)] & MC_WRITES_FLAGS)) break;
    }
    if (!producer) return 0;
    operation = MACHINE_OP(producer);
    switch (operation) {
    case MC_DEC: case MC_INC: case MC_ADC: case MC_ADD: case MC_SBB: case MC_SUB:
        if (OPERAND_KIND(&instruction->operands[1]) != OPERAND_IMMEDIATE ||
            instruction->operands[1].value.constant) return 0;
        exact_compare = 0;
        break;
    case MC_AND: case MC_OR: case MC_XOR:
        if (OPERAND_KIND(&instruction->operands[1]) != OPERAND_IMMEDIATE ||
            instruction->operands[1].value.constant) return 0;
        exact_compare = 1;
        break;
    case MC_CMP:
        if (!instruction_operands_equal(producer, 1, instruction, 1) ||
            operand_changed_between(producer, instruction, &producer->operands[1], MACHINE_BYTES(producer))) return 0;
        exact_compare = 1;
        break;
    default:
        return 0;
    }
    if (!instruction_operands_equal(producer, 0, instruction, 0) ||
        operand_changed_between(producer, instruction, &producer->operands[0], MACHINE_BYTES(producer))) return 0;
    for (consumer = instruction->next; (consumer->opcode & 15) &&
         (consumer->opcode & 15) != 1 && (consumer->opcode & 15) != 7; consumer = consumer->next) ;
    if ((consumer->opcode & 15) || (MACHINE_OP(consumer) != MC_JCC && MACHINE_OP(consumer) != MC_SCC)) return 0;
    condition = consumer->operands[1].flags & 15;
    /* The native two-stage condition tables at 80330..80348 allow only
       equality and signed less/greater-equal after a zero-result test.
       An exact comparison or logical result already has the required flags. */
    if (!exact_compare) {
        if (condition == CC_L) consumer->operands[1].flags =
            (consumer->operands[1].flags & ~15U) | CC_S;
        else if (condition == CC_GE) consumer->operands[1].flags =
            (consumer->operands[1].flags & ~15U) | CC_NS;
        else if (condition != CC_E && condition != CC_NE) return 0;
    }
    for (preceding = producer->next; (preceding->opcode & 15) &&
         (preceding->opcode & 15) != 1 && (preceding->opcode & 15) != 7; preceding = preceding->next) ;
    if (!(preceding->opcode & 15) && MACHINE_OP(preceding) == MACHINE_OP(consumer)) {
        condition = consumer->operands[1].flags & 15;
        earlier_condition = preceding->operands[1].flags & 15;
        difference = condition ^ earlier_condition;
        relation = condition_relation[earlier_condition];
        if (!difference || relation == (int)condition || (relation > 0 && condition == CC_E)) {
            if (*next == consumer) *next = consumer->next;
            --((LabelSymbol *)consumer->operands[0].value.symbol)->references;
            remove_machine_instruction(consumer);
        } else if ((MACHINE_OP(consumer) == MC_JCC && difference == 1) ||
                   (int)(condition ^ 1) == relation ||
                   (earlier_condition == CC_NE && condition_relation[condition] > 0)) {
            consumer->opcode = (consumer->opcode & 0xffff10ffU) | (MC_JMP << 8);
        }
    }
    tree_changed = 1;
    remove_machine_instruction(instruction);
    return 1;
}

/* Original [0001a5d0,0001a5df). Insert inside a nonterminal stream. */
MachineInstruction *__fastcall insert_machine_after(MachineInstruction *previous,
    MachineInstruction *instruction)
{
    instruction->previous = previous;
    instruction->next = previous->next;
    instruction->next->previous = instruction;
    previous->next = instruction;
    return instruction;
}

/* Original80388..8039c: masks indexed by the integer operand's byte
   width. Zero and the unused three-byte width have no integer mask. */
static const unsigned integer_width_masks[5] = {0, 0xff, 0xffff, 0, 0xffffffffU};
/* Original [00021c90,00021c98). Integer operations use widths1,2 or4. */
unsigned __fastcall integer_width_mask(unsigned bytes)
{
    return integer_width_masks[bytes];
}

/* Original [00022a40,00022ade). A rematerializable register retains its
   source only when that source survives the intervening instruction stream. */
int __fastcall register_source_changed_between(MachineInstruction *first,
    MachineInstruction *last, unsigned short reg)
{
    MachineBlockHeader *block;
    MachineInstruction *definition;
    unsigned operation;
    if (first->next == last) return 0;
    block = register_rematerialization(reg);
    if (!block) return 0;
    definition = block->next;
    if ((definition->next->opcode & 15) != 7) return 1;
    operation = MACHINE_OP(definition);
    if (operation == MC_LEA)
        return address_changed_between(first, last, &definition->operands[1]) != 0;
    if (operation != MC_MOV && operation != MC_MOVZX && operation != MC_MOVZXW &&
        operation != MC_MOVSX && operation != MC_MOVSXW) return 1;
    return operand_changed_between(first, last, &definition->operands[1],
        MACHINE_BYTES(definition)) != 0;
}

/* Original [000218f0,000219c7). Search a bounded shift/add multiplication
   recipe, then replace this immediate IMUL with the selected sequence. */
int __fastcall expand_constant_multiply(MachineInstruction *instruction)
{
    char recipe[20];
    int temporary = 0, cost = 30000, budget, maximum;
    unsigned value, magnitude, length;
    if (((instruction->opcode >> 8) & 255) != MC_IMUL2 ||
        (instruction->operands[1].flags & 15) != OPERAND_IMMEDIATE)
        compiler_internal_error("@(#)peeps.c:1.148", 0x109a);
    value = instruction->operands[1].value.constant;
    magnitude = (int)value < 0 ? 0U - value : value;
    maximum = constant_multiply_budget(magnitude);
    budget = maximum - 1;
    if (!choose_multiply_recipe(magnitude, 0, &budget, &cost, &temporary, recipe)) return 0;
    if (budget >= maximum)
        compiler_internal_error("@(#)peeps.c:1.148", 0x10a5);
    for (length = 0; recipe[length]; ++length) {}
    if (length >= sizeof(recipe))
        compiler_internal_error("@(#)peeps.c:1.148", 0x10a6);
    emit_multiply_recipe(instruction, temporary, recipe);
    return 1;
}

/* Original [000254a0,000254b3). Native IMUL cost bound for a constant. */
int __fastcall constant_multiply_budget(unsigned magnitude)
{
    int bits = integer_log2(magnitude);
    if (bits < 3) bits = 3;
    return bits + 6;
}
/* Encountered small factors from original803e8..804a8. L followed by a
   factor byte denotes an address-form multiply; byte and cycle costs belong
   to that recipe. Original slots0,1,7,11,13,14 have no direct recipe. */
static MultiplyRecipe multiply_by_two = { "L\002", 2, 3 };
static MultiplyRecipe multiply_by_three = { "L\003", 2, 3 };
static MultiplyRecipe multiply_by_four = { "L\004", 2, 3 };
static MultiplyRecipe multiply_by_five = { "L\005", 2, 3 };
static MultiplyRecipe multiply_by_six = { "L\003L\002", 4, 6 };
static MultiplyRecipe multiply_by_eight = { "L\010", 2, 3 };
static MultiplyRecipe multiply_by_nine = { "L\011", 2, 3 };
static MultiplyRecipe multiply_by_ten = { "L\005L\002", 4, 6 };
static MultiplyRecipe multiply_by_twelve = { "L\003L\004", 4, 6 };
static MultiplyRecipe multiply_by_fifteen = { "L\003L\005", 4, 6 };
#define SMALL_MULTIPLY_RECIPE(n) \
    ((n) == 2 ? &multiply_by_two : (n) == 3 ? &multiply_by_three : \
     (n) == 4 ? &multiply_by_four : (n) == 5 ? &multiply_by_five : \
     (n) == 6 ? &multiply_by_six : (n) == 8 ? &multiply_by_eight : \
     (n) == 9 ? &multiply_by_nine : (n) == 10 ? &multiply_by_ten : \
     (n) == 12 ? &multiply_by_twelve : (n) == 15 ? &multiply_by_fifteen : (MultiplyRecipe *)0)
/* Original803b8..803e4: descending direct factors and terminating zero. */
static const int multiply_factors[] = { 15, 12, 10, 9, 8, 6, 5, 4, 3, 2, 0 };
typedef struct MultiplyCorrection {
    int delta;
    char *instructions;
    int bytes, cycles;
} MultiplyCorrection;
/* Original five aligned entries of804a8,804c0,804d8,804f0. Trying zero
   first preserves an equally priced exact recipe ahead of adjusted ones. */
static const MultiplyCorrection multiply_corrections[5] = {
    {0, "", 0, 0}, {-1, "+", 2, 2}, {1, "-", 2, 2},
    {-2, "++", 4, 4}, {2, "--", 4, 4}
};
/* Native compiler inlines these string operations; keep their original graph. */
#define COPY_MULTIPLY_RECIPE(d,s) do { \
    char *copy_to = (d); const char *copy_from = (s); \
    while ((*copy_to++ = *copy_from++) != 0) {} \
} while (0)
#define APPEND_MULTIPLY_RECIPE(d,s) do { \
    char *copy_to = (d); const char *copy_from = (s); \
    while (*copy_to) ++copy_to; \
    while ((*copy_to++ = *copy_from++) != 0) {} \
} while (0)

/* Original [00025000,00025494). Search factorization and nearby constants
   for the shortest recipe; cycle cost breaks ties. Four recursive levels
   bound composite factors. Adjustments may require retaining the input. */
int __fastcall choose_multiply_recipe(unsigned magnitude, unsigned depth,
    int *budget, int *cost, int *temporary, char *recipe)
{
    MultiplyRecipe factor_scratch, quotient_scratch, *part, *quotient_part;
    const MultiplyCorrection *correction;
    char part_buffer[20], candidate[20], *end;
    int adjustment, value, factor, power, state, factor_index = 0;
    int quotient, remaining, quotient_cost, quotient_temporary;
    int bytes, cycles, adjustment_temporary, difference, improved = 0;
    for (adjustment = 0; adjustment < 5; ++adjustment) {
        correction = &multiply_corrections[adjustment];
        value = (int)(magnitude + correction->delta);
        if (value < 2) continue;
        if (!(value & (value - 1)) || SMALL_MULTIPLY_RECIPE(value)) {
            factor = value;
            part = lookup_multiply_recipe(value, &factor_scratch, part_buffer);
            state = 4;
        } else {
            if (depth == 4 || correction->delta < -1 || correction->delta > 1) continue;
            state = 0;
        }
        for (;;) {
            if (!state) {
                factor = 0;
                for (power = 1; power < (value >> 2); power += power) factor = power;
                state = 1;
            } else if (state == 1) factor >>= 1;
            if (state == 1) {
                while (factor > multiply_factors[0] && value % factor) factor >>= 1;
                if (factor > multiply_factors[0])
                    part = lookup_multiply_recipe(factor, &factor_scratch, part_buffer);
                else state = 2;
            }
            if (state == 2) {
                factor_index = 0;
                while (value < multiply_factors[factor_index]) ++factor_index;
                state = 3;
            } else if (state == 3) ++factor_index;
            if (state == 3) {
                while ((factor = multiply_factors[factor_index]) != 0 && value % factor)
                    ++factor_index;
                if (!factor) break;
                part = SMALL_MULTIPLY_RECIPE(factor);
            }
            remaining = *budget - part->bytes;
            if (remaining < 0) goto next_factor;
            if (value == factor) {
                quotient_temporary = quotient_cost = remaining = 0;
                COPY_MULTIPLY_RECIPE(candidate, part->instructions);
            } else {
                quotient = value / factor;
                if (!(quotient & (quotient - 1)) || SMALL_MULTIPLY_RECIPE(quotient)) {
                    quotient_part = lookup_multiply_recipe(quotient, &quotient_scratch, candidate);
                    quotient_temporary = 0;
                    quotient_cost = quotient_part->cycles;
                    remaining = quotient_part->bytes;
                    COPY_MULTIPLY_RECIPE(candidate, quotient_part->instructions);
                } else {
                    quotient_cost = *cost - part->cycles;
                    if (!choose_multiply_recipe(quotient, depth + 1, &remaining,
                        &quotient_cost, &quotient_temporary, candidate)) goto next_factor;
                }
                APPEND_MULTIPLY_RECIPE(candidate, part->instructions);
            }
            adjustment_temporary = 0;
            cycles = part->cycles;
            bytes = part->bytes;
            end = candidate;
            while (*end) ++end;
            if (value < (int)magnitude && end[-2] == 'L' && !(end[-1] & 1)) {
                adjustment_temporary = 1;
                end[-2] = 'l';
                difference = (int)(magnitude - (unsigned)value) - 1;
                while (difference-- > 0) {
                    *end++ = '+';
                    bytes += 2;
                    cycles += 2;
                }
                *end = 0;
            } else if (value != (int)magnitude) {
                adjustment_temporary = 1;
                cycles += correction->cycles;
                bytes += correction->bytes;
                APPEND_MULTIPLY_RECIPE(candidate, correction->instructions);
            }
            cycles += quotient_cost;
            bytes += remaining;
            if (adjustment_temporary && !quotient_temporary) {
                bytes += 2;
                cycles += 2;
            }
            if (bytes < *budget || (bytes == *budget && cycles < *cost)) {
                improved = 1;
                *temporary = adjustment_temporary || quotient_temporary;
                *cost = cycles;
                *budget = bytes;
                COPY_MULTIPLY_RECIPE(recipe, candidate);
            }
next_factor:
            if (state >= 4) break;
        }
    }
    return improved;
}

/* Original empty descriptors803e8/803f4 can receive the caller's buffer
   when the lookup is explicitly made for0 or1. Preserve those identities. */
static MultiplyRecipe multiply_by_zero, multiply_by_one;
/* Original [000254c0,00025518). Direct factors share their descriptor;
   other powers of two use an S/shift-count recipe in caller-owned storage. */
MultiplyRecipe *__fastcall lookup_multiply_recipe(unsigned magnitude,
    MultiplyRecipe *scratch, char *buffer)
{
    MultiplyRecipe *small;
    if (magnitude < 16) {
        small = magnitude == 0 ? &multiply_by_zero :
            magnitude == 1 ? &multiply_by_one : SMALL_MULTIPLY_RECIPE(magnitude);
        if (small) {
            if (small->instructions) return small;
            scratch = small;
        }
    }
    if (magnitude & (magnitude - 1)) return 0;
    buffer[0] = 'S';
    buffer[1] = (char)integer_log2(magnitude);
    buffer[2] = 0;
    scratch->instructions = buffer;
    scratch->cycles = 2;
    scratch->bytes = 3;
    return scratch;
}
