/* Final machine forms and byte-operand selection after register assignment. */
#include "backend-passes.h"
#include "instruction-rewrite.h"
#include "frame.h"
#include "register-use.h"
extern int optimize_speed, cpu_model;
#define MACHINE_OP(i) (((i)->opcode >> 8) & 255)
#define MACHINE_BYTES(i) (((i)->opcode >> 4) & 15)
#define OPERAND_KIND(o) ((o)->flags & 15)
#define SET_OPERATION(i,op) ((i)->opcode = ((i)->opcode & ~0xff00U) | ((unsigned)(op) << 8))
#define SET_WIDTH(i,n) ((i)->opcode = ((i)->opcode & ~0xf0U) | ((unsigned)(n) << 4))
#define HIGH_FIRST 0x80000U
#define HIGH_SECOND 0x100000U
/* Original [00023ee0,000246d0). Complete the selected instruction forms,
   replace frame-size pseudo operands, and resolve high-byte annotations. */
void __fastcall finalize_machine_context(MachineInstruction *instruction)
{
    MachineInstruction *next, *scan, *inserted;
    MachineOperand *left, *right;
    MachineOperand frame_operand;
    MachineAddress *address;
    unsigned operation, flags, immediate, scale, width;
    unsigned short destination, other;
    int excluded = 0, byte_offset, count;
    for (; instruction; instruction = next) {
        next = instruction->next;
        if ((instruction->opcode & 15) == 7) {
            excluded = ((MachineBlockHeader *)instruction)->flags & 2;
            continue;
        }
        if (instruction->opcode & 15) continue;
        instruction->record.next_use = 0;
        left = &instruction->operands[0];
        right = &instruction->operands[1];
        if (!excluded) {
            operation = MACHINE_OP(instruction);
            switch (operation) {
            case MC_MOV:
                flags = instruction->opcode;
                if (!((MACHINE_BYTES(instruction) <= 1) &&
                      (((flags >> 19) ^ (flags >> 20)) & 1)) && operands_equal(left, right))
                    remove_machine_instruction(instruction);
                else if (OPERAND_KIND(left) == OPERAND_REGISTER &&
                         OPERAND_KIND(right) == OPERAND_IMMEDIATE && !right->value.constant &&
                         !find_flags_use(instruction)) {
                    SET_OPERATION(instruction, MC_SUB);
                    flags = instruction->opcode;
                    instruction->opcode = (flags & ~HIGH_SECOND) | ((flags & HIGH_FIRST) << 1);
                    right->flags = (right->flags & ~15U) | OPERAND_REGISTER;
                    right->value.reg.words.number = left->value.reg.words.number;
                }
                break;
            case MC_LEA:
                destination = left->value.reg.words.number;
                if (!find_flags_use(instruction)) {
                    if (OPERAND_KIND(right) == OPERAND_BASED) {
                        address = right->value.address;
                        if (address->base.words.number == destination && !address->symbol &&
                            (MACHINE_BYTES(instruction) != 1 || byte_register_available(destination))) {
                            SET_OPERATION(instruction, MC_ADD);
                            immediate = (unsigned)address->first.displacement;
                            right->flags = (right->flags & ~15U) | OPERAND_IMMEDIATE;
                            right->value.constant = immediate;
                            release_machine_address(address);
                            goto finish_add;
                        }
                    } else if (OPERAND_KIND(right) == OPERAND_INDEXED) {
                        address = right->value.address;
                        if (!address->first.displacement && !address->symbol) {
                            scale = (right->flags >> 8) & 3;
                            if (!scale) {
                                other = address->base.words.number;
                                if (destination == other) other = address->index.words.number;
                                else if (destination != address->index.words.number) other = 0x1a;
                                if (other != 0x1a && (MACHINE_BYTES(instruction) != 1 ||
                                    (byte_register_available(destination) && byte_register_available(other)))) {
                                    SET_OPERATION(instruction, MC_ADD);
                                    release_operand_address(right);
                                    initialize_operand(right, OPERAND_REGISTER, other, 0, 0);
                                    right->flags = (right->flags & ~15U) | OPERAND_REGISTER;
                                }
                            } else if (address->base.words.number == 0x1b &&
                                       address->index.words.number == destination) {
                                release_operand_address(right);
                                if (scale == 1) {
                                    SET_OPERATION(instruction, MC_ADD);
                                    initialize_operand(right, OPERAND_REGISTER, left->value.reg.words.number, 0, 0);
                                } else {
                                    SET_OPERATION(instruction, MC_SHL);
                                    initialize_operand(right, OPERAND_IMMEDIATE, 0x1a, scale, 0);
                                }
                            }
                        }
                    }
                }
                if (MACHINE_OP(instruction) == MC_LEA) SET_WIDTH(instruction, 4);
                break;
            case MC_ADD:
finish_add:
                if (OPERAND_KIND(right) == OPERAND_IMMEDIATE) {
                    immediate = right->value.constant;
                    if (immediate == 0xffffffffU) {
                        if (!find_carry_flag_use(instruction)) SET_OPERATION(instruction, MC_DEC);
                        else goto negative_add;
                    } else if (immediate == 1) {
                        if (!find_carry_flag_use(instruction)) SET_OPERATION(instruction, MC_INC);
                    } else {
negative_add:
                        if ((int)right->value.constant < 0) {
                            SET_OPERATION(instruction, MC_SUB);
                            right->value.constant = 0U - right->value.constant;
                        }
                    }
                }
                break;
            case MC_CMP:
                if (OPERAND_KIND(left) == OPERAND_REGISTER && OPERAND_KIND(right) == OPERAND_IMMEDIATE &&
                    !right->value.constant) {
                    SET_OPERATION(instruction, MC_TEST);
                    flags = instruction->opcode;
                    instruction->opcode = (flags & ~HIGH_SECOND) | ((flags & HIGH_FIRST) << 1);
                    right->flags = (right->flags & ~15U) | OPERAND_REGISTER;
                    right->value = left->value;
                }
                break;
            case MC_SUB:
                if (OPERAND_KIND(right) == OPERAND_IMMEDIATE && !right->value.constant &&
                    !find_flags_use(instruction)) {
                    remove_machine_instruction(instruction);
                    instruction = 0;
                } else if (OPERAND_KIND(right) == OPERAND_FRAME_SIZE)
                    resolve_frame_allocation(instruction);
                break;
            case MC_TEST:
                width = MACHINE_BYTES(instruction);
                if (width < 2 || OPERAND_KIND(right) != OPERAND_IMMEDIATE) break;
                byte_offset = -1;
                immediate = right->value.constant;
                if (!(immediate & 0xffff0000U) &&
                    (OPERAND_KIND(left) != OPERAND_REGISTER || byte_register_available(left->value.reg.words.number))) {
                    if (!(immediate & 0xffffff00U)) byte_offset = 0;
                    else if (!(immediate & 0xffff00ffU)) byte_offset = 1;
                } else if (!(immediate & 0xffff) && width == 4 && OPERAND_KIND(left) != OPERAND_REGISTER) {
                    if (!(immediate & 0xff00ffffU)) byte_offset = 2;
                    else if (!(immediate & 0xffffffU)) byte_offset = 3;
                }
                if (byte_offset >= 0) {
                    while (byte_offset--) {
                        advance_operand_byte(left);
                        advance_operand_byte(right);
                    }
                    SET_WIDTH(instruction, 1);
                }
                break;
            case MC_SAHF:
                if (optimize_speed) optimize_sahf(instruction);
                break;
            case MC_EPOP:
                if (dynamic_stack_allocation &&
                    ((instruction->previous->opcode & 15) || MACHINE_OP(instruction->previous) != MC_POP)) {
                    scan = instruction->next;
                    count = 1;
                    while (MACHINE_OP(scan) == MC_EPOP) { scan = scan->next; ++count; }
                    initialize_operand(&frame_operand, OPERAND_BASED, REG_EBP,
                        0U - ((unsigned)count * 4 + current_symbol->field30), 0);
                    inserted = new_machine_record(0, MC_LEA, 4, &operand_esp, &frame_operand);
                    insert_machine_after(instruction->previous, inserted);
                }
                SET_OPERATION(instruction, MC_POP);
                break;
            case MC_EPUSH:
                SET_OPERATION(instruction, MC_PUSH);
                break;
            case MC_MOVZX: case MC_MOVZXW:
                if (optimize_speed && OPERAND_KIND(right) == OPERAND_REGISTER &&
                    left->value.reg.words.number == right->value.reg.words.number &&
                    !(instruction->opcode & HIGH_SECOND) && !find_flags_use(instruction)) {
                    right->flags = (right->flags & ~15U) | OPERAND_IMMEDIATE;
                    right->value.constant = operation == MC_MOVZXW ? 0xffff : 0xff;
                    SET_OPERATION(instruction, MC_AND);
                } else if (cpu_model >= 4 && optimize_speed && MACHINE_BYTES(instruction) >= 2 &&
                           !find_flags_use(instruction) && expand_zero_extension(instruction))
                    next = instruction->next;
                break;
            case MC_EFWAIT:
                if (!(current_function->field28 & 8)) remove_machine_instruction(instruction);
                else SET_OPERATION(instruction, MC_FWAIT);
                break;
            }
        } else if (machine_operand_counts[MACHINE_OP(instruction)] == 2 &&
                   OPERAND_KIND(right) == OPERAND_FRAME_SIZE) {
            if (!(current_symbol->symbol.field0e & 0x400))
                compiler_internal_error("@(#)peeps.c:1.148", 0xf88);
            resolve_frame_allocation(instruction);
        }
        if (instruction) {
            count = machine_operand_counts[MACHINE_OP(instruction)];
            if (count == 2) {
                if (instruction->opcode & HIGH_SECOND) {
                    if (machine_operand_bytes(instruction, 1) != 1)
                        compiler_internal_error("@(#)peeps.c:1.148", 0xf93);
                    advance_operand_byte(right);
                }
            } else if (count != 1) continue;
            if (instruction->opcode & HIGH_FIRST) {
                if (machine_operand_bytes(instruction, 0) != 1)
                    compiler_internal_error("@(#)peeps.c:1.148", 0xf99);
                advance_operand_byte(left);
            }
        }
    }
}
