/* x87 instruction peepholes. Integer control flow and operand rewrites are C. */
#include "float-peepholes.h"
#include "instruction-selection.h"
#include "instruction-rewrite.h"
#include "value-selection.h"
#include "scheduler.h"
#include "register-use.h"
#include "extended.h"
#include <recon.h>
extern int cpu_model, optimize_precision;

/* Six encountered arithmetic relationships. Original normal/pop maps are
  82d90..82da8 and82da8..82dc0; right-hand/pop/integer/reversal maps are
  82f60..82f78,82f78..82f90,82f90..82fa8. These describe ISA operations,
   not unrelated data in the surrounding module. */
typedef struct X87ArithmeticForms {
    unsigned normal, popping, right_popping, integer, right_integer, reversed;
} X87ArithmeticForms;
typedef char X87ArithmeticFormsSizeCheck[sizeof(X87ArithmeticForms) == 24 ? 1 : -1];
static const X87ArithmeticForms x87_arithmetic_forms[6] = {
    {MC_FADD, MC_FADDP, MC_FADDP, MC_FIADD, MC_FIADD, MC_FADD},
    {MC_FMUL, MC_FMULP, MC_FMULP, MC_FIMUL, MC_FIMUL, MC_FMUL},
    {MC_FSUB, MC_FSUBP, MC_FSUBRP, MC_FISUB, MC_FISUBR, MC_FSUBR},
    {MC_FSUBR, MC_FSUBRP, MC_FSUBP, MC_FISUBR, MC_FISUB, MC_FSUB},
    {MC_FDIV, MC_FDIVP, MC_FDIVRP, MC_FIDIV, MC_FIDIVR, MC_FDIVR},
    {MC_FDIVR, MC_FDIVRP, MC_FDIVP, MC_FIDIVR, MC_FIDIV, MC_FDIV}
};
/* Original82dc8..82e08: the condition after exchanging floating operands.
   This DWORD map is distinct from the byte map used by integer matching. */
enum X87Condition { XCC_O, XCC_NO, XCC_B, XCC_AE, XCC_E, XCC_NE, XCC_BE, XCC_A,
    XCC_S, XCC_NS, XCC_P, XCC_NP, XCC_L, XCC_GE, XCC_LE, XCC_G };
static const unsigned exchanged_x87_condition[16] = {
    XCC_NO, XCC_O, XCC_A, XCC_BE, XCC_E, XCC_NE, XCC_AE, XCC_B,
    XCC_NS, XCC_S, XCC_P, XCC_NP, XCC_G, XCC_LE, XCC_GE, XCC_L
};
#define FP_KIND(i) ((i)->opcode & 15)
#define FP_OPERATION(i) (((i)->opcode >> 8) & 255)
#define FP_WIDTH(i) (((i)->opcode >> 4) & 15)
#define FP_ELIGIBLE(i) (FP_KIND(i) == 0 || FP_KIND(i) == 8 || FP_KIND(i) == 9 || FP_KIND(i) == 10)
#define FP_FLOAT(i) (FP_ELIGIBLE(i) && ((FP_OPERATION(i) >= MC_FADD && FP_OPERATION(i) <= MC_FLD1) || \
    (FP_OPERATION(i) >= MC_FUCOM && FP_OPERATION(i) <= MC_FCOS)))
#define FP_MEMORY(o) (((o)->flags & 15) == OPERAND_ABSOLUTE || ((o)->flags & 15) == OPERAND_BASED || ((o)->flags & 15) == OPERAND_INDEXED)
#define FP_REG(o) ((o)->value.constant & 0x3fff)
#define FP_OTHER_STORAGE(o) (FP_MEMORY(o) || (((o)->flags & 15) == OPERAND_REGISTER && FP_REG(o) > REG_ST0 + 7))
#define FP_SET_OPERATION(i,op) ((i)->opcode = ((i)->opcode & ~0xff00UL) | ((unsigned)(op) << 8))
#define FP_SET_WIDTH(i,width) ((i)->opcode = ((i)->opcode & ~0xf0UL) | ((unsigned)(width) << 4))
#define FP_ARITHMETIC(i) (FP_OPERATION(i) >= MC_FADD && FP_OPERATION(i) <= MC_FDIVR)
#define FP_POP_ARITHMETIC(i) (FP_OPERATION(i) >= MC_FADDP && FP_OPERATION(i) <= MC_FDIVRP)
#define FP_FORMS(i) x87_arithmetic_forms[FP_OPERATION(i) - MC_FADD]
#define FP_POP_FORMS(i) x87_arithmetic_forms[FP_OPERATION(i) - MC_FADDP]
#define FP_LABEL_BARRIER(i) (!(i)->record.symbol || ((LabelSymbol *)(i)->record.symbol)->references || \
    backend_stage == 5 || ((LabelSymbol *)(i)->record.symbol)->label_kind == 4)
#define FP_BARRIER(i) ((FP_ELIGIBLE(i) && (FP_OPERATION(i) == MC_JCC || FP_OPERATION(i) == MC_JMP || \
    FP_OPERATION(i) == MC_CALL || FP_OPERATION(i) == MC_LEAVE || FP_OPERATION(i) == MC_RET)) || \
    (FP_KIND(i) == 1 && FP_LABEL_BARRIER(i)))
/* The old shared operand table gives ST(n-1) or ST(n+1) by index.
   Copies do not expose its address; express its register encoding directly. */
#define FP_SHIFT_REGISTER(destination,number,delta) do { \
    (destination).flags = OPERAND_REGISTER; \
    (destination).value.constant = (unsigned)((int)(number) + (delta)); \
} while (0)
#define FP_REVERSE_BRANCH(i) ((i)->operands[1].flags = ((i)->operands[1].flags & ~15U) | \
    exchanged_x87_condition[(i)->operands[1].flags & 15])
#define FP_FOLD_LITERAL_LOAD(i) do { \
    if (FP_OPERATION(i) == MC_FLD && ((i)->operands[1].flags & 15) == OPERAND_ABSOLUTE && \
        (((BackendSymbol *)(i)->operands[1].value.address->symbol)->attributes & 0x70) == 0x40) { \
        number = floating_value_for_symbol((i)->operands[1].value.address->symbol); \
        if (number) { \
            if (compare_extended(1, number, &extended_zero)) { \
                if (cpu_model == 3 || (cpu_model >= 4 && FP_WIDTH(i) == 10)) FP_SET_OPERATION(i, MC_FLDZ); \
            } else if (compare_extended(1, number, &extended_one) && \
                ((cpu_model == 3 && FP_WIDTH(i) > 4) || (cpu_model >= 4 && FP_WIDTH(i) == 10))) \
                FP_SET_OPERATION(i, MC_FLD1); \
        } \
    } \
} while (0)

/* Original [000359e0,0003771c). Scan a selected block, or the complete
   instruction stream, combining x87 loads, arithmetic, comparisons and stores.
   The previous floating instruction survives ordinary integer instructions;
   control-flow/label barriers reset it. */
void __fastcall peephole_selected_block(MachineBlockHeader *block, int global)
{
    MachineInstruction *instruction, *last = 0, *scan, *before, *load, *transfer;
    MachineInstruction *anchor, *created, *push, *increment, *register_move;
    MachineOperand temporary, *source, *operand, *register_operand;
    MachineAddress *address;
    const Extended *number;
    unsigned operation, width, reg;
    int delete_previous = 0, precise = 0, disabled = 0, stop, depth, reversed;
    unsigned short temporary_register;
    temporary_operand_count = 0;
    if (block) {
        if (block->flags & 2) return;
        precise = (block->flags & 1) != 0;
        instruction = block->next;
    } else instruction = machine_instructions;
    while (instruction) {
        if (block && FP_KIND(instruction) == 7) return;
        if (FP_ELIGIBLE(instruction) && !block) instruction->record.next_use = 0;
        if (FP_KIND(instruction) == 7) {
            disabled = (((MachineBlockHeader *)instruction)->flags & 2) != 0;
            precise = (((MachineBlockHeader *)instruction)->flags & 1) != 0;
        }
        if (FP_BARRIER(instruction) || (disabled && FP_KIND(instruction) == 7) ||
            (block && FP_KIND(instruction) == 1)) {
            if (!global && last) FP_FOLD_LITERAL_LOAD(last);
            last = 0;
        }
        if (disabled || !FP_FLOAT(instruction)) goto next_instruction;
        if (!last) goto remember_float;
        /* Replace equal-width reloads by ST0 when the memory has not changed. */
        if (FP_WIDTH(last) == 10 && FP_WIDTH(instruction) == 10 &&
            !(instruction->operands[1].flags & 0x8000) && FP_OPERATION(instruction) == MC_FLD &&
            FP_OPERATION(last) == MC_FLD && operands_equal(&last->operands[1], &instruction->operands[1]) &&
            !operand_changed_between(last, instruction, &last->operands[1], 10)) {
            instruction->operands[1] = operand_st0;
            goto remember_float;
        }
        if (cpu_model < 4 && FP_ARITHMETIC(instruction) &&
            FP_WIDTH(last) == FP_WIDTH(instruction) && !(instruction->operands[1].flags & 0x8000) &&
            FP_OPERATION(last) == MC_FLD && (((instruction->operands[1].flags & 15) != OPERAND_REGISTER) ||
            FP_REG(&instruction->operands[1]) > 21 || FP_REG(&instruction->operands[1]) < 14) &&
            operands_equal(&last->operands[1], &instruction->operands[1]) &&
            !operand_changed_between(last, instruction, &last->operands[1], FP_WIDTH(last))) {
            instruction->operands[1] = operand_st0;
            goto remember_float;
        }
        if (cpu_model == 3 && FP_ARITHMETIC(instruction) && FP_WIDTH(last) == 10 &&
            (instruction->operands[1].flags & 15) == OPERAND_REGISTER &&
            FP_REG(&instruction->operands[1]) > 14 && FP_REG(&instruction->operands[1]) < 22 &&
            FP_OPERATION(last) == MC_FLD) {
            scan = instruction->next;
            if (!(FP_KIND(scan) == 0 && FP_OPERATION(scan) == MC_FSTP &&
                  operands_equal(&instruction->operands[1], &scan->operands[0])) &&
                FP_MEMORY(&last->operands[1]) && !(last->operands[1].flags & 0x8000) &&
                !operand_changed_between(last, instruction, &last->operands[1], 10)) {
                reg = FP_REG(&instruction->operands[1]);
                FP_SET_OPERATION(instruction, FP_FORMS(instruction).reversed);
                instruction->operands[1] = last->operands[1];
                FP_SHIFT_REGISTER(last->operands[1], reg, -1);
                FP_SET_WIDTH(instruction, FP_WIDTH(last));
                goto remember_float;
            }
        }
        if (FP_ARITHMETIC(last) && FP_OPERATION(instruction) == MC_FSTP &&
            (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
            FP_REG(&instruction->operands[0]) >= 14 && FP_REG(&instruction->operands[0]) < 22 &&
            operands_equal(&last->operands[1], &instruction->operands[0])) {
            FP_SET_OPERATION(last, FP_FORMS(last).right_popping);
            temporary = last->operands[0];
            last->operands[0] = last->operands[1]; last->operands[1] = temporary;
            delete_previous = 1;
            if (operands_equal(&last->operands[0], &operand_st1)) {
                before = last->previous;
                fold_previous_float_load(before, last);
            }
            goto remember_float;
        }
        if (FP_ARITHMETIC(instruction) && !optimize_precision && !precise &&
            fold_float_compare_store(instruction, 0)) goto remember_float;
        if (cpu_model > 3 && FP_ARITHMETIC(instruction) && FP_OPERATION(instruction) != MC_FMUL &&
            FP_MEMORY(&instruction->operands[1]) && !(instruction->operands[1].flags & 0x8000) &&
            !operand_changed_between(last, instruction, &instruction->operands[1], FP_WIDTH(instruction)) &&
            FP_OPERATION(last) == MC_FLD && (last->operands[1].flags & 15) == OPERAND_REGISTER &&
            FP_REG(&last->operands[1]) >= 14 && FP_REG(&last->operands[1]) <= 20) {
            reg = FP_REG(&last->operands[1]);
            FP_SET_OPERATION(instruction, FP_FORMS(instruction).reversed);
            last->operands[1] = instruction->operands[1];
            FP_SHIFT_REGISTER(instruction->operands[1], reg, 1);
            FP_SET_WIDTH(last, FP_WIDTH(instruction));
            goto remember_float;
        }
        if (FP_OPERATION(instruction) == MC_FSTP &&
            (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
            FP_REG(&instruction->operands[0]) == REG_ST0 &&
            (FP_OPERATION(last) == MC_FLD || FP_OPERATION(last) == MC_FLDZ ||
             FP_OPERATION(last) == MC_FLD1 || FP_OPERATION(last) == MC_FILD)) {
            delete_previous = 1;
            remove_machine_instruction(last);
        } else if (FP_OPERATION(last) == MC_FLD && operands_equal(&last->operands[1], &operand_st0) &&
                   FP_WIDTH(instruction) != 10 && FP_OPERATION(instruction) == MC_FSTP) {
            if (FP_MEMORY(&instruction->operands[0])) {
                remove_machine_instruction(last);
                FP_SET_OPERATION(instruction, MC_FST);
            } else if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                       FP_REG(&instruction->operands[0]) > 15 && FP_REG(&instruction->operands[0]) < 22) {
                remove_machine_instruction(last);
                FP_SET_OPERATION(instruction, MC_FST);
                reg = FP_REG(&instruction->operands[0]);
                FP_SHIFT_REGISTER(instruction->operands[0], reg, -1);
            }
        } else if (FP_OPERATION(last) == MC_FST && FP_OPERATION(instruction) == MC_FSTP &&
                   operands_equal(&instruction->operands[0], &operand_st0)) {
            FP_SET_OPERATION(last, MC_FSTP);
            delete_previous = 1;
        } else if (FP_POP_ARITHMETIC(instruction) && FP_REG(&instruction->operands[0]) == REG_ST0 + 1) {
            /* Pop-form arithmetic can consume an earlier load directly. */
            source = &last->operands[1];
            if (FP_WIDTH(last) != 10 && (FP_OPERATION(last) == MC_FLD || FP_OPERATION(last) == MC_FILD) &&
                FP_OTHER_STORAGE(source) && !operand_changed_between(last, instruction, source, FP_WIDTH(last))) {
                if (cpu_model == 5 && FP_OPERATION(last) == MC_FILD) {
                    scan = last->previous; stop = 0;
                    while (scan) {
                        if (FP_BARRIER(scan) || stop) break;
                        if (FP_FLOAT(scan)) stop = 1;
                        else scan = scan->previous;
                    }
                    if (stop && FP_OPERATION(scan) == MC_FLD && FP_WIDTH(scan) != 10 &&
                        FP_OTHER_STORAGE(&scan->operands[1]) &&
                        !operand_changed_between(scan, instruction, &scan->operands[1], FP_WIDTH(scan))) {
                        FP_SET_OPERATION(instruction, FP_POP_FORMS(instruction).reversed);
                        instruction->operands[0] = operand_st0;
                        instruction->operands[1] = scan->operands[1];
                        FP_SET_WIDTH(instruction, FP_WIDTH(scan));
                        remove_machine_instruction(scan);
                    }
                } else if (FP_OPERATION(last) != MC_FILD || FP_WIDTH(last) != 8) {
                    instruction->operands[1] = *source;
                    instruction->operands[0] = operand_st0;
                    operation = FP_OPERATION(last) == MC_FILD ? FP_POP_FORMS(instruction).integer : FP_POP_FORMS(instruction).normal;
                    FP_SET_OPERATION(instruction, operation);
                    FP_SET_WIDTH(instruction, FP_WIDTH(last));
                    remove_machine_instruction(last);
                    if (!optimize_precision && !precise) fold_float_compare_store(instruction, 0);
                }
            } else if (FP_OPERATION(last) == MC_FLD && operands_equal(&operand_st0, source) &&
                       operands_equal(&operand_st1, &instruction->operands[0])) {
                FP_SET_OPERATION(instruction, FP_POP_FORMS(instruction).normal);
                instruction->operands[0] = operand_st0;
                remove_machine_instruction(last);
                goto remember_float;
            } else if (FP_OPERATION(last) == MC_FXCH && operands_equal(&operand_st1, &last->operands[0]) &&
                       operands_equal(&operand_st1, &instruction->operands[0])) {
                FP_SET_OPERATION(instruction, FP_POP_FORMS(instruction).right_popping);
                before = last->previous;
                remove_machine_instruction(last);
                fold_previous_float_load(before, instruction);
            } else if (global && FP_WIDTH(instruction) != 10) {
                /* Search backwards for the second live stack producer. */
                scan = last->previous; stop = 0; depth = 2;
                while (scan) {
                    if (FP_BARRIER(scan) || stop) break;
                    if (!depth) break;
                    if (FP_FLOAT(scan)) {
                        operation = FP_OPERATION(scan);
                        if (operation == MC_FLD || operation == MC_FBLD || operation == MC_FLDZ || operation == MC_FILD) {
                            if (!--depth) continue;
                        } else if (operation == MC_FST) {
                            if (depth == 1 && global && FP_OTHER_STORAGE(&scan->operands[0])) {
                                depth = 0; continue;
                            }
                        } else if (FP_POP_ARITHMETIC(scan)) {
                            if (depth == 1) { stop = 1; continue; }
                            ++depth;
                        } else if (operation == MC_FXCH || depth == 1) { stop = 1; continue; }
                    }
                    scan = scan->previous;
                }
                if (!depth) goto combine_pop_operand;
            }
        } else {
            /* Remaining compare and load/store transfer patterns. */
            goto compare_and_transfer;
        }
        goto remember_float;
compare_and_transfer:
        if (combine_float_store_load(last, instruction)) goto remember_float;
        /* A memory comparison may consume a load and reverse its condition. */
        if ((FP_OPERATION(instruction) == MC_FCOMPP ||
            (FP_OPERATION(instruction) == MC_FCOMP && operands_equal(&instruction->operands[0], &operand_st0) &&
             operands_equal(&instruction->operands[1], &operand_st1))) && FP_WIDTH(last) != 10 &&
            !operand_changed_between(last, instruction, &last->operands[1], FP_WIDTH(last)) &&
            FP_OPERATION(last) == MC_FLD) {
            for (scan = instruction->next; scan; scan = scan->next) {
                if (FP_ELIGIBLE(scan) && (FP_OPERATION(scan) == MC_JCC || FP_OPERATION(scan) == MC_SCC)) break;
                if (FP_KIND(scan) == 1 || FP_KIND(scan) == 7 ||
                    (FP_ELIGIBLE(scan) && (FP_OPERATION(scan) == MC_JMP || FP_OPERATION(scan) == MC_CMP ||
                     (FP_OPERATION(scan) >= MC_FCOM && FP_OPERATION(scan) <= MC_FCOMPP)))) { scan = 0; break; }
            }
            if (scan && FP_OTHER_STORAGE(&last->operands[1])) {
                operation = FP_OPERATION(instruction) == MC_FCOMPP ? MC_FCOMP : MC_FCOM;
                FP_SET_OPERATION(instruction, operation);
                instruction->operands[1] = last->operands[1];
                instruction->operands[0] = operand_st0;
                FP_SET_WIDTH(instruction, FP_WIDTH(last));
                FP_REVERSE_BRANCH(scan);
                remove_machine_instruction(last);
                if (operation == MC_FCOMP && !optimize_precision && !precise &&
                    !fold_float_compare_store(instruction, scan)) {
                    before = previous_float_load(instruction);
                    if (before && FP_OPERATION(before) == MC_FST) combine_float_store_load(before, instruction);
                }
            }
            goto remember_float;
        }
        if (FP_OPERATION(last) == MC_FSTP && FP_OPERATION(instruction) == MC_FLD &&
            FP_WIDTH(last) != 10 && FP_WIDTH(last) == FP_WIDTH(instruction) &&
            FP_OTHER_STORAGE(&last->operands[0]) && can_reuse_float_load_width(instruction, FP_WIDTH(instruction)) &&
            (global || can_reuse_float_load(instruction)) &&
            !operand_changed_between(last, instruction, &last->operands[0], FP_WIDTH(last)) &&
            !precise && !optimize_precision && !(last->operands[0].flags & 0x8000) &&
            operands_equal(&last->operands[0], &instruction->operands[1])) {
            if ((instruction->operands[1].flags & 15) == OPERAND_REGISTER && (instruction->operands[1].flags & 0x400))
                remove_machine_instruction(last);
            else FP_SET_OPERATION(last, MC_FST);
            delete_previous = 1;
            goto remember_float;
        }
        if (FP_OPERATION(instruction) == MC_FSTP &&
            (instruction->operands[0].flags & 15) == OPERAND_REGISTER && FP_REG(&instruction->operands[0]) > 21) {
            before = instruction->previous;
            if (FP_ELIGIBLE(before) && FP_OPERATION(before) == MC_FLD && FP_MEMORY(&before->operands[1])) {
                transfer = find_float_transfer_match(instruction);
                if (transfer) {
                    last = transfer->previous;
                    before = instruction->previous;
                    split_float_memory_transfer(before, transfer, before, transfer, 0, 0);
                    remove_machine_instruction(instruction);
                    remove_machine_instruction(last);
                    instruction = transfer;
                    goto remember_float;
                }
            }
        }
        if (FP_OPERATION(instruction) == MC_FSTP) {
            if (FP_OPERATION(last) == MC_FLD && FP_WIDTH(last) == FP_WIDTH(instruction) && FP_MEMORY(&last->operands[1])) {
                load = last; anchor = last; reversed = 0;
                goto merge_load_store;
            }
            if (FP_OPERATION(last) == MC_FCOM) {
                before = previous_float_load(last);
                if (before && FP_OPERATION(before) == MC_FLD && FP_WIDTH(before) == FP_WIDTH(instruction) &&
                    FP_MEMORY(&before->operands[1]) &&
                    !operand_changed_between(before, instruction, &before->operands[1], FP_WIDTH(before))) {
                    load = before; anchor = instruction; reversed = 1;
                    goto merge_load_store;
                }
            }
        }
        if (!global && FP_OPERATION(last) == MC_FLD &&
            (last->operands[1].flags & 15) == OPERAND_ABSOLUTE &&
            (((BackendSymbol *)last->operands[1].value.address->symbol)->attributes & 0x70) == 0x40) {
            FP_FOLD_LITERAL_LOAD(last);
        } else if ((FP_ARITHMETIC(instruction) || FP_OPERATION(instruction) == MC_FCOMP) &&
                   FP_OPERATION(last) == MC_FLD && FP_WIDTH(last) != 10 && FP_MEMORY(&last->operands[1]) &&
                   !operand_changed_between(last, instruction, &last->operands[1], FP_WIDTH(last)) &&
                   (instruction->operands[1].flags & 15) == OPERAND_ABSOLUTE &&
                   (((BackendSymbol *)instruction->operands[1].value.address->symbol)->attributes & 0x70) == 0x40) {
            number = floating_value_for_symbol(instruction->operands[1].value.address->symbol);
            if (number) {
                stop = compare_extended(1, number, &extended_zero);
                if (stop || (compare_extended(1, number, &extended_one) && FP_WIDTH(instruction) > 4)) {
                    if (FP_OPERATION(instruction) != MC_FCOMP) {
                        FP_SET_OPERATION(instruction, FP_FORMS(instruction).reversed);
                        FP_SET_WIDTH(instruction, FP_WIDTH(last));
                        instruction->operands[1] = last->operands[1];
                        FP_SET_OPERATION(last, stop ? MC_FLDZ : MC_FLD1);
                    } else {
                        for (scan = instruction->next; scan; scan = scan->next) {
                            if (FP_ELIGIBLE(scan) && (FP_OPERATION(scan) == MC_JCC || FP_OPERATION(scan) == MC_SCC)) break;
                            if (FP_KIND(scan) == 1 || FP_KIND(scan) == 7 ||
                                (FP_ELIGIBLE(scan) && (FP_OPERATION(scan) == MC_JMP || FP_OPERATION(scan) == MC_CMP ||
                                 (FP_OPERATION(scan) >= MC_FCOM && FP_OPERATION(scan) <= MC_FCOMPP)))) { scan = 0; break; }
                        }
                        if (scan) {
                            FP_SET_WIDTH(instruction, FP_WIDTH(last));
                            instruction->operands[1] = last->operands[1];
                            FP_SET_OPERATION(last, stop ? MC_FLDZ : MC_FLD1);
                            FP_REVERSE_BRANCH(scan);
                        }
                    }
                }
            }
        }
        goto remember_float;

combine_pop_operand:
        /* scan identifies the earlier stack producer found at depth zero. */
        operation = FP_OPERATION(scan);
        if (operation == MC_FST) source = &scan->operands[0];
        else if (operation == MC_FLD || (operation == MC_FILD && FP_WIDTH(scan) != 8))
            source = &scan->operands[1];
        else goto remember_float;
        if (FP_WIDTH(scan) != 10 && !(source->flags & 0x8000) && FP_OTHER_STORAGE(source) &&
            !operand_changed_between(scan, instruction, source, FP_WIDTH(scan))) {
            assign_machine_operand(&instruction->operands[1], source);
            instruction->operands[0] = operand_st0;
            if (FP_OPERATION(scan) == MC_FST) FP_SET_OPERATION(scan, MC_FSTP);
            operation = FP_OPERATION(scan);
            operation = operation == MC_FILD ? FP_POP_FORMS(instruction).right_integer : FP_POP_FORMS(instruction).reversed;
            FP_SET_OPERATION(instruction, operation);
            FP_SET_WIDTH(instruction, FP_WIDTH(scan));
            if ((source->flags & 15) == OPERAND_REGISTER) {
                for (transfer = scan; transfer != instruction; transfer = transfer->next) {
                    if (!FP_FLOAT(transfer)) continue;
                    if (operands_equal(source, &transfer->operands[0]) && (transfer->operands[0].flags & 0x400)) {
                        transfer->operands[0].flags &= ~0x400U;
                        instruction->operands[1].flags |= 0x400;
                    }
                    if (operands_equal(source, &transfer->operands[1]) && (transfer->operands[1].flags & 0x400)) {
                        transfer->operands[1].flags &= ~0x400U;
                        instruction->operands[1].flags |= 0x400;
                    }
                }
            }
            if (FP_OPERATION(scan) != MC_FSTP) remove_machine_instruction(scan);
        }
        goto remember_float;

merge_load_store:
        /* An FLD/FSTP pair through [ESP] may become integer loads and pushes,
           avoiding an x87 round trip and consuming the stack adjustment. */
        if ((instruction->operands[0].flags & 15) == OPERAND_BASED) {
            address = instruction->operands[0].value.address;
            increment = instruction->previous;
            if (address->base.words.number == REG_ESP && !address->first.displacement && !address->symbol &&
                FP_ELIGIBLE(increment) && (FP_OPERATION(increment) == MC_SUB || FP_OPERATION(increment) == MC_ADD) &&
                (increment->operands[0].flags & 15) == OPERAND_REGISTER &&
                increment->operands[0].value.reg.words.number == REG_ESP &&
                (increment->operands[1].flags & 15) == OPERAND_IMMEDIATE &&
                FP_OPERATION(last) == MC_FLD && increment->previous == last) {
                temporary_register = new_value_register(0x100);
                register_operand = operand_for_register(temporary_register);
                register_operand->flags &= ~0x400U;
                width = FP_WIDTH(last);
                if (width == 10) {
                    operand = duplicate_operand(&last->operands[1]);
                    if ((operand->flags & 15) == OPERAND_BASED ||
                        ((operand->flags & 15) == OPERAND_INDEXED &&
                         ((operand->flags &= ~0x800U), operand->value.address->base.words.number != 27)))
                        operand->flags &= ~0x400U;
                    operand->value.address->first.displacement += 8;
                    created = new_machine_instruction(0);
                    created->opcode |= 0x400000;
                    if (cpu_model >= 4 && operand->value.address->symbol &&
                        ((operand->value.address->symbol->kind == 1 &&
                         ((((BackendSymbol *)operand->value.address->symbol)->attributes & 0x70) == 0x10 ||
                          (((BackendSymbol *)operand->value.address->symbol)->attributes & 0x70) == 0x30)) ||
                         operand->value.address->symbol->kind == 19)) FP_SET_WIDTH(created, 4);
                    else FP_SET_WIDTH(created, 2);
                    FP_SET_OPERATION(created, MC_MOV);
                    created->operands[0] = *register_operand;
                    created->operands[1] = *operand;
                    last->previous->next = created; created->previous = last->previous;
                    created->next = last; last->previous = created;
                    push = new_machine_instruction(0);
                    push->opcode |= 0x400000; FP_SET_WIDTH(push, 4); FP_SET_OPERATION(push, MC_PUSH);
                    register_operand = operand_for_register(temporary_register);
                    register_operand->flags |= 0x400;
                    push->operands[0] = *register_operand;
                    created->next = push; push->previous = last->previous;
                    push->next = last; last->previous = push;
                    temporary_register = new_value_register(0x100);
                    register_operand = operand_for_register(temporary_register);
                    register_operand->flags &= ~0x400U;
                    width = 8;
                }
                if (width == 8) {
                    operand = duplicate_operand(&last->operands[1]);
                    if ((operand->flags & 15) == OPERAND_BASED ||
                        ((operand->flags & 15) == OPERAND_INDEXED &&
                         ((operand->flags &= ~0x800U), operand->value.address->base.words.number != 27)))
                        operand->flags &= ~0x400U;
                    operand->value.address->first.displacement += 4;
                    created = new_machine_instruction(0);
                    created->opcode |= 0x400000; FP_SET_WIDTH(created, 4);
                    FP_SET_OPERATION(created, cpu_model == 4 ? MC_MOV : MC_PUSH);
                    created->operands[0] = cpu_model == 4 ? *register_operand : *operand;
                    created->operands[1] = *operand;
                    last->previous->next = created; created->previous = last->previous;
                    created->next = last; last->previous = created;
                    if (cpu_model == 4) {
                        push = new_machine_instruction(0);
                        push->opcode |= 0x400000; FP_SET_WIDTH(push, 4); FP_SET_OPERATION(push, MC_PUSH);
                        register_operand = operand_for_register(temporary_register);
                        register_operand->flags |= 0x400;
                        push->operands[0] = *register_operand;
                        created->next = push; push->previous = last->previous;
                        push->next = last; last->previous = push;
                    }
                }
                if (width != 8 || cpu_model == 4) {
                    if (cpu_model == 4) {
                        temporary_register = new_value_register(0x100);
                        register_operand = operand_for_register(temporary_register);
                        register_operand->flags &= ~0x400U;
                    }
                }
                FP_SET_OPERATION(last, cpu_model == 4 ? MC_MOV : MC_PUSH);
                last->opcode |= 0x400000; FP_SET_WIDTH(last, 4);
                last->operands[0] = cpu_model == 4 ? *register_operand : last->operands[1];
                if (cpu_model == 4) {
                    register_move = new_machine_instruction(0);
                    register_move->opcode |= 0x400000; FP_SET_WIDTH(register_move, 4); FP_SET_OPERATION(register_move, MC_PUSH);
                    register_operand = operand_for_register(temporary_register);
                    register_operand->flags |= 0x400;
                    register_move->operands[0] = *register_operand;
                    register_move->previous = last; register_move->next = last->next;
                    last->next->previous = register_move; last->next = register_move;
                }
                remove_machine_instruction(increment);
                delete_previous = 1;
                goto remember_float;
            }
        }
        if (!global && can_split_float_transfer(load, instruction) && FP_MEMORY(&instruction->operands[0]))
            split_float_memory_transfer(load, instruction, anchor, anchor, reversed, last);
        goto remember_float;

remember_float:
        last = instruction;
next_instruction:
        temporary_operand_count = 0;
        instruction = instruction->next;
        if (delete_previous) {
            remove_machine_instruction(instruction->previous);
            delete_previous = 0; last = 0;
        }
    }
}
