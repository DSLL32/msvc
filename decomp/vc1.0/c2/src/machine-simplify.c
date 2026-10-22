/* Whole-stream simplification passes before physical register assignment. */
#include "instruction-selection.h"
#include "backend-passes.h"
#include "machine-analysis.h"
#include "transformation-cursor.h"
#include "register-variables.h"
#include "temporaries.h"
#include "instruction-rewrite.h"
extern int optimize_precision, cpu_model, optimization_immediate_registers;
unsigned short immediate_register_cutoff; /* 0007e2ac */

/* Original [000358b0,00035920). Floating data preparation discards old
   per-instruction use links and records whether a protected block occurs. */
void simplify_machine_data(void)
{
    MachineInstruction *instruction;
    unsigned kind;
    int has_protected_blocks = 0;
    if (!machine_instructions || !(current_function->field28 & 8) || optimize_precision) return;
    for (instruction = machine_instructions; instruction; instruction = instruction->next) {
        kind = instruction->opcode & 15;
        if (kind == 0 || kind == 8 || kind == 9 || kind == 10) instruction->record.next_use = 0;
        if (kind == 7 && (((MachineBlockHeader *)instruction)->flags & 1)) has_protected_blocks = 1;
    }
    prepare_float_machine_data(has_protected_blocks);
    rewrite_float_machine_data();
}

/* Original [000359b0,000359e0). The global floating peephole driver
   reuses the selected-block helper with a null block and global mode0. */
void simplify_machine_branches(void)
{
    if (!machine_instructions || !(current_function->field28 & 8)) return;
    if (!optimize_precision) prepare_float_stream_uses();
    peephole_selected_block(0, 0);
}

/* Original [0004b350,0004b380). The -noimmreg switch disables this i486/Pentium
   immediate-register pass. Preserve its register cutoff and null-child call. */
void combine_machine_instructions(void)
{
    if (cpu_model <= 3 || !optimization_immediate_registers) return;
    immediate_register_cutoff = next_tree_register;
    promote_region_immediates(machine_region_root->children);
}

/* Original [0004b380,0004b950). Count eligible constants outside child and
   protected blocks, promote at most four frequent values, then replace their
   uses with new tree-class registers. Recursion reports a barrier upward. */
int __fastcall promote_region_immediates(MachineFlowRegion *regions)
{
    enum { MAX_CANDIDATES = 16, MAX_PROMOTED = 4 };
    typedef struct ImmediateCandidate {
        unsigned kind, value;
        SymbolHeader *symbol;
        long displacement;
        unsigned short number, reserved;
        int count;
    } ImmediateCandidate;
    typedef char CandidateSizeCheck[sizeof(ImmediateCandidate) == 24 ? 1 : -1];
    ImmediateCandidate candidates[MAX_CANDIDATES], selected[MAX_PROMOTED];
    MachineFlowRegion *region, *child, *active;
    MachineInstruction *instruction, *end, *child_start, *copy, *created;
    MachineOperand *operand, register_operand, literal_operand;
    MachineAddress *address;
    SymbolHeader *home;
    unsigned kind, operation, operand_kind, destination_kind;
    unsigned short number;
    int candidate_count, selected_count, index, best, i, result = 1, eligible;
    for (region = regions; region; region = region->next) {
        if (!promote_region_immediates(region->children) || (region->flags & 1)) {
            result = 0;
            continue;
        }
        candidate_count = 0;
        child = region->children;
        end = region->last->next;
        child_start = child ? child->first : end;
        instruction = region->first;
        while (instruction != end) {
            if (instruction == child_start) {
                instruction = child->last->next;
                child = child->next;
                child_start = child ? child->first : end;
                continue;
            }
            kind = instruction->opcode & 15;
            if (kind == 7 && (((MachineBlockHeader *)instruction)->flags & 2)) {
                instruction = next_machine_block(instruction);
                continue;
            }
            if (kind != 0 && kind != 8 && kind != 9 && kind != 10) goto next_candidate;
            operation = (unsigned char)(instruction->opcode >> 8);
            if (machine_operand_counts[operation] != 2) goto next_candidate;
            operand = &instruction->operands[1];
            operand_kind = operand->flags & 15;
            if (operand_kind != OPERAND_IMMEDIATE && operand_kind != OPERAND_LABEL_ADDRESS) goto next_candidate;
            if (operation == MC_SHL || operation == MC_SHR || operation == MC_SAR) goto next_candidate;
            if (operation == MC_ADD && operand_kind == OPERAND_IMMEDIATE &&
                (operand->value.constant == 1 || operand->value.constant == ~0U)) goto next_candidate;
            destination_kind = instruction->operands[0].flags & 15;
            eligible = 0;
            if (destination_kind == OPERAND_ABSOLUTE || destination_kind == OPERAND_BASED || destination_kind == OPERAND_INDEXED) {
                address = instruction->operands[0].value.address;
                eligible = address->symbol || address->first.displacement;
            }
            if (!eligible && (destination_kind != OPERAND_REGISTER ||
                !(instruction->operands[0].value.reg.words.number & 0x4000) ||
                (instruction->operands[0].value.reg.words.number & 0x3fff) < immediate_register_cutoff)) goto next_candidate;
            for (index = 0; index < candidate_count; ++index) {
                if (candidates[index].kind != operand_kind) continue;
                if (operand_kind == OPERAND_IMMEDIATE) {
                    if (candidates[index].value == operand->value.constant) break;
                } else if (candidates[index].symbol == operand->value.address->symbol &&
                           candidates[index].displacement == operand->value.address->first.displacement) break;
            }
            if (index < candidate_count) ++candidates[index].count;
            else if (candidate_count < MAX_CANDIDATES) {
                ++candidate_count;
                candidates[index].kind = operand_kind;
                candidates[index].value = 0;
                candidates[index].symbol = 0;
                candidates[index].displacement = 0;
                candidates[index].number = candidates[index].reserved = 0;
                candidates[index].count = 1;
                if (operand_kind == OPERAND_IMMEDIATE) candidates[index].value = operand->value.constant;
                else {
                    candidates[index].symbol = operand->value.address->symbol;
                    candidates[index].displacement = operand->value.address->first.displacement;
                }
            }
next_candidate:
            instruction = instruction->next;
        }
        if (!candidate_count) continue;
        selected_count = candidate_count < MAX_PROMOTED ? candidate_count : MAX_PROMOTED;
        if (candidate_count <= MAX_PROMOTED) {
            for (i = 0; i < selected_count; ++i) selected[i] = candidates[i];
        } else {
            for (i = 0; i < MAX_PROMOTED; ++i) {
                best = 0;
                for (index = 1; index < candidate_count; ++index)
                    if (candidates[index].count > candidates[best].count) best = index;
                candidates[best].count = 0;
                selected[i] = candidates[best];
            }
        }
        prepare_machine_region_transform(region);
        prepare_region_insertion(region);
        for (i = 0; i < selected_count; ++i) {
            active = active_machine_region;
            number = next_tree_register | 0x4000;
            ++next_tree_register;
            home = (SymbolHeader *)allocate_stack_temporary(4);
            record_register_variable_range(number, (short)active->first_block, (short)active->last_block,
                0, 0, 4, home, 0);
            initialize_operand(&register_operand, OPERAND_REGISTER, number, 0, 0);
            selected[i].number = number;
            if (selected[i].kind == OPERAND_IMMEDIATE)
                initialize_operand(&literal_operand, OPERAND_IMMEDIATE, 0, selected[i].value, 0);
            else initialize_operand(&literal_operand, OPERAND_LABEL_ADDRESS, 0,
                    (unsigned)selected[i].displacement, selected[i].symbol);
            created = new_machine_record(0, MC_MOV, 0x104, &register_operand, &literal_operand);
            transformation_cursor.instruction = insert_machine_after(transformation_cursor.instruction, created);
            record_machine_definition_range(number, transformation_cursor.instruction, transformation_cursor.instruction);
            record_region_register(region, number, 1, 0);
        }
        for (instruction = region->first->next; instruction != region->last; instruction = instruction->next) {
            kind = instruction->opcode & 15;
            if (kind != 0 && kind != 8 && kind != 9 && kind != 10) continue;
            operation = (unsigned char)(instruction->opcode >> 8);
            if (operation == MC_SHL || operation == MC_SHR || operation == MC_SAR) continue;
            if (operation == MC_ADD && (instruction->operands[1].flags & 15) == OPERAND_IMMEDIATE &&
                (instruction->operands[1].value.constant == 1 || instruction->operands[1].value.constant == ~0U)) continue;
            operand_kind = instruction->operands[1].flags & 15;
            eligible = machine_operand_counts[operation] == 2 &&
                (operand_kind == OPERAND_IMMEDIATE || operand_kind == OPERAND_LABEL_ADDRESS || operand_kind == 7) &&
                ((instruction->operands[0].flags & 15) != OPERAND_REGISTER ||
                 instruction->operands[0].value.reg.words.number != REG_ESP);
            if (!eligible) {
                operand_kind = instruction->operands[0].flags & 15;
                if (operation != MC_PUSH || (operand_kind != OPERAND_IMMEDIATE &&
                    operand_kind != OPERAND_LABEL_ADDRESS && operand_kind != 7)) continue;
            }
            operand = &instruction->operands[operation == MC_PUSH ? 0 : 1];
            operand_kind = operand->flags & 15;
            for (index = 0; index < selected_count; ++index) {
                if (selected[index].kind != operand_kind) continue;
                if (operand_kind == OPERAND_IMMEDIATE) {
                    if (selected[index].value != operand->value.constant) continue;
                } else if (operand_kind != OPERAND_LABEL_ADDRESS ||
                           selected[index].symbol != operand->value.address->symbol ||
                           selected[index].displacement != operand->value.address->first.displacement) continue;
                if (instruction->opcode & 0xff000000) copy = clone_machine_record(instruction);
                operand->flags = (operand->flags & ~15U) | OPERAND_REGISTER;
                operand->value.reg.words.number = selected[index].number;
                operand->value.reg.words.flags = 1;
                if (instruction->opcode & 0xff000000) {
                    replace_machine_definition(instruction->opcode >> 24, copy, instruction);
                    release_machine_instruction(copy);
                }
                break;
            }
        }
    }
    return result;
}

/* Original [00066e20,00066e30). This build retains an empty pass hook. */
void finish_tree_optimization(void)
{
}
