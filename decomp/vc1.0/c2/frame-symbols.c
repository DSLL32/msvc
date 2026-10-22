/* Ordering and assignment of referenced local frame slots. */
#include "frame-symbols.h"
#include "frame.h"
#include "listing.h"
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
