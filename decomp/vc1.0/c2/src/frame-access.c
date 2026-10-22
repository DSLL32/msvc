/* Reference-based optimization of local frame storage. */
#include "frame-access.h"
#include "register-use.h"
#include "instruction-rewrite.h"
/* Original [000351a0,00035210). Only ordinary frames enter collection;
   both lists use the native generic sorter and its separate comparator. */
void __fastcall set_machine_frame(MachineContext *context, unsigned flags)
{
    FrameAccess *uses = 0, *definitions = 0;
    if (!(current_symbol->symbol.field0e & 0x400) &&
        collect_frame_accesses(&uses, &definitions, context, flags)) {
        uses = sort_linked_list(uses, compare_frame_accesses);
        definitions = sort_linked_list(definitions, compare_frame_accesses);
        combine_frame_accesses(uses, definitions);
        reset_arena(4);
    }
}

/* Original [00034e80,00035150). Record symbol-backed frame uses/defs
   with the current ESP depth and protected-block marker. An unbacked indexed
   frame address makes this transformation inapplicable. */
int __fastcall collect_frame_accesses(FrameAccess **uses, FrameAccess **definitions,
    MachineContext *context, unsigned use_stack_pointer)
{
    MachineInstruction *instruction = context;
    MachineOperand *operand;
    MachineAddress *address;
    FunctionSymbol *callee;
    unsigned operation, first_kind, kind, selected, i;
    unsigned short frame_register = use_stack_pointer ? REG_ESP : REG_EBP;
    int depth = 0, blocks_seen = 0, protected_block = 0;
    int probe_bytes = 0;
    char *name;
    for (; instruction; instruction = instruction->next) {
        kind = instruction->opcode & 15;
        if (kind == 7) {
            protected_block = (((MachineBlockHeader *)instruction)->flags & 2) == 2;
            ++blocks_seen;
            continue;
        }
        if (kind != 0) continue;
        operation = (instruction->opcode >> 8) & 255;
        if (!machine_operand_counts[operation]) continue;
        operand = &instruction->operands[0];
        first_kind = operand->flags & 15;
        if (first_kind == OPERAND_BASED && operand->value.address->base.words.number == frame_register &&
            writes_first_operand(instruction)) {
            *definitions = new_frame_access(*definitions, instruction, (unsigned char)1, depth, protected_block);
        } else {
            selected = machine_operand_counts[operation] == 2 && first_kind == OPERAND_REGISTER ? 1 : 0;
            operand = &instruction->operands[selected];
            kind = operand->flags & 15;
            if (kind == OPERAND_BASED || kind == OPERAND_INDEXED) {
                address = operand->value.address;
                if (address->base.words.number == frame_register ||
                    (kind == OPERAND_INDEXED && address->index.words.number == frame_register)) {
                    if (kind == OPERAND_INDEXED && !address->symbol) return 0;
                    *uses = new_frame_access(*uses, instruction, (unsigned char)(selected + 1), depth, protected_block);
                }
            }
        }
        if (!use_stack_pointer) continue;
        switch (operation) {
        case MC_MOV:
            /* A recognized __chkstk call has a preceding prologue MOV EAX,size.
               Initialize the unused default so invalid streams never read junk. */
            if (!blocks_seen && first_kind == OPERAND_REGISTER &&
                instruction->operands[0].value.reg.words.number == REG_EAX &&
                (instruction->operands[1].flags & 15) == OPERAND_IMMEDIATE)
                probe_bytes = instruction->operands[1].value.constant;
            break;
        case MC_ADD:
            if (first_kind == OPERAND_REGISTER && instruction->operands[0].value.reg.words.number == REG_ESP &&
                (instruction->operands[1].flags & 15) == OPERAND_IMMEDIATE)
                depth -= instruction->operands[1].value.constant;
            break;
        case MC_SUB:
            kind = instruction->operands[1].flags & 15;
            if (first_kind == OPERAND_REGISTER && instruction->operands[0].value.reg.words.number == REG_ESP &&
                (kind == OPERAND_IMMEDIATE || kind == OPERAND_FRAME_SIZE))
                depth += instruction->operands[1].value.constant;
            break;
        case MC_POP: case MC_EPOP: depth -= 4; break;
        case MC_PUSH: case MC_EPUSH: depth += 4; break;
        case MC_CALL:
            depth -= instruction->operands[1].value.call->argument_bytes;
            if (!blocks_seen && first_kind == OPERAND_SYMBOL) {
                callee = (FunctionSymbol *)instruction->operands[0].value.symbol;
                if ((callee->field24 & 0x700) == 0x100) {
                    name = callee->symbol.name;
                    /* The native inline comparison covers eight bytes plus NUL. */
                    for (i = 0; i < 9 && name[i] == "__chkstk"[i]; ++i) ;
                    if (i == 9) depth += probe_bytes;
                }
            }
            break;
        }
    }
    if (depth) compiler_internal_error("@(#)pgoMD.c:1.49", 0x8cd);
    return 1;
}

/* Original [00035150,000351a0). Arena4 provides zero-filled16-byte
   storage; selector bits0..1 and protected bit2 are set here, retained bit3
   starts clear and is owned by the later frame-definition combiner. */
FrameAccess *__fastcall new_frame_access(FrameAccess *next, MachineInstruction *instruction,
    unsigned char operand, int depth, int protected_block)
{
    FrameAccess *access = arena_alloc(4, sizeof(FrameAccess));
    access->instruction = instruction;
    access->flags = (unsigned char)((operand & 3) | ((protected_block & 1) << 2));
    access->depth = depth;
    access->next = next;
    return access;
}

/* Original [00034e10,00034e80). Accesses sort by byte displacement;
   address-taking LEAs precede actual reads at the same displacement. */
int __fastcall compare_frame_accesses(void *left_record, void *right_record)
{
    FrameAccess *left = left_record, *right = right_record;
    int distance = frame_access_distance(left, right);
    unsigned left_operation, right_operation;
    if (distance < 0) return -1;
    if (distance > 0) return 1;
    left_operation = (left->instruction->opcode >> 8) & 255;
    right_operation = (right->instruction->opcode >> 8) & 255;
    if (left_operation == MC_LEA && right_operation != MC_LEA) return -1;
    if (right_operation == MC_LEA && left_operation != MC_LEA) return 1;
    return 0;
}

/* Original [00034d00,00034e10). Compare the selected memory bytes after
   accounting for changing ESP depth, symbol placement and high-byte lanes.
   LEA and indexed references describe the whole symbol instead of a byte. */
int __fastcall frame_access_distance(FrameAccess *left, FrameAccess *right)
{
    unsigned left_selector = left->flags & 3, right_selector = right->flags & 3;
    MachineOperand *left_operand = &left->instruction->operands[left_selector == 1 ? 0 : 1];
    MachineOperand *right_operand = &right->instruction->operands[right_selector == 1 ? 0 : 1];
    MachineAddress *left_address = left_operand->value.address;
    MachineAddress *right_address = right_operand->value.address;
    unsigned left_opcode = left->instruction->opcode, right_opcode = right->instruction->opcode;
    long left_offset = 0, right_offset = 0;
    if (((left_opcode >> 8) & 255) != MC_LEA &&
        (left_operand->flags & 15) != OPERAND_INDEXED)
        left_offset = left_address->first.displacement - left->depth;
    if (((right_opcode >> 8) & 255) != MC_LEA &&
        (right_operand->flags & 15) != OPERAND_INDEXED)
        right_offset = right_address->first.displacement - right->depth;
    if (left_address->symbol) left_offset += left_address->symbol->field04;
    if (right_address->symbol) right_offset += right_address->symbol->field04;
    if (left_opcode & (left_selector == 1 ? 0x80000UL : 0x100000UL)) ++left_offset;
    if (right_opcode & (right_selector == 1 ? 0x80000UL : 0x100000UL)) ++right_offset;
    return left_offset - right_offset;
}

/* Original [00035210,000354f0). Process equal-displacement store groups
   against the sorted read list. Flag8 keeps overlapping definitions; flag4
   protects instructions in blocks which cannot discard their stores. */
void __fastcall combine_frame_accesses(FrameAccess *uses, FrameAccess *definitions)
{
    FrameAccess *group_end, *scan, *definition;
    MachineOperand *defined_operand, *read_operand = 0;
    SymbolHeader *symbol;
    unsigned read_bytes = 0, extent = 0, bytes, operation;
    int distance = 0, removable, all_moves, retained;
    while (definitions) {
        defined_operand = &definitions->instruction->operands[(definitions->flags & 3) == 1 ? 0 : 1];
        removable = 1;
        while (uses) {
            read_operand = &uses->instruction->operands[(uses->flags & 3) == 1 ? 0 : 1];
            symbol = defined_operand->value.address->symbol;
            if ((((uses->instruction->opcode >> 8) & 255) == MC_LEA && symbol &&
                 read_operand->value.address->symbol == symbol) ||
                ((read_operand->flags & 15) == OPERAND_INDEXED &&
                 symbol == read_operand->value.address->symbol)) {
                removable = 0;
                distance = 0;
                read_bytes = 0;
                break;
            }
            /* Preserve the native one-based selector passed to this helper. */
            read_bytes = machine_operand_bytes(uses->instruction, uses->flags & 3);
            distance = frame_access_distance(uses, definitions);
            if ((int)(read_bytes + distance) > 0) break;
            uses = uses->next;
        }
        extent = read_bytes + distance;
        if (!uses) {
            distance = 9;
            extent = 0;
            read_operand = 0;
        } else if (distance <= 0) {
            removable = 0;
        } else if (read_operand && defined_operand->value.address->symbol &&
                   read_operand->value.address->symbol == defined_operand->value.address->symbol &&
                   removable) {
            symbol = defined_operand->value.address->symbol;
            for (scan = uses; scan; scan = scan->next) {
                read_operand = &scan->instruction->operands[(scan->flags & 3) == 1 ? 0 : 1];
                if (read_operand->value.address->symbol != symbol) break;
                if ((read_operand->flags & 15) == OPERAND_INDEXED) {
                    removable = 0;
                    break;
                }
            }
        }
        all_moves = 1;
        retained = 0;
        group_end = definitions;
        while (group_end && frame_access_distance(group_end, definitions) == 0) {
            if (removable && !(group_end->flags & 8)) {
                bytes = (group_end->instruction->opcode >> 4) & 15;
                if (distance - (int)bytes < 0) group_end->flags |= 8;
                else if (instruction_has_side_effects(group_end->instruction)) {
                    removable = 0;
                    group_end->flags |= 8;
                }
            }
            if (!removable || (group_end->flags & 8)) {
                operation = (group_end->instruction->opcode >> 8) & 255;
                if (operation != MC_MOV) all_moves = 0;
                bytes = (group_end->instruction->opcode >> 4) & 15;
                if ((int)bytes > (int)extent) extent = bytes;
                retained = 1;
            }
            group_end = group_end->next;
        }
        if (retained) {
            if (all_moves) {
                for (scan = definitions; scan && frame_access_distance(scan, definitions) < (int)extent;
                     scan = scan->next) {
                    if (((scan->instruction->opcode >> 8) & 255) != MC_MOV) {
                        scan->flags |= 8;
                        all_moves = 0;
                        break;
                    }
                }
            }
            if (!all_moves) {
                for (scan = definitions; scan && frame_access_distance(scan, definitions) < (int)extent;
                     scan = scan->next)
                    scan->flags |= 8;
            }
        }
        for (definition = definitions; definition != group_end; definition = definition->next) {
            if (!(definition->flags & 8) && removable && !(definition->flags & 4))
                remove_machine_instruction(definition->instruction);
        }
        definitions = group_end;
    }
}
