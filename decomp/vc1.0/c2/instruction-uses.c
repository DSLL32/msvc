#include "instruction-rewrite.h"
#include "tree-rewrite.h"
#include "register-use.h"
#include "function-input.h"
/* Original [000217c0,00021860). Determine whether an intervening instruction
   invalidates an operand before its use. Symbolic/immediate values are stable;
   a label ends availability for the other operand forms. */
int __fastcall operand_changed_between(MachineInstruction *first, MachineInstruction *last,
    MachineOperand *operand, unsigned bytes)
{
    MachineInstruction *instruction;
    unsigned kind = operand->flags & 15;
    if (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS || kind == 7 || kind == OPERAND_SYMBOL)
        return 0;
    for (instruction = first->next; instruction != last; instruction = instruction->next) {
        kind = instruction->opcode & 15;
        if (kind == 1) return 1;
        if ((kind == 0 || kind == 8 || kind == 9 || kind == 10) &&
            instruction_modifies_operand(instruction, operand, bytes)) return 1;
    }
    return 0;
}

/* Original [00035760,00035840). Port, shift and extending-move operands
   have widths independent of the instruction's result width. */
unsigned __fastcall machine_operand_bytes(MachineInstruction *instruction, int index)
{
    unsigned operation = (instruction->opcode >> 8) & 255;
    if (operation == MC_IN && index == 1)
        return (instruction->operands[1].flags & 15) == OPERAND_REGISTER ? 2 : 1;
    if (operation == MC_OUT && index == 0)
        return (instruction->operands[0].flags & 15) == OPERAND_REGISTER ? 2 : 1;
    if (((operation >= MC_RCL && operation <= MC_SHR) ||
         operation == MC_MOVZX || operation == MC_MOVSX) && index == 1)
        return 1;
    if (operation == MC_FSTSW || operation == MC_FNSTSW)
        return 2;
    if ((operation == MC_MOVZXW || operation == MC_MOVSXW) && index == 1)
        return 2;
    return (instruction->opcode >> 4) & 15;
}

/* Original [00022740,000229d0), including its operation dispatch table.
   Check the x86 operand restrictions before changing the selected record. */
MachineInstruction *__fastcall replace_machine_operand(MachineInstruction *instruction, unsigned index,
    MachineOperand *operand, unsigned char apply)
{
    unsigned operation = (instruction->opcode >> 8) & 255;
    unsigned kind = operand->flags & 15;
    MachineOperand *destination;
    int check_general = 1;
    switch (operation) {
    case MC_IMUL: case MC_DIV: case MC_IDIV: case MC_MUL:
        if (kind == OPERAND_IMMEDIATE) return 0;
        break;
    case MC_IN: case MC_OUT:
        if (index != (unsigned)(operation == MC_IN) || kind != OPERAND_IMMEDIATE ||
            (operand->value.constant & 255) != operand->value.constant) return 0;
        check_general = 0;
        break;
    case MC_CALL:
        if (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS || kind == 7) return 0;
        check_general = 0;
        break;
    case MC_RCL: case MC_RCR: case MC_ROL: case MC_ROR:
    case MC_SAR: case MC_SHL: case MC_SHR:
        if (index == 1 && kind != OPERAND_IMMEDIATE &&
            (kind != OPERAND_REGISTER || operand->value.reg.words.number != REG_ECX)) return 0;
        break;
    case MC_IMUL3: case MC_IMUL2:
        if (index == 0 && kind != OPERAND_REGISTER) return 0;
        break;
    case MC_MOVZX: case MC_MOVZXW: case MC_MOVSX: case MC_MOVSXW:
        if (index == 0) {
            if (kind != OPERAND_REGISTER) return 0;
        } else if (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS || kind == 7)
            return 0;
        break;
    case MC_BT: case MC_BTS: case MC_BTR: case MC_BTC:
        if (index == 1 && kind != OPERAND_REGISTER && kind != OPERAND_IMMEDIATE) return 0;
        if (index == 0 && kind != OPERAND_REGISTER && kind != OPERAND_ABSOLUTE) return 0;
        break;
    }
    if (check_general && !operand_replacement_allowed(instruction, index, operand, apply)) return 0;
    if (apply & 1) {
        destination = &instruction->operands[index == 0 ? 0 : 1];
        release_operand_address(destination);
        *destination = *operand;
        tree_changed = 1;
    }
    return instruction;
}

/* Original [00022c40,00022d20). Prevent two memory operands and changes
   to a written destination unless the caller explicitly permits them. */
int __fastcall operand_replacement_allowed(MachineInstruction *instruction, unsigned index,
    MachineOperand *operand, unsigned char apply)
{
    int count = machine_operand_counts[(instruction->opcode >> 8) & 255];
    unsigned kind = operand->flags & 15;
    unsigned other;
    if (count == 1) {
        if (kind == OPERAND_REGISTER && !(apply & 2) && writes_first_operand(instruction)) return 0;
        return 1;
    }
    if (count != 2) return count;
    if (index == 1) {
        if (kind == OPERAND_LABEL_ADDRESS) return machine_operand_bytes(instruction, 1) == 4;
        return (instruction->operands[0].flags & 15) == OPERAND_REGISTER ||
            kind == OPERAND_REGISTER || kind == OPERAND_IMMEDIATE;
    }
    if (kind == OPERAND_REGISTER) {
        if (!(apply & 2) && writes_first_operand(instruction)) return 0;
        return 1;
    }
    if (kind == OPERAND_IMMEDIATE || kind == OPERAND_LABEL_ADDRESS) return 0;
    other = instruction->operands[1].flags & 15;
    return other == OPERAND_REGISTER || other == OPERAND_IMMEDIATE || other == OPERAND_LABEL_ADDRESS;
}

/* Original [000224a0,00022660). Collapse fall-through branches, invert a
   conditional branch around a jump, and bypass a compatible target jump.
   The comparison conditions below express the encountered 82c38 relation. */
MachineInstruction *__fastcall simplify_branch(MachineInstruction *instruction)
{
    enum { CC_B = 2, CC_AE = 3, CC_E = 4, CC_NE = 5, CC_BE = 6,
           CC_L = 12, CC_GE = 13, CC_LE = 14, CC_G = 15 };
    MachineInstruction *next = instruction->next, *scan, *after;
    LabelSymbol *target, *replacement;
    unsigned operation, condition, target_condition;
    int compatible;
    if ((instruction->operands[0].flags & 15) != OPERAND_SYMBOL ||
        instruction->operands[0].value.symbol->kind != 3) return next;
    target = (LabelSymbol *)instruction->operands[0].value.symbol;
    if (branch_falls_through(instruction)) {
        tree_changed = 1;
        --target->references;
        remove_machine_instruction(instruction);
        return next;
    }
    for (scan = next; scan && ((scan->opcode & 15) == 7 || (scan->opcode & 15) == 4);
         scan = scan->next) ;
    if (scan && !(scan->opcode & 15) && ((scan->opcode >> 8) & 255) == MC_JMP &&
        (scan->operands[0].flags & 15) == OPERAND_SYMBOL) {
        replacement = (LabelSymbol *)scan->operands[0].value.symbol;
        if (target == replacement) {
            tree_changed = 1;
            --target->references;
            remove_machine_instruction(instruction);
            return next;
        }
        if (((instruction->opcode >> 8) & 255) == MC_JCC) {
            for (after = scan->next; after && ((after->opcode & 15) == 7 ||
                 (after->opcode & 15) == 4); after = after->next) ;
            if (after && (after->opcode & 15) == 1 && after->record.symbol == &target->symbol) {
                --target->references;
                instruction->operands[0].value.symbol = &replacement->symbol;
                instruction->operands[1].flags ^= 1;
                remove_machine_instruction(scan);
                tree_changed = 1;
                return instruction;
            }
        }
    }
    if (target->instruction) {
        scan = target->instruction->next;
        while (scan && ((scan->opcode & 15) == 4 || (scan->opcode & 15) == 7 ||
               (scan->opcode & 15) == 1)) scan = scan->next;
        if (scan && !(scan->opcode & 15)) {
            operation = (scan->opcode >> 8) & 255;
            compatible = operation == MC_JMP &&
                (scan->operands[0].flags & 15) == OPERAND_SYMBOL &&
                scan->operands[0].value.symbol != &target->symbol;
            if (operation == MC_JCC && ((instruction->opcode >> 8) & 255) == MC_JCC) {
                condition = instruction->operands[1].flags & 15;
                target_condition = scan->operands[1].flags & 15;
                compatible = condition == target_condition ||
                    (condition == CC_B && (target_condition == CC_NE || target_condition == CC_BE)) ||
                    (condition == CC_E && (target_condition == CC_AE || target_condition == CC_BE ||
                         target_condition == CC_GE || target_condition == CC_LE)) ||
                    (condition == CC_L && target_condition == CC_LE) ||
                    (condition == CC_G && target_condition == CC_GE);
            }
            if (compatible) {
                --target->references;
                replacement = (LabelSymbol *)scan->operands[0].value.symbol;
                ++replacement->references;
                instruction->operands[0].value.symbol = &replacement->symbol;
                tree_changed = 1;
                return instruction;
            }
        }
    }
    return next;
}

/* Original [00022ae0,00022b20). Only source/block markers and intervening
   labels may lie between a branch and its fall-through destination. */
int __fastcall branch_falls_through(MachineInstruction *instruction)
{
    MachineInstruction *scan;
    unsigned kind;
    for (scan = instruction->next; scan; scan = scan->next) {
        kind = scan->opcode & 15;
        if (kind == 1) {
            if (scan->record.symbol == instruction->operands[0].value.symbol) return 1;
        } else if (kind != 4 && kind != 7) return 0;
    }
    return 0;
}

/* Original [00023670,00023750). Keep flag producers whose flags are used,
   volatile accesses, implicit effects and floating operations. */
int __fastcall instruction_has_side_effects(MachineInstruction *instruction)
{
    unsigned operation = (instruction->opcode >> 8) & 255;
    int count = machine_operand_counts[operation];
    if ((machine_selection_properties[operation] & MC_WRITES_FLAGS) && find_flags_use(instruction)) return 1;
    if (count == 2 && (instruction->operands[1].flags & 0x8000)) return 1;
    if ((count == 1 || count == 2) && (instruction->operands[0].flags & 0x8000)) return 1;
    if (operation == MC_IMUL || (operation >= MC_IN && operation <= MC_JMP) ||
        operation == MC_DIV || operation == MC_IDIV || operation == MC_MUL ||
        (operation >= MC_CMPS && operation <= MC_STOS) || operation == MC_REP ||
        (operation >= MC_REPNZ && operation <= MC_EPUSH)) return 1;
    return (operation >= MC_FADD && operation <= MC_FLD1) ||
        (operation >= MC_FUCOM && operation <= MC_FCOS);
}

int conservative_flag_analysis; /* 0007c8b0: enabled for temporary/spill streams */
/* Original [00021aa0,00021b60). Locate a flag consumer before the next
   flag definition or unconditional transfer. REP string instructions can
   execute zero times and therefore do not always kill incoming flags. */
MachineInstruction *__fastcall find_flags_use(MachineInstruction *instruction)
{
    MachineInstruction *scan, *previous;
    unsigned operation, properties;
    if (conservative_flag_analysis &&
        !(machine_selection_properties[(instruction->opcode >> 8) & 255] & MC_WRITES_FLAGS))
        return instruction;
    for (scan = instruction->next; scan; scan = scan->next) {
        if (scan->opcode & 15) continue;
        operation = (scan->opcode >> 8) & 255;
        properties = machine_selection_properties[operation];
        if (properties & MC_READS_FLAGS) return scan;
        if (properties & MC_WRITES_FLAGS) {
            if ((properties & MC_STRING_OPERATION) && (previous = previous_machine_operation(scan)) &&
                (machine_selection_properties[(previous->opcode >> 8) & 255] & MC_REPEAT_PREFIX)) {
                if (machine_selection_properties[(instruction->opcode >> 8) & 255] & MC_WRITES_FLAGS) return 0;
                return instruction;
            }
            return 0;
        }
        if (operation == MC_JMP) return 0;
    }
    return 0;
}

/* Original [00021b60,00021be0). Test explicit operands in native reverse
   order, then physical implicit registers. Virtual classes have no implicit
   register mask and therefore need no physical-register lookup. */
int __fastcall instruction_mentions_register(MachineInstruction *instruction, unsigned short reg)
{
    unsigned operation = (unsigned char)(instruction->opcode >> 8);
    if (machine_operand_counts[operation] > 1 && operand_mentions_register(&instruction->operands[1], reg)) return 1;
    if (machine_operand_counts[operation] > 0 && operand_mentions_register(&instruction->operands[0], reg)) return 1;
    if (!(reg & 0xc000) && (implicit_register_mask(instruction, 3) & (1U << (reg & 31)))) return 1;
    return 0;
}

/* Original [00021fd0,00022020). Direct operands carry a short register;
   based/indexed addresses can mention their base and index independently. */
int __fastcall operand_mentions_register(MachineOperand *operand, unsigned short reg)
{
    switch (operand->flags & 15) {
    case OPERAND_REGISTER:
        return operand->value.reg.words.number == reg;
    case OPERAND_INDEXED:
        if (operand->value.address->index.words.number == reg) return 1;
        /* Fall through to the base register. */
    case OPERAND_BASED:
        return operand->value.address->base.words.number == reg;
    }
    return 0;
}

/* Original [00021790,000217c0). Tree-virtual registers cannot replace an
   absent/nonregister destination or another tree-virtual destination. The
   native private ABI leaves EDX unused and receives reg on the stack. */
int __fastcall register_replacement_blocked(MachineInstruction *user, unsigned unused_edx, unsigned reg)
{
    (void)unused_edx;
    if (!(reg & 0x4000)) return 0;
    return !user || (user->operands[0].flags & 15) != OPERAND_REGISTER ||
        (user->operands[0].value.reg.words.number & 0x4000) != 0;
}

/* Original [00023830,000239f0). Calls clobber exposed memory homes,
   string writers clobber memory operands, and explicit destinations are
   checked for overlapping storage. Implicit register writes also invalidate
   physical bases/indexes, with one shared mask query for an indexed pair. */
int __fastcall instruction_modifies_operand(MachineInstruction *instruction, MachineOperand *operand, unsigned bytes)
{
    unsigned operation = (unsigned char)(instruction->opcode >> 8);
    unsigned kind = operand->flags & 15, attributes, storage, mask;
    unsigned short reg;
    SymbolHeader *symbol;
    MachineAddress *address;
    if (operation == MC_CALL) {
        if (kind == OPERAND_ABSOLUTE) return 1;
        if (kind == OPERAND_BASED || kind == OPERAND_INDEXED) {
            symbol = operand->value.address->symbol;
            if (!symbol) return 1;
            if (symbol->kind != 19) {
                if (symbol->kind != 1) return 1;
                attributes = ((TemporarySymbol *)symbol)->attributes;
                storage = attributes & 0x70;
                if ((storage != 0x10 && storage != 0x30) || (attributes & 0x12000)) return 1;
            }
        }
    } else if (operation == MC_INS || operation == MC_MOVS || operation == MC_STOS) {
        if (kind == OPERAND_ABSOLUTE || kind == OPERAND_BASED || kind == OPERAND_INDEXED) return 1;
    }
    if (machine_operand_counts[operation] > 0 && writes_first_operand(instruction))
        return operands_overlap_storage(&instruction->operands[0], (instruction->opcode >> 4) & 15, operand, bytes);
    if (!(machine_selection_properties[operation] & MC_IMPLICIT_REGISTERS)) return 0;
    switch (kind) {
    case OPERAND_REGISTER:
        reg = operand->value.reg.words.number;
        break;
    case OPERAND_INDEXED:
        address = operand->value.address;
        reg = address->index.words.number;
        if (!(reg & 0xc000)) {
            mask = implicit_register_mask(instruction, 2);
            if (mask & (1U << (reg & 31))) return 1;
            reg = address->base.words.number;
            return !(reg & 0xc000) && (mask & (1U << (reg & 31))) != 0;
        }
        /* A virtual index cannot be modified implicitly; test the base. */
    case OPERAND_BASED:
        reg = operand->value.address->base.words.number;
        break;
    default:
        return 0;
    }
    return !(reg & 0xc000) && (implicit_register_mask(instruction, 2) & (1U << (reg & 31))) != 0;
}

/* Original [00023a20,00023c70). Compare explicit storage locations before
   treating unrelated addressing expressions conservatively. Absolute versus
   indexed storage is intentionally asymmetric in this native analysis. */
int __fastcall operands_overlap_storage(MachineOperand *left, unsigned left_bytes, MachineOperand *right, unsigned right_bytes)
{
    unsigned left_kind = left->flags & 15, right_kind = right->flags & 15;
    unsigned attributes, storage;
    MachineAddress *a, *b;
    SymbolHeader *left_symbol, *right_symbol;
    if (left_kind == OPERAND_REGISTER)
        return operand_mentions_register(right, left->value.reg.words.number) != 0;
    if (left_kind == OPERAND_ABSOLUTE) {
        if (right_kind == OPERAND_ABSOLUTE) {
            if (left->value.address->symbol != right->value.address->symbol) return 0;
            return memory_displacements_overlap(left, left_bytes, right, right_bytes) != 0;
        }
        if (right_kind == OPERAND_BASED)
            return right->value.address->base.words.number != REG_EBP;
        return right_kind == OPERAND_INDEXED;
    }
    if (left_kind == OPERAND_BASED) {
        if (right_kind == OPERAND_ABSOLUTE) {
            if (left->value.address->base.words.number == REG_EBP) return 0;
        } else if (right_kind == OPERAND_BASED) {
            a = left->value.address;
            b = right->value.address;
            if (a->symbol == b->symbol && a->base.words.number == b->base.words.number &&
                !memory_displacements_overlap(left, left_bytes, right, right_bytes)) return 0;
        } else if (right_kind != OPERAND_INDEXED) return 0;
    } else if (left_kind == OPERAND_INDEXED) {
        if (right_kind == OPERAND_ABSOLUTE) return 1;
        if (right_kind == OPERAND_INDEXED) {
            a = left->value.address;
            b = right->value.address;
            if (a->symbol == b->symbol && a->base.words.number == b->base.words.number &&
                a->index.words.number == b->index.words.number &&
                !((left->flags ^ right->flags) & 0x300) &&
                !memory_displacements_overlap(left, left_bytes, right, right_bytes)) return 0;
        } else if (right_kind != OPERAND_BASED) return 0;
    } else return 0;

    left_symbol = left->value.address->symbol;
    right_symbol = right->value.address->symbol;
    if (!right_symbol) {
        if (left_symbol) {
            if (left_symbol->kind == 19) return 0;
            if (left_symbol->kind == 1) {
                attributes = ((TemporarySymbol *)left_symbol)->attributes;
                storage = attributes & 0x70;
                if ((storage == 0x10 || storage == 0x30) && !(attributes & 0x12000)) return 0;
            }
        }
    } else if (!left_symbol) {
        if (right_symbol->kind == 19) return 0;
        if (right_symbol->kind == 1) {
            attributes = ((TemporarySymbol *)right_symbol)->attributes;
            storage = attributes & 0x70;
            if ((storage == 0x10 || storage == 0x30) && !(attributes & 0x12000)) return 0;
        }
    } else if (left_symbol != right_symbol) return 0;
    return 1;
}

/* Original [00022d20,00022dd0). Address equality is insufficient when
   instruction widths differ or byte operands select different register lanes. */
int __fastcall instruction_operands_equal(MachineInstruction *left, unsigned left_index, MachineInstruction *right, unsigned right_index)
{
    unsigned left_flags = left->opcode, right_flags = right->opcode;
    unsigned left_high, right_high;
    if ((left_flags ^ right_flags) & 0xf0) return 0;
    if ((left_flags & 0xf0) == 0x10) {
        left_high = (left_flags >> (left_index ? 20 : 19)) & 1;
        right_high = (right_flags >> (right_index ? 20 : 19)) & 1;
        if (left_high != right_high) return 0;
    }
    return operands_equal(&left->operands[left_index ? 1 : 0], &right->operands[right_index ? 1 : 0]);
}

/* Original [00022030,000220e0). An address remains valid only while its
   base and optional index retain their register values. Base27 is the native
   no-base sentinel; preserve the helper's 0x104 storage-size argument. */
int __fastcall address_changed_between(MachineInstruction *first, MachineInstruction *last, MachineOperand *operand)
{
    enum { NO_ADDRESS_BASE = 27 };
    unsigned kind = operand->flags & 15;
    MachineOperand reg;
    MachineAddress *address;
    if (kind != OPERAND_BASED && kind != OPERAND_INDEXED) return 0;
    address = operand->value.address;
    initialize_operand(&reg, OPERAND_REGISTER, address->base.words.number, 0, 0);
    if (address->base.words.number != NO_ADDRESS_BASE &&
        operand_changed_between(first, last, &reg, 0x104)) return 1;
    if (kind == OPERAND_BASED) return 0;
    reg.value.reg.words.number = address->index.words.number;
    return operand_changed_between(first, last, &reg, 0x104);
}

/* Original [00022140,000224a0). Substitute a LEA's address for its result
   register in one based/indexed use. Testing and applying deliberately differ
   for a repeated base/index register, as in the native decision tree. */
int __fastcall fold_lea_address(MachineInstruction *instruction, MachineOperand *operand, int apply)
{
    enum { NO_ADDRESS_BASE = 27 };
    MachineOperand *source = &instruction->operands[1];
    MachineAddress *from, *to;
    MachineRegisterValue saved;
    SymbolHeader *symbol;
    unsigned kind = operand->flags & 15, source_kind, scale, source_scale;
    unsigned short reg;
    if (kind != OPERAND_BASED && kind != OPERAND_INDEXED) return 0;
    reg = instruction->operands[0].value.reg.words.number;
    if (!operand_mentions_register(operand, reg)) return 0;
    from = source->value.address;
    to = operand->value.address;
    symbol = from->symbol;
    if (symbol && to->symbol) return 0;
    source_kind = source->flags & 15;
    if (source_kind == OPERAND_ABSOLUTE) {
        if (kind == OPERAND_BASED) {
            if (apply) {
                operand->flags = (operand->flags & ~15U) | OPERAND_ABSOLUTE;
                to->symbol = symbol;
                to->first.displacement = (long)((unsigned long)to->first.displacement + (unsigned long)from->first.displacement);
            }
            return 1;
        }
        if (to->base.words.number == reg) {
            if (to->index.words.number == reg) return 0;
            if (apply) {
                if (!(operand->flags & 0x300)) {
                    operand->flags = (operand->flags & ~15U) | OPERAND_BASED;
                    to->base = to->index;
                } else to->base.words.number = NO_ADDRESS_BASE;
                to->symbol = symbol;
                to->first.displacement = (long)((unsigned long)to->first.displacement + (unsigned long)from->first.displacement);
            }
        } else {
            if (operand->flags & 0x300) return 0;
            if (apply) {
                operand->flags = (operand->flags & ~15U) | OPERAND_BASED;
                to->symbol = symbol;
                to->first.displacement = (long)((unsigned long)to->first.displacement + (unsigned long)from->first.displacement);
            }
        }
        return 1;
    }
    if (source_kind == OPERAND_BASED) {
        if (kind == OPERAND_BASED) {
            if (apply) {
                to->base = from->base;
                to->first.displacement = (long)((unsigned long)to->first.displacement + (unsigned long)from->first.displacement);
                if (symbol) to->symbol = symbol;
            }
        } else if (apply) {
            if (to->base.words.number == reg) {
                if (to->index.words.number == reg) return 0;
                to->base = from->base;
                to->first.displacement = (long)((unsigned long)to->first.displacement + (unsigned long)from->first.displacement);
            } else {
                to->index = from->base;
                scale = (operand->flags >> 8) & 3;
                to->first.displacement = (long)((unsigned long)to->first.displacement + ((unsigned long)from->first.displacement << scale));
            }
            if (symbol) to->symbol = symbol;
        }
        return 1;
    }
    if (source_kind != OPERAND_INDEXED) return 0;
    if (kind == OPERAND_BASED) {
        if (apply) {
            operand->flags = (operand->flags & ~15U) | OPERAND_INDEXED;
            to->first.displacement = (long)((unsigned long)to->first.displacement + (unsigned long)from->first.displacement);
            to->base = from->base;
            to->index = from->index;
            operand->flags = (operand->flags & ~0x300U) | (source->flags & 0x300);
            if (symbol) to->symbol = symbol;
        }
        return 1;
    }
    if (from->base.words.number != NO_ADDRESS_BASE && to->base.words.number != NO_ADDRESS_BASE) return 0;
    if (to->base.words.number == reg) {
        if (to->index.words.number == reg || (operand->flags & 0x300)) return 0;
        if (apply) {
            saved = to->base;
            to->base = to->index;
            to->index = saved;
        }
    }
    source_scale = (source->flags >> 8) & 3;
    scale = (operand->flags >> 8) & 3;
    if (from->base.words.number != NO_ADDRESS_BASE || source_scale + scale > 3) return 0;
    if (apply) {
        to->index = from->index;
        to->first.displacement = (long)((unsigned long)to->first.displacement + ((unsigned long)from->first.displacement << scale));
        operand->flags = (operand->flags & ~0x300U) | ((source_scale + scale) << 8);
        if (symbol) to->symbol = symbol;
    }
    return 1;
}

/* Original [00068cc0,00068d00). Visit explicit operands in reverse order;
   conditional-code and call-metadata payloads are not symbol operands. */
void __fastcall adjust_machine_symbol_references(MachineInstruction *instruction, int delta)
{
    int count;
    if (instruction->opcode & 15) return;
    count = machine_operand_counts[(unsigned char)(instruction->opcode >> 8)];
    if (count == 2) adjust_operand_label_references(&instruction->operands[1], delta);
    if (count == 1 || count == 2) adjust_operand_label_references(&instruction->operands[0], delta);
}

/* Original [00068c70,00068cc0). Return whether this is a symbol-bearing
   operand, adjusting only local-label references within that operand. */
int __fastcall adjust_operand_label_references(MachineOperand *operand, int delta)
{
    unsigned kind = operand->flags & 15;
    SymbolHeader *symbol;
    if (kind >= OPERAND_ABSOLUTE && kind <= OPERAND_LABEL_ADDRESS) {
        symbol = operand->value.address->symbol;
        if (symbol && symbol->kind == 3) ((LabelSymbol *)symbol)->references += delta;
        return 1;
    }
    if (kind == OPERAND_SYMBOL) {
        symbol = operand->value.symbol;
        if (symbol->kind == 3) ((LabelSymbol *)symbol)->references += delta;
        return 1;
    }
    return 0;
}
