/* Ordering and assignment of referenced local frame slots. */
#include "frame-symbols.h"
#include "frame.h"
#include "listing.h"
#include "instruction-rewrite.h"
#include <recon.h>
FrameSymbolReference **frame_symbol_references;

/* Original [000354f0,00035680). Repack referenced frame locals after
   physical coloring. A local head is shared with the counting helper only
   during this call; the native second cleared stack word is never read. */
void __fastcall allocate_frame_symbols(WorkItem *work, int use_stack_pointer)
{
    WorkItem *current;
    MachineInstruction *instruction;
    MachineOperand *operand;
    SymbolHeader *symbol;
    FrameSymbolReference *head, *entry, *following, *first;
    int weight, i;
    unsigned kind;
    for_each_symbol(1, clear_local_frame_assignment);
    if ((int)calculate_stack_frame_size() <= 0) return;
    local_stack_offset = 0;
    head = 0;
    frame_symbol_references = &head;
    for (current = work; current; current = current->next) {
        weight = loop_depth_weight((unsigned)(short)current->region_depth);
        instruction = current->body.block->next;
        for (; (instruction->opcode & 15) != 7; instruction = instruction->next) {
            kind = instruction->opcode & 15;
            if (kind != 0 && kind != 8 && kind != 9) continue;
            for (i = machine_operand_counts[(instruction->opcode >> 8) & 255]; i > 0; --i) {
                operand = &instruction->operands[i == 2 ? 1 : 0];
                kind = operand->flags & 15;
                if ((kind != OPERAND_BASED && kind != OPERAND_INDEXED) ||
                    operand->value.address->base.words.number != REG_EBP) continue;
                symbol = operand->value.address->symbol;
                if (symbol && ((symbol->kind == 1 &&
                    (((BackendSymbol *)symbol)->attributes & 0x70) == 0x10) || symbol->kind == 19))
                    count_frame_symbol_reference(symbol, weight);
            }
        }
    }
    if (use_stack_pointer && *frame_symbol_references) {
        first = *frame_symbol_references;
        entry = first->next;
        first->next = 0;
        while (entry) {
            following = entry->next;
            entry->next = *frame_symbol_references;
            *frame_symbol_references = entry;
            entry = following;
        }
    }
    for (entry = *frame_symbol_references; entry; entry = entry->next) {
        symbol = entry->symbol;
        symbol->field04 = reserve_local_storage(symbol->value);
        if (symbol->kind == 1 && (((BackendSymbol *)symbol)->attributes & 0x70) == 0x10) {
            symbol->field0e |= 0x800;
            ((BackendSymbol *)symbol)->attributes &= ~0x8000U;
        }
    }
}

/* Original [00035740,00035760). A referenced ordinary local will be
   marked assigned again when its new frame offset has been reserved. */
void __fastcall clear_local_frame_assignment(SymbolHeader *symbol)
{
    if (symbol->kind == 1 && (((BackendSymbol *)symbol)->attributes & 0x70) == 0x10)
        symbol->field0e &= ~0x800;
}

/* Original [00035680,00035740). Group frame objects by signed size and
   move an already-seen symbol forward when its accumulated use weight grows.
   New symbols enter after their existing equal-size group. */
void __fastcall count_frame_symbol_reference(SymbolHeader *symbol, int weight)
{
    FrameSymbolReference **link = frame_symbol_references, **group, **position;
    FrameSymbolReference *entry;
    long bytes = symbol->value;
    while (*link && (*link)->bytes < bytes) link = &(*link)->next;
    group = link;
    while (*link && (*link)->bytes == bytes) {
        entry = *link;
        if (entry->symbol == symbol) {
            entry->weight += weight;
            position = group;
            while ((*position)->weight > entry->weight) position = &(*position)->next;
            if (position != link) {
                *link = entry->next;
                entry->next = *position;
                *position = entry;
            }
            return;
        }
        link = &entry->next;
    }
    entry = arena_alloc(3, sizeof(FrameSymbolReference));
    entry->weight = weight;
    entry->bytes = bytes;
    entry->symbol = symbol;
    entry->next = *link;
    *link = entry;
}

/* Original [0002adf0,0002b1b0), including ten dispatch entries2b184..2b1ac.
   Replace fixed EBP frame addresses with ESP plus the tracked stack depth.
   Return1 is part of the original ABI even though its caller discards it. */
int __fastcall rewrite_stack_pointer_frame(MachineInstruction *instructions)
{
    MachineInstruction *instruction, *next, *adjacent, *inserted;
    MachineOperand *operand, frame_operand;
    MachineAddress *address;
    SymbolHeader *symbol;
    unsigned depth = 0, operation, opcode, kind;
    int in_prologue = 0, adjusted, i;
    for (instruction = instructions; instruction; instruction = next) {
        next = instruction->next;
        opcode = instruction->opcode;
        kind = opcode & 15;
        switch (kind) {
        case 0: case 8: case 9:
            operation = (opcode >> 8) & 255;
            for (i = 0; i < machine_operand_counts[operation]; ++i) {
                operand = &instruction->operands[i ? 1 : 0];
                kind = operand->flags & 15;
                if (kind != OPERAND_BASED && kind != OPERAND_INDEXED) continue;
                address = operand->value.address;
                adjusted = 0;
                if (address->base.words.number == REG_EBP) {
                    address->base.words.number = REG_ESP;
                    adjusted = 1;
                }
                if (kind == OPERAND_INDEXED && address->index.words.number == REG_EBP) {
                    address->index.words.number = REG_ESP;
                    adjusted = 1;
                }
                if (adjusted) {
                    address->first.displacement += depth;
                    symbol = address->symbol;
                    if (symbol->kind == 1 && (((BackendSymbol *)symbol)->attributes & 0x70) == 0x30)
                        address->first.displacement -= 4;
                }
                break; /* Only the first addressing operand is inspected. */
            }
            switch (operation) {
            case MC_MOV:
                if (in_prologue && (instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                    instruction->operands[0].value.reg.words.number == REG_EBP)
                    remove_machine_instruction(instruction);
                else if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                    instruction->operands[0].value.reg.words.number == REG_ESP &&
                    (instruction->operands[1].flags & 15) == OPERAND_REGISTER &&
                    instruction->operands[1].value.reg.words.number == REG_EBP) {
                    instruction->opcode = (opcode & ~0xff00U) | (MC_ADD << 8);
                    initialize_operand(&instruction->operands[1], OPERAND_IMMEDIATE, 0, depth, 0);
                    depth = 0;
                }
                break;
            case MC_ADD:
                if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                    instruction->operands[0].value.reg.words.number == REG_ESP)
                    depth -= instruction->operands[1].value.constant;
                break;
            case MC_POP: depth -= 4; break;
            case MC_PUSH: depth += 4; break;
            case MC_CALL: depth -= instruction->operands[1].value.call->argument_bytes; break;
            case MC_SUB:
                if ((instruction->operands[0].flags & 15) == OPERAND_REGISTER &&
                    instruction->operands[0].value.reg.words.number == REG_ESP) {
                    kind = instruction->operands[1].flags & 15;
                    if (kind == OPERAND_IMMEDIATE) depth += instruction->operands[1].value.constant;
                    else if (kind == OPERAND_FRAME_SIZE) depth += resolve_frame_allocation(instruction);
                }
                break;
            case MC_LEAVE:
                instruction->opcode = (opcode & ~0xfff0U) | (MC_ADD << 8) | 0x40;
                initialize_operand(&instruction->operands[0], OPERAND_REGISTER, REG_ESP, 0, 0);
                initialize_operand(&instruction->operands[1], OPERAND_IMMEDIATE, 0, depth, 0);
                depth = 0;
                break;
            case MC_EPOP:
                adjacent = instruction->previous;
                if (frame_register_allocated && ((adjacent->opcode & 15) != 0 ||
                    ((adjacent->opcode >> 8) & 255) != MC_EPOP)) {
                    initialize_operand(&frame_operand, OPERAND_REGISTER, REG_EBP, 0, 0);
                    inserted = new_machine_record(0, MC_EPOP, 0x104, &frame_operand);
                    insert_machine_after(adjacent, inserted);
                    depth -= 4;
                }
                depth -= 4;
                break;
            case MC_EPUSH:
                if (instruction->operands[0].value.reg.words.number == REG_EBP)
                    remove_machine_instruction(instruction);
                else {
                    if (frame_register_allocated && ((next->opcode & 15) != 0 ||
                        ((next->opcode >> 8) & 255) != MC_EPUSH)) {
                        initialize_operand(&frame_operand, OPERAND_REGISTER, REG_EBP, 0, 0);
                        inserted = new_machine_record(0, MC_EPUSH, 0x104, &frame_operand);
                        insert_machine_after(instruction, inserted);
                        depth += 4;
                    }
                    depth += 4;
                }
                break;
            }
            break;
        case 2: in_prologue = 1; break;
        case 3:
            if (depth) compiler_internal_error("@(#)color.c:1.101", 0xe78);
            break;
        case 7: in_prologue = 0; break;
        }
    }
    return 1;
}
